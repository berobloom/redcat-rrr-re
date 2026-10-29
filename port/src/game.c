/* redcat: the game's round as MOTOR.EXE runs it: each world opens with a
   race, then for each of its modules the exercises (REKEN.EXE) and the
   race (RRR.EXE) of that stage, with what they earned; after the last
   one the next world.  There is no menu yet: RRRMENU's Start! is taken
   at once.

   usage: redcat <data dir> [map 0..22] [--race] [--groep 3..8]
                 [--module 1..4] [--fps N]

   Exercises: module 1 (Handig rekenen): digits (keys or the keypad)
   answer, Backspace or TERUG takes one back, Enter or OK confirms; module 2
   (Wat is waar?): 1, 2, 3 or a click picks the true row; module 3
   (Memory): click a SOM card, then the ANTWOORD card with its value; Enter
   ends it early; module 4 (Grote getallenspel): 1..4 take a number,
   + - x : (keypad too) an operator, Enter or = works it out, Backspace
   or T is TERUG, O is OPNIEUW.  S or Stop quits.
   --module starts with that module (in the first world that has it);
   --race only races, the map given or track1a, without the exercises.

   Race keys (RRRIO.INF defaults): arrows steer and accelerate, right Shift
   boost, Enter rocket, F1/F2/F3 follow RedCat/Brutus/Snitch, Esc quits.

   The original's frame loop has no timer (research/discoveries.md "The
   race: speed"): it ran 29 frames/s at DOSBox cycles=40000 and faster on
   faster machines.  This port runs a fixed rate, 30 by default. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "anims.h"
#include "archive.h"
#include "gfx8.h"
#include "race.h"
#include "reken.h"
#include "reken_screen.h"

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
    int exercising;             /* the exercise screen is up */
    int flow;                   /* the round, not just one race */
    int map_override;           /* the next race's map, -1: the stage's */
    Uint32 race_end;            /* when the finished race gives way */
    rc_archive reken, menu;
    rc_exercise ex;
    rc_session session;
    Uint32 last_tick;
    int logged_sum;
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
    int start_order[RC_RACERS] = { 0, 1, 2 }, k;
    uint32_t ms, ts;
    const uint8_t *md, *td;

    if (g.flow)                 /* race_init: RRRIO.INF +0x1fc */
        for (k = 0; k < RC_RACERS; k++)
            start_order[k] = g.session.order[k] / 2 % RC_RACERS;
    rc_race_free(&g.race);
    g.race_end = 0;
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

static int exercise_key(SDL_Keycode k)
{
    if (k >= SDLK_0 && k <= SDLK_9)
        return '0' + (int)(k - SDLK_0);
    if (k >= SDLK_KP_1 && k <= SDLK_KP_9)
        return '1' + (int)(k - SDLK_KP_1);
    switch (k) {
    case SDLK_KP_0: return '0';
    case SDLK_BACKSPACE: case SDLK_t: return RC_KEY_BACK;
    case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_EQUALS: return RC_KEY_OK;
    case SDLK_o: return RC_KEY_AGAIN;
    case SDLK_KP_PLUS: case SDLK_PLUS: return RC_KEY_PLUS;
    case SDLK_KP_MINUS: case SDLK_MINUS: return RC_KEY_MINUS;
    case SDLK_KP_MULTIPLY: case SDLK_x: case SDLK_PAGEUP: return RC_KEY_TIMES;
    case SDLK_KP_DIVIDE: case SDLK_COLON: case SDLK_UP: return RC_KEY_DIVIDE;
    case SDLK_h: return RC_KEY_HELP;
    case SDLK_s: return RC_KEY_STOP;
    }
    return 0;
}

static void to_menu(void);

/* RRR.EXE: the stage's race with the counters of RRRIO.INF. */
static void play_race(void)
{
    int map = g.map_override >= 0 ? g.map_override
                                  : rc_stage_map(g.session.world, g.session.stage);

    g.map_override = -1;
    printf("world %d, stage %d\n", g.session.world, g.session.stage);
    if (map < 0 || start_race(map) != 0) {
        g.running = 0;
        return;
    }
    rc_race_set_player(&g.race, g.session.fuel, g.session.repair,
                       g.session.boosts, g.session.rockets);
}

/* REKEN.EXE: the next module of the world, or back to the menu. */
static void to_reken(int module)
{
    if (module < 0)
        module = rc_pick_module(&g.session);
    if (module < 0) {
        to_menu();
        return;
    }
    rc_exercise_start(&g.ex, &g.session, module, (uint32_t)time(NULL));
    g.logged_sum = 0;
    g.exercising = 1;
}

/* RRRMENU.EXE: moves on to the next world once the last race of one is
   run (0x10f95), and Start! (0x1212c) clears race_done, so it hands over
   to RRR: the stage's race again, or the new world's first. */
static void to_menu(void)
{
    if (rc_world_next(&g.session))
        printf("on to world %d\n", g.session.world);
    g.session.race_done = 0;
    rc_session_store(&g.session);
    play_race();
}

/* race end 0x1585e: the player's counters back into RRRIO.INF, the
   finishing order, 3 / 2 / 1 points, race_done. */
static void race_result(void)
{
    const rc_stats *p = &g.race.stats[RC_REDCAT];
    rc_session *s = &g.session;
    int k;

    s->fuel = p->fuel < 0 ? 0 : p->fuel >> 16;
    s->boosts = p->boosts;
    s->stat18 = p->f10;
    s->stat1c = p->f14;
    s->rockets = p->rockets;
    s->repair = p->damage < 0 ? 0 : p->damage;
    for (k = 0; k < RC_RACERS; k++) {
        s->progress[k] = g.race.wp_next[k];
        s->order[k] = g.race.order[k];
        s->points[g.race.order[k] / 2 % RC_RACERS] += 3 - k;
    }
    s->race_done = 1;
    rc_session_store(s);
    printf("finish: %d %d %d; points %d %d %d; fuel %d, repair %d, "
           "boosts %d, rockets %d\n", s->order[0] / 2, s->order[1] / 2,
           s->order[2] / 2, s->points[0], s->points[1], s->points[2],
           s->fuel, s->repair, s->boosts, s->rockets);
}

/* After the last sum: REKEN writes the counters back (0x199cf) and exits
   to RRR. */
static void finish_exercises(void)
{
    const rc_reken *r = &g.ex.r;

    g.exercising = 0;
    if (g.ex.stopped) {
        g.running = 0;
        return;
    }
    rc_reken_finish(r, &g.session);
    printf("%d of %d right: fuel %d, repair %d, boosts %d, rockets %d\n",
           r->correct, g.ex.module == 3 ? r->asked : RC_SUMS_PER_ROUND,
           r->fuel, r->repair, r->boosts,
           r->rockets);
    play_race();
}

static void exercise_frame(void)
{
    SDL_Event e;
    Uint32 now = SDL_GetTicks();

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT ||
            (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
            g.running = 0;
        else if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            int k = exercise_key(e.key.keysym.sym);
            if (k)
                rc_exercise_key(&g.ex, k);
        } else if (e.type == SDL_MOUSEBUTTONDOWN &&
                   e.button.button == SDL_BUTTON_LEFT)
            rc_exercise_click(&g.ex, e.button.x, e.button.y);
    }
    rc_exercise_tick(&g.ex, (int)(now - g.last_tick));
    g.last_tick = now;
    if (g.ex.r.asked != g.logged_sum) {     /* as REKEN /DEBUG logs it */
        g.logged_sum = g.ex.r.asked;
        if (g.ex.module == 4)
            printf("round %d: %d %d %d %d -> %d\n", g.ex.r.asked, g.ex.r.num[0],
                   g.ex.r.num[1], g.ex.r.num[2], g.ex.r.num[3], g.ex.r.target);
        else if (g.ex.module == 3)
            printf("pair %d: %d found\n", g.ex.r.asked, g.ex.r.correct);
        else if (g.ex.module == 2)
            printf("sum %d: %s | %s | %s  (true: row %d)\n", g.ex.r.asked,
                   g.ex.r.choice[rc_module2_choice(&g.ex.r, 0)],
                   g.ex.r.choice[rc_module2_choice(&g.ex.r, 1)],
                   g.ex.r.choice[rc_module2_choice(&g.ex.r, 2)],
                   g.ex.r.true_row + 1);
        else
            printf("sum %d: %s%d\n", g.ex.r.asked, g.ex.r.text, g.ex.r.answer);
    }
    rc_exercise_draw(&g.ex);
    memcpy(g.fb, g.ex.fb, sizeof g.fb);
    memcpy(g.pal, g.ex.pal, sizeof g.pal);
    present();
    if (rc_exercise_done(&g.ex))
        finish_exercises();
}

static void frame(void)
{
    SDL_Event e;
    Uint32 now;

    if (g.exercising) {
        exercise_frame();
#ifdef __EMSCRIPTEN__
        if (!g.running)
            emscripten_cancel_main_loop();
#else
        SDL_Delay(5);
#endif
        return;
    }

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
        /* The podium (0x15a9f) and the world's final ranking (0x15a14)
           are not drawn yet: the race runs on a few seconds instead. */
        if (g.flow && !g.race_end && rc_race_over(&g.race)) {
            race_result();
            g.race_end = now + 4000;
        } else if (g.race_end && (Sint32)(now - g.race_end) >= 0) {
            g.race_end = 0;
            to_reken(-1);
        }
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
    int map = -1, fps = 30, groep = 6, module = 0, race_only = 0, i;
    uint32_t size;
    const uint8_t *d;

#ifdef __EMSCRIPTEN__
    g.dir = "/data";            /* preloaded by port/web/build.sh */
#else
    if (argc < 2) {
        fprintf(stderr, "usage: %s <data dir> [map 0..22] [--race] "
                "[--groep 3..8] [--module 1..4] [--fps N]\n", argv[0]);
        return 2;
    }
    g.dir = argv[1];
#endif
    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--fps") == 0 && i + 1 < argc)
            fps = atoi(argv[++i]);
        else if (strcmp(argv[i], "--groep") == 0 && i + 1 < argc)
            groep = atoi(argv[++i]);
        else if (strcmp(argv[i], "--module") == 0 && i + 1 < argc)
            module = atoi(argv[++i]);
        else if (strcmp(argv[i], "--race") == 0)
            race_only = 1;
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
    /* A new pupil's session (RRRIO.INF defaults): world 0, stage 0, so
       the round opens with De Welkomstrace's race. */
    rc_session_default(&g.session);
    if (groep >= 3 && groep <= 8)
        g.session.groep = groep;
    /* --module: the first world that plays it, at its stage */
    if (module >= 1 && module <= 4)
        for (i = 0; i < 6; i++)
            if (rc_stage_map(i, module) >= 0) {
                g.session.world = i;
                g.session.stage = module;
                break;
            }
    rc_session_store(&g.session);
    g.map_override = map;
    if (!race_only && (rc_archive_open(&g.reken, g.dir, "REKEN") != 0 ||
                       rc_archive_open(&g.menu, g.dir, "MENU") != 0 ||
                       rc_exercise_load(&g.ex, &g.reken, &g.menu) != 0)) {
        fprintf(stderr, "REKEN and MENU archives not found in %s; "
                "racing only\n", g.dir);
        race_only = 1;
    }
    g.flow = !race_only;
    g.running = 1;
    if (race_only) {
        if (start_race(map < 0 ? 0 : map) != 0)
            return 1;
    } else if (module >= 1 && module <= 4)
        to_reken(module == 4 && g.session.groep == 3 ? 3 : module);
    else
        to_menu();
    if (!g.running)
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
    g.last_tick = SDL_GetTicks();
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (g.running)
        frame();
#endif
    SDL_Quit();
    return 0;
}
