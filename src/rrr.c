/*
 * RRR.EXE, the race: the game's own C, rebuilt function by function.
 *
 * Compiled by src/check.sh with Watcom C/C++ 10.5, `wcc386 -bt=dos -3r -s -ox` (MATCHING.md), and compared
 * with RRR.EXE function by function. Functions are in address order. The original was several
 * modules (sndman.c, main.c, new2io.c, order.c, shade.c, init.c, raceanim.c, heap.c, map.c, ...;
 * research/discoveries.md); this file is one module until their boundaries are known, so each
 * function is compared on its own. Names, types and struct layouts are this project's, with their
 * evidence in reverse-engineering/analysis/rrr_names.txt; the original's are not known.
 */
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <dos.h>

/* ---- types ---------------------------------------------------------------------------------- */

/* RRRIO.INF: RRR's globals 0x75178..0x753a0, dumped to and read from disk as one block
   (reverse-engineering/file-formats.md "RRRIO.INF") */
struct rrrio {
    int  version;               /* 0x00: 3; anything else and rrrio_load writes the defaults */
    int  damage;                /* 0x04: 100 */
    int  f08;                   /* 0x08: not set by the defaults */
    int  boosts;                /* 0x0c: 5 */
    int  f10;
    int  rockets;               /* 0x14: 10 */
    int  f18, f1c;
    int  fuel;                  /* 0x20: 75 */
    int  stage;                 /* 0x24: map column */
    int  groep;                 /* 0x28: 3..8 */
    int  world;                 /* 0x2c: 0..5, the row of the map table */
    int  sound;                 /* 0x30: bit 0 effects, bit 1 music */
    int  joystick;              /* 0x34 */
    int  joy[6];                /* 0x38: -1 */
    int  keys[6];               /* 0x50: Enter, right Shift, left, right, up, down */
    char names[4][100];         /* 0x68: the pupil's name first */
    int  race_done;             /* 0x1f8 */
    int  start_order[3];        /* 0x1fc: 0, 2, 4 */
    int  f208[3];
    int  points[3];             /* 0x214 */
    int  f220, f224;
};

/* an object slot, 16 bytes, 306 of them at [objects] (research/discoveries.md, objects_tick) */
struct object {
    short period;               /* +0: tick period, -1 = free */
    short sprite;               /* +2: sprite slot */
    short x, y;                 /* +4, +6 */
    short anim;                 /* +8 */
    short frame;                /* +10 */
    void (*callback)(void);     /* +12 */
};

/* a sprite slot, 16 bytes, at [sprites] */
struct sprite {
    short f0;
    short f2;
    short f4, f6;
    short mode;                 /* +8 */
    short fa, fc, fe;
};

/* the 40-byte movement record per object at 0x6cbf0 (16.16 fixed point) */
struct body {
    int x, y, z;                /* +0x00 */
    int vx, vy, vz;             /* +0x0c */
    int speed;                  /* +0x18 */
    int f1c;                    /* +0x1c */
    int stats;                  /* +0x20 */
    int f24;
};

/* ---- functions not rebuilt yet --------------------------------------------------------------- */

void fn_261a9(int n);
void rc_error(char *fmt, ...);
void debug_printf(char *fmt, ...);
short has_arg(char *opt, short argc, char **argv);            /* 0x1bc7f, stdlib.c */
int mix_track_free(int track);                                /* 0x1cd3f */
int fn_1c5e4(short port, short irq, short dma);
void fn_1c85b(int stereo);
void fn_1d2a6(int rate, int n, int *handles, char **names);
void fn_1d2ac(int v);
void fn_1d329(int v);
void fn_1d3a6(int v);
void fn_1d4a0(int v);
void sndbuf_play(int track, int sample, int vol);             /* 0x1ec03 */
void fn_1f579(void);
void fn_1f5a0(void);
int easy_init(int a, int n, int *handles, char **names, int last);   /* 0x20b80 */
void __cdecl fn_20d1f(int x0, int x1, int y0, int y1, char *name);
void goodbye(void);
int sound_init(short argc, char **argv);
void fn_1f210(int a);
void cd_check(char *dir, char *name);                         /* 0x21815 */
void fn_1f1fe(int a);
void fn_1f204(int a);
void fn_1f222(int h, int w);
#pragma aux fn_1f222 parm routine [] modify [eax ecx edx];
void fn_21af9(void);
int fn_218e4(void *p, char *name);
void fn_22ab5(int ind, int img);
void fn_22ec9(int ind, int img);
void fn_1f329(int a);
void fn_2369f(char *name, int a);
void fn_1f3dc(void);
short trans_load(char *name);
void trans_generate(void);
void trans_save(char *name);
short fn_24a9a(char *name);
void fn_246b5(char *name);
#pragma aux fn_246b5 parm routine [] modify [eax ecx edx];
void fn_24879(int a);
#pragma aux fn_24879 parm routine [] modify [eax ecx edx];
void fn_24942(int a);
#pragma aux fn_24942 parm routine [] modify [eax ecx edx];
int rrrio_load(void);
void fn_1b469(void);
short fn_1f83f(int n);
short fn_25bbd(int n);
int stage_map_index(void);
short load_map(char *name);
short load_xvga_map_images(char *name);
int find_waypoints(void);
void init_anims(void);
int fn_15c66(void);
int fn_15e02(void);
int fn_15f9e(void);
int fn_1613a(void);
void fn_1bcb7(void);
void fn_13c34(void);
void fn_13cba(void);
void fn_13f00(void);
void fn_13f91(void);
void clear_objects(void);
void init_objects(void);
void camera_start(int a);
void find_bonus_objects(void);
int fn_1aea5(void);
int key_pressed(int scancode);
#pragma aux key_pressed parm routine [] modify [eax ecx edx];
void mix_stop_track(int track);
void fn_15166(void);
int switch_anims(void);
void objects_tick(void);
void camera_follow(int racer);
void sort_draw_order(void);
void joystick_to_keys(void);
void hud_update(void);
void draw_view(short dy, short dx);
#pragma aux draw_view parm routine [];
void race_end(int quit);
void user_exit(int code);
#pragma aux user_exit parm routine [];
void high_score(char *name, int points);
void winner_film(int racer);
void podium(int quit);
void draw_frame(void *dst, void *img, int x, int y, int w, int h);  /* 0x2425d */
void delay_ms(int ms);
#pragma aux delay_ms parm routine [] modify [eax ebx edx];   /* its own code: ECX kept */

/* ---- globals (addresses in rrr_names.txt) --------------------------------------------------- */

extern char           movie_dir[];          /* 0x55e3a: "d:\\MOVIE" */
extern char          *wav_names[34];        /* 0x55e58 */
extern char          *blaster_ports[6];     /* 0x55ef5: "A210".."A260" */
extern char          *blaster_irqs[5];      /* 0x55f1f: "I2".."I10" */
extern char          *blaster_dmas[3];      /* 0x55f39: "D0", "D1", "D3" */
extern short          snd_cfg_b;            /* 0x55f45 */
extern short          snd_cfg_c;            /* 0x55f47 */
extern short          snd_cfg_d;            /* 0x55f49 */
extern unsigned char  snd_volume;           /* 0x55ee0 */
extern int            sound_ready;          /* 0x55ee1 */
extern short          sb_ports[8];          /* 0x55ee5: 0x210..0x280 */
extern short          sb_irqs[5];           /* 0x55f15: 2, 3, 5, 7, 10 */
extern short          sb_dmas[3];           /* 0x55f33: 0, 1, 3 */
extern char          *dma_opts[3];          /* 0x55f4b: "/DMA:0" .. */
extern char          *port_opts[8];         /* 0x55f57: "/SBP:210" .. */
extern char          *irq_opts[5];          /* 0x55f77: "/IRQ:2" .. */
extern int            shot_no;              /* 0x55f8f */
extern int            num_tracks;           /* 0x5fe67 */
extern int            tracks[4];            /* 0x6c970: sample per track, -1 = none */
extern short          screen_w;             /* 0x7f95e */
extern short          screen_h;             /* 0x7f962 */
extern char           font12_name[];        /* 0x55400: "ascii12.img" */
extern char           font20_name[];        /* 0x5591d: "ascii20.img" */
extern char          *track_maps[];         /* 0x57a60: "track1a.map" .. */
extern char          *track_imgs[];         /* 0x57abc: "track1.img" .. */
extern int            view_size;
extern short          v5feba;
extern int            debug_on;             /* 0x5ff3c */
extern int            view_tiles_w;
extern int            view_tiles_h;
extern long           anim_bytes;           /* 0x600d6 */
extern char           v6cad0[];
extern char           v6cb60[];
extern int            stereo;               /* 0x6fc50 */
extern int            sound_enabled;        /* 0x6fc54 */
extern int            music_on;             /* 0x6fc58 */
extern int            map_no;               /* 0x6fc5c */
extern int            v7f978, v7f97c, scroll_y, scroll_x;
struct frame {                              /* 12 bytes: an anim frame image */
    void   *img;
    short   w;                                  /* +4 */
    short   h;                                  /* +6 */
    short   f8, fa;
};
struct animset {
    char    pad[0x2b4];
    short  *lamp;                               /* +0x2b4: start-light frames, [1] [3] .. [9] */
};
extern int            lights_done;          /* 0x55f8b */
extern int            finish_order[3];      /* 0x55f93 */
extern int            finish_count;         /* 0x55f9f */
extern int            finish_done[3];       /* 0x55fa3 */
extern int            camera_racer;         /* 0x55faf */
extern int            finished0;            /* 0x55fbc: racer 0's finished flag */
extern int            finished2;            /* 0x55ff0 */
extern int            finished4;            /* 0x56014 */
extern struct frame  *frames;               /* 0x600d0 */
extern void          *screen_buf;           /* 0x7f954 */
extern struct animset *animset;             /* 0x804bc */
extern int            stage_table[][5];     /* 0x57b8c: map per [world][stage], 0 = no race */
extern int            wav_handles[34];      /* 0x6c980 */
extern char           strbuf[];             /* 0x6ca08 */
extern short          snd_cfg_f;            /* 0x6fc60 */
extern short          snd_cfg_e;            /* 0x6fc62 */
extern short          snd_cfg_g;            /* 0x6fc64 */
extern short          snd_cfg_a;            /* 0x6fc66 */
extern struct sprite *sprites;              /* 0x5fec0 */
extern int            free_objects;         /* 0x57a5c */
extern short          num_objects;          /* 0x600c8 */
extern struct body    bodies[];             /* 0x6cbf0 */
extern struct rrrio   rrrio;                /* 0x75178 */
extern struct object *objects;              /* 0x804b8 */

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

/* differs: 2 rows, the game copies dist into EBX before the sound_on call, ours after it (tried:
   initialiser, statement before the test, body under if (sound_on()), unprototyped sound_on, a
   second copy local, ?: in the divisor) */
int play_fx(int sample, int dist)
{
    int vol, t;
    if (!sound_on())
        return 0;
    vol = dist;
    if (dist < 1)
        vol = 1;
    vol = 1500 / vol * 64;
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

/* differs: 13 rows, all at the /STEREO argument: the game branches (je; mov eax,1; jmp; xor eax,eax)
   where ? 1 : 0 gives setne, and keeps EDX/EBX/ECX from fn_1d2a6's call for easy_init. Two calls in
   an if/else get the branches (cross-jumped) but then reload the three registers (10 rows). Tried:
   if/else into a local (braced and not), = 0 then if, !x ? 0 : 1, == 0, != 0, (int) cast, short and
   char parameter aliases. Matched on the way: the two early returns as separate ifs, the tracks
   fill as a loop (memset gives other registers), declaration order dma, port, irq. */
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
    fn_1c85b(has_arg("/STEREO", argc, argv) ? 1 : 0);
    fn_1d2a6(11025, 34, wav_handles, wav_names);
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
    v7f978 = 0;
    v7f97c = 0;
    debug_printf("Init direction tables.\n");
    fn_13c34();
    fn_13cba();
    fn_13f00();
    fn_13f91();
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

    f = &frames[animset->lamp[1]];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    delay_ms(1500);
    f = &frames[animset->lamp[3]];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    snd_play(1, 30, 64);
    delay_ms(1000);
    f = &frames[animset->lamp[5]];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    snd_play(1, 30, 64);
    delay_ms(1000);
    f = &frames[animset->lamp[7]];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    snd_play(1, 30, 64);
    delay_ms(1000);
    f = &frames[animset->lamp[9]];
    draw_frame(screen_buf, f->img, 226, 50, f->w, f->h);
    snd_play(1, 32, 64);
    delay_ms(500);
}

/* ---- 0x111e8 -------------------------------------------------------------------------------- */

/* differs: 6 rows, block placement in the quit-after-finish fill (0x1138d..0x11481): the game tests
   finished2 with jne to the finished2 branch and keeps the finished4 -> order[2] = 2 block before
   it, ours tests with je and places them the other way round; same size, same stores. 22 spellings
   of the block tried (nested if/else both ways round, else-if chains on the four cases, && forms,
   a goto, separate ifs); the others merge the two order[1] = 2 stores (794 bytes) or grow. */
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

/* ---- 0x14861 -------------------------------------------------------------------------------- */

void clear_objects(void)
{
    int i;
    for (i = 0; i < num_objects; i++) {
        bodies[i].f1c = 0;
        objects[i].period = -1;
        objects[i].sprite = i;
        sprites[i].f0 = sprites[i].f2 = 4000;
        objects[i].anim = 0;
        objects[i].frame = 0;
    }
}

/* ---- 0x17e0b -------------------------------------------------------------------------------- */

int alloc_object(void)
{
    int i;
    if (free_objects != 0) {
        for (i = 6; i < 306; i++) {
            if (objects[i].period == -1) {
                free_objects--;
                return i;
            }
        }
    }
    return -1;
}

/* ---- 0x17e4d -------------------------------------------------------------------------------- */

void free_object(int n)
{
    if (n >= 6 && n <= 306) {
        fn_261a9(n);
        objects[n].frame = 0;
        objects[n].period = -1;
        free_objects++;
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
