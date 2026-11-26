/* Call-and-response practice on the physical keyboard. Each group of keys
 * struck together is one event. The answering creature uses its own register,
 * patch and release. Neither notes nor practice state enter the sequencer. */
#define REPLY_MAX 8
struct reply_event {
    uint32_t keys;
    uint16_t start_ms, gate_ms;
    uint8_t notes[4], count, velocity;
};
static struct {
    struct reply_event event[REPLY_MAX];
    uint32_t origin, last_up, open_keys, attack_at;
    uint8_t enabled, count, next, answering, complete, dirty, keys_were_down;
} reply;
static uint8_t reply_discovered, reply_hint_cache = 0xff;

/* A fresh, empty HOME quietly invites exploration after the player pauses.
 * The alternating fish connect the portraits to ARP's blinking lamp without
 * requiring a word or permanently covering the creature art. */
static int reply_hint_active(void)
{
    return !reply.enabled && !reply_discovered && page == ORCHESTRA &&
           !song.playing && !song.rec && !rec_wait && abyss_can_demo() &&
           fm1_ms > 5000 && fm1_ms - ui_input_ms >= 4200;
}

static void reply_reset(void)
{
    if (reply.answering) {
        /* Let the audio context end its audition, including a request which
         * has not yet reached the DAC. The live keys belong to the other part. */
        fm1_irq_off();
        an_n = 0;
        an_req = 1;
        fm1_irq_on();
    }
    reply.count = reply.next = reply.answering = reply.complete = 0;
    reply.open_keys = reply.keys_were_down = 0;
    reply.dirty = 1;
}

static void reply_close_group(uint32_t now)
{
    if (!reply.open_keys) return;
    uint32_t held = now - reply.attack_at;
    reply.event[reply.count-1].gate_ms = held < 28 ? 28 : held > 2200 ? 2200 : held;
    reply.open_keys = 0;
    reply.last_up = now;
}

/* Adjacent fingers rarely arrive on one matrix scan. Overlapping keys in
 * the first 45 ms form a chord. Releasing before the next key still makes a
 * new attack, even inside that window. Keep its original start and register. */
static void reply_add_keys(struct reply_event *e, unsigned partner, uint32_t pressed)
{
    for (unsigned k = 0; k < 27 && e->count < 4; ++k) {
        if (!(pressed & (1u << k))) continue;
        uint8_t notes[4];
        unsigned n = kb_notes(&trk[partner], k, notes);
        if (!n) continue;
        e->keys |= 1u << k;
        for (unsigned j = 0; j < n && e->count < 4; ++j) {
            unsigned i = 0;
            while (i < e->count && e->notes[i] != notes[j]) ++i;
            if (i == e->count) e->notes[e->count++] = notes[j];
        }
        unsigned v = touch_is_on() ? touch_vel(k) : 100;
        v = clamp((int)v*7/8, 1, 127);
        if (v > e->velocity) e->velocity = v;
    }
}

/* Turn a finished exchange into a one-bar duet. The child's phrase occupies
 * the first eight steps and the answer the last eight, preserving its relative
 * rhythm while fitting the fixed child-friendly loop. Only an empty song may
 * reach this function, so practice can never replace an existing creation. */
static int reply_commit(void)
{
    if (!reply.complete || !reply.count || !abyss_song_empty()) return 0;
    unsigned source = song.sel, partner = source ^ 2;
    const struct reply_event *last = &reply.event[reply.count-1];
    unsigned duration = last->start_ms + last->gate_ms;
    if (!duration) duration = 1;
    const unsigned previous_velocity = key_vel;
    abyss_begin_edit((1u << source) | (1u << partner));
    for (unsigned i = 0; i < reply.count; ++i) {
        const struct reply_event *e = &reply.event[i];
        unsigned step = ((uint32_t)e->start_ms * 8 + duration/2) / duration;
        if (step > 7) step = 7;
        unsigned length = ((uint32_t)e->gate_ms * 8 + duration/2) / duration;
        length = clamp(length, 1, 8-step);
        uint8_t source_notes[4];
        unsigned source_count = 0;
        for (unsigned k = 0; k < 27 && source_count < 4; ++k) {
            if (!(e->keys & (1u << k))) continue;
            uint8_t notes[4];
            unsigned n = kb_notes(&trk[source], k, notes);
            for (unsigned j = 0; j < n && source_count < 4; ++j) {
                unsigned q = 0;
                while (q < source_count && source_notes[q] != notes[j]) ++q;
                if (q == source_count) source_notes[source_count++] = notes[j];
            }
        }
        fm1_irq_off();
        key_vel = e->velocity;
        for (unsigned j = 0; j < source_count; ++j)
            (void)se_add(&trk[source], step, source_notes[j], length);
        for (unsigned j = 0; j < e->count; ++j)
            (void)se_add(&trk[partner], step+8, e->notes[j], length);
        key_vel = previous_velocity;
        fm1_irq_on();
    }
    abyss_end_edit();
    demo_considered = 1;
    step_signature = ~0u;
    return 1;
}

static void reply_tick(uint32_t pressed)
{
    if (!reply.enabled) return;
    uint32_t now = fm1_ms;
    unsigned partner = song.sel ^ 2;
    if (pressed) {
        if (reply.answering || reply.complete) reply_reset();
        if ((reply.open_keys & oin_keys) && now - reply.attack_at <= 45) {
            struct reply_event *e = &reply.event[reply.count-1];
            reply_add_keys(e, partner, pressed);
            reply.open_keys = e->keys;
        } else {
            reply_close_group(now);
            if (!reply.count) reply.origin = now;
            uint32_t offset = now - reply.origin;
            if (reply.count < REPLY_MAX && offset <= 8000) {
                struct reply_event *e = &reply.event[reply.count];
                *e = (struct reply_event){.start_ms = offset, .gate_ms = 28};
                reply_add_keys(e, partner, pressed);
                if (e->count) {
                    reply.open_keys = e->keys;
                    reply.attack_at = now;
                    ++reply.count;
                }
            }
        }
        reply.last_up = now;
        reply.dirty = 1;
    }
    if (reply.open_keys && !(oin_keys & reply.open_keys)) reply_close_group(now);
    if (reply.keys_were_down && !oin_keys) reply.last_up = now;
    reply.keys_were_down = oin_keys != 0;
    if (!reply.answering && !reply.complete && reply.count && !oin_keys && !reply.open_keys) {
        unsigned pause = clamp(45000 / clamp(song.g[G_BPM],40,240), 300, 620);
        if (now - reply.last_up >= pause) {
            reply.answering = reply.dirty = 1;
            reply.origin = now;
        }
    }
    if (!reply.answering) return;
    uint32_t elapsed = now - reply.origin;
    while (reply.next < reply.count && elapsed >= reply.event[reply.next].start_ms) {
        const struct reply_event *e = &reply.event[reply.next++];
        unsigned gate = ((uint32_t)e->gate_ms * FS) / (1000u * CTL);
        audition_notes_for(partner, e->notes, e->count, e->velocity, gate ? gate : 1);
    }
    const struct reply_event *last = &reply.event[reply.count-1];
    if (reply.next == reply.count && elapsed >= (unsigned)last->start_ms + last->gate_ms) {
        /* The audio audition releases the last note on its own block timer. */
        reply.answering = 0;
        reply.next = 0;
        reply.complete = reply.dirty = 1;
    }
}

/* A pair of fish marks the two participants. The filled fish has the turn. */
static void reply_badge(unsigned slot, int partner, int turn)
{
    int lit = turn == partner;
    uint16_t ink = lit ? (partner ? 0xffae : 0x5f1b) : 0x8410;
    cv_begin(24, 14, 0x0862);
    for (int y=0; y<7; ++y) {
        int half = 3 - (y>3 ? y-3 : 3-y);
        int left = 11-half, right = 15+half;
        cv_rect(partner ? 23-right : left, 3+y, right-left+1, 1, ink);
        int tail = 3-half;
        cv_rect(partner ? 18-tail : 5, 3+y, tail+1, 1, ink);
    }
    cv_rect(partner ? 7 : 15, 5, 2, 2, 0x0862);
    cv_blit((slot&1)*120+91, (slot>>1)*120+5);
}

/* The partner can sound an octave away. Show the original fingers on the
 * panel, using the same timebase and note gate as the audible answer. */
static void reply_light_keys(void)
{
    if (!reply.enabled) return;
    uint32_t keys = oin_keys;
    if (reply.answering && reply.next) {
        const struct reply_event *e = &reply.event[reply.next-1];
        if (fm1_ms - reply.origin - e->start_ms < e->gate_ms) keys |= e->keys;
    }
    for (unsigned k = 0; keys; ++k, keys >>= 1) {
        if (keys & 1u) fm1_led_key(14u + k, 1);
    }
}
