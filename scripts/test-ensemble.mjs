// Read only the simulated panel, LCD, flash and DAC of the packaged target.
import assert from 'node:assert/strict';
import {readFileSync,writeFileSync} from 'node:fs';
import {join} from 'node:path';
import {changedSectors,packSectors} from '../simulator/persistence.js';

export function checkEnsemble({boot,chip,run,enc,btn,shots,screen,out,movieStart,movieEnd}) {
    const key=(k,down)=>chip.fm1_key(14+k,Number(down));
    const hold=id=>{chip.fm1_key(id,1);run(255);chip.fm1_key(id,0);run(80);};
    const tap=k=>{key(k,true);run(38);key(k,false);run(32);};
    const saved=()=>{
        const snap=p=>new Uint8Array(chip.memory.buffer,p,chip.fm1_flash_len()).slice();
        return changedSectors(snap(chip.fm1_flash_base()),snap(chip.fm1_flash()));
    };
    const meter=a=>Math.sqrt(a.reduce((sum,v)=>sum+v*v,0)/a.length);
    const notes=()=>[0,1,2,3].map(i=>{
        const x=8+(i%2)*119+91,y=19+(i>>1)*91;
        return screen()[240*(y+58)+x]!==screen()[240*(y+22)+x]?1:0;
    });
    const pad=[21,23,24,26];
    const summary={};
    boot(readFileSync(new URL('../evidence/choir/four-lanes-2.1.0.bin',import.meta.url)));
    btn(8);hold(6);run(150);
    const late=chip.fm1_starved();
    movieStart('strum');
    for(const seq of [[0,1,2,3],[3,2,1,0]]) {
        key(0,true);
        const root=run(120);assert.ok(root<.001,'root alone is silent');
        let mask=[0,0,0,0];
        for(const i of seq) {
            tap(pad[i]);const level=run(65);mask[i]=1;
            assert.deepEqual(notes(),mask,'only the strummed seats are latched');
            assert.ok(level>.002,'launched voice reaches the DAC');
        }
        shots('all-strummed');key(0,false);run(250);
        assert.deepEqual(notes(),[0,0,0,0],'root release releases all seats');
    }
    movieEnd();summary.strum={orders:['ABCD','DCBA'],starved:chip.fm1_starved()-late};
    assert.equal(summary.strum.starved,0);
    boot();btn(8);hold(6);key(0,true);tap(pad[2]);assert.ok(run(65)<.001,'empty seat is silent');
    key(0,false);run(100);
    // A captured Crab becomes the lower-row Bloop. Collect the direct PCM
    // reference and compare that same clip against the DAC during its answer.
    boot();btn(10);btn(12);btn(12); // leave the game, then start ordinary loop playback
    enc(0,2);btn(11);enc(1,-2);btn(13);key(7,true);run(170);key(7,false);btn(13);
    run(170);key(0,true);const reference=run(110,true);key(0,false);run(220);
    btn(9);shots('guest-seat');btn(10);
    movieStart('guest-reply');
    const t=chip.fm1_ms();key(0,true);run(140);key(0,false);
    while(chip.fm1_ms()-t<2320) run(2);
    const reply=run(190,true);movieEnd();
    const refStart=reference.findIndex(x=>Math.abs(x)>.005)+200;
    let match=-1;
    for(let ofs=0;ofs<reply.length-1400;ofs+=2) {
        let xy=0,xx=0,yy=0;
        for(let k=0;k<1200;k+=8) {
            const a=reference[refStart+k], b=reply[ofs+k];xy+=a*b;xx+=a*a;yy+=b*b;
        }
        if(xx*yy>0) match=Math.max(match,xy/Math.sqrt(xx*yy));
    }
    console.log('guest reply',match,meter(reference),meter(reply));
    assert.ok(match>.9 && meter(reply)>.002,'reply reproduces the recorded PCM');
    run(350);btn(12);btn(9);
    const savedGuest=saved();boot(packSectors(savedGuest));btn(8);shots('guest-missing');
    chip.fm1_key(12,1);run(15);chip.fm1_key(12,0);
    const restoredCaller=run(85);run(760);const missingReply=run(110);
    assert.ok(restoredCaller>.003 && missingReply<.001,'reboot retains call and mutes missing guest');
    summary.guest={correlation:match,replyRms:meter(reply),restoredCaller,missingReply};

    // Compare the full pitched guest clip at two roots with per-seat echo
    // turned down. The four synth parts start independently in the next trial.
    boot();btn(8);enc(5,-24);btn(11);btn(9);
    function clipDuration(k) {
        key(k,true);const pcm=run(240,true);key(k,false);run(180);
        const first=pcm.findIndex(v=>Math.abs(v)>.002);
        const last=pcm.findLastIndex(v=>Math.abs(v)>.002);
        assert.ok(first>=0 && last>first);
        return last-first;
    }
    const low=clipDuration(0), high=clipDuration(9);
    assert.ok(high/low>.46 && high/low<.55,'one octave doubles guest playback speed');
    summary.guest.pitchDurationSamples=[low,high];

    boot();btn(8);for(let i=0;i<3;i++) btn(3);
    hold(6);
    function isolatedLevels() {
        const levels=[];
        for(const k of pad) {
            key(0,true);run(30);key(k,true);run(35);
            levels.push(run(70));key(k,false);key(0,false);run(220);
        }
        return levels;
    }
    const before=isolatedLevels();enc(0,-1);btn(11);btn(9);hold(6);
    const after=isolatedLevels();
    for(const i of [0,1,3]) {
        assert.ok(before[i]>.002 && Math.abs(after[i]/before[i]-1)<.08,'other singers keep their voice levels');
    }
    assert.ok(Math.abs(after[2]/before[2]-1)>.12,'guest changes only the chosen seat');
    summary.guest.isolatedSeatRms={before,after};

    // Three recognizably different recordings. Each keeps its own first pitch.
    boot();btn(8);
    for(let i=0;i<3;i++) {
        btn(13);key([0,7,14][i],true);run(145);key([0,7,14][i],false);
        run(160);btn(12);hold(7);if(i) enc(0,1);
        btn(9);btn(7); // store then close without loading another working song
    }
    hold(7);btn(13);for(const k of [4,0,2]) tap(k);
    const sectors=saved();
    function storedFirst(i) {
        const b=Buffer.from(sectors.find(s=>s.at===0xd8000+i*8192).data);
        const off=b.indexOf('MCK1');assert.ok(off>=0);return b[off+9];
    }
    const pitches=[0,1,2].map(storedFirst);
    assert.deepEqual(pitches,[48,57,67],'independent recorded songs persisted');
    boot(packSectors(sectors));btn(8);hold(7);
    const colorAt=(x,y)=>screen()[y*240+x];
    function playing(){
        const bg=colorAt(0,104);
        return [12,91,170].findIndex(x=>colorAt(x,104)!==bg);
    }
    // Preview C, exit, play the restored working song and measure its DAC.
    tap(4);assert.equal(playing(),2);btn(7);run(240);
    chip.fm1_key(12,1);run(12);chip.fm1_key(12,0);
    const workingAudio=run(70,true), previewReturn=meter(workingAudio);assert.ok(previewReturn>.003);
    btn(12);hold(7);movieStart('shell-order');
    chip.fm1_key(12,1);run(12);chip.fm1_key(12,0);
    const start=chip.fm1_ms();const indices=[],levels=[];
    for(let i=0;i<3;i++) {
        while(chip.fm1_ms()-start<i*4800+90) run(3);
        indices.push(playing());levels.push(run(55));shots('chain-'+i);
    }
    while(chip.fm1_ms()-start<14550) run(4);
    movieEnd();assert.deepEqual(indices,[2,0,1]);assert.equal(playing(),-1);
    assert.ok(levels.every(x=>x>.003),'all three songs reach DAC at the chain boundary');
    const savedAfter=saved();
    assert.deepEqual(savedAfter,sectors,'previews and chain playback never rewrite flash');
    // Preview B again and return to A. A save in C should contain the
    // original A phrase. Inspect whichever C mirror contains this new write.
    tap(2);btn(7);hold(7);enc(0,1);btn(9);
    const copies=saved().filter(s=>s.at===0xdc000 || s.at===0xdd000);
    assert.ok(copies.some(s=>{const b=Buffer.from(s.data), n=b.indexOf('MCK1');
        return n>=0 && b[n+9]===48;}),'preview and playback restore the working song');
    summary.shells={pitches,indices,levels,previewReturn,writtenSectors:sectors.map(s=>s.at)};
    console.log(summary);
    writeFileSync(join(out,'ensemble-results.json'),JSON.stringify(summary,null,2)+'\n');
}
