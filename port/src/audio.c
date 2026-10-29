/* The Sound Blaster mixer of sndbuf.c / irq7.c (RRR 0x1db94; RRRMENU and
   REKEN have the same code).  For each byte the card plays: every track
   adds its next sample byte, a free track 0x80, and the sum is divided by
   the number of tracks.  A stereo sample gives the mean of a byte pair.
   The volume a sample is started with (entry +0xe) is never read, so
   play_fx's distance does nothing; the rate in the WAV header is not used
   either (turbo.wav, 8403 Hz, plays at the card's rate). */
#include <stdlib.h>
#include <string.h>

#include "audio.h"

static uint32_t get32(const uint8_t *d)
{
    return d[0] | d[1] << 8 | d[2] << 16 | (uint32_t)d[3] << 24;
}

/* RIFF WAVE: the fmt chunk's channel count and the data chunk. */
static int parse_wav(rc_wave *w, const uint8_t *d, uint32_t size)
{
    uint32_t at = 12;
    int channels = 0;

    if (size < 12 || memcmp(d, "RIFF", 4) != 0 || memcmp(d + 8, "WAVE", 4))
        return -1;
    while (at + 8 <= size) {
        uint32_t n = get32(d + at + 4);
        if (memcmp(d + at, "fmt ", 4) == 0 && n >= 4 && at + 12 <= size)
            channels = d[at + 10] | d[at + 11] << 8;
        else if (memcmp(d + at, "data", 4) == 0) {
            if (n > size - at - 8)
                n = size - at - 8;
            w->pcm = d + at + 8;
            w->len = n;
            w->stereo = channels > 1;
            return 0;
        }
        at += 8 + n + (n & 1);
    }
    return -1;
}

int rc_soundbank_load(rc_soundbank *b, const rc_archive *a,
                      const char *const *names, int count)
{
    int i;

    b->wave = calloc((size_t)count, sizeof *b->wave);
    if (!b->wave) {
        b->count = 0;
        return -1;
    }
    b->count = count;
    for (i = 0; i < count; i++) {
        uint32_t size;
        const uint8_t *d = rc_archive_find(a, names[i], &size);
        strncpy(b->wave[i].name, names[i], sizeof b->wave[i].name - 1);
        if (d)
            parse_wav(&b->wave[i], d, size);
    }
    return 0;
}

void rc_soundbank_free(rc_soundbank *b)
{
    free(b->wave);
    b->wave = NULL;
    b->count = 0;
}

void rc_mixer_setup(rc_mixer *m, const rc_soundbank *bank, int tracks,
                    int rate)
{
    int i;

    m->bank = bank;
    m->tracks = tracks < 1 ? 1 : tracks > RC_AUDIO_TRACKS ? RC_AUDIO_TRACKS
                                                          : tracks;
    m->rate = rate;
    for (i = 0; i < RC_AUDIO_TRACKS; i++) {
        m->t[i].w = NULL;
        m->t[i].sample = -1;
        m->t[i].pos = 0;
    }
    m->phase = 0;
    m->held = 0;
}

int rc_mixer_busy(const rc_mixer *m, int track)
{
    return track >= 0 && track < m->tracks && m->t[track].w != NULL;
}

int rc_mixer_play(rc_mixer *m, int track, int sample)
{
    const rc_wave *w;

    if (!m->bank || track < 0 || track >= m->tracks || m->t[track].w ||
        sample < 0 || sample >= m->bank->count)
        return -1;
    w = &m->bank->wave[sample];
    if (!w->pcm)
        return -1;
    m->t[track].w = w;
    m->t[track].sample = sample;
    m->t[track].pos = 0;
    return 0;
}

void rc_mixer_stop(rc_mixer *m, int track)
{
    if (track >= 0 && track < RC_AUDIO_TRACKS) {
        m->t[track].w = NULL;
        m->t[track].sample = -1;
        m->t[track].pos = 0;
    }
}

/* one byte of the card's output (0x1dbaf..0x1dd1f) */
static int next_byte(rc_mixer *m)
{
    unsigned sum = 0;
    int i;

    for (i = 0; i < m->tracks; i++) {
        const rc_wave *w = m->t[i].w;
        uint32_t p = m->t[i].pos;

        if (w && p >= w->len)
            rc_mixer_stop(m, i);        /* mode 1: played once */
        if (!m->t[i].w) {
            sum += 0x80;
            continue;
        }
        if (w->stereo && p + 1 < w->len) {
            sum += (w->pcm[p] + w->pcm[p + 1]) >> 1;
            m->t[i].pos += 2;
        } else {
            sum += w->pcm[p];
            m->t[i].pos += w->stereo ? 2 : 1;
        }
    }
    return (int)(sum / (unsigned)m->tracks);
}

void rc_mixer_mix(rc_mixer *m, int16_t *out, int frames, int out_rate)
{
    int i;

    for (i = 0; i < frames; i++) {
        m->phase += (uint32_t)m->rate;
        while (m->phase >= (uint32_t)out_rate) {
            m->phase -= (uint32_t)out_rate;
            m->held = (int16_t)((next_byte(m) - 0x80) << 8);
        }
        out[i] = m->held;
    }
}
