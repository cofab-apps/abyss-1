// Args: path to the emulator web directory and to the .fwsc package.
import { readFileSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { pathToFileURL } from 'node:url';
const [site, firmware] = process.argv.slice(2);
const { jitImports } = await import(pathToFileURL(resolve(site, 'jit.js')));
const wasm = readFileSync(join(site, 'fm1_web.wasm'));
const bytes = readFileSync(firmware);
for (let channel = 0; channel < 4; channel++) {
  let chip;
  const { instance } = await WebAssembly.instantiate(wasm, {env: jitImports(() => chip)});
  chip = instance.exports;
  chip.fm1_jit(1, 16);
  chip.fm1_ahead(1024);
  const p = chip.fm1_alloc(bytes.length);
  new Uint8Array(chip.memory.buffer, p, bytes.length).set(bytes);
  if (chip.fm1_boot_fwsc(p, bytes.length, 0, 0, 100)) throw Error('Boot failed');
  function render(blocks) {
    let energy = 0, frames = 0;
    for (let i = 0; i < blocks; i++) {
      frames += chip.fm1_render(128, 50000000);
      for (const sample of new Float32Array(chip.memory.buffer, chip.fm1_out_l(), 128)) energy += sample * sample;
    }
    if (chip.fm1_halted() || frames < blocks * 120) throw Error('Audio underrun or CPU halted');
    return Math.sqrt(energy / (blocks * 128));
  }
  render(1100);
  const idle = render(160);
  for (const b of [0x90 + channel, 60, 100]) chip.fm1_midi(b);
  const note = render(200);
  for (const b of [0x80 + channel, 60, 0]) chip.fm1_midi(b);
  console.log(JSON.stringify({channel:channel + 1, idleRms:idle, noteRms:note}));
  if (!(note > idle * 10 + 0.001)) process.exitCode = 1;
}
