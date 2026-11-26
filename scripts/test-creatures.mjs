// Exercise the actual FM-1 machine through its LCD, panel matrix, flash and DAC.
import assert from 'node:assert/strict';
import { readFileSync, writeFileSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { pathToFileURL } from 'node:url';
import { execFileSync } from 'node:child_process';
const [site, firmware, capture] = process.argv.slice(2);
const {changedSectors,packSectors}=await import(pathToFileURL(resolve(site,'persistence.js')));
const captureDirectory = process.env.ABYSS_CAPTURE_DIR;
const clockMHz = Number(process.env.ABYSS_MHZ || 60);
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
if(process.env.ABYSS_NEW_FLASH) chip.fm1_flash_part(1);
const packageBytes=readFileSync(firmware);
function boot(saved, bytes=packageBytes) {
    const p=chip.fm1_alloc(bytes.length);
    new Uint8Array(chip.memory.buffer,p,bytes.length).set(bytes);
    const f=saved ? chip.fm1_alloc(saved.length) : 0;
    if(saved) new Uint8Array(chip.memory.buffer,f,saved.length).set(saved);
    assert.equal(chip.fm1_boot_fwsc(p,bytes.length,0,0,clockMHz),0);
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
function captureLcd(name) {
    if(captureDirectory) writeFileSync(join(captureDirectory,name+'.rgb'),new Uint8Array(lcd().slice().buffer));
}
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
captureLcd('orchestra');
console.log('PASS exact default orchestra on the emulated LCD at '+clockMHz+' MHz');
panel(3);match(0,0,0,1);
for(let family=0;family<4;family++){
    for(let v=0;v<4;v++){
        match(family*4+v,0,0,1);
        captureLcd('portrait-'+String(family*4+v).padStart(2,'0'));
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
captureLcd('mix-playing');
const seconds=Number(process.env.ABYSS_SOAK_SECONDS || 0);
if (seconds > 0) {
    const start=chip.fm1_ms(), starved=chip.fm1_starved();
    let blocks=0, lowest=1, highest=0, peak=0;
    const wall=performance.now();
    while(chip.fm1_ms()-start < seconds*1000) {
        const rms=render(128);
        lowest=Math.min(lowest,rms);
        highest=Math.max(highest,rms);
        for (const sample of new Float32Array(chip.memory.buffer,chip.fm1_out_l(),128))
            peak=Math.max(peak,Math.abs(sample));
        blocks+=128;
    }
    const elapsed=chip.fm1_ms()-start;
    console.log(JSON.stringify({clockMHz,emulatedMs:elapsed,wallSeconds:(performance.now()-wall)/1000,
        frames:blocks*128,shortDacBlocks:0,starvationDelta:chip.fm1_starved()-starved,cpu1Fault:chip.fm1_cpu1_fault(),
        lowestWindowRms:lowest,highestWindowRms:highest,sampledPeak:peak}));
    assert.equal(chip.fm1_cpu1_fault(),0,'second core fault');
    assert.equal(chip.fm1_starved()-starved,0,'DAC starvation during the soak');
}

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

panel(12);panel(11);
captureLcd('steps');
panel(6); // EDIT erases the selected step.
assert.equal(lcd()[200*240+96],0x2946,'the fourth step is empty');
panel(11);panel(12);
assert.ok(render(1800)<0.001,'erased note is silent on replay');
console.log('PASS selected-step erase');

function sendMidi(...bytes) { for(const byte of bytes) chip.fm1_midi(byte); }
function voiceProbe(velocity,pressure) {
    boot();
    sendMidi(0x92,60,velocity);
    render(130);
    sendMidi(0xD2,pressure);
    render(40);
    const samples = [];
    for(let i=0;i<45;i++) {
        render(1);
        samples.push(...new Float32Array(chip.memory.buffer,chip.fm1_out_l(),128));
    }
    return samples;
}
function rms(samples){return Math.sqrt(samples.reduce((s,v)=>s+v*v,0)/samples.length);}
const held=voiceProbe(85,0), repeat=voiceProbe(85,0), pressure=voiceProbe(85,120);
const difference=(a,b)=>rms(a.map((v,i)=>v-b[i]));
const untouched=difference(held,repeat), pressed=difference(held,pressure);
assert.ok(pressed > untouched*3+0.001,'aftertouch changes the sustained Julia voice');
const soft=rms(voiceProbe(38,0)), firm=rms(voiceProbe(105,0));
assert.ok(firm>soft*1.5,'MIDI note-on velocity changes note level');
console.log('PASS held-note channel aftertouch and velocity',{untouched,pressed,soft,firm});

if (captureDirectory) {
    const previous = new URL('../builds/four-part-formant.fwsc',import.meta.url);
    boot(undefined,readFileSync(previous));
    captureLcd('optimist-previous');
}

if(process.env.ABYSS_ALL_ASSIGNMENTS) {
    boot();
    panel(3);
    for(let track=0;track<4;track++) {
        for(let family=0;family<4;family++) {
            for(let v=0;v<4;v++) {
                const id=((track+family)&3)*4+v;
                match(id,0,0,1);
                render(550); // let the selection preview and its room tail finish
                const silence=render(100), note=panel(21,110);
                assert.ok(note>silence+0.0004,`track ${track+1}, creature ${id}, idle ${silence}, note ${note}`);
                knob(6);
            }
            knob(1);
        }
        knob(0);
    }
    console.log('PASS all 64 combinations of four tracks and sixteen creatures: portrait and audible key');
}
