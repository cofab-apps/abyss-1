/* SPDX-License-Identifier: MPL-2.0
 * Capture the keyboard and knobs together. The spare buffer lets a child
 * arm and cancel an empty take without losing the previous performance. */
enum { TAPE_IDLE, RECORD, REPLAY, EVENTS=512, TAKE_MS=30000 };
struct gesture_state {
    uint32_t keys;
    uint8_t animal, value[4], ocean[4], preview;
    int8_t oct;
    uint8_t arp;
    struct sculpture patch;
};
struct tape_event { uint32_t at; struct gesture_state state; };
static struct tape_event takes[2][EVENTS];
static struct gesture_state live_state, replay_state;
static int take_bank, tape_mode, draft_count, take_count, cursor;
static uint32_t tape_start, take_length;
static uint8_t prior_preview;

static struct gesture_state get_state(uint32_t keys)
{
    struct gesture_state s={.keys=keys,.animal=animal,.oct=register_shift,.arp=tide_arp,.patch=sculpt,.preview=preview_serial};
    for(int k=0;k<4;k++) { s.value[k]=knob[k]; s.ocean[k]=sea[k]; }
    return s;
}
static void set_state(const struct gesture_state *s)
{
    sculpt=s->patch; tide_arp=s->arp; animal=s->animal; register_shift=s->oct;
    for(int k=0;k<4;k++) { knob[k]=s->value[k]; sea[k]=s->ocean[k]; }
    redraw=1;
}
static void finish_take(uint32_t now)
{
    if(draft_count) {
        take_bank^=1; take_count=draft_count;
        take_length=clamp(now-tape_start,40,TAKE_MS);
    }
    tape_mode=TAPE_IDLE; redraw=1;
}
static void capture_event(uint32_t now,uint32_t keys)
{
    struct gesture_state s=get_state(keys);
    if(!draft_count) {
        if(!keys && s.preview==prior_preview) return;
        tape_start=now;
    }
    struct tape_event *draft=takes[take_bank^1];
    if(draft_count && (now-tape_start>=TAKE_MS || draft_count==EVENTS)) { finish_take(now); return; }
    if(draft_count) {
        struct gesture_state *last=&draft[draft_count-1].state;
        int same=last->keys==s.keys && last->animal==s.animal && last->oct==s.oct && last->preview==s.preview;
        for(int k=0;k<4;k++) same &= last->value[k]==s.value[k] && last->ocean[k]==s.ocean[k];
        if(same && last->arp==s.arp && !memcmp(&last->patch,&s.patch,sizeof(s.patch))) return;
    }
    draft[draft_count++]=(struct tape_event){now-tape_start,s};
}
static void begin_replay(uint32_t now)
{
    live_state=get_state(0); cursor=0; tape_start=now; tape_mode=REPLAY;
    replay_state=(struct gesture_state){0};
    redraw=1;
}
static void end_replay(void)
{
    set_state(&live_state); tape_mode=TAPE_IDLE;
}
static uint32_t replay_event(uint32_t now)
{
    uint32_t elapsed=now-tape_start;
    if(elapsed>=take_length) { end_replay(); return 0; }
    while(cursor<take_count && takes[take_bank][cursor].at<=elapsed) {
        struct gesture_state s=takes[take_bank][cursor].state;
        if((cursor && s.preview!=replay_state.preview) || (!cursor && !s.keys)) {
            ++preview_serial; preview_until=now+450;
        }
        set_state(&s); replay_state=s; ++cursor;
    }
    return replay_state.keys;
}
