/* SPDX-License-Identifier: MPL-2.0
 * Shells are cached at boot. Playback never reads flash at a bar boundary. */
static int gesture_button(int id,uint32_t press,uint32_t now)
{
    static uint32_t since[2];
    static uint8_t state[2];
    int i=id==B_GLO;
    if(press&(1u<<id)) { since[i]=now; state[i]=1; }
    if(plat_buttons()&(1u<<id)) {
        if(state[i]==1 && now-since[i]>=520) { state[i]=2; return 2; }
    } else {
        int tap=state[i]==1; state[i]=0; return tap;
    }
    return 0;
}
static uint32_t shelf_keys;
static void enter_shelf(void)
{
    stop_choir(); audition=-1; shelf=1;
    for(int k=0;k<4;k++) song.sea[k]=sea[k];
    working_song=song; building_order=-1; shelf_keys=0; shelf_message=0; redraw=1;
}
static void shelf_controls(uint32_t now,uint32_t keys,uint32_t buttons,int move)
{
    uint32_t fresh=keys&~shelf_keys;
    shelf_keys=keys;
    if(move || buttons || fresh) redraw=1;
    if(move) cursor=wrap_index(cursor+move,3);
    if(buttons&((1u<<B_GLO)|(1u<<B_HOME))) {
        shelf_stop(); shelf=0; return;
    }
    if(buttons&(1u<<B_REC)) {
        shelf_stop(); building_order=0; shelf_message=0; return;
    }
    if(buttons&(1u<<B_SAVE)) {
        shelf_stop(); shelf_message=store_shell(cursor)?1:-1;
        building_order=-1; return;
    }
    if(buttons&(1u<<B_SEL)) {
        shelf_stop();
        if(shell_used[cursor]) { use_song(&shells[cursor]); undo_mask=0; shelf=0; }
        else shelf_message=-2;
        return;
    }
    if(buttons&(1u<<B_PLAY)) {
        if(shell_playing>=0) { shelf_stop(); return; }
        building_order=-1;
        for(int i=0;i<3;i++) if(!shell_used[order[i]]) { shelf_message=-2; return; }
        chain_at=0; shell_playing=order[0]; use_song(&shells[shell_playing]); start_clock(now);
        return;
    }
    for(int i=0;i<3;i++) if(fresh&(1u<<white_ids[i])) {
        cursor=i;
        if(!shell_used[i]) { shelf_message=-2; return; }
        shelf_stop(); shelf_message=0;
        if(building_order>=0) {
            new_order[building_order++]=i;
            if(building_order==3) {
                if(plat_store_save(OBJ_ORDER,new_order,3)==0) {
                    memcpy(order,new_order,3); shelf_message=1;
                } else shelf_message=-1;
                building_order=-1;
            }
        } else { shell_playing=i; use_song(&shells[i]); start_clock(now); }
    }
}
