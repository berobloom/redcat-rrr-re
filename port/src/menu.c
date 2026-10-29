/* RRRMENU.EXE's main screen: menubs01.pcx, RRRANIM.IMG sprites through the
   anim table 0x457bc (rrrmenu_anims_tab.c), RRRFONT.IMG for the Race panel.
   Sprites are drawn with their top-left at the given point (the hotspot is
   not used), 0xff clear.  Bediening (0x1118f) is the keyboard half: the
   joystick set-up is not ported, nor the closing film.  The sounds
   (menu_sound 0x10b4f) are queued in sound_q for the caller.  Rapport (P, 0x12990) reads the pupil's SMD file (smd.c), Einde
   (0x119cc) shows RRRHIGH.DAT (highscore.c). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "anims.h"
#include "highscore.h"
#include "menu.h"
#include "smd.h"

#define W 640
#define H 480
#define FRAME_MS 20             /* a frame, present and delay(1) (0x11056) */
#define FLASH_MS 100            /* world_flash: delay(10), hundredths */

enum { ST_IDLE, ST_ANIM, ST_FLASH, ST_KEYS, ST_KEYS_WAIT, ST_RAPPORT,
       ST_HIGH };
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
/* Rapport: the text column (0x45914), the graphs' left edge (0x45918),
   each module's top (0x4591c) */
#define RAPPORT_X 36
#define GRAPH_X 310
static const int rapport_y[4] = { 111, 192, 274, 354 };

static const char *const world_names[6] = {         /* 0x4589c */
    "De Welkomstrace   ", "De Rotsvlakte     ", "De Snelweg        ",
    "De Puzzelkloof    ", "De Cactusvallei   ", "De Laatste Kans   ",
};
static const uint8_t world_modules[6][4] = {        /* 0x4558e */
    { 1, 0, 1, 0 }, { 0, 1, 1, 0 }, { 1, 1, 1, 0 },
    { 1, 0, 1, 1 }, { 0, 1, 1, 1 }, { 1, 1, 1, 1 },
};

/* 0x453e2: the keys Bediening takes, scan code and name; 0 (Esc) aborts */
const rc_menu_keyname rc_menu_keys[RC_MENU_KEYS] = {
    { 1, "       " }, { 30, "A      " }, { 48, "B      " }, { 46, "C      " },
    { 32, "D      " }, { 18, "E      " }, { 33, "F      " }, { 34, "G      " },
    { 35, "H      " }, { 23, "I      " }, { 36, "J      " }, { 37, "K      " },
    { 38, "L      " }, { 50, "M      " }, { 49, "N      " }, { 24, "O      " },
    { 25, "P      " }, { 16, "Q      " }, { 19, "R      " }, { 31, "S      " },
    { 20, "T      " }, { 22, "U      " }, { 47, "V      " }, { 17, "W      " },
    { 45, "X      " }, { 21, "Y      " }, { 44, "Z      " }, { 79, "End    " },
    { 80, "Omlaag " }, { 81, "PgDn   " }, { 75, "Links  " }, { 76, "5      " },
    { 77, "Rechts " }, { 71, "HOME   " }, { 72, "Omhoog " }, { 73, "PgUp   " },
    { 82, "INS    " }, { 83, "DEL    " }, { 57, "SPATIE " }, { 28, "ENTER  " },
    { 42, "L SHIFT" }, { 54, "R SHIFT" }, { 29, "CTRL   " }, { 56, "ALT    " },
    { 58, "CAPS LK" }, { 15, "TAB    " },
};

/* Bediening's prompts and where each key goes in RRRIO.INF */
static const struct {
    const char *label;
    int offset;
} key_slots[6] = {
    { "Linksaf:                     ", 0x58 },
    { "Rechtsaf:                    ", 0x5c },
    { "Vooruit:                     ", 0x60 },
    { "Achteruit:                   ", 0x64 },
    { "Turbo Boost:                 ", 0x50 },
    { "Raket:                       ", 0x54 },
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
    d = rc_archive_find(&m->rrrmenu, "rapport.pcx", &size);
    if (!d || rc_pcx_decode(&m->rapport_bg, d, size) != 0 ||
        m->rapport_bg.w != W || m->rapport_bg.h != H)
        return -1;
    d = rc_archive_find(&m->rrrmenu, "highscor.pcx", &size);
    if (!d || rc_pcx_decode(&m->high_bg, d, size) != 0 ||
        m->high_bg.w != W || m->high_bg.h != H)
        return -1;
    d = rc_archive_find(&m->rrrmenu, "MENUKEY.PCX", &size);
    if (!d || rc_pcx_decode(&m->keys_bg, d, size) != 0 || m->keys_bg.w != W ||
        m->keys_bg.h != H)
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
    rc_pcx_free(&m->keys_bg);
    rc_pcx_free(&m->rapport_bg);
    rc_pcx_free(&m->high_bg);
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
                if (c == 0 && m->clear)     /* 0x10a04: 0 becomes 0xff */
                    continue;
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

/* menubs01.pcx and what menu_init draws on it */
static void main_screen(rc_menu *m)
{
    memcpy(m->screen, m->bg.pixels, sizeof m->screen);
    memcpy(m->pal, m->bg.palette, sizeof m->pal);
    draw_groep(m, 1);
    draw_bediening(m);
    draw_geluid(m);
}

/* Bediening without a joystick (0x1150e): MENUKEY.PCX and the prompts */
static void keys_start(rc_menu *m)
{
    m->colour = 0;
    memcpy(m->screen, m->keys_bg.pixels, sizeof m->screen);
    memcpy(m->pal, m->keys_bg.palette, sizeof m->pal);
    text(m, 0xc9, 0xca, "Geef 1 voor 1 de toetsen voor");
    text(m, 0xc9, 0xde, key_slots[0].label);
    m->keys_n = 0;
    m->state = ST_KEYS;
}

/* 0x10add: a key of the table not taken yet */
static void keys_take(rc_menu *m, int scan)
{
    int i, k, n = m->keys_n;

    for (i = 0; i < RC_MENU_KEYS && rc_menu_keys[i].scan != scan; i++)
        ;
    if (i == RC_MENU_KEYS)
        return;
    if (i == 0) {               /* Esc: nothing is kept */
        text(m, 0xc9, 0x142, "Afgebroken!                  ");
        text(m, 0xc9, 0x156, "Druk op [Enter]              ");
        m->state = ST_KEYS_WAIT;
        return;
    }
    for (k = 0; k < n; k++)
        if (m->keys_taken[k] == i)
            return;
    m->keys_taken[n] = i;
    m->colour = 1;
    text(m, 0x141, 0xde + 20 * n, rc_menu_keys[i].name);
    m->colour = 0;
    if (++m->keys_n < 6) {
        text(m, 0xc9, 0xde + 20 * m->keys_n, key_slots[m->keys_n].label);
        return;
    }
    for (k = 0; k < 6; k++)
        rc_session_set(m->s, key_slots[k].offset,
                       rc_menu_keys[m->keys_taken[k]].scan);
    text(m, 0xc9, 0x156, "Druk op [Enter]              ");
    m->state = ST_KEYS_WAIT;
}

/* 0x1f46e moveto, 0x1f485 lineto: Bresenham from the current point */
static void line_to(rc_menu *m, int x1, int y1, int c)
{
    int x0 = m->pen_x, y0 = m->pen_y;
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    m->pen_x = x1;
    m->pen_y = y1;
    for (;;) {
        int e2 = 2 * err;
        if (x0 >= 0 && x0 < W && y0 >= 0 && y0 < H)
            m->screen[y0 * W + x0] = (uint8_t)c;
        if (x0 == x1 && y0 == y1)
            break;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void move_to(rc_menu *m, int x, int y)
{
    m->pen_x = x;
    m->pen_y = y;
}

/* a score's height in the graph: 0 at y + 48, 100 clamped to y */
static int graph_y(int y, int score)
{
    int v = y + 0x30 - score / 2;
    return v < y ? y : v;
}

/* 0x12722: grid lines every 5 px (the middle one 0x40), a vertical line
   per record when they are more than 1 px apart, the record's scores in
   white (0xfd) and the running total in yellow (0xf9), both from the
   bottom-left corner */
static void rapport_graph(rc_menu *m, const rc_smd *p, int module)
{
    int n = rc_smd_count(p, module), y = rapport_y[module];
    int step, i, x;

    if (!n)
        return;
    for (step = 300 / n; step > 1 && step * n >= 300; step--)
        ;
    for (i = 0; i < 10; i++) {
        move_to(m, GRAPH_X, y + 0x30 - 5 * i);
        line_to(m, GRAPH_X + 300, y + 0x30 - 5 * i, i == 5 ? 0x40 : 0x10);
    }
    if (step > 1)
        for (i = 0; i <= n; i++) {
            move_to(m, GRAPH_X + i * step, y + 0x30);
            line_to(m, GRAPH_X + i * step, y, 0x10);
        }
    move_to(m, GRAPH_X, y + 0x30);
    for (i = 0, x = GRAPH_X; i < p->count; i++) {
        const rc_smd_record *r = &p->rec[i];
        if (r->module != module)
            continue;
        x += step;
        if (x - GRAPH_X <= 300)
            line_to(m, x, graph_y(y, r->asked ? r->correct * 100 / r->asked
                                  : 0), 0xfd);
    }
    move_to(m, GRAPH_X, y + 0x30);
    for (i = 0, x = GRAPH_X; i < n; i++) {
        x += step;
        if (x - GRAPH_X <= 300)
            line_to(m, x, graph_y(y, rc_smd_total(p, module, i + 1)), 0xf9);
    }
}

/* Rapport (0x12990): rapport.pcx, per module the last score and the total
   as a mark out of 10 with the graph, the pupil's name */
static void rapport_start(rc_menu *m)
{
    const char *name = (const char *)m->s->raw + 0x68;
    char line[64];
    rc_smd p;
    int i;

    memcpy(m->screen, m->rapport_bg.pixels, sizeof m->screen);
    memcpy(m->pal, m->rapport_bg.palette, sizeof m->pal);
    if (!*name)
        name = "RedCat";
    rc_smd_load(&p, m->save_dir ? m->save_dir : ".", name);
    m->colour = 2;                      /* 0x1294e */
    text(m, 0x26, 0x1b6, "Rapport van");
    m->colour = 0;
    snprintf(line, sizeof line, "%.31s", name);
    text(m, 0x9c, 0x1b6, line);
    for (i = 0; i < 4; i++) {
        int last = rc_smd_last(&p, i), total = rc_smd_total(&p, i, -1);
        m->colour = 2;
        text(m, RAPPORT_X, rapport_y[i], module_names[i]);
        if (last < 0) {
            m->colour = 0;
            text(m, RAPPORT_X, rapport_y[i] + 0x19, "Nog niet gedaan");
            continue;
        }
        m->colour = 0;                  /* 0x12673 */
        snprintf(line, sizeof line, "Cijfer:%2d", last / 10);
        text(m, RAPPORT_X, rapport_y[i] + 0x19, line);
        m->colour = 1;
        snprintf(line, sizeof line, "Totaal:%2d", total / 10);
        text(m, RAPPORT_X + 0x78, rapport_y[i] + 0x19, line);
        rapport_graph(m, &p, i);
    }
    rc_smd_free(&p);
    m->state = ST_RAPPORT;
}

static int high_rnd(void *m)
{
    return rnd(m);
}

/* Einde (0x119cc): highscor.pcx and RRRHIGH.DAT's ten entries, the
   font's background clear (0x10a04): each line in pair 1 at (51, 101 +
   30 i), then in pair 0 one pixel up and left.  A line is 40 columns
   with the name at 2, the world at 21 and "%3d" at 40. */
static void high_start(rc_menu *m)
{
    rc_high h;
    int i, pass;

    memcpy(m->screen, m->high_bg.pixels, sizeof m->screen);
    memcpy(m->pal, m->high_bg.palette, sizeof m->pal);
    rc_high_load(&h, m->save_dir ? m->save_dir : ".", high_rnd, m);
    m->clear = 1;
    for (pass = 0; pass < 2; pass++) {
        m->colour = pass ? 0 : 1;
        for (i = 0; i < RC_HIGH_ENTRIES; i++) {
            char line[64];
            const rc_high_entry *e = &h.e[i];
            int n, w = e->world >= 0 && e->world < 6 ? e->world : 0;

            memset(line, ' ', 40);
            n = (int)strlen(e->name);
            memcpy(line + 2, e->name, (size_t)(n > 19 ? 19 : n));
            memcpy(line + 21, world_names[w], strlen(world_names[w]));
            snprintf(line + 40, sizeof line - 40, "%3d  ", (int)e->score);
            text(m, 0x33 - pass, 0x65 + 30 * i - pass, line);
        }
    }
    m->clear = 0;
    m->state = ST_HIGH;
}

const char *const rc_menu_sound_names[RC_MSND_COUNT] = {
    "CHKSND.WAV", "KNOPPR.WAV", "RC4.WAV", "WHIZZ.WAV", "RAPPORT.WAV",
    "BEDIEN1.WAV", "REDCAT.WAV", "TRACK1.WAV", "TRACK2.WAV", "RCEND.WAV",
    "MENU.WAV"
};

static void menu_sound(rc_menu *m, int sample)
{
    if (m->sounds < (int)(sizeof m->sound_q / sizeof m->sound_q[0]))
        m->sound_q[m->sounds++] = sample;
}

void rc_menu_start(rc_menu *m, rc_session *s, uint32_t seed)
{
    m->s = s;
    m->rng = seed;
    m->result = RC_MENU_NONE;
    m->state = ST_IDLE;
    m->sounds = 0;
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

int rc_menu_key(rc_menu *m, int scan)
{
    if (m->state == ST_KEYS) {
        keys_take(m, scan);
        return 1;
    }
    if (m->state == ST_KEYS_WAIT) {
        if (scan == 0x1c) {     /* back: 0x1198c */
            main_screen(m);
            panel(m, 1, 1);
            m->state = ST_IDLE;
            idle_wait(m);
        }
        return 1;
    }
    if (m->state == ST_HIGH) {
        if (scan == 0x1c || scan == 0x12)   /* then EINDANI2.SMK, not ported */
            m->result = RC_MENU_QUIT;
        return 1;
    }
    if (m->state == ST_RAPPORT) {
        if (scan == 0x14) {     /* Terug: menubs01.pcx, menu_init(0, 1) */
            menu_sound(m, RC_MSND_WHIZZ);
            main_screen(m);
            panel(m, 1, 1);
            m->state = ST_IDLE;
            idle_wait(m);
        }
        return 1;
    }
    if (m->state != ST_IDLE || m->result)
        return 0;
    switch (scan) {
    case 0x1c: case 0x1f:       /* Start!: RedCat drives off */
        menu_sound(m, RC_MSND_RC4);
        play(m, 3, 22, 94, 1, AFTER_START);
        break;
    case 0x12:                  /* Einde: the record, then the high scores */
        menu_sound(m, RC_MSND_WHIZZ);   /* waited for */
        leave(m, 0);
        m->result = RC_MENU_NONE;
        high_start(m);
        break;
    case 0x19:                  /* Rapport */
        menu_sound(m, RC_MSND_RAPPORT);
        rapport_start(m);
        break;
    case 0x30:                  /* Bediening: no joystick here */
        menu_sound(m, RC_MSND_BEDIEN1);
        keys_start(m);
        break;
    case 0x13:                  /* the next world */
        menu_sound(m, RC_MSND_TRACK1);
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
            menu_sound(m, RC_MSND_KNOPPR);
            draw_groep(m, 0);
            m->groep = scan - 4;
            draw_groep(m, 1);
            panel(m, 1, 1);
        } else if (scan < 4 || scan > 9)
            return 0;
        break;
    }
    return 1;
}

void rc_menu_click(rc_menu *m, int x, int y)
{
    size_t i;

    if (m->state == ST_HIGH) {          /* 0x455ee: Enter, the screen */
        rc_menu_key(m, 0x1c);
        return;
    }
    if (m->state == ST_RAPPORT) {       /* 0x4592c: Terug */
        if (x >= 561 && x <= 628 && y >= 436 && y <= 464)
            rc_menu_key(m, 0x14);
        return;
    }
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
    if (m->state == ST_KEYS || m->state == ST_KEYS_WAIT)
        return;                 /* 0x1e3b1 hides it there */
    for (j = 0; j < p->h; j++)
        for (i = 0; i < p->w; i++) {
            int c = p->pixels[j * p->w + i];
            int X = m->mouse_x + i, Y = m->mouse_y + j;
            if (c != 0xff && X >= 0 && X < W && Y >= 0 && Y < H)
                m->fb[Y * W + X] = (uint8_t)c;
        }
}
