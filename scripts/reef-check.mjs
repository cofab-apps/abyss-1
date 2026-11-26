// Functional checks through the panel matrix, chip LCD and stereo DAC.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';

export function probeShell({boot, run, chip, out}) {
    for (const note of [36, 48, 60, 72]) {
        boot();
        for (const byte of [0x91, note, 110]) chip.fm1_midi(byte);
        const hit = run(160, true);
        for (const byte of [0x81, note, 0]) chip.fm1_midi(byte);
        const release = run(160, true);
        fs.writeFileSync(path.join(out, 'kraken-' + note + '.f32'),
            Buffer.concat([Buffer.from(hit.buffer), Buffer.from(release.buffer)]));
    }
}

export function checkReef({boot, btn, enc, run, chip, shots, screen, pattern, out, movieStart, movieEnd, saveAndReboot}) {
    const key = (id, down) => chip.fm1_key(id, Number(down));
    const frame = () => Uint16Array.from(screen());
    function changed(a, b, x, y, w, h) {
        let count = 0;
        for (let j = y; j < y + h; j++)
            for (let i = x; i < x + w; i++)
                if (a[j*240+i] !== b[j*240+i]) count++;
        return count;
    }
    const expected = [[0,3,6,10,12], [0,2,4,7,8,10,12,15], [2,5,9,14], [1,9]];
    function tracks() {
        btn(11);
        const result=[];
        for (let i=0;i<4;i++) {
            result.push(pattern());
            btn(8); enc(0,1); btn(11);
        }
        btn(8);
        return result;
    }
    boot();
    shots('quiet');
    const before = chip.fm1_starved();
    movieStart();
    // In the orchestra, PLAY should work with no prior SEQ or key press.
    btn(12);
    assert.ok(run(590) > .005, 'first PLAY produces the quartet');
    shots('quartet-playing');
    run(940); btn(12); run(450);
    assert.ok(run(300) < .0005, 'stopped quartet decays');
    movieEnd();
    assert.equal(chip.fm1_starved(), before);
    let notes = tracks();
    notes.forEach((track,i) => assert.ok(expected[i].every(n=>track.includes(n))));
    btn(6); // HOME / EDIT: undo all four factory tracks in one gesture
    assert.deepEqual(tracks(), [[],[],[],[]]);
    btn(12); assert.ok(run(1400)<.0005,'an intentionally empty bar stays empty'); btn(12);
    btn(6); assert.deepEqual(tracks(), notes);
    console.log('PASS first PLAY, stop, one-gesture removal and restoration of the complete quartet.');

    boot();
    btn(11); enc(0,3); btn(24); btn(8);
    btn(6); assert.deepEqual(tracks(), [[],[],[],[]]);
    btn(6); assert.deepEqual(tracks(), [[3],[],[],[]]);
    // Record another track, undo it, and check the earlier part survives.
    enc(0,1); btn(13); btn(22); run(300); btn(13); btn(12); btn(8);
    const take = tracks();
    assert.ok(take[0].length>0 && take[3].includes(3));
    btn(6); assert.deepEqual(tracks(), [[],[],[],[3]]);
    btn(6); assert.deepEqual(tracks(), take);
    console.log('PASS undo and redo of step entry and an overdub preserve the earlier track.');

    boot();
    btn(13); btn(21); enc(0,1); btn(24); btn(13); btn(12); btn(8);
    const multi = tracks();
    assert.ok(multi[0].length && multi[3].length);
    btn(13); btn(13); // cancel a fresh arm without playing a key
    btn(6); assert.deepEqual(tracks(), [[],[],[],[]]);
    btn(6); assert.deepEqual(tracks(), multi);
    console.log('PASS one undo lifts a two-track take, including after a cancelled arm.');

    boot(); enc(0,2); run(100);
    const juliaRest = frame();
    key(21,true); run(32);
    assert.ok(run(12)>.001);
    const juliaAttack = frame();
    shots('julia-attack');
    const amount = changed(juliaRest,juliaAttack,4,180,108,49);
    assert.ok(amount>80,'Julia arms move on the sounding track: '+amount);
    assert.equal(changed(juliaRest,juliaAttack,8,20,104,90),0);
    key(21,false); run(700);
    assert.equal(changed(juliaRest,frame(),5,125,110,108),0,'rest restores exact tile');
    // A MIDI note on a different track must animate its owner, even when unselected.
    for (const byte of [0x91,48,110]) chip.fm1_midi(byte);
    run(28); shots('kraken-strike');
    assert.ok(changed(juliaRest,frame(),126,62,108,54)>80,'MIDI Kraken moves');
    for (const byte of [0x81,48,0]) chip.fm1_midi(byte);
    run(700);
    assert.equal(changed(juliaRest,frame(),125,5,110,110),0);
    btn(3); run(200); const portrait = frame();
    key(21,true);run(64);
    assert.ok(changed(portrait,frame(),10,115,215,68)>80);
    assert.equal(changed(portrait,frame(),0,184,240,56),0,'legend stays readable');
    key(21,false);run(600);
    assert.equal(changed(portrait,frame(),10,115,215,68),0);
    console.log('PASS native movement on physical key, MIDI and full portrait; exact restoration and intact legend.',{changedJuliaPixels:amount});
    boot(); btn(10); btn(10); // Dismiss the idle invitation before exact artwork comparisons.
    const responseBlocks = [];
    for (let family=0; family<4; family++) {
        for (let variant=0; variant<4; variant++) {
            run(750);
            const still = frame();
            key(21,true);
            let first = 0;
            for (let block=4; block<=48; block+=4) {
                run(4);
                if (!first && changed(still,frame(),4,17,112,99)>10) first=block;
            }
            key(21,false);
            assert.ok(first, 'motion missing for 139 ms on creature '+(family*4+variant));
            responseBlocks.push(first);
            run(850);
            assert.equal(changed(still,frame(),4,17,112,99),0,'idle artwork restored');
            enc(6,1);
        }
        enc(1,1);
    }
    console.log('PASS all sixteen creatures move and restore. Maximum observed first movement (ms)', Math.ceil(Math.max(...responseBlocks)*128000/44100));

    if (process.env.ABYSS_PRIOR) {
        boot(undefined, fs.readFileSync(process.env.ABYSS_PRIOR));
        enc(0,2); btn(11); enc(0,6); btn(22); btn(8);
        saveAndReboot(); // Write using the old firmware, boot the release on its changed flash sectors.
        btn(12); assert.ok(run(1400)>.001); btn(12);
        assert.deepEqual(tracks(), [[6],[],[],[]]);
        console.log('PASS older saved song plays from HOME with its original solo track.');
    }
    if (process.env.ABYSS_QUARTET_SECONDS) {
        boot(); btn(12);
        let peak=0, power=0, blocks=0;
        const start=chip.fm1_ms(), starved=chip.fm1_starved();
        while(chip.fm1_ms()-start<Number(process.env.ABYSS_QUARTET_SECONDS)*1000) {
            const pcm=run(128,true);
            for (const value of pcm) {
                peak=Math.max(peak,Math.abs(value));
                power+=value*value;
            }
            blocks+=128;
        }
        const late=chip.fm1_starved()-starved;
        console.log('Factory quartet soak',JSON.stringify({ms:chip.fm1_ms()-start,blocks,
            peak,rms:Math.sqrt(power/(blocks*128)),missedRefills:late,cpu1Fault:chip.fm1_cpu1_fault()}));
        assert.equal(late,0);
        assert.equal(chip.fm1_cpu1_fault(),0);
        assert.ok(peak>.01 && peak<.99);
    }
}

export function showcase({boot, btn, enc, run, chip, out, movieStart, movieEnd, time}) {
    const scenes=[];
    const scene = caption => scenes.push({at:time(),caption});
    const note = (pitch, velocity, duration, rest) => {
        for (const b of [0x91,pitch,velocity]) chip.fm1_midi(b);
        run(duration);
        for (const b of [0x81,pitch,0]) chip.fm1_midi(b);
        run(rest);
    };
    boot(); enc(0,1); movieStart('reef');
    scene('Kraken  /  from shell drum to rattle');
    for (const n of [36,36,48,60,72,72]) note(n,110,53,70);
    scene('Julia  /  hold a key to sing');
    enc(0,1);
    for (const n of [21,24,28]) {chip.fm1_key(n,1);run(185);chip.fm1_key(n,0);run(80);}
    scene('PLAY  /  the quartet enters');
    btn(12); run(2850);
    scene('EDIT  /  lift the entire phrase');
    btn(6); run(450);
    scene('EDIT  /  bring it back on the beat');
    btn(6); run(1650); btn(12); run(400);
    movieEnd();
    fs.writeFileSync(path.join(out,'reef-scenes.json'),JSON.stringify({scenes,duration:time()}));
}
