"""Cross-check the WAV sender and firmware receiver, plus routed USB PCM capture."""
import ctypes as ct
import importlib.util
import math
import os
import sys
from pathlib import Path
import struct
import subprocess
import tempfile
import wave

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("sender", root / "scripts/load-sample.py")
sender = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sender)

with tempfile.TemporaryDirectory() as temp:
    tmp = Path(temp)
    src = tmp / "io.c"
    src.write_text(r"""
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
enum { NKEYS=27 }; enum { B_REC, B_SEL, B_ARP, B_PLAY, B_OCTDN, B_OCTUP };
enum { EN_SELECT, EN_PRESET, EN_ALGO, EN_K1, SPECIES=20 };
static int redraw, register_shift, depth_q8, animal;
static uint32_t musical_keys,note_step[112];
static uint8_t white_ids[16], key_pitch[NKEYS];
static int8_t sine_wave[256];
static int32_t input[256],output[256];
static uint8_t reply[13];
static int plat_sysex_send(const uint8_t *p,unsigned n) { if(n!=13) abort(); memcpy(reply,p,n);return 0; }
static const int32_t *plat_usb_audio_in(void) { return input; }
static void render_monsters(int32_t *p,unsigned n,int m) { memset(p,0,2*n*sizeof(*p)); }
static void sea_render(int32_t *p,unsigned n,int m) {}
static void smooth_depth(void) {}
static int clamp(int v,int a,int b){return v<a?a:v>b?b:v;}
static void plat_usb_audio_route(int on) {}
static void select_animal(int i) {}
static int plat_enc(int r) {return 0;}
static int wrap_index(int v,int n) {return (v%n+n)%n;}
static void adjust_sea(int i,int d,uint32_t now) {}
#include "sampler.h"
int packet(uint8_t *p, unsigned n){memset(reply,0,sizeof(reply)); return app_sysex(p,n)?reply[8]:-1;}
int offset(void){return reply[9]|reply[10]<<7|reply[11]<<14;}
int ready(void){return import_ready;}
int word(int i){return clips[sample_bank^1][i];}
void init(void){sample_mode=1;sample_len=100;sample_stop=sample_arm=sample_count=import_ready=0;transfer_id=-1;}
int route(int blocks,int value){
    sample_usb=sample_arm=1; sample_count=0;
    for(int i=0;i<256;i++)input[i]=value;
    for(int j=0;j<blocks;j++)render_sampler(output,128,4096);
    if(sample_arm) finish_sample();
    return sample_len;
}
int current(int i){return clips[sample_bank][i];}
""")
    so = tmp / "io.so"
    subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-O2", "-I", str(root/"monster-keys"),
                    str(src), "-o", str(so)], check=True)
    c = ct.CDLL(str(so))
    c.init()
    wav = tmp / "call.wav"
    with wave.open(str(wav), "wb") as stream:
        stream.setparams((2, 2, 44100, 0, "NONE", ""))
        sample = [int(math.sin(2*math.pi*330*i/44100)*16000) for i in range(97020)]
        stream.writeframes(b"".join(struct.pack("<hh", v, v//2) for v in sample))
    data = sender.read_clip(wav, .1)
    assert len(data) == 44100
    frames = list(sender.packets(data, 19))

    def send(frame):
        raw = (ct.c_ubyte * len(frame))(*frame)
        return c.packet(raw, len(frame))

    begin, _ = frames[0]
    assert send(begin) == 1
    assert send(frames[2][0]) == 0 and c.ready() == 0  # out of order
    assert send(frames[-1][0]) == 0                      # incomplete
    for frame, offset in frames[:-1]:
        damaged = frame.copy()
        if frame[4] == 2 and not offset % 192:
            damaged[-1] ^= 1
        assert send(damaged) == 1
    assert send(frames[-1][0]) == 0 and c.ready() == 0  # CRC
    for frame, offset in frames:
        assert send(frame) == 1 and c.offset() == offset
    assert c.ready() == 22050
    assert all(c.word(i) == ((data[2*i] | data[2*i+1]<<7)-8192)*4 for i in range(22050))
    assert c.route(9, 0) == 100  # silent input preserves the original sample
    assert c.route(14, 655360) == 14*32 and c.current(20) == 2560
    print("PASS full-length WAV, offset, interrupted transfer, CRC, all decoded samples, silence and routed stereo PCM")
    if "--browser" in sys.argv:
        from playwright.sync_api import sync_playwright

        c.init()

        def midi(frame):
            ok = send(frame[1:-1])
            return [240, *sender.PREFIX, 4, frame[6], frame[5], ok, *sender.packed(c.offset()), 247]

        with sync_playwright() as pw:
            browser = pw.chromium.launch(executable_path=os.getenv("CHROMIUM"))
            page = browser.new_page()
            page.expose_function("testDevice", midi)
            page.add_init_script("""
                const input=Object.assign(new EventTarget(),{name:'Seamonster Choir',id:'in',open:async()=>{}});
                const output={name:'Seamonster Choir',id:'out',open:async()=>{},send:async frame=>{
                    const data=await testDevice(frame);
                    input.dispatchEvent(Object.assign(new Event('midimessage'),{data}));
                }};
                Object.defineProperty(navigator,'requestMIDIAccess',{value:async()=>({
                    inputs:new Map([['in',input]]),outputs:new Map([['out',output]])})});
            """)
            page.goto((root/"tools/sample-loader.html").as_uri())
            page.get_by_text("Find MIDI ports").click()
            page.locator("#file").set_input_files(wav)
            page.locator("#send").click()
            page.wait_for_function("document.querySelector('#status').value.startsWith('Clip received')")
            assert c.ready() == 22050 and max(abs(c.word(j)) for j in range(400)) > 1000
            browser.close()
        print("PASS browser WAV selection, resampling and acknowledgements through the compiled receiver")
