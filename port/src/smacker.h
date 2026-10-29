/* A Smacker 2 (SMK2) decoder, taken unchanged from the A2 Racer II port
 * (../a2, port/src/menu/msmacker.c), where it matches ffmpeg frame for
 * frame.  Here it plays the game's films: rrrintro.smk (INTRO.IMG, 320x200,
 * 1119 frames, 15 fps), w{redcat,brutus,snitch}.smk (RCS, 320x200, 160
 * frames) and eindani2.smk (RRRMENU, 640x480, 20 frames at 20 fps), all
 * without an audio track; ffmpeg decodes all five identically.
 *
 * The format: the header's four Huffman "big trees" (MMAP, MCLR, FULL,
 * TYPE) with their three-entry code cache, palette deltas, block types
 * skip / mono / full / fill, and Huffman-DPCM audio.
 */
#ifndef RC_SMACKER_H
#define RC_SMACKER_H

#include <stddef.h>
#include <stdint.h>

typedef struct smk smk;

smk *smk_open(const uint8_t *data, size_t size); /* data must outlive it */
void smk_close(smk *s);
int smk_width(const smk *s);
int smk_height(const smk *s);
int smk_frames(const smk *s);
double smk_frame_ms(const smk *s);
/* Audio track 0: rate, channels, bits (8 or 16); 0 rate if none. */
void smk_audio_info(const smk *s, int *rate, int *channels, int *bits);

/* Decode the next frame into the internal buffers.  Returns 0, or -1 at the
 * end or on a damaged frame. */
int smk_next(smk *s);
const uint8_t *smk_pixels(const smk *s); /* width * height indices */
const uint8_t *smk_palette(const smk *s); /* 256 * RGB */
/* This frame's audio for track 0, in the track's own format (u8 or s16le). */
const uint8_t *smk_audio(const smk *s, size_t *bytes);

#endif /* RC_SMACKER_H */
