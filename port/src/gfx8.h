/* 8-bit paletted images: PCX screens, sprites, race tilesets and maps
   (reverse-engineering/file-formats.md). */
#ifndef RC_GFX8_H
#define RC_GFX8_H

#include <stdint.h>

typedef struct {
    uint8_t r, g, b;
} rc_rgb;

typedef struct {
    int w, h;
    uint8_t *pixels;        /* w*h palette indices */
    rc_rgb palette[256];
} rc_pcx;

/* 8-bit, single-plane RLE PCX with the trailing 256-colour palette. */
int rc_pcx_decode(rc_pcx *out, const uint8_t *data, uint32_t size);
void rc_pcx_free(rc_pcx *p);

typedef struct {
    int w, h, x, y;         /* x, y: hotspot, signed (RRR's image record
                               +8, +10; checked for all 544 race images) */
    const uint8_t *pixels;  /* points into the archive */
} rc_sprite;

#define RC_SPRITE_CLEAR 0xFF

/* One entry of RC/MENU/RRRANIM/INSTMNU: u16 w,h,x,y + w*h bytes. */
int rc_sprite_parse(rc_sprite *s, const uint8_t *data, uint32_t size);

#define RC_TILE_W 32
#define RC_TILE_H 40

typedef struct {
    int count;
    const uint8_t *tiles;   /* count * 32*40, points into the archive */
    rc_rgb palette[256];    /* scaled up from the 6-bit DAC values */
} rc_tileset;

int rc_tileset_parse(rc_tileset *t, const uint8_t *data, uint32_t size);

typedef struct {
    uint8_t tile;
    uint8_t object;         /* 0 in the files; the race stores an object here */
    uint8_t waypoint;       /* number, when racer != 0 */
    uint8_t racer;          /* 1 REDCAT, 2 BULL, 4 MUIS: whose waypoint */
    uint8_t type;           /* object/surface type 0..21 */
    uint8_t level;          /* ground level: blocks z < level*1000 */
} rc_cell;

typedef struct {
    int w, h;
    rc_cell *cells;
} rc_map;

int rc_map_parse(rc_map *m, const uint8_t *data, uint32_t size);
void rc_map_free(rc_map *m);

/* RRR.EXE 0x57a60 / 0x57abc: the 23 race maps and their tilesets. */
#define RC_NUM_MAPS 23
extern const char *const rc_map_files[RC_NUM_MAPS];
extern const char *const rc_map_tilesets[RC_NUM_MAPS];

#endif
