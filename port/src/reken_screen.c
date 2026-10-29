/* The arithmetic part's screen: see reken_screen.h.  Addresses are
   REKEN.EXE's.  The original draws into the screen as things change; this
   redraws the whole frame each time, which shows the same picture. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "anims.h"
#include "reken_screen.h"

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

/* Delays, ms: 0x249f3's arguments */
#define FRAME_MS 10             /* prize and face animations (0x18e0b) */
#define PRESS_MS 150            /* a button shown pressed (0x19842) */
#define UNIT_MS 200             /* one fuel / boost / rocket unit (0x1910f) */
#define END_MS 1000
#define BAR_MS 5                /* 0x1294b, 0x12b59, 0x129c6 */
#define SHOW_MS 1000            /* the true row after the prize (0x13229) */

enum {
    ST_PRIZE, ST_INPUT, ST_PRESS_OK, ST_FACE, ST_AWARD, ST_DONE, ST_OVER,
    ST_OPEN, ST_CLOSE_WRONG, ST_SHOW, ST_CLOSE,     /* module 2 */
};

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

/* 0x1e929 / 0x1ea52: text in the current font; '?' leaves its cell empty.
   bk < 0 keeps the glyphs' background clear, as set_bk(0xff) does. */
static void draw_text(rc_exercise *x, const rc_font *f, int px, int py,
                      const char *s, int bk)
{
    for (; *s; s++, px += f->w) {
        const uint8_t *g = f->glyphs + (uint8_t)*s * f->w * f->h;
        int gx, gy;

        if (*s == '?' || !f->present[(uint8_t)*s])
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
    if (load_font(&x->font12, reken, "ASCII12.IMG") != 0 ||
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
    rc_font_free(&x->font12);
    rc_font_free(&x->font30);
}

static void set_state(rc_exercise *x, int st)
{
    x->state = st;
    x->state_ms = 0;
    x->frame = 0;
}

static void next_sum(rc_exercise *x)
{
    char digits[12];

    if (x->module == 2) {
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
    x->prize_frame = 0;
    set_state(x, ST_PRIZE);
}

void rc_exercise_start(rc_exercise *x, const rc_session *s, int module,
                       uint32_t seed)
{
    int k;

    rc_reken_start(&x->r, s, seed);
    x->module = module == 2 ? 2 : 1;
    x->bar_x = x->r.level >= 3 ? 61 : 131;     /* module2_init */
    for (k = 0; k < 3; k++)
        x->bar_frame[k] = -1;
    x->box_value = -1;
    x->face_anim = A_FACE_RIGHT;    /* module1_init: anim 13, frame 0 */
    x->face_frame = 0;
    x->shown_fuel = x->r.fuel;
    x->shown_repair = x->r.repair;
    x->shown_boosts = x->r.boosts;
    x->shown_rockets = x->r.rockets;
    x->stopped = 0;
    x->pressed_ms = 0;
    next_sum(x);
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
    switch (x->state) {
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
    case ST_OPEN:                       /* 0x1294b: frames 5..0 */
        while (x->state_ms >= BAR_MS && x->bar_frame[0] > 0) {
            int k;
            x->state_ms -= BAR_MS;
            for (k = 0; k < 3; k++)
                x->bar_frame[k]--;
        }
        if (x->bar_frame[0] == 0) {
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
            set_state(x, ST_CLOSE);
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
        while (x->state_ms >= FRAME_MS && x->face_frame < FACE_FRAMES - 1) {
            x->state_ms -= FRAME_MS;
            x->face_frame++;
        }
        if (x->face_frame == FACE_FRAMES - 1)
            set_state(x, ST_AWARD);
        break;
    case ST_AWARD:
        /* 0x1910f counts fuel, boosts and rockets one per 200 ms; repair
           goes up in one step */
        if (!counting(x)) {
            if (x->module == 2)
                set_state(x, ST_SHOW);
            else
                next_sum(x);
        } else if (x->state_ms >= UNIT_MS || x->frame == 0) {
            if (x->frame)
                x->state_ms -= UNIT_MS;
            x->frame = 1;
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

void rc_exercise_key(rc_exercise *x, int key)
{
    int n = (int)strlen(x->typed);

    if (key == RC_KEY_STOP) {
        x->stopped = 1;
        set_state(x, ST_OVER);
        return;
    }
    if (key == RC_KEY_HELP) {
        press(x, A_HELP, HELP_X, HELP_Y);   /* the example (0x11a6a) is not ported */
        return;
    }
    if (x->state != ST_INPUT)
        return;
    if (x->module == 2) {
        if (key >= '1' && key <= '3') {
            x->picked = key - '1';
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

void rc_exercise_click(rc_exercise *x, int sx, int sy)
{
    const button *b = x->module == 2 ? buttons2 : buttons1;
    int n = x->module == 2 ? (int)(sizeof buttons2 / sizeof *b) :
                             (int)(sizeof buttons1 / sizeof *b), i;

    for (i = 0; i < n; i++) {
        int dx = x->module == 2 && i < 3 ? x->bar_x : 0;
        /* the corners are taken as inside (LIKELY; 0x24117 not read) */
        if (sx >= b[i].x0 + dx && sx <= b[i].x1 + dx &&
            sy >= b[i].y0 && sy <= b[i].y1) {
            rc_exercise_key(x, b[i].key);
            return;
        }
    }
}

static void draw_gauges(rc_exercise *x)         /* 0x18d8a */
{
    static const int counter_y[2] = { 79, 125 };    /* 0x46900, 0x46914 */
    int v[2] = { x->shown_boosts, x->shown_rockets }, k;
    int repair = x->shown_repair / 10 - 1, fuel = x->shown_fuel;

    for (k = 0; k < 2; k++) {
        draw_at(x, A_DIGIT_SMALL, v[k] / 10, 385, counter_y[k]);
        draw_at(x, A_DIGIT_SMALL, v[k] % 10, 385 + 17, counter_y[k]);
    }
    if (repair > 9)
        repair = 9;
    if (repair < 0)
        repair = 0;
    draw_at(x, A_REPAIR, repair, 114, 48);          /* 0x468ec */
    draw_at(x, A_DIGIT_FUEL, fuel / 100, 10, 125);  /* 0x46928 */
    draw_at(x, A_DIGIT_FUEL, fuel / 10 % 10, 10 + 22, 125);
    draw_at(x, A_DIGIT_FUEL, fuel % 10, 10 + 44, 125);
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

    if (x->r.level >= 3) {
        draw_at(x, A_BOX, 0, BOX_X, BOX_Y);
        if (x->box_value >= 0) {
            int n = snprintf(s, sizeof s, "%d", x->box_value);
            draw_text(x, f, BOX_X + (RESULT_W - n * f->w) / 2,
                      RESULT_Y + (RESULT_H - f->h) / 2, s, -1);
        }
    }
    for (k = 0; k < 3; k++) {
        const char *t = x->r.choice[rc_module2_choice(&x->r, k)];
        int n = (int)strlen(t);

        if (x->bar_frame[k] < 0)
            continue;
        draw_centred(x, A_BAR + k, x->bar_frame[k], x->bar_x + BAR_DX, bar_y[k]);
        /* a closing row starts with frame 0 over its text (0x12b59) */
        if (x->texts_shown && x->bar_frame[k] == 0 &&
            !(x->state == ST_CLOSE_WRONG && k != x->r.true_row) &&
            !(x->state == ST_CLOSE && k == x->r.true_row))
            draw_text(x, f, x->bar_x + TEXT_DX + (TEXT_W - n * f->w) / 2,
                      text_y[k] + (TEXT_H - f->h) / 2, t, -1);
    }
}

void rc_exercise_draw(rc_exercise *x)
{
    char s[16];
    int d, left = RC_SUMS_PER_ROUND - x->r.asked + 1;

    memcpy(x->fb, x->bg.pixels, sizeof x->fb);
    draw_at(x, A_TITLE, x->module - 1, 183, 10);
    draw_centred(x, x->face_anim, x->face_frame, FACE_X, FACE_Y);
    draw_gauges(x);
    if (x->state != ST_DONE && x->state != ST_OVER)
        draw_centred(x, prize_anim[x->r.prize], x->prize_frame, PRIZE_X, PRIZE_Y);
    if (x->module == 1) {
        for (d = 0; d < 10; d++)
            draw_at(x, d ? A_KEY_1 + d - 1 : A_KEY_1 + 9, 0, key_xy[d][0], key_xy[d][1]);
        draw_at(x, A_OK, 0, OK_X, OK_Y);
        draw_at(x, A_BACK, 0, BACK_X, BACK_Y);
    }
    if (x->pressed_ms > 0)
        draw_at(x, x->pressed_anim, 1, x->pressed_x, x->pressed_y);

    /* 0x195e3, 0x1955b: Naam (empty without the menu), Groep, sums left */
    snprintf(s, sizeof s, "%d", x->r.level <= 2 ? x->r.level + 6 : x->r.level);
    draw_text(x, &x->font12, 520, 105, s, -1);
    if (left < 0)
        left = 0;
    snprintf(s, sizeof s, "%d ", left);
    draw_text(x, &x->font12, 576, 134, s, 0);

    if (x->module == 2)
        draw_choices(x);
    else
        draw_sum(x);
}
