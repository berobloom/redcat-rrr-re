/* The arithmetic part's screen: see reken_screen.h.  Addresses are
   REKEN.EXE's.  The original draws into the screen as things change; this
   redraws the whole frame each time, which shows the same picture. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "anims.h"
#include "reken_screen.h"

const char *const rc_reken_sound_names[RC_RSND_COUNT] = {
    "click.wav", "fuel.wav", "fout1.wav", "fouttun1.wav", "doing.wav",
    "noise.wav", "whizz.wav", "knoppr.wav", "roll.wav", "yiha.wav", "rc3.wav",
    "rc4.wav", "rc5.wav", "rc6.wav", "rc7.wav", "rc8.wav", "rc9.wav",
    "rc10.wav", "rc11.wav", "01.wav", "02.wav", "03.wav", "04.wav", "05.wav",
    "06.wav", "07.wav", "08.wav", "09.wav", "10.wav", "11.wav", "12.wav",
    "13.wav", "14.wav", "15.wav", "16.wav", "17.wav", "18.wav", "19.wav",
    "20.wav", "21.wav", "22.wav", "23.wav", "24.wav", "25.wav", "26.wav",
    "27.wav", "28.wav", "29.wav", "30.wav", "31.wav", "32.wav", "33.wav",
    "34.wav", "35.wav", "36.wav", "37.wav", "38.wav", "39.wav", "40.wav",
    "41.wav", "42.wav", "43.wav", "44.wav", "45.wav", "46.wav", "47.wav",
    "48.wav", "49.wav", "50.wav", "51.wav", "52.wav", "53.wav", "54.wav",
    "55.wav", "56a.wav", "56b.wav", "57.wav", "58.wav", "59.wav", "60.wav",
    "61.wav", "62.wav", "63.wav", "64.wav", "65.wav", "66.wav", "67.wav",
    "68.wav", "69.wav", "70.wav", "71.wav", "72.wav", "73.wav", "74.wav",
    "75.wav", "76.wav", "77.wav", "78.wav", "79.wav", "80.wav"
};

#define SCREEN_W 640
#define SCREEN_H 480

/* Anims (table 0x4660c) and where module 1 draws them */
#define A_DIGIT_SMALL 1         /* boosts and rockets, 0x18d03 */
#define A_DIGIT_FUEL 3          /* fuel, 0x18dad */
#define A_REPAIR 4              /* repair bar, 0x18c17 */
#define A_FACE_RIGHT 13         /* 0x4680c +0x18 */
#define A_FACE_WRONG 15         /* 0x467f0 +0x18 */
#define A_HELP 17
#define A_OK 18
#define A_BACK 19
#define A_PANEL 32              /* sum panel, 0x111c1 */
#define A_KEY_1 33              /* keys 1..9: 33..41, 0: 42 (0x110e2) */
#define A_TITLE 43              /* module title bar, frame = module - 1 */
#define A_BOX 22                /* module 2 Uitkomst box (0x195e3) */
#define A_BAR 28                /* module 2 rows: 28, 29, 30 (0x11fcc) */
#define A_DIGIT_FUEL3 2         /* module 3's fuel digits */
#define A_CARD_SOM 5            /* module 3 cards; frame 0 face down, 6 up */
#define A_SHAKE_SOM 6           /* a card refused (0x13a16) */
#define A_CARD_GONE 7           /* a pair found (0x13d95) */
#define A_BACK_SOM 8            /* a pair turned back (0x13d95) */
#define A_BACK_ANTWOORD 9
#define A_CARD_ANTWOORD 10
#define A_SHAKE_ANTWOORD 11
#define A_EQUALS 16             /* module 4 (0x14fe3) */
#define A_AGAIN 20              /* OPNIEUW */
#define A_SLOT 21               /* the number slots, frame = slot */
#define A_PANEL4 23             /* the work panel */
#define A_PLUS 24
#define A_TIMES 25
#define A_MINUS 26
#define A_DIVIDE 27

#define PANEL_X 8
#define PANEL_Y 168
#define PANEL_W 624             /* 0x11cfd centres the sum in 0x270 x 0xa8 */
#define PANEL_H 168
#define FACE_X 51               /* centre */
#define FACE_Y 83
#define PRIZE_X 230             /* prize records 0x46828.. +8, +0xc */
#define PRIZE_Y 100
#define PRIZE_FRAMES 20
#define FACE_FRAMES 16

#define CELL_BK 0x36            /* 0x17f7b(0x36): typed digits and blanks */
#define CURSOR_BK 0x90          /* 0x17f81(0x90): the cell typed next */

/* prize [0x46c86] -> anim (records 0x46844, 0x46828, 0x4687c, 0x46860,
   0x46898, 0x468b4, 0x468d0, +0x18) */
static const int prize_anim[7] = { 47, 45, 51, 49, 53, 55, 57 };

static const int key_xy[10][2] = {      /* 0x110e2; index = digit */
    { 413, 416 }, { 17, 351 }, { 105, 351 }, { 193, 351 }, { 281, 351 },
    { 369, 351 }, { 61, 416 }, { 149, 416 }, { 237, 416 }, { 325, 416 },
};
#define OK_X 502                /* 0x195e3 */
#define OK_Y 424
#define BACK_X 502
#define BACK_Y 368
#define HELP_X 564              /* 0x11e23 */
#define HELP_Y 3

/* Module 2: rows centred at (bar_x + 182, bar_y), the sum in the 342 x 41
   box from (bar_x + 22, text_y) (0x12ee0), the result in 132 x 144 from
   (452, 267) (0x13229) */
#define BAR_DX 182
#define BAR_FRAMES 6
static const int bar_y[3] = { 227, 316, 404 };
static const int text_y[3] = { 208, 297, 385 };
#define TEXT_DX 22
#define TEXT_W 342
#define TEXT_H 41
/* Module 3 puts the face, the prize and the gauges in a column on the
   right (each record's first two words) and anims one lower */
#define FACE3_X 577
#define FACE3_Y 90
#define PRIZE3_X 568
#define PRIZE3_Y 224
#define CARD_FRAMES 7
#define SHAKE_FRAMES 5
#define GONE_FRAMES 8
#define BACK_FRAMES 4
/* Module 4: slots at (8, 168 + 48 i), the number centred in 182 x 41 from
   (30, 169 + 48 i); the work panel (224, 168), 280 x 185, its lines from
   y = 178, 20 apart; the Uitkomst box (502, 168), the target in 132 x 144
   from (502, 209) (0x161d3) */
#define SLOT_X 8
#define SLOT_Y 168
#define SLOT_DY 48
#define WORK_X 224
#define WORK_Y 168
#define WORK_W 280
#define WORK_H 185
#define BOX4_X 502
#define BOX4_Y 168
#define BUTTON4_X 503           /* TERUG, OPNIEUW (0x195e3) */
#define BOX_X 452
#define BOX_Y 226
#define RESULT_Y 267
#define RESULT_W 132
#define RESULT_H 144

/* Mouse buttons (0x24741's tables: rectangles and the key each stands
   for) */
typedef struct { int key, x0, y0, x1, y1; } button;

static const button buttons1[] = {      /* 0x45060 */
    { '0', 413, 416, 469, 472 }, { '1', 17, 351, 73, 407 },
    { '2', 105, 351, 161, 407 }, { '3', 193, 351, 249, 407 },
    { '4', 281, 351, 337, 407 }, { '5', 369, 351, 425, 407 },
    { '6', 61, 416, 117, 472 }, { '7', 149, 416, 205, 472 },
    { '8', 237, 416, 293, 472 }, { '9', 325, 416, 381, 472 },
    { RC_KEY_HELP, 564, 3, 632, 33 }, { RC_KEY_BACK, 502, 368, 634, 416 },
    { RC_KEY_OK, 502, 424, 634, 472 }, { RC_KEY_STOP, 10, 6, 78, 35 },
};
static const button buttons2[] = {      /* 0x453fc; the rows' x is bar_x */
    { '1', 0, 207, 342, 248 }, { '2', 0, 296, 342, 337 },
    { '3', 0, 384, 342, 425 },
    { RC_KEY_HELP, 564, 3, 632, 33 }, { RC_KEY_STOP, 10, 4, 78, 33 },
};
static const button buttons4[] = {      /* 0x457d4; = and x, : patched in */
    { '1', 8, 168, 190, 209 }, { '2', 8, 216, 190, 257 },
    { '3', 8, 264, 190, 305 }, { '4', 8, 312, 190, 353 },
    { RC_KEY_PLUS, 8, 368, 70, 416 }, { RC_KEY_MINUS, 8, 425, 70, 473 },
    { RC_KEY_HELP, 564, 3, 632, 33 }, { RC_KEY_BACK, 503, 368, 635, 416 },
    { RC_KEY_STOP, 10, 6, 78, 35 }, { RC_KEY_AGAIN, 503, 424, 635, 472 },
    { RC_KEY_OK, 0, 368, 62, 474 },
    { RC_KEY_TIMES, 78, 368, 140, 416 }, { RC_KEY_DIVIDE, 78, 425, 140, 473 },
};
#define BUTTON4_EQUALS 10
#define BUTTON4_TIMES 11
static const button buttons3[] = {      /* 0x4550c; the grid is patched in */
    { RC_KEY_HELP, 564, 3, 632, 33 }, { RC_KEY_STOP, 10, 6, 78, 35 },
};

/* Delays: 0x249f3(ms) waits until clock() (hundredths, from INT 21h
   AH=2Ch) has gone on by ms; that clock moves with the BIOS tick, 18.2 a
   second, so each wait ends on a tick and an animation's frames come one
   tick apart at least (DOSBox-X: module 2's rows close one frame a tick) */
#define TICK_MS 55
#define DELAY(ms) (((ms) * 10 + 548) / 549 * TICK_MS)  /* whole ticks */
#define FRAME_MS DELAY(10)      /* prize and face animations (0x18e0b) */
#define PRESS_MS DELAY(150)     /* a button shown pressed (0x19842) */
#define UNIT_MS DELAY(200)      /* one fuel / boost / rocket unit (0x1910f) */
#define END_MS DELAY(1000)
#define BAR_MS DELAY(5)         /* 0x1294b, 0x12b59, 0x129c6 */
#define SHOW_MS DELAY(1000)     /* the true row after the prize (0x13229);
                                   module 3's pair after a miss (0x14967) */
#define CARD_MS DELAY(5)        /* 0x13abb, 0x13a16, 0x13d95 */
#define BACK_MS DELAY(40)       /* 0x13d95 */

/* REKENSND sample numbers */
#define S_CLICK 0
#define S_FUEL 1
#define S_FOUT 2
#define S_DOING 4
#define S_NOISE 5
#define S_WHIZZ 6
#define S_KNOPPR 7
#define S_ROLL 8
#define S_YIHA 9

/* 0x19a50: what each module says first (0x4693c: count, REKENSND
   number - 18), phrase by phrase; Esc, Enter or S stop it */
static const int intro_say[4][6] = {
    { 4, 3, 4, 5, 6 }, { 5, 15, 16, 17, 18, 19 }, { 4, 51, 52, 53, 54 },
    { 4, 26, 27, 28, 29 }
};

enum {
    ST_INTRO,
    ST_PRIZE, ST_INPUT, ST_PRESS_OK, ST_FACE, ST_AWARD, ST_DONE, ST_OVER,
    ST_OPEN, ST_CLOSE_WRONG, ST_SHOW, ST_CLOSE,     /* module 2 */
    ST_REFUSE, ST_TURN, ST_VANISH, ST_TURN_BACK,    /* module 3 */
    ST_HELP,
};

/* Help: a script of phrases (REKENSND number - 18, each played on track 0
   if it is free and waited for by 0x19c04) and what is shown between them.
   Without sound nothing waits and Help passes at once. */
enum {
    H_SAY,
    H_SHOW,             /* module 1: an example in the sum panel */
    H_PRESS,            /* the Help button pressed, waited for */
    H_CLOSE_ALL,        /* module 2 (0x12a96): every row closes */
    H_CLEAR,            /* 0x12ca2: restore_rect(10, 202, 620, 230) */
    H_OPEN,             /* 0x1294b at x = 131 (0x83) */
    H_TEXTS,            /* the examples in the rows */
    H_CLOSE,            /* 0x129c6 on row n */
};
typedef struct {
    int op, n;
    const char *text, *width;   /* H_SHOW: centred by the width of the second */
} help_line;
#define SAY(n) { H_SAY, n, NULL, NULL }
#define SHOW(t, w) { H_SHOW, 0, t, w }
#define DO(op, n) { op, n, NULL, NULL }

static const help_line help1[] = {     /* 0x11a6a */
    SAY(3), SAY(4), SAY(5), SAY(6),
    SHOW("2 + 7 + 8 + 3 = ?", "2 + 7 + 8 + 3 = ?"),
    SAY(7), SAY(8), SAY(9), SAY(10),
    SHOW("2 + 8 = ?    7 + 3 = ?", "2 + 8  +  7 + 3 = ?"),
    SAY(11),
    SHOW("10 + 10 = ?", "10 + 10 = ?"),
    SAY(12), SAY(13),
    SHOW("10 + 10 = 20", "10 + 10 = 20"),
    SAY(14),
};
/* module 2: 0x13229 presses Help and closes the rows, then 0x12ca2 */
static const help_line help2[] = {
    DO(H_PRESS, 0), DO(H_CLOSE_ALL, 0), DO(H_CLEAR, 0),
    SAY(15), SAY(16), SAY(17), SAY(18), SAY(19), SAY(20),
    DO(H_OPEN, 0), DO(H_TEXTS, 0),
    SAY(21), SAY(22), SAY(23),
    DO(H_CLOSE, 0), SAY(24), DO(H_CLOSE, 1), SAY(25), DO(H_CLOSE, 2),
};
/* each centred as written, but the second by "2 * 4 = 7" (0x12ca2) */
static const char *const help2_text[3] = { "4 + 3 = 6", "2 x 4 = 7", "8 : 2 = 4" };
#define HELP2_BAR_X 131
#define A_TITLE_HELP 58         /* "Help!", 0x19b77 */
/* 0x19b77 swaps the title every 50 passes of 0x19c04's wait loop, which
   has no delay: about every 250 ms under DOSBox-X (redcat.conf) */
#define HELP_BLINK_MS 250

static int anim_image(int anim, int frame)
{
    const rc_anim *a = &rc_menu_anims[anim];

    if (frame >= a->count)
        frame = a->count - 1;
    if (frame < 0)
        frame = 0;
    return a->frames[frame].image;
}

static const rc_sprite *image(rc_exercise *x, int anim, int frame)
{
    int i = anim_image(anim, frame);

    return i > 0 && i <= RC_MENU_IMAGES && x->img[i].pixels ? &x->img[i] : NULL;
}

static void blit(rc_exercise *x, const rc_sprite *s, int x0, int y0)
{
    int sx, sy;

    if (!s)
        return;
    for (sy = 0; sy < s->h; sy++) {
        int y = y0 + sy;
        if (y < 0 || y >= SCREEN_H)
            continue;
        for (sx = 0; sx < s->w; sx++) {
            int px = x0 + sx;
            uint8_t c = s->pixels[sy * s->w + sx];
            if (c != RC_SPRITE_CLEAR && px >= 0 && px < SCREEN_W)
                x->fb[y * SCREEN_W + px] = c;
        }
    }
}

/* 0x10dd8: top-left at (x, y) */
static void draw_at(rc_exercise *x, int anim, int frame, int px, int py)
{
    blit(x, image(x, anim, frame), px, py);
}

/* 0x10f78: the image's hotspot at (x + 1, y) (it increments x first) */
static void draw_centred(rc_exercise *x, int anim, int frame, int px, int py)
{
    const rc_sprite *s = image(x, anim, frame);

    if (s)
        blit(x, s, px + 1 - s->x, py - s->y);
}

/* 0x1e929 / 0x1ea52: text in the current font; in sums '?' leaves its
   cell empty (draw_text), Help prints it (draw_text_q).  bk < 0 keeps the
   glyphs' background clear, as set_bk(0xff) does. */
static void glyphs(rc_exercise *x, const rc_font *f, int px, int py,
                   const char *s, int bk, int qmark)
{
    for (; *s; s++, px += f->w) {
        const uint8_t *g = f->glyphs + (uint8_t)*s * f->w * f->h;
        int gx, gy;

        if ((*s == '?' && !qmark) || !f->present[(uint8_t)*s])
            continue;
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < f->w; gx++) {
                int c = g[gy * f->w + gx], X = px + gx, Y = py + gy;
                if (c == f->bk)
                    c = bk;
                if (c >= 0 && X >= 0 && X < SCREEN_W && Y >= 0 && Y < SCREEN_H)
                    x->fb[Y * SCREEN_W + X] = (uint8_t)c;
            }
    }
}

static void draw_text(rc_exercise *x, const rc_font *f, int px, int py,
                      const char *s, int bk)
{
    glyphs(x, f, px, py, s, bk, 0);
}

static int load_font(rc_font *f, const rc_archive *a, const char *name)
{
    uint32_t size;
    const uint8_t *d = rc_archive_find(a, name, &size);

    if (!d || rc_font_parse(f, d, size) != 0)
        return -1;
    /* 0x10654: fg 254, 201 (the shadow) black */
    rc_font_recolor(f, (uint8_t)f->fg, 0xfe);
    rc_font_recolor(f, 0xc9, 0);
    f->fg = 0xfe;
    return 0;
}

int rc_exercise_load(rc_exercise *x, const rc_archive *reken,
                     const rc_archive *menu)
{
    uint32_t size;
    const uint8_t *d;
    int i;

    memset(x, 0, sizeof *x);
    d = rc_archive_find(reken, "mod_124.pcx", &size);
    if (!d || rc_pcx_decode(&x->bg, d, size) != 0 || x->bg.w != SCREEN_W ||
        x->bg.h != SCREEN_H)
        return -1;
    memcpy(x->pal, x->bg.palette, sizeof x->pal);
    d = rc_archive_find(reken, "module3.pcx", &size);
    if (!d || rc_pcx_decode(&x->bg3, d, size) != 0 || x->bg3.w != SCREEN_W ||
        x->bg3.h != SCREEN_H)
        return -1;
    if (load_font(&x->font12, reken, "ASCII12.IMG") != 0 ||
        load_font(&x->font20, reken, "Ascii20.img") != 0 ||
        load_font(&x->font30, reken, "Ascii30.img") != 0)
        return -1;
    for (i = 1; i <= RC_MENU_IMAGES && i - 1 < menu->count; i++) {
        d = rc_archive_get(menu, i - 1, &size);
        if (d)
            rc_sprite_parse(&x->img[i], d, size);
    }
    return 0;
}

void rc_exercise_free(rc_exercise *x)
{
    rc_pcx_free(&x->bg);
    rc_pcx_free(&x->bg3);
    rc_font_free(&x->font12);
    rc_font_free(&x->font20);
    rc_font_free(&x->font30);
}

/* sndman: play only on a free track; the "cut" form first stops the
   track (0x1b69e) as REKEN does before most sounds */
static int sound_on(const rc_exercise *x)
{
    return x->snd_play && x->r.speech;
}

static void snd(rc_exercise *x, int track, int sample)
{
    if (sound_on(x))
        x->snd_play(x->snd_ctx, track, sample);
}

static void snd_cut(rc_exercise *x, int track, int sample)
{
    if (sound_on(x)) {
        x->snd_stop(x->snd_ctx, track);
        x->snd_play(x->snd_ctx, track, sample);
    }
}

static int snd_busy(const rc_exercise *x, int track)
{
    return sound_on(x) && x->snd_busy(x->snd_ctx, track);
}

static void set_state(rc_exercise *x, int st)
{
    x->state = st;
    x->state_ms = 0;
    x->frame = 0;
    if (st == ST_FACE)
        x->said = 0;
    /* m3_pair_done 0x13d95: a found pair goes with doing.wav, a missed
       one turns back with noise.wav */
    if (st == ST_VANISH)
        snd(x, 1, S_DOING);
    else if (st == ST_TURN_BACK)
        snd(x, 1, S_NOISE);
}

/* prize_choose (0x18e7f): the prize and, cutting off track 0, its phrase */
static void show_prize(rc_exercise *x)
{
    x->prize_frame = 0;
    x->prize = x->r.prize;
    x->prize_say = x->r.prize_say;
    snd_cut(x, 0, x->prize_say);
    set_state(x, ST_PRIZE);
}

static void next_sum(rc_exercise *x)
{
    char digits[12];

    if (x->module == 4) {
        x->shown = x->r;
        if (!rc_module4_next(&x->r)) {
            set_state(x, ST_OVER);
            return;
        }
    } else if (x->module == 2) {
        if (!rc_module2_next(&x->r)) {
            set_state(x, ST_OVER);
            return;
        }
        x->texts_shown = 0;
    } else {
        if (!rc_module1_next(&x->r)) {
            set_state(x, ST_DONE);
            return;
        }
        x->typed[0] = 0;
        x->field = snprintf(digits, sizeof digits, "%d", x->r.answer);
    }
    show_prize(x);
}

/* the module intro done or stopped: the first sum (Memory: the grid) */
static void after_intro(rc_exercise *x)
{
    if (sound_on(x))
        x->snd_stop(x->snd_ctx, 0);
    if (x->module == 3)
        show_prize(x);
    else
        next_sum(x);
}

void rc_exercise_start(rc_exercise *x, const rc_session *s, int module,
                       uint32_t seed)
{
    int k;

    rc_reken_start(&x->r, s, seed);
    x->module = module >= 2 && module <= 4 ? module : 1;
    memcpy(x->pal, x->module == 3 ? x->bg3.palette : x->bg.palette, sizeof x->pal);
    x->bar_x = x->r.level >= 3 ? 61 : 131;     /* module2_init */
    for (k = 0; k < 3; k++)
        x->bar_frame[k] = -1;
    x->box_value = -1;
    x->help_anim = 0;
    x->help_rows = 0;
    x->equals_x = x->r.level >= 3 ? 78 : 152;   /* module4_init */
    x->face_anim = A_FACE_RIGHT;    /* module1_init: anim 13, frame 0 */
    x->face_frame = 0;
    x->shown_fuel = x->r.fuel;
    x->shown_repair = x->r.repair;
    x->shown_boosts = x->r.boosts;
    x->shown_rockets = x->r.rockets;
    x->stopped = 0;
    x->pressed_ms = 0;
    if (x->module == 3) {
        x->face_anim = A_FACE_RIGHT - 1;        /* module3_init: anim 12 */
        rc_module3_start(&x->r);
        for (k = 0; k < x->r.cells; k++) {
            x->cell_anim[k] = x->r.card[k].kind == RC_CARD_SOM ? A_CARD_SOM :
                                                                  A_CARD_ANTWOORD;
            x->cell_frame[k] = 0;
            x->cell_text[k] = 0;
        }
    }
    x->intro = 0;
    if (sound_on(x)) {
        set_state(x, ST_INTRO);
        snd(x, 0, 18 + intro_say[x->module - 1][1]);
    } else
        after_intro(x);
}

/* Module 3: step the cards in anim_cell through frames 0..count - 1, each
   held ms; 1 when the last one has been shown. */
static int animate_cards(rc_exercise *x, int count, int ms)
{
    int k;

    while (x->state_ms >= ms && x->frame < count) {
        x->state_ms -= ms;
        x->frame++;
    }
    for (k = 0; k < x->anim_cells; k++) {
        int kind = x->anim_kind[k], anim;
        switch (x->state) {
        case ST_REFUSE:
            anim = kind == RC_CARD_SOM ? A_SHAKE_SOM : A_SHAKE_ANTWOORD;
            break;
        case ST_TURN:
            anim = kind == RC_CARD_SOM ? A_CARD_SOM : A_CARD_ANTWOORD;
            break;
        case ST_VANISH:
            anim = A_CARD_GONE;
            break;
        default:
            anim = kind == RC_CARD_SOM ? A_BACK_SOM : A_BACK_ANTWOORD;
            break;
        }
        x->cell_anim[x->anim_cell[k]] = anim;
        x->cell_frame[x->anim_cell[k]] = x->frame < count ? x->frame : count - 1;
        x->cell_text[x->anim_cell[k]] = 0;
    }
    return x->frame >= count;
}

static void memory_tick(rc_exercise *x)
{
    int k;

    switch (x->state) {
    case ST_REFUSE:
        if (animate_cards(x, SHAKE_FRAMES, CARD_MS))
            set_state(x, ST_INPUT);
        break;
    case ST_TURN:
        if (!animate_cards(x, CARD_FRAMES, CARD_MS))
            break;
        x->cell_text[x->anim_cell[0]] = x->anim_kind[0] == RC_CARD_SOM ? 1 : 2;
        if (x->r.second == -1) {
            set_state(x, ST_INPUT);
            break;
        }
        x->anim_cell[0] = x->r.first;
        x->anim_cell[1] = x->r.second;
        x->anim_kind[0] = x->r.card[x->r.first].kind;
        x->anim_kind[1] = x->r.card[x->r.second].kind;
        x->anim_cells = 2;
        x->picked = rc_module3_judge(&x->r);
        x->face_anim = (x->picked ? A_FACE_RIGHT : A_FACE_WRONG) - 1;
        x->face_frame = 0;
        set_state(x, ST_FACE);
        break;
    case ST_VANISH:
        if (!animate_cards(x, GONE_FRAMES, CARD_MS))
            break;
        for (k = 0; k < 2; k++)
            x->cell_anim[x->anim_cell[k]] = -1;
        x->anim_cells = 0;
        if (x->r.left) {
            show_prize(x);
        } else {
            set_state(x, ST_OVER);
        }
        break;
    case ST_TURN_BACK:
        if (animate_cards(x, BACK_FRAMES, BACK_MS)) {
            x->anim_cells = 0;
            set_state(x, ST_INPUT);
        }
        break;
    }
}

int rc_exercise_done(const rc_exercise *x)
{
    return x->state == ST_OVER;
}

static void press(rc_exercise *x, int anim, int px, int py)
{
    x->pressed_anim = anim;
    x->pressed_x = px;
    x->pressed_y = py;
    x->pressed_ms = PRESS_MS;
}

/* 0x1294b: frames 5 down to 0, each presented and followed by a delay,
   so frame 0 stands empty for a tick too; 1 when done */
static int open_rows(rc_exercise *x)
{
    int k;

    while (x->state_ms >= BAR_MS && x->frame < BAR_FRAMES) {
        x->state_ms -= BAR_MS;
        x->frame++;
        for (k = 0; k < 3; k++)
            x->bar_frame[k] = x->frame < BAR_FRAMES ? BAR_FRAMES - 1 - x->frame : 0;
    }
    return x->frame == BAR_FRAMES;
}

static const help_line *help_script(const rc_exercise *x, int *n)
{
    if (x->module == 2) {
        *n = (int)(sizeof help2 / sizeof help2[0]);
        return help2;
    }
    *n = (int)(sizeof help1 / sizeof help1[0]);
    return help1;
}

/* After Help, module 1 (0x11e0d): the panel blank, the Help button pressed,
   the title back, and the sum asked again with an empty answer.  Module 2
   (0x13575): the rows back where they were, the Uitkomst box, the rows
   open with the sum's texts, Help pressed, the title back. */
static void help_end(rc_exercise *x)
{
    if (x->snd_stop && sound_on(x))
        x->snd_stop(x->snd_ctx, 0);         /* 0x19c04's mix_stop_track */
    x->help_text = NULL;
    x->help_anim = 0;
    press(x, A_HELP, HELP_X, HELP_Y);
    if (x->module == 2) {
        int k;
        x->help_rows = 0;
        x->bar_x = x->r.level >= 3 ? 61 : 131;
        /* 0x13575: the true row becomes [0x53c70], which nothing writes:
           the true sum moves to the first row (DOSBox-X: row 1 then wins) */
        x->r.true_row = 0;
        for (k = 0; k < 3; k++)
            x->bar_frame[k] = BAR_FRAMES - 1;
        set_state(x, ST_OPEN);
        return;
    }
    x->typed[0] = 0;
    set_state(x, x->help_from);
}

/* a row animation of the script, BAR_MS a frame; 1 when done */
static int help_animate(rc_exercise *x)
{
    int k;

    if (x->help_anim == H_PRESS)
        return x->state_ms >= PRESS_MS;
    if (x->help_anim == H_OPEN)
        return open_rows(x);
    while (x->state_ms >= BAR_MS) {
        x->state_ms -= BAR_MS;
        if (++x->frame > BAR_FRAMES)
            return 1;
        for (k = 0; k < 3; k++)
            if ((x->help_anim == H_CLOSE_ALL || k == x->help_row) &&
                x->bar_frame[k] >= 0)
                x->bar_frame[k] = x->frame > BAR_FRAMES - 1 ? -1 : x->frame;
    }
    return 0;
}

/* the script's next step: what is shown goes up at once, a phrase or a
   row animation is waited for */
static void help_next(rc_exercise *x)
{
    int n, k;
    const help_line *script = help_script(x, &n);

    x->help_anim = 0;
    while (++x->help_step < n) {
        const help_line *h = &script[x->help_step];
        switch (h->op) {
        case H_SAY:
            snd(x, 0, 18 + h->n);
            if (snd_busy(x, 0))
                return;
            break;
        case H_SHOW:
            x->help_text = h->text;
            x->help_width = h->width;
            break;
        case H_CLEAR:
            x->help_rows = 1;       /* the rows' texts are the examples' */
            x->bar_x = HELP2_BAR_X;
            for (k = 0; k < 3; k++)
                x->bar_frame[k] = -1;
            break;
        case H_TEXTS:
            x->texts_shown = 1;
            break;
        case H_OPEN:
            for (k = 0; k < 3; k++)
                x->bar_frame[k] = BAR_FRAMES - 1;
            /* fall through */
        default:
            if (h->op == H_PRESS)
                press(x, A_HELP, HELP_X, HELP_Y);
            if (h->op == H_CLOSE_ALL)
                x->texts_shown = 0;
            x->help_anim = h->op;
            x->help_row = h->n;
            x->state_ms = 0;
            x->frame = 0;
            return;
        }
    }
    help_end(x);
}

/* 0x11de6, 0x13502: H while a sum is asked */
static void help_start(rc_exercise *x)
{
    if (x->module == 2 && x->snd_stop && sound_on(x))
        x->snd_stop(x->snd_ctx, 0);
    x->help_from = x->state;
    x->help_step = -1;
    x->help_text = NULL;
    set_state(x, ST_HELP);
    help_next(x);
}

static int counting(const rc_exercise *x)
{
    return x->shown_fuel != x->r.fuel || x->shown_repair != x->r.repair ||
           x->shown_boosts != x->r.boosts || x->shown_rockets != x->r.rockets;
}

static int step_to(int shown, int real)
{
    return shown < real ? shown + 1 : shown > real ? shown - 1 : shown;
}

void rc_exercise_tick(rc_exercise *x, int ms)
{
    if (x->pressed_ms > 0)
        x->pressed_ms -= ms;
    x->state_ms += ms;
    if (x->module == 3)
        memory_tick(x);
    switch (x->state) {
    case ST_INTRO:
        if (snd_busy(x, 0))
            break;
        if (++x->intro < intro_say[x->module - 1][0])
            snd(x, 0, 18 + intro_say[x->module - 1][1 + x->intro]);
        else
            after_intro(x);
        break;
    case ST_HELP:
        if (x->help_anim) {
            if (help_animate(x))
                help_next(x);
        } else if (snd_busy(x, 0))
            x->help_ms += ms;
        else
            help_next(x);
        break;
    case ST_PRIZE:
        while (x->state_ms >= FRAME_MS && x->prize_frame < PRIZE_FRAMES - 1) {
            x->state_ms -= FRAME_MS;
            x->prize_frame++;
        }
        if (x->prize_frame == PRIZE_FRAMES - 1) {
            if (x->module == 2) {
                int k;
                for (k = 0; k < 3; k++)
                    x->bar_frame[k] = BAR_FRAMES - 1;
                set_state(x, ST_OPEN);
            } else {
                set_state(x, ST_INPUT);
            }
        }
        break;
    case ST_OPEN:                       /* 0x1294b */
        if (open_rows(x)) {
            x->texts_shown = 1;
            if (x->r.level >= 3)
                x->box_value = x->r.answer;
            set_state(x, ST_INPUT);
        }
        break;
    case ST_CLOSE_WRONG:                /* 0x12b59: the other rows close */
    case ST_CLOSE:                      /* 0x129c6: then the true one */
        while (x->state_ms >= BAR_MS && x->frame <= BAR_FRAMES) {
            int k;
            x->state_ms -= BAR_MS;
            x->frame++;
            for (k = 0; k < 3; k++)
                if ((k == x->r.true_row) == (x->state == ST_CLOSE) &&
                    x->bar_frame[k] >= 0)
                    x->bar_frame[k] = x->frame > BAR_FRAMES - 1 ? -1 : x->frame;
        }
        if (x->frame > BAR_FRAMES) {
            if (x->state == ST_CLOSE) {
                next_sum(x);
            } else {
                int right = rc_module2_answer(&x->r, x->picked);
                x->face_anim = right ? A_FACE_RIGHT : A_FACE_WRONG;
                x->face_frame = 0;
                set_state(x, ST_FACE);
            }
        }
        break;
    case ST_SHOW:
        if (x->state_ms >= SHOW_MS)
            set_state(x, x->module == 3 ? ST_TURN_BACK : ST_CLOSE);
        break;
    case ST_PRESS_OK:
        if (x->state_ms >= PRESS_MS) {
            int right = rc_reken_answer(&x->r, atoi(x->typed));
            x->face_anim = right ? A_FACE_RIGHT : A_FACE_WRONG;
            x->face_frame = 0;
            set_state(x, ST_FACE);
        }
        break;
    case ST_FACE:
        /* prize_award 0x1910f / prize_none 0x19471: once track 1 is
           free, "goed" or "fout" on track 0, then the face; the count
           waits for the phrase to end */
        if (!x->said) {
            if (snd_busy(x, 1)) {
                x->state_ms = 0;
                break;
            }
            snd_cut(x, 0, x->r.verdict_say);
            x->said = 1;
        }
        while (x->state_ms >= FRAME_MS && x->face_frame < FACE_FRAMES - 1) {
            x->state_ms -= FRAME_MS;
            x->face_frame++;
        }
        if (x->face_frame == FACE_FRAMES - 1 && !snd_busy(x, 0))
            set_state(x, ST_AWARD);
        break;
    case ST_AWARD:
        /* 0x1910f counts fuel, boosts and rockets one per 200 ms; repair
           goes up in one step.  yiha.wav when the counter was below its
           top, then a roll.wav per boost or rocket (track 1), fuel.wav per
           unit of fuel (track 0) or for the repair (track 1); then it
           waits for track 0 */
        if (!counting(x)) {
            if (snd_busy(x, 0))
                break;
            if (x->module == 3)
                set_state(x, x->picked ? ST_VANISH : ST_SHOW);
            else if (x->module == 4)
                next_sum(x);
            else if (x->module == 2)
                set_state(x, ST_SHOW);
            else
                next_sum(x);
        } else if (x->state_ms >= UNIT_MS || x->frame == 0) {
            if (x->frame)
                x->state_ms -= UNIT_MS;
            else if ((x->prize == RC_PRIZE_BOOSTS && x->shown_boosts < 99) ||
                     (x->prize == RC_PRIZE_ROCKETS && x->shown_rockets < 99) ||
                     ((x->prize == RC_PRIZE_REPAIR ||
                       x->prize == RC_PRIZE_REPAIR_FULL) &&
                      x->shown_repair < 100))
                snd(x, 0, S_YIHA);
            x->frame = 1;
            if (x->prize == RC_PRIZE_BOOSTS || x->prize == RC_PRIZE_ROCKETS)
                snd(x, 1, S_ROLL);
            else if (x->prize == RC_PRIZE_REPAIR)
                snd(x, 1, S_FUEL);
            else if (x->prize >= RC_PRIZE_FUEL5)
                snd(x, 0, S_FUEL);      /* 0x1d244: past sndman */
            x->shown_fuel = step_to(x->shown_fuel, x->r.fuel);
            x->shown_boosts = step_to(x->shown_boosts, x->r.boosts);
            x->shown_rockets = step_to(x->shown_rockets, x->r.rockets);
            x->shown_repair = x->r.repair;
        }
        break;
    case ST_DONE:
        if (x->state_ms >= END_MS)
            set_state(x, ST_OVER);
        break;
    }
}

/* Module 4's buttons (0x165fb); the error sound is not ported */
static void big_key(rc_exercise *x, int key)
{
    int res;

    if (x->state != ST_INPUT)
        return;
    switch (key) {
    case '1': case '2': case '3': case '4':
        /* 0x166e2: a number with click.wav, one not allowed now fout1 */
        res = rc_module4_number(&x->r, key - '1');
        if (res == RC_BIG_TAKEN)
            snd_cut(x, 0, S_CLICK);
        else if (res == RC_BIG_REFUSED)
            snd(x, 1, S_FOUT);
        return;
    case RC_KEY_PLUS:
    case RC_KEY_MINUS:
    case RC_KEY_TIMES:
    case RC_KEY_DIVIDE: {
        static const struct { int key, op, anim, x, y; } ops[4] = {
            { RC_KEY_TIMES, RC_OP_TIMES, A_TIMES, 78, 368 },
            { RC_KEY_DIVIDE, RC_OP_DIVIDE, A_DIVIDE, 78, 425 },
            { RC_KEY_PLUS, RC_OP_PLUS, A_PLUS, 8, 368 },
            { RC_KEY_MINUS, RC_OP_MINUS, A_MINUS, 8, 425 },
        };
        int i;
        for (i = 0; ops[i].key != key; i++)
            ;
        if (rc_module4_op(&x->r, ops[i].op) == RC_BIG_TAKEN) {
            snd_cut(x, 0, S_CLICK);
            press(x, ops[i].anim, ops[i].x, ops[i].y);
        } else
            snd(x, 1, S_FOUT);
        return;
    }
    case RC_KEY_BACK:           /* 0x16b9e: a number flies back (whizz) */
        if (rc_module4_back(&x->r) == RC_BIG_TAKEN) {
            snd_cut(x, 0, S_KNOPPR);
            snd_cut(x, 1, S_WHIZZ);
            press(x, A_BACK, BUTTON4_X, 368);
        } else
            snd_cut(x, 0, S_FOUT);
        return;
    case RC_KEY_AGAIN:
        snd_cut(x, 0, S_KNOPPR);
        press(x, A_AGAIN, BUTTON4_X, 424);
        rc_module4_again(&x->r);
        return;
    case RC_KEY_OK:
        res = rc_module4_equals(&x->r);
        if (res == RC_BIG_REFUSED) {
            snd(x, 0, S_FOUT);  /* 0x171aa: no whole sum yet */
            return;
        }
        snd_cut(x, 0, S_KNOPPR);
        press(x, A_EQUALS, x->equals_x, 368);
        if (res == RC_BIG_RIGHT || res == RC_BIG_WRONG) {
            x->face_anim = res == RC_BIG_RIGHT ? A_FACE_RIGHT : A_FACE_WRONG;
            x->face_frame = 0;
            set_state(x, ST_FACE);
        }
        return;
    }
}

void rc_exercise_key(rc_exercise *x, int key)
{
    int n = (int)strlen(x->typed);

    if (x->state == ST_INTRO) {
        if (key == RC_KEY_OK || key == RC_KEY_STOP)
            after_intro(x);
        return;
    }
    if (key == RC_KEY_STOP) {
        x->stopped = 1;
        set_state(x, ST_OVER);
        return;
    }
    if (x->state == ST_HELP) {
        /* 0x19c04: Esc, Enter or S ends it (keys are read there only) */
        if (!x->help_anim && (key == RC_KEY_OK || key == RC_KEY_STOP))
            help_end(x);
        return;
    }
    if (key == RC_KEY_HELP) {
        if (x->module <= 2 && x->state == ST_INPUT)
            help_start(x);
        else    /* the examples of modules 3 and 4 are not ported */
            press(x, A_HELP, HELP_X, HELP_Y);
        return;
    }
    if (x->module == 4) {
        big_key(x, key);
        return;
    }
    if (x->module == 1 && key == RC_KEY_AGAIN)
        key = RC_KEY_OK;
    if (x->module == 3) {
        /* Enter leaves Memory with what was won (0x14d11) */
        if (key == RC_KEY_OK && x->state != ST_OVER)
            set_state(x, ST_OVER);
        return;
    }
    if (x->state != ST_INPUT)
        return;
    if (x->module == 2) {
        if (key >= '1' && key <= '3') {
            x->picked = key - '1';
            snd_cut(x, 0, S_CLICK);     /* module2_ask 0x133d2 */
            set_state(x, ST_CLOSE_WRONG);
        }
        return;
    }
    if (key >= '0' && key <= '9') {
        int d = key - '0';
        press(x, d ? A_KEY_1 + d - 1 : A_KEY_1 + 9, key_xy[d][0], key_xy[d][1]);
        if (n < x->field) {
            x->typed[n] = (char)key;
            x->typed[n + 1] = 0;
        }
    } else if (key == RC_KEY_BACK) {
        press(x, A_BACK, BACK_X, BACK_Y);
        if (n > 0)
            x->typed[n - 1] = 0;
    } else if (key == RC_KEY_OK && n > 0) {     /* an empty answer is ignored */
        press(x, A_OK, OK_X, OK_Y);
        set_state(x, ST_PRESS_OK);
    }
}

/* Module 3: a click on the grid (0x14967) picks the card under it; the gaps
   between cards do nothing. */
static void memory_click(rc_exercise *x, int sx, int sy)
{
    const rc_grid *g = rc_module3_grid(x->r.level);
    int col, row, cell, first;

    if (x->state != ST_INPUT || sx < g->x || sy < g->y)
        return;
    col = (sx - g->x) / (g->w + g->gap_x);
    row = (sy - g->y) / (g->h + g->gap_y);
    if (col >= g->cols || row >= g->rows ||
        g->x + col * (g->w + g->gap_x) + g->w <= sx ||
        g->y + row * (g->h + g->gap_y) + g->h <= sy)
        return;
    cell = row * g->cols + col;
    first = x->r.first == -1;
    /* 0x14967: the first card cuts track 0 for its click, the second
       clicks on track 1; a card refused gives fout1 on track 1 */
    switch (rc_module3_pick(&x->r, cell)) {
    case RC_PICK_REFUSED:
        snd(x, 1, S_FOUT);
        set_state(x, ST_REFUSE);
        break;
    case RC_PICK_TURNED:
        if (first)
            snd_cut(x, 0, S_CLICK);
        else
            snd(x, 1, S_CLICK);
        set_state(x, ST_TURN);
        break;
    default:
        return;
    }
    x->anim_cell[0] = cell;
    x->anim_kind[0] = x->r.card[cell].kind;
    x->anim_cells = 1;
}

void rc_exercise_click(rc_exercise *x, int sx, int sy)
{
    const button *b = x->module == 4 ? buttons4 : x->module == 3 ? buttons3 :
                      x->module == 2 ? buttons2 : buttons1;
    int n = x->module == 4 ? (int)(sizeof buttons4 / sizeof *b) :
            x->module == 3 ? (int)(sizeof buttons3 / sizeof *b) :
            x->module == 2 ? (int)(sizeof buttons2 / sizeof *b) :
                             (int)(sizeof buttons1 / sizeof *b), i;

    /* groep 3..5 have no x and : (module4_init zeroes them) */
    if (x->module == 4 && x->r.level >= 3)
        n = BUTTON4_TIMES;
    for (i = 0; i < n; i++) {
        int dx = x->module == 2 && i < 3 ? x->bar_x :
                 x->module == 4 && i == BUTTON4_EQUALS ? x->equals_x : 0;
        /* the corners are taken as inside (LIKELY; 0x24117 not read) */
        if (sx >= b[i].x0 + dx && sx <= b[i].x1 + dx &&
            sy >= b[i].y0 && sy <= b[i].y1) {
            rc_exercise_key(x, b[i].key);
            return;
        }
    }
    if (x->module == 3)
        memory_click(x, sx, sy);
}

static void draw_gauges(rc_exercise *x)         /* 0x18d8a */
{
    /* records 0x46900, 0x46914 (boosts, rockets), 0x468ec (repair), 0x46928
       (fuel): module 3's position, the others' */
    static const int counter_xy[2][2][2] = {
        { { 385, 79 }, { 385, 125 } }, { { 600, 280 }, { 600, 316 } },
    };
    int v[2] = { x->shown_boosts, x->shown_rockets }, k, m3 = x->module == 3;
    int repair = x->shown_repair / 10 - 1, fuel = x->shown_fuel;

    for (k = 0; k < 2; k++) {
        const int *xy = counter_xy[m3][k];
        draw_at(x, A_DIGIT_SMALL, v[k] / 10, xy[0], xy[1]);
        draw_at(x, A_DIGIT_SMALL, v[k] % 10, xy[0] + 17, xy[1]);
    }
    if (repair > 9)
        repair = 9;
    if (repair < 0)
        repair = 0;
    if (m3) {
        draw_at(x, A_REPAIR, repair, 612, 188);
        /* 0x18cc8: three digits stepping up a pixel each */
        draw_at(x, A_DIGIT_FUEL3, fuel / 100, 539, 149);
        draw_at(x, A_DIGIT_FUEL3, fuel / 10 % 10, 539 + 21, 149 - 1);
        draw_at(x, A_DIGIT_FUEL3, fuel % 10, 539 + 43, 149 - 2);
    } else {
        draw_at(x, A_REPAIR, repair, 114, 48);
        draw_at(x, A_DIGIT_FUEL, fuel / 100, 10, 125);
        draw_at(x, A_DIGIT_FUEL, fuel / 10 % 10, 10 + 22, 125);
        draw_at(x, A_DIGIT_FUEL, fuel % 10, 10 + 44, 125);
    }
}

static void draw_cards(rc_exercise *x)          /* 0x1410a, 0x13abb */
{
    const rc_grid *g = rc_module3_grid(x->r.level);
    const rc_font *f = &x->font20;
    int k;

    for (k = 0; k < x->r.cells; k++) {
        int cx = g->x + k % g->cols * (g->w + g->gap_x);
        int cy = g->y + k / g->cols * (g->h + g->gap_y);

        if (x->cell_anim[k] < 0)
            continue;
        draw_centred(x, x->cell_anim[k], x->cell_frame[k], cx + g->w / 2,
                     cy + g->h / 2);
        if (x->cell_text[k]) {
            const rc_card *c = &x->r.card[k];
            char s[50];
            int n = x->cell_text[k] == 1 ? snprintf(s, sizeof s, "%s", c->text) :
                                           snprintf(s, sizeof s, "%d", c->value);
            draw_text(x, f, cx + (g->w - n * f->w) / 2,
                      cy + (g->h - f->h) / 2 + 1, s, -1);
        }
    }
}

static void draw_sum(rc_exercise *x)            /* 0x11cfd, 0x17fb2, 0x180af */
{
    const rc_font *f = &x->font30;
    char line[128];
    int len, tx, ty, cx, j, n = (int)strlen(x->typed);

    draw_at(x, A_PANEL, 0, PANEL_X, PANEL_Y);
    len = snprintf(line, sizeof line, "%s", x->r.text);
    for (j = 0; j < x->field && len < (int)sizeof line - 1; j++)
        line[len++] = '?';
    line[len] = 0;
    tx = PANEL_X + (PANEL_W - len * f->w) / 2;      /* 0x194e7 */
    ty = PANEL_Y + (PANEL_H - f->h) / 2;            /* 0x1953f */
    draw_text(x, f, tx, ty, line, -1);
    cx = tx + (len - x->field) * f->w;
    for (j = 0; j < x->field; j++) {
        char c[2] = { ' ', 0 };
        int bk = CELL_BK;
        if (j < n)
            c[0] = x->typed[j];
        else if (j == n && x->state == ST_INPUT) {
            c[0] = '_';
            bk = CURSOR_BK;
        }
        draw_text(x, f, cx + j * f->w, ty, c, bk);
    }
}

static void draw_choices(rc_exercise *x)        /* 0x12ee0, 0x13229 */
{
    const rc_font *f = &x->font30;
    char s[16];
    int k;

    if (x->r.level >= 3 && !x->help_rows) {
        draw_at(x, A_BOX, 0, BOX_X, BOX_Y);
        if (x->box_value >= 0) {
            int n = snprintf(s, sizeof s, "%d", x->box_value);
            draw_text(x, f, BOX_X + (RESULT_W - n * f->w) / 2,
                      RESULT_Y + (RESULT_H - f->h) / 2, s, -1);
        }
    }
    for (k = 0; k < 3; k++) {
        const char *t = x->help_rows ? help2_text[k] :
                        x->r.choice[rc_module2_choice(&x->r, k)];
        int n = (int)strlen(t);

        if (x->bar_frame[k] < 0)
            continue;
        draw_centred(x, A_BAR + k, x->bar_frame[k], x->bar_x + BAR_DX, bar_y[k]);
        /* a closing row starts with frame 0 over its text (0x12b59) */
        if (x->texts_shown && x->bar_frame[k] == 0 &&
            !(x->state == ST_CLOSE_WRONG && k != x->r.true_row) &&
            !(x->state == ST_CLOSE && k == x->r.true_row) &&
            !(x->help_anim == H_CLOSE && k == x->help_row))
            draw_text(x, f, x->bar_x + TEXT_DX + (TEXT_W - n * f->w) / 2,
                      text_y[k] + (TEXT_H - f->h) / 2, t, -1);
    }
}

static void draw_big(rc_exercise *x)            /* 0x14fe3, 0x161d3 */
{
    const rc_reken *r = x->state == ST_PRIZE ? &x->shown : &x->r;
    const rc_font *f = &x->font30;
    char s[64];
    int i, n;

    for (i = 0; i < x->r.count; i++)
        draw_at(x, A_SLOT, i, SLOT_X, SLOT_Y + SLOT_DY * i);
    draw_at(x, A_BOX, 0, BOX4_X, BOX4_Y);
    draw_at(x, A_PANEL4, 0, WORK_X, WORK_Y);
    draw_at(x, A_PLUS, 0, 8, 368);
    draw_at(x, A_MINUS, 0, 8, 425);
    if (x->r.level < 3) {
        draw_at(x, A_TIMES, 0, 78, 368);
        draw_at(x, A_DIVIDE, 0, 78, 425);
    }
    draw_at(x, A_EQUALS, 0, x->equals_x, 368);
    if (r->asked == 0)          /* before the first round's numbers */
        return;
    for (i = 0; i < r->count; i++) {
        if (r->num[i] == -1)
            continue;
        n = snprintf(s, sizeof s, "%d", r->num[i]);
        draw_text(x, f, 30 + (182 - n * f->w) / 2,
                  169 + SLOT_DY * i + (41 - f->h) / 2, s, -1);
    }
    n = snprintf(s, sizeof s, "%d", r->target);
    draw_text(x, f, BOX4_X + (132 - n * f->w) / 2, 209 + (144 - f->h) / 2, s, -1);
    rc_module4_entry(r, s, sizeof s);
    n = (int)strlen(s);
    draw_text(x, f, WORK_X + (WORK_W - n * f->w) / 2,
              WORK_Y + (WORK_H - f->h) / 2, s, -1);
    for (i = 0; i < RC_BIG_LINES; i++) {
        if (!r->line_on[i])
            continue;
        n = (int)strlen(r->line[i]);
        draw_text(x, &x->font12, WORK_X + (WORK_W - n * x->font12.w) / 2,
                  178 + 20 * i, r->line[i], -1);
    }
}

void rc_exercise_draw(rc_exercise *x)
{
    const int m3 = x->module == 3;
    char s[16];
    int d, left = RC_SUMS_PER_ROUND - x->r.asked + 1;

    memcpy(x->fb, m3 ? x->bg3.pixels : x->bg.pixels, sizeof x->fb);
    if (x->state == ST_HELP && x->help_ms / HELP_BLINK_MS % 2)
        draw_at(x, A_TITLE_HELP, 0, 183, 10);
    else
        draw_at(x, A_TITLE, x->module - 1, 183, 10);
    draw_centred(x, x->face_anim, x->face_frame, m3 ? FACE3_X : FACE_X,
                 m3 ? FACE3_Y : FACE_Y);
    draw_gauges(x);
    if (x->state != ST_DONE && x->state != ST_OVER)
        draw_centred(x, prize_anim[x->prize] - m3, x->prize_frame,
                     m3 ? PRIZE3_X : PRIZE_X, m3 ? PRIZE3_Y : PRIZE_Y);
    if (x->module == 4) {
        draw_at(x, A_BACK, 0, BUTTON4_X, 368);
        draw_at(x, A_AGAIN, 0, BUTTON4_X, 424);
        draw_big(x);
    }
    if (x->module == 1) {
        for (d = 0; d < 10; d++)
            draw_at(x, d ? A_KEY_1 + d - 1 : A_KEY_1 + 9, 0, key_xy[d][0], key_xy[d][1]);
        draw_at(x, A_OK, 0, OK_X, OK_Y);
        draw_at(x, A_BACK, 0, BACK_X, BACK_Y);
    }
    if (x->pressed_ms > 0)
        draw_at(x, x->pressed_anim, 1, x->pressed_x, x->pressed_y);

    /* 0x195e3, 0x1955b: Naam (empty without the menu), Groep, sums left;
       module 3 centres Groep in 98 pixels from x = 537 as if one digit */
    snprintf(s, sizeof s, "%d", x->r.level <= 2 ? x->r.level + 6 : x->r.level);
    if (m3) {
        draw_text(x, &x->font12, 537 + (98 - x->font12.w) / 2, 420, s, -1);
        draw_cards(x);
        return;
    }
    draw_text(x, &x->font12, 520, 105, s, -1);
    if (left < 0)
        left = 0;
    snprintf(s, sizeof s, "%d ", left);
    draw_text(x, &x->font12, 576, 134, s, 0);

    if (x->module == 2)
        draw_choices(x);
    else if (x->module == 1 && x->state == ST_HELP) {
        const rc_font *f = &x->font30;
        draw_at(x, A_PANEL, 0, PANEL_X, PANEL_Y);
        if (x->help_text)
            glyphs(x, f, PANEL_X + (PANEL_W - (int)strlen(x->help_width) * f->w) / 2,
                   PANEL_Y + (PANEL_H - f->h) / 2, x->help_text, -1, 1);
    } else if (x->module == 1)
        draw_sum(x);
}
