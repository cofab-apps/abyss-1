// Compare the same key, level, register and hold on each packaged build.
// All probes use physical panel events and samples emitted by the emulated DAC.
import assert from 'node:assert/strict';
import {readFileSync,writeFileSync} from 'node:fs';
import {join} from 'node:path';

import {changedSectors,packSectors} from '../simulator/persistence.js';

export function checkSound({boot,chip,run,enc,btn,shots,out,movieStart,movieEnd},baseline) {
    const key=(index,on)=>chip.fm1_key(14+index,Number(on));
    const rms=a=>Math.sqrt(a.reduce((sum,x)=>sum+x*x,0)/a.length);
    const energy=(a,hz)=>{
        let x=0,y=0;
        for(let j=0;j<a.length;j++) {
            const w=(.5-.5*Math.cos(2*Math.PI*j/(a.length-1)))*a[j];
            x+=w*Math.cos(j*2*Math.PI*hz/44100); y+=w*Math.sin(j*2*Math.PI*hz/44100);
        }
        return 4*Math.hypot(x,y)/a.length;
    };
    function frequency(a,lo,hi) {
        let peak=0,hz=0;
        for(let f=lo;f<=hi;f+=2) {
            const level=energy(a,f);
            if(level>peak) { peak=level;hz=f; }
        }
        return hz;
    }
    const cast=[['Crab',0],['Hydra',2],['Bloop',9],['Colossal Whale',16],['SCP-169',17]];
    const extracts=[];
    let last=2;
    boot();
    for(const [name,id] of cast) {
        enc(0,id-last);last=id;run(450);
        movieStart('cast-'+id);
        const before=chip.fm1_starved();
        key(7,true);const a=run(330,true);key(7,false);run(125);
        movieEnd();
        const peak=a.reduce((p,x)=>Math.max(p,Math.abs(x)),0);
        const missed=chip.fm1_starved()-before;
        extracts.push({name,id,rms:rms(a),peak,missed});
        assert.ok(peak<.97 && rms(a)>.002 && missed===0,name+' clean solo');
    }
    const evidence={firmware:baseline?'2.4.0':'2.5.0',extracts};
    if(!baseline) {
        // Dry Hydra has a minor third, fifth and octave above a A3 input transposed down five semitones.
        boot();enc(4,-24);enc(5,-24);run(450);key(7,true);run(170);
        const hydra=run(64,true);
        const headHz=[164.81,196.0,246.94,329.63]; // key 7: A3, Hydra -5 = E3
        const heads=headHz.map(f=>energy(hydra,f));
        const off=energy(hydra,220);
        assert.ok(heads.every(v=>v>.0005 && v>off*2),'four individually resolved Hydra pitches '+heads);
        key(7,false);
        // Dry Ness is a stable measuring note. Use low harmonics to detect vibrato and octave order.
        boot();enc(0,3);enc(4,-24);enc(5,-24);run(450);
        function held(){key(7,true);run(100);const a=run(60,true);key(7,false);run(250);return a;}
        function panel(id){btn(id);run(10);}
        const clean=held();
        panel(6);enc(4,24);const rich=held();
        enc(2,24);shots('filters');const muffled=held();
        enc(2,-24);enc(3,24);const thin=held();
        const levels=[clean,rich,muffled,thin].map(a=>[energy(a,329.63),energy(a,988.9)]);
        assert.ok(levels[1][1]>levels[0][1]*1.5,'third harmonic oscillator blend');
        assert.ok(levels[2][1]/levels[2][0]<levels[1][1]/levels[1][0]*.8,'low-pass suppresses overtones');
        assert.ok(levels[3][0]<levels[1][0]*.6,'high-pass removes fundamental');
        enc(3,-24);enc(4,-24);panel(5);enc(2,8);enc(3,20);enc(4,10);enc(5,6);shots('motion');
        key(7,true);run(100);const moving=run(260,true);key(7,false);run(150);
        const motion=[];
        for(let i=0;i+2048<=moving.length;i+=2048) motion.push(frequency(moving.subarray(i,i+2048),276,385));
        assert.ok(Math.max(...motion)-Math.min(...motion)>22,'vibrato changes sustained pitch');
        for(let i=0;i<4;i++)enc(2+i,-24);
        panel(4);enc(2,24);enc(3,10);enc(4,10);enc(5,22);shots('contour');run(700);
        key(7,true);const onset=rms(run(25,true));run(250);const sustain=run(50);
        key(7,false);const release=run(60);run(500);const quiet=run(60);
        assert.ok(onset<sustain*.3 && release>sustain*.45 && quiet<.0002,'attack and release '+[onset,sustain,release,quiet]);
        // Turn sustain fully down and back up on a held key, isolating the envelope.
        enc(2,-24);enc(3,-24);enc(5,-24);enc(4,-24);run(500);
        key(7,true);run(170);const zeroSustain=run(80);
        enc(4,24);const raisedSustain=run(80);key(7,false);
        assert.ok(zeroSustain<.0002 && raisedSustain>.02,'held sustain can close and reopen');
        boot();enc(0,3);enc(4,-24);enc(5,-24);run(450);
        chip.fm1_key(10,1);run(220);chip.fm1_key(10,0);run(80);shots('arp');
        key(7,true);const arp=run(195,true);key(7,false);
        const arpf=[2500,9800,17000].map(i=>frequency(arp.subarray(i,i+2048),225,750));
        assert.ok(arpf[1]>arpf[0]*1.9 && Math.abs(arpf[2]-arpf[0])<12,'root octave root '+arpf);
        // A two-note hand traverses both pitches, then both pitches an octave up.
        run(220);key(7,true);key(11,true);const arpChord=run(240,true);key(7,false);key(11,false);
        const order=[3600,10800,18000,25200].map(i=>frequency(arpChord.subarray(i,i+2048),225,990));
        assert.ok(order[1]>1.17*order[0] && order[2]>order[1] &&
            Math.abs(order[2]/order[0]-2)<.10 && Math.abs(order[3]/order[1]-2)<.10,
            'two-key arpeggio order '+order);
        // Actual 2.4 firmware saves a nonempty phrase. Boot the new package with
        // those sectors, play it, save a modified filter, and check a second boot.
        const snapshot=()=>{
            const flash=p=>new Uint8Array(chip.memory.buffer,p,chip.fm1_flash_len()).slice();
            return packSectors(changedSectors(flash(chip.fm1_flash_base()),flash(chip.fm1_flash())));
        };
        boot(undefined,readFileSync(new URL('../builds/seamonster-choir-2.4.0.fwsc',import.meta.url)));
        btn(8);btn(13);key(7,true);run(150);key(7,false);run(1650);btn(12);btn(9);run(200);
        const old=snapshot();
        assert.ok(old.length>0);
        boot(old);btn(8);btn(12);const recalled=run(100);btn(12);
        assert.ok(recalled>.003,'2.4 favourite plays after migration');
        btn(8);panel(6);enc(2,9);btn(8);btn(9);run(200);
        const upgraded=snapshot();boot(upgraded);
        const flash=Buffer.from(chip.memory.buffer,chip.fm1_flash(),chip.fm1_flash_len());
        const at=flash.indexOf('MCK1',0xd8000);
        assert.ok(at>=0 && at<0xda000 && flash[at+1068]>0,'saved filter value in upgraded favourite');
        btn(8);btn(12);const withFilter=run(100);btn(12);
        assert.ok(withFilter>.001 && Math.abs(withFilter-recalled)>.0002,'stored patch changes playback');
        // Simultaneous new effects and all four Hydra keys, including 16 head oscillators.
        boot();panel(6);enc(2,8);enc(3,4);enc(4,12);enc(5,12);
        panel(5);for(let i=0;i<4;i++)enc(2+i,12);
        panel(2);for(let i=0;i<4;i++)enc(2+i,12);
        const before=chip.fm1_starved();
        for(const i of [0,2,4,7])key(i,true);
        const stress=run(500,true);const missed=chip.fm1_starved()-before;
        assert.equal(missed,0,'wet four-Hydra chord meets deadlines');
        assert.ok(rms(stress)>.001);
        Object.assign(evidence,{heads,off,levels,motion,onset,sustain,release,quiet,zeroSustain,raisedSustain,arpf,order,recalled,withFilter,stressRms:rms(stress),missed});
    }
    writeFileSync(join(out,'sound-results.json'),JSON.stringify(evidence,null,2)+'\n');
    console.log(evidence);
}
