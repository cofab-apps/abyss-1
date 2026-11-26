/* ABYSS-1 performance surface for the four-synth-part Optimist build. */
#include "abyss_art.h"

enum abyss_view { ORCHESTRA, PORTRAIT, MIX, STEPS, LFO_EDIT, FX_EDIT };
static enum abyss_view page;
static uint8_t creature[4], cursor, paint_mask = 15, pending_save;
static uint32_t flash_until, save_until;
static int drawn_status = -1;
static int border_cache[4] = {-1, -1, -1, -1};
static unsigned step_signature = ~0u;
static uint32_t macro_until;
static uint8_t macro_owner, macro_knob, macro_visible;
static uint8_t editor_dirty = 1, fx_seen[4];
static const char *const family_knobs[4][4] = {
    {"FM DECAY", "FEEDBACK", "FM INDEX", "ROOM"},
    {"DECAY", "RESONANCE", "CUTOFF", "ROOM"},
    {"RESONANCE", "BUZZ", "VOWEL", "ROOM"},
    {"LOW STRINGS", "CUTOFF", "ENSEMBLE", "ROOM"}
};
static const char *const lfo_names[4] = {"SPEED", "PITCH SWING", "WAVE", "CHOP"};
static const char *const fx_names[4] = {"DRIVE", "CHORUS", "ECHO", "ROOM"};
static const uint8_t lfo_params[4] = {P_L2RATE, P_L2AA, P_L2WAVE, P_L2AB};
static const uint8_t fx_params[4] = {P_DIST, P_CHOR, P_DLY, P_REV};

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
        fx_seen[part] = 0;
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
    editor_dirty = 1;
    macro_until = 0;
    paint_mask = 15;
    step_signature = ~0u;
    drawn_status = -1;
}

/* The four knob values live in the existing per-track song parameters. Clearing
 * the two LFO destinations bypasses modulation while retaining its settings. */
static void abyss_effect_button(enum abyss_view target)
{
    int16_t *p = TSEL->p;
    if (target == LFO_EDIT) {
        if (page == target && p[P_L2DA] == LT_PIT) {
            p[P_L2DA] = p[P_L2DB] = LT_OFF;
        } else {
            if (!p[P_L2AA] && !p[P_L2AB]) {
                p[P_L2RATE] = 78;
                p[P_L2WAVE] = 2;
                p[P_L2AA] = 106;
                p[P_L2AB] = 55;
            }
            p[P_L2DA] = LT_PIT;
            p[P_L2DB] = LT_AMP;
        }
    } else {
        if (!fx_seen[song.sel] && !p[P_DIST] && !p[P_CHOR] && !p[P_DLY]) {
            p[P_DIST] = 48;
            p[P_CHOR] = 35;
            p[P_DLY] = 75;
        }
        fx_seen[song.sel] = 1;
        p[P_FXOFF] = page == target ? !p[P_FXOFF] : 0;
    }
    abyss_scene(target);
}

/* A one-bar C-minor call and response. Each hit is step, MIDI note,
 * sixteenth-note length and strike velocity. Use the user's current voices
 * and tempo so the same phrase can also audition a different quartet. */
static const struct abyss_hit {
    uint8_t part, step, pitch, length, velocity;
} abyss_demo_hits[] = {
    {0,0,48,2,110}, {0,3,48,1,75}, {0,6,55,2,96}, {0,10,46,1,82}, {0,12,43,2,105},
    {1,0,36,1,125}, {1,2,66,1,55}, {1,4,50,1,112}, {1,7,69,1,64},
    {1,8,36,1,116}, {1,10,66,1,65}, {1,12,50,1,115}, {1,15,70,1,82},
    {2,2,67,2,93}, {2,5,70,2,106}, {2,9,67,2,82}, {2,14,63,2,94},
    {3,0,60,7,76}, {3,8,55,7,78}
};
static uint8_t demo_considered;

static int abyss_can_demo(void)
{
    if (demo_considered || song.rec || rec_wait) return 0;
    for (unsigned part = 0; part < 4; ++part) {
        if (trk[part].seq_active) return 0;
        for (unsigned i = 0; i < NSTEP; ++i)
            if (step_on(&trk[part].step[i]) || trk[part].step[i].time == ST_TIE)
                return 0;
    }
    return 1;
}

static void abyss_demo(void)
{
    const unsigned previous_velocity = key_vel;
    demo_considered = 1;
    for (unsigned i = 0; i < sizeof abyss_demo_hits / sizeof abyss_demo_hits[0]; ++i) {
        const struct abyss_hit *hit = &abyss_demo_hits[i];
        fm1_irq_off();
        key_vel = hit->velocity;
        (void)se_add(&trk[hit->part], hit->step, hit->pitch, hit->length);
        key_vel = previous_velocity;
        fm1_irq_on();
    }
    step_signature = ~0u;
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
    if (buttons & BIT(B_LFO)) abyss_effect_button(LFO_EDIT);
    if (buttons & BIT(B_FX)) abyss_effect_button(FX_EDIT);
    if ((buttons & BIT(B_EDIT)) && page == STEPS) {
        demo_considered = 1;
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
        } else if (page == PORTRAIT) {
            int next = TSEL->p[P_QUANT] == Q_SNAP ? Q_OFF : Q_SNAP;
            for (unsigned i = 0; i < 4; ++i) {
                trk[i].p[P_SCALE] = 6; /* minor pentatonic */
                trk[i].p[P_QUANT] = next;
            }
            paint_mask |= 1u << song.sel;
        } else abyss_scene(PORTRAIT);
    }
    if (select) {
        if (page == STEPS) cursor = (cursor + select) & 15;
        else {
            macro_until = 0;
            editor_dirty = 1;
            track_select((song.sel + select) & 3);
            if (page == PORTRAIT) paint_mask = 15;
        }
    }
    unsigned part = song.sel, id = creature[part];
    if (family || variant) {
        unsigned f = (id / 4 + family) & 3;
        editor_dirty = 1;
        macro_until = 0;
        abyss_assign(part, 4 * f + ((id + variant) & 3));
        paint_mask |= 1u << part;
        if (!song.playing && !rec_wait)
            audition_notes(part, &families[f].preview, 1, 100);
    }
    for (unsigned k = 0; k < 4; ++k) {
        int d = panel_enc(EN_K1 + k);
        if (!d) continue;
        ui_input_ms = fm1_ms;
        if (page == LFO_EDIT || page == FX_EDIT) {
            unsigned param = (page == LFO_EDIT ? lfo_params : fx_params)[k];
            int step = page == LFO_EDIT && k == 2 ? 1 : 3;
            TSEL->p[param] = clamp(TSEL->p[param] + d * step, 0, TP[param].max);
            if (page == FX_EDIT) TSEL->p[P_FXOFF] = 0;
            else {
                TSEL->p[P_L2DA] = LT_PIT;
                TSEL->p[P_L2DB] = LT_AMP;
            }
            editor_dirty = 1;
            continue;
        }
        if (page == MIX) {
            if (k == 0) TSEL->p[P_LEVEL] = clamp(TSEL->p[P_LEVEL] + d * 3, 0, 127);
            if (k == 1) song.g[G_BPM] = clamp(song.g[G_BPM] + d, 40, 240);
            paint_mask |= 1u << part;
        } else {
            unsigned m = families[creature[part] / 4].macro[k];
            const param_desc_t *desc = m >= P_E0 && m <= P_E7 ?
                &ENGINES[TSEL->eng_req]->edit[m-P_E0] : &TP[m];
            TSEL->p[m] = clamp(TSEL->p[m] + d * 3, desc->min, desc->max);
        }
        macro_owner = part;
        macro_knob = k;
        macro_until = fm1_ms + 1600;
    }
    if (buttons & BIT(B_PLAY)) {
        if (page == STEPS && !song.playing && abyss_can_demo()) abyss_demo();
        op_play();
    }
    if (buttons & BIT(B_REC)) { demo_considered = 1; (void)op_rec(); }
    if (buttons & BIT(B_OCTDN)) oct_tap(B_OCTDN);
    if (buttons & BIT(B_OCTUP)) oct_tap(B_OCTUP);
    if (buttons & BIT(B_SAVE)) pending_save = 1;
    if (keys) {
        flash_until = fm1_ms + 140;
        if (page == STEPS && !song.rec && !rec_wait) {
            demo_considered = 1;
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

static void abyss_line(unsigned y, unsigned height, const char *left, const char *right,
                       const felucca_font_t *font, uint16_t ink);

static void abyss_steps(void)
{
    unsigned bits = 0;
    for (unsigned i = 0; i < 16; ++i)
        if (step_on(&TSEL->step[i])) bits |= 1u << i;
    unsigned head = song.playing ? TSEL->seq_idx % 16 : 16;
    unsigned sig = bits | (unsigned)cursor << 16 | head << 20;
    if (sig == step_signature) return;
    if (step_signature == ~0u || ((step_signature ^ sig) & 0xffff))
        abyss_line(172, 22, "SEQUENCER", abyss_can_demo() ? "PLAY: DEMO" : "PLAY: LOOP", &FONT_S, 0x5f1b);
    step_signature = sig;
    lcd_fill(0, 194, 240, 46, 0x0862);
    for (unsigned i = 0; i < 16; ++i) {
        unsigned x = 2 + (i % 8) * 30, y = 196 + (i / 8) * 21;
        lcd_fill(x, y, 26, 15, bits & (1u << i) ? 0x5F1B : 0x2946);
        if (i == cursor) lcd_fill(x, y + 15, 26, 3, 0xFFFF);
        if (i == head) lcd_fill(x + 10, y + 3, 6, 8, 0xFFE0);
    }
}

/* Draw a text band in strips that fit the platform's display DMA buffer. */
static void abyss_line(unsigned y, unsigned height, const char *left, const char *right,
                       const felucca_font_t *font, uint16_t ink)
{
    unsigned rows = CV_MAX / 240;
    for (unsigned r = 0; r < height; r += rows) {
        cv_begin(240, height - r < rows ? height - r : rows, 0x0862);
        cv_oy = -(int32_t)r;
        cv_text(7, 2, font, left, ink);
        if (right) cv_text(233 - text_w(font, right), 2, font, right, 0xffff);
        cv_oy = 0;
        cv_blit(0, y + r);
    }
}

static void abyss_editor(void)
{
    const int lfo = page == LFO_EDIT;
    const int on = lfo ? TSEL->p[P_L2DA] == LT_PIT : !TSEL->p[P_FXOFF];
    char title[] = "T1  LFO";
    title[1] += song.sel;
    if (!lfo) str_cpy(title + 4, "FX", 3);
    lcd_fill(0, 0, 240, 240, 0x0862);
    abyss_line(21, 28, title, on ? "ON" : "BYPASS", &FONT_L, 0x5f1b);
    for (unsigned k = 0; k < 4; k++) {
        unsigned param = (lfo ? lfo_params : fx_params)[k];
        int value = TSEL->p[param];
        char label[24] = "K1  ", formatted[24];
        const char *unit;
        label[1] += k;
        str_cpy(label + 4, (lfo ? lfo_names : fx_names)[k], 19);
        param_format(&TP[param], value, formatted, &unit);
        if (lfo && k == 0 && value >= 0) str_cpy(formatted + str_len(formatted), " Hz", 4);
        unsigned y = 55 + 38 * k;
        abyss_line(y, 24, label, formatted, &FONT_S, 0xffff);
        lcd_fill(8,y+24,224,4,0x2946);
        unsigned w = clamp(value, 0, TP[param].max) * 224 / TP[param].max;
        if (w) lcd_fill(8,y+24,w,4,on ? 0x5f1b : 0x8410);
    }
    abyss_line(215, 21, lfo ? "LFO: ON/OFF" : "FX: ON/OFF", "HOME: EXIT", &FONT_S, 0x5f1b);
}

static void abyss_legend(void)
{
    const char *const *names = family_knobs[creature[song.sel] / 4];
    for (unsigned row = 0; row < 2; row++) {
        char left[24] = "K1 ", right[24] = "K2 ";
        left[1] += row*2;
        right[1] += row*2;
        str_cpy(left+3, names[row*2], 19);
        str_cpy(right+3, names[row*2+1], 19);
        abyss_line(184+row*18,18,left,right,&FONT_S,0xffff);
    }
    abyss_line(220,20,"SCL: SCALE",TSEL->p[P_QUANT] == Q_SNAP ? "PENTA ON" : "FREE",&FONT_S,0x5f1b);
}

static void abyss_draw(void)
{
    int editor = page == LFO_EDIT || page == FX_EDIT;
    int grid = page == ORCHESTRA || page == MIX;
    int status = (song.playing ? 1 : 0) | ((song.rec || rec_wait) ? 2 : 0) |
                 ((int32_t)(save_until - fm1_ms) > 0 ? 4 : 0);
    if (status != drawn_status && drawn_status > 0) paint_mask |= 1u << (grid ? 0 : song.sel);
    int show_macro = !editor && page != STEPS && macro_owner == song.sel && (int32_t)(macro_until - fm1_ms) > 0;
    if (macro_visible && !show_macro) paint_mask = 15;
    macro_visible = show_macro;
    if (editor && editor_dirty) {
        abyss_editor();
        editor_dirty = 0;
        drawn_status = -1;
    }
    for (unsigned i = 0; !editor && i < 4; ++i) {
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
        if (status || editor) lcd_fill(6,6,32,12,0x0862);
        if (status & 1) for (unsigned i = 0; i < 7; ++i) lcd_fill(9+i,8+i/2,1,8-i,0x57EC);
        if (status & 2) lcd_fill(20,8,7,7,0xF800);
        if (status & 4) lcd_fill(30,8,5,7,0xFFFF);
        drawn_status = status;
    }
    if (page == PORTRAIT && paint_mask) abyss_legend();
    if (show_macro) {
        unsigned k = macro_knob;
        char value[16], label[26] = "K1  ";
        label[1] += k;
        if (page == MIX) {
            const char *name = k == 0 ? "LEVEL" : k == 1 ? "TEMPO BPM" : "UNASSIGNED";
            str_cpy(label + 4, name, 21);
            if (k < 2) fmt_int(value, k ? song.g[G_BPM] : TSEL->p[P_LEVEL]);
            else value[0] = 0;
        } else {
            str_cpy(label+4, family_knobs[creature[song.sel]/4][k], 21);
            fmt_int(value, TSEL->p[families[creature[song.sel]/4].macro[k]]);
        }
        abyss_line(215,25,label,value,&FONT_L,0x5f1b);
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
    fm1_led_key(panel.btn[B_FX], page == FX_EDIT && !TSEL->p[P_FXOFF]);
    fm1_led_key(panel.btn[B_LFO], TSEL->p[P_L2DA] == LT_PIT);
    fm1_led_key(panel.btn[B_REC], rec_wait || song.rec);
}
