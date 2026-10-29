/* rcview: look at the game's data through the port's readers.

   usage: rcview <data dir> [map 0..22]

   Map mode shows a race map at the original's 640x480 (the tile map scrolls,
   no racers yet).  Keys: arrows scroll, PgUp/PgDn change map, Tab switches to
   the PCX screens (left/right step through them), Esc quits. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "archive.h"
#include "gfx8.h"

#define SCREEN_W 640
#define SCREEN_H 480

typedef struct {
    const char *archive, *name;
} screen_ref;

static const screen_ref screens[] = {
    { "RRRMENU", "menubs01.pcx" }, { "RRRMENU", "MENUKEY.PCX" },
    { "RRRMENU", "MENUJOY.PCX" },  { "RRRMENU", "rapport.pcx" },
    { "RRRMENU", "HIGHSCOR.PCX" }, { "RCS", "WACHT.PCX" },
    { "RCS", "RACESCR3.PCX" },     { "RCS", "rbs.pcx" },
    { "RCS", "rsb.pcx" },          { "RCS", "brs.pcx" },
    { "RCS", "bsr.pcx" },          { "RCS", "sbr.pcx" },
    { "RCS", "srb.pcx" },          { "REKEN", "mod_124.pcx" },
    { "REKEN", "module3.pcx" },    { "REKEN", "wachten.pcx" },
};
#define NUM_SCREENS ((int)(sizeof screens / sizeof *screens))

static struct {
    const char *dir;
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    uint8_t fb[SCREEN_W * SCREEN_H];
    rc_rgb pal[256];
    rc_archive rcs;
    rc_map map;
    rc_tileset tiles;
    int map_index, scroll_x, scroll_y;
    int pcx_mode, pcx_index;
    rc_pcx pcx;
    int running;
} g;

static void load_map(int index)
{
    uint32_t ms, ts;
    const uint8_t *md, *td;

    rc_map_free(&g.map);
    g.map_index = (index + RC_NUM_MAPS) % RC_NUM_MAPS;
    md = rc_archive_find(&g.rcs, rc_map_files[g.map_index], &ms);
    td = rc_archive_find(&g.rcs, rc_map_tilesets[g.map_index], &ts);
    if (!md || !td || rc_map_parse(&g.map, md, ms) != 0 ||
        rc_tileset_parse(&g.tiles, td, ts) != 0) {
        fprintf(stderr, "map %d (%s) failed to load\n", g.map_index,
                rc_map_files[g.map_index]);
        exit(1);
    }
    g.scroll_x = g.scroll_y = 0;
    printf("map %d: %s, %s, %dx%d tiles\n", g.map_index,
           rc_map_files[g.map_index], rc_map_tilesets[g.map_index], g.map.w,
           g.map.h);
}

static void load_screen(int index)
{
    rc_archive a;
    uint32_t size;
    const uint8_t *d;

    g.pcx_index = (index + NUM_SCREENS) % NUM_SCREENS;
    rc_pcx_free(&g.pcx);
    if (rc_archive_open(&a, g.dir, screens[g.pcx_index].archive) != 0)
        return;
    d = rc_archive_find(&a, screens[g.pcx_index].name, &size);
    if (d && rc_pcx_decode(&g.pcx, d, size) == 0)
        printf("%s:%s\n", screens[g.pcx_index].archive,
               screens[g.pcx_index].name);
    rc_archive_close(&a);
}

static void draw_map(void)
{
    int max_x = g.map.w * RC_TILE_W - SCREEN_W;
    int max_y = g.map.h * RC_TILE_H - SCREEN_H;
    int y;

    if (g.scroll_x > max_x) g.scroll_x = max_x;
    if (g.scroll_y > max_y) g.scroll_y = max_y;
    if (g.scroll_x < 0) g.scroll_x = 0;
    if (g.scroll_y < 0) g.scroll_y = 0;
    memset(g.fb, 0, sizeof g.fb);
    for (y = 0; y < SCREEN_H; y++) {
        int my = g.scroll_y + y, ty = my / RC_TILE_H, x;
        if (ty >= g.map.h)
            break;
        for (x = 0; x < SCREEN_W; x++) {
            int mx = g.scroll_x + x, tx = mx / RC_TILE_W;
            const rc_cell *c;
            if (tx >= g.map.w)
                break;
            c = &g.map.cells[ty * g.map.w + tx];
            g.fb[y * SCREEN_W + x] =
                g.tiles.tiles[c->tile * RC_TILE_W * RC_TILE_H +
                              (my % RC_TILE_H) * RC_TILE_W + mx % RC_TILE_W];
        }
    }
    memcpy(g.pal, g.tiles.palette, sizeof g.pal);
}

static void draw_pcx(void)
{
    int y;

    memset(g.fb, 0, sizeof g.fb);
    if (!g.pcx.pixels)
        return;
    for (y = 0; y < SCREEN_H && y < g.pcx.h; y++)
        memcpy(g.fb + y * SCREEN_W, g.pcx.pixels + y * g.pcx.w,
               g.pcx.w < SCREEN_W ? g.pcx.w : SCREEN_W);
    memcpy(g.pal, g.pcx.palette, sizeof g.pal);
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

static void frame(void)
{
    const Uint8 *keys;
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            g.running = 0;
        if (e.type != SDL_KEYDOWN)
            continue;
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE: g.running = 0; break;
        case SDLK_TAB:
            g.pcx_mode = !g.pcx_mode;
            if (g.pcx_mode && !g.pcx.pixels)
                load_screen(0);
            break;
        case SDLK_PAGEUP: if (!g.pcx_mode) load_map(g.map_index - 1); break;
        case SDLK_PAGEDOWN: if (!g.pcx_mode) load_map(g.map_index + 1); break;
        case SDLK_LEFT: if (g.pcx_mode) load_screen(g.pcx_index - 1); break;
        case SDLK_RIGHT: if (g.pcx_mode) load_screen(g.pcx_index + 1); break;
        }
    }
    if (!g.pcx_mode) {
        keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_LEFT]) g.scroll_x -= 8;
        if (keys[SDL_SCANCODE_RIGHT]) g.scroll_x += 8;
        if (keys[SDL_SCANCODE_UP]) g.scroll_y -= 8;
        if (keys[SDL_SCANCODE_DOWN]) g.scroll_y += 8;
        draw_map();
    } else {
        draw_pcx();
    }
    present();
#ifdef __EMSCRIPTEN__
    if (!g.running)
        emscripten_cancel_main_loop();
#endif
}

int main(int argc, char **argv)
{
#ifdef __EMSCRIPTEN__
    g.dir = "/data";            /* preloaded by port/web/build.sh */
#else
    if (argc < 2) {
        fprintf(stderr, "usage: %s <data dir> [map 0..22]\n", argv[0]);
        return 2;
    }
    g.dir = argv[1];
#endif
    if (rc_archive_open(&g.rcs, g.dir, "RCS") != 0) {
        fprintf(stderr, "RCS.IND/RCS.IMG not found in %s\n", g.dir);
        return 1;
    }
    load_map(argc > 2 ? atoi(argv[2]) : 0);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    g.win = SDL_CreateWindow("RedCat's Razende Rekenrace", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, SCREEN_W * 2,
                             SCREEN_H * 2, SDL_WINDOW_RESIZABLE);
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
