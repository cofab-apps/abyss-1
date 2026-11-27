import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {readFileSync,writeFileSync} from 'node:fs';
import {join} from 'node:path';

export function checkRows({boot,chip,run,enc,btn,shots,screen,out,movieStart,movieEnd}) {
    const key=(id,down)=>chip.fm1_key(14+id,Number(down));
    function portrait(seat) {
        const lcd=screen(), x=8+seat%2*119, y=19+Math.floor(seat/2)*91;
        const h=createHash('sha256');
        for(let j=0;j<84;j++) h.update(lcd.subarray((y+j)*240+x,(y+j)*240+x+84));
        return h.digest('hex');
    }
    boot();btn(8);enc(0,2);
    const empty=portrait(2); enc(1,6); const chosen=portrait(2);
    enc(0,1);
    const stayed=chosen===portrait(2) && chosen!==empty;
    console.log('Third seat retained after moving to fourth:',stayed);
    // Load a real 2.1.0 favourite containing notes on all four seats.
    boot(readFileSync(new URL('../evidence/choir/four-lanes-2.1.0.bin',import.meta.url)));
    btn(8); const before=Array.from({length:4},(_,i)=>portrait(i));
    assert.equal(new Set(before).size,4);
    btn(10);
    // Each half-bar is 1.2 seconds. Sample the middle of each sounding slot,
    // then a rest well past the short default echo.
    const parts=[], gaps=[], markers=[];
    movieStart('rows');
    const start=chip.fm1_ms();
    function until(ms) {
        while(chip.fm1_ms()-start<ms) run(3);
    }
    for(let slot=0;slot<4;slot++) {
        until(slot*1200+(slot?95:0));
        if(slot<2) key(slot?11:0,true);
        parts.push(run(80));
        shots('turn-'+slot);
        const lcd=screen();
        const dots=Array.from({length:4},(_,i)=>lcd[(19+Math.floor(i/2)*91+36)*240+8+(i%2)*119+92]);
        const background=dots.find(c=>dots.filter(p=>p===c).length>=3);
        markers.push(dots.findIndex(c=>c!==background));
        if(slot<2) key(slot?11:0,false);
        until(slot*1200+920);
        gaps.push(run(45));
    }
    movieEnd();
    btn(12);btn(9);run(150);
    const raw=Buffer.from(new Uint8Array(chip.memory.buffer,chip.fm1_flash(),chip.fm1_flash_len()));
    const laneNotes=(bytes,i)=>Array.from({length:32},(_,j)=>bytes[5+i*261+j*8]);
    const copies=[0xd8000,0xd9000].map(base=>{
        const area=raw.subarray(base,base+4096), loc=area.indexOf('MCK1');
        return loc<0?null:Array.from({length:4},(_,i)=>laneNotes(area.subarray(loc+4),i));
    }).filter(Boolean);
    const active=notes=>notes.flatMap((n,i)=>n?[i]:[]);
    const rows=copies.find(lines=>lines.every((notes,i)=>notes.some(Boolean) &&
        active(notes).every(j=>Math.floor(j/8)===i)));
    console.log({parts,gaps,rows:rows?.map(active)});
    assert.ok(stayed,'browsed portrait remains when leaving seat');
    assert.deepEqual(markers,[0,1,2,3],'LCD marker follows both rows');
    assert.ok(rows,'four saved lanes occupy four distinct half-bars');
    assert.deepEqual(rows[0].slice(0,8),rows[2].slice(16,24));
    assert.deepEqual(rows[1].slice(8,16),rows[3].slice(24));
    assert.ok(parts.every(n=>n>.002) && gaps.every(n=>n<.001),'four audible turns with clean rests');
    btn(6);btn(9);run(150); // undo must restore every original lane
    const flash=()=>new Uint8Array(chip.memory.buffer,chip.fm1_flash(),chip.fm1_flash_len()).slice();
    const undone=flash();
    assert.notDeepEqual(raw.subarray(0xd9000,0xda000),Buffer.from(undone.subarray(0xd9000,0xda000)),
        'undo save wrote a new revision into the older mirror');
    boot(readFileSync(new URL('../evidence/choir/four-lanes-2.1.0.bin',import.meta.url)));
    const original=flash();
    function fourParts(bytes) {
        const area=Buffer.from(bytes.subarray(0xd9000,0xda000));
        const pos=area.indexOf('MCK1');
        assert.ok(pos>=0);
        return area.subarray(pos+4,pos+1048);
    }
    assert.deepEqual(fourParts(undone),fourParts(original),'undo restores all four original parts');
    // Three taps of the panel's SEL button fill the three empty seats.
    boot();btn(8);
    for(let i=0;i<3;i++) btn(3);
    const invited=Array.from({length:4},(_,i)=>portrait(i));
    assert.equal(new Set(invited).size,4);
    assert.ok(invited.every(hash=>hash!==empty),'SEL fills each seat with a distinct singer');
    // A lone monster in either row owns both half-bars in that row.
    for(const [extra,expected] of [[1,[0,1,2,2]],[3,[0,0,2,3]]]) {
        boot();btn(8);enc(0,extra);enc(1,6);
        if(extra===3) {enc(0,-1);enc(1,9);}
        btn(10);key(0,true);
        const turns=[];
        const t=chip.fm1_ms();
        for(let slot=0;slot<4;slot++) {
            while(chip.fm1_ms()-t<slot*1200+220) run(4);
            const lcd=screen();
            const dots=Array.from({length:4},(_,i)=>lcd[(19+Math.floor(i/2)*91+36)*240+8+i%2*119+92]);
            const background=dots.find(c=>dots.filter(v=>v===c).length===3);
            turns.push(dots.findIndex(c=>c!==background));
        }
        key(0,false);assert.deepEqual(turns,expected,'three-seat routing');
    }
    writeFileSync(join(out,'row-results.json'),JSON.stringify({stayed,parts,gaps,markers,steps:rows.map(active)},null,2)+'\n');
}
