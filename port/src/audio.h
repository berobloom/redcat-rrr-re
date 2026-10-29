/* The sound library RRR, RRRMENU and REKEN share ("sndman.c" over
   "sndbuf.c" and the Sound Blaster interrupt "irq7.c"; RRR 0x10303..0x1043a,
   0x1cd3f, 0x1cd62, 0x1ec03, the mixer 0x1db94).  A fixed number of tracks
   each play one sample once.  research/discoveries.md "Sound". */
#ifndef RC_AUDIO_H
#define RC_AUDIO_H

#include <stdint.h>

#include "archive.h"

#define RC_AUDIO_TRACKS 4

typedef struct {
    char name[21];
    const uint8_t *pcm;         /* the data chunk, 8-bit unsigned */
    uint32_t len;
    int stereo;                 /* entry +0x20: channels > 1 (0x1daa0) */
} rc_wave;

typedef struct {
    rc_wave *wave;
    int count;
} rc_soundbank;

/* The named samples of a named archive (SNDRCS, RRRMSND, REKENSND), in
   the order of names[] (the program's table); a missing one has no data.
   The bank points into the archive. */
int rc_soundbank_load(rc_soundbank *b, const rc_archive *a,
                      const char *const *names, int count);
void rc_soundbank_free(rc_soundbank *b);

typedef struct {
    const rc_soundbank *bank;
    int tracks;                 /* RRR 4, RRRMENU and REKEN 2 ([0x5fe67]) */
    int rate;                   /* the card's rate: 11025, REKEN 22050 */
    struct {
        const rc_wave *w;
        int sample;             /* -1 free */
        uint32_t pos;
    } t[RC_AUDIO_TRACKS];
    uint32_t phase;
    int16_t held;
} rc_mixer;

/* A new set-up, every track free. */
void rc_mixer_setup(rc_mixer *m, const rc_soundbank *bank, int tracks,
                    int rate);
/* 0x1cd62: start sample on a track if the track is free; 0 if started. */
int rc_mixer_play(rc_mixer *m, int track, int sample);
void rc_mixer_stop(rc_mixer *m, int track);         /* 0x1ce02 */
int rc_mixer_busy(const rc_mixer *m, int track);    /* !0x1cd3f */
/* The card's output resampled to out_rate, signed 16-bit mono. */
void rc_mixer_mix(rc_mixer *m, int16_t *out, int frames, int out_rate);

#endif
