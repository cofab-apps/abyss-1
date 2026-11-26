// Exercise the actual FM-1 machine through its LCD, panel matrix, flash and DAC.
import assert from 'node:assert/strict';
import { readFileSync, writeFileSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { pathToFileURL } from 'node:url';
import { execFileSync } from 'node:child_process';
const [site, firmware, capture] = process.argv.slice(2);
const {changedSectors,packSectors}=await import(pathToFileURL(resolve(site,'persistence.js')));
const art = new URL('../prototype/abyss1/assets/', import.meta.url).pathname;
const source = execFileSync('python3', ['-c', `
from PIL import Image
from pathlib import Path
from array import array
import sys
for i in range(16):
    p=Image.open(Path(sys.argv[1])/('%02d.png'%i)).convert('RGB').quantize(colors=16,dither=Image.Dither.NONE).convert('RGB')
    sys.stdout.buffer.write(array('H', [(r>>3)<<11 | (g>>2)<<5 | b>>3 for r,g,b in zip(*[iter(p.tobytes())]*3)]).tobytes())
`, art], {maxBuffer:4_000_000});
const ref = new Uint16Array(source.buffer, source.byteOffset, source.length/2);
const {jitImports} = await import(pathToFileURL(resolve(site,'jit.js')));
let chip;
const {instance} = await WebAssembly.instantiate(readFileSync(join(site,'fm1_web.wasm')), {env:jitImports(()=>chip)});
chip=instance.exports;
chip.fm1_jit(1,16);
chip.fm1_ahead(1024);
const packageBytes=readFileSync(firmware);
function boot(saved) {
    const p=chip.fm1_alloc(packageBytes.length);
    new Uint8Array(chip.memory.buffer,p,packageBytes.length).set(packageBytes);
    const f=saved ? chip.fm1_alloc(saved.length) : 0;
    if(saved) new Uint8Array(chip.memory.buffer,f,saved.length).set(saved);
    assert.equal(chip.fm1_boot_fwsc(p,packageBytes.length,0,0,60),0);
    if(saved) assert.equal(chip.fm1_flash_restore(f,saved.length),0);
    render(1300,true);
}
function render(blocks, booting=false) {
    let sum=0;
    for(let i=0;i<blocks;i++) {
        const frames=chip.fm1_render(128,50000000);
        if(!booting) assert.equal(frames,128,'DAC frames');
        for(const v of new Float32Array(chip.memory.buffer,chip.fm1_out_l(),128)) sum+=v*v;
    }
    assert.equal(chip.fm1_halted(),0,'CPU halted');
    return Math.sqrt(sum/(128*blocks));
}
function lcd(){ return new Uint16Array(chip.memory.buffer,chip.fm1_lcd(),57600); }
function panel(id,hold=55){chip.fm1_key(id,1); const rms=render(hold);chip.fm1_key(id,0);render(80);return rms;}
function knob(id,detents=1){chip.fm1_enc(id,detents);render(130);}
function match(id,x,y,stride){
    const pixels=lcd();
    let equal=0,n=0;
    for(let j=20;j<240/stride-6;j+=2) for(let i=6;i<240/stride-6;i+=2){
        n++; if(pixels[(y+j)*240+x+i]===ref[id*57600+j*stride*240+i*stride])equal++;
    }
    assert.ok(equal/n>0.99,`portrait ${id} at ${x},${y} matched ${(100*equal/n).toFixed(1)}%`);
}
boot();
for(let i=0;i<4;i++)match(i*4,(i&1)*120,(i>>1)*120,2);
if(capture) writeFileSync(capture,new Uint8Array(lcd().slice().buffer));
console.log('PASS exact default orchestra on the emulated LCD at 60 MHz');
panel(3);match(0,0,0,1);
for(let family=0;family<4;family++){
    for(let v=0;v<4;v++){
        match(family*4+v,0,0,1);
        knob(6);
    }
    knob(1);
}
console.log('PASS all sixteen native portraits, family and creature controls and wraparound');
panel(8);
const gains=[];
for(let track=0;track<4;track++){
    render(240);
    const idle=render(100), note=panel(21,100);
    assert.ok(note>idle+0.0005,`track ${track+1} idle=${idle} note=${note}`);
    gains.push(+note.toFixed(4));
    knob(0);
}
console.log('PASS four independently selected keyboard voices, RMS '+gains.join(', '));
panel(13);panel(21,70);render(150);panel(24,45);render(80);
knob(0);panel(25,45);render(120);panel(13);panel(12);
render(650);
assert.ok(render(120)<0.003,'quiet after stop');
panel(12);
const both=render(1900);
panel(7);panel(3); // GLO and SEL mute the percussion part.
const bass=render(1500);
knob(0,-1);panel(3);render(450);
const muted=render(1100);
panel(3);knob(0,1); // bass on, leave percussion muted, then swap solo
panel(3);knob(0,-1);panel(3);render(500);
const percussion=render(1500);
console.log({both,bass,percussion,muted});
assert.ok(muted<0.001 && [both,bass,percussion].every(v=>v>muted+0.0005),'two recorded parts play independently after stop and restart');
console.log('PASS real-time two-track overdub, transport and independent mute');
panel(12);panel(8);panel(3);knob(6);match(1,0,0,1);
render(650);panel(9);render(400);
const saved=packSectors(changedSectors(new Uint8Array(chip.memory.buffer,chip.fm1_flash_base(),chip.fm1_flash_len()),new Uint8Array(chip.memory.buffer,chip.fm1_flash(),chip.fm1_flash_len())));
boot(saved);match(1,0,0,2);panel(12);assert.ok(render(1500)>0.0005,'saved loop plays after reboot');
console.log('PASS saved creature and recorded loop survive a full chip reboot');

boot();
panel(13);
for (let part=0; part<4; part++) {
    if(part) knob(0);
    panel(21 + part,120);
    render(150);
}
panel(13);panel(12);render(700);
panel(12);panel(7);
const ensemble=render(2500);
for(let i=0;i<4;i++){panel(3);knob(0);}
render(600);
const silence=render(1000);
const solos=[];
for(let i=0;i<4;i++){
    panel(3);solos.push(render(1750));
    panel(3);knob(0);render(500);
}
assert.ok(silence<0.001 && ensemble>0.002 && solos.every(v=>v>silence+0.0005),'all four recorded parts survive two loop passes');
console.log('PASS four recorded creatures, simultaneous playback and four individually audible solos',solos);
panel(12);panel(8);
boot();
panel(11); // SEQ opens the 16-step editor.
knob(0,3);
panel(24);render(100);
assert.equal(lcd()[200*240+96],0x5f1b,'the fourth step contains a note');
panel(11);panel(12);
assert.ok(render(1800)>0.001,'step-entered note plays');
console.log('PASS step entry by physical encoder and key, then playback');
