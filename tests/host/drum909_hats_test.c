/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "drum909.h"

#define N 256
static drum909_t d;
static float dry[N], rev[N], dly[N];

static void render(int n)
{
    memset(dry, 0, sizeof dry);
    memset(rev, 0, sizeof rev);
    memset(dly, 0, sizeof dly);
    drum909_render(&d, dry, rev, dly, n);
}

int main(void)
{
    drum909_init(&d);
    assert(d.smp[0].buf && d.smp[1].buf && d.smp[0].buf != d.smp[1].buf);
    assert(d.smp[2].buf && d.smp[3].buf);
    /* Unity playback must preserve all 16 bits before gain and FX. */
    for (int voice = DR_CH; voice <= DR_OH; ++voice) {
        drum909_init(&d);
        drum909_trigger(&d, voice, 1.0f);
        d9_smp_t *s = &d.smp[voice - DR_CH];
        const int16_t *pcm = s->buf;
        d9_env_set(&s->out, 1.0f);
        render(N);
        for (int i = 0; i < N; ++i)
            assert(dry[i] == (float)pcm[i] / 32768.0f);
        /* Last fractional frame fades into zero without reading past the buffer. */
        s->pos = s->len - 1;
        s->frac = 0x80000000u;
        render(2);
        assert(dry[0] == (float)pcm[s->len - 1] / 65536.0f);
        assert(dry[1] == 0.0f);
    }
    /* Both directions: the old hat fades over 3 ms and stops rendering by 6 ms. */
    for (int voice = DR_CH; voice <= DR_OH; ++voice) {
        drum909_init(&d);
        const int other = voice == DR_CH ? DR_OH : DR_CH;
        drum909_trigger(&d, other, 1.0f);
        render(N);
        d9_smp_t *old = &d.smp[other - DR_CH];
        const float start = old->out.cur;
        drum909_trigger(&d, voice, 1.0f);
        assert(old->out.t1 == 3.0f * 0.001f * D9_SR);
        assert(old->out.cur == start);
        render(133);
        assert(old->out.v <= 0.000011f);
        render(133);
        assert(old->mute == 0);
        const uint32_t stopped_at = old->pos;
        render(N);
        assert(old->pos == stopped_at);
        assert(d.smp[voice - DR_CH].mute > 0);
    }
    /* Every tuning extreme, overlapping hits and retriggers stay finite. */
    const int tunes[] = { 0, 64, 127 };
    for (unsigned t = 0; t < sizeof tunes / sizeof tunes[0]; ++t) {
        drum909_init(&d);
        drum909_set(&d, DR_CH, 2, tunes[t]);
        drum909_set(&d, DR_OH, 2, tunes[t]);
        for (int b = 0; b < 220; ++b) {
            if (b % 17 == 0) drum909_trigger(&d, DR_OH, 1.0f);
            if (b % 11 == 0) drum909_trigger(&d, DR_CH, 0.6f);
            render(N);
            for (int i = 0; i < N; ++i) assert(isfinite(dry[i]));
        }
    }
    printf("909 hats: PCM16, interpolation, sample ends, reciprocal choke, tuning and retriggers OK\n");
    return 0;
}
