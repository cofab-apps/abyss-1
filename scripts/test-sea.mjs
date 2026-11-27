import assert from 'node:assert/strict';
import {writeFileSync} from 'node:fs';
import {join} from 'node:path';
import {changedSectors,packSectors} from '../simulator/persistence.js';

export function checkSea({boot,chip,run,enc,btn,shots,out,movieStart,movieEnd}) {
    const key=(k,on)=>chip.fm1_key(14+k,Number(on));
    const flash=ptr=>new Uint8Array(chip.memory.buffer,ptr,chip.fm1_flash_len()).slice();
    const rms=a=>Math.sqrt(a.reduce((sum,x)=>sum+x*x,0)/a.length);
    function pitch(a) {
        let best=0,score=-1;
        // The whistle is E4. Use the first strong autocorrelation peak to
        // distinguish an octave descent from a change in overtone balance.
        let prev=0,prev2=0;
        for(let lag=100;lag<320;lag++) {
            let xy=0,xx=0,yy=0;
            for(let j=0;j<2048;j++) {
                const x=a[j],y=a[j+lag]; xy+=x*y;xx+=x*x;yy+=y*y;
            }
            const value=xy/Math.sqrt(xx*yy);
            if(prev>prev2 && prev>value && prev>.80) return 44100/(lag-1);
            if(value>score) {score=value;best=lag;}
            prev2=prev;prev=value;
        }
        assert.ok(score>.7); return 44100/best;
    }
    function harmonicRatio(a,hz) {
        const harmonics=[];
        for(let h=1;h<=10;h++) {
            let re=0,im=0;
            for(let j=0;j<4096;j++) {
                const v=a[j]*(.5-.5*Math.cos(2*Math.PI*j/4095));
                const angle=2*Math.PI*j*hz*h/44100;
                re+=v*Math.cos(angle); im+=v*Math.sin(angle);
            }
            harmonics.push(re*re+im*im);
        }
        return harmonics.slice(3).reduce((a,b)=>a+b,0)/harmonics.reduce((a,b)=>a+b,0);
    }
    boot();enc(0,3); // Loch Ness whistle
    enc(4,-22);enc(5,-22);run(350); // no Wobble or Cave
    btn(2);key(7,true);run(200);
    const clean=run(80,true), surface=pitch(clean);
    movieStart('dive');
    enc(2,8);run(180);enc(2,12);shots('depth');run(220);
    const deep=run(80,true), bottom=pitch(deep);
    assert.ok(bottom/surface>.48 && bottom/surface<.53,'Depth descends one octave');
    enc(2,-24);run(180);enc(3,24);shots('anger');run(120);
    const angry=run(80,true);
    const cleanHigh=harmonicRatio(clean,surface), angryHigh=harmonicRatio(angry,surface);
    assert.ok(angryHigh>cleanHigh+.01,'Anger adds upper harmonics');
    enc(3,-24);btn(5);enc(4,24);shots('current');run(100);
    const widths=[], correlations=[];
    for(let j=0;j<28;j++) {
        run(25);
        const left=new Float32Array(chip.memory.buffer,chip.fm1_out_l(),128);
        const right=new Float32Array(chip.memory.buffer,chip.fm1_out_r(),128);
        let midPower=0,sidePower=0,cross=0;
        for(let i=0;i<left.length;i++) {
            const mid=(left[i]+right[i])/2, side=left[i]-right[i];
            midPower+=mid*mid; sidePower+=side*side; cross+=mid*side;
        }
        widths.push(Math.sqrt(sidePower/left.length));
        correlations.push(cross/Math.sqrt(midPower*sidePower));
    }
    const motion=Math.max(...correlations)-Math.min(...correlations);
    assert.ok(Math.min(...widths)>.01 && motion>.5,'stereo comb phase sweeps across the held note');
    enc(4,-24);enc(5,24);shots('cavern');run(150);key(7,false);
    run(130);const tail=run(75);run(2400);const quiet=run(100);
    movieEnd();
    assert.ok(tail>.0004 && quiet<.0005,'Cavern has a decaying tail');
    // Check the most expensive path with all four voices and every sea effect.
    btn(8);enc(0,1);enc(1,3);enc(0,1);enc(1,6);enc(0,1);enc(1,9);
    enc(2,24);enc(3,24);enc(4,24);
    const start=chip.fm1_starved();key(7,true);const chord=run(450,true);
    const peak=chord.reduce((v,x)=>Math.max(v,Math.abs(x)),0);
    key(7,false);run(170);const missed=chip.fm1_starved()-start;
    assert.ok(rms(chord)>.01 && peak<.97 && missed===0,'full wet chord meets DAC deadlines');
    btn(9);run(160);
    const saved=packSectors(changedSectors(flash(chip.fm1_flash_base()),flash(chip.fm1_flash())));
    boot(saved);btn(8);key(7,true);run(450);const recalled=run(100);
    key(7,false);
    const raw=Buffer.from(flash(chip.fm1_flash()));
    const at=raw.indexOf('MCK1',0xd8000);
    assert.ok(at>=0 && at<0xda000);
    assert.deepEqual([...raw.subarray(at+1048,at+1052)],[127,127,127,127]);
    assert.ok(recalled>.01);
    const results={surface,bottom,cleanHigh,angryHigh,motion,tail,quiet,peak,missed,recalled};
    console.log(results);writeFileSync(join(out,'sea-results.json'),JSON.stringify(results,null,2)+'\n');
}
