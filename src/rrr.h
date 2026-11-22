/*
 * RRR.EXE's own C: the types, globals and prototypes every module of src/ shares.
 *
 * Names, types and struct layouts are this project's, with their evidence in
 * reverse-engineering/analysis/rrr_names.txt; the original's are not known.
 */
#ifndef RRR_H
#define RRR_H

#include <stdio.h>
#include <math.h>
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
    short x;
    short y;
    unsigned char *img;         /* +4 */
    short mode;                 /* +8: 1, 2 opaque; 4, 5, 6 translucent */
    short w;                    /* +0xa */
    short h;                    /* +0xc */
    short fe;
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
int trans_save(char *name);
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
int init_anims(void);
int fn_15c66(void);
int fn_15e02(void);
int fn_15f9e(void);
int fn_1613a(void);
void fn_1bcb7(void);
#pragma aux fn_1bcb7 modify [eax ebx ecx edx];
void cos_sin_init(void);
void dir_tbl_init(void);
void turn_tbl_init(void);
void merge_tbl_init(void);
void clear_objects(void);
void init_objects(void);
void camera_start(int racer);
int ai_unstick(int racer);
void find_bonus_objects(void);
int fn_1aea5(void);
int key_pressed(int scancode);
#pragma aux key_pressed parm routine [] modify [eax ecx edx];
void mix_stop_track(int track);
void fn_15166(void);
int switch_anims(void);
void objects_tick(void);
void camera_follow(int racer);

void joystick_to_keys(void);
void hud_update(void);
void draw_view(short dy, short dx);
#pragma aux draw_view parm routine [] modify [eax ebx ecx edx];
void race_end(int quit);
void user_exit(int code);
#pragma aux user_exit parm routine [];
void high_score(char *name, int points);
void winner_film(int racer);
void podium(int quit);
void draw_tile(int tile, int x, int y);                       /* 0x1fa94 */
void __cdecl blit_sprite(void *dst, unsigned char *src, int x, int y, int w, int h, int skip, int pitch);
void __cdecl blit_sprite_trans(void *dst, unsigned char *src, unsigned char *trans, int x, int y, int w, int h, int skip, int pitch);
void __cdecl present_view(int x0, int x1, int y0, int y1, int sx, int sy);
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
extern int            tile_y, tile_x, scroll_y, scroll_x;
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
struct order {                              /* 16 bytes: the draw-order list, 306 elements */
    int     key;                                /* +0: sort key, larger first */
    int     sprite;                             /* +4: -1 free, -2 taken but not linked */
    int     next;                               /* +8: -1 = end */
    int     prev;                               /* +0xc */
};
extern unsigned char *map;                  /* 0x5fec4: 6 bytes per cell, +0 tile */
extern struct order   order[306];           /* 0x6fc6c */
extern int            order_head;           /* 0x70f8c */
extern unsigned char *trans;                /* 0x70f90 */
extern unsigned       map_w;                /* 0x7f944: map size in cells */
extern unsigned       map_h;                /* 0x7f948 */
extern short          num_sprites;          /* 0x7f964 */
extern int            turn_tbl[32][32];     /* 0x71de0: [target][current] -> next heading */
extern int            merge_tbl[32][32];    /* 0x72de0 */
extern int            cos_tbl[32];          /* 0x73de0 */
extern int            sin_tbl[32];          /* 0x73e60 */
extern int            dir_tbl[33][33];      /* 0x73ee0: heading by (dy, dx) + 16 */
struct stats {                              /* 48 bytes per racer (discoveries.md) */
    float   mass;
    int     heading;                            /* +4: 0..31 */
    int     fuel, boosts;
    int     f10, f14, f18;
    int     rockets;                            /* +0x1c */
    int     f20, f24;
    int     damage;                             /* +0x28 */
    int     top_speed;                          /* +0x2c */
};
extern struct stats   stats[];              /* 0x6fbc0 */
extern int            stuck2, stuck4;       /* 0x56070, 0x56074 */
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

/* new2io.c */
/* draw_view: above, with its pragma */

/* order.c */
int order_clear(void);
int order_alloc(void);
int order_insert_before(int e, int n);
int order_add(int sprite, int key);
int order_has(int sprite);
int sort_draw_order(void);

/* shade.c */
extern unsigned char  palette[256][3];      /* 0x7fd38 */
unsigned char nearest_colour(unsigned char r, unsigned char g, unsigned char b);


/* raceanim.c and the animation library it calls */
extern int            anims_inited;         /* 0x57a58 */
extern char           anim_list[];          /* 0x57798 */
extern char           anim_buf[];           /* 0x7552c */
extern char           anim_img_path[];      /* 0x750ac */
extern char           anim_ind_path[];      /* 0x74fe4 */
extern short          anim_err;             /* 0x600d4 */
extern char           anim_set_57620[];
extern char           anim_set_57670[];
extern char           anim_set_5767c[];
extern char           anim_set_57684[];
extern char           anim_set_5768c[];
extern char           anim_set_57710[];
extern char           anim_set_57754[];
short fn_25a9d(void *list);
short fn_26782(int path, int n);
short fn_2643f(int img, int ind, void *buf);
void fn_2670e(void *buf);
void fn_1bcb7(void);
#pragma aux fn_1bcb7 modify [eax ebx ecx edx];
void fn_1bd71(void *set, void *list);
void fn_1bda1(void *set, void *list);
#pragma aux fn_1bd71 parm routine [] modify [eax ecx edx];
#pragma aux fn_1bda1 parm routine [] modify [eax ecx edx];
int fn_15c66(void);
int fn_15cf1(void);
int fn_15d34(void);
int fn_15dbf(void);
int fn_15e02(void);
int fn_15e8d(void);
int fn_15ed0(void);
int fn_15f5b(void);
int fn_15f9e(void);
int fn_16029(void);
int fn_1606c(void);
int fn_160f7(void);
int fn_1613a(void);
int fn_161c5(void);
int fn_16208(void);
int fn_16293(void);
int fn_162d6(void);
int fn_16361(void);
int fn_163a4(void);
int fn_1642f(void);
int fn_16472(void);
int fn_164fd(void);
int fn_16540(void);
int fn_165cb(void);
int fn_1660e(void);
int fn_16699(void);
int fn_166dc(void);
int fn_16767(void);

/* rrr.c, still unsplit */
void fn_19391(int a);
void fn_15166(void);
int free_object_tick(short n);
int fn_18886(int v, int lo, int hi);
int fn_1aea5(void);
int check_object_nr(int n, char *file, int line);
int check_waypoint_nr(int n, char *file, int line);
#endif
