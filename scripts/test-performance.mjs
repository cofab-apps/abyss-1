// Run the packaged FM-1 firmware, capture its native screen and stereo DAC.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
const [site,fw,out] = process.argv.slice(2);
const clock = Number(process.env.ABYSS_MHZ || 60);
fs.mkdirSync(out, {recursive:true});
const {jitImports} = await import(pathToFileURL(path.resolve(site,'jit.js')));
let chip;
const mod = await WebAssembly.instantiate(fs.readFileSync(path.join(site,'fm1_web.wasm')), {env:jitImports(()=>chip)});
chip=mod.instance.exports;
chip.fm1_jit(1,16); chip.fm1_ahead(1024);
const file=fs.readFileSync(fw);
const hash = data => createHash('sha256').update(data).digest('hex');
console.log('Firmware SHA-256',hash(file));
const screen=()=>new Uint16Array(chip.memory.buffer,chip.fm1_lcd(),57600);
const rawScreen=()=>Buffer.from(new Uint8Array(screen().buffer,screen().byteOffset,115200));
const shots=name=>fs.writeFileSync(path.join(out,name+'.rgb'),rawScreen());
// Use the same panel renderer as the browser, with the real LCD and scanned LEDs.
function panelFrame(levels) {
    const lcd=Uint16Array.from(screen());
    const ptr=chip.panel_lcd();
    new Uint16Array(chip.memory.buffer,ptr,240*240).set(lcd);
    new Float32Array(chip.memory.buffer,chip.panel_leds(),41).set(levels);
    const pixels=chip.panel_draw();
    return Buffer.from(chip.memory.buffer,pixels,chip.panel_width()*chip.panel_height()*4);
}
let movie=false, count=0, videoFrames=0, video, audio, panelVideo;
function run(blocks, capture=false, booting=false) {
    const samples=capture?new Float32Array(blocks*128):null;
    let power=0;
    for(let i=0;i<blocks;i++) {
        const n=chip.fm1_render(128,50000000);
        if(!booting)assert.equal(n,128,'short DAC block');
        const left=new Float32Array(chip.memory.buffer,chip.fm1_out_l(),128);
        if(capture)samples.set(left,128*i);
        for(const v of left)power+=v*v;
        if(movie) {
            const right=new Float32Array(chip.memory.buffer,chip.fm1_out_r(),128);
            const stereo=new Int16Array(256);
            for(let j=0;j<128;j++) {
                stereo[2*j]=Math.round(Math.max(-1,Math.min(1,left[j]))*32767);
                stereo[2*j+1]=Math.round(Math.max(-1,Math.min(1,right[j]))*32767);
            }
            fs.writeSync(audio,Buffer.from(stereo.buffer)); count+=128;
            if(videoFrames*44100<count*10) {
                fs.writeSync(video,rawScreen());
                if(panelVideo!==undefined) {
                    const levels=Float32Array.from(new Float32Array(chip.memory.buffer,chip.fm1_leds(),41));
                    fs.writeSync(panelVideo,panelFrame(levels));
                }
                videoFrames++;
            }
        }
    }
    assert.equal(chip.fm1_halted(),0);
    return capture?samples:Math.sqrt(power/(128*blocks));
}
function chipInput(bytes) {
    const at = chip.fm1_alloc(bytes.length);
    new Uint8Array(chip.memory.buffer, at, bytes.length).set(bytes);
    return at;
}
function boot() {
    const at = chipInput(file);
    assert.equal(chip.fm1_boot_fwsc(at, file.length, 0, 0, clock), 0);
    run(1300, false, true);
}
function btn(id){chip.fm1_key(id,1);run(50);chip.fm1_key(id,0);run(70);}
function enc(id,d){chip.fm1_enc(id,d);run(105);}

function movieStart(name) {
    audio=fs.openSync(path.join(out,name+'.s16'),'w');
    video=fs.openSync(path.join(out,name+'.rgb'),'w');
    if(process.env.ABYSS_PANEL_FILM) panelVideo=fs.openSync(path.join(out,name+'-panel.rgba'),'w');
    movie=true;
}
function movieEnd() {
    movie=false;
    fs.closeSync(audio);
    fs.closeSync(video);
    if(panelVideo!==undefined) fs.closeSync(panelVideo);
}
const {checkMonsters} = await import('./test-monsters.mjs');
checkMonsters({boot, chip, run, enc, btn, shots, screen, out, movieStart, movieEnd});
