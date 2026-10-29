/* redcat: the game's round as MOTOR.EXE runs it: each world opens with a
   race, then for each of its modules the exercises (REKEN.EXE) and the
   race (RRR.EXE) of that stage, with what they earned; after the last
   one the next world.  It opens with RRRMENU's screen (menu.c): S or
   Enter Start!, R the next world, 3..8 the groep, G the sound, P the
   Rapport, E Einde (the high scores); Esc in a race goes back to it.

   usage: redcat <data dir> [map 0..22] [--race] [--groep 3..8]
                 [--module 1..4] [--fps N] [--save DIR]

   The record (RRRIO.INF) and the pupil files (%8.8d.SMD) are kept in the
   save directory: --save, else SDL's preferences path; in the browser
   /save, kept in IndexedDB.  The game data directory is only read.

   Exercises: module 1 (Handig rekenen): digits (keys or the keypad)
   answer, Backspace or TERUG takes one back, Enter or OK confirms; module 2
   (Wat is waar?): 1, 2, 3 or a click picks the true row; module 3
   (Memory): click a SOM card, then the ANTWOORD card with its value; Enter
   ends it early; module 4 (Grote getallenspel): 1..4 take a number,
   + - x : (keypad too) an operator, Enter or = works it out, Backspace
   or T is TERUG, O is OPNIEUW.  S or Stop quits.
   --module starts with that module (in the first world that has it);
   --race only races, the map given or track1a, without the exercises.

   Race keys: those of RRRIO.INF, set in the menu's Bediening (defaults:
   arrows steer and accelerate, Enter boost, right Shift rocket); T the
   horn; F1/F2/F3 follow RedCat/Brutus/Snitch, Esc goes back to the menu.

   Sound: the menu's Geluid (RRRIO +0x30): 3 effects and music, 1 effects,
   0 none.  One mixer (audio.c) is set up as the program on screen sets up
   its Sound Blaster.

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
#include "audio.h"
#include "gfx8.h"
#include "highscore.h"
#include "menu.h"
#include "podium.h"
#include "race.h"
#include "reken.h"
#include "reken_screen.h"
#include "smd.h"

#define SCREEN_W 640
#define SCREEN_H 480
#define RC_IMAGES 1056
#define LIGHTS_ANIM 86          /* start_lights 0x1103e: [0x804bc] + 0x2b4 */
#define DIGITS_ANIM 18          /* hud_update 0x1918d: anims 18..21 */
#define SPEEDO_ANIM 19
#define FUEL_ANIM 20
#define DAMAGE_ANIM 21

#define AUDIO_RATE 22050        /* the device; the mixer resamples */

/* start_lights: lamp images and how long each stays, ms */
static const int light_ms[5] = { 1500, 1000, 1000, 1000, 500 };

static struct {
    const char *dir;
    char save[1024];            /* RRRIO.INF and the SMD files; "" none */
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
    Uint32 race_end;            /* while the podium shows: when it goes */
    rc_podium podium;
    rc_menu mainmenu;           /* RRRMENU */
    int have_menu, in_menu;
    rc_archive reken, menu;
    rc_exercise ex;
    rc_session session;
    Uint32 last_tick;
    int logged_sum;
    int light;                  /* lamp shown, 5 once the race runs */
    Uint32 light_until, next_frame, frame_ms;
    int running;

    SDL_AudioDeviceID audio;
    rc_archive sndrcs, rrrmsnd, rekensnd;
    rc_soundbank bank_race, bank_menu, bank_reken;
    rc_mixer mix;
    rc_pcx wacht;               /* the race's loading screen */
    int race_music;             /* RRR [0x6fc58] */
    int race_intro;             /* start.wav before a world's first race */
    int race_rcend;             /* the finish: 1 rcend.wav plays, 2 done */
    int menu_sound;             /* the Geluid setting the menu last had */
} g;

static void audio_callback(void *u, Uint8 *stream, int len)
{
    rc_mixer_mix(&g.mix, (int16_t *)stream, len / 2, AUDIO_RATE);
}

static void audio_setup(const rc_soundbank *bank, int tracks, int rate)
{
    if (g.audio)
        SDL_LockAudioDevice(g.audio);
    rc_mixer_setup(&g.mix, bank, tracks, rate);
    if (g.audio)
        SDL_UnlockAudioDevice(g.audio);
}

/* sndman's play (RRR 0x10350): only on a free track */
static int snd_play(int track, int sample)
{
    int r;

    if (g.audio)
        SDL_LockAudioDevice(g.audio);
    r = rc_mixer_play(&g.mix, track, sample);
    if (g.audio)
        SDL_UnlockAudioDevice(g.audio);
    return r;
}

static void snd_stop(int track)
{
    if (g.audio)
        SDL_LockAudioDevice(g.audio);
    rc_mixer_stop(&g.mix, track);
    if (g.audio)
        SDL_UnlockAudioDevice(g.audio);
}

static int snd_busy(int track)
{
    int r;

    if (g.audio)
        SDL_LockAudioDevice(g.audio);
    r = rc_mixer_busy(&g.mix, track);
    if (g.audio)
        SDL_UnlockAudioDevice(g.audio);
    return r;
}

/* REKEN's sounds through the exercise screen's hooks */
static int ex_busy(void *ctx, int track) { return snd_busy(track); }
static void ex_play(void *ctx, int track, int sample) { snd_play(track, sample); }
static void ex_stop(void *ctx, int track) { snd_stop(track); }

/* RRR: effects with Geluid bit 0 ([0x751a8] & 1, sound initialised at
   all), music with bit 1 as well */
static int race_fx(void)
{
    return g.session.speech & 1;
}

/* play_fx (0x103d1): the first free of tracks 1..3; track 0 is music */
static void play_fx(int sample)
{
    int t;

    if (!race_fx())
        return;
    for (t = 1; t < 4; t++)
        if (snd_play(t, sample) == 0)
            return;
}

/* race_init, after the dashboard: tune_m.wav on track 0, looped by
   race_main playing it again whenever the track is free (tune.wav, the
   stereo one, only under /STEREO) */
static void race_music_start(void)
{
    if (g.race_music && snd_play(0, RC_SND_TUNE_M) != 0)
        g.race_music = 0;
}

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

/* PC scan codes (set 1, extended keys as their keypad twins, as the
   original's keyboard handler sees them) of the SDL keys */
static int pc_scan(SDL_Scancode c)
{
    static const char letters[] = {
        30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38, 50,   /* a..m */
        49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,   /* n..z */
    };

    if (c >= SDL_SCANCODE_A && c <= SDL_SCANCODE_Z)
        return letters[c - SDL_SCANCODE_A];
    if (c >= SDL_SCANCODE_1 && c <= SDL_SCANCODE_0)
        return 2 + (int)(c - SDL_SCANCODE_1);
    switch (c) {
    case SDL_SCANCODE_ESCAPE: return 1;
    case SDL_SCANCODE_BACKSPACE: return 14;
    case SDL_SCANCODE_TAB: return 15;
    case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: return 28;
    case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: return 29;
    case SDL_SCANCODE_LSHIFT: return 42;
    case SDL_SCANCODE_RSHIFT: return 54;
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT: return 56;
    case SDL_SCANCODE_SPACE: return 57;
    case SDL_SCANCODE_CAPSLOCK: return 58;
    case SDL_SCANCODE_HOME: case SDL_SCANCODE_KP_7: return 71;
    case SDL_SCANCODE_UP: case SDL_SCANCODE_KP_8: return 72;
    case SDL_SCANCODE_PAGEUP: case SDL_SCANCODE_KP_9: return 73;
    case SDL_SCANCODE_LEFT: case SDL_SCANCODE_KP_4: return 75;
    case SDL_SCANCODE_KP_5: return 76;
    case SDL_SCANCODE_RIGHT: case SDL_SCANCODE_KP_6: return 77;
    case SDL_SCANCODE_END: case SDL_SCANCODE_KP_1: return 79;
    case SDL_SCANCODE_DOWN: case SDL_SCANCODE_KP_2: return 80;
    case SDL_SCANCODE_PAGEDOWN: case SDL_SCANCODE_KP_3: return 81;
    case SDL_SCANCODE_INSERT: case SDL_SCANCODE_KP_0: return 82;
    case SDL_SCANCODE_DELETE: case SDL_SCANCODE_KP_PERIOD: return 83;
    default: return 0;
    }
}

static int pc_down(const Uint8 *k, int scan)
{
    int c;

    for (c = 0; c < SDL_NUM_SCANCODES; c++)
        if (k[c] && pc_scan((SDL_Scancode)c) == scan)
            return 1;
    return 0;
}

static void step(void)
{
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    rc_input in;
    int i;

    /* the keys of RRRIO.INF (RRR 0x1b469; set in the menu's Bediening) */
    in.left = pc_down(k, rc_session_get(&g.session, 0x58));
    in.right = pc_down(k, rc_session_get(&g.session, 0x5c));
    in.up = pc_down(k, rc_session_get(&g.session, 0x60));
    in.down = pc_down(k, rc_session_get(&g.session, 0x64));
    in.boost = pc_down(k, rc_session_get(&g.session, 0x50));
    in.rocket = pc_down(k, rc_session_get(&g.session, 0x54));
    if (k[SDL_SCANCODE_F1]) g.race.camera_racer = RC_REDCAT;
    if (k[SDL_SCANCODE_F2]) g.race.camera_racer = RC_BULL;
    if (k[SDL_SCANCODE_F3]) g.race.camera_racer = RC_MUIS;
    rc_race_tick(&g.race, &in);
    for (i = 0; i < g.race.sounds; i++)
        play_fx(g.race.sound[i]);
    if (g.race_rcend)           /* 0x112fd: only the objects run */
        return;
    rc_race_over(&g.race);
    if (k[SDL_SCANCODE_T])      /* race_main 0x113af: the horn */
        play_fx(RC_SND_HORN);
    if (g.race_music && !snd_busy(0))
        race_music_start();
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

/* The browser's /save lives in IndexedDB: write it back (save.js). */
static void save_sync(void)
{
#ifdef __EMSCRIPTEN__
    EM_ASM(FS.syncfs(false, function (e) { if (e) console.log(e); }););
#endif
}

/* RRRIO.INF into the save directory, as each program writes it on exit */
static void save_session(void)
{
    char path[1100];
    FILE *f;

    if (!*g.save)
        return;
    rc_session_store(&g.session);
    snprintf(path, sizeof path, "%s/RRRIO.INF", g.save);
    if (!(f = fopen(path, "wb")) ||
        fwrite(g.session.raw, RC_SESSION_SIZE, 1, f) != 1)
        fprintf(stderr, "cannot write %s\n", path);
    if (f)
        fclose(f);
    save_sync();
}

static int load_session(void)
{
    char path[1100];
    uint8_t d[RC_SESSION_SIZE];
    FILE *f;
    int ok;

    if (!*g.save)
        return -1;
    snprintf(path, sizeof path, "%s/RRRIO.INF", g.save);
    if (!(f = fopen(path, "rb")))
        return -1;
    ok = fread(d, 1, sizeof d, f) == sizeof d;
    fclose(f);
    return ok ? rc_session_parse(&g.session, d, sizeof d) : -1;
}

static void race_sound_start(void);

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
    race_sound_start();
}

/* race_init: start.wav over the loading screen when the stage is the
   world's first (0x1aea5), until it ends or S, Esc, Enter or Space; then
   the music */
static void race_sound_start(void)
{
    audio_setup(&g.bank_race, 4, 11025);
    g.race_music = race_fx() && (g.session.speech & 2);
    g.race_rcend = 0;
    g.race_intro = race_fx() && g.session.stage == 0 &&
                   snd_play(0, RC_SND_START) == 0;
    if (!g.race_intro)
        race_music_start();
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
    audio_setup(&g.bank_reken, 2, 22050);
    rc_exercise_start(&g.ex, &g.session, module, (uint32_t)time(NULL));
    g.logged_sum = 0;
    g.exercising = 1;
}

/* RRRMENU.EXE: moves on to the next world once the last race of one is
   run (0x10f95); Start! (0x1212c) clears race_done, so it hands over to
   RRR: the stage's race again, or the new world's first.  Without the
   RRRMENU files Start! is taken at once. */
static void to_menu(void)
{
    if (g.have_menu) {
        rc_menu_start(&g.mainmenu, &g.session, (uint32_t)time(NULL));
        audio_setup(&g.bank_menu, 2, 11025);
        g.menu_sound = g.mainmenu.sound;
        g.in_menu = 1;
        return;
    }
    if (rc_world_next(&g.session))
        printf("on to world %d\n", g.session.world);
    g.session.race_done = 0;
    rc_session_store(&g.session);
    play_race();
}

static void menu_frame(void)
{
    rc_menu *m = &g.mainmenu;
    SDL_Event e;
    Uint32 now = SDL_GetTicks();
    int i;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            g.running = 0;
        else if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            int scan = pc_scan(e.key.keysym.scancode);
            /* Esc aborts Bediening; on the main screen the port quits */
            if (!rc_menu_key(m, scan) && scan == 1)
                g.running = 0;
        }
        else if (e.type == SDL_MOUSEMOTION)
            rc_menu_mouse(m, e.motion.x, e.motion.y);
        else if (e.type == SDL_MOUSEBUTTONDOWN &&
                 e.button.button == SDL_BUTTON_LEFT)
            rc_menu_click(m, e.button.x, e.button.y);
    }
    rc_menu_tick(m, (int)(now - g.last_tick));
    g.last_tick = now;
    /* Geluid: to 1 the music stops, to 0 both tracks (0x11f92, 0x11fd4) */
    if (m->sound != g.menu_sound) {
        if (m->sound != 3)
            snd_stop(0);
        if (m->sound == 0)
            snd_stop(1);
        g.menu_sound = m->sound;
    }
    /* menu_sound (0x10b4f): track 1, cutting off what plays there */
    for (i = 0; i < m->sounds; i++)
        if (m->sound) {
            snd_stop(1);
            snd_play(1, m->sound_q[i]);
        }
    m->sounds = 0;
    /* menu_loop 0x11c51: REDCAT.WAV on track 0 whenever it is free */
    if (m->sound & 2)
        snd_play(0, RC_MSND_REDCAT);
    rc_menu_draw(m);
    memcpy(g.fb, m->fb, sizeof g.fb);
    memcpy(g.pal, m->pal, sizeof g.pal);
    present();
    if (m->result == RC_MENU_START) {
        g.in_menu = 0;
        printf("menu: world %d, groep %d\n", g.session.world,
               g.session.groep);
        save_session();
        play_race();
    } else if (m->result == RC_MENU_QUIT) {
        save_session();
        g.running = 0;
    }
}

static int high_rnd(void *ctx)
{
    return rand();
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
    save_session();
    /* high_score 0x1b6bf: RedCat's points this world under the record's
       name (+0x68), into RRRHIGH.DAT */
    if (*g.save) {
        rc_high h;
        char name[32];
        snprintf(name, sizeof name, "%.29s", (const char *)s->raw + 0x68);
        rc_high_load(&h, g.save, high_rnd, NULL);
        if (rc_high_add(&h, g.save, name, s->points[0], s->world) >= 0)
            save_sync();
    }
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
    /* smd_append (0x19f07) at the end of each module: module 0..3 as
       played, the sums asked (Memory: the pairs tried) and right; module
       4 hands over [0x5c094], which nothing counts up, so 0 */
    if (*g.save) {
        const char *name = (const char *)g.session.raw + 0x68;
        char date[11];
        rc_smd_date(date);
        if (rc_smd_append(g.save, *name ? name : "RedCat", g.ex.module - 1,
                          g.ex.module == 3 ? r->asked : RC_SUMS_PER_ROUND,
                          g.ex.module == 4 ? 0 : r->correct,
                          g.session.groep, date) != 0)
            fprintf(stderr, "cannot write the SMD file in %s\n", g.save);
    }
    save_session();
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

    if (g.in_menu || g.exercising) {
        if (g.in_menu)
            menu_frame();
        else
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
        if (g.race_intro) {     /* S, Esc, Enter, Space end start.wav */
            switch (e.key.keysym.scancode) {
            case SDL_SCANCODE_S: case SDL_SCANCODE_ESCAPE:
            case SDL_SCANCODE_RETURN: case SDL_SCANCODE_SPACE:
                snd_stop(0);
                break;
            default:
                break;
            }
            continue;
        }
        if (g.race_rcend == 1 && e.key.keysym.sym == SDLK_ESCAPE) {
            snd_stop(0);        /* 0x11359: Esc ends rcend.wav */
            continue;
        }
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:       /* RRR: to the menu, nothing saved */
            if (g.flow && g.have_menu && !g.race_end)
                to_menu();
            else
                g.running = 0;
            break;
        case SDLK_PAGEUP: start_race(g.map_index - 1); break;
        case SDLK_PAGEDOWN: start_race(g.map_index + 1); break;
        }
    }
    now = SDL_GetTicks();
    if (g.race_intro) {
        if (!snd_busy(0)) {
            g.race_intro = 0;
            race_music_start();
        } else if (g.wacht.pixels) {
            memcpy(g.fb, g.wacht.pixels, sizeof g.fb);
            memcpy(g.pal, g.wacht.palette, sizeof g.pal);
            present();
            memcpy(g.pal, g.tiles.palette, sizeof g.pal);
            goto out;
        }
    }
    if (g.race_end) {           /* the podium stays up */
        if ((Sint32)(now - g.race_end) >= 0) {
            g.race_end = 0;
            to_reken(-1);
        }
        goto out;
    }
    if (g.light < 5) {
        /* race_main draws one frame, then start_lights blocks */
        if (!g.light_until)
            g.light_until = now + light_ms[0];
        else if ((Sint32)(now - g.light_until) >= 0) {
            if (++g.light < 5) {
                g.light_until = now + light_ms[g.light];
                if (race_fx())  /* ti, ti, ti, tu on track 1 */
                    snd_play(1, g.light < 4 ? RC_SND_TI : RC_SND_TU);
            } else {
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
        /* The race stops at the finish (0x11364); after race_end the
           podium for 4 s (0x15a9f).  After a stage-4 race the original
           plays the winner's film instead (0x15a14, w*.smk), which is not
           ported: the podium stands in. */
        /* the finish (0x112cb): the music stops; with music on, rcend.wav
           plays on track 0 and the objects run on until it ends */
        if (g.flow && rc_race_over(&g.race)) {
            if (g.race_rcend == 0) {
                snd_stop(0);
                g.race_rcend = g.race_music &&
                               snd_play(0, RC_SND_RCEND) == 0 ? 1 : 2;
            } else if (g.race_rcend == 1 && !snd_busy(0))
                g.race_rcend = 2;
        }
        if (g.race_rcend == 2) {
            char name[32];      /* RRRIO +0x68; RRRMENU puts RedCat if empty */
            snprintf(name, sizeof name, "%.31s",
                     (const char *)g.session.raw + 0x68);
            race_result();
            compose();
            rc_podium_draw(&g.podium, &g.rcs, g.session.world,
                           g.session.order, g.session.points,
                           *name ? name : "RedCat", g.fb, g.pal);
            present();
            g.race_end = now + 4000;
            goto out;
        }
    }
    compose();
    present();
out:
#ifdef __EMSCRIPTEN__
    if (!g.running)
        emscripten_cancel_main_loop();
#else
    SDL_Delay(1);
#endif
}

int main(int argc, char **argv)
{
    int map = -1, fps = 30, groep = 0, module = 0, race_only = 0, i;
    const char *save = NULL;
    uint32_t size;
    const uint8_t *d;

#ifdef __EMSCRIPTEN__
    g.dir = "/data";            /* preloaded by port/web/build.sh */
#else
    if (argc < 2) {
        fprintf(stderr, "usage: %s <data dir> [map 0..22] [--race] "
                "[--groep 3..8] [--module 1..4] [--fps N] [--save DIR]\n",
                argv[0]);
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
        else if (strcmp(argv[i], "--save") == 0 && i + 1 < argc)
            save = argv[++i];
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
#ifdef __EMSCRIPTEN__
    save = "/save";             /* mounted by save.js */
#else
    if (!save) {
        char *pref = SDL_GetPrefPath("Davilex", "RedCat");
        if (pref) {
            size_t n = strlen(pref);
            if (n && (pref[n - 1] == '/' || pref[n - 1] == '\\'))
                pref[n - 1] = 0;
            snprintf(g.save, sizeof g.save, "%s", pref);
            SDL_free(pref);
        }
    } else
#endif
        snprintf(g.save, sizeof g.save, "%s", save);
    if (race_only)
        *g.save = 0;
    /* The saved record, else a new pupil's (RRRIO.INF defaults): world 0,
       stage 0, so the round opens with De Welkomstrace's race. */
    if (load_session() == 0)
        printf("record: %s/RRRIO.INF\n", g.save);
    else
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
    if (g.flow && rc_menu_load(&g.mainmenu, g.dir) != 0)
        fprintf(stderr, "RRRMENU files not found in %s; no menu\n", g.dir);
    else if (g.flow)
        g.have_menu = 1;
    g.mainmenu.save_dir = g.save;
    g.ex.snd_busy = ex_busy;
    g.ex.snd_play = ex_play;
    g.ex.snd_stop = ex_stop;
    g.running = 1;
    /* the sound archives; without them the game is silent */
    if (rc_archive_open(&g.sndrcs, g.dir, "SNDRCS") == 0)
        rc_soundbank_load(&g.bank_race, &g.sndrcs, rc_race_sound_names,
                          RC_SND_COUNT);
    if (rc_archive_open(&g.rrrmsnd, g.dir, "RRRMSND") == 0)
        rc_soundbank_load(&g.bank_menu, &g.rrrmsnd, rc_menu_sound_names,
                          RC_MSND_COUNT);
    if (rc_archive_open(&g.rekensnd, g.dir, "REKENSND") == 0)
        rc_soundbank_load(&g.bank_reken, &g.rekensnd, rc_reken_sound_names,
                          RC_RSND_COUNT);
    d = rc_archive_find(&g.rcs, "WACHT.PCX", &size);
    if (!d || rc_pcx_decode(&g.wacht, d, size) != 0 ||
        g.wacht.w != SCREEN_W || g.wacht.h != SCREEN_H)
        rc_pcx_free(&g.wacht);
    if (race_only) {
        if (start_race(map < 0 ? 0 : map) != 0)
            return 1;
        race_sound_start();
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
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0) {
        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = AUDIO_RATE;
        want.format = AUDIO_S16SYS;
        want.channels = 1;
        want.samples = 512;
        want.callback = audio_callback;
        g.audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (g.audio)
            SDL_PauseAudioDevice(g.audio, 0);
    }
    if (!g.audio)
        fprintf(stderr, "no sound: %s\n", SDL_GetError());
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
