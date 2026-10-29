/* The arithmetic part's screen (REKEN.EXE module 1, Handig rekenen): drawn
   into a 640x480 8-bit frame buffer, no SDL.  The caller passes time, keys
   and clicks, and shows fb with pal.

   Layout and timing are the original's (research/discoveries.md "The
   arithmetic part: screen"). */
#ifndef RC_REKEN_SCREEN_H
#define RC_REKEN_SCREEN_H

#include <stdint.h>

#include "archive.h"
#include "gfx8.h"
#include "reken.h"

#define RC_MENU_IMAGES 479

enum {                          /* rc_exercise_key */
    RC_KEY_0 = '0',             /* '0'..'9': digits */
    RC_KEY_BACK = '\b',         /* TERUG */
    RC_KEY_OK = '\r',
    RC_KEY_HELP = 'h',
    RC_KEY_STOP = 's',
};

typedef struct {
    uint8_t fb[640 * 480];
    rc_rgb pal[256];

    rc_pcx bg;                  /* mod_124.pcx */
    rc_font font12, font30;
    rc_sprite img[RC_MENU_IMAGES + 1];  /* by image number (MENU entry + 1) */

    rc_reken r;
    int state, state_ms, frame;
    char typed[12];
    int field;                  /* answer digits */
    int face_anim, face_frame;
    int prize_frame;
    int pressed_anim, pressed_x, pressed_y, pressed_ms;
    int shown_fuel, shown_repair, shown_boosts, shown_rockets;
    int stopped;
} rc_exercise;

int rc_exercise_load(rc_exercise *x, const rc_archive *reken,
                     const rc_archive *menu);
void rc_exercise_free(rc_exercise *x);

/* Start module 1 with the session's groep and counters. */
void rc_exercise_start(rc_exercise *x, const rc_session *s, uint32_t seed);
void rc_exercise_tick(rc_exercise *x, int ms);
void rc_exercise_key(rc_exercise *x, int key);
void rc_exercise_click(rc_exercise *x, int sx, int sy);
void rc_exercise_draw(rc_exercise *x);

/* 1 after the last sum (or Stop); the counters are then in x->r. */
int rc_exercise_done(const rc_exercise *x);

#endif
