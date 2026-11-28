/* SPDX-License-Identifier: MPL-2.0 */
enum edit_page { VOICE, MOTION, CONTOUR, FILTER };
struct sculpture { uint8_t motion[4], contour[4], filter[4]; };
static struct sculpture sculpt={.contour={0,0,64,0}};
static int editor, tide_arp, arp_octave;
static uint32_t arp_samples, arp_index, arp_previous;
static const char *const page_title[]={"","LFO","ENVELOPE","FILTER + OSC"};
static const char *const parameter[][4]={
    {"","","",""}, {"RATE","VIBRATO","TREMOLO","SWEEP"},
    {"ATTACK","DECAY","SUSTAIN","RELEASE"}, {"MUFFLE","THIN","OVERTONE","SUB"}
};
static uint8_t *page_values(void)
{
    return editor==MOTION?sculpt.motion:editor==CONTOUR?sculpt.contour:sculpt.filter;
}

/* Ascending notes on the same held chord first, then their octave.
 * A short gate between steps lets repetitions retrigger even on one finger. */
static uint32_t ripple_keys(uint32_t keys,unsigned frames)
{
    if(!tide_arp || !keys) {
        arp_samples=arp_index=arp_previous=0; arp_octave=0;
        return keys;
    }
    uint32_t groups=0;
    for(int k=0;k<NKEYS;k++) if(keys&(1u<<k)) groups|=1u<<voice_key(k);
    if(groups!=arp_previous) { arp_samples=arp_index=0; arp_previous=groups; }
    int notes=__builtin_popcount(groups);
    unsigned which=arp_index%(2u*notes);
    arp_octave=which>=(unsigned)notes;
    uint32_t choice=groups;
    for(unsigned i=0;i<which%notes;i++) choice&=choice-1;
    choice&=0u-choice;
    uint32_t result=arp_samples<5100?(keys & voice_keys(__builtin_ctz(choice))):0;
    arp_samples+=frames;
    if(arp_samples>=7200) { arp_samples-=7200; ++arp_index; }
    return result;
}
