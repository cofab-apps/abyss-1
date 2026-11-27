import assert from 'node:assert/strict';
import {writeFileSync} from 'node:fs';
import {join} from 'node:path';
import {createHash} from 'node:crypto';

export function checkMonsters({boot, chip, run, enc, btn, shots, screen, out, movieStart, movieEnd}) {
    const results = [], pictures = new Set();
    const press = (k,on) => chip.fm1_key(14+k,Number(on));
    function portrait() {
        const pixels=screen(), x=36,y=25,w=168;
        const h=createHash('sha256');
        for(let j=0;j<w;j++) h.update(new Uint8Array(pixels.buffer,pixels.byteOffset+2*((y+j)*240+x),2*w));
        return h.digest('hex');
    }
    function stats(samples) {
        let power=0, peak=0, crosses=0;
        for(let i=0;i<samples.length;i++) {
            const v=samples[i];
            power+=v*v; peak=Math.max(peak,Math.abs(v));
            if(i&&v*samples[i-1]<0) crosses++;
        }
        return {rms:Math.sqrt(power/samples.length),peak,crosses};
    }
    function bandLevels(samples) {
        // Relative energy in octave-wide bands, using a window from the
        // sustained part of the note. Comparable across different volumes.
        const size=1024, energy=[0,0,0,0,0,0,0];
        for(let bin=1;bin<size/2;bin++) {
            let re=0,im=0;
            for(let j=0;j<size;j++) {
                const v=samples[8192+j]*(.5-.5*Math.cos(2*Math.PI*j/(size-1)));
                const angle=2*Math.PI*j*bin/size;
                re+=v*Math.cos(angle); im+=v*Math.sin(angle);
            }
            const band=Math.max(0,Math.min(6,Math.floor(Math.log2(bin*44100/size/125))));
            energy[band]+=re*re+im*im;
        }
        const total=energy.reduce((a,b)=>a+b);
        return energy.map(e=>Math.round(e/total*1000)/1000);
    }
    boot();
    shots('boot-hydra');
    enc(0,-2);
    for(let id=0;id<20;id++) {
        if(id) enc(0,1);
        run(175);
        const image=portrait();
        assert.ok(!pictures.has(image),'distinct resting portrait '+id);
        pictures.add(image);
        shots('rest-'+id);
        const missed=chip.fm1_starved();
        press(7,true);
        const samples=run(225,true);
        const sound=stats(samples);
        assert.ok(sound.rms>.001 && sound.peak<.97,'playable note '+id+': '+JSON.stringify(sound));
        assert.notEqual(portrait(),image,'singing animation '+id);
        for(const k of [0,4,11]) press(k,true);
        const chord=stats(run(100,true));
        for(const k of [0,4,7,11]) press(k,false);
        run(150);
        results.push({id,...sound,bands:bandLevels(samples),chordPeak:chord.peak,missed:chip.fm1_starved()-missed});
        console.log('Monster',id,JSON.stringify(results.at(-1)));
    }
    enc(0,1);run(175);
    assert.equal(portrait(),pictures.values().next().value,'wrap to crab');
    for(let id=0;id<4;id++) {
        enc(2+id,-24); run(600);
        press(7,true); const low=run(280,true); press(7,false); run(400);
        enc(2+id,24); run(600);
        press(7,true); const high=run(280,true); press(7,false); run(400);
        shots('knob-'+id);
        let delta=0;
        for(let i=0;i<low.length;i++) delta+=(low[i]-high[i])**2;
        assert.ok(Math.sqrt(delta/low.length)>.001,'audible knob '+id);
    }
    enc(0,2);
    press(0,true);press(4,true);press(7,true);press(11,true);
    const missed=chip.fm1_starved(), chord=stats(run(350,true));
    assert.ok(chord.rms>.005 && chord.peak<.97,'four-note chord');
    const chordMissed=chip.fm1_starved()-missed;
    for(const k of [0,4,7,11]) press(k,false);
    run(850); const quiet=run(60); assert.ok(quiet<.0002,'rests return to silence');
    // Each raised key is the expressive surface for the white note to its
    // left. Rocking four held notes onto those keys must not steal the four
    // voices and leave the still-held chord silent.
    boot();
    for(const k of [0,2,4,7]) press(k,true);
    const plainChord=run(220);
    for(const k of [1,3,5,8]) press(k,true);
    const bentChord=run(220);
    for(const k of [1,3,5,8]) press(k,false);
    run(850);
    const chordStillHeld=run(120);
    assert.ok(plainChord>.005 && bentChord>.005 && chordStillHeld>.005,
        'raised-key bends retain the four held voices');
    assert.ok(Math.abs(plainChord-bentChord)>.001,'raised-key bend changes the DAC output');
    for(const k of [0,2,4,7]) press(k,false);
    boot();
    // Record a rest between two notes and change the second note's mouth.
    btn(13); press(7,true); run(110); press(7,false); run(430);
    press(11,true); enc(2,5); run(160); press(11,false); btn(13); run(700);
    btn(13); btn(13); // Cancel an empty take, retaining the earlier one.
    chip.fm1_key(12,1); run(10); chip.fm1_key(12,0);
    const first=run(65);
    run(300);
    const middle=run(40);
    run(160);
    const second=run(55);
    assert.ok(first>.005 && middle<.002 && second>.005,'recorded notes, rest and second note');
    run(900);
    movieStart('monster-calls');
    for(const [turns,key,knob] of [[0,7,2],[7,2,3],[2,7,5],[8,4,4]]) {
        if(turns) enc(0,turns);
        run(40); press(key,true);run(150);enc(knob,6);run(100);press(key,false);run(115);
    }
    movieEnd();
    writeFileSync(join(out,'results.json'),JSON.stringify({results,chord,chordMissed,quiet,
        fingerBend:{plainChord,bentChord,chordStillHeld},replay:{first,middle,second}},null,2)+'\n');
    assert.ok(results.every(r=>r.missed===0) && chordMissed===0,'DAC deadlines at requested clock');
    console.log('PASS portrait bank, DAC, navigation, four knobs, raised-key finger bends, four-note polyphony, silence.');
}
