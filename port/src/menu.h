/* RRRMENU.EXE's main screen (menu_init 0x10e69, menu_loop 0x11bf7). */
#ifndef RC_MENU_H
#define RC_MENU_H

#include <stdint.h>

#include "archive.h"
#include "gfx8.h"
#include "reken.h"

#define RC_RRRMENU_IMAGES 72    /* RRRANIM.IMG */

enum { RC_MENU_NONE, RC_MENU_START, RC_MENU_QUIT };

/* RRRMENU's sample table (0x45164), RRRMSND entries */
enum {
    RC_MSND_CHKSND, RC_MSND_KNOPPR, RC_MSND_RC4, RC_MSND_WHIZZ,
    RC_MSND_RAPPORT, RC_MSND_BEDIEN1, RC_MSND_REDCAT, RC_MSND_TRACK1,
    RC_MSND_TRACK2, RC_MSND_RCEND, RC_MSND_MENU, RC_MSND_COUNT
};
extern const char *const rc_menu_sound_names[RC_MSND_COUNT];

/* 0x453e2: the keys Bediening accepts (PC scan code, name) */
#define RC_MENU_KEYS 46
typedef struct {
    int scan;
    const char *name;
} rc_menu_keyname;
extern const rc_menu_keyname rc_menu_keys[RC_MENU_KEYS];

typedef struct {
    rc_archive rrrmenu, rrranim;
    rc_pcx bg;                  /* menubs01.pcx */
    rc_pcx keys_bg;             /* MENUKEY.PCX */
    rc_pcx rapport_bg;          /* rapport.pcx */
    rc_pcx high_bg;             /* highscor.pcx */
    const char *save_dir;       /* where the SMD files are; "." if unset */
    rc_sprite img[RC_RRRMENU_IMAGES + 1];   /* by image number */
    rc_font font;               /* RRRFONT.IMG */
    uint8_t *font_data;

    rc_session *s;
    int world;                  /* [0x45552]: the world shown */
    int groep;                  /* [0x4555e]: groep - 3 */
    int sound;                  /* [0x45556]: RRRIO +0x30, 0 / 1 / 3 */
    int joystick;               /* [0x4555a]: RRRIO +0x34 */
    int colour;                 /* [0x45566]: font colour pair */
    int clear;                  /* the font's background not drawn */

    int state, after;
    int anim, ax, ay, frame, step, end;
    int timer, flashes, idle;
    int keys_n, keys_taken[6];  /* Bediening: table indices chosen */
    uint32_t rng;
    int mouse_x, mouse_y;
    int pen_x, pen_y;           /* Rapport's line drawing */
    int result;
    int sound_q[8], sounds;     /* menu_sound (0x10b4f) calls, in order */

    uint8_t screen[640 * 480];  /* what the menu has drawn */
    uint8_t fb[640 * 480];      /* screen with the pointer */
    rc_rgb pal[256];
} rc_menu;

int rc_menu_load(rc_menu *m, const char *dir);
void rc_menu_free(rc_menu *m);
/* menu_init(1): the record's settings, the next world after a finished
   one, the screen. */
void rc_menu_start(rc_menu *m, rc_session *s, uint32_t seed);
/* A key by its PC scan code (0x1f S, 0x1c Enter, 0x13 R, 0x12 E, 0x19 P,
   0x30 B, 0x22 G, 4..9 the digits 3..8; on the Bediening screen any of
   rc_menu_keys, 1 Esc aborting; on Rapport 0x14 T back). */
int rc_menu_key(rc_menu *m, int scan);   /* 1 if the menu used it */
void rc_menu_click(rc_menu *m, int x, int y);
void rc_menu_mouse(rc_menu *m, int x, int y);
void rc_menu_tick(rc_menu *m, int ms);
void rc_menu_draw(rc_menu *m);  /* into fb */

#endif
