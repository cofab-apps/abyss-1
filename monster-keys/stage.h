/* SPDX-License-Identifier: MPL-2.0
 * The portrait occupies the same place during every interaction. The lower
 * strip points at the panel: sixteen white keys and the four coloured knobs. */
#define OCEAN RGB(4,13,22)
#define FOAM RGB(220,241,237)
#define TIDE RGB(71,216,205)
#define STONE RGB(77,105,119)
static const uint16_t ink[4]={RGB(250,110,114),RGB(102,195,254),RGB(116,221,143),RGB(255,200,96)};
static const char *const effects[4]={"MOUTH","TEETH","WOBBLE","CAVE"};
static uint16_t portrait[TILE*TILE] __attribute__((section(".pool")));
static int portrait_id=-1, last_motion=-1, old_guide=-1, old_focus=-2;
static uint32_t previous_paint;

static void unpack_portrait(int id)
{
    const uint8_t *src=&monster_pixels[id*TILE*TILE*3/4];
    const uint16_t *pal=&monster_palette[id*64];
    for (int i=0; i<TILE*TILE; i+=4,src+=3) {
        portrait[i]=pal[src[0]>>2];
        portrait[i+1]=pal[((src[0]&3)<<4)|(src[1]>>4)];
        portrait[i+2]=pal[((src[1]&15)<<2)|(src[2]>>6)];
        portrait[i+3]=pal[src[2]&63];
    }
    portrait_id=id;
}

static void chevron(int x,int y,int sign,uint16_t col)
{
    for(int i=0;i<3;i++) {
        cv_line(x,y-5+i,x+sign*5,y+i,col);
        cv_line(x+sign*5,y+i,x,y+5+i,col);
    }
}

static void crown(void)
{
    const char *name=creatures[animal].name;
    cv_begin(240,25,OCEAN);
    chevron(14,11,-1,TIDE); chevron(225,11,1,TIDE);
    cv_text((240-text_w(&FONT_B,name))/2,4,&FONT_B,name,FOAM);
    cv_blit(0,0);
}

static void paint_animal(int motion)
{
    /* Render a breathing call in horizontal bands. Stretch the centre
     * against fixed edges, so the creature never swims out of its frame. */
    for (int y=0;y<TILE;y+=8) {
        int source=y + (sine_wave[y*128/TILE]*motion >> 7);
        source=clamp(source,0,TILE-8);
        lcd_sync();
        lcd_blit(36,25+y,TILE,8,&portrait[source*TILE]);
    }
}

static void bubbles(int side,int voice)
{
    cv_begin(36,54,OCEAN);
    if(voice) {
        for(int j=0;j<3;j++) {
            int h=4+(j*5+voice*4)%17, x=side?6+j*7:28-j*7;
            cv_rect(x,27-h/2,3,h,j?STONE:TIDE);
        }
    }
    cv_blit(side?204:0,83);
}

static void signet(int k)
{
    int left=k%2?204:0, top=k<2?40:144;
    int amt=editor?page_values()[k]:sea_page?sea[k]:knob[k], span=2+amt/21;
    uint16_t c=ink[k];
    cv_begin(36,36,OCEAN);
    if(editor==CONTOUR) {
        int x=7+amt/9, y=26-amt/7;
        if(k==0) cv_line(4,28,x+8,6,c);
        else if(k==1) cv_line(4,6,x+8,23,c);
        else if(k==2) cv_rect(3,y,30,2,c);
        else cv_line(4,10,x+8,28,c);
    } else if(editor==MOTION) {
        int cycles=k==0?1+amt/32:2;
        for(int x=2;x<33;x++) {
            int y=17+(sine_wave[(x*cycles*8)&255]*(k==0?11:2+amt/12)>>7);
            cv_rect(x,y,2,2,c);
        }
    } else if(editor==FILTER) {
        if(k<2) {
            int x=5+(k?amt:127-amt)/6;
            cv_line(2,k?28:8,x,8,c);
            cv_line(x,8,32,k?8:28,c);
        } else {
            for(int x=2;x<33;x++) cv_rect(x,17+(sine_wave[(x*(k==2?24:8))&255]*(3+amt/12)>>7),2,2,c);
        }
    } else if(sea_page && k==0) {
        cv_rect(5,5,26,2,c);
        int y=10+span*2;
        cv_line(18,8,18,y,c); cv_line(13,y-4,18,y,c); cv_line(18,y,23,y-4,c);
    } else if(k==0) { /* opening lips */
        for(int x=0;x<=24;x++) {
            int curve=(12-(x>12?x-12:12-x))*span/12;
            cv_rect(6+x,17-curve,1,2,c); cv_rect(6+x,18+curve,1,2,c);
        }
    } else if(k==1) { /* three teeth grow into the open jaw */
        cv_rect(5,11,26,2,c);
        for(int x=6;x<29;x+=8) {
            cv_line(x,13,x+3,17+span,c); cv_line(x+3,17+span,x+6,13,c);
        }
    } else if(k==2) { /* vibrating wave */
        for(int x=4;x<30;x++) {
            int y=18+(sine_wave[((x-4)*14)&255]*span>>7);
            cv_line(x,y,x+1,18+(sine_wave[((x-3)*14)&255]*span>>7),c);
        }
    } else { /* echoes move away from their source */
        cv_rect(5,16,3,4,c);
        for(int i=0;i<3;i++) {
            int x=10+i*6,h=3+(span+i*4)/2;
            cv_line(x,18-h,x+3,18,c);cv_line(x+3,18,x,18+h,c);
        }
    }
    if(k==0 && touch_is_on()) cv_rect(15,1,6,3,TIDE);
    if(focus==k) cv_rect(9,32,18,2,c);
    cv_blit(left,top);
}

static void guidance(int guide,uint32_t keys)
{
    cv_begin(240,47,OCEAN);
    if(editor) {
        uint8_t *values=page_values();
        cv_text(8,0,&FONT_S,page_title[editor],FOAM);
        for(int k=0;k<4;k++) {
            int x=6+58*k;
            cv_text(x,18,&FONT_T,parameter[editor][k],ink[k]);
            cv_rect(x,32,49,4,STONE); cv_rect(x,32,1+48*values[k]/127,4,ink[k]);
        }
        cv_blit(0,193); return;
    }
    for(int k=0;k<4;k++) {
        int x=11+k*59;
        cv_rect(x,3,8,6,ink[k]); cv_rect(x+2,1,4,2,ink[k]);
        cv_rect(x+10,5,36,3,STONE);
        cv_rect(x+10,5,1+(sea_page?sea[k]:knob[k])*35/127,3,ink[k]);
    }
    if(focus>=0) {
        const char *label=sea_page?sea_names[focus]:effects[focus];
        cv_text(12,17,&FONT_B,label,ink[focus]);
        int x=94, full=132;
        cv_rect(x,23,full,7,STONE);
        cv_rect(x,23,1+(sea_page?sea[focus]:knob[focus])*(full-1)/127,7,ink[focus]);
    } else {
        if(sea_page || tide_arp) cv_text(9,9,&FONT_T,sea_page?"SEA FX":"OCTAVE ARP",TIDE);
        for(int w=0;w<16;w++) {
            int key=white_ids[w], x=8+w*14;
            int active=(keys>>key)&1u;
            if(key+1<NKEYS && (raised&(1u<<(key+1)))) active|=(keys>>(key+1))&1u;
            cv_rect(x,19,12,24,active?ink[3]:(!keys && guide==w?TIDE:FOAM));
            if(key+1<NKEYS && (raised&(1u<<(key+1))))
                cv_rect(x+8,19,4,12,(keys & (1u<<(key+1)))?ink[0]:OCEAN);
        }
        int x=14+guide*14;
        if(!keys) { cv_line(x-4,12,x,16,TIDE); cv_line(x,16,x+4,12,TIDE); }
    }
    if(tape_mode==RECORD) cv_rect(2,13,4,5,ink[0]);
    if(tape_mode==REPLAY) chevron(3,13,1,TIDE);
    cv_blit(0,193);
}

void app_ui_init(void)
{
    lcd_fill(0,0,240,240,OCEAN);
    redraw=1;
}
static int grid_visible, grid_id[SEATS]={-2,-2,-2,-2}, grid_light[SEATS];
static uint16_t thumb[84*84] __attribute__((section(".pool")));
static void paint_seat(int i,int id,int light)
{
    int x=8+(i%2)*119, y=19+(i/2)*91;
    int marked=echo_game ? i==turn_seat(step_index) : i==selected;
    uint16_t border=marked?ink[3]:STONE;
    if(light) border=TIDE;
    int marker=x+91;
    lcd_fill(marker,y,12,84,OCEAN);
    if(song.guest_seat==(unsigned)(i+1)) {
        uint16_t c=guest_signature!=song.guest_hash?ink[0]:light?FOAM:TIDE;
        lcd_fill(marker,y+9,10,2,c); lcd_fill(marker+2,y+5,2,8,c);
        if(guest_signature==song.guest_hash) lcd_fill(marker+6,y+2,2,14,c);
        else lcd_fill(marker+4,y+15,3,3,c);
    }
    if(strum && id!=EMPTY) lcd_fill(marker,y+19,9,3,ink[i]);
    if(marked) {
        lcd_fill(marker,y+35,3,13,ink[3]); lcd_fill(marker+3,y+38,3,7,ink[3]);
    }
    if(light) {
        int opening=2+light;
        lcd_fill(marker,y+61-opening,9,2,TIDE);
        lcd_fill(marker,y+61+opening,9,2,TIDE);
    }
    lcd_fill(x-3,y-3,90,2,border); lcd_fill(x-3,y+84,90,2,border);
    lcd_fill(x-3,y-1,2,85,border); lcd_fill(x+84,y-1,2,85,border);
    if(id!=grid_id[i]) {
        if(id==EMPTY) {
            lcd_fill(x,y,84,84,OCEAN); lcd_fill(x+28,y+40,28,4,STONE);
            lcd_fill(x+40,y+28,4,28,STONE);
        } else {
            const uint8_t *src=monster_pixels+id*TILE*TILE*3/4;
            for(int row=0;row<84;row++) for(int col=0;col<84;col++) {
                int off=(row*2*TILE+col*2)*3/4;
                int index=col%2?((src[off]&15)<<2 | src[off+1]>>6):src[off]>>2;
                thumb[row*84+col]=monster_palette[id*64+index];
            }
            lcd_sync(); lcd_blit(x,y,84,84,thumb); lcd_sync();
        }
        grid_id[i]=id;
    }
    grid_light[i]=light;
}
static void paint_choir(void)
{
    const struct choir_sound *voices=sound_frames[sound_front];
    for(int i=0;i<SEATS;i++) {
        int id=song.seats[i].species;
        int light=voices[i].note?1+voices[i].pressure/24:0;
        if(redraw || id!=grid_id[i] || light!=grid_light[i]) paint_seat(i,id,light);
    }
    cv_begin(240,15,OCEAN);
    const char *label=echo_game?(step_index<16?"TOP ROW CALLS":"BOTTOM REPLIES"):
        strum?"STRUM THE CHOIR":recording>=0?"RECORD":armed>=0?"READY":running?"CHOIR LOOP":sea_page?"SEA FX":"SEAMONSTER CHOIR";
    cv_text(6,0,&FONT_S,label,FOAM);
    if(save_pending || favourite_status) cv_text(167,0,&FONT_S,
        save_pending?"SAVE ...":favourite_status==2?"RECALLED":favourite_status>0?"SAVED":"NO SAVE",ink[3]);
    if(echo_game) {
        int turn=turn_seat(step_index);
        for(int i=0;i<SEATS;i++) cv_rect(136+i*6,5,4,4,i==turn?ink[3]:STONE);
        chevron(116,6,step_index<16?1:-1,TIDE);
    }
    cv_blit(0,0);
    cv_begin(240,42,OCEAN);
    const struct syllable *line=recording==selected?draft:song.seats[selected].line;
    for(int j=0;j<STEPS;j++) {
        int x=5+(j%16)*14+(j%16)/4, y=1+(j/16)*11;
        int note=echo_game ? (j<16 && recording>=0 ? draft[j].note :
            song.seats[turn_seat(j)].line[j].note) : line[j].note;
        cv_rect(x,y,11,7,j==step_index && running?ink[3]:note?TIDE:STONE);
    }
    if(echo_game) cv_text(5,25,&FONT_S,step_index<16?"PLAY YOUR CALL":"LISTEN TO THE REPLY",FOAM);
    else if(strum) {
        cv_text(5,25,&FONT_S,"HOLD LOW",FOAM);
        for(int i=0;i<4;i++) cv_rect(129+i*22,24,16,15,
            song.seats[i].species==EMPTY?STONE:(rung&(1u<<i))?FOAM:ink[i]);
    } else if(song.guest_seat && guest_signature!=song.guest_hash) {
        cv_text(5,25,&FONT_S,"GUEST MISSING   SEQ + SAVE",ink[0]);
    } else if(sea_page) {
        int k=focus>=0?focus:DEPTH;
        cv_text(5,25,&FONT_S,sea_names[k],ink[k]);
        cv_rect(85,31,143,4,STONE); cv_rect(85,31,1+sea[k]*142/127,4,ink[k]);
    } else if(!running && armed<0) {
        const char *guide=plat_ms()/2500%2?"HOLD EDIT STRUM / GLO SHELLS":"SEL + SINGER    SELECT: SEAT";
        cv_text(5,25,&FONT_S,guide,FOAM);
    }
    else cv_text(5,25,&FONT_S,"REC    PLAY    ARP",FOAM);
    cv_blit(0,198);
    plat_lights((running?LED_PLAY_GREEN:0)|((armed>=0||recording>=0)?1u<<B_REC:0),
                musical_keys,0,strum?pad_keys():0,2);
    redraw=0;
}
static void paint_shelf(void)
{
    static const char *const letters[]={"A","B","C"};
    cv_begin(240,46,OCEAN);
    cv_text(7,4,&FONT_B,"SONGS IN SHELLS",FOAM);
    cv_text(7,28,&FONT_S,building_order>=0?"PICK THREE SHELLS":"SELECT A SHELL",TIDE);
    cv_blit(0,0);
    cv_begin(240,62,OCEAN);
    for(int i=0;i<3;i++) {
        int x=12+i*79; uint16_t col=shell_used[i]?ink[i]:STONE;
        /* A scallop, a pointed conch and a spiral remain distinct in grayscale. */
        if(i==0) {
            for(int k=0;k<5;k++) {
                int y=15+(k-2)*(k-2)*3;
                cv_line(x+25,46,x+k*12,y,col);
                if(k) cv_line(x+(k-1)*12,15+(k-3)*(k-3)*3,x+k*12,y,col);
            }
        } else if(i==1) {
            cv_line(x+3,45,x+42,11,col); cv_line(x+42,11,x+46,36,col);
            cv_line(x+46,36,x+3,45,col);
            for(int k=1;k<5;k++) cv_line(x+3+k*8,45-k*7,x+9+k*8,44-k*2,col);
        } else {
            static const int8_t spiral[][2]={{46,43},{27,48},{9,43},{4,29},{12,17},
                {30,14},{41,22},{41,34},{30,40},{18,35},{17,26},{26,22},{32,28},{27,32}};
            for(unsigned k=1;k<sizeof(spiral)/sizeof(*spiral);k++)
                cv_line(x+spiral[k-1][0],spiral[k-1][1],x+spiral[k][0],spiral[k][1],col);
        }
        cv_text(x+20,2,&FONT_S,letters[i],FOAM);
        if(i==cursor) cv_rect(x,52,50,3,FOAM);
        if(i==shell_playing) cv_rect(x,58,1+49*step_index/31,3,TIDE);
    }
    cv_blit(0,46);
    cv_begin(240,56,OCEAN);
    cv_text(8,3,&FONT_S,"ORDER",FOAM);
    for(int i=0;i<3;i++) {
        int slot=building_order>=0?new_order[i]:order[i];
        cv_text(89+45*i,1,&FONT_B,building_order>=0 && i>=building_order?"-":letters[slot],ink[slot]);
    }
    cv_text(8,29,&FONT_S,shelf_message==-2?"EMPTY SHELL":shelf_message<0?"SAVE FAILED":
            shelf_message>0?"SAVED":shell_playing>=0?"SINGING":"LOW KEYS PREVIEW",TIDE);
    cv_blit(0,108);
    cv_begin(240,66,OCEAN);
    cv_text(7,2,&FONT_S,"SAVE STORE      SEL LOAD",FOAM);
    cv_text(7,23,&FONT_S,"REC ORDER       PLAY SONGS",FOAM);
    cv_text(7,44,&FONT_S,"GLO BACK",FOAM); cv_blit(0,172);
    uint32_t pads=0;
    for(int i=0;i<3;i++) if(shell_used[i]) pads|=1u<<white_ids[i];
    plat_lights(running?LED_PLAY_GREEN:0,0,0,pads,2);
    redraw=0;
}
static int sample_visible;
static void paint_sampler(void)
{
    static const char *const modes[]={"PITCHED KEYS","BACKWARDS","FOUR SLICES"};
    cv_begin(240,48,OCEAN);
    cv_text(8,3,&FONT_B,"SHELL SAMPLER",FOAM);
    cv_text(8,26,&FONT_S,modes[sample_style],TIDE);
    cv_blit(0,0);
    cv_begin(240,65,OCEAN);
    int size=sample_len, bank=sample_bank;
    for(int x=0;x<224;x++) {
        int peak=0, lo=x*size/224, hi=(x+1)*size/224;
        for(int j=lo;j<hi;j+=4) {
            int v=clips[bank][j]; if(v<0) v=-v;
            if(v>peak) peak=v;
        }
        int h=clamp(peak/400,1,27);
        cv_rect(8+x,31-h,1,h*2,sample_style==CLIP_CHOPS?ink[x/56]:TIDE);
    }
    if(sample_pos>=0 && size) cv_rect(8+clamp(sample_pos*223/size,0,223),1,2,60,FOAM);
    cv_blit(0,56);
    cv_begin(240,63,OCEAN);
    const char *status=transfer_id>=0?"RECEIVING SAMPLE":sample_arm?(sample_count?"RECORDING":"WAITING FOR SOUND"):"REC TO CATCH A SOUND";
    cv_text(8,2,&FONT_B,status,sample_arm?ink[0]:FOAM);
    cv_rect(8,24,224,5,STONE);
    cv_rect(8,24,1+(transfer_id>=0?transfer_count*223/transfer_length:(sample_arm?sample_count:size)*223/CLIP_MAX),5,TIDE);
    cv_text(8,38,&FONT_S,sample_usb?"SOURCE: USB AUDIO":creatures[animal].name,ink[1]);
    cv_blit(0,126);
    cv_begin(240,42,OCEAN);
    cv_text(8,1,&FONT_S,sample_origin && song.guest_seat==(unsigned)(sample_target+1)?
            "EDIT REMOVE GUEST":"SEL SOURCE     ARP STYLE",FOAM);
    cv_text(8,22,&FONT_S,"SAVE >",ink[3]);
    int target=sample_origin?song.seats[sample_target].species:initialized?song.seats[0].species:animal;
    cv_text(72,22,&FONT_S,creatures[target==EMPTY?animal:target].name,FOAM);
    cv_blit(0,196);
    plat_lights((sample_arm?(1u<<B_REC):0)|(sample_pos>=0?LED_PLAY_GREEN:0),musical_keys,0,0,0);
    redraw=0;
}
void app_frame(void)
{
    input_monster();
    uint32_t now=plat_ms();
    if(now-previous_paint<65 && !redraw) return;
    previous_paint=now;
    if(sample_visible!=sample_mode) {
        sample_visible=sample_mode; lcd_fill(0,0,240,240,OCEAN);
        grid_visible=-1; portrait_id=-1; redraw=1;
    }
    if(sample_mode) { paint_sampler(); return; }
    int stage=shelf?2:choir_mode;
    if(stage!=grid_visible) {
        grid_visible=stage; lcd_fill(0,0,240,240,OCEAN);
        portrait_id=-1; redraw=1;
        for(int i=0;i<SEATS;i++) grid_id[i]=-2;
    }
    if(shelf) { paint_shelf(); return; }
    if(choir_mode) { paint_choir(); return; }
    int guide=4+(int)((now/800u)%3u)*2;
    int singing=musical_keys || (int32_t)(preview_until-now)>0;
    unsigned force=0;
    for(int k=0;k<NKEYS;k++) if((musical_keys&(1u<<k)) && finger_force[k]>force) force=finger_force[k];
    int motion=singing?force/24+2+(int)((now/(110-knob[2]/2))%4):0;
    int fresh=portrait_id!=animal;
    if(fresh) {unpack_portrait(animal); crown();}
    if(fresh || motion!=last_motion) {
        paint_animal(motion); bubbles(0,motion); bubbles(1,motion);
        last_motion=motion;
    }
    if(redraw || fresh || guide!=old_guide || focus!=old_focus) {
        if(redraw || fresh || focus!=old_focus) for(int k=0;k<4;k++) signet(k);
        guidance(guide,musical_keys);
        uint32_t lights=tape_mode==RECORD?(1u<<B_REC):tape_mode==REPLAY?LED_PLAY_GREEN:0;
        plat_lights(lights,musical_keys,LED_PLAY_GREEN,musical_keys?0:1u<<white_ids[guide],2);
        old_guide=guide; old_focus=focus; redraw=0;
    }
}

void app_say(const char *a,const char *b)
{
    cv_begin(240,25,OCEAN);
    int x=cv_text(5,4,&FONT_B,a?a:"",FOAM);
    if(b) cv_text(x+4,4,&FONT_S,b,TIDE);
    cv_blit(0,0);
    portrait_id=-1;
}
