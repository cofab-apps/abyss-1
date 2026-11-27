/* SPDX-License-Identifier: MPL-2.0 */
enum { SEATS=4, STEPS=32, STEP_MS=150, EMPTY=255 };
struct syllable { uint8_t note, bend, pressure, strike, knobs[4]; };
struct singer { uint8_t species, knobs[4]; struct syllable line[STEPS]; };
struct arrangement { uint32_t format; struct singer seats[SEATS]; uint8_t sea[4]; };
static const uint32_t choir_format=0x314b434d; /* MCK1 */
static struct arrangement song;
static struct syllable draft[STEPS], undo_lines[SEATS][STEPS];
static unsigned undo_mask;
static int selected, step_index, running, armed=-1, recording=-1;
static int echo_game, reply_ready, nonempty, initialized, favourite_status;
static uint32_t next_step, old_keys, preview_end;
static int recent_key, audition=-1, save_pending;
static volatile uint8_t choir_mode, finger_force[NKEYS];
struct choir_sound { uint8_t note, species, knobs[4], bend, pressure, attack; };
static struct choir_sound sound_frames[2][SEATS];
static volatile uint8_t sound_front;
static uint8_t attack_count[SEATS], last_attack[SEATS];
static int wrap_index(int v,int limit) { return ((v%limit)+limit)%limit; }

static void assign_singer(int seat,int id)
{
    const struct species *p=&creatures[id];
    struct singer *s=&song.seats[seat];
    s->species=id;
    s->knobs[0]=p->mouth; s->knobs[1]=p->teeth;
    s->knobs[2]=p->wobble; s->knobs[3]=p->cave;
}
static void invite_singer(void)
{
    static const int favourites[SEATS]={2,9,1,6};
    unsigned present=0;
    for(int i=0;i<SEATS;i++)
        if(song.seats[i].species!=EMPTY) present|=1u<<song.seats[i].species;
    for(int n=0;n<SEATS;n++) {
        int i=(selected+n)%SEATS;
        if(song.seats[i].species!=EMPTY) continue;
        int id=favourites[i];
        while(present&(1u<<id)) id=(id+1)%SPECIES;
        assign_singer(i,id); selected=i;
        return;
    }
}
static int recall_favourite(void)
{
    static struct arrangement saved;
    int bytes=plat_store_load(OBJ_FAVOURITE,&saved,sizeof(saved));
    if(bytes==(int)sizeof(saved)-4) memset(saved.sea,0,sizeof(saved.sea));
    else if(bytes!=(int)sizeof(saved)) return 0;
    for(int k=0;k<4;k++) if(saved.sea[k]>127) return 0;
    if (saved.format!=choir_format || saved.seats[0].species>=SPECIES) return 0;
    for(int i=0;i<SEATS;i++) {
        const struct singer *s=&saved.seats[i];
        if(s->species!=EMPTY && s->species>=SPECIES) return 0;
        for(int k=0;k<4;k++) if(s->knobs[k]>127) return 0;
        for(int j=0;j<STEPS;j++) {
            const struct syllable *v=&s->line[j];
            if(v->note>=112 || v->pressure>127 || v->bend>1 || v->strike>1) return 0;
            for(int k=0;k<4;k++) if(v->knobs[k]>127) return 0;
        }
    }
    song=saved;
    for(int k=0;k<4;k++) sea[k]=saved.sea[k];
    return 1;
}
static void init_choir(void)
{
    if(recall_favourite()) { initialized=1; return; }
    memset(&song,0,sizeof(song)); song.format=choir_format;
    for(int i=0;i<SEATS;i++) song.seats[i].species=EMPTY;
}
static void finish_lane(void)
{
    if(recording>=0 && nonempty) {
        undo_mask=1u<<recording;
        memcpy(undo_lines[recording],song.seats[recording].line,sizeof(draft));
        memcpy(song.seats[recording].line,draft,sizeof(draft));
    }
    armed=recording=-1; nonempty=0;
}
static void answer_turn(void);
static void stop_choir(void)
{
    if(echo_game && recording>=0) answer_turn(); else finish_lane();
    running=echo_game=0; audition=-1; step_index=0; redraw=1;
}
static void toggle_choir(void)
{
    stop_choir();
    if(!initialized) { assign_singer(0,animal); initialized=1; }
    choir_mode=!choir_mode;
    selected=0; old_keys=0;
    musical_keys=0; redraw=1;
}
static void begin_lane(int seat)
{
    memset(draft,0,sizeof(draft)); nonempty=0; recording=seat; armed=-1;
}
/* In each row a lone singer gets the whole bar. With both seats filled
 * each gets two beats, left then right. The lower row echoes the upper. */
static int turn_seat(int step)
{
    int first=step<16?0:2;
    int preferred=first+(step%16>=8);
    return song.seats[preferred].species==EMPTY ? preferred^1 : preferred;
}
static void answer_turn(void)
{
    reply_ready=nonempty;
    if(nonempty) {
        undo_mask=0;
        for(int i=0;i<SEATS;i++) {
            struct singer *s=&song.seats[i];
            if(s->species==EMPTY) continue;
            memcpy(undo_lines[i],s->line,sizeof(draft));
            undo_mask|=1u<<i;
            memset(s->line,0,sizeof(s->line));
        }
        for(int j=0;j<16;j++) {
            struct singer *caller=&song.seats[turn_seat(j)];
            struct singer *reply=&song.seats[turn_seat(j+16)];
            caller->line[j]=reply->line[j+16]=draft[j];
            memcpy(reply->line[j+16].knobs,reply->knobs,4);
        }
    }
    recording=armed=-1; nonempty=0;
}
static void advance_step(void)
{
    step_index=(step_index+1)%STEPS;
    if(echo_game && step_index==16) answer_turn();
    if(step_index==0) {
        int waiting=armed;
        finish_lane();
        if(echo_game) begin_lane(turn_seat(0));
        else if(waiting>=0) begin_lane(waiting);
    }
    for(int i=0;i<SEATS;i++) if(song.seats[i].line[step_index].strike) ++attack_count[i];
}
static void start_clock(uint32_t now)
{
    running=1; step_index=0; next_step=now+STEP_MS;
    if(armed>=0) begin_lane(armed);
    for(int i=0;i<SEATS;i++) ++attack_count[i];
}
static void start_game(void)
{
    stop_choir();
    if(song.seats[2].species==EMPTY && song.seats[3].species==EMPTY)
        assign_singer(2,song.seats[0].species==9?2:9);
    reply_ready=0;
    echo_game=1; armed=turn_seat(0);
}
static void choir_input(uint32_t now,uint32_t keys,uint32_t buttons)
{
    int move=plat_enc(EN_SELECT), browse=plat_enc(EN_PRESET)+plat_enc(EN_ALGO);
    if(move && recording<0 && armed<0 && !echo_game) {
        selected=wrap_index(selected+move,SEATS);
        audition=-1;
    }
    if((buttons&(1u<<B_SEL)) && !running && armed<0 && !echo_game) {
        invite_singer();
        audition=-1;
    }
    struct singer *seat=&song.seats[selected];
    if(!echo_game && browse) {
        int id=seat->species==EMPTY?0:seat->species;
        assign_singer(selected,wrap_index(id+browse,SPECIES));
        audition=seat->species; preview_end=now+280; ++attack_count[selected];
    }
    for(int k=0;k<4;k++) {
        int d=plat_enc(EN_K1+k);
        if(!d) continue;
        if(sea_page) adjust_sea(k,d,now);
        else if(seat->species!=EMPTY) seat->knobs[k]=clamp(seat->knobs[k]+7*d,0,127);
        redraw=1;
    }
    if(buttons&(1u<<B_OCTDN)) register_shift=clamp(register_shift-1,-1,1);
    if(buttons&(1u<<B_OCTUP)) register_shift=clamp(register_shift+1,-1,1);
    if(buttons&(1u<<B_ARP)) { if(echo_game) stop_choir(); else start_game(); }
    if(buttons&(1u<<B_PLAY)) {
        if(running || echo_game) stop_choir(); else start_clock(now);
    }
    if((buttons&(1u<<B_REC)) && !echo_game && seat->species!=EMPTY) {
        if(recording>=0 || armed>=0) finish_lane(); else armed=selected;
    }
    if((buttons&(1u<<B_EDIT)) && !echo_game && recording<0 && undo_mask) {
        for(int i=0;i<SEATS;i++) if(undo_mask&(1u<<i))
            memcpy(song.seats[i].line,undo_lines[i],sizeof(draft));
        undo_mask=0;
    }
    if((buttons&(1u<<B_GLO)) && !running && armed<0 && !keys) {
        if(recall_favourite()) {
            audition=-1; undo_mask=0;
            favourite_status=2;
        } else favourite_status=-1;
    }
    if(buttons&(1u<<B_SAVE)) { save_pending=1; favourite_status=0; }
    if(save_pending && !running && !keys && armed<0) {
        for(int k=0;k<4;k++) song.sea[k]=sea[k];
        favourite_status=plat_store_save(OBJ_FAVOURITE,&song,sizeof(song))==0?1:-1;
        save_pending=0;
    }
    if(armed>=0 && !running && keys) start_clock(now);
    int boundary=0;
    while(running && (int32_t)(now-next_step)>=0) {
        next_step+=STEP_MS; advance_step(); boundary=1;
    }
    uint32_t fresh=keys & ~old_keys;
    if(keys && (fresh || !(keys & (1u<<recent_key)))) {
        uint32_t pick=fresh?fresh:keys;
        for(int k=0;k<NKEYS;k++) if(pick&(1u<<k)) recent_key=k;
    }
    int key=voice_key(recent_key);
    int strike=keys && !(old_keys & voice_keys(key));
    struct syllable live={.note=keys?48+key_pitch[key]+12*register_shift:0,
        .bend=!!(keys & raised & voice_keys(key)), .pressure=finger_force[key],
        .strike=strike};
    int turn=echo_game?turn_seat(step_index):selected;
    memcpy(live.knobs,song.seats[turn].knobs,4);
    if(recording>=0 && keys) {
        struct syllable *cell=&draft[step_index];
        live.strike |= !cell->note && (!old_keys || strike);
        int attack=cell->strike || live.strike;
        *cell=live; cell->strike=attack; nonempty=1;
    }
    if(audition>=0 && (int32_t)(now-preview_end)>=0) audition=-1;
    unsigned back=sound_front^1;
    static const int intervals[4]={0,7,4,-12};
    int root=live.note;
    for(int i=0;i<SEATS;i++) {
        struct singer *s=&song.seats[i];
        struct syllable val={0};
        int id=s->species;
        memcpy(val.knobs,s->knobs,4);
        if(echo_game) {
            if(running && i==turn) {
                if(step_index<16) val=live;
                else if(reply_ready) val=s->line[step_index];
            }
        } else if(running && id!=EMPTY) {
            val=s->line[step_index];
            if(i==recording || (i==selected && keys)) val=live;
        } else if(keys && id!=EMPTY) {
            val=live; val.note=root+intervals[i];
            if(i==2 && (root%12==2 || root%12==4 || root%12==9)) --val.note;
            memcpy(val.knobs,s->knobs,4);
        }
        if(strike && (echo_game ? i==turn && step_index<16 : !running || i==selected)) ++attack_count[i];
        if(!echo_game && i==selected && audition>=0 && !keys) {
            id=audition; val.note=60;
            const struct species *p=&creatures[id];
            val.knobs[0]=p->mouth; val.knobs[1]=p->teeth; val.knobs[2]=p->wobble; val.knobs[3]=p->cave;
        }
        struct choir_sound *out=&sound_frames[back][i];
        out->note=id==EMPTY?0:val.note; out->species=id==EMPTY?0:id;
        out->pressure=val.pressure; out->bend=val.bend; out->attack=attack_count[i];
        memcpy(out->knobs,val.knobs,4);
    }
    __asm__ volatile("" ::: "memory");
    sound_front=back;
    if(boundary || keys!=old_keys || move || browse || buttons) redraw=1;
    old_keys=keys; musical_keys=keys;
}
