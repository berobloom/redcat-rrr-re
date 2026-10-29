/* RRRMENU.EXE's main screen: menubs01.pcx, RRRANIM.IMG sprites through the
   anim table 0x457bc (rrrmenu_anims_tab.c), RRRFONT.IMG for the Race panel.
   Sprites are drawn with their top-left at the given point (the hotspot is
   not used), 0xff clear.  Not ported: Rapport (P, 0x12990), the Bediening
   screen (B, 0x1118f), the sounds (0x10b4f) and the closing film. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "anims.h"
#include "menu.h"

#define W 640
#define H 480
#define FRAME_MS 20             /* a frame, present and delay(1) (0x11056) */
#define FLASH_MS 100            /* world_flash: delay(10), hundredths */

enum { ST_IDLE, ST_ANIM, ST_FLASH };
enum { AFTER_NONE, AFTER_START };

/* menu buttons 0x45244: scan code and rectangle */
static const struct {
    int scan, x0, y0, x1, y1;
} buttons[] = {
    { 0x1f, 22, 105, 133, 216 },    /* Start! */
    { 0x13, 360, 162, 632, 418 },   /* the Race panel: next world */
    { 0x19, 181, 103, 253, 224 },   /* Rapport */
    { 0x30, 26, 268, 157, 431 },    /* Bediening */
    { 0x22, 177, 301, 255, 425 },   /* Geluid */
    { 0x12, 561, 436, 628, 464 },   /* Einde */
    { 4, 342, 108, 378, 144 }, { 5, 392, 108, 428, 144 },
    { 6, 442, 108, 478, 144 }, { 7, 492, 108, 528, 144 },
    { 8, 542, 108, 578, 144 }, { 9, 592, 108, 628, 144 },
};

/* 0x4556e: font colour pairs (RRRFONT's 253 and 254 become these) */
static const uint8_t colours[4][2] = {
    { 253, 254 }, { 249, 250 }, { 247, 248 }, { 251, 252 },
};

static const char *const module_names[4] = {        /* 0x45888 */
    "Handig rekenen    ", "Wat is waar?      ", "Memory            ",
    "Grote getallenspel",
};
static const char *const world_names[6] = {         /* 0x4589c */
    "De Welkomstrace   ", "De Rotsvlakte     ", "De Snelweg        ",
    "De Puzzelkloof    ", "De Cactusvallei   ", "De Laatste Kans   ",
};
static const uint8_t world_modules[6][4] = {        /* 0x4558e */
    { 1, 0, 1, 0 }, { 0, 1, 1, 0 }, { 1, 1, 1, 0 },
    { 1, 0, 1, 1 }, { 0, 1, 1, 1 }, { 1, 1, 1, 1 },
};

static uint8_t *read_file(const char *dir, const char *name, uint32_t *size)
{
    FILE *f = rc_fopen_nocase(dir, name, "rb");
    uint8_t *d = NULL;
    long n;

    if (!f)
        return NULL;
    if (fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) > 0 &&
        fseek(f, 0, SEEK_SET) == 0 && (d = malloc((size_t)n)) &&
        fread(d, 1, (size_t)n, f) != (size_t)n) {
        free(d);
        d = NULL;
    }
    fclose(f);
    *size = d ? (uint32_t)n : 0;
    return d;
}

int rc_menu_load(rc_menu *m, const char *dir)
{
    const uint8_t *d;
    uint32_t size;
    int i;

    memset(m, 0, sizeof *m);
    if (rc_archive_open(&m->rrrmenu, dir, "RRRMENU") != 0 ||
        rc_archive_open(&m->rrranim, dir, "RRRANIM") != 0)
        return -1;
    d = rc_archive_find(&m->rrrmenu, "menubs01.pcx", &size);
    if (!d || rc_pcx_decode(&m->bg, d, size) != 0 || m->bg.w != W ||
        m->bg.h != H)
        return -1;
    for (i = 1; i <= RC_RRRMENU_IMAGES && i - 1 < m->rrranim.count; i++)
        if (!(d = rc_archive_get(&m->rrranim, i - 1, &size)) ||
            rc_sprite_parse(&m->img[i], d, size) != 0)
            return -1;
    m->font_data = read_file(dir, "RRRFONT.IMG", &size);
    if (!m->font_data || rc_font_parse(&m->font, m->font_data, size) != 0)
        return -1;
    return 0;
}

void rc_menu_free(rc_menu *m)
{
    rc_pcx_free(&m->bg);
    rc_font_free(&m->font);
    free(m->font_data);
}

static int rnd(rc_menu *m)
{
    m->rng = m->rng * 1103515245u + 12345u;
    return (m->rng >> 16) & 0x7fff;
}

static void blit(rc_menu *m, int image, int x, int y)
{
    const rc_sprite *s;
    int i, j;

    if (image < 1 || image > RC_RRRMENU_IMAGES)
        return;
    s = &m->img[image];
    for (j = 0; j < s->h; j++)
        for (i = 0; i < s->w; i++) {
            int c = s->pixels[j * s->w + i], X = x + i, Y = y + j;
            if (c != 0xff && X >= 0 && X < W && Y >= 0 && Y < H)
                m->screen[Y * W + X] = (uint8_t)c;
        }
}

static void draw_frame(rc_menu *m, int anim, int frame, int x, int y)
{
    const rc_anim *a = &rc_rrrmenu_anims[anim];

    if (frame >= 0 && frame < a->count)
        blit(m, a->frames[frame].image, x, y);
}

/* 0x17f29: RRRFONT text in the current colour pair on black */
static void text(rc_menu *m, int x, int y, const char *s)
{
    const rc_font *f = &m->font;

    for (; *s; s++, x += f->w) {
        const uint8_t *g = f->glyphs + (uint8_t)*s * f->w * f->h;
        int gx, gy;

        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < f->w; gx++) {
                int c = g[gy * f->w + gx], X = x + gx, Y = y + gy;
                if (c == f->bk)
                    c = 0;
                else if (c == 253)
                    c = colours[m->colour][0];
                else if (c == 254)
                    c = colours[m->colour][1];
                if (X >= 0 && X < W && Y >= 0 && Y < H)
                    m->screen[Y * W + X] = (uint8_t)c;
            }
    }
}

/* next_module (0x12548) for the world shown */
static int next_module(const rc_menu *m)
{
    rc_session t = *m->s;

    t.world = m->world;
    return rc_next_module(&t, 0);
}

/* 0x10c18: the Race panel.  bright: the header white, the modules dim up to
   the next one; else the header yellow.  modules: list them too. */
static void panel(rc_menu *m, int bright, int modules)
{
    int next = next_module(m), first = 1, y = 0x109, k;
    char line[32];

    m->colour = bright ? 0 : 1;
    snprintf(line, sizeof line, "%d:", m->world + 1);
    text(m, 0x17c, 0xcd, line);
    text(m, 0x1a4, 0xcd, world_names[m->world]);
    m->colour = !bright ? 0 : modules ? 3 : 1;
    if (!modules)
        return;
    for (k = 0; k < 4; k++) {
        if (!world_modules[m->world][k])
            continue;
        /* once set the colour stays: the next module and those after */
        if ((k + 1 == next && m->s->world == m->world) ||
            (first && m->s->world != m->world)) {
            m->colour = 1;
            first = 0;
        }
        /* groep 3 plays Memory for Grote getallenspel */
        text(m, 400, y, module_names[k == 3 && m->groep < 1 ? 2 : k]);
        y += 30;
    }
    for (; y < 0x168; y += 30)
        text(m, 400, y, "                  ");
}

/* 0x10d67, 0x10db8, 0x10bfe / 0x10bb9 */
static void draw_bediening(rc_menu *m)
{
    draw_frame(m, 2, m->joystick ? 0 : rc_rrrmenu_anims[2].count - 1, 26, 270);
}

static void draw_geluid(rc_menu *m)
{
    int anim = m->sound == 0 ? 7 : m->sound == 1 ? 6 : 5;

    draw_frame(m, anim, 0, 176, 300);
}

static void draw_groep(rc_menu *m, int selected)
{
    draw_frame(m, selected ? 0 : 1, m->groep, 342 + 50 * m->groep, 108);
}

static void idle_wait(rc_menu *m)
{
    /* 0x11c00: 20000 + rand() % 10000 loop passes; here milliseconds */
    m->idle = 20000 + rnd(m) % 10000;
}

static void play(rc_menu *m, int anim, int x, int y, int step, int after)
{
    m->state = ST_ANIM;
    m->anim = anim;
    m->ax = x;
    m->ay = y;
    m->step = step;
    m->frame = step > 0 ? 0 : rc_rrrmenu_anims[anim].count - 1;
    m->end = step > 0 ? rc_rrrmenu_anims[anim].count : -1;
    m->after = after;
    m->timer = 0;
    draw_frame(m, anim, m->frame, x, y);
}

void rc_menu_start(rc_menu *m, rc_session *s, uint32_t seed)
{
    m->s = s;
    m->rng = seed;
    m->result = RC_MENU_NONE;
    m->state = ST_IDLE;
    m->mouse_x = 320;
    m->mouse_y = 240;
    m->sound = s->speech;
    m->groep = s->groep - 3;
    m->joystick = rc_session_get(s, 0x34);
    m->world = s->world;
    memcpy(m->screen, m->bg.pixels, sizeof m->screen);
    memcpy(m->pal, m->bg.palette, sizeof m->pal);
    panel(m, 1, 0);
    draw_bediening(m);
    draw_geluid(m);
    draw_groep(m, 1);
    /* after the last race of a world: the next one, flashing */
    if (rc_world_next(s)) {
        m->world = s->world;
        m->state = ST_FLASH;
        m->flashes = 0;
        m->timer = 0;
        panel(m, 0, 1);
    } else
        panel(m, 1, 1);
    idle_wait(m);
}

/* 0x120de: the choices into the record; Start! also clears race_done */
static void leave(rc_menu *m, int start)
{
    rc_session *s = m->s;

    if (s->world != m->world) {
        s->stage = 0;
        s->world = m->world;
        rc_world_reset(s);
    }
    /* menu_init 0x10ed1: an empty name becomes RedCat */
    if (!s->raw[0x68])
        memcpy(s->raw + 0x68, "RedCat", 7);
    s->groep = m->groep + 3;
    s->speech = m->sound;
    rc_session_set(s, 0x34, m->joystick);
    if (start)
        s->race_done = 0;
    rc_session_store(s);
    m->result = start ? RC_MENU_START : RC_MENU_QUIT;
}

void rc_menu_key(rc_menu *m, int scan)
{
    if (m->state != ST_IDLE || m->result)
        return;
    switch (scan) {
    case 0x1c: case 0x1f:       /* Start!: RedCat drives off */
        play(m, 3, 22, 94, 1, AFTER_START);
        break;
    case 0x12:                  /* Einde */
        leave(m, 0);
        break;
    case 0x13:                  /* the next world */
        m->world = (m->world + 1) % 6;
        panel(m, 1, 1);
        break;
    case 0x22:                  /* Geluid: 3 -> 1 -> 0 -> 3 */
        if (m->sound == 0) {
            m->sound = 3;
            play(m, 7, 176, 300, 1, AFTER_NONE);
        } else if (m->sound == 1) {
            m->sound = 0;
            play(m, 6, 176, 300, 1, AFTER_NONE);
        } else if (m->sound == 3) {
            m->sound = 1;
            play(m, 5, 176, 300, 1, AFTER_NONE);
        }
        break;
    default:
        if (scan >= 4 && scan <= 9 && scan - 4 != m->groep) {
            draw_groep(m, 0);
            m->groep = scan - 4;
            draw_groep(m, 1);
            panel(m, 1, 1);
        }
        break;
    }
}

void rc_menu_click(rc_menu *m, int x, int y)
{
    size_t i;

    for (i = 0; i < sizeof buttons / sizeof buttons[0]; i++)
        if (x >= buttons[i].x0 && x <= buttons[i].x1 &&
            y >= buttons[i].y0 && y <= buttons[i].y1) {
            rc_menu_key(m, buttons[i].scan);
            return;
        }
}

void rc_menu_mouse(rc_menu *m, int x, int y)
{
    if (x != m->mouse_x || y != m->mouse_y)
        idle_wait(m);
    m->mouse_x = x;
    m->mouse_y = y;
}

void rc_menu_tick(rc_menu *m, int ms)
{
    m->timer += ms;
    switch (m->state) {
    case ST_IDLE:
        if ((m->idle -= ms) <= 0) {     /* RedCat blinks */
            play(m, 4, 22, 94, 1, AFTER_NONE);
            idle_wait(m);
        }
        break;
    case ST_ANIM:
        while (m->state == ST_ANIM && m->timer >= FRAME_MS) {
            m->timer -= FRAME_MS;
            m->frame += m->step;
            if (m->frame == m->end) {
                m->state = ST_IDLE;
                if (m->after == AFTER_START)
                    leave(m, 1);
            } else
                draw_frame(m, m->anim, m->frame, m->ax, m->ay);
        }
        break;
    case ST_FLASH:              /* world_flash 0x10e2d, then 0x10ff8 */
        while (m->state == ST_FLASH && m->timer >= FLASH_MS) {
            m->timer -= FLASH_MS;
            if (++m->flashes < 20)
                panel(m, m->flashes % 2, 1);
            else {
                panel(m, 1, 1);
                m->state = ST_IDLE;
            }
        }
        break;
    }
}

void rc_menu_draw(rc_menu *m)
{
    const rc_sprite *p = &m->img[28];   /* the paw (anim 8) */
    int i, j;

    memcpy(m->fb, m->screen, sizeof m->fb);
    for (j = 0; j < p->h; j++)
        for (i = 0; i < p->w; i++) {
            int c = p->pixels[j * p->w + i];
            int X = m->mouse_x + i, Y = m->mouse_y + j;
            if (c != 0xff && X >= 0 && X < W && Y >= 0 && Y < H)
                m->fb[Y * W + X] = (uint8_t)c;
        }
}
