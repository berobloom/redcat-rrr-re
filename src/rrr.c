/*
 * RRR.EXE, the race: the game's own C, rebuilt function by function.
 *
 * Compiled by src/check.sh with Watcom C/C++ 10.5, `wcc386 -bt=dos -3r -s -ox` (MATCHING.md), and compared
 * with RRR.EXE function by function. Functions are in address order. The original was several
 * modules (sndman.c, main.c, new2io.c, order.c, shade.c, init.c, raceanim.c, heap.c, map.c, ...;
 * research/discoveries.md); a module moves to its own file in src/ once its boundary is known
 * (order.c, new2io.c, map.c, racer.c, hud.c, ... so far; racer.c and map.c only partly), the rest
 * stays here, so each function is compared on its own. Types,
 * globals and prototypes are in rrr.h; names and layouts are this project's, with their evidence
 * in reverse-engineering/analysis/rrr_names.txt.
 */
#include "rrr.h"

/* ---- 0x10010 -------------------------------------------------------------------------------- */

char *fn_10010(char *s)
{
    sprintf(strbuf, "%s", s);
    return strbuf;
}

/* ---- 0x10029 -------------------------------------------------------------------------------- */

FILE *fn_10029(char *name, char *mode)
{
    return fopen(name, mode);
}

/* ---- 0x1002e -------------------------------------------------------------------------------- */

int movie_dir_exists(void)
{
    struct find_t ft;
    return _dos_findfirst(movie_dir, _A_SUBDIR, &ft) == 0;
}

/* ---- 0x10052 -------------------------------------------------------------------------------- */

void movie_dir_make(void)
{
    mkdir(movie_dir);
    if (!movie_dir_exists())
        rc_error("Couldn't create directory %s", movie_dir);
}

/* ---- 0x10078 -------------------------------------------------------------------------------- */

int wav_lookup(char *name)
{
    int i;
    for (i = 0; i < 34; i++)
        if (stricmp(name, wav_names[i]) == 0)
            return wav_handles[i];
    return -1;
}

/* ---- 0x100ae -------------------------------------------------------------------------------- */

int blaster_port(void)
{
    char *env;
    int i;
    env = getenv("BLASTER");
    if (env != NULL)
        for (i = 0; i < 6; i++)
            if (strstr(env, blaster_ports[i]) != NULL)
                return i;
    return -1;
}

/* ---- 0x100f1 -------------------------------------------------------------------------------- */

int blaster_irq(void)
{
    char *env;
    int i;
    env = getenv("BLASTER");
    if (env != NULL)
        for (i = 0; i < 5; i++)
            if (strstr(env, blaster_irqs[i]) != NULL)
                return i;
    return -1;
}

/* ---- 0x10134 -------------------------------------------------------------------------------- */

int blaster_dma(void)
{
    char *env;
    int i;
    env = getenv("BLASTER");
    if (env != NULL)
        for (i = 0; i < 3; i++)
            if (strstr(env, blaster_dmas[i]) != NULL)
                return i;
    return -1;
}

/* ---- 0x10177 -------------------------------------------------------------------------------- */

int snd_cfg_load(char *name)
{
    FILE *f = fopen(name, "rb");
    if (f == NULL)
        return 1000;
    fread(&snd_cfg_a, 2, 1, f);
    fread(&snd_cfg_b, 2, 1, f);
    fread(&snd_cfg_c, 2, 1, f);
    fread(&snd_cfg_d, 2, 1, f);
    fread(&snd_cfg_e, 2, 1, f);
    fread(&snd_cfg_f, 2, 1, f);
    fread(&snd_cfg_g, 2, 1, f);
    fclose(f);
    return 0;
}

/* ---- 0x1023d -------------------------------------------------------------------------------- */

int snd_cfg_save(char *name)
{
    FILE *f = fopen(name, "wb+");
    if (f == NULL)
        return 1000;
    fwrite(&snd_cfg_a, 2, 1, f);
    fwrite(&snd_cfg_b, 2, 1, f);
    fwrite(&snd_cfg_c, 2, 1, f);
    fwrite(&snd_cfg_d, 2, 1, f);
    fwrite(&snd_cfg_e, 2, 1, f);
    fwrite(&snd_cfg_f, 2, 1, f);
    fwrite(&snd_cfg_g, 2, 1, f);
    fclose(f);
    return 0;
}

/* ---- 0x10303 -------------------------------------------------------------------------------- */

int sound_on(void);

int snd_track_free(int track)
{
    if (sound_on()) {
        if (track < 0 || track >= 4)
            rc_error("%s(%d): track out of bounds %d (0..4)\n", "sndman.c", 157, track);
        if (tracks[track] != -1 && mix_track_free(track))
            return 0;
    }
    return 1;
}

/* ---- 0x10350 -------------------------------------------------------------------------------- */

int snd_play(int track, int sample, int vol)
{
    if (!sound_on())
        return 0;
    if (track < 0 || track >= 4)
        rc_error("%s(%d): track out of bounds %d (0..4)\n", "sndman.c", 176, track);
    if (sample < 0 || sample >= 34)
        rc_error("%s(%d): sample index out of bounds %d (0..%d)\n", "sndman.c", 179, sample, 33);
    if (tracks[track] != -1 && !snd_track_free(track))
        return 0;
    tracks[track] = sample;
    sndbuf_play(track, sample, (unsigned char)vol);
    return 1;
}

/* ---- 0x103d1 -------------------------------------------------------------------------------- */

int play_fx(int sample, int dist)
{
    int vol, t;

    if (!sound_on())
        return 0;
    if (dist < 1)
        dist = 1;
    vol = 1500 / dist * 64;
    if (vol > 64)
        vol = 64;
    else if (vol < 0)
        vol = 0;
    for (t = 1; t < 4; t++) {
        if (snd_track_free(t)) {
            snd_play(t, sample, vol);
            return 1;
        }
    }
    return 0;
}

/* ---- 0x1043a -------------------------------------------------------------------------------- */

int sound_init(short argc, char **argv)
{
    int dma, port, irq, i, err;

    for (i = 0; i < 4; i++)
        tracks[i] = -1;
    if (has_arg("/NOSOUND", argc, argv))
        return -1;
    if (has_arg("/NS", argc, argv))
        return -1;
    snd_cfg_load("sb.ins");
    if (snd_cfg_b == -1) {
        snd_cfg_b = blaster_port();
        if (snd_cfg_b == -1)
            return -1;
    }
    if (snd_cfg_c == -1) {
        snd_cfg_c = blaster_irq();
        if (snd_cfg_c == -1)
            return -1;
    }
    if (snd_cfg_d == -1) {
        snd_cfg_d = blaster_dma();
        if (snd_cfg_d == -1)
            return -1;
    }
    irq = sb_irqs[snd_cfg_c];
    dma = sb_dmas[snd_cfg_d];
    port = sb_ports[snd_cfg_b];
    for (i = 0; i < 5; i++)
        if (has_arg(irq_opts[i], argc, argv))
            irq = sb_irqs[i];
    for (i = 0; i < 8; i++)
        if (has_arg(port_opts[i], argc, argv))
            port = sb_ports[i];
    for (i = 0; i < 3; i++)
        if (has_arg(dma_opts[i], argc, argv))
            dma = sb_dmas[i];
    debug_printf("Adres:%x, IRQ:%d, DMA:%d\n", port, irq, dma);
    err = fn_1c5e4(port, irq, dma);
    if (err)
        rc_error("error setting soundblaster dsp variables {error=%d}", err);
    if (has_arg("/STEREO", argc, argv))
        fn_1c85b(1);
    else
        fn_1c85b(0);
    fn_1d2a6(34, wav_handles, wav_names, 11025);
    num_tracks = 4;
    if (easy_init(30, 34, wav_handles, wav_names, 33)) {
        rc_error("Soundblaster initialisatie fout. Probeer andere instellingen");
        exit(0);
    }
    debug_printf("init done\n");
    if (has_arg("/VMIN", argc, argv)) {
        snd_volume = 0x33;
        debug_printf("Volume set to minimum\n");
    }
    if (has_arg("/VLOW", argc, argv)) {
        snd_volume = 0x77;
        debug_printf("Volume set to low\n");
    }
    if (has_arg("/VNORM", argc, argv)) {
        snd_volume = 0xaa;
        debug_printf("Volume set to normal\n");
    }
    if (has_arg("/VMAX", argc, argv)) {
        snd_volume = 0xff;
        debug_printf("Volume set to maximum\n");
    }
    fn_1d329(snd_volume);
    fn_1d3a6(0);
    fn_1d4a0(0);
    fn_1d2ac(0xff);
    sound_ready = 1;
    return 0;
}

/* ---- 0x10754 -------------------------------------------------------------------------------- */

int sound_on(void)
{
    return sound_ready;
}

/* ---- 0x1075a -------------------------------------------------------------------------------- */

void screenshot(void)
{
    char name[16];
    fn_1f579();
    sprintf(name, "screen%#2.2d.pcx", shot_no++);
    fn_20d1f(0, screen_w - 1, 0, screen_h - 1, name);
    if (shot_no > 99)
        shot_no = 0;
    fn_1f5a0();
}

/* ---- 0x107c1 -------------------------------------------------------------------------------- */

void banner(void)
{
    printf("\xda\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xb7\n");
    printf("\xb3                                Redcats Razende Rekenrace V1.2               \xba\n");
    printf("\xb3                                 (c) 1997 Davilex Software BV                \xba\n");
    printf("\xd4\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbc\n");
}

/* ---- 0x107f6 -------------------------------------------------------------------------------- */

void goodbye(void)
{
    printf("\n \n U kunt het spel opstarten door RCRACE in te typen en op [Enter] te drukken.\n");
    printf("\n \n Veel plezier !\n");
    exit(0);
}

/* ---- 0x10817 -------------------------------------------------------------------------------- */

void init_game(char **argv, int argc);
/* stack arguments, callee pops (ret 8), saves only ESI/EDI: race_main pushes argc, then argv */
#pragma aux init_game parm routine [] modify [eax ebx ecx edx];

/* main.c's own declaration of sndman.c's sound_init: an int argc, which init_game passes without
   the movsx that sndman.c's short parameter gets (0x10a45) */
int main_sound_init(int argc, char **argv);
#pragma aux main_sound_init "sound_init_";

void init_game(char **argv, int argc)
{
    time_t t;

    view_size = 0;
    if (!has_arg("/DAVILEX", argc, argv))
        goodbye();
    if (!has_arg("/M", argc, argv))
        fn_1f210(0);
    debug_on = has_arg("/DEBUG", argc, argv) != 0;
    cd_check(0, "intro.img");
    view_tiles_w = 19;
    view_tiles_h = 10;
    if (has_arg("/V", argc, argv))
        fn_1f1fe(1);
    fn_1f204(25);
    fn_1f222(480, 640);
    fn_21af9();
    fn_22ab5(fn_218e4(v6cad0, "rcs.ind"), fn_218e4(v6cb60, "rcs.img"));
    fn_22ec9(fn_218e4(v6cad0, "sndrcs.ind"), fn_218e4(v6cb60, "sndrcs.img"));
    time(&t);
    srand(t);
    fn_1f329(0);
    debug_printf("Loading palette.\n");
    fn_2369f("wacht.pcx", 15);
    fn_1f3dc();
    debug_printf("Palette loaded and stored ok.\n");
    debug_printf("Loading trans-tables.\n");
    if (trans_load("trans.dat")) {
        debug_printf("Load trans-tables failed, generating tables.\n");
        trans_generate();
        trans_save("trans.dat");
        debug_printf("Trans-tables generated & saved ok.\n");
    } else {
        debug_printf("Trans-tables loaded ok.\n");
    }
    debug_printf("Loading ascii sets.\n");
    if (fn_24a9a(font12_name))
        rc_error("unable to create ascii set1");
    if (fn_24a9a(font20_name))
        rc_error("unable to create ascii set2");
    fn_246b5(font12_name);
    fn_24879(15);
    fn_24942(0);
    debug_printf("Ascii sets loaded ok.\n");
    rrrio_load();
    fn_1b469();
    debug_printf("Init sound manager.\n");
    if ((rrrio.sound & 1) && main_sound_init(argc, argv) == 0) {
        sound_enabled = 1;
        debug_printf("Init sound manager ok.\n");
    } else {
        sound_enabled = 0;
        debug_printf("Sound off.\n");
    }
    debug_printf("Alloc sprites.\n");
    if (fn_1f83f(306))
        rc_error("not enough memory for xvga sprites");
    debug_printf("Sprites alloc'd ok.\n");
    debug_printf("Alloc objects.\n");
    if (fn_25bbd(306))
        rc_error("not enough memory for objects");
    debug_printf("Objects alloc'd ok.\n");
    map_no = stage_map_index();
    if (map_no < 0 || map_no > 22) {
        map_no = rand() % 22;
        if (map_no < 0 || map_no > 22)
            rc_error("%s(%d) : Illegal ammount of maps in rrrio.inf: %d", "main.c", 270, map_no);
    }
    v5feba = 1;
    debug_printf("Loading bakground map : %s and images track : %s\n", track_maps[map_no], track_imgs[map_no]);
    if (load_map(track_maps[map_no]))
        rc_error("not enough memory for foreground map file <%s>", track_maps[rrrio.world]);
    if (load_xvga_map_images(track_imgs[map_no]))
        rc_error("not enough memory for foreground map file <'dirt.img'>");
    debug_printf("Map and images loaded ok.\n");
    debug_printf("Searching waypoints in map\n");
    if (find_waypoints())
        rc_error("Waypoint-search found missing waypoint!");
    else
        debug_printf("Waypoints search returned ok.\n");
    debug_printf("Init anims.\n");
    init_anims();
    debug_printf("use config.\n");
    if (fn_15c66())
        rc_error("memory overflow initialising anims.");
    fn_1bcb7();
    if (fn_15e02())
        rc_error("memory overflow initialising anims.");
    fn_1bcb7();
    debug_printf("RC Drive Anims initialised ok using %ld bytes. \n", anim_bytes);
    if (fn_15f9e())
        rc_error("memory overflow initialising anims.");
    fn_1bcb7();
    debug_printf("BULL Drive Anims initialised ok using %ld bytes. \n", anim_bytes);
    if (fn_1613a())
        rc_error("memory overflow initialising anims.");
    fn_1bcb7();
    debug_printf("MUIS Drive Anims initialised ok using %ld bytes. \n", anim_bytes);
    debug_printf("Direction tables initialised ok.\n");
    scroll_y = 0;
    scroll_x = 0;
    tile_y = 0;
    tile_x = 0;
    debug_printf("Init direction tables.\n");
    cos_sin_init();
    dir_tbl_init();
    turn_tbl_init();
    merge_tbl_init();
    debug_printf("Init objects.\n");
    clear_objects();
    init_objects();
    debug_printf("Objects initialised ok.\n");
    camera_start(0);
    debug_printf("Searching bonus objects in map\n");
    find_bonus_objects();
    debug_printf("Bonus search returned.\n");
    music_on = (!has_arg("/NOMUSIC", argc, argv) && (rrrio.sound & 2)) ? 1 : 0;
    stereo = has_arg("/STEREO", argc, argv);
    if (sound_enabled && fn_1aea5()) {
        debug_printf("Starting music.\n");
        snd_play(0, 31, 64);
        do {
            if (key_pressed(0x1f)) {
                while (key_pressed(0x1f))
                    ;
                mix_stop_track(0);
                break;
            }
            if (key_pressed(0x01)) {
                while (key_pressed(0x01))
                    ;
                mix_stop_track(0);
                break;
            }
            if (key_pressed(0x1c)) {
                while (key_pressed(0x1c))
                    ;
                mix_stop_track(0);
                break;
            }
            if (key_pressed(0x39)) {
                while (key_pressed(0x39))
                    ;
                mix_stop_track(0);
                break;
            }
        } while (mix_track_free(0));
    }
    fn_2369f("racescr3.pcx", 15);
    debug_printf("Initialising dashboard.\n");
    fn_15166();
    if (sound_enabled && music_on) {
        debug_printf("Starting music.\n");
        if (stereo) {
            debug_printf("playing stereo music\n");
            snd_play(0, 11, 64);
            if (!mix_track_free(0))
                music_on = 0;
        } else {
            debug_printf("playing mono music\n");
            snd_play(0, 12, 64);
            if (!mix_track_free(0))
                music_on = 0;
        }
    }
    debug_printf("exit init_game : %d,%d\n", view_tiles_w, view_tiles_h);
}

/* ---- 0x10eff -------------------------------------------------------------------------------- */

int finish_check(void)
{
    if (finished0 && !finish_done[0]) {
        finish_done[0] = 1;
        finish_order[finish_count] = 0;
        finish_count++;
    }
    if (finished2 && !finish_done[1]) {
        finish_done[1] = 1;
        finish_order[finish_count] = 2;
        finish_count++;
    }
    if (finished4 && !finish_done[2]) {
        finish_done[2] = 1;
        finish_order[finish_count] = 4;
        finish_count++;
    }
    if ((finish_count == 2 && finish_done[0]) || finish_count == 3) {
        if (!finish_done[1])
            finish_order[finish_count] = 2;
        else if (!finish_done[2])
            finish_order[finish_count] = 4;
        if (finish_order[0] < 0 || finish_order[1] < 0 || finish_order[2] < 0)
            rc_error("%s(%d) Finish line not complete!", "main.c", 441);
        debug_printf(" finishline 1 : %d 2 : %d 3 : %d\n", finish_order[0], finish_order[1], finish_order[2]);
        return 1;
    }
    return 0;
}

/* ---- 0x1103e -------------------------------------------------------------------------------- */

/* differs: 3 rows, the fourth lamp (lamp[7], 0x11144): the game computes frames + i*12 as
   imul edx; mov eax,[frames]; add eax,edx, ours with the registers swapped. The other four lamps
   match from the same text. Tried: a frame pointer local, direct frames[..] expressions, frames + i,
   an int index local, byte arithmetic, f += i, (int) cast, *(frames + i), mixes of these for lamp 7. */
void start_lights(void)
{
    struct frame *f;

    f = &frames[animset[0x56].steps[0].frame];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    delay_ms(1500);
    f = &frames[animset[0x56].steps[1].frame];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    snd_play(1, 30, 64);
    delay_ms(1000);
    f = &frames[animset[0x56].steps[2].frame];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    snd_play(1, 30, 64);
    delay_ms(1000);
    f = &frames[animset[0x56].steps[3].frame];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    snd_play(1, 30, 64);
    delay_ms(1000);
    f = &frames[animset[0x56].steps[4].frame];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    snd_play(1, 32, 64);
    delay_ms(500);
}

/* ---- 0x111e8 -------------------------------------------------------------------------------- */

/* differs: 6 rows, block placement in the quit-after-finish fill (0x1138d..0x11481): the game tests
   finished2 with jne to the finished2 branch and keeps the finished4 -> order[2] = 2 block before
   it, ours tests with je and places them the other way round; same size, same stores. 24 spellings
   of the block tried (nested if/else both ways round, else-if chains on the four cases, && forms,
   a goto, separate ifs, both tests negated [11 rows] and the shared order[2] = 4 spelled as an
   else-if [14]); the others merge the two order[1] = 2 stores (794 bytes) or grow. The game does
   NOT merge them, and order[2] = 4 is shared with the finished2 branch - a source shape that
   keeps both order[1] = 2 stores separate is what is missing. */
int main(int argc, char **argv)
{
    int quit = 0;
    int best = -1;
    int best_points = 0;
    int i;

    init_game(argv, argc);
    for (;;) {
        if (key_pressed(0x01) || key_pressed(0x1f)) {
            quit = 1;
            break;
        }
        if (key_pressed(0x3b))
            camera_racer = 0;
        if (key_pressed(0x3c))
            camera_racer = 2;
        if (key_pressed(0x3d))
            camera_racer = 4;
        if (switch_anims())
            rc_error("%s(%d) : Error switching anims", "main.c", 503);
        objects_tick();
        camera_follow(camera_racer);
        sort_draw_order();
        joystick_to_keys();
        hud_update();
        draw_view(scroll_y % 40, scroll_x % 32);
        if (!lights_done) {
            start_lights();
            lights_done = 1;
        }
        if (finish_check()) {
            mix_stop_track(0);
            if (sound_enabled && music_on)
                snd_play(0, 29, 64);
            do {
                objects_tick();
                camera_follow(0);
                sort_draw_order();
                draw_view(scroll_y % 40, scroll_x % 32);
            } while (!snd_track_free(0) && music_on && !key_pressed(0x01));
            break;
        }
        if (key_pressed(0x14))
            play_fx(4, 0);
        if (key_pressed(0x44) && key_pressed(0x2a) && key_pressed(0x1d))
            screenshot();
        if (sound_enabled && music_on) {
            if (stereo) {
                debug_printf("playing Stereo Music\n");
                snd_play(0, 11, 64);
            } else {
                debug_printf("playing Stereo Music\n");
                snd_play(0, 12, 64);
            }
        }
    }
    if (quit && finished0 && finish_done[0]) {
        if (finished2) {
            if (finished4)
                finish_order[1] = 2;
            finish_order[2] = 4;
        } else if (finished4) {
            finish_order[2] = 2;
        } else {
            finish_order[1] = 2;
            finish_order[2] = 4;
        }
        quit = 0;
    }
    race_end(quit);
    if (quit)
        user_exit(42);
    high_score(rrrio.names[0], rrrio.points[0]);
    if (stage_is_last()) {
        for (i = 0; i < 3; i++) {
            if (best_points < rrrio.points[i]) {
                best = i;
                best_points = rrrio.points[i];
            }
        }
        if (best >= 0 && best < 3)
            winner_film(best * 2);
    } else {
        podium(quit);
    }
    user_exit(43);
    debug_printf("do user_exit succeeded\n");
    return 43;
}

/* ---- 0x138ed camera_follow -----------------------------------------------------------------
 * The view eases 1/16 of the way per frame toward the racer minus the view
 * size plus a lead of 128 px horizontally and 160 px vertically, the lead
 * following the sign of the racer's own velocity; scroll_x/scroll_y are then
 * clamped to 0 .. map size - view size and tile_x/tile_y recomputed.
 * scroll_step_x/scroll_step_y (0x56060, 0x56064) are written here and by
 * fn_13a78 and read nowhere in RRR.EXE.
 *
 * near miss: 62 -> 19 rows. The C is established: vw/vh/tx/ty short (the word
 * loads of view_tiles_w/h are Watcom narrowing the short locals' assignments),
 * x/y int with (short) casts, int vx/vy locals (the game tests registers),
 * both ternaries inline in tx/ty (the (5*sy)<<5 shape of sy*160 says no sy
 * local), statements vw,vh,x,y,vx,vy,tx,ty. Prologue, [eax*8] indexing, x/y
 * normalize-at-def/plain-reload, both easings, both clamps (map_h - vh) * 40
 * and the vh*20 shape then match line for line, and our epilogue jmps the
 * shared turn_tbl_init epilogue like the game's. What is left is Watcom
 * allocation ties, and the declaration order resolves one but never both:
 * one family of orders spills tx to [esp+8] (frame 0xc, x@0, y@4, the game's)
 * but mirrors vx/vy (ours EDX/ESI, the game ESI/EDX; sx, ty and the step
 * registers ripple); the order below assigns vx/vy right but keeps tx in ESI
 * (frame 0x8) and y's reload, the step registers and the map_w clamp ripple.
 * ~200 spellings: 50+ declaration orders (random sweeps, all 55
 * transpositions of the one below, statement interleavings, casts,
 * short/int/unsigned mixes, an unsigned short alias for view_tiles_w/h,
 * vw * 16, sx * 128, (map_w - vw) * 32, renamed locals, an int txs
 * intermediate, sx/sy locals, register keyword, int t). Same class as
 * draw_view 0x11526's homes (watcom-levers "register tie"); take the next
 * line.
 * -------------------------------------------------------------------------------------------- */

void camera_follow(int racer)
{
    int x;
    int sx;
    unsigned t;
    short tx;
    short vh;
    int vx;
    short ty;
    short vw;
    int vy;
    int sy;
    int y;

    vw = view_tiles_w + 1;
    vh = view_tiles_h + 1;
    x = (short)(bodies[racer].x >> 16);
    y = (short)((bodies[racer].y - bodies[racer].z) >> 16);
    vx = bodies[racer].vx;
    vy = bodies[racer].vy;
    tx = (short)(x - (vw << 4) + ((vx < 0 ? -1 : vx > 0) << 7));
    ty = (short)(y - vh * 20 + (vy < 0 ? -1 : vy > 0) * 160);
    scroll_step_x = (tx - scroll_x) >> 4;
    if (tx != scroll_x)
        scroll_x += scroll_step_x;
    scroll_step_y = (ty - scroll_y) >> 4;
    if (ty != scroll_y)
        scroll_y += scroll_step_y;
    if (scroll_x < 0)
        scroll_x = 0;
    t = (map_w - vw) << 5;
    if (scroll_x > t)
        scroll_x = t;
    if (scroll_y < 0)
        scroll_y = 0;
    t = (map_h - vh) * 40;
    if (scroll_y > t)
        scroll_y = t;
    tile_x = scroll_x / 32;
    tile_y = scroll_y / 40;
}

/* ---- 0x13a78 --------------------------------------------------------------------------------
 * camera_follow's twin for a caller that already knows the pixel position: same view size, same
 * 1/16 easing into scroll_x/scroll_y, same clamps and tile_x/tile_y; the lead is 64 px by 80 px
 * instead of 128 by 160, and the caller (fn_13bb8, which draws the view from scroll_x/scroll_y
 * straight after) hands it objects[racer].x/y rather than a body.
 *
 * differs: 15 rows, all register mirrors - the game keeps the vx sign in EAX and shifts it there
 * and puts vw << 4 in EBX (we load vw into EAX and move the sign to EBX), the same mirror for the
 * vy sign (its -1 goes to EBX), and in the map_h clamp it extends vh, still in DX, into EAX and
 * loads map_h into EDX, we the other way round; we end up 1 byte longer. Refs agree (nmrefs), so
 * the C is right and the allocation is Watcom's. Tried: 14 declaration orders of vw/vh/tx/ty/t,
 * the lead as (sign << 6), sign * 64, (sign * 5) << 4, (x - A) + B and (x + B) - A, the two
 * leads as separate statements, a hoisted w = vw << 4, sx/sy locals, the clamps as * 32, * 5 * 8,
 * * 8 * 5, * 40u and with unsigned casts, a separate (map_w - vw) local, vw as int (both spill
 * differently) and this function compiled in an object of its own with camera_follow and
 * fn_13bb8 (camera_follow stays at its 62 rows there, the same mirror).
 * -------------------------------------------------------------------------------------------- */

void fn_13a78(int x, int y, int vx, int vy)
{
    short vw, vh;
    short tx, ty;
    unsigned t;

    vw = view_tiles_w + 1;
    vh = view_tiles_h + 1;
    tx = x - (vw << 4) + ((vx < 0 ? -1 : vx > 0) << 6);
    ty = y - vh * 20 + (vy < 0 ? -1 : vy > 0) * 80;
    scroll_step_x = (tx - scroll_x) >> 4;
    if (tx != scroll_x)
        scroll_x += scroll_step_x;
    scroll_step_y = (ty - scroll_y) >> 4;
    if (ty != scroll_y)
        scroll_y += scroll_step_y;
    if (scroll_x < 0)
        scroll_x = 0;
    t = (map_w - vw) << 5;
    if (scroll_x > t)
        scroll_x = t;
    if (scroll_y < 0)
        scroll_y = 0;
    t = (map_h - vh) * 40;
    if (scroll_y > t)
        scroll_y = t;
    tile_x = scroll_x / 32;
    tile_y = scroll_y / 40;
}

/* ---- 0x13bb8 -------------------------------------------------------------------------------- */

void fn_13bb8(int racer)
{
    objects_tick();
    fn_13a78(objects[racer].x, objects[racer].y, (short)(bodies[racer].vx >> 16), (short)(bodies[racer].vy >> 16));
    draw_view(scroll_y % 40, scroll_x % 32);
}


/* ---- 0x13c34 -------------------------------------------------------------------------------- */

/* SKIP: 2 rows, and only the slot of x (ours [esp+0x10], the game [esp+8]; everything else, frame
   included, is identical). Matched on the way: a double t for the angle (the game's frame has its 8
   bytes), x and i and y in that order. Tried: all 120 declaration orders of i, a, y, x, t (2 rows is
   the floor, no order puts x at 8), six statement orders for x = / i = / y = with six declaration
   orders, x and y as temporaries (cos_tbl[i = a] = ...), (int)a indexes. Open question 24 (10.5 vs a
   patched 10.5/10.6) is the likely cause: this is Watcom's FP local layout, not the C. */
void cos_sin_init(void)
{
    int i;
    float a;
    int y, x;
    double t;
    for (a = 0; a < 32; a++) {
        t = (a + 8.0) * -0.19634954087500001;
        x = cos(t) * 65536.0;
        i = a;
        y = sin(t) * 65536.0;
        cos_tbl[i] = x;
        sin_tbl[i] = y;
    }
}

/* ---- 0x13cba -------------------------------------------------------------------------------- */

void dir_tbl_init(void)
{
    int r, c, d;
    for (r = 0; r < 33; r++) {
        for (c = 0; c < 33; c++) {
            d = atan2(c + -16.0, r + -16.0) * 5.0929581782756487 + 16.0 + 0.5;
            dir_tbl[r][c] = (32 - d + 24) & 31;
            debug_printf("%2d ", dir_tbl[r][c]);
        }
        debug_printf("\n");
    }
}

/* ---- 0x13d60 -------------------------------------------------------------------------------- */

int direction_to_waypoint(int racer, int x, int y)
{
    int i, j;
    float dy, dx, d;

    dx = bodies[racer].x * (1.0 / 65536) - x * (1.0 / 65536);
    dy = bodies[racer].y * (1.0 / 65536) - y * (1.0 / 65536);
    d = sqrt(dx * dx + dy * dy);
    if (d == 0)
        return 0;
    i = (int)((dx / d + 1) * 16.0) % 33;
    j = (int)((dy / d + 1) * 16.0) % 33;
    return dir_tbl[i][j];
}


/* ---- 0x13e3d -------------------------------------------------------------------------------- */

/* both coordinates are the racer's own, so dx = dy = 0 and this always returns 0 (one fild, fsub st(0)) */
int aim_at_player(int racer, int target)
{
    int i, j;
    float dy, dx, d;

    dx = bodies[racer].x * (1.0 / 65536) - bodies[racer].x * (1.0 / 65536);
    dy = bodies[racer].y * (1.0 / 65536) - bodies[racer].y * (1.0 / 65536);
    d = sqrt(dx * dx + dy * dy);
    if (d == 0)
        return 0;
    i = (int)((dx / d + 1) * 16.0) % 33;
    j = (int)((dy / d + 1) * 16.0) % 33;
    return dir_tbl[i][j];
}


/* ---- 0x13f00 -------------------------------------------------------------------------------- */

void turn_tbl_init(void)
{
    int cur, tgt, d;
    for (cur = 0; cur < 32; cur++) {
        for (tgt = 0; tgt < 32; tgt++) {
            d = tgt - cur;
            if (d == 0)
                turn_tbl[tgt][cur] = cur;
            else if (d > 0) {
                if (d > 16)
                    turn_tbl[tgt][cur] = (cur + 31) & 31;
                else
                    turn_tbl[tgt][cur] = (cur + 1) & 31;
            } else {
                if (d < -16)
                    turn_tbl[tgt][cur] = (cur + 1) & 31;
                else
                    turn_tbl[tgt][cur] = (cur + 31) & 31;
            }
        }
    }
}

/* ---- 0x13f91 -------------------------------------------------------------------------------- */

void merge_tbl_init(void)
{
    int cur, tgt, d;
    for (cur = 0; cur < 32; cur++) {
        for (tgt = 0; tgt < 32; tgt++) {
            d = tgt - cur;
            if (d == 0)
                merge_tbl[tgt][cur] = cur;
            else if (d > 0) {
                if (d > 16)
                    merge_tbl[tgt][cur] = (cur + 1) & 31;
                else
                    merge_tbl[tgt][cur] = (cur + 31) & 31;
            } else {
                if (d < -16)
                    merge_tbl[tgt][cur] = (cur + 31) & 31;
                else
                    merge_tbl[tgt][cur] = (cur + 1) & 31;
            }
        }
    }
}

/* ---- 0x1401d -------------------------------------------------------------------------------- */

/* SKIP: 8 rows, one register mirror. Both sides compute (y1-y3)*(x3-x2) in EDX and
   (x1-x3)*(y3-y2) in EAX and return their difference with setg; the game saves x1 in ESI at entry
   (`mov esi,eax`) and uses EAX for the x3 - x2 temp, ours keeps x1 in EAX and uses ESI for the
   temp. 4 bytes shorter. Tried: one expression in four operand orders, a, b locals, if/return,
   > between the products, compound assignments on y1 and x1 (this one), a copy t = x1, the two
   halves swapped, longhand instead of compound, `y1 > x1`, temporaries for the two products. */
int left_of(int x1, int y1, int x2, int y2, int x3, int y3)
{
    y1 -= y3;
    y1 *= x3 - x2;
    x1 -= x3;
    x1 *= y3 - y2;
    return y1 - x1 > 0;
}

/* ---- 0x1467c -------------------------------------------------------------------------------- */

int ai_unstick(int racer)
{
    if (bodies[racer].vx == 0 && bodies[racer].vy == 0) {
        if (racer == 2) {
            stuck2++;
            if (stuck2 > 20) {
                stats[bodies[racer].stats].heading = (rand() % 40 + stats[bodies[racer].stats].heading + 8) & 31;
                stuck2 = 0;
                return 1;
            }
        } else if (racer == 4) {
            stuck4++;
            if (stuck4 > 20) {
                stats[bodies[racer].stats].heading = (rand() % 40 + stats[bodies[racer].stats].heading + 8) & 31;
                stuck4 = 0;
                return 1;
            }
        }
    }
    return 0;
}

/* ---- 0x1475f -------------------------------------------------------------------------------- */

void camera_start(int racer)
{
    scroll_x = (short)(bodies[racer].x >> 16) - view_tiles_w * 32 / 2;
    if (scroll_x < 0)
        scroll_x = 0;
    if ((map_w - (view_tiles_w + 1)) * 32 < scroll_x)
        scroll_x = (map_w - (view_tiles_w + 1)) * 32;
    scroll_y = (short)(bodies[racer].y >> 16) - view_tiles_h * 40 / 2;
    if (scroll_y < 0)
        scroll_y = 0;
    if ((map_h - (view_tiles_h + 1)) * 40 < scroll_y)
        scroll_y = (map_h - (view_tiles_h + 1)) * 40;
    tile_x = scroll_x / 32;
    tile_y = scroll_y / 40;
}

/* ---- 0x14861 -------------------------------------------------------------------------------- */

void clear_objects(void)
{
    int i;
    for (i = 0; i < num_objects; i++) {
        bodies[i].f1c = 0;
        objects[i].period = -1;
        objects[i].sprite = i;
        sprites[i].x = sprites[i].y = 4000;
        objects[i].anim = 0;
        objects[i].frame = 0;
    }
}

/* ---- 0x14f3e -------------------------------------------------------------------------------- */

void start_crash(int racer)
{
    switch (racer) {
    case 0:
        stats[bodies[racer].stats].top_speed -= 0x20000;
        objects[racer].callback = (void (*)(void))player_crash_tick;
        objects[racer].frame = 0;
        objects[racer].anim = stats[bodies[racer].stats].heading + 22;
        break;
    case 2:
        stats[bodies[racer].stats].top_speed -= 0x20000;
        objects[racer].callback = (void (*)(void))bull_crash_tick;
        objects[racer].frame = 0;
        objects[racer].anim = (stats[bodies[racer].stats].heading >> 1) + 54;
        break;
    case 4:
        stats[bodies[racer].stats].top_speed -= 0x20000;
        objects[racer].callback = (void (*)(void))muis_crash_tick;
        objects[racer].frame = 0;
        objects[racer].anim = (stats[bodies[racer].stats].heading >> 1) + 70;
        break;
    }
}


/* ---- 0x15084 -------------------------------------------------------------------------------- */

int switch_anims(void)
{
    if (!crash0 && !crash2 && !crash4)
        return 0;
    if (crash0) {
        fn_15e8d();
        if (fn_162d6())
            return -1;
        start_crash(0);
        crash0 = 0;
        fn_1bcb7();
        return 0;
    } else if (crash2) {
        fn_16029();
        if (fn_16472())
            return -1;
        start_crash(2);
        crash2 = 0;
        stats[bodies[2].stats].damage = 100;
        fn_1bcb7();
        return 0;
    } else if (crash4) {
        fn_161c5();
        if (fn_1660e())
            return -1;
        start_crash(4);
        crash4 = 0;
        stats[bodies[4].stats].damage = 100;
        fn_1bcb7();
        return 0;
    }
    return 0;
}


/* ---- 0x15166 -------------------------------------------------------------------------------- */

void fn_15166(void)
{
    hud_count(13);
    hud_count(23);
}


/* ---- 0x1585e race_end ---------------------------------------------------------------------
 * Only after a finished race (quit 0, main 0x111e8): racer 0's stats back
 * into the RRRIO block (fuel fixed point >> 16, negative damage and fuel to
 * 0), the waypoints reached per racer to +0x208..+0x210, race_done +0x1f8 = 1,
 * the finishing order to +0x1fc, then 3/2/1 points per place to
 * rrrio.points[racer >> 1], and rrrio_save (discoveries.md "race end").
 * -------------------------------------------------------------------------------------------- */

void race_end(int quit)
{
    if (!quit) {
        if (stats[bodies[0].stats].fuel < 0)
            rrrio.fuel = 0;
        else
            rrrio.fuel = (short)(stats[bodies[0].stats].fuel >> 16);
        rrrio.boosts = stats[bodies[0].stats].boosts;
        rrrio.f18 = stats[bodies[0].stats].f10;
        rrrio.f1c = stats[bodies[0].stats].f14;
        rrrio.rockets = stats[bodies[0].stats].rockets;
        if (stats[bodies[0].stats].damage < 0)
            rrrio.damage = 0;
        else
            rrrio.damage = stats[bodies[0].stats].damage;
        rrrio.f208[0] = waypoint_index0;
        rrrio.f208[1] = waypoint_index2;
        rrrio.f208[2] = waypoint_index4;
        rrrio.race_done = 1;
        rrrio.start_order[0] = finish_order[0];
        rrrio.start_order[1] = finish_order[1];
        rrrio.start_order[2] = finish_order[2];
        if (finish_order[0] < 0 || finish_order[0] > 4) {
            rc_error("%s(%d) : finishline not okay", "init.c", 603);
        } else {
            rrrio.points[finish_order[0] >> 1] += 3;
        }
        if (finish_order[1] < 0 || finish_order[1] > 4) {
            rc_error("%s(%d) : finishline not okay", "init.c", 608);
        } else {
            rrrio.points[finish_order[1] >> 1] += 2;
        }
        if (finish_order[2] < 0 || finish_order[2] > 4) {
            rc_error("%s(%d) : finishline not okay", "init.c", 613);
        } else {
            rrrio.points[finish_order[2] >> 1] += 1;
        }
        debug_printf("rc:%d b:%d m:%d\n", waypoint_index0, waypoint_index2, waypoint_index4);
        rrrio_save();
    }
}


/* ---- 0x15a14 -------------------------------------------------------------------------------- */

void winner_film(int racer)
{
    mix_stop_track(0);
    switch (racer) {
    case 0:
        smk_play(screen_buf, 0, 0, screen_w, screen_h, "wredcat.smk", 42, 0, 33);
        break;
    case 2:
        smk_play(screen_buf, 0, 0, screen_w, screen_h, "wbrutus.smk", 42, 0, 33);
        break;
    case 4:
        smk_play(screen_buf, 0, 0, screen_w, screen_h, "wsnitch.smk", 42, 0, 33);
        break;
    }
}


/* ---- 0x15a9f podium -------------------------------------------------------------------------
 * The picture of the first two places (r RedCat, b Brutus, s Snitch) with
 * flags 0xf (picture, palette, shown), then the points of the three racers
 * in Ascii20, fg 0xd0 on background 0xff, kept 4 s (discoveries.md "podium").
 * An order the two switches have no case for leaves the race's screen.
 * -------------------------------------------------------------------------------------------- */

void podium(int quit)
{
    if (!quit) {
        switch (finish_order[0]) {
        case 0:
            switch (finish_order[1]) {
            case 2:
                fn_2369f("rbs.pcx", 15);
                break;
            case 4:
                fn_2369f("rsb.pcx", 15);
                break;
            }
            break;
        case 2:
            switch (finish_order[1]) {
            case 0:
                fn_2369f("brs.pcx", 15);
                break;
            case 4:
                fn_2369f("bsr.pcx", 15);
                break;
            }
            break;
        case 4:
            switch (finish_order[1]) {
            case 0:
                fn_2369f("srb.pcx", 15);
                break;
            case 2:
                fn_2369f("sbr.pcx", 15);
                break;
            }
            break;
        }
        fn_246b5(font20_name);
        fn_24942(0xff);
        fn_24879(0xd0);
        debug_printf(" Naam:%s\n", rrrio.names[0]);
        rcascii_printf(150, 20, "Punten Totaal voor %s", world_names[rrrio.world]);
        rcascii_printf(200, 50, "%-15s    :    %3d", rrrio.names[0], rrrio.points[0]);
        rcascii_printf(200, 70, "Brutus de Bulldog  :    %3d", rrrio.points[1]);
        rcascii_printf(200, 90, "Max de Muis        :    %3d", rrrio.points[2]);
        delay_ms(4000);
    }
}


/* ---- 0x1ab7f -------------------------------------------------------------------------------- */

void rrrio_save(void)
{
    FILE *f = fopen("RRRIO.INF", "wb");
    fwrite(&rrrio, 0x228, 1, f);
    fclose(f);
}

/* ---- 0x1abb6 -------------------------------------------------------------------------------- */

int rrrio_load(void)
{
    FILE *f;
    int i;

    f = fopen("RRRIO.INF", "rb");
    if (f != NULL) {
        fread(&rrrio, 0x228, 1, f);
        if (rrrio.version == 3) {
            fclose(f);
            rrrio.race_done = 0;
            return 1;
        }
    }
    rrrio.version = 3;
    rrrio.damage = 100;
    rrrio.boosts = 5;
    rrrio.f10 = 0;
    rrrio.rockets = 10;
    rrrio.f18 = 0;
    rrrio.f1c = 0;
    rrrio.fuel = 75;
    rrrio.stage = 0;
    rrrio.groep = 6;
    rrrio.world = 0;
    rrrio.sound = 3;
    rrrio.joystick = 0;
    rrrio.joy[0] = -1;
    rrrio.joy[1] = -1;
    rrrio.joy[4] = -1;
    rrrio.joy[5] = -1;
    rrrio.joy[2] = -1;
    rrrio.joy[3] = -1;
    rrrio.keys[0] = 0x1c;
    rrrio.keys[1] = 0x36;
    rrrio.keys[2] = 0x4b;
    rrrio.keys[3] = 0x4d;
    rrrio.keys[4] = 0x48;
    rrrio.keys[5] = 0x50;
    for (i = 0; i < 4; i++)
        rrrio.names[i][0] = 0;
    rrrio.race_done = 0;
    rrrio.start_order[0] = 0;
    rrrio.start_order[1] = 2;
    rrrio.start_order[2] = 4;
    rrrio.f208[0] = 0;
    rrrio.f208[1] = 0;
    rrrio.f208[2] = 0;
    rrrio.points[0] = 0;
    rrrio.points[1] = 0;
    rrrio.points[2] = 0;
    rrrio.f220 = 0;
    rrrio.f224 = 0;
    rrrio_save();
    return 0;
}

/* ---- 0x1ad9a -------------------------------------------------------------------------------- */

int next_module(int set, int world, int groep)
{
    int i;

    if (set || world == -1)
        world = rrrio.world;
    if (set || groep == -1)
        groep = rrrio.groep;
    i = rrrio.stage;
    if (!rrrio.race_done && i > 0)
        return i;
    do
        i++;
    while (i <= 4 && module_table[world][i] == 0);
    if (i > 4 || i < 1)
        return -1;
    if (i == 4 && groep == 3)
        i = groep;
    if (set)
        rrrio.stage = i;
    return i;
}


/* ---- 0x1ae20 -------------------------------------------------------------------------------- */

int next_module_set(void)
{
    return next_module(1, -1, -1);
}


/* ---- 0x1ae36 -------------------------------------------------------------------------------- */

int next_module_of(int world, int groep)
{
    return next_module(0, world, groep);
}


/* ---- 0x1ae44 -------------------------------------------------------------------------------- */

int stage_map_index(void)
{
    return stage_table[rrrio.world][rrrio.stage] - 1;
}

/* ---- 0x1ae67 -------------------------------------------------------------------------------- */

/* the spelling matters: a `for (i = stage; i < 5; i++) if (...) break;` is rotated by -ol and
   tests at the bottom; the game tests at the top */
int stage_is_last(void)
{
    int i = rrrio.stage;
    while (i <= 4 && stage_table[rrrio.world][i] == 0)
        i++;
    return i == 4;
}

/* ---- 0x1aea5 -------------------------------------------------------------------------------- */

int fn_1aea5(void)
{
    int i = rrrio.stage;
    while (i > 0 && stage_table[rrrio.world - 1][i] != 0)
        i--;
    return i == rrrio.stage;
}

/* ---- 0x1aedf -------------------------------------------------------------------------------- */

int ai_script_find(int racer, int n)
{
    int i;

    if (racer == 2) {
        for (i = 0; i < 150; i++) {
            if (script2[rrrio.world][i].key == -1)
                break;
            if (n < script2[rrrio.world][i].key)
                break;
            if (n == script2[rrrio.world][i].key)
                return i;
        }
    } else if (racer == 4) {
        for (i = 0; i < 150; i++) {
            if (script4[rrrio.world][i].key == -1)
                break;
            if (n < script4[rrrio.world][i].key)
                break;
            if (n == script4[rrrio.world][i].key)
                return i;
        }
    }
    return -1;
}
