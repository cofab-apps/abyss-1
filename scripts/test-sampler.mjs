import assert from 'node:assert/strict';
import {writeFileSync} from 'node:fs';
import {join} from 'node:path';

export function checkSampler({boot,chip,run,btn,shots,out,movieStart,movieEnd}) {
    const key=(k,on)=>chip.fm1_key(14+k,Number(on));
    function audition(k) {
        key(k,true);const samples=run(160,true);key(k,false);run(180);
        return samples;
    }
    function measured(a) {
        let power=0,last=0,peak=0;
        for(let i=0;i<a.length;i++) {
            power+=a[i]*a[i];peak=Math.max(peak,Math.abs(a[i]));
            if(Math.abs(a[i])>.001) last=i;
        }
        return {rms:Math.sqrt(power/a.length),last,peak};
    }
    boot();btn(11);shots('shell');
    const slow=measured(audition(0)), fast=measured(audition(9));
    assert.ok(slow.rms>.002 && fast.rms>.002 && fast.last<slow.last*.65,'higher key plays the sample faster');
    btn(10); const back=audition(9);
    const reversed=measured(back);
    assert.ok(reversed.rms>.002);
    btn(10); const slice1=audition(0),slice2=audition(2);
    let delta=0;
    for(let i=0;i<slice1.length;i++) delta+=(slice1[i]-slice2[i])**2;
    assert.ok(measured(slice1).last<slow.last*.4 && delta/slice1.length>.000005,'short, distinct slices');
    btn(10);btn(13);key(7,true);run(125);key(7,false);btn(13);run(70);
    // The captured voice can play on its own with every physical key up.
    movieStart('shell-song');chip.fm1_key(12,1);const copied=run(90,true);chip.fm1_key(12,0);run(180);
    const captured=measured(copied);assert.ok(captured.rms>.01);
    btn(10);btn(12);run(200);btn(10);audition(0);audition(2);movieEnd();
    const slice=measured(audition(0));assert.ok(slice.rms>.002);
    // USB can be selected while stopped. An empty USB pass must retain the take.
    btn(3);btn(13);run(200);btn(13);
    const retained=measured(audition(0));assert.ok(retained.rms>.002);
    shots('usb-source');btn(11);
    key(7,true);const solo=run(100);key(7,false);assert.ok(solo>.01);
    btn(8);btn(11);btn(11);shots('return-to-choir');
    const results={slow,fast,reversed,slice,captured,retained,solo};
    console.log(results);writeFileSync(join(out,'sample-results.json'),JSON.stringify(results,null,2)+'\n');
}
