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

/* ---- globals (addresses in rrr_names.txt) --------------------------------------------------- */

extern char           movie_dir[];          /* 0x55e3a: "d:\\MOVIE" */
extern char          *wav_names[34];        /* 0x55e58 */
extern char          *blaster_ports[6];     /* 0x55ef5: "A210".."A260" */
extern char          *blaster_irqs[5];      /* 0x55f1f: "I2".."I10" */
extern char          *blaster_dmas[3];      /* 0x55f39: "D0", "D1", "D3" */
extern short          snd_cfg_b;            /* 0x55f45 */
extern short          snd_cfg_c;            /* 0x55f47 */
extern short          snd_cfg_d;            /* 0x55f49 */
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
