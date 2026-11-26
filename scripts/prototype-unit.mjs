// Test the shipped script's track and transport logic with audio and UI stubs.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';
import test from 'node:test';

for (const path of ['prototype/abyss1/index.html', 'prototype/ABYSS-1-prototype.html']) {
  const html = readFileSync(new URL('../' + path, import.meta.url), 'utf8');
  const code = html.match(/function pick\(id\)[\s\S]+?(?=\$\('prev'\)\.onclick)/)[0];
  function makeContext() {
    const ctx = vm.createContext({tracks: Array.from({length:4}, () => ({
      steps:Array(16).fill(null), params:[50,30,50,25], id:0, mute:false
    })), selected:0, CREATURES:Array.from({length:16}, (_,i)=>({name:String(i)})),
      recording:false, playing:false, step:-1, bpm:120, events:[],
      status(){}, audio(){}, refresh(){}, setTimeout(){return 1}, clearTimeout(){},
      $(){return {setAttribute(){}}}
    });
    vm.runInContext('function trigger(i,n){events.push([i,n])};' + code, ctx);
    return ctx;
  }
  test(path + ' assigns every creature independently and preserves recorded notes', () => {
    const c = makeContext();
    c.tracks.forEach((t,i)=>{t.steps[i]=60+i});
    for (let track=0;track<4;track++) {
      c.selected=track;
      for (let id=0;id<16;id++) {
        const before=structuredClone(c.tracks);
        c.pick(id);
        before[track].id=id;
        assert.deepEqual(c.tracks, before);
      }
    }
  });
  test(path + ' recording targets the selected track and playback wraps to step zero', () => {
    const c=makeContext();
    c.selected=2; c.recording=true; c.note(67);
    assert.equal(c.tracks[2].steps[0],67);
    for(const i of [0,1,3]) assert.equal(c.tracks[i].steps[0],null);
    c.events.length=0;
    for(let i=0;i<16;i++) c.tick();
    assert.equal(c.step,0);
    assert.equal(JSON.stringify(c.events),'[[2,67]]');
    c.stop();
    assert.equal(c.playing,false);
  });
}
