/* The arithmetic part's screen (REKEN.EXE modules 1 "Handig rekenen",
   2 "Wat is waar?" and 3 "Memory"): drawn into a 640x480 8-bit frame buffer, no SDL.  The caller passes time, keys
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
    RC_KEY_0 = '0',             /* '0'..'9': digits; module 2: '1'..'3' rows */
    RC_KEY_BACK = '\b',         /* TERUG */
    RC_KEY_OK = '\r',
    RC_KEY_HELP = 'h',
    RC_KEY_STOP = 's',
};

typedef struct {
    uint8_t fb[640 * 480];
    rc_rgb pal[256];

    rc_pcx bg, bg3;             /* mod_124.pcx, module3.pcx */
    rc_font font12, font20, font30;
    rc_sprite img[RC_MENU_IMAGES + 1];  /* by image number (MENU entry + 1) */

    rc_reken r;
    int module;                 /* 1, 2, 3 */
    int state, state_ms, frame;
    char typed[12];
    int field;                  /* answer digits */
    int face_anim, face_frame;
    int prize_frame, prize;     /* the prize shown */
    int pressed_anim, pressed_x, pressed_y, pressed_ms;
    int shown_fuel, shown_repair, shown_boosts, shown_rockets;
    int stopped;
    /* module 2 */
    int bar_x;                  /* [0x53c74]: 61 for groep 3..5, else 131 */
    int bar_frame[3];           /* anims 28..30; -1 = not drawn */
    int texts_shown;
    int picked;                 /* the row answered */
    int box_value;              /* the Uitkomst box (groep 3..5); -1 empty */
    /* module 3: what each cell shows (anim, frame, its text on top) */
    int cell_anim[RC_MEMORY_CELLS], cell_frame[RC_MEMORY_CELLS];
    int cell_text[RC_MEMORY_CELLS];     /* 1 the sum, 2 the value */
    int anim_cell[2], anim_kind[2], anim_cells;     /* cards animating */
} rc_exercise;

int rc_exercise_load(rc_exercise *x, const rc_archive *reken,
                     const rc_archive *menu);
void rc_exercise_free(rc_exercise *x);

/* Start module 1, 2 or 3 with the session's groep and counters. */
void rc_exercise_start(rc_exercise *x, const rc_session *s, int module,
                       uint32_t seed);
void rc_exercise_tick(rc_exercise *x, int ms);
void rc_exercise_key(rc_exercise *x, int key);
void rc_exercise_click(rc_exercise *x, int sx, int sy);
void rc_exercise_draw(rc_exercise *x);

/* 1 after the last sum (or Stop); the counters are then in x->r. */
int rc_exercise_done(const rc_exercise *x);

#endif
