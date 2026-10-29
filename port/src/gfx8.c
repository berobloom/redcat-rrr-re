#include "gfx8.h"

#include <stdlib.h>
#include <string.h>

static int rd16(const uint8_t *p)
{
    return p[0] | p[1] << 8;
}

static rc_rgb vga6(const uint8_t *p)
{
    rc_rgb c = { (uint8_t)(p[0] << 2 | p[0] >> 4),
                 (uint8_t)(p[1] << 2 | p[1] >> 4),
                 (uint8_t)(p[2] << 2 | p[2] >> 4) };
    return c;
}

int rc_pcx_decode(rc_pcx *out, const uint8_t *d, uint32_t size)
{
    int bpl, x, y;
    uint32_t pos = 128;
    const uint8_t *pal;

    memset(out, 0, sizeof *out);
    if (size < 128 + 769 || d[0] != 0x0A || d[2] != 1 || d[3] != 8 ||
        d[65] != 1)
        return -1;
    out->w = rd16(d + 8) - rd16(d + 4) + 1;
    out->h = rd16(d + 10) - rd16(d + 6) + 1;
    bpl = rd16(d + 66);
    if (out->w <= 0 || out->h <= 0 || bpl < out->w)
        return -1;
    out->pixels = malloc((size_t)out->w * out->h);
    if (!out->pixels)
        return -1;
    for (y = 0; y < out->h; y++) {
        x = 0;
        while (x < bpl) {
            int c, n = 1;
            if (pos >= size)
                goto bad;
            c = d[pos++];
            if ((c & 0xC0) == 0xC0) {
                n = c & 0x3F;
                if (pos >= size)
                    goto bad;
                c = d[pos++];
            }
            while (n-- > 0 && x < bpl) {    /* runs do not cross lines here */
                if (x < out->w)
                    out->pixels[y * out->w + x] = c;
                x++;
            }
        }
    }
    pal = d + size - 769;
    if (pal[0] != 0x0C)
        goto bad;
    for (x = 0; x < 256; x++) {
        out->palette[x].r = pal[1 + 3 * x];
        out->palette[x].g = pal[2 + 3 * x];
        out->palette[x].b = pal[3 + 3 * x];
    }
    return 0;
bad:
    rc_pcx_free(out);
    return -1;
}

void rc_pcx_free(rc_pcx *p)
{
    free(p->pixels);
    p->pixels = NULL;
}

int rc_sprite_parse(rc_sprite *s, const uint8_t *d, uint32_t size)
{
    if (size < 8)
        return -1;
    s->w = rd16(d);
    s->h = rd16(d + 2);
    s->x = rd16(d + 4);
    s->y = rd16(d + 6);
    if (8 + (uint32_t)s->w * s->h != size)
        return -1;
    s->pixels = d + 8;
    return 0;
}

int rc_tileset_parse(rc_tileset *t, const uint8_t *d, uint32_t size)
{
    int i;

    if (size < 2)
        return -1;
    t->count = rd16(d);
    if (size != 2 + (uint32_t)t->count * RC_TILE_W * RC_TILE_H + 768)
        return -1;
    t->tiles = d + 2;
    for (i = 0; i < 256; i++)
        t->palette[i] = vga6(d + size - 768 + 3 * i);
    return 0;
}

int rc_map_parse(rc_map *m, const uint8_t *d, uint32_t size)
{
    int i;

    if (size < 4)
        return -1;
    m->w = rd16(d);
    m->h = rd16(d + 2);
    if (size != 4 + 6u * m->w * m->h)
        return -1;
    m->cells = malloc(sizeof *m->cells * ((size_t)m->w * m->h + 1));
    if (!m->cells)
        return -1;
    for (i = 0; i < m->w * m->h; i++) {
        m->cells[i].tile = rd16(d + 4 + 6 * i);
        m->cells[i].f1 = rd16(d + 6 + 6 * i);
        m->cells[i].f2 = rd16(d + 8 + 6 * i);
    }
    return 0;
}

void rc_map_free(rc_map *m)
{
    free(m->cells);
    m->cells = NULL;
}

const char *const rc_map_files[RC_NUM_MAPS] = {
    "track1a.map", "track1b.map", "track1c.map", "track2a.map",
    "track2b.map", "track2c.map", "track3a.map", "track3b.map",
    "track3c.map", "track3d.map", "track4a.map", "track4b.map",
    "track4c.map", "track4d.map", "track5a.map", "track4a.map",
    "track6b.map", "track4d.map", "track5a.map", "track6b.map",
    "track6c.map", "track6d.map", "track6e.map",
};

const char *const rc_map_tilesets[RC_NUM_MAPS] = {
    "track1.img", "track2.img", "track1.img", "track1.img",
    "track1.img", "track1.img", "track1.img", "track2.img",
    "track2.img", "track2.img", "track2.img", "track2.img",
    "track2.img", "track2.img", "track2.img", "track2.img",
    "track2.img", "track2.img", "track2.img", "track2.img",
    "track2.img", "track2.img", "track2.img",
};
