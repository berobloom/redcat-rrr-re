/* The race (RRR.EXE): racers, map cells, objects, camera.  No SDL: the
   caller feeds key states and draws the object list.

   Everything is per frame, as in the original, whose loop ran as fast as
   the PC allowed (research/discoveries.md "The race: speed").  Addresses
   in the comments are RRR.EXE's (LE image). */
#ifndef RC_RACE_H
#define RC_RACE_H

#include <stdint.h>

#include "gfx8.h"

#define RC_RACERS 3
#define RC_OBJECTS 306          /* 0..5 racers and their shadows */
#define RC_MAX_WAYPOINTS 150

/* View rectangle on the 640x480 screen (inclusive bounds 20..620, 0..400). */
#define RC_VIEW_X 20
#define RC_VIEW_Y 0
#define RC_VIEW_W 601
#define RC_VIEW_H 401

enum { RC_REDCAT, RC_BULL, RC_MUIS };   /* racer index: objects 0, 2, 4 */

/* 0x6cbf0 + 0x28 n, 16.16 fixed point; z is height above the ground. */
typedef struct {
    int32_t x, y, z, vx, vy, vz, speed;
    int32_t prio;               /* subtracted from the draw order key */
    int stats;                  /* racer index, -1 for other objects */
    int32_t life;
} rc_body;

/* 0x6fbc0 + 0x30 k */
typedef struct {
    int32_t mass, heading, fuel, boosts, f10, f14, f18, rockets, drops;
    int32_t boost_state, damage, top;
} rc_stats;

typedef struct rc_race rc_race;

typedef struct {
    short period;               /* -1 free; the callback runs every period frames */
    short sx, sy;               /* sprite top-left in map pixels */
    short anim, frame;
    short kind;                 /* sprite kind of the shown frame */
    short image;                /* RC.IMG entry + 1 of the shown frame */
    void (*tick)(rc_race *r, int obj);
} rc_object;

/* Sprite hotspots, from RC.IMG (image i = entry i - 1): +8, +10 of the
   run-time image record. */
typedef struct {
    int16_t hx, hy;
} rc_hotspot;

typedef struct {
    int up, down, left, right, boost, rocket;
} rc_input;

struct rc_race {
    rc_map map;
    const rc_hotspot *hotspots; /* indexed by image number, 1..1056 */
    int map_index;

    rc_body body[RC_OBJECTS];
    rc_stats stats[RC_RACERS];
    rc_object obj[RC_OBJECTS];
    int free_objects;

    int32_t wp[RC_RACERS][RC_MAX_WAYPOINTS][2];   /* 16.16 */
    int wp_count[RC_RACERS], wp_next[RC_RACERS];

    int started;                /* [0x55f8b]: after the start lights */
    int finished[RC_RACERS];    /* 0x55fbc, 0x55ff0, 0x56014 */
    int out[RC_RACERS];         /* fuel or damage < 0: 0x56078, 0x56080, 0x5607c */
    int wrecked[RC_RACERS];     /* crash callbacks installed by 0x14f3e */
    int order[RC_RACERS], order_n, order_done[RC_RACERS];

    rc_input in;                /* this frame's keys */
    int boost_latch, rocket_latch, contact_latch[2];
    int exhaust_count[RC_RACERS];
    int stuck_count[RC_RACERS];
    int ai_accel[RC_RACERS];

    int camera_racer;           /* [0x55faf] / 2 */
    int scroll_x, scroll_y;     /* [0x7f984], [0x7f980] */
    uint32_t frame_count;       /* [0x600c4] */
    uint32_t rng;
};

/* Start order: start_order[slot] = racer standing on waypoint 0 of slot's
   list (RRRIO.INF 0x1fc, default 0, 2, 4 as object numbers / 2). */
int rc_race_init(rc_race *r, const uint8_t *map_data, uint32_t map_size,
                 int map_index, const rc_hotspot *hotspots,
                 const int start_order[RC_RACERS]);
void rc_race_free(rc_race *r);

/* The player's counters from RRRIO.INF (race_init reads fuel +0x20, damage
   +0x04, boosts +0x0c, rockets +0x14); rc_race_init uses the defaults. */
void rc_race_set_player(rc_race *r, int fuel, int repair, int boosts,
                        int rockets);

/* One frame of race_main's loop. */
void rc_race_tick(rc_race *r, const rc_input *in);

/* Draw order (0x11b0e): object numbers back to front.  Returns the count. */
int rc_race_draw_order(const rc_race *r, int *out);

/* finish_check (0x10eff): 1 once the finishing order is known. */
int rc_race_over(rc_race *r);

/* Tables built at start-up in the original (discoveries "driving"). */
extern int32_t rc_cos[32], rc_sin[32];
extern uint8_t rc_turn[32][32], rc_merge[32][32], rc_dir[33][33];
void rc_tables_init(void);

#endif
