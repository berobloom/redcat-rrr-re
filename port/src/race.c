/* The race, after RRR.EXE's player_tick (0x11eb5), bull_tick (0x12826),
   muis_tick (0x130f8) and their helpers.  research/discoveries.md has the
   evidence; comments here name the original's addresses.

   Not yet ported: rockets (0x185d4), dropped objects (0x18759), exhaust and
   explosion objects (0x17e87, 0x17faa), over-speed (0x180d3), sounds. */
#include "race.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "anims.h"

#define FX(n) ((int32_t)((n) * 65536))
#define TILE_W 32
#define TILE_H 40
#define VIEW_COLS 19
#define VIEW_ROWS 10

int32_t rc_cos[32], rc_sin[32];
uint8_t rc_turn[32][32], rc_merge[32][32], rc_dir[33][33];

/* 0x56030: [up/none/down][left/none/right] -> heading, -1 none */
static const int steer_dir[3][3] = {
    { 4, 0, 28 }, { 8, -1, 24 }, { 12, 16, 20 },
};

/* init_objects 0x148db: RedCat drives anim 2, Brutus 0, Snitch 1 */
static const int racer_anim[RC_RACERS] = { 2, 0, 1 };

/* 0x57b18: computer racers' top speed per map, px/frame */
static const int ai_top_speed[RC_NUM_MAPS] = {
    13, 13, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
};

static int32_t fxmul(int32_t a, int32_t b)
{
    return (int32_t)(((int64_t)a * b) >> 16);
}

static int32_t iabs(int32_t v)
{
    return v < 0 ? -v : v;
}

/* Watcom C rand(): LCG, 15 bits. */
static int rc_rand(rc_race *r)
{
    r->rng = r->rng * 1103515245u + 12345u;
    return (int)((r->rng >> 16) & 0x7FFF);
}

static int step_toward(int target, int cur, int away)
{
    int d = target - cur, plus;

    if (d == 0)
        return cur;
    plus = (d >= 1 && d <= 16) || d <= -17;
    if (away)
        plus = !plus;
    return (cur + (plus ? 1 : 31)) & 31;
}

void rc_tables_init(void)
{
    int h, t, c;

    for (h = 0; h < 32; h++) {          /* 0x13c34 */
        double a = (h + 8) * -0.196349540875;
        rc_cos[h] = (int32_t)(cos(a) * 65536.0);
        rc_sin[h] = (int32_t)(sin(a) * 65536.0);
    }
    for (t = 0; t < 32; t++)            /* 0x13f00, 0x13f91 */
        for (c = 0; c < 32; c++) {
            rc_turn[t][c] = (uint8_t)step_toward(t, c, 0);
            rc_merge[t][c] = (uint8_t)step_toward(t, c, 1);
        }
    for (t = 0; t < 33; t++)            /* 0x13cba */
        for (c = 0; c < 33; c++) {
            int k = (int)(atan2(c - 16.0, t - 16.0) * 5.092958178275649 + 16.0
                          + 0.5);
            rc_dir[t][c] = (uint8_t)((56 - k) & 31);
        }
}

static rc_cell *cell_at(rc_race *r, int cx, int cy)
{
    static rc_cell outside;

    if (cx < 0 || cy < 0 || cx >= r->map.w || cy >= r->map.h) {
        memset(&outside, 0, sizeof outside);
        return &outside;
    }
    return &r->map.cells[cy * r->map.w + cx];
}

/* Watcom's (x >> 16) / 32 for signed x, as the sar/sbb sequence does. */
static int px_to_cx(int32_t x)
{
    return (x >> 16) / TILE_W;
}

static int px_to_cy(int32_t y)
{
    return (y >> 16) / TILE_H;
}

/* direction_to_waypoint (0x13d60) */
static int direction_to(const rc_body *b, int32_t wx, int32_t wy)
{
    float dx = (float)b->x * (1.0f / 65536) - (float)wx * (1.0f / 65536);
    float dy = (float)b->y * (1.0f / 65536) - (float)wy * (1.0f / 65536);
    float d = sqrtf(dy * dy + dx * dx);
    int i, j;

    if (d == 0.0f)
        return 0;
    i = (int)(((double)dx / d + 1.0) * 16.0) % 33;
    j = (int)(((double)dy / d + 1.0) * 16.0) % 33;
    return rc_dir[i][j];
}

static const rc_frame *obj_frame(const rc_object *o)
{
    return &rc_anims[o->anim].frames[o->frame];
}

/* Sprite top-left from the body: the code repeated at the end of every
   callback (x - hotspot_x - 16, y - z - hotspot_y). */
static void place_sprite(rc_race *r, int n)
{
    rc_object *o = &r->obj[n];
    const rc_body *b = &r->body[n];
    const rc_hotspot *h = &r->hotspots[obj_frame(o)->image];

    o->sx = (short)((b->x >> 16) - h->hx - 16);
    o->sy = (short)((b->y >> 16) - (b->z >> 16) - h->hy);
}

static void free_object(rc_race *r, int n)    /* 0x17e4d */
{
    if (n > 5 && n < RC_OBJECTS) {
        r->obj[n].frame = 0;
        r->obj[n].period = -1;
        r->free_objects++;
    }
}

static int alloc_object(rc_race *r)            /* 0x17e0b */
{
    int n;

    if (r->free_objects == 0)
        return -1;
    for (n = 6; n < RC_OBJECTS; n++)
        if (r->obj[n].period == -1) {
            r->free_objects--;
            return n;
        }
    return -1;
}

/* ground_check (0x1889d): walls are cells whose level*1000 is above z.
   Returns the level the landing test uses. */
static int ground_check(rc_race *r, int n)
{
    rc_body *b = &r->body[n];
    rc_stats *s = &r->stats[b->stats];
    int nx = px_to_cx(b->x + b->vx), ny = px_to_cy(b->y + b->vy);
    int ox = px_to_cx(b->x), oy = px_to_cy(b->y);
    int lv = cell_at(r, nx, ny)->level, side;
    /* The original tests the player's left/right keys for every racer. */
    int steering = r->in.left || r->in.right;

    if (b->z >= lv * FX(1000))
        return lv;
    lv = cell_at(r, ox, ny)->level;
    if (b->z < lv * FX(1000)) {
        lv = cell_at(r, nx, oy)->level;
        if (b->z < lv * FX(1000)) {
            lv = cell_at(r, ox, oy)->level;
            b->vx = b->vy = 0;
            return lv;
        }
        side = b->vx < -FX(3) ? -1 : b->vx > FX(3) ? 1 : 0;    /* 0x18886 */
        if (!steering && side)
            s->heading = rc_turn[side < 0 ? 8 : 24][s->heading];
        b->vy = 0;
        return lv;
    }
    side = b->vy < -FX(3) ? -1 : b->vy > FX(3) ? 1 : 0;
    if (!steering && side)
        s->heading = rc_turn[side < 0 ? 0 : 16][s->heading];
    b->vx = 0;
    return lv;
}

/* cell_effect (0x18b8d) for racer object n at cell (cx, cy). */
static void cell_effect(rc_race *r, int n, int cx, int cy, int chance)
{
    rc_body *b = &r->body[n];
    rc_stats *s = &r->stats[b->stats];
    rc_cell *c = cell_at(r, cx, cy);
    int t = c->type;

    if (t > 21)
        return;
    if (t == 11) {
        if (s->top <= FX(12))
            s->top += FX(3);
    } else if (s->top > FX(12)) {
        s->top -= FX(3);
    }
    switch (t) {
    case 12:                    /* mine: -10 damage, a bump up */
        if (b->z >= FX(20) || r->body[c->object].life > 0)
            return;
        free_object(r, c->object);
        s->damage -= 10;
        c->type = 0;
        b->vz += fxmul(b->speed, 0x1999);
        return;
    case 13:                    /* boost pickup */
        if (b->z >= FX(20))
            return;
        free_object(r, c->object);
        s->boosts++;
        c->type = 0;
        return;
    case 14:                    /* ramp */
        if (b->z >= FX(20) || b->speed <= FX(4))
            return;
        b->vz += fxmul(b->speed, 0x1999);
        (void)chance;           /* only picks a sound */
        return;
    case 15:
    case 16:                    /* cactus */
        if (s->damage < 1 || s->fuel < 1) {
            s->damage -= 2;
            return;
        }
        if (b->z > 0x1dffff)
            return;
        r->body[c->object].life = 5;
        s->damage -= 1;
        c->type = 0;
        return;
    case 20:                    /* finish line */
        r->finished[b->stats] = 1;
        return;
    }
}

static void cell_checks(rc_race *r, int n)     /* 0x18ae5 */
{
    const rc_body *b = &r->body[n];
    int chance = rc_rand(r) % 100;
    int cx = px_to_cx(b->x), cy = px_to_cy(b->y);

    cell_effect(r, n, cx, cy, chance);
    cell_effect(r, n, cx + 1, cy, chance);
    cell_effect(r, n, cx, cy + 1, chance);
    cell_effect(r, n, cx - 1, cy, chance);
    cell_effect(r, n, cx, cy - 1, chance);
}

/* racer_contact (0x1404a): is racer a inside a 100 px square around b,
   turned to b's heading, and within 60 height units?  The original tests
   the four sides with cross products (0x1401d). */
static int racer_contact(rc_race *r, int a, int bn)
{
    const rc_body *A = &r->body[a], *B = &r->body[bn];
    int h = r->stats[B->stats].heading, hs = (h + 40) & 31;
    int32_t dx, dy, along, across;

    if (iabs(A->z - B->z) > FX(60))
        return 0;
    if (iabs((A->x >> 16) - (B->x >> 16)) + iabs((A->y >> 16) - (B->y >> 16))
        > 300)
        return 0;
    dx = (A->x - B->x) >> 16;
    dy = (A->y - B->y) >> 16;
    along = (int32_t)(((int64_t)dx * rc_cos[h] + (int64_t)dy * rc_sin[h]) >> 16);
    across = (int32_t)(((int64_t)dx * rc_cos[hs] + (int64_t)dy * rc_sin[hs]) >> 16);
    return iabs(along) < 50 && iabs(across) < 50;
}

static void racer_bounce(rc_race *r, int a, int bn)    /* 0x14534 */
{
    rc_stats *sa = &r->stats[r->body[a].stats];
    rc_stats *sb = &r->stats[r->body[bn].stats];
    int ha = sa->heading, hb = sb->heading;
    int na = rc_merge[ha][hb], nb = rc_merge[hb][ha];
    int32_t avg;

    if (iabs(ha - na) < 9)
        sa->heading = na;
    else
        r->body[a].speed = 0;
    if (iabs(hb - nb) < 9)
        sb->heading = nb;
    else
        r->body[bn].speed = 0;
    avg = (r->body[a].speed + r->body[bn].speed) >> 1;
    r->body[a].speed = avg;
    r->body[bn].speed = avg;
}

/* Gravity, landing and bounce, shared by the racers. */
static void fall(rc_body *b, int32_t ground)
{
    b->z += b->vz;
    if (b->vz > -FX(20))
        b->vz -= fxmul(iabs(b->vz), 0x4000) + 0x4000;
    if (b->z < ground) {
        b->vx -= fxmul(b->vx, 0x2666);
        b->vy -= fxmul(b->vy, 0x2666);
        b->z = 0;
        if (iabs(b->vz) <= 0x10000)
            b->vz = 0;
        else
            b->vz = -fxmul(b->vz, 0x4ccc);
    }
}

static void friction(rc_body *b)
{
    b->speed = fxmul(b->speed, 0xf851);
    if (b->speed < 0x3333)
        b->speed = 0;
}

static void clamp(rc_body *b)
{
    if (b->x > 0x18e00000) b->x = 0x18e00000;
    if (b->x < 0) b->x = 0;
    if (b->y > 0x1f180000) b->y = 0x1f180000;
    if (b->y < 0) b->y = 0;
}

static void clamp_and_pose(rc_race *r, int n)
{
    rc_body *b = &r->body[n];
    rc_object *o = &r->obj[n];
    int pose;

    clamp(b);
    if (b->vz > 0x30000)
        pose = b->vz > 0x40000 ? 0x40 : 0x20;
    else if (b->vz < -0x50000)
        pose = b->vz < -0x70000 ? 0x80 : 0x60;
    else
        pose = 0;
    o->frame = (short)(r->stats[b->stats].heading + pose);
    place_sprite(r, n);
}

static void player_tick(rc_race *r, int n)     /* 0x11eb5 */
{
    rc_body *b = &r->body[n];
    rc_stats *s = &r->stats[b->stats];
    const rc_input *in = &r->in;
    int any = 0, row = 1, col = 1, lv;
    int32_t accel, *wp;

    b->prio = 0;
    if (s->damage < 0 || s->fuel < 0) {
        r->out[RC_REDCAT] = 1;
        return;
    }
    accel = (int32_t)(0x1e00000000LL / s->mass);
    if (r->started)
        any = in->up || in->down || in->left || in->right;
    wp = r->wp[RC_REDCAT][r->wp_next[RC_REDCAT]];
    if (iabs((b->x >> 16) - (wp[0] >> 16)) + iabs((b->y >> 16) - (wp[1] >> 16))
        < 120 && !r->finished[RC_REDCAT]
        && r->wp_next[RC_REDCAT] < r->wp_count[RC_REDCAT] - 1)
        r->wp_next[RC_REDCAT]++;

    if (!r->finished[RC_REDCAT] && s->boost_state == 0) {
        int rocket = r->started && in->rocket, boost = r->started && in->boost;

        if (!rocket)
            r->rocket_latch = 0;
        else if (!r->rocket_latch) {
            r->rocket_latch = 1;
            /* 0x185d4 would fire a rocket here (not ported yet) */
        }
        if (!boost)
            r->boost_latch = 0;
        else if (!r->boost_latch && b->z < FX(40)) {
            r->boost_latch = 1;
            if (s->boosts != 0) {
                b->speed += accel * 16;
                b->vz += accel * 64;
                s->boosts--;
            }
        }
        if (any && b->speed <= s->top) {
            b->speed += accel;
            s->fuel -= 0x666;
            if (++r->exhaust_count[RC_REDCAT] > 3)
                r->exhaust_count[RC_REDCAT] = 0;
        }
        if (r->started) {
            if (in->left) col = 0;
            if (in->right) col = 2;
            if (in->up) row = 0;
            if (in->down) row = 2;
        }
        if (steer_dir[row][col] >= 0) {
            int t = steer_dir[row][col];
            s->heading = rc_turn[t][rc_turn[t][s->heading]];
        }
        if (!any || s->top < b->speed)
            friction(b);
    }
    if (!r->finished[RC_REDCAT]) {
        b->vx = fxmul(b->speed, rc_cos[s->heading]);
        b->vy = fxmul(b->speed, rc_sin[s->heading]);
    } else {
        b->vx = b->vy = 0;
    }
    lv = ground_check(r, n);
    fall(b, lv * FX(1000));
    b->x += b->vx;
    b->y += b->vy;
    if (racer_contact(r, n, 4)) {
        if (!r->contact_latch[0]) {
            racer_bounce(r, n, 4);
            r->contact_latch[0] = 1;
        }
    } else if (racer_contact(r, n, 2)) {
        if (!r->contact_latch[0]) {
            racer_bounce(r, n, 2);
            r->contact_latch[0] = 1;
        }
    } else {
        r->contact_latch[0] = 0;
    }
    cell_checks(r, n);
    clamp_and_pose(r, n);
}

/* player_crash_tick (0x19497): RedCat after damage or fuel ran out drives
   on as a wreck for the rest of the race: anim heading + 22 (8 frames),
   acceleration 65/mass without the out check, boosts that cost nothing,
   friction 0.9 and a stop below 2 px/frame. */
static void player_wreck_tick(rc_race *r, int n)
{
    rc_body *b = &r->body[n];
    rc_stats *s = &r->stats[b->stats];
    rc_object *o = &r->obj[n];
    const rc_input *in = &r->in;
    int any = 0, row = 1, col = 1, lv;
    int32_t accel = (int32_t)(0x4100000000LL / s->mass), *wp;

    b->prio = 0;
    if (r->started)
        any = in->up || in->down || in->left || in->right;
    wp = r->wp[RC_REDCAT][r->wp_next[RC_REDCAT]];
    if (iabs((b->x >> 16) - (wp[0] >> 16)) + iabs((b->y >> 16) - (wp[1] >> 16))
        < 120 && !r->finished[RC_REDCAT]
        && r->wp_next[RC_REDCAT] < r->wp_count[RC_REDCAT] - 1)
        r->wp_next[RC_REDCAT]++;
    if (r->finished[RC_REDCAT]) {
        b->speed = 0;
        o->frame = 2;
    } else if (s->boost_state == 0) {
        int boost = r->started && in->boost;

        if (!(r->started && in->rocket))
            r->rocket_latch = 0;
        else if (!r->rocket_latch)
            r->rocket_latch = 1;        /* 0x185d4: rocket, not ported */
        if (!boost)
            r->boost_latch = 0;
        else if (!r->boost_latch && b->z < FX(40)) {
            b->speed += accel * 8;
            r->boost_latch = 1;
            b->vz += accel * 16;
        }
        if (any && b->speed <= s->top) {
            b->speed += accel;
            s->fuel -= 0x666;
        }
        if (r->started) {
            if (in->left) col = 0;
            if (in->right) col = 2;
            if (in->up) row = 0;
            if (in->down) row = 2;
        }
        if (steer_dir[row][col] >= 0) {
            int t = steer_dir[row][col];
            s->heading = rc_turn[t][rc_turn[t][s->heading]];
        }
        if (!any || s->top < b->speed) {
            b->speed = fxmul(b->speed, 0xe666);
            if (b->speed < FX(2)) {
                b->speed = 0;
                o->frame = 2;
            }
        }
    }
    b->vx = fxmul(b->speed, rc_cos[s->heading]);
    b->vy = fxmul(b->speed, rc_sin[s->heading]);
    lv = ground_check(r, n);
    fall(b, lv * FX(1000));
    b->x += b->vx;
    b->y += b->vy;
    if (racer_contact(r, n, 4) || racer_contact(r, n, 2)) {
        if (!r->contact_latch[0]) {
            racer_bounce(r, n, racer_contact(r, n, 4) ? 4 : 2);
            r->contact_latch[0] = 1;
        }
    } else {
        r->contact_latch[0] = 0;
    }
    cell_checks(r, n);
    clamp(b);
    if (b->z > FX(8))
        o->frame = 0;
    o->anim = (short)(s->heading + 22);
    place_sprite(r, n);
}

/* 0x1467c: a computer racer standing still for 21 checks turns at random. */
static int ai_unstick(rc_race *r, int n)
{
    rc_body *b = &r->body[n];
    rc_stats *s = &r->stats[b->stats];

    if (b->vx != 0 || b->vy != 0)
        return 0;
    if (++r->stuck_count[b->stats] < 21)
        return 0;
    r->stuck_count[b->stats] = 0;
    s->heading = (rc_rand(r) % 40 + s->heading + 8) & 31;
    return 1;
}

/* 0x1af83 with the (empty) per-map scripts: 10% drop an object, else 10%
   boost.  Dropping is not ported yet; the rand() calls are kept. */
static void ai_waypoint_action(rc_race *r, int n)
{
    rc_body *b = &r->body[n];
    rc_stats *s = &r->stats[b->stats];

    if (rc_rand(r) % 100 < 10) {
        rc_rand(r);
        return;
    }
    if (rc_rand(r) % 100 < 10 && s->boosts != 0) {
        b->speed += FX(8);
        b->vz += FX(8);
        s->boosts--;
    }
}

/* bull_tick 0x12826, muis_tick 0x130f8, and after a crash bull_crash_tick
   0x19c8c / muis_crash_tick 0x1a42a: the same driving without the out check,
   with the crash anim (heading/2 + 54 or + 70, looping) instead of poses. */
static void ai_tick(rc_race *r, int n)
{
    rc_body *b = &r->body[n];
    int k = b->stats;
    rc_stats *s = &r->stats[k];
    rc_object *o = &r->obj[n];
    int wreck = r->wrecked[k];
    int32_t accel, *wp;

    b->prio = 0;
    if (!wreck && (s->damage < 0 || s->fuel < 0)) {
        r->out[k] = 1;
        return;
    }
    accel = (int32_t)(0x1e00000000LL / s->mass);
    if (r->started && !r->finished[k]) {
        wp = r->wp[k][r->wp_next[k]];
        if (iabs((b->x >> 16) - (wp[0] >> 16)) + iabs((b->y >> 16) - (wp[1] >> 16))
            < 40) {
            ai_waypoint_action(r, n);
            if (!r->finished[k] && r->wp_next[k] < r->wp_count[k] - 1)
                r->wp_next[k]++;
        }
        if (!ai_unstick(r, n)) {
            int d;
            wp = r->wp[k][r->wp_next[k]];
            d = direction_to(b, wp[0], wp[1]);
            if (d != s->heading)
                s->heading = rc_turn[d][s->heading];
        }
        r->ai_accel[k] = !r->finished[k];
        if (wreck && r->finished[k])
            o->frame = 2;
    }
    if (!r->finished[k] && s->boost_state == 0 && r->ai_accel[k]
        && b->speed < s->top) {
        b->speed += accel;
        if (++r->exhaust_count[k] > 3)
            r->exhaust_count[k] = 0;
    }
    if (!r->ai_accel[k] || s->top < b->speed)
        friction(b);
    if (!r->finished[k]) {
        b->vx = fxmul(b->speed, rc_cos[s->heading]);
        b->vy = fxmul(b->speed, rc_sin[s->heading]);
    } else {
        b->vx = b->vy = 0;
        if (wreck) {
            b->speed = 0;
            o->frame = 2;
        }
    }
    fall(b, 0);
    if (r->started && !r->finished[k])
        ai_unstick(r, n);
    ground_check(r, n);
    b->x += b->vx;
    b->y += b->vy;
    if (n == 2) {
        if (racer_contact(r, n, 4)) {
            if (!r->contact_latch[1]) {
                racer_bounce(r, n, 4);
                r->contact_latch[1] = 1;
            }
        } else {
            r->contact_latch[1] = 0;
        }
    }
    cell_checks(r, n);
    if (!wreck) {
        clamp_and_pose(r, n);
        return;
    }
    clamp(b);
    o->anim = (short)((s->heading >> 1) + (k == RC_BULL ? 54 : 70));
    place_sprite(r, n);
}

static void shadow_tick(rc_race *r, int n)     /* follower_tick 0x167aa */
{
    int racer = n - 1;
    rc_body *b = &r->body[n];
    const rc_stats *s = &r->stats[r->body[racer].stats];

    b->x = r->body[racer].x;
    b->y = r->body[racer].y;
    b->z = 0;
    b->prio = -100000;
    r->obj[n].anim = 4;
    r->obj[n].frame = (s->damage < 1 || s->fuel < 1) ? 32 : (short)(s->heading & 31);
    place_sprite(r, n);
}

/* Map objects: boosts, mines, signs, cacti.  Their callbacks (0x16a63,
   0x16b53, 0x16c5d, 0x16d30, 0x17236) place the sprite.  The cactus one
   counts its hit timer down and removes it at 0; the mine one counts its
   arming timer (life 10) down and draws the mine under everything. */
static void map_object_tick(rc_race *r, int n)
{
    rc_body *b = &r->body[n];

    b->prio = 0;
    if (r->obj[n].anim == 12) {
        if (b->life == -1) {
            r->obj[n].frame = 0;
        } else if (b->life <= 0) {
            free_object(r, n);
            return;
        } else {
            b->life--;
        }
    } else if (r->obj[n].anim == 6) {
        if (b->life > 0)
            b->life--;
        b->prio = -100000;
    }
    place_sprite(r, n);
}

static int new_map_object(rc_race *r, int anim, int period, int32_t x,
                          int32_t y)
{
    int n = alloc_object(r);

    if (n < 0)
        return -1;
    r->obj[n].period = (short)period;
    r->obj[n].anim = (short)anim;
    r->obj[n].frame = (short)(rc_rand(r) % rc_anims[anim].count);
    r->obj[n].tick = map_object_tick;
    memset(&r->body[n], 0, sizeof r->body[n]);
    r->body[n].stats = -1;
    r->body[n].x = x;
    r->body[n].y = y;
    return n;
}

static void find_bonus_objects(rc_race *r)     /* 0x15597 */
{
    int i, n;

    for (i = 0; i < r->map.w * r->map.h; i++) {
        rc_cell *c = &r->map.cells[i];
        int32_t x = ((i % r->map.w) * TILE_W + 16) << 16;
        int32_t y = ((i / r->map.w) * TILE_H + 20) << 16;

        switch (c->type) {
        case 10:
            n = new_map_object(r, 5, 3, x, y);
            if (n >= 0)
                r->body[n].life = (rc_rand(r) & 0xff) + 100;
            break;
        case 12:
            n = new_map_object(r, 6, 3, x, y);
            if (n >= 0)
                r->body[n].life = 10;
            break;
        case 13:
            n = new_map_object(r, 8, 2, x, y);
            break;
        case 15:
        case 16:
            n = alloc_object(r);
            if (n >= 0) {
                r->obj[n].period = 1;
                r->obj[n].anim = 12;
                r->obj[n].frame = 0;
                r->obj[n].tick = map_object_tick;
                memset(&r->body[n], 0, sizeof r->body[n]);
                r->body[n].stats = -1;
                r->body[n].life = -1;
                r->body[n].x = x + FX(16);
                r->body[n].y = y - FX(10);
            }
            break;
        case 18:
            n = new_map_object(r, 11, 3, x, y);
            break;
        case 19:
            n = new_map_object(r, 10, 3, x, y);
            break;
        default:
            continue;
        }
        c->object = (uint8_t)(n < 0 ? 0 : n);
    }
}

static void find_waypoints(rc_race *r)         /* 0x1517a */
{
    int i;

    memset(r->wp, 0xff, sizeof r->wp);
    memset(r->wp_count, 0, sizeof r->wp_count);
    for (i = 0; i < r->map.w * r->map.h; i++) {
        const rc_cell *c = &r->map.cells[i];
        int k = c->racer == 1 ? RC_REDCAT : c->racer == 2 ? RC_BULL
              : c->racer == 4 ? RC_MUIS : -1;
        if (k < 0 || c->waypoint >= RC_MAX_WAYPOINTS)
            continue;
        r->wp[k][c->waypoint][0] = ((i % r->map.w) * TILE_W + 16) << 16;
        r->wp[k][c->waypoint][1] =
            ((i / r->map.w) * TILE_H + (k == RC_MUIS ? 16 : 20)) << 16;
        r->wp_count[k]++;
    }
}

static void init_racer(rc_race *r, int k, int32_t mass, int32_t fuel,
                       int32_t damage, int32_t top)
{
    int n = 2 * k;
    rc_stats *s = &r->stats[k];
    rc_body *b = &r->body[n];

    memset(s, 0, sizeof *s);
    s->mass = mass;
    s->fuel = fuel;
    s->damage = damage;
    s->top = top;
    b->stats = k;
    b->z = b->vx = b->vy = b->vz = b->speed = 0;
    s->heading = direction_to(b, r->wp[k][1][0], r->wp[k][1][1]);
    r->obj[n].period = 1;
    r->obj[n].anim = (short)racer_anim[k];
    r->obj[n].tick = k == RC_REDCAT ? player_tick : ai_tick;
    r->obj[n + 1].period = 1;
    r->obj[n + 1].anim = 4;
    r->obj[n + 1].tick = shadow_tick;
    r->body[n + 1].stats = -1;
}

/* 0x1475f, after init_objects: centre on the player, clamped */
static void camera_start(rc_race *r)
{
    const rc_body *b = &r->body[0];
    int max_x = (r->map.w - (VIEW_COLS + 1)) * TILE_W;
    int max_y = (r->map.h - (VIEW_ROWS + 1)) * TILE_H;

    r->scroll_x = (short)(b->x >> 16) - VIEW_COLS * TILE_W / 2;
    if (r->scroll_x < 0) r->scroll_x = 0;
    if (r->scroll_x > max_x) r->scroll_x = max_x;
    r->scroll_y = (short)(b->y >> 16) - VIEW_ROWS * TILE_H / 2;
    if (r->scroll_y < 0) r->scroll_y = 0;
    if (r->scroll_y > max_y) r->scroll_y = max_y;
}

int rc_race_init(rc_race *r, const uint8_t *map_data, uint32_t map_size,
                 int map_index, const rc_hotspot *hotspots,
                 const int start_order[RC_RACERS])
{
    int i;

    memset(r, 0, sizeof *r);
    if (rc_map_parse(&r->map, map_data, map_size) != 0)
        return -1;
    rc_tables_init();
    r->hotspots = hotspots;
    r->map_index = map_index;
    r->rng = 1;
    find_waypoints(r);
    for (i = 0; i < RC_RACERS; i++)
        if (r->wp_count[i] < 1) {   /* "No waypoints in map file" */
            rc_map_free(&r->map);
            return -1;
        }
    for (i = 0; i < RC_OBJECTS; i++) {
        r->obj[i].period = -1;
        r->body[i].stats = -1;
    }
    r->free_objects = RC_OBJECTS - 6;

    /* init_objects (0x148db): slot i of the start order stands on waypoint 0
       of list i. */
    for (i = 0; i < RC_RACERS; i++) {
        int k = start_order[i];
        r->body[2 * k].x = r->wp[i][0][0];
        r->body[2 * k].y = r->wp[i][0][1];
        r->wp_next[i] = 1;
    }
    init_racer(r, RC_REDCAT, FX(60), FX(75), 100, FX(16));
    r->stats[RC_REDCAT].boosts = 5;
    r->stats[RC_REDCAT].rockets = 10;
    init_racer(r, RC_BULL, FX(90), FX(100), 100, FX(ai_top_speed[map_index]));
    init_racer(r, RC_MUIS, FX(60), FX(100), 100, FX(ai_top_speed[map_index]));
    for (i = RC_BULL; i <= RC_MUIS; i++) {
        r->stats[i].boosts = 3;
        r->stats[i].f18 = 3;
        r->stats[i].rockets = 3;
        r->stats[i].drops = 3;
    }
    find_bonus_objects(r);
    for (i = 0; i < RC_RACERS; i++)
        r->order[i] = -1;
    camera_start(r);
    rc_race_tick(r, &(rc_input){ 0 });  /* place sprites before the lights */
    r->frame_count = 0;
    return 0;
}

void rc_race_free(rc_race *r)
{
    rc_map_free(&r->map);
}

/* camera_follow (0x138ed): eases 1/16 toward the racer, leading 128 px
   horizontally and 160 px vertically in the direction of motion.  The view
   counts as 19 x 10 tiles ([0x60044], [0x60048], set in race_init). */
static void camera_follow(rc_race *r)
{
    const rc_body *b = &r->body[r->camera_racer * 2];
    int cols = VIEW_COLS, rows = VIEW_ROWS;
    int sx = b->vx < 0 ? -1 : b->vx > 0;
    int sy = b->vy < 0 ? -1 : b->vy > 0;
    int tx = (short)((b->x >> 16) - (cols + 1) * 16 + sx * 128);
    int ty = (short)(((b->y - b->z) >> 16) - (rows + 1) * 20 + sy * 160);
    int max_x = (r->map.w - (cols + 1)) * TILE_W;
    int max_y = (r->map.h - (rows + 1)) * TILE_H;

    r->scroll_x += (tx - r->scroll_x) >> 4;
    r->scroll_y += (ty - r->scroll_y) >> 4;
    if (r->scroll_x < 0) r->scroll_x = 0;
    if (r->scroll_x > max_x) r->scroll_x = max_x;
    if (r->scroll_y < 0) r->scroll_y = 0;
    if (r->scroll_y > max_y) r->scroll_y = max_y;
}

/* switch_anims (0x15084) + start_crash (0x14f3e): the first racer that is
   out (player, then Brutus, then Snitch) loses 2 px/frame of top speed and
   becomes a wreck; a computer racer also gets its damage back to 100. */
static void switch_anims(rc_race *r)
{
    static const int obj_of[RC_RACERS] = { 0, 2, 4 };
    int k;

    for (k = 0; k < RC_RACERS; k++)
        if (r->out[k])
            break;
    if (k == RC_RACERS)
        return;
    r->out[k] = 0;
    r->wrecked[k] = 1;
    r->stats[k].top -= FX(2);
    r->obj[obj_of[k]].frame = 0;
    if (k == RC_REDCAT) {
        r->obj[0].tick = player_wreck_tick;
        r->obj[0].anim = (short)(r->stats[k].heading + 22);
    } else {
        r->obj[obj_of[k]].anim =
            (short)((r->stats[k].heading >> 1) + (k == RC_BULL ? 54 : 70));
        r->stats[k].damage = 100;
    }
}

void rc_race_tick(rc_race *r, const rc_input *in)
{
    int n;

    r->in = *in;
    switch_anims(r);
    r->frame_count++;
    for (n = 0; n < RC_OBJECTS; n++) {         /* objects_tick 0x25c55 */
        rc_object *o = &r->obj[n];
        const rc_frame *f;

        if (o->period <= 0 || r->frame_count % o->period != 0)
            continue;
        o->tick(r, n);
        if (o->period <= 0)
            continue;
        f = obj_frame(o);
        o->kind = f->kind;
        o->image = f->image;
        o->frame = (short)((o->frame + 1) % rc_anims[o->anim].count);
    }
    camera_follow(r);
}

int rc_race_draw_order(const rc_race *r, int *out)
{
    int32_t key[RC_OBJECTS];
    int n, count = 0, i;

    for (n = 0; n < RC_OBJECTS; n++) {
        const rc_body *b = &r->body[n];

        if (r->obj[n].period == -1 || r->obj[n].image <= 0)
            continue;
        /* 0x11b0e: descending (1000 - x)*10 - 1000*y - z - prio */
        key[n] = (1000 - (b->x >> 16)) * 10 - (b->y >> 16) * 1000
               - (b->z >> 16) - b->prio;
        for (i = count; i > 0 && key[out[i - 1]] < key[n]; i--)
            out[i] = out[i - 1];
        out[i] = n;
        count++;
    }
    return count;
}

int rc_race_over(rc_race *r)
{
    static const int obj_of[RC_RACERS] = { 0, 2, 4 };
    int k;

    for (k = 0; k < RC_RACERS; k++)
        if (r->finished[k] && !r->order_done[k]) {
            r->order_done[k] = 1;
            r->order[r->order_n++] = obj_of[k];
        }
    if (!((r->order_n == 2 && r->order_done[RC_REDCAT]) || r->order_n == 3))
        return 0;
    if (r->order_n < 3)
        r->order[2] = !r->order_done[RC_BULL] ? 2 : 4;
    return 1;
}
