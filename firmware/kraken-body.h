/* Kraken has two playing zones on one continuous keyboard. Low keys excite
 * a tuned membrane; the upper half brings the resonant shell noise forward.
 * Gain and pitch sweep are calculated once per 32-sample control block. */
static void abyss_kraken_render(track_t *t, voice_t *v, int32_t *out,
                               uint32_t n, const vmod_t *m)
{
    uint32_t phase = v->shell_phase, energy = v->shell_level;
    uint32_t base = pitch_inc(clamp(v->pitch_cur, 0, 2047));
    uint32_t inc = base + (base >> 10) * (energy >> 13);
    int body_gain = (int)(energy >> 8) * clamp((64-v->note)*5, 0, 100) / 100;
    int noise_gain = 80 + clamp((int)v->note-36, 0, 48) * 3;
    int32_t s1 = v->s[0], s2 = v->s[1], rng = v->s[2];
    tsvf_t shell;
    int cutoff = t->p[P_E2]*256 + ((int)v->note-48)*175 +
                 ((int)v->vel-80)*28 + m->cutoff;
    tsvf_coef(&shell, cutoff, t->p[P_E3]);
    for (uint32_t i = 0; i < n; ++i) {
        int noise = (int)(noise32(&rng) >> 17) - 16384;
        int32_t band;
        (void)tsvf_tick(&shell, noise, &s1, &s2, &band);
        int sound = (band * noise_gain) >> 7;
        if (body_gain) {
            int body = (SINE[phase >> 22] >> 1) + (SINE[(phase << 1) >> 22] >> 3);
            sound += mulq15(body, body_gain);
            phase += inc;
        }
        out[i] += VOICE_OUT(clamp(sound, -30000, 30000), amp_at(m, i), 0);
    }
    v->shell_phase = phase;
    uint32_t loss = ((energy + 2047) >> 11) * n;
    v->shell_level = loss < energy ? energy - loss : 0;
    v->s[0] = s1;
    v->s[1] = s2;
    v->s[2] = rng;
}
