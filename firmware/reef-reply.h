/* Call-and-response practice on the physical keyboard. Each group of keys
 * struck together is one event. The answering creature uses its own register,
 * patch and release. Neither notes nor practice state enter the sequencer. */
#define REPLY_MAX 8
struct reply_event {
    uint16_t start_ms, gate_ms;
    uint8_t notes[4], count, velocity;
};
static struct {
    struct reply_event event[REPLY_MAX];
    uint32_t origin, last_up, open_keys, attack_at;
    uint8_t enabled, count, next, answering, dirty, keys_were_down;
} reply;

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
    reply.count = reply.next = reply.answering = 0;
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

static void reply_tick(uint32_t pressed)
{
    if (!reply.enabled) return;
    uint32_t now = fm1_ms;
    unsigned partner = song.sel ^ 2;
    if (pressed) {
        if (reply.answering) reply_reset();
        reply_close_group(now);
        if (reply.count < REPLY_MAX) {
            struct reply_event *e = &reply.event[reply.count];
            unsigned count = 0, velocity = 0;
            for (unsigned k = 0; k < 27 && count < 4; ++k) if (pressed & (1u << k)) {
                uint8_t pitched[4];
                unsigned n = kb_notes(&trk[partner], k, pitched);
                for (unsigned j = 0; j < n && count < 4; ++j) {
                    unsigned i = 0;
                    while (i < count && e->notes[i] != pitched[j]) ++i;
                    if (i == count) e->notes[count++] = pitched[j];
                }
                unsigned v = touch_is_on() ? touch_vel(k) : 100;
                if (v > velocity) velocity = v;
            }
            if (count) {
                if (!reply.count) reply.origin = now;
                uint32_t offset = now - reply.origin;
                if (offset <= 8000) {
                    e->count = count;
                    e->velocity = clamp((int)velocity*7/8, 1, 127);
                    e->start_ms = offset;
                    e->gate_ms = 28;
                    reply.open_keys = pressed;
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
    if (!reply.answering && reply.count && !oin_keys && !reply.open_keys) {
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
        reply_reset();
    }
}

/* A pair of fish marks the two participants. The filled fish has the turn. */
static void reply_badge(unsigned slot, int partner)
{
    int lit = reply.answering == partner;
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
