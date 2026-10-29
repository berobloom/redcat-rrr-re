/* redcat: a race, as RRR.EXE runs one.

   usage: redcat <data dir> [map 0..22] [--fps N]

   Keys (RRRIO.INF defaults): arrows steer and accelerate, right Shift boost,
   Enter rocket, F1/F2/F3 follow RedCat/Brutus/Snitch, Esc quits.

   The original's frame loop has no timer (research/discoveries.md "The
   race: speed"): it ran 29 frames/s at DOSBox cycles=40000 and faster on
   faster machines.  This port runs a fixed rate, 30 by default. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "anims.h"
#include "archive.h"
#include "gfx8.h"
#include "race.h"

#define SCREEN_W 640
#define SCREEN_H 480
#define RC_IMAGES 1056
#define LIGHTS_ANIM 86          /* start_lights 0x1103e: [0x804bc] + 0x2b4 */
#define DIGITS_ANIM 18          /* hud_update 0x1918d: anims 18..21 */
#define SPEEDO_ANIM 19
#define FUEL_ANIM 20
#define DAMAGE_ANIM 21

/* start_lights: lamp images and how long each stays, ms */
static const int light_ms[5] = { 1500, 1000, 1000, 1000, 500 };

static struct {
    const char *dir;
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    uint8_t fb[SCREEN_W * SCREEN_H];
    uint8_t view[RC_VIEW_W * RC_VIEW_H];
    rc_rgb pal[256];
    uint8_t trans[256 * 256];
    rc_archive rcs, rc;
    rc_tileset tiles;
    rc_pcx hud;
    rc_sprite sprite[RC_IMAGES + 1];    /* by image number, 1..1056 */
    rc_hotspot hotspot[RC_IMAGES + 1];
    rc_race race;
    int map_index;
    int light;                  /* lamp shown, 5 once the race runs */
    Uint32 light_until, next_frame, frame_ms;
    int running;
} g;

static int load_trans(void)     /* trans_load 0x11e2a */
{
    FILE *f = rc_fopen_nocase(g.dir, "TRANS.DAT", "rb");
    int row;

    if (!f)
        return -1;
    for (row = 0; row < 255; row++) {
        if (fread(g.trans + row * 256, 1, 255, f) != 255) {
            fclose(f);
            return -1;
        }
        g.trans[row * 256 + 255] = (uint8_t)row;
    }
    fclose(f);
    return 0;
}

static int load_sprites(void)
{
    int i;

    if (rc_archive_open(&g.rc, g.dir, "RC") != 0)
        return -1;
    for (i = 1; i <= RC_IMAGES && i - 1 < g.rc.count; i++) {
        uint32_t size;
        const uint8_t *d = rc_archive_get(&g.rc, i - 1, &size);
        if (d && rc_sprite_parse(&g.sprite[i], d, size) == 0) {
            g.hotspot[i].hx = (int16_t)g.sprite[i].x;
            g.hotspot[i].hy = (int16_t)g.sprite[i].y;
        }
    }
    return 0;
}

static int start_race(int index)
{
    static const int start_order[RC_RACERS] = { 0, 1, 2 };
    uint32_t ms, ts;
    const uint8_t *md, *td;

    rc_race_free(&g.race);
    g.map_index = (index + RC_NUM_MAPS) % RC_NUM_MAPS;
    md = rc_archive_find(&g.rcs, rc_map_files[g.map_index], &ms);
    td = rc_archive_find(&g.rcs, rc_map_tilesets[g.map_index], &ts);
    if (!md || !td || rc_tileset_parse(&g.tiles, td, ts) != 0 ||
        rc_race_init(&g.race, md, ms, g.map_index, g.hotspot, start_order)
            != 0) {
        fprintf(stderr, "map %d (%s) failed to load\n", g.map_index,
                rc_map_files[g.map_index]);
        return -1;
    }
    /* During the race the screen uses the tileset's palette (the same as
       RACESCR3.PCX's; wacht.pcx's is only for the loading screen). */
    memcpy(g.pal, g.tiles.palette, sizeof g.pal);
    g.light = 0;
    g.light_until = 0;
    printf("map %d: %s, waypoints %d/%d/%d\n", g.map_index,
           rc_map_files[g.map_index], g.race.wp_count[0], g.race.wp_count[1],
           g.race.wp_count[2]);
    return 0;
}

/* blit_sprite 0x11bb4 / blit_sprite_trans 0x17d94, clipped to the view */
static void draw_sprite(int image, int kind, int x, int y)
{
    const rc_sprite *s = &g.sprite[image];
    int sx, sy;

    if (image <= 0 || image > RC_IMAGES || !s->pixels)
        return;
    for (sy = 0; sy < s->h; sy++) {
        int vy = y + sy;
        const uint8_t *src = s->pixels + sy * s->w;
        uint8_t *dst;
        if (vy < 0 || vy >= RC_VIEW_H)
            continue;
        dst = g.view + vy * RC_VIEW_W;
        for (sx = 0; sx < s->w; sx++) {
            int vx = x + sx;
            uint8_t c = src[sx];
            if (c == RC_SPRITE_CLEAR || vx < 0 || vx >= RC_VIEW_W)
                continue;
            dst[vx] = kind >= 4 ? g.trans[dst[vx] << 8 | c] : c;
        }
    }
}

static void draw_view(void)     /* 0x11526 */
{
    const rc_map *m = &g.race.map;
    int order[RC_OBJECTS], n, i, y;

    for (y = 0; y < RC_VIEW_H; y++) {
        int my = g.race.scroll_y + y, ty = my / RC_TILE_H, x;
        uint8_t *row = g.view + y * RC_VIEW_W;
        if (ty >= m->h) {
            memset(row, 0, RC_VIEW_W);
            continue;
        }
        for (x = 0; x < RC_VIEW_W; x++) {
            int mx = g.race.scroll_x + x, tx = mx / RC_TILE_W;
            const rc_cell *c;
            if (tx >= m->w) {
                row[x] = 0;
                continue;
            }
            c = &m->cells[ty * m->w + tx];
            row[x] = g.tiles.tiles[c->tile * RC_TILE_W * RC_TILE_H +
                                   (my % RC_TILE_H) * RC_TILE_W +
                                   mx % RC_TILE_W];
        }
    }
    n = rc_race_draw_order(&g.race, order);
    for (i = 0; i < n; i++) {
        const rc_object *o = &g.race.obj[order[i]];
        draw_sprite(o->image, o->kind, o->sx - g.race.scroll_x,
                    o->sy - g.race.scroll_y);
    }
}

/* 0x2425d: an image at screen (x, y), 0xFF transparent (0x32c78) */
static void draw_image(int image, int x, int y)
{
    const rc_sprite *s = &g.sprite[image];
    int sx, sy;

    if (image <= 0 || image > RC_IMAGES || !s->pixels)
        return;
    for (sy = 0; sy < s->h && y + sy < SCREEN_H; sy++)
        for (sx = 0; sx < s->w && x + sx < SCREEN_W; sx++) {
            uint8_t c = s->pixels[sy * s->w + sx];
            if (c != RC_SPRITE_CLEAR && x + sx >= 0 && y + sy >= 0)
                g.fb[(y + sy) * SCREEN_W + x + sx] = c;
        }
}

static int anim_image(int anim, int frame, int max)
{
    if (frame > max) frame = max;
    if (frame < 0) frame = 0;
    return rc_anims[anim].frames[frame].image;
}

/* hud_update (0x1918d): the player's gauges and counters.  The original
   redraws each only when its value changes; the images are opaque
   rectangles, so drawing them every frame looks the same. */
static void draw_hud(void)
{
    const rc_stats *s = &g.race.stats[RC_REDCAT];
    int32_t v = (int32_t)(((int64_t)g.race.body[0].speed * 11) >> 16);
    int speed = (int)(short)(v / 20), fuel = (int)(short)(s->fuel >> 16);
    int counter[2], k;

    if (speed < 0)
        speed = -speed;
    draw_image(anim_image(DAMAGE_ANIM, s->damage / 10, 9), 217, 438);
    draw_image(anim_image(FUEL_ANIM, fuel / 16, 8), 110, 422);
    draw_image(anim_image(SPEEDO_ANIM, speed, 10), 20, 414);
    counter[0] = s->boosts;     /* 0x19391(13) */
    counter[1] = s->rockets;    /* 0x19391(0x17) */
    for (k = 0; k < 2; k++) {
        int x = k ? 494 : 389;
        draw_image(anim_image(DIGITS_ANIM, counter[k] / 10 % 10, 9), x, 415);
        draw_image(anim_image(DIGITS_ANIM, counter[k] % 10, 9), x + 16, 415);
    }
}

static void compose(void)
{
    int y;

    if (g.hud.pixels)
        memcpy(g.fb, g.hud.pixels, sizeof g.fb);
    else
        memset(g.fb, 0, sizeof g.fb);
    draw_view();
    for (y = 0; y < RC_VIEW_H; y++)
        memcpy(g.fb + (RC_VIEW_Y + y) * SCREEN_W + RC_VIEW_X,
               g.view + y * RC_VIEW_W, RC_VIEW_W);
    draw_hud();
    if (g.light < 5)            /* screen coordinates: box at x 226 */
        draw_image(anim_image(LIGHTS_ANIM, g.light, 4), 226, 50);
}

static void present(void)
{
    static uint32_t rgb[SCREEN_W * SCREEN_H];
    int i;

    for (i = 0; i < SCREEN_W * SCREEN_H; i++) {
        rc_rgb c = g.pal[g.fb[i]];
        rgb[i] = 0xFF000000u | c.r << 16 | c.g << 8 | c.b;
    }
    SDL_UpdateTexture(g.tex, NULL, rgb, SCREEN_W * 4);
    SDL_RenderClear(g.ren);
    SDL_RenderCopy(g.ren, g.tex, NULL, NULL);
    SDL_RenderPresent(g.ren);
}

static void step(void)
{
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    rc_input in;

    in.up = k[SDL_SCANCODE_UP];
    in.down = k[SDL_SCANCODE_DOWN];
    in.left = k[SDL_SCANCODE_LEFT];
    in.right = k[SDL_SCANCODE_RIGHT];
    in.boost = k[SDL_SCANCODE_RSHIFT];
    in.rocket = k[SDL_SCANCODE_RETURN];
    if (k[SDL_SCANCODE_F1]) g.race.camera_racer = RC_REDCAT;
    if (k[SDL_SCANCODE_F2]) g.race.camera_racer = RC_BULL;
    if (k[SDL_SCANCODE_F3]) g.race.camera_racer = RC_MUIS;
    rc_race_tick(&g.race, &in);
    rc_race_over(&g.race);
}

static void frame(void)
{
    SDL_Event e;
    Uint32 now;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            g.running = 0;
        if (e.type != SDL_KEYDOWN)
            continue;
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE: g.running = 0; break;
        case SDLK_PAGEUP: start_race(g.map_index - 1); break;
        case SDLK_PAGEDOWN: start_race(g.map_index + 1); break;
        }
    }
    now = SDL_GetTicks();
    if (g.light < 5) {
        /* race_main draws one frame, then start_lights blocks */
        if (!g.light_until)
            g.light_until = now + light_ms[0];
        else if ((Sint32)(now - g.light_until) >= 0) {
            if (++g.light < 5)
                g.light_until = now + light_ms[g.light];
            else {
                g.race.started = 1;
                g.next_frame = now;
            }
        }
    } else {
        int steps = 0;
        while ((Sint32)(now - g.next_frame) >= 0 && steps++ < 4) {
            step();
            g.next_frame += g.frame_ms;
        }
        if (steps > 4)
            g.next_frame = now;
    }
    compose();
    present();
#ifdef __EMSCRIPTEN__
    if (!g.running)
        emscripten_cancel_main_loop();
#else
    SDL_Delay(1);
#endif
}

int main(int argc, char **argv)
{
    int map = 0, fps = 30, i;
    uint32_t size;
    const uint8_t *d;

#ifdef __EMSCRIPTEN__
    g.dir = "/data";            /* preloaded by port/web/build.sh */
#else
    if (argc < 2) {
        fprintf(stderr, "usage: %s <data dir> [map 0..22] [--fps N]\n",
                argv[0]);
        return 2;
    }
    g.dir = argv[1];
#endif
    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--fps") == 0 && i + 1 < argc)
            fps = atoi(argv[++i]);
        else
            map = atoi(argv[i]);
    }
    if (fps < 1)
        fps = 30;
    g.frame_ms = 1000 / fps;
    if (rc_archive_open(&g.rcs, g.dir, "RCS") != 0 || load_sprites() != 0) {
        fprintf(stderr, "RCS and RC archives not found in %s\n", g.dir);
        return 1;
    }
    if (load_trans() != 0) {
        fprintf(stderr, "TRANS.DAT not found in %s\n", g.dir);
        return 1;
    }
    d = rc_archive_find(&g.rcs, "RACESCR3.PCX", &size);
    if (!d || rc_pcx_decode(&g.hud, d, size) != 0 || g.hud.w != SCREEN_W ||
        g.hud.h != SCREEN_H)
        rc_pcx_free(&g.hud);
    if (start_race(map) != 0)
        return 1;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    g.win = SDL_CreateWindow("RedCat's Razende Rekenrace",
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             SCREEN_W * 2, SCREEN_H * 2, SDL_WINDOW_RESIZABLE);
    g.ren = SDL_CreateRenderer(g.win, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(g.ren, SCREEN_W, SCREEN_H);
    g.tex = SDL_CreateTexture(g.ren, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
    g.running = 1;
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (g.running)
        frame();
#endif
    SDL_Quit();
    return 0;
}
