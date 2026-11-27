/* SPDX-License-Identifier: MPL-2.0 */
enum { CLIP_RATE=11025, CLIP_MAX=2*CLIP_RATE, CLIP_MIN=64 };
enum { CLIP_KEYS, CLIP_BACK, CLIP_CHOPS };
static int16_t clips[2][CLIP_MAX] __attribute__((section(".pool")));
static volatile int sample_mode, sample_usb, sample_style, sample_arm, sample_stop, sample_audition;
static volatile int sample_bank, sample_len, sample_count, sample_pos=-1;
static int sample_origin;
#include "sample-transfer.h"

/* The render loop owns the live bank. A silent or aborted pass leaves the
 * previous clip playable. The editor and its waveform only read that bank. */
static void finish_sample(void)
{
    if(sample_count>=CLIP_MIN) {
        sample_len=sample_count; sample_bank^=1;
    }
    sample_arm=sample_count=0;
}

static void sample_init(void)
{
    /* A short bubbling chirp makes the keys playable before the first take. */
    uint32_t phase=0;
    int size=CLIP_RATE/2;
    for(int i=0;i<size;i++) {
        int local=i%(size/3), pitch=340+local/6;
        phase+=(uint32_t)(4294967296.0/CLIP_RATE)*pitch;
        int level=(size-i)*48/size;
        clips[0][i]=sine_wave[phase>>24]*level*(size/3-local)/(size/3);
    }
    sample_len=size;
}

static void capture_clip(const int32_t *pcm,unsigned n)
{
    if(!pcm) return;
    int bank=sample_bank^1;
    for(unsigned i=0;i+3<n;i+=4) {
        int total=0;
        for(unsigned j=0;j<8;j++) total+=pcm[2*i+j]/256;
        int v=clamp(total/8,-32768,32767);
        if(!sample_count && v<80 && v>-80) continue;
        clips[bank][sample_count++]=v;
        if(sample_count==CLIP_MAX) { finish_sample(); return; }
    }
}

static void render_sampler(int32_t *lr,unsigned n,int master)
{
    static uint32_t prior_keys;
    static int prior_audition, cursor=-1, increment, begin, end, attack;
    uint32_t keys=musical_keys, fresh=keys&~prior_keys;
    prior_keys=keys;
    if(sample_stop) { finish_sample(); cursor=-1; sample_stop=0; }
    if(sample_arm) {
        cursor=sample_pos=-1;
        const int32_t *pcm;
        if(sample_usb) {
            pcm=plat_usb_audio_in();
            for(unsigned i=0;i<2*n;i++) lr[i]=pcm?(pcm[i]/256)*master/16:0;
        } else {
            render_monsters(lr,n,4096); pcm=lr;
        }
        capture_clip(pcm,n);
        if(!sample_usb) for(unsigned i=0;i<2*n;i++) lr[i]=(lr[i]/256)*master/16;
        return;
    }
    if(sample_pos<0) cursor=-1;
    if(fresh || prior_audition!=sample_audition) {
        int key=0;
        for(int k=0;k<NKEYS;k++) if(fresh&(1u<<k)) key=k;
        prior_audition=sample_audition;
        int pitch=key_pitch[key]+12*register_shift;
        begin=0; end=sample_len;
        if(sample_style==CLIP_CHOPS) {
            int white=0;
            while(white<15 && white_ids[white+1]<=key) white++;
            begin=(white%4)*sample_len/4; end=((white%4)+1)*sample_len/4;
            pitch=12*register_shift;
        }
        increment=(int)(note_step[48+pitch]/(note_step[48]/1024u));
        if(sample_style==CLIP_BACK) { increment=-increment; cursor=(end-1)*4096; }
        else cursor=begin*4096;
        attack=0;
    }
    smooth_depth();
    int rate=increment-(increment*(depth_q8>>8))/256;
    const int16_t *clip=clips[sample_bank];
    for(unsigned i=0;i<n;i++) {
        int dry=0;
        if(cursor>=begin*4096 && cursor<end*4096) {
            int pos=cursor>>12, a=clip[pos], b=pos+1<end?clip[pos+1]:a;
            int v=a+((b-a)*(cursor&4095)>>12);
            int edge=increment<0?pos-begin:end-1-pos;
            int fade=clamp(edge*4,0,64);
            if(attack<64) ++attack;
            if(fade>attack) fade=attack;
            dry=v*fade/128;
            cursor+=rate;
        } else cursor=-1;
        lr[2*i]=lr[2*i+1]=dry;
    }
    sample_pos=cursor<0?-1:cursor>>12;
    sea_render(lr,n,master);
}

static void sampler_input(uint32_t keys,uint32_t press,uint32_t now)
{
    if((press&(1u<<B_REC)) && !import_ready) {
        transfer_id=-1;
        if(sample_arm) sample_stop=1;
        else { sample_pos=-1; sample_arm=1; }
    }
    if((press&(1u<<B_SEL)) && !sample_arm) {
        sample_usb=!sample_usb; plat_usb_audio_route(sample_usb);
    }
    if(press&(1u<<B_ARP)) { sample_style=(sample_style+1)%3; sample_pos=-1; }
    if(press&(1u<<B_PLAY)) ++sample_audition;
    if(press&(1u<<B_OCTDN)) register_shift=clamp(register_shift-1,-1,1);
    if(press&(1u<<B_OCTUP)) register_shift=clamp(register_shift+1,-1,1);
    int move=plat_enc(EN_SELECT)+plat_enc(EN_PRESET)+plat_enc(EN_ALGO);
    if(move) select_animal(wrap_index(animal+move,SPECIES));
    for(int k=0;k<4;k++) {
        int delta=plat_enc(EN_K1+k);
        if(delta) adjust_sea(k,delta,now);
    }
    if(keys!=musical_keys || press) redraw=1;
    musical_keys=keys;
}
