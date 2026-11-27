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
    int level, key, gated, accent, species, pressure;
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
        if (!calls[i].level) { slot = i; break; }
        if (calls[i].gated < calls[slot].gated ||
            (calls[i].gated == calls[slot].gated && calls[i].age > calls[slot].age))
            slot = i;
    }
    int k = key == NKEYS ? 7 : voice_key(key);
    int note = clamp(48 + key_pitch[k] + creatures[species].transpose + 12*register_shift, 24, 98);
    calls[slot] = (struct call){.key=key, .species=species, .gated=1, .level=1,
        .step=note_step[note]};
}

static int speaker_sample(int sample,int master);
#include "sea.h"
#include "choir.h"

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
    uint32_t second = step * p->ratio;
    int pulse = 128, type = p->type;
    if (type == CHOIR) second = step + (step >> 1) + (step >> 9);
    if (type == WHALE) second = (step >> 1) + (step >> 9);
    if (type == CLACK) pulse = 28 + ((movement+126)*(movement+126) >> 9);
    /* Two damped resonances restart at each glottal closure. Their frequencies
     * follow the tract and stay put as the player moves up the keyboard. */
    static const uint16_t first[4]={285,490,825,305}, next[4]={710,1030,1460,2510};
    int f1=first[band]+(((int)first[band+1]-first[band])*blend>>7);
    int f2=next[band]+(((int)next[band+1]-next[band])*blend>>7);
    uint32_t tract=75000u+v->species%5*5600u+teeth*130u;
    uint32_t throat_step=f1*tract, nose_step=f2*tract;
    int target = v->gated ? 4096 + 12*v->pressure : 0;
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
        if (v->gated) v->level += (target-v->level+127) >> 7;
        else v->level -= (v->level+511) >> 9;
        mix[i] += body*v->level >> 7;
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

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    smooth_depth();
    int32_t mix[APP_BLOCK] = {0};
    uint32_t keys = musical_keys, fresh = keys & ~played_keys;
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
            if (v->level) {
                int32_t part[APP_BLOCK]={0};
                render_call(v,part,n,s->knobs);
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
    for (int i=0; i<POLY; ++i) if (calls[i].level) {
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
    select_animal(2);
    init_choir();
}

#include "tape.h"

static void input_monster(void)
{
    uint32_t now=plat_ms(), keys=plat_keys(), buttons=plat_buttons();
    uint32_t press=buttons & ~previous_buttons;
    previous_buttons=buttons;
    if ((press & (1u<<B_ENV)) && !keys) touch_on(!touch_is_on());
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
    if ((press & (1u<<B_HOME)) || (!choir_mode && (press & (1u<<B_ARP)))) {
        if (tape_mode==REPLAY) end_replay();
        if (tape_mode==RECORD) finish_take(now);
        toggle_choir();
    }
    if(press & ((1u<<B_FX)|(1u<<B_LFO))) {
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
