/* ABYSS-1 performance surface for the four-synth-part Optimist build. */
#include "abyss_art.h"

enum abyss_view { ORCHESTRA, PORTRAIT, MIX, STEPS };
static enum abyss_view page;
static uint8_t creature[4], cursor, paint_mask = 15, pending_save;
static uint32_t flash_until, save_until;
static int drawn_status = -1;
static int border_cache[4] = {-1, -1, -1, -1};
static unsigned step_signature = ~0u;
static uint32_t macro_until;
static uint8_t macro_owner, macro_knob, macro_value, macro_visible;

/* Apply physical pressure only to notes still held by that same key on its
 * original part. A track change during a held note leaves its expression there. */
static void abyss_sense(void)
{
    static uint32_t last;
    if (!touch_is_on() || fm1_ms - last < 5) return;
    last = fm1_ms;
    memset(abyss_pressure, 0, sizeof abyss_pressure);
    for (unsigned k = 0; k < 27; ++k) {
        if (!touch_down(k) || kb_kind[k] != KS_NOTE) continue;
        unsigned part = kb_trk[k] % NPART, pressure = touch_pressure(k);
        for (unsigned n = 0; n < kb_n[k] && n < 4; ++n) {
            uint8_t *level = &abyss_pressure[part][kb_nt[k][n] & 127];
            if (pressure > *level) *level = pressure;
        }
    }
}


static void abyss_assign(unsigned part, unsigned id)
{
    track_t *t = &trk[part];
    unsigned engine = families[id / 4].engine;
    set_engine_of(t, engine);
    if (t->eng_req == engine) {
        apply_preset_to(t, id & 3);
        creature[part] = id;
    }
}

static void abyss_restore(void)
{
    /* Fixed 16-step overdubs. An empty project otherwise starts a free-length take. */
    rec_tempo = 1;
    rec_count = 0;
    for (unsigned i = 0; i < 4; ++i) {
        unsigned f = 0;
        while (f < 4 && trk[i].eng_req != families[f].engine) ++f;
        if (f == 4 || trk[i].preset > 3) abyss_assign(i, i * 4);
        else creature[i] = f * 4 + trk[i].preset;
    }
}

/* RLE rows are independently addressable. A tile samples the native artwork
 * at every second row and column. The strip buffer is shared with the LCD DMA. */
static void abyss_picture(unsigned id, unsigned x, unsigned y, unsigned stride)
{
    unsigned width = 240 / stride;
    for (unsigned band = 0; band < width; band += 8) {
        lcd_sync();
        uint16_t *dst = cv_px;
        for (unsigned row = band; row < band + 8; ++row) {
            const uint8_t *src = abyss_art[id].run + abyss_art[id].row[row * stride];
            unsigned col = 0;
            while (col < 240) {
                unsigned code = *src++;
                unsigned end = col + 1 + (code >> 4);
                uint16_t colour = abyss_art[id].pal[code & 15];
                for (unsigned sample = (col + stride - 1) / stride; sample < (end + stride - 1) / stride; ++sample)
                    *dst++ = colour;
                col = end;
            }
        }
        lcd_blit(x, y + band, width, 8, cv_px);
    }
    lcd_sync();
}

static void abyss_frame(unsigned x, unsigned y, unsigned w, uint16_t c)
{
    lcd_fill(x, y, w, 3, c);
    lcd_fill(x, y + w - 3, w, 3, c);
    lcd_fill(x, y + 3, 3, w - 6, c);
    lcd_fill(x + w - 3, y + 3, 3, w - 6, c);
}

static void abyss_scene(enum abyss_view next)
{
    page = next;
    paint_mask = 15;
    step_signature = ~0u;
    drawn_status = -1;
}

static void abyss_input(void)
{
    uint32_t buttons = fm1_input_edges(0), keys = fm1_input_note_edges();
    hp_poll();
    abyss_sense();
    /* ENV is the optional physical-key sensing mode. Plain matrix keys remain
     * available to browsers that emulate only on/off contacts. */
    if ((buttons & BIT(B_ENV)) && !oin_keys) {
        touch_on(!touch_is_on());
        memset(abyss_pressure, 0, sizeof abyss_pressure);
    }
    if (buttons & BIT(B_FX)) {
        int next = TSEL->p[P_QUANT] == Q_SNAP ? Q_OFF : Q_SNAP;
        for (unsigned i = 0; i < 4; ++i) {
            trk[i].p[P_SCALE] = 6; /* minor pentatonic */
            trk[i].p[P_QUANT] = next;
        }
    }
    if ((buttons & BIT(B_EDIT)) && page == STEPS) {
        fm1_irq_off();
        step_wipe(TSEL, cursor, 0);
        fm1_irq_on();
    }
    enc_hold = 0;
    int select = panel_enc(EN_SELECT), family = panel_enc(EN_ALGO), variant = panel_enc(EN_PRESET);
    if (buttons || keys || select || family || variant) ui_input_ms = fm1_ms;
    if (buttons & BIT(B_HOME)) abyss_scene(ORCHESTRA);
    if (buttons & BIT(B_GLO)) abyss_scene(page == MIX ? ORCHESTRA : MIX);
    if (buttons & BIT(B_SEQ)) abyss_scene(page == STEPS ? ORCHESTRA : STEPS);
    if (buttons & BIT(B_SCL)) {
        if (page == MIX) {
            TSEL->p[P_MUTE] ^= 1;
            paint_mask |= 1u << song.sel;
        } else abyss_scene(PORTRAIT);
    }
    if (select) {
        if (page == STEPS) cursor = (cursor + select) & 15;
        else {
            track_select((song.sel + select) & 3);
            if (page == PORTRAIT) paint_mask = 15;
        }
    }
    unsigned part = song.sel, id = creature[part];
    if (family || variant) {
        unsigned f = (id / 4 + family) & 3;
        abyss_assign(part, 4 * f + ((id + variant) & 3));
        paint_mask |= 1u << part;
        if (!song.playing && !rec_wait)
            audition_notes(part, &families[f].preview, 1, 100);
    }
    for (unsigned k = 0; k < 4; ++k) {
        int d = panel_enc(EN_K1 + k);
        if (!d) continue;
        ui_input_ms = fm1_ms;
        if (page == MIX) {
            if (k == 0) TSEL->p[P_LEVEL] = clamp(TSEL->p[P_LEVEL] + d * 3, 0, 127);
            if (k == 1) song.g[G_BPM] = clamp(song.g[G_BPM] + d, 40, 240);
            paint_mask |= 1u << part;
        } else {
            unsigned m = families[creature[part] / 4].macro[k];
            const param_desc_t *desc = m >= P_E0 && m <= P_E7 ?
                &ENGINES[TSEL->eng_req]->edit[m-P_E0] : &TP[m];
            TSEL->p[m] = clamp(TSEL->p[m] + d * 3, desc->min, desc->max);
            macro_owner = part;
            macro_knob = k;
            macro_value = (TSEL->p[m] - desc->min) * 100 / (desc->max - desc->min);
            macro_until = fm1_ms + 650;
        }
    }
    if (buttons & BIT(B_PLAY)) op_play();
    if (buttons & BIT(B_REC)) (void)op_rec();
    if (buttons & BIT(B_OCTDN)) oct_tap(B_OCTDN);
    if (buttons & BIT(B_OCTUP)) oct_tap(B_OCTUP);
    if (buttons & BIT(B_SAVE)) pending_save = 1;
    if (keys) {
        flash_until = fm1_ms + 140;
        if (page == STEPS && !song.rec && !rec_wait) {
            for (unsigned key = 0; key < 27; ++key) if (keys & (1u << key)) {
                uint8_t notes[4];
                unsigned n = kb_notes(TSEL, key, notes);
                fm1_irq_off();
                for (unsigned j = 0; j < n; ++j)
                    (void)se_add(TSEL, cursor, notes[j], 1);
                fm1_irq_on();
            }
        }
    }
}

static void abyss_steps(void)
{
    unsigned bits = 0;
    for (unsigned i = 0; i < 16; ++i)
        if (step_on(&TSEL->step[i])) bits |= 1u << i;
    unsigned head = song.playing ? TSEL->seq_idx % 16 : 16;
    unsigned sig = bits | (unsigned)cursor << 16 | head << 20;
    if (sig == step_signature) return;
    step_signature = sig;
    lcd_fill(0, 194, 240, 46, 0x0862);
    for (unsigned i = 0; i < 16; ++i) {
        unsigned x = 2 + (i % 8) * 30, y = 196 + (i / 8) * 21;
        lcd_fill(x, y, 26, 15, bits & (1u << i) ? 0x5F1B : 0x2946);
        if (i == cursor) lcd_fill(x, y + 15, 26, 3, 0xFFFF);
        if (i == head) lcd_fill(x + 10, y + 3, 6, 8, 0xFFE0);
    }
}

static void abyss_draw(void)
{
    int grid = page == ORCHESTRA || page == MIX;
    int status = (song.playing ? 1 : 0) | ((song.rec || rec_wait) ? 2 : 0) |
                 ((int32_t)(save_until - fm1_ms) > 0 ? 4 : 0);
    if (status != drawn_status && drawn_status > 0) paint_mask |= 1u << (grid ? 0 : song.sel);
    int show_macro = page != STEPS && page != MIX && macro_owner == song.sel && (int32_t)(macro_until - fm1_ms) > 0;
    if (macro_visible && !show_macro) paint_mask = 15;
    macro_visible = show_macro;
    for (unsigned i = 0; i < 4; ++i) {
        if (!grid && i != song.sel) continue;
        unsigned stride = grid ? 2 : 1, w = 240 / stride;
        unsigned x = grid ? (i & 1) * 120 : 0, y = grid ? (i >> 1) * 120 : 0;
        int repaint = paint_mask & (1u << i);
        if (repaint) {
            abyss_picture(creature[i], x, y, stride);
            if (page == MIX) {
                lcd_fill(x+6,y+103,108,10,0x0862);
                unsigned level = trk[i].p[P_LEVEL] * 104 / 127;
                if (level) lcd_fill(x+8,y+105,level,6,trk[i].p[P_MUTE] ? 0xF800 : 0x57BA);
                if (trk[i].p[P_MUTE]) lcd_fill(x+52,y+99,5,17,0xFFFF);
            }
        }
        int active = trk[i].seq_n || trk[i].nheld || (i == song.sel && (int32_t)(flash_until - fm1_ms) > 0);
        int colour = active ? 0xFFE0 : i == song.sel ? 0xFFFF : 0x0862;
        if (repaint || colour != border_cache[i]) {
            abyss_frame(x,y,w,(uint16_t)colour);
            border_cache[i] = colour;
            if (page == STEPS) step_signature = ~0u;
        }
    }
    if (pending_save && flash_ok && !song.playing && !rec_wait && !transport_req && audio_quiet()) {
        proj_capture(&autosave_buf, &autosave_dl);
        if (!autosave_put(autosave_hash_of(&autosave_buf))) save_until = fm1_ms + 900;
        pending_save = 0;
    }
    if (status != drawn_status || paint_mask) {
        if (status) lcd_fill(6,6,32,12,0x0862);
        if (status & 1) for (unsigned i = 0; i < 7; ++i) lcd_fill(9+i,8+i/2,1,8-i,0x57EC);
        if (status & 2) lcd_fill(20,8,7,7,0xF800);
        if (status & 4) lcd_fill(30,8,5,7,0xFFFF);
        drawn_status = status;
    }
    if (show_macro) {
        unsigned x = grid ? (song.sel & 1) * 120 + 10 : 70;
        unsigned y = grid ? (song.sel >> 1) * 120 + 99 : 217;
        static const uint16_t colours[] = {0x5F1B, 0xFB45, 0xD4FF, 0x57BF};
        lcd_fill(x,y,100,8,0x0862);
        if (macro_value) lcd_fill(x,y,macro_value,8,colours[macro_knob]);
    }
    paint_mask = 0;
    if (page == STEPS) abyss_steps();
}

static void abyss_lights(void)
{
    ui_leds();
    fm1_led_key(panel.btn[B_HOME], page == ORCHESTRA);
    fm1_led_key(panel.btn[B_SCL], page == PORTRAIT);
    fm1_led_key(panel.btn[B_SEQ], page == STEPS);
    fm1_led_key(panel.btn[B_GLO], page == MIX);
    fm1_led_key(panel.btn[B_ENV], touch_is_on());
    fm1_led_key(panel.btn[B_FX], TSEL->p[P_QUANT] == Q_SNAP);
    fm1_led_key(panel.btn[B_REC], rec_wait || song.rec);
}
