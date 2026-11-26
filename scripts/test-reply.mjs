// Use a muted caller to isolate the reply at the DAC. The real panel and
// synth remain active. The recording never has access to firmware RAM.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';

export function checkReply({boot, btn, enc, run, chip, shots, screen, panelFrame, pattern, out, movieStart, movieEnd, saveAndReboot}) {
    const key = (id, on) => chip.fm1_key(id, on ? 1 : 0);
    function strike(id, held, gap) {
        key(id,true); run(held); key(id,false); run(gap);
    }
    function prepare(muteCaller) {
        boot(); enc(0,2); // Julia calls, Bloop answers one tile above her.
        if (muteCaller) {btn(7); btn(3); btn(8);}
        btn(10); // ARP
    }
    const activeFish = () => screen()[(5+6)*240 + (91+10)] === 0xffae;
    function waitForAnswer() {
        let n=0;
        while (!activeFish() && n<320) {run(2);n+=2;}
        assert.ok(n<320,'answer badge appears after release');
        return n;
    }
    const power = seq => Math.sqrt(seq.reduce((s,v)=>s+v*v,0) / seq.length);
    const hzOf = (seq,hz) => {
        let re=0,im=0;
        for(let i=0;i<seq.length;i++){
            const angle=2*Math.PI*hz*i/44100;
            re+=seq[i]*Math.cos(angle); im+=seq[i]*Math.sin(angle);
        }
        return Math.hypot(re,im);
    };

    // With no words and no prior song, HOME eventually points at the game.
    // The invitation alternates between the two creatures and blinks ARP.
    boot();
    const sourceFish = () => screen()[11*240 + 101] === 0x5f1b;
    const partnerFish = () => screen()[131*240 + 101] === 0xffae;
    let latestLamps;
    const arpLamp = () => {
        chip.fm1_leds(); run(6);
        latestLamps=Float32Array.from(new Float32Array(chip.memory.buffer,chip.fm1_leds(),41));
        return latestLamps[10];
    };
    const sourceFrame = () => screen()[2*240 + 42] === 0x5f1b;
    const partnerFrame = () => screen()[122*240 + 42] === 0xffae;
    let sourceSeen=false, partnerSeen=false, sourceFrameSeen=false, partnerFrameSeen=false, lampSeen=false;
    for(let i=0;i<900;i+=6) {
        run(6);
        sourceSeen ||= sourceFish();
        partnerSeen ||= partnerFish();
        sourceFrameSeen ||= sourceFrame();
        partnerFrameSeen ||= partnerFrame();
        lampSeen ||= arpLamp()>.08;
        if(sourceSeen && partnerSeen && sourceFrameSeen && partnerFrameSeen && lampSeen) break;
    }
    assert.ok(sourceSeen && partnerSeen && sourceFrameSeen && partnerFrameSeen && lampSeen,
              'idle invitation alternates fish, portrait frames and the ARP lamp');
    shots('idle-invitation');
    let invitationLevels;
    for(let i=0;i<400;i+=6) {
        const level=arpLamp();
        if(sourceFrame() && level>.08) {
            invitationLevels=latestLamps;
            break;
        }
    }
    assert.ok(invitationLevels,'invitation frame and ARP lamp overlap');
    fs.writeFileSync(path.join(out,'idle-invitation.rgba'),panelFrame(invitationLevels));
    let partnerLevels;
    for(let i=0;i<400;i+=6) {
        const level=arpLamp();
        if(partnerFrame() && level>.08) {
            partnerLevels=latestLamps;
            break;
        }
    }
    assert.ok(partnerLevels,'partner invitation frame and ARP lamp overlap');
    fs.writeFileSync(path.join(out,'partner-invitation.rgba'),panelFrame(partnerLevels));
    btn(10);btn(10); // Discover the game, then leave it.
    run(2200);
    assert.ok(!sourceFish() && !partnerFish() && arpLamp()<.04,'invitation stays dismissed after ARP is discovered');
    boot();btn(11);enc(0,3);btn(24);btn(8);saveAndReboot();
    run(2200);
    assert.ok(!sourceFish() && !partnerFish() && arpLamp()<.04,'a restored song never receives the new-player invitation');
    console.log('PASS wordless idle invitation, alternating turn icons, ARP lamp, session dismissal and saved-song exclusion.');

    prepare(true);
    key(21,true);
    assert.ok(run(250)<.0005,'a held muted caller generates no answer');
    key(21,false);run(24);
    strike(24,64,34);
    strike(28,92,0);
    const wait = waitForAnswer();
    assert.equal(screen()[2*240+42],0xffae,'the responder also owns the entire frame');
    // Onset spacing from the first attack. Convert from the 128-frame units
    // used to play the call. The first, long note should remain pitched.
    const pcm=run(600,true);
    const expected = [130.81,155.56,196.00];
    const offsets = [18,250+24+18,250+24+64+34+18];
    for (let i=0;i<3;i++) {
        const sample=pcm.subarray(offsets[i]*128,(offsets[i]+34)*128);
        const peaks=expected.map(f=>hzOf(sample,f));
        assert.ok(power(sample)>.003,'reply voice is audible');
        assert.equal(peaks.indexOf(Math.max(...peaks)),i,'reply pitch '+i+' / '+peaks.join(','));
    }
    assert.ok(run(300)<.0005,'one reply, then silence');
    fs.writeFileSync(path.join(out,'answer.f32'),Buffer.from(pcm.buffer));
    console.log('PASS held key waits; three returned pitches in the caller’s rhythm; response delay after release (ms)',Math.round(wait*128000/44100));

    // A fresh key during the first answer interrupts it. Then the child can
    // cancel the new phrase with ARP before more notes sound.
    strike(21,60,20);strike(28,100,0);waitForAnswer();
    run(12);key(24,true);run(25);key(24,false);btn(10);
    run(250);assert.ok(run(700)<.0005);
    btn(11);assert.deepEqual(pattern(),[]);btn(8);enc(0,2);btn(11);assert.deepEqual(pattern(),[]);
    console.log('PASS interrupt, cancel and two empty sequencer tracks after practice.');

    prepare(true);
    const chord=[21,24,28];
    for (const k of chord) {key(k,true);run(3);}
    run(180);
    for (const k of chord) key(k,false);
    waitForAnswer();
    const harmony=run(90,true).subarray(20*128);
    const bins=expected.map(f=>hzOf(harmony,f));
    const neighbours=expected.map(f=>hzOf(harmony,f*2**(1/12)));
    assert.ok(bins.every((v,i)=>v>3*neighbours[i]),'three chord pitches above neighbouring bins: '+JSON.stringify({bins,neighbours}));
    run(400);
    console.log('PASS a three-finger chord spread over 17 ms returns at all three pitches.');

    // Once the partner has finished, PLAY turns the exchange into a real
    // two-creature song: call in the first half, response in the second.
    prepare(false);movieStart('duet');
    strike(21,90,30);strike(24,45,40);strike(28,120,0);
    waitForAnswer();run(650);
    shots('duet-ready');
    let playLampSeen=false;
    for(let i=0;i<220;i+=6) {
        chip.fm1_leds();run(6);
        const levels=new Float32Array(chip.memory.buffer,chip.fm1_leds(),41);
        playLampSeen ||= levels[12]>.08;
    }
    assert.ok(playLampSeen,'PLAY lamp invites the child to keep the exchange');
    btn(12);
    assert.ok(run(900)>.001,'the kept duet plays from the DAC');
    shots('duet-loop');
    btn(12);
    movieEnd();
    btn(11);const sourcePattern=pattern();
    assert.ok(sourcePattern.length && sourcePattern.every(s=>s<8),JSON.stringify({sourcePattern}));
    btn(8);enc(0,2);btn(11);const partnerPattern=pattern();
    assert.ok(partnerPattern.length && partnerPattern.every(s=>s>=8),JSON.stringify({partnerPattern}));
    btn(8);enc(0,1);btn(11);assert.deepEqual(pattern(),[],'unused track 1 remains empty');
    btn(8);enc(0,2);btn(11);assert.deepEqual(pattern(),[],'unused track 3 remains empty');
    btn(8);btn(6); // one HOME / EDIT gesture owns both halves
    for(let i=0;i<4;i++) {
        btn(11);assert.deepEqual(pattern(),[],'duet undo clears track '+(i+1));
        btn(8);enc(0,1);
    }
    btn(6); // redo the complete duet
    enc(0,3);btn(11);
    assert.deepEqual(pattern(),sourcePattern,'redo restores the source half');
    btn(8);enc(0,2);btn(11);
    assert.deepEqual(pattern(),partnerPattern,'redo restores the partner half');
    btn(8);saveAndReboot();btn(11);
    assert.deepEqual(pattern(),partnerPattern,'saved duet restores its partner half');
    btn(8);enc(0,2);btn(11);
    assert.deepEqual(pattern(),sourcePattern,'saved duet restores its source half');
    console.log('PASS completed Reef Reply becomes an audible two-track loop; halves, unused tracks and atomic undo/redo are correct.',
                JSON.stringify({sourcePattern,partnerPattern}));

    // Practising over a project is safe: PLAY leaves the game and starts the
    // existing song, never injecting the temporary answer into it.
    boot();btn(11);enc(0,3);btn(24);btn(8);enc(0,2);
    btn(10);strike(21,65,0);waitForAnswer();run(450);btn(12);run(300);btn(12);
    btn(11);assert.deepEqual(pattern(),[],'temporary caller receives no sequenced notes');
    btn(8);enc(0,2);btn(11);assert.deepEqual(pattern(),[3],'existing partner track is unchanged');
    console.log('PASS PLAY after practice preserves a pre-existing song step-for-step.');

    prepare(true);
    for (let i=0;i<8;i++) strike(21+i%3,22,16);
    key(25,true);run(300);key(25,false);
    assert.ok(run(60)<.0005,'full phrase waits for the ninth key to lift and leaves a breath');
    waitForAnswer();run(650);
    strike(21,50,0);btn(12); // PLAY, while the delayed answer is pending
    assert.equal(activeFish(),false);
    btn(12);run(250);
    prepare(true);
    strike(21,45,0);enc(0,1);run(220);
    assert.ok(run(300)<.0005,'switching partners cancels pending practice');
    btn(13);strike(24,100,100);btn(13);btn(12);btn(11);
    assert.ok(pattern().length>0,'the new live note was recorded');
    btn(8);enc(0,2);btn(11);
    assert.deepEqual(pattern(),[],'the answering track stays empty when REC takes over');
    console.log('PASS eight-group limit, release of the overflow key, partner change, transport and a fresh recording.');

    boot();
    const statistics=[];
    for(let pair=0;pair<4;pair++) {
        btn(10);
        const start=chip.fm1_starved();
        for (const k of [21,24,28]) key(k,true);
        const call=run(110);
        for (const k of [21,24,28]) key(k,false);
        const response=run(520);
        btn(10);
        assert.ok(call>.0005 && response>.0005,'audible chord call and reply');
        statistics.push({call,response,missed:chip.fm1_starved()-start});
        enc(0,1);
    }
    console.log('Four default caller/partner pairs, three-key chords',JSON.stringify(statistics));
    assert.ok(statistics.every(x=>x.missed===0),'audio deadline under chord practice');
    assert.equal(chip.fm1_cpu1_fault(),0);

    // The musical partner may be an octave away. The child sees the same
    // physical positions they played, including a chord and the intervening rest.
    const lamps = () => {
        chip.fm1_leds();
        run(6);
        return Array.from(new Float32Array(chip.memory.buffer,chip.fm1_leds(),41));
    };
    prepare(true);
    run(150);
    const idle=lamps();
    for(const k of [21,24]) key(k,true);
    run(115);
    const hand=lamps();
    for(const k of [21,24]) key(k,false);
    run(42);strike(28,150,0);
    waitForAnswer();run(20);
    const echo1=lamps();
    fs.writeFileSync(path.join(out,'reply-chord.rgba'),panelFrame(echo1));
    run(104);
    const gap=lamps();
    run(40);
    const echo2=lamps();
    fs.writeFileSync(path.join(out,'reply-note.rgba'),panelFrame(echo2));
    const lit=levels=>[21,24,28].filter(k=>levels[k]>.04);
    assert.deepEqual([idle,hand,echo1,gap,echo2].map(lit),[[],[21,24],[21,24],[],[28]]);
    console.log('PASS scanned key lights on call, echoed chord, rest and echoed single note.',
                JSON.stringify([idle,hand,echo1,gap,echo2].map(m=>[m[21],m[24],m[28]])));

    prepare(true);
    strike(21,7,6);strike(24,205,0);
    waitForAnswer();
    const separate=run(165,true).subarray(95*128);
    assert.ok(hzOf(separate,155.56)>5*hzOf(separate,130.81),'quick released notes stay separate');
    console.log('PASS two released attacks about 38 ms apart remain separate notes.');


    prepare(false); movieStart('reply');
    // Both animals are audible here. Two phrases separated by an answer.
    strike(21,62,35); strike(24,45,28); strike(28,105,0); waitForAnswer();
    run(42); shots('answer'); run(358); shots('ready');
    strike(28,80,24); strike(25,55,22); strike(24,55,22); strike(21,145,0); waitForAnswer();
    run(530); btn(10); run(120); movieEnd();
    console.log('Captured real stereo call and response and the native display.');
}
