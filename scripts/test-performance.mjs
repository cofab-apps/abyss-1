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
const {changedSectors, packSectors} = await import(pathToFileURL(path.resolve(site, 'persistence.js')));
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
let movie=false, count=0, videoFrames=0, video, audio;
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
            if(videoFrames*44100<count*10){fs.writeSync(video,rawScreen());videoFrames++;}
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
function boot(saved, bytes = file) {
    const at = chipInput(bytes);
    const flash = saved ? chipInput(saved) : 0;
    assert.equal(chip.fm1_boot_fwsc(at, bytes.length, 0, 0, clock), 0);
    if (saved) assert.equal(chip.fm1_flash_restore(flash, saved.length), 0);
    run(1300, false, true);
}
function saveAndReboot() {
    run(950);
    btn(9);
    run(900);
    const len = chip.fm1_flash_len();
    const base = new Uint8Array(chip.memory.buffer, chip.fm1_flash_base(), len);
    const current = new Uint8Array(chip.memory.buffer, chip.fm1_flash(), len);
    boot(packSectors(changedSectors(base, current)));
}
function btn(id){chip.fm1_key(id,1);run(50);chip.fm1_key(id,0);run(70);}
function enc(id,d){chip.fm1_enc(id,d);run(105);}
function cell(i){return screen()[(200+Math.floor(i/8)*21)*240+6+30*(i%8)];}
function pattern(){
    return Array.from({length:16},(_,i)=>cell(i)===0x5f1b?i:-1).filter(i=>i>=0);
}
if (process.argv[5] === '--reef' || process.argv[5] === '--probe-shell' || process.argv[5] === '--showcase') {
    const {checkReef, probeShell, showcase} = await import('./reef-check.mjs');
    const movieStart = (name = "quartet") => {
        audio=fs.openSync(path.join(out,name+'.s16'),'w');
        video=fs.openSync(path.join(out,name+'.rgb'),'w');
        movie=true;
    };
    const movieEnd = () => {movie=false;fs.closeSync(audio);fs.closeSync(video);};
    const rig = {boot,btn,enc,run,chip,shots,screen,pattern,out,movieStart,movieEnd,saveAndReboot, time: () => count/44100};
    if (process.argv[5] === "--showcase") {showcase(rig);process.exit(0);}
    if (process.argv[5] === '--reef') checkReef(rig);
    probeShell(rig);
    process.exit(0);
}
boot();btn(11);
assert.deepEqual(pattern(),[]);
shots('sequencer-empty');
btn(12);assert.ok(run(1600)>.001,'demo plays');
btn(12);
const expected=[[0,3,6,10,12],[0,2,4,7,8,10,12,15],[2,5,9,14],[1,9]];
for(let i=0;i<4;i++){
    // Tied steps may appear occupied. All authored attacks must exist.
    const notes=pattern();
    assert.ok(expected[i].every(s=>notes.includes(s)),JSON.stringify({part:i+1,notes}));
    btn(8);enc(0,1);btn(11);
}
btn(12);btn(7);
for(let i=0;i<4;i++){btn(3);enc(0,1);}
run(600);const muted=run(1300);
const solo=[];
for(let i=0;i<4;i++){btn(3);solo.push(run(1800));btn(3);enc(0,1);run(600);}
assert.ok(muted<.0003 && solo.every(v=>v>muted+.0004),JSON.stringify({muted,solo}));
console.log('PASS SEQ/PLAY loads the authored four-track pattern; muted and isolated RMS',muted,solo);

boot();btn(11);btn(12);run(400);btn(12);
saveAndReboot();
btn(11);
for (let i = 0; i < 4; i++) {
    assert.ok(expected[i].every(step => pattern().includes(step)));
    btn(8);enc(0,1);btn(11);
}
btn(12);
assert.ok(run(1500) > .001);
console.log('PASS SAVE retains the four-part demo across a full emulator reboot.');

boot();enc(0,2);btn(11);enc(0,3);btn(24);
saveAndReboot();
btn(11);btn(12);run(400);btn(12);
for (let i = 0; i < 4; i++) {
    assert.deepEqual(pattern(), i === 0 ? [3] : []);
    btn(8);enc(0,1);btn(11);
}
console.log('PASS a restored user phrase on track 3 takes priority over the demo, keeping the other three tracks empty.');

function noteProbe(effect, setting=-1, change=0, target=2) {
    boot();enc(0,target);
    if(effect)btn(effect);else run(120);
    if(setting>=0)enc(2+setting,change);else run(105);
    chip.fm1_key(21,1);const held=run(260,true);
    chip.fm1_key(21,0);const tail=run(230,true);
    return Float32Array.from([...held,...tail]);
}
const distance=(a,b)=>Math.sqrt(a.reduce((sum,v,i)=>sum+(v-b[i])**2,0)/a.length);
const dry=noteProbe(0);
for(const [name,id] of [['LFO',5],['FX',2]]) {
    const engaged=noteProbe(id),same=noteProbe(id);
    const delta=distance(dry,engaged),repeat=distance(same,engaged);
    assert.ok(delta>repeat+.001,name+' engage: '+delta);
    const knobs=[];
    for(let k=0;k<4;k++){
        const low=noteProbe(id,k,-36),high=noteProbe(id,k,36);
        const d=distance(low,high);knobs.push(d);
        assert.ok(d>repeat+.0001,name+' K'+(k+1)+': '+d);
    }
    console.log('PASS '+name+' changes held Julia and its release. Difference from dry = '+delta+', repeat = '+repeat+', K1–K4 = '+knobs);
}

boot();
audio=fs.openSync(path.join(out,'demo.s16'),'w');
video=fs.openSync(path.join(out,'demo.rgb'),'w');
const scenes=[];
const scene=title=>scenes.push([count/44100,title]);
movie=true;
scene('SEQ / PLAY on an empty song');btn(11);run(250);btn(12);run(650);
scene('HOME / four creatures playing');btn(8);run(760);
scene('SELECT / Julia on track 3');enc(0,2);btn(3);shots('portrait');run(470);
scene('LFO / pitch swing on Julia');btn(5);shots('lfo');run(760);
scene('K3 / stepped random wave');enc(4,2);run(750);
scene('FX / echo, drive and chorus');btn(2);shots('fx');run(810);
scene('FX off / LFO still running');btn(2);run(310);btn(5);btn(5);
scene('HOME / modulation bypassed');btn(8);run(600);
movie=false;fs.closeSync(audio);fs.closeSync(video);
fs.writeFileSync(path.join(out,'scenes.json'),JSON.stringify({scenes,duration:count/44100}));
console.log('Captured stereo DAC and LCD:',count,'audio frames,',videoFrames,'LCD frames');
