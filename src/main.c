/* main.c: 0x10010-0x11526, the game's first object. The original main.c #included sndman.c (the
   sound functions 0x10010-0x107f6 carry "sndman.c" in their messages, init_game onwards "main.c",
   and the CONST and _DATA run on unaligned across the seam: research/discoveries.md); here both are
   one file. */

#include "rrr.h"

/* ---- _DATA 0x55400-0x55fb3, in address order (Watcom packs it, no alignment) ----------------- */

/* the two ascii sets: 1309 bytes each, the name first; the rest (zero here) is LIKELY filled by
   the font loader fn_24a9a. Their real type is not known. */
char           font12_name[1309] = "ascii12.img";       /* 0x55400 */
char           font20_name[1309] = "ascii20.img";       /* 0x5591d */

/* sndman.c's */
char           movie_dir[30] = "d:\\MOVIE";            /* 0x55e3a */
char          *wav_names[34] = {                        /* 0x55e58 */
    "shot.wav", "explo_m.wav", "ping_m.wav", "to_m.wav", "r_horn_m.wav", "rengine.wav",
    "bengine.wav", "sengine.wav", "crash1.wav", "crash2.wav", "big_expl.wav", "tune.wav",
    "tune_m.wav", "duhh.wav", "kick.wav", "slidcar.wav", "boost.wav", "turbo.wav", "boing.wav",
    "kick.wav", "firerock.wav", "bump2.wav", "putbom.wav", "rcow.wav", "yippee.wav",
    "cacsmash.wav", "r_yiha06.wav", "b_yiha14.wav", "s_yiha01.wav", "rcend.wav", "ti.wav",
    "start.wav", "tu.wav", "tropical.wav"
};
unsigned char  snd_volume = 0xaa;                       /* 0x55ee0 */
int            sound_ready = 0;                         /* 0x55ee1 */
short          sb_ports[8] = { 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0x270, 0x280 };
char          *blaster_ports[8] = { "A210", "A220", "A230", "A240", "A250", "A260", "A270", "A280" };
short          sb_irqs[5] = { 2, 3, 5, 7, 10 };         /* 0x55f15 */
char          *blaster_irqs[5] = { "I2", "I3", "I5", "I7", "I10" };
short          sb_dmas[3] = { 0, 1, 3 };                /* 0x55f33 */
char          *blaster_dmas[3] = { "D0", "D1", "D3" };
short          snd_cfg_b = -1;                          /* 0x55f45 */
short          snd_cfg_c = -1;                          /* 0x55f47 */
short          snd_cfg_d = -1;                          /* 0x55f49 */
char          *dma_opts[3] = { "/DMA:0", "/DMA:1", "/DMA:3" };
char          *port_opts[8] = { "/SBP:210", "/SBP:220", "/SBP:230", "/SBP:240", "/SBP:250",
                                "/SBP:260", "/SBP:270", "/SBP:280" };
char          *irq_opts[5] = { "/IRQ:2", "/IRQ:3", "/IRQ:5", "/IRQ:7", "/IRQ:10" };

/* main.c's */
int            lights_done = 0;                         /* 0x55f8b */
int            shot_no = 0;                             /* 0x55f8f */
int            finish_order[3] = { -1, -1, -1 };        /* 0x55f93 */
int            finish_count = 0;                        /* 0x55f9f */
int            finish_done[3] = { 0, 0, 0 };            /* 0x55fa3 */
int            camera_racer = 0;                        /* 0x55faf */

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
        rc_error("Soundblaster initialisatie fout. Probeer andere instellingen!");
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
    fg_makepcx(0, screen_w - 1, 0, screen_h - 1, name);
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
    set_keys();
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
    if (load_object_anims())
        rc_error("memory overflow initialising anims.");
    mem_images_reset();
    if (load_rc_drive_anims())
        rc_error("memory overflow initialising anims.");
    mem_images_reset();
    debug_printf("RC Drive Anims initialised ok using %ld bytes. \n", anim_bytes);
    if (load_bull_drive_anims())
        rc_error("memory overflow initialising anims.");
    mem_images_reset();
    debug_printf("BULL Drive Anims initialised ok using %ld bytes. \n", anim_bytes);
    if (load_muis_drive_anims())
        rc_error("memory overflow initialising anims.");
    mem_images_reset();
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

/* MATCH 2026-11-26 with two pointer locals: lamps 1, 2 and 5 through f, lamps 3 and 4 through g. One
   local for all five lamps leaves the fourth lamp's product in EAX (imul eax,eax,0xc; mov
   edx,[frames]) where the game has imul edx,eax,0xc; mov eax,[frames] - the 3-row SKIP that ~2400
   one-local spellings did not move (open question 33). Of the 52 ways to share locals among the five
   lamps (.work/probe/sl4), 26 match; which one the original used is not decided by the code. */
void start_lights(void)
{
    struct frame *f, *g;

    f = &frames[animset[0x56].steps[0].frame];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    delay_ms(1500);
    f = &frames[animset[0x56].steps[1].frame];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    snd_play(1, 30, 64);
    delay_ms(1000);
    g = &frames[animset[0x56].steps[2].frame];
    draw_frame(screen_buf, g->img, 226, 50, g->w, g->h);
    snd_play(1, 30, 64);
    delay_ms(1000);
    g = &frames[animset[0x56].steps[3].frame];
    draw_frame(screen_buf, g->img, 226, 50, g->w, g->h);
    snd_play(1, 30, 64);
    delay_ms(1000);
    f = &frames[animset[0x56].steps[4].frame];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    snd_play(1, 32, 64);
    delay_ms(500);
}

/* ---- 0x111e8 -------------------------------------------------------------------------------- */

/* The quit-after-finish fill (0x1138d..0x11481) is written with BOTH tests negated: `if
   (!finished2)` and inside each arm `if (!finished4)`. That is what puts the game's blocks where
   they are: the !finished2 && !finished4 arm falls through (0x113a0 order[1] = 2, jmp 0x11481), its
   inner else (order[2] = 2, jmp the quit = 0 at 0x1148b) sits out of line at 0x11462, and the
   finished2 arm at 0x1146e tests finished4 with `je` straight to the shared order[2] = 4. The two
   order[1] = 2 stores (0x113a0, 0x11477) stay separate - one jumps to the shared store, the other
   falls into it - while every positive-test spelling of the same behaviour merges them (794 bytes,
   11-14 rows). 72 spellings in .work/probe/mn: nested with both test polarities x 3 negation
   spellings, flat else-if arms, and if/else-if chains over the four cases in all 24 orders (best 823
   bytes, 18 rows). */
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
        if (!finished2) {
            if (!finished4) {
                finish_order[1] = 2;
                finish_order[2] = 4;
            } else {
                finish_order[2] = 2;
            }
        } else {
            if (!finished4) {
                finish_order[2] = 4;
            } else {
                finish_order[1] = 2;
                finish_order[2] = 4;
            }
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
