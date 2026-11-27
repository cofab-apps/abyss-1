import assert from 'node:assert/strict';
import {writeFileSync} from 'node:fs';
import {join} from 'node:path';
import {changedSectors,packSectors} from '../simulator/persistence.js';

export async function checkChoir({boot,chip,run,btn,enc,shots,out,chipInput,movieStart,movieEnd}) {
    const key=(k,on)=>chip.fm1_key(k+14,Number(on));
    const snapshot=ptr=>new Uint8Array(chip.memory.buffer,ptr,chip.fm1_flash_len()).slice();
    const script=s=>{const bytes=new TextEncoder().encode(s);
        assert.equal(chip.fm1_bench_script(chipInput(bytes),bytes.length),0);};
    boot();
    btn(10); // ARP from the solo screen. Hydra asks, Bloop replies.
    shots('duet-ready');
    movieStart('duet');
    key(7,true); const call=run(120); key(7,false);
    run(450); const silence=run(100);
    run(180); const reply=run(90);
    shots('duet-reply');
    run(300); const rest=run(80);
    movieEnd();
    console.log({call,silence,reply,rest});
    assert.ok(call>.005 && reply>.005 && silence<.001 && rest<.001,'one-bar reply with rests');
    btn(12); btn(9); run(180);
    const sectors=changedSectors(snapshot(chip.fm1_flash_base()),snapshot(chip.fm1_flash()));
    assert.ok(sectors.some(s=>[0xd8000,0xd9000].includes(s.at)),'favourite wrote its sector');
    boot(packSectors(sectors));
    btn(8); shots('restored-seats');
    chip.fm1_key(12,1); run(12); chip.fm1_key(12,0);
    const restoredCall=run(90); run(730); const restoredReply=run(90);
    assert.ok(restoredCall>.005 && restoredReply>.005,'both phrases survive reboot');
    btn(12);
    // Add two more seats. Each uses the confirmed auditioned species.
    for(const [seat,creature] of [[2,1],[1,6]]) {enc(0,seat);enc(1,creature);btn(3);}
    run(200);
    const missed=chip.fm1_starved();key(7,true);const chord=run(150);shots('four-singers');
    key(7,false);run(140);
    const starved=chip.fm1_starved()-missed;
    assert.ok(chord>.01 && starved===0,'full choir and audio deadline');
    // Record a third voice, then overdub the fourth while transport is running.
    // Compare the actual favourite payload with the two lanes captured before reboot.
    function readLanes(raw) {
        return Array.from({length:4},(_,i)=>({id:raw[i*261],
            notes:Array.from({length:32},(_,j)=>raw[i*261+5+j*8]),
            bends:Array.from({length:32},(_,j)=>raw[i*261+6+j*8]),
            mouths:Array.from({length:32},(_,j)=>raw[i*261+9+j*8])}));
    }
    // Select seat 3 (index 2) from seat 4, and capture a phrase immediately.
    enc(0,-1);btn(13);key(4,true);run(120);key(4,false);
    run(1600);enc(0,1);btn(13);run(1500);
    key(7,true);run(90);key(8,true);enc(2,4);run(90);
    key(8,false);key(7,false);run(1500);
    btn(12);btn(9);run(200);
    // The platform alternates A/B copies. Read the newest through a reboot and
    // GLO recall, then use its contents in both mirrors to identify the new pass.
    const flash=Buffer.from(snapshot(chip.fm1_flash()));
    const copies=[0xd8000,0xd9000].map(start=>{
        const at=flash.subarray(start,start+4096).indexOf('MCK1');
        return at<0?null:readLanes(flash.subarray(start+at+4));
    }).filter(Boolean);
    const lanes=copies.find(c=>c.every(l=>l.id<20 && l.notes.some(Boolean)));
    assert.ok(lanes,'four independent recorded lines in saved favourite');
    assert.ok(lanes.every(l=>l.notes.some(n=>n===0)),'each lane retained its rests');
    assert.equal(new Set(lanes.map(l=>l.id)).size,4,'four different portraits');
    assert.deepEqual(lanes[0].notes.slice(0,16),lanes[1].notes.slice(16),'answer repeats caller rhythm and pitches');
    assert.ok(lanes[3].bends.includes(1) && !lanes[2].bends.includes(1),'bend belongs to the fourth seat');
    const mouths=lanes[3].mouths.filter((v,j)=>lanes[3].notes[j]);
    assert.ok(new Set(mouths).size>=2,'fourth seat recorded mouth movement');
    btn(6); // Undo the last recorded lane and save it into the alternating copy.
    btn(9);run(150);
    const after=Buffer.from(snapshot(chip.fm1_flash()));
    const loc=after.subarray(0xd8000,0xd9000).indexOf('MCK1');
    const undone=readLanes(after.subarray(0xd8000+loc+4));
    assert.deepEqual(undone.slice(0,3),lanes.slice(0,3),'undo preserved the first three phrases');
    assert.ok(undone[3].notes.every(n=>!n),'undo restored the previously empty fourth phrase');
    btn(7); // GLO recalls while stopped. The four portraits return.
    shots('four-part-favourite');
    function pressureTrial(force) {
        boot();btn(4);const t=chip.fm1_ms();
        script((t+15)+':21@32,'+(t+1000)+':21~'+force);
        run(520);const level=run(160);shots('force-'+force);return level;
    }
    const light=pressureTrial(32), firm=pressureTrial(126);
    console.log({light,firm});
    assert.ok(light>.001 && Math.abs(firm-light)>.001,'held pressure changes the DAC');
    const results={call,silence,reply,rest,restoredCall,restoredReply,chord,starved,light,firm,
        savedLines:lanes,writtenSectors:sectors.map(s=>s.at)};
    writeFileSync(join(out,'choir-results.json'),JSON.stringify(results,null,2)+'\n');
    console.log('PASS',results);
}
