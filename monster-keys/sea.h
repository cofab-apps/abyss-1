/* SPDX-License-Identifier: MPL-2.0
 * The four sea controls act on the whole instrument. Process the stereo
 * voice mix before the master limiter. All delay lengths are at 44.1 kHz. */
enum { DEPTH, ANGER, CURRENT, CAVERN };
static volatile uint8_t sea[4];
static int sea_page, depth_q8, lowpass[2], cavern_damp;
static uint32_t tide_phase;
static unsigned sea_pos;
static int16_t tide_line[512], cavern_line[8192];
static const char *const sea_names[4]={"DEPTH","ANGER","CURRENT","CAVERN"};

static void adjust_sea(int k,int d,uint32_t now)
{
    sea[k]=clamp(sea[k]+7*d,0,127);
    focus=k; focus_until=now+1400; redraw=1;
}
static void smooth_depth(void)
{
    int diff=sea[DEPTH]*256-depth_q8;
    depth_q8+=diff/16+(diff>0)-(diff<0);
}
static void sea_render(int32_t *lr,unsigned n,int master)
{
    int depth=depth_q8>>8, anger=sea[ANGER], current=sea[CURRENT], cavern=sea[CAVERN];
    /* At the bottom, pitch is an octave lower and the filter approaches 500 Hz. */
    int light=127-depth, cutoff=18+(light*light*238/16129);
    unsigned speed=14000+current*360;
    int delay0=128+sine_wave[tide_phase>>24]*current/150;
    tide_phase+=speed*n;
    int delay1=128+sine_wave[tide_phase>>24]*current/150;
    int delay=delay0*256, delta=(delay1-delay0)*256/(int)n;
    for(unsigned i=0;i<n;i++) {
        int left=lr[2*i], right=lr[2*i+1];
        if(depth) {
            lowpass[0]+=(left-lowpass[0])*cutoff/256;
            lowpass[1]+=(right-lowpass[1])*cutoff/256;
            left=lowpass[0]; right=lowpass[1];
        } else { lowpass[0]=left; lowpass[1]=right; }
        if(anger) {
            int gain=128+anger*5, limit=6600-anger*25;
            left+=(clamp(left*gain/128,-limit,limit)-left)*anger/128;
            right+=(clamp(right*gain/128,-limit,limit)-right)*anger/128;
        }
        int mono=(left+right)/2;
        tide_line[sea_pos&511]=clamp(mono,-30000,30000);
        if(current) {
            unsigned at=sea_pos-(delay>>8); int frac=delay&255;
            int tap=tide_line[at&511]+((tide_line[(at-1)&511]-tide_line[at&511])*frac>>8);
            left+=tap*current/192; right-=tap*current/224;
        }
        delay+=delta;
        int far=cavern_line[(sea_pos-7877)&8191], near=cavern_line[(sea_pos-4931)&8191];
        cavern_damp+=(far+near-2*cavern_damp)/8;
        cavern_line[sea_pos&8191]=clamp((mono+cavern_damp)*cavern/148,-24000,24000);
        left+=(near+cavern_line[(sea_pos-1451)&8191])*cavern/256;
        right+=(far+cavern_line[(sea_pos-2243)&8191])*cavern/256;
        sea_pos++;
        lr[2*i]=speaker_sample(left,master);
        lr[2*i+1]=speaker_sample(right,master);
    }
}
