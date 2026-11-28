/* SPDX-License-Identifier: MPL-2.0 */
#include "fastmath.h"
#include "voices.h"
#include "monster-data.h"

enum { SPECIES = 20, POLY = 4, TILE = 168, RING = 8192 };
static volatile uint8_t animal, knob[4], preview_serial;
static volatile int8_t register_shift;
static volatile uint32_t musical_keys;
static uint32_t note_step[112], scanned_keys, previous_buttons, focus_until, preview_until;
static int focus = -1, redraw = 1;
static const uint8_t white_ids[] = PLAT_WHITE_KEYS;
/* Every finger lands in the same pentatonic collection. The raised keys give
 * the neighbouring note a sharper articulated attack. */
static const uint8_t key_pitch[NKEYS] = {
    0,0,2,2,4,4,7,9,9,12,12,14,16,16,19,19,21,21,24,26,26,28,28,31,33,33,36
};
static const uint32_t raised = (1u<<1)|(1u<<3)|(1u<<5)|(1u<<8)|(1u<<10)|
    (1u<<13)|(1u<<15)|(1u<<17)|(1u<<20)|(1u<<22)|(1u<<25);

struct call {
    uint32_t phase, companion, throat, nose, lfo, step, age;
    uint32_t head_phase[3], sub_phase, ring_phase;
    int level, key, gated, accent, species, pressure, lp, hp;
};
static struct call calls[POLY];
static int16_t echo_ring[RING];
static unsigned echo_pos, audition_blocks;
static uint32_t played_keys;
static uint8_t last_preview, last_species;

/* A raised key shares its pitch with the white key immediately to its left.
 * Treat the pair as one playable surface: rocking a finger onto the raised key
 * bends the held voice instead of consuming another polyphonic slot. */
static int voice_key(int key)
{
    return key < NKEYS && (raised & (1u << key)) ? key - 1 : key;
}

static uint32_t voice_keys(int key)
{
    uint32_t keys = 1u << key;
    if (key + 1 < NKEYS && (raised & (1u << (key + 1))) &&
        key_pitch[key + 1] == key_pitch[key])
        keys |= 1u << (key + 1);
    return keys;
}

#include "sculpt.h"

static void select_animal(int index)
{
    const struct species *p = &creatures[index];
    knob[0] = p->mouth; knob[1] = p->teeth; knob[2] = p->wobble; knob[3] = p->cave;
    animal = index;
    redraw = 1;
}

static void start_call(int key, int species)
{
    int slot = 0;
    for (int i = 0; i < POLY; ++i) {
        if (!calls[i].level && !calls[i].gated) { slot = i; break; }
        if (calls[i].gated < calls[slot].gated ||
            (calls[i].gated == calls[slot].gated && calls[i].age > calls[slot].age))
            slot = i;
    }
    int k = key == NKEYS ? 7 : voice_key(key);
    int note = clamp(48 + key_pitch[k] + creatures[species].transpose + 12*(register_shift+(key<NKEYS?arp_octave:0)), 24, 98);
    calls[slot] = (struct call){.key=key, .species=species, .gated=1, .level=1,
        .step=note_step[note]};
}

static int speaker_sample(int sample,int master);
#include "sea.h"
#include "choir.h"
static void render_guest(struct call *,int32_t *,unsigned);

/* Advance the slower articulators once per DAC block. The mouth oscillator,
 * modulation oscillator, glottal rasp and envelope run at the sample rate. */
static void render_call(struct call *v, int32_t *mix, uint32_t n, const volatile uint8_t *controls)
{
    const struct species *p = &creatures[v->species];
    int mouth = clamp(controls[0] + v->pressure/2, 0, 127);
    int teeth = clamp(controls[1] + v->pressure/2, 0, 127), wobble = controls[2];
    v->lfo += (uint32_t)(2 + p->syllables) * (22u + wobble) * n * 45u;
    int movement = sine_wave[v->lfo >> 24];
    int openness = clamp(mouth*3 + (movement*wobble >> 9), 0, 383);
    int band = openness >> 7, blend = openness & 127;
    const int8_t *a = &mouth_waves[(v->species*4 + band)*256];
    const int8_t *b = a + 256;
    int onset = 255 - (int)(v->age < 11264 ? v->age*255u/11264u : 255u);
    int bend = p->scoop*onset + v->accent*160 + (movement*wobble >> 4);
    uint32_t step = v->step + (int32_t)(v->step >> 14)*bend;
    step-=(step>>8)*(depth_q8>>8);
    uint32_t second;
    int pulse = 128, type = p->type;
    if (type == CLACK) pulse = 28 + ((movement+126)*(movement+126) >> 9);
    /* Two damped resonances restart at each glottal closure. Their frequencies
     * follow the tract and stay put as the player moves up the keyboard. */
    static const uint16_t first[4]={285,490,825,305}, next[4]={710,1030,1460,2510};
    int f1=first[band]+(((int)first[band+1]-first[band])*blend>>7);
    int f2=next[band]+(((int)next[band+1]-next[band])*blend>>7);
    uint32_t tract=75000u+v->species%5*5600u+teeth*130u;
    uint32_t throat_step=f1*tract, nose_step=f2*tract;
    const struct signature *sig=&signature[v->species];
    unsigned age=v->age, attack=(sig->attack_ms+sculpt.contour[0]*5u)*44u;
    unsigned decay=(sig->decay_ms+sculpt.contour[1]*7u)*44u;
    int trim=sculpt.contour[2];
    int sustain=trim<=64 ? sig->sustain*trim/64 :
        sig->sustain+(127-sig->sustain)*(trim-64)/63;
    int target=4096+12*v->pressure, end;
    if(!v->gated) {
        unsigned time=(sig->release_ms+sculpt.contour[3]*5u)*44u;
        end=clamp(v->level-(int)(4096u*n/time+1),0,v->level);
    } else if(age+n<attack) end=(age+n)*target/attack;
    else if(age+n<attack+decay) end=target-target*(127-sustain)/127*(age+n-attack)/decay;
    else end=target*sustain/127;
    int env=v->level*256, slope=(end-v->level)*256/(int)n;
    v->level=end;
    /* The oscillator LFO starts in phase at each new articulation. */
    uint32_t rate=33000+sculpt.motion[0]*sculpt.motion[0]*100u;
    int wave=sine_wave[(age*rate)>>24];
    step+=(int32_t)(step>>14)*(wave*sculpt.motion[1]>>3);
    int dive=age<9000?(int)(9000-age):0;
    step+=(step>>11)*(unsigned)(sig->drop*dive/210);
    /* Space Bloop orbits in three registers while its key stays down. */
    if(v->species==19) {
        unsigned orbit=(age/7350u)%4;
        if(orbit==1 || orbit==3) step=(step>>1)*3;
        if(orbit==2) step*=2;
    }
    if(type==CHOIR) second=step+(step>>1)+(step>>9);
    else if(type==WHALE) second=(step>>1)+(step>>9);
    else second=step*p->ratio;
    int trem=256-((127+wave)*sculpt.motion[2]>>7);
    if(sig->pulses) {
        int pulse_wave=sine_wave[(age*(uint32_t)sig->pulses*97400u)>>24];
        trem=trem*(194+(pulse_wave>>1))>>8;
    }
    if(v->species==0) {
        /* Two claws strike on a three-part cycle, with space after the pair. */
        unsigned tick=age%13700u;
        int distance=tick<4500u?(int)tick:(int)tick-4500;
        trem=35+clamp(215-distance/9,0,215);
        trem=trem*(256-((127+wave)*sculpt.motion[2]>>7))>>8;
    }
    int dark=clamp(sculpt.filter[0]+((127+wave)*sculpt.motion[3]>>8),0,127);
    int open=127-dark, cut=4+open*open*252/16129;
    int high=sculpt.filter[1], hp_cut=1+high*high*64/16129;
    int sub=clamp(sig->sub+sculpt.filter[3],0,127);
    int overtone=clamp(sig->overtone+sculpt.filter[2],0,127);
    unsigned tone_step=v->species==3?(step>>6)*273u:step*5u;
    int strike=clamp(112-(int)(age<24576u?age:24576u)/180,0,112);
    uint32_t head_step[3];
    int head_gain[3]={0};
    static const uint16_t ratios[20]={256,271,287,304,323,342,362,384,406,431,456,483,
        512,542,575,609,645,683,724,767};
    for(int h=0;h<sig->heads;h++) {
        head_step[h]=(step>>8)*ratios[(int)sig->semitones[h]];
        head_gain[h]=clamp(((int)age-(h+1)*sig->spacing_ms*44)/16,0,96);
    }
    for (uint32_t i=0; i<n; ++i) {
        v->phase += step; v->companion += second;
        int mod = sine_wave[v->companion >> 24];
        unsigned phase = v->phase >> 24;
        if (v->phase<step) v->throat=v->nose=0;
        v->throat+=throat_step; v->nose+=nose_step;
        int vowel=(sine_wave[v->throat>>24]*2 + sine_wave[v->nose>>24])*(255-(int)phase)>>9;
        unsigned idx = (phase + (mod*(teeth + v->accent*20) >> 8)) & 255;
        int body = a[idx] + ((b[idx]-a[idx])*blend >> 7);
        switch (type) {
        case GROWL: body = ((body+vowel)*(85+((int)(v->companion>>31)*teeth/2)) >> 8) + (mod >> 2); break;
        case CHOIR: body = (vowel*3 + a[v->companion>>24] + sine_wave[(phase+(movement>>3))&255])/5; break;
        case WHALE: body = (body + mod*2 + vowel) >> 2; break;
        case WHISTLE: body = (body + 2*sine_wave[idx]) / 3; break;
        case RADIO: body = ((body*(150-teeth) + (body*mod >> 6)*teeth) >> 7)*(165+movement/2)>>7; break;
        case CLACK: body = (body*pulse >> 7) + (mod*onset >> 10); break;
        }
        /* Distinct exciter/resonator pairs for creatures that share a family. */
        switch(v->species) {
        case 3: /* A struck bone rings above the whale and decays independently. */
            v->ring_phase+=tone_step;
            body+=(sine_wave[v->ring_phase>>24]*strike)>>7;
            break;
        case 4: /* Airy overtone above Ningen's gently entering octave. */
            body=(body>>1)+(sine_wave[(phase*5)&255]>>1)+(sine_wave[(phase*7)&255]>>2);
            break;
        case 6: /* Metallic plates emphasise a high, pitched ring modulation. */
            v->ring_phase+=tone_step;
            body=(body>>1)+(sine_wave[v->ring_phase>>24]*mod>>6);
            break;
        case 13: /* A broad island horn speaks through a little speaker, too. */
            body=(body>>1)+(sine_wave[(phase*3)&255]>>1)+(sine_wave[(phase*7)&255]>>2);
            break;
        case 17: /* A brief upper crack makes the onset precede the low rumble. */
            body+=(sine_wave[(phase*9+(mod>>2))&255]*strike)>>7;
            break;
        }
        if(sig->heads) {
            int harmony=0;
            for(int h=0;h<sig->heads;h++) {
                v->head_phase[h]+=head_step[h];
                harmony+=a[v->head_phase[h]>>24]*head_gain[h];
            }
            body=(body*96+(harmony>>1))/(96+sig->heads*24);
        }
        v->sub_phase+=step>>1;
        body+=(sine_wave[v->sub_phase>>24]*sub + sine_wave[(phase*3)&255]*overtone)>>7;
        if(dark || high) {
            v->lp+=((body*256-v->lp)*cut)>>8;
            body=v->lp>>8;
            if(high) { v->hp+=((body*256-v->hp)*hp_cut)>>8; body-=v->hp>>8; }
        }
        env+=slope;
        mix[i]+=((body*(env>>8)>>7)*trem)>>8;
    }
    if (v->age < 0x70000000u) v->age += n;
}

static int speaker_sample(int sample, int master)
{
    int magnitude=sample<0?-sample:sample;
    if(magnitude>8192) {
        int excess=magnitude-8192;
        magnitude=8192+(8192*excess)/(8192+excess);
        sample=sample<0?-magnitude:magnitude;
    }
    return sample*master/8;
}

static void render_monsters(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    smooth_depth();
    int32_t mix[APP_BLOCK] = {0};
    uint32_t keys = choir_mode?musical_keys:ripple_keys(musical_keys,n), fresh = keys & ~played_keys;
    int species = animal;
    static int was_choir;
    int ensemble = choir_mode;
    if (ensemble != was_choir) {
        for (int i=0; i<POLY; ++i) calls[i]=(struct call){0};
        played_keys=audition_blocks=0;
        fresh=keys;
        last_preview=preview_serial;
        was_choir=ensemble;
    }
    int wet = knob[3];
    if (ensemble) {
        const struct choir_sound *frame = sound_frames[sound_front];
        int32_t send[APP_BLOCK] = {0};
        for (int i=0; i<POLY; ++i) {
            const struct choir_sound *s = &frame[i];
            struct call *v = &calls[i];
            if (s->note && (!v->gated || v->key!=s->note || v->species!=s->species ||
                            last_attack[i]!=s->attack)) {
                *v=(struct call){.step=note_step[s->note], .species=s->species,
                                 .key=s->note, .level=1, .gated=1};
            }
            last_attack[i]=s->attack;
            v->gated=!!s->note;
            v->accent=s->bend;
            v->pressure=s->pressure;
            if (v->level || (v->gated && v->species!=SPECIES)) {
                int32_t part[APP_BLOCK]={0};
                if(v->species==SPECIES) render_guest(v,part,n);
                else render_call(v,part,n,s->knobs);
                for (unsigned j=0;j<n;j++) {
                    mix[j]+=part[j];
                    send[j]+=part[j]*s->knobs[3] >> 7;
                }
            }
        }
        for (unsigned j=0;j<n;j++) {
            int l=echo_ring[(echo_pos-5767)&(RING-1)], r=echo_ring[(echo_pos-7823)&(RING-1)];
            echo_ring[echo_pos]=clamp(send[j]+r/5,-30000,30000);
            echo_pos=(echo_pos+1)&(RING-1);
            lr[2*j]=mix[j]+l/2;
            lr[2*j+1]=mix[j]+r/2;
        }
        sea_render(lr,n,master_q12);
        return;
    }
    if (species != last_species) {
        for (int i=0; i<POLY; ++i) calls[i].gated = 0;
        fresh = keys;
        last_species = species;
    }
    for (int i=0; i<POLY; ++i) {
        if (calls[i].key == NKEYS) continue;
        uint32_t group = voice_keys(calls[i].key);
        calls[i].accent = !!(keys & group & raised);
        if (!(keys & group)) calls[i].gated = 0;
    }
    for (int k=0; fresh; ++k, fresh >>= 1) if (fresh & 1u) {
        int key = voice_key(k), active = 0;
        for (int i=0; i<POLY; ++i)
            if (calls[i].gated && calls[i].key == key && calls[i].species == species)
                active = 1;
        if (!active) start_call(key,species);
    }
    played_keys = keys;
    if (preview_serial != last_preview) {
        last_preview = preview_serial;
        for (int i=0; i<POLY; ++i) if (calls[i].key == NKEYS) calls[i].gated = 0;
        start_call(NKEYS,species);
        audition_blocks = 155;
    }
    if (audition_blocks && !--audition_blocks)
        for (int i=0; i<POLY; ++i) if (calls[i].key == NKEYS) calls[i].gated = 0;
    for (int i=0; i<POLY; ++i) if (calls[i].level || calls[i].gated) {
        calls[i].pressure=calls[i].key<NKEYS ? finger_force[calls[i].key] : 0;
        render_call(&calls[i],mix,n,knob);
    }
    for (uint32_t i=0; i<n; ++i) {
        int dry = mix[i];
        int l = echo_ring[(echo_pos-5767)&(RING-1)], r = echo_ring[(echo_pos-7823)&(RING-1)];
        echo_ring[echo_pos] = clamp(dry+(r*wet >> 9),-30000,30000);
        echo_pos = (echo_pos+1)&(RING-1);
        lr[2*i] = dry+(l*wet >> 8);
        lr[2*i+1] = dry+(r*wet >> 8);
    }
    sea_render(lr,n,master_q12);
}

#include "sampler.h"
static int16_t guest_pcm[CLIP_MAX] __attribute__((section(".pool")));
static int guest_length;
static void render_guest(struct call *v,int32_t *part,unsigned n)
{
    unsigned step=v->step/(note_step[48]/1024u);
    for(unsigned i=0;i<n;i++) {
        unsigned j=v->phase>>12;
        if(j+1>=(unsigned)guest_length) { v->level=0; break; }
        int a=guest_pcm[j], b=guest_pcm[j+1];
        int value=a+((b-a)*(int)(v->phase&4095)>>12);
        v->level=clamp(v->level+(v->gated?1:-1),0,64);
        int fade=clamp((guest_length-1-(int)j)*4,0,v->level);
        part[i]=value*fade/128; v->phase+=step;
    }
}
static void adopt_guest(void)
{
    unsigned hash=2166136261u;
    guest_length=sample_len;
    memcpy(guest_pcm,clips[sample_bank],guest_length*sizeof(*guest_pcm));
    for(int i=0;i<guest_length;i++) hash=(hash^(uint16_t)guest_pcm[i])*16777619u;
    guest_signature=hash?hash:1;
    sample_mode=0; transfer_id=-1; sample_pos=-1; plat_usb_audio_route(0);
    toggle_choir(); selected=sample_target;
    if(song.seats[selected].species==EMPTY) assign_singer(selected,animal);
    song.guest_seat=selected+1; song.guest_hash=guest_signature;
    app_panic(); redraw=1;
}

void app_render(int32_t *lr,uint32_t n,uint32_t master)
{
    if(import_ready) {
        sample_bank^=1; sample_len=import_ready; sample_pos=-1; import_ready=0;
    }
    if(sample_mode) render_sampler(lr,n,master);
    else render_monsters(lr,n,master);
}

void app_panic(void)
{
    musical_keys = played_keys = audition_blocks = 0;
    for (int i=0;i<POLY;i++) calls[i] = (struct call){0};
    for (int i=0;i<RING;i++) echo_ring[i] = 0;
    memset(tide_line,0,sizeof(tide_line)); memset(cavern_line,0,sizeof(cavern_line));
    lowpass[0]=lowpass[1]=cavern_damp=0;
}

void app_init(void)
{
    for (int i=0; i<112; ++i)
        note_step[i] = (uint32_t)(440.f*fm_exp2f((i-69)/12.f)*(4294967296.f/44100.f));
    sample_init();
    select_animal(2);
    init_choir();
}

#include "tape.h"

static void input_monster(void)
{
    uint32_t now=plat_ms(), keys=plat_keys(), buttons=plat_buttons();
    uint32_t press=buttons & ~previous_buttons;
    previous_buttons=buttons;
    if (choir_mode && !shelf && (press & (1u<<B_ENV)) && !keys) touch_on(!touch_is_on());
    if (touch_is_on()) { touch_poll(); keys=touch_keys(); }
    static uint32_t rock_start[NKEYS], prior_contacts;
    for(int i=0;i<NKEYS;i++) {
        int force=touch_is_on()?touch_pressure(i):0;
        uint32_t group=voice_keys(i);
        if((group & raised) && !(raised & (1u<<i)) && (keys & group)==group) {
            if((prior_contacts & group)!=group) rock_start[i]=now;
            uint32_t age=now-rock_start[i];
            int rock=age>=700u?127:(int)(age*127u/700u);
            if(rock>force) force=rock;
        }
        finger_force[i]=force;
    }
    prior_contacts=keys;
    if(shelf && choir_mode) { choir_input(now,keys,press); return; }
    if(sample_mode && sample_origin && !sample_arm && !import_ready && transfer_id<0 &&
       (press&(1u<<B_EDIT)) && song.guest_seat==(unsigned)(sample_target+1)) {
        song.guest_hash=song.guest_seat=0; redraw=1; return;
    }
    if(sample_mode && (press&(1u<<B_SAVE)) && !sample_arm && !sample_stop &&
       !import_ready && transfer_id<0 && sample_len>=CLIP_MIN) { adopt_guest(); return; }
    if((press&(1u<<B_SEQ)) || (sample_mode && (press&(1u<<B_HOME)))) {
        if(sample_mode) {
            transfer_id=-1; sample_mode=0; sample_arm=sample_count=sample_stop=0;
            sample_pos=-1; plat_usb_audio_route(0);
            if(sample_origin || (press&(1u<<B_HOME))) { toggle_choir(); selected=sample_target; }
        } else {
            if(tape_mode==REPLAY) end_replay();
            if(tape_mode==RECORD) finish_take(now);
            sample_origin=choir_mode; sample_target=choir_mode?selected:0;
            if(choir_mode) toggle_choir();
            sample_mode=1; plat_usb_audio_route(sample_usb);
        }
        app_panic(); redraw=1; return;
    }
    if(sample_mode) { sampler_input(keys,press,now); return; }
    static uint32_t held_arp, held_env;
    if(!choir_mode) {
        if(press&(1u<<B_ARP)) held_arp=now+1;
        if(press&(1u<<B_ENV)) held_env=now+1;
        press&=~((1u<<B_ARP)|(1u<<B_ENV));
        if(held_arp && !(buttons&(1u<<B_ARP))) {
            if(now+1-held_arp>=550) { tide_arp=!tide_arp; redraw=1; }
            else press|=1u<<B_ARP;
            held_arp=0;
        }
        if(held_env && !(buttons&(1u<<B_ENV))) {
            if(now+1-held_env>=550) { if(!keys) touch_on(!touch_is_on()); }
            else press|=1u<<B_ENV;
            held_env=0;
        }
        int page=(press&(1u<<B_ENV))?CONTOUR:(press&(1u<<B_LFO))?MOTION:(press&(1u<<B_EDIT))?FILTER:VOICE;
        if(page) {
            editor=editor==page?VOICE:page; sea_page=0;
            focus=-1; redraw=1;
        }
    } else held_arp=held_env=0;
    if ((press & (1u<<B_HOME)) || (!choir_mode && (press & (1u<<B_ARP)))) {
        if (tape_mode==REPLAY) end_replay();
        if (tape_mode==RECORD) finish_take(now);
        editor=VOICE;
        toggle_choir();
    }
    if((press & (1u<<B_FX)) || (choir_mode && (press & (1u<<B_LFO)))) {
        editor=VOICE;
        sea_page=(press&(1u<<B_LFO))?1:!sea_page;
        focus=sea_page?((press&(1u<<B_LFO))?CURRENT:DEPTH):-1;
        focus_until=now+1400; redraw=1;
    }
    if (choir_mode) { choir_input(now,keys,press); return; }
    int nav=plat_enc(EN_SELECT)+plat_enc(EN_PRESET)+plat_enc(EN_ALGO);
    if (press & (1u<<B_SEL)) ++nav;
    int delta[4], gesture=nav || keys;
    for (int i=0;i<4;++i) {delta[i]=plat_enc(EN_K1+i); gesture|=delta[i]!=0;}
    if (tape_mode==REPLAY && (gesture || press)) {
        end_replay();
        press &= ~(1u<<B_PLAY);
    }
    if (nav) {
        select_animal(((animal+nav)%SPECIES+SPECIES)%SPECIES);
        focus=-1;
        if (!keys) { ++preview_serial; preview_until=now+450; }
    }

    if (press & (1u<<B_OCTDN)) register_shift=clamp(register_shift-1,-1,1);
    if (press & (1u<<B_OCTUP)) register_shift=clamp(register_shift+1,-1,1);
    if (press & (1u<<B_REC)) {
        if (tape_mode==RECORD) finish_take(now);
        else { tape_mode=RECORD; draft_count=0; prior_preview=preview_serial; redraw=1; }
    }
    if (press & (1u<<B_PLAY)) {
        if (tape_mode==RECORD) finish_take(now);
        if (take_count) begin_replay(now);
        else { ++preview_serial; preview_until=now+450; }
    }
    for (int i=0;i<4;++i) {
        int d=delta[i];
        if (d) {
            if(sea_page) adjust_sea(i,d,now);
            else if(editor) page_values()[i]=clamp(page_values()[i]+d*7,0,127);
            else knob[i]=clamp(knob[i]+d*7,0,127);
            focus=i; focus_until=now+1400; redraw=1;
            /* A child turning the first knob can hear its effect before
             * learning to hold a key with the other hand. */
            if (!keys && (int32_t)(now-preview_until)>=0) {
                ++preview_serial; preview_until=now+450;
            }
        }
    }
    if (focus>=0 && (int32_t)(now-focus_until)>=0) {focus=-1; redraw=1;}
    if (tape_mode==RECORD) capture_event(now,keys);
    if (tape_mode==REPLAY) keys=replay_event(now);
    musical_keys=keys;
    if (keys!=scanned_keys) {scanned_keys=keys; redraw=1;}
}
void app_input_only(void) { input_monster(); }

#include "stage.h"
