/* The screen after a race: podium 0x15a9f.  A picture of the first two
   places, then in ASCII20 (fg 0xd0, background clear) the world's points
   so far; the original holds it 4 s (delay_ms 0xfa0). */
#include <stdio.h>
#include <string.h>

#include "podium.h"

#define SCREEN_W 640
#define SCREEN_H 480

/* 0x57c18: world names, padded to 16 */
static const char *const world_names[6] = {
    "De Welkomstrace ", "De Rotsvlakte   ", "De Snelweg      ",
    "De Puzzelkloof  ", "De Cactusvallei ", "De Laatste Kans ",
};

/* r(edcat), b(rutus), s(nitch) by the first two places */
static const char *picture(const int32_t order[3])
{
    static const char *const names[3][3] = {
        { NULL, "rbs.pcx", "rsb.pcx" },
        { "brs.pcx", NULL, "bsr.pcx" },
        { "srb.pcx", "sbr.pcx", NULL },
    };
    int a = order[0] / 2, b = order[1] / 2;

    if (order[0] < 0 || order[0] > 4 || order[1] < 0 || order[1] > 4)
        return NULL;
    return names[a][b];
}

/* rcascii 0x24dbe, as REKEN's 0x1e929: one cell per character */
static void text(const rc_font *f, uint8_t *fb, int px, int py, const char *s)
{
    for (; *s; s++, px += f->w) {
        const uint8_t *g = f->glyphs + (uint8_t)*s * f->w * f->h;
        int gx, gy;

        if (*s == '?' || !f->present[(uint8_t)*s])
            continue;
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < f->w; gx++) {
                int c = g[gy * f->w + gx], x = px + gx, y = py + gy;
                if (c != f->bk && x >= 0 && x < SCREEN_W && y >= 0 &&
                    y < SCREEN_H)
                    fb[y * SCREEN_W + x] = (uint8_t)c;
            }
    }
}

int rc_podium_draw(rc_podium *p, const rc_archive *rcs, int world,
                   const int32_t order[3], const int32_t points[3],
                   const char *name, uint8_t *fb, rc_rgb *pal)
{
    const char *pic = picture(order);
    const uint8_t *d;
    uint32_t size;
    char line[64];

    rc_pcx_free(&p->pic);
    /* an order the switch has no picture for leaves the race's screen */
    if (pic && (d = rc_archive_find(rcs, pic, &size)) &&
        rc_pcx_decode(&p->pic, d, size) == 0 && p->pic.w == SCREEN_W &&
        p->pic.h == SCREEN_H) {
        memcpy(fb, p->pic.pixels, SCREEN_W * SCREEN_H);
        memcpy(pal, p->pic.palette, sizeof p->pic.palette);
    }
    if (!p->font.glyphs) {
        if (!(d = rc_archive_find(rcs, "Ascii20.img", &size)) ||
            rc_font_parse(&p->font, d, size) != 0)
            return -1;
        rc_font_recolor(&p->font, (uint8_t)p->font.fg, 0xd0);
        p->font.fg = 0xd0;
    }
    if (world < 0 || world > 5)
        world = 0;
    snprintf(line, sizeof line, "Punten Totaal voor %s", world_names[world]);
    text(&p->font, fb, 150, 20, line);
    snprintf(line, sizeof line, "%-15s    :    %3d", name, (int)points[0]);
    text(&p->font, fb, 200, 50, line);
    snprintf(line, sizeof line, "Brutus de Bulldog  :    %3d", (int)points[1]);
    text(&p->font, fb, 200, 70, line);
    snprintf(line, sizeof line, "Max de Muis        :    %3d", (int)points[2]);
    text(&p->font, fb, 200, 90, line);
    return 0;
}

void rc_podium_free(rc_podium *p)
{
    rc_pcx_free(&p->pic);
    rc_font_free(&p->font);
}
