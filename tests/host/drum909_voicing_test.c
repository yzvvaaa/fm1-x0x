/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "drum909.h"
#include "fxbus.h"

#define N 256
static drum909_t a, b;
static fxbus_t f;
static int16_t delay[88200];
static float x[N], y[N], rev[N], dly[N], zero[N], l[N], r[N];

static void render(drum909_t *d, float *out, int n)
{
    memset(out, 0, N * sizeof *out);
    memset(rev, 0, sizeof rev);
    memset(dly, 0, sizeof dly);
    drum909_render(d, out, rev, dly, n);
}

static int param(int voice, const char *name)
{
    for (int i = 0; i < drum909_nparams(voice); ++i)
        if (!strcmp(drum909_param(voice, i)->name, name)) return i;
    assert(!"parameter missing");
    return -1;
}

static float band_gain(float hz)
{
    d9_biquad_t hp = a.bt[DR_SD].noise_hpf, lp = a.bt[DR_SD].noise_lpf;
    float in = 0, out = 0;
    d9_biquad_reset(&hp);
    d9_biquad_reset(&lp);
    for (int i = 0; i < 4096; ++i) {
        const float s = sinf(6.2831853f * hz * (float)i / D9_SR);
        const float v = d9_biquad_tick(&lp, d9_biquad_tick(&hp, s));
        if (i > 2048) { in += s * s; out += v * v; }
    }
    return sqrtf(out / in);
}

int main(void)
{
    drum909_init(&a);
    drum909_init(&b);
    drum909_trigger(&a, DR_SD, 1.0f);
    drum909_trigger(&b, DR_SD, 1.0f);
    b.bt[DR_SD].attack = 0;
    render(&a, x, N);
    render(&b, y, N);
    float attack = 0;
    for (int i = 0; i < N; ++i) {
        if (i < 133) attack += (x[i] - y[i]) * (x[i] - y[i]);
        else assert(x[i] == y[i]);
    }
    assert(attack > 0.00001f);
    assert(band_gain(800.0f) > 0.7f && band_gain(9000.0f) > 0.7f);

    float energy[2] = { 0, 0 };
    for (int k = 0; k < 2; ++k) {
        drum909_init(&a);
        drum909_set(&a, DR_SD, param(DR_SD, "Snappy"), 0);
        drum909_set(&a, DR_SD, param(DR_SD, "Decay"), k ? 127 : 0);
        drum909_trigger(&a, DR_SD, 1.0f);
        for (int n = 0; n < 40; ++n) {
            render(&a, x, N);
            if (n > 25) for (int i = 0; i < N; ++i) energy[k] += x[i] * x[i];
        }
    }
    assert(energy[1] > energy[0] * 100.0f + 0.00001f);
    puts("snare: short attack restored, wider noise band and body decay verified");

    drum909_init(&a);
    drum909_trigger(&a, DR_CP, 1.0f);
    int at[4], hits = 0, previous = 0;
    for (int i = 0; i < 2205; ++i) {
        render(&a, x, 1);
        if (a.clap.pulse_index != previous) {
            assert(hits < 4);
            at[hits++] = i;
            previous = a.clap.pulse_index;
        }
    }
    assert(hits == 4);
    const float gaps[] = { 9.0f, 13.0f, 17.0f };
    for (int i = 0; i < 3; ++i)
        assert(fabsf((float)(at[i + 1] - at[i]) - gaps[i] * D9_MS) <= 1.0f);
    puts("clap: four bursts with unequal spacing verified");

    for (int voice = DR_SD; voice <= DR_CP; voice += DR_CP - DR_SD) {
        for (int p = 0; p < drum909_nparams(voice); ++p) {
            for (int end = 0; end < 2; ++end) {
                drum909_init(&a);
                drum909_set(&a, voice, p, end ? drum909_param(voice, p)->max : 0);
                for (int n = 0; n < 180; ++n) {
                    if (n % 23 == 0) drum909_trigger(&a, voice, n % 2 ? 0.3f : 1.0f);
                    render(&a, x, N);
                    for (int i = 0; i < N; ++i) assert(isfinite(x[i]) && fabsf(x[i]) < 16.0f);
                }
            }
        }
    }
    puts("snare/clap: all control extremes and retriggers stay finite");

    for (int decay = 0; decay <= 127; decay += 127) {
        for (int tone = 0; tone <= 127; tone += 127) {
            fxbus_init(&f, delay, 88200);
            fxbus_set(&f, FX_RV_DECAY, decay);
            fxbus_set(&f, FX_RV_TONE, tone);
            fxbus_set(&f, FX_VOLUME, 127);
            fxbus_set(&f, FX_RV_LEVEL, 127);
            float el = 0, er = 0;
            for (int n = 0; n < 1034; ++n) {
                memset(rev, 0, sizeof rev);
                if (n == 0) rev[0] = 0.5f;
                fxbus_process_st(&f, zero, zero, rev, zero, l, r, N);
                for (int i = 0; i < N; ++i) {
                    assert(isfinite(l[i]) && isfinite(r[i]));
                    assert(fabsf(l[i]) < 1.0f && fabsf(r[i]) < 1.0f);
                    el += l[i] * l[i]; er += r[i] * r[i];
                    if (n > 1000) assert(fabsf(l[i]) < 0.000001f && fabsf(r[i]) < 0.000001f);
                }
            }
            assert(el > 0.000001f && er > el * 0.5f && er < el * 2.0f);
        }
    }
    puts("reverb: stereo impulse energy, tail decay and stability at control extremes verified");
    return 0;
}
