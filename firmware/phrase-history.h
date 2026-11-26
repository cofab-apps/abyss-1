/* One user gesture can change several tracks. Two banks let a cancelled arm
 * leave the previous edit available. Sound parameters and transport position
 * belong to the player and are deliberately outside these snapshots. */
struct abyss_phrase {
    step_t steps[NSTEP];
#if FELUCCA_AUTO
    auto_list_t events;
#endif
    int16_t length, division;
    uint8_t active;
};
static struct abyss_phrase phrase_bank[2][4];
static uint8_t history_bank, history_mask, editing_mask, recording_edit;

static void abyss_copy_phrase(struct abyss_phrase *p, const track_t *t)
{
    memcpy(p->steps, t->step, sizeof p->steps);
#if FELUCCA_AUTO
    p->events = *AUTO_L(t - trk);
#endif
    p->length = t->p[P_SLEN];
    p->division = t->p[P_SDIV];
    p->active = t->seq_active;
}

static void abyss_begin_edit(unsigned mask)
{
    editing_mask = mask;
    for (unsigned i = 0; i < 4; ++i) if (mask & (1u << i)) {
        fm1_irq_off();
        abyss_copy_phrase(&phrase_bank[history_bank ^ 1][i], &trk[i]);
        fm1_irq_on();
    }
}

static void abyss_end_edit(void)
{
    unsigned changed = 0;
    for (unsigned i = 0; i < 4; ++i) if (editing_mask & (1u << i)) {
        const struct abyss_phrase *before = &phrase_bank[history_bank ^ 1][i];
        const track_t *t = &trk[i];
        if (memcmp(before->steps, t->step, sizeof before->steps) ||
#if FELUCCA_AUTO
            memcmp(&before->events, AUTO_L(i), sizeof before->events) ||
#endif
            before->active != t->seq_active || before->length != t->p[P_SLEN] ||
            before->division != t->p[P_SDIV]) changed |= 1u << i;
    }
    if (changed) {
        history_bank ^= 1;
        history_mask = changed;
        redo_available = 0;
    }
    editing_mask = 0;
    recording_edit = 0;
}

static int abyss_swap_edit(void)
{
    if (!history_mask || editing_mask) return 0;
    for (unsigned i = 0; i < 4; ++i) if (history_mask & (1u << i)) {
        const struct abyss_phrase *p = &phrase_bank[history_bank][i];
        track_t *t = &trk[i];
        fm1_irq_off();
        abyss_copy_phrase(&phrase_bank[history_bank ^ 1][i], t);
        memcpy(t->step, p->steps, sizeof p->steps);
#if FELUCCA_AUTO
        *AUTO_L(i) = p->events;
        auto_touch(i);
#endif
        t->p[P_SLEN] = p->length;
        t->p[P_SDIV] = p->division;
        t->seq_active = p->active;
        fm1_irq_on();
    }
    history_bank ^= 1;
    redo_available ^= 1;
    return 1;
}
