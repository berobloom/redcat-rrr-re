/* shadeobj.c: the race's object callbacks, 0x167aa-0x17d93 (its __FILE__ string, 'shadeobj.c',
   starts the module's strings after raceanim.c's padding; the callbacks' 'illegal object : %d'
   follows it). The callbacks jump into each other's code, so they match only together. */

#include "rrr.h"

/* ---- 0x167aa -------------------------------------------------------------------------------- */

/* differs: 41 rows, register choice in the frame tail; tried both declaration orders and three
   orders of the body stores. */
int follower_tick(short n)
{
    short k;
    int t;

#line 16 "shadeobj.c"
    t = check_object_nr(n - 1, "shadeobj.c", __LINE__);
    sprites[objects[n].sprite].mode = 6;
    if (objects[n].period == -1)
        return 0;
    if (t < 0 || t > 5)
        rc_error("illegal shade object : %d following :%d", (int)n, t);
    bodies[n].x = bodies[t].x;
    bodies[n].y = bodies[t].y;
    bodies[n].z = 0;
    bodies[n].f1c = -100000;
    objects[n].anim = 4;
    if (stats[bodies[t].stats].damage <= 0 || stats[bodies[t].stats].fuel <= 0)
        objects[n].frame = 32;
    else
        objects[n].frame = stats[bodies[t].stats].heading & 31;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}


/* ---- 0x16945 -------------------------------------------------------------------------------- */

/* the early return matters: an if around the store keeps the offset in a register */
int free_object_tick(short n)
{
    if (objects[n].period == -1)
        return 0;
    objects[n].period = -1;
    return 0;
}

/* ---- 0x1696c -------------------------------------------------------------------------------- */

int obj_callback_1696c(short n)
{
    short k;

    sprites[objects[n].sprite].mode = 2;
    if (objects[n].period == -1)
        return 0;
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = 0;
    objects[n].anim = 7;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x16a63 -------------------------------------------------------------------------------- */

int obj_callback_16a63(short n)
{
    short k;

    sprites[objects[n].sprite].mode = 2;
    if (objects[n].period == -1)
        return 0;
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = 0;
    objects[n].anim = 8;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x16ad9 -------------------------------------------------------------------------------- */

int obj_callback_16ad9(short n)
{
    short k;

    sprites[objects[n].sprite].mode = 2;
    if (objects[n].period == -1)
        return 0;
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = 0;
    objects[n].anim = 9;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x16b53 -------------------------------------------------------------------------------- */

int mine_tick(short n)
{
    short k;

    sprites[objects[n].sprite].mode = 2;
    if (objects[n].period == -1)
        return 0;
    if (bodies[n].f24 > 0)
        bodies[n].f24--;
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = -100000;
    objects[n].anim = 6;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x16c5d -------------------------------------------------------------------------------- */

int sign_tick(short n)
{
    short k;

    sprites[objects[n].sprite].mode = 2;
    if (objects[n].period == -1)
        return 0;
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = 0;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x16d30 -------------------------------------------------------------------------------- */

int bomb_tick(short n)
{
    short k;

    sprites[objects[n].sprite].mode = 2;
    if (objects[n].period == -1)
        return 0;
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f24--;
    if (bodies[n].f24 <= 0) {
        explode_objects(1, n);
        free_object(n);
        return 0;
    }
    bodies[n].f1c = -100000;
    objects[n].anim = 5;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x16e46 -------------------------------------------------------------------------------- */

int fn_16e46(short n)
{
    short k;
    int cx, cy;

    sprites[objects[n].sprite].mode = 6;
    if (objects[n].period == -1)
        return 0;
    bodies[n].f24--;
    if (bodies[n].f24 <= 0) {
        free_object(n);
        return 0;
    }
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = -10;
    objects[n].anim = 7;
    objects[n].frame = objects[n].frame % animset[objects[n].anim].count;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    bodies[n].x += bodies[n].vx;
    bodies[n].y += bodies[n].vy;
    bodies[n].z += bodies[n].vz;
    bodies[n].vx = fixmul(bodies[n].vx, 0xe666);
    bodies[n].vy = fixmul(bodies[n].vy, 0xe666);
    bodies[n].vz = fixmul(bodies[n].vz, 0xe666);
    cx = (bodies[n].x >> 16) / 32;
    cy = (bodies[n].y >> 16) / 40;
    if (map[(cy * map_w + cx) * 6 + 5] * 1000 > bodies[n].z) {
        bodies[n].x -= bodies[n].vx >> 1;
        bodies[n].y -= bodies[n].vy >> 1;
        bodies[n].vx = 0;
        bodies[n].vy = 0;
        bodies[n].speed = -bodies[n].speed >> 1;
    }
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x1709c -------------------------------------------------------------------------------- */

int obj_callback_1709c(short n)
{
    short k;

    sprites[objects[n].sprite].mode = 1;
    if (objects[n].period == -1)
        return 0;
    bodies[n].f24--;
    if (bodies[n].f24 <= 0) {
        free_object(n);
        return 0;
    }
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = -10;
    objects[n].anim = 17;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    bodies[n].x += bodies[n].vx;
    bodies[n].y += bodies[n].vy;
    bodies[n].z += bodies[n].vz;
    bodies[n].vx = fixmul(bodies[n].vx, 0xe666);
    bodies[n].vy = fixmul(bodies[n].vy, 0xe666);
    bodies[n].vz = fixmul(bodies[n].vz, 0xe666);
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x17236 -------------------------------------------------------------------------------- */

int cactus_tick(short n)
{
    short k;

    sprites[objects[n].sprite].mode = 1;
    if (objects[n].period == -1)
        return 0;
    if (bodies[n].f24 == -1)
        objects[n].frame = 0;
    else if (bodies[n].f24 <= 0) {
        free_object(n);
        return 0;
    } else
        bodies[n].f24--;
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = 0;
    objects[n].anim = 12;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x1733e -------------------------------------------------------------------------------- */

int fn_1733e(short n)
{
    short k;
    int g;

    sprites[objects[n].sprite].mode = 6;
    if (objects[n].period == -1)
        return 0;
    bodies[n].f24--;
    if (bodies[n].f24 <= 0) {
        free_object(n);
        return 0;
    }
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = -100000;
    objects[n].anim = 3;
    if (objects[n].frame > 0)
        objects[n].frame--;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    g = ground_check(n);
    bodies[n].z += bodies[n].vz;
    if (bodies[n].vz > -0x140000)
        bodies[n].vz -= fixmul(0x4000, abs(bodies[n].vz)) + 0x4000;
    if (g * 1000 << 16 > bodies[n].z) {
        bodies[n].vx -= fixmul(bodies[n].vx, 0x2666);
        bodies[n].vy -= fixmul(bodies[n].vy, 0x2666);
        bodies[n].z = g * 1000 << 16;
        if (abs(bodies[n].vz) > 0x10000)
            bodies[n].vz = -fixmul(0x4ccc, bodies[n].vz);
        else
            bodies[n].vz = 0;
    }
    bodies[n].x += bodies[n].vx;
    bodies[n].y += bodies[n].vy;
    bodies[n].vx = fixmul(bodies[n].vx, 0xe666);
    bodies[n].vy = fixmul(bodies[n].vy, 0xe666);
    bodies[n].vz = fixmul(bodies[n].vz, 0xe666);
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x175b4 -------------------------------------------------------------------------------- */

int rocket_tick(short n)
{
    short k;
    int cx, cy, h;

    sprites[objects[n].sprite].mode = 2;
    if (objects[n].period == -1)
        return 0;
    bodies[n].f24--;
    if (bodies[n].f24 <= 0) {
        free_object(n);
        explode_objects(1, n);
        return 0;
    }
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = 0;
    objects[n].anim = 15;
    objects[n].frame = bodies[n].stats;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    if (bodies[n].f24 < 98) {
        if (hit_test(n, 2, 20)) {
            free_object(n);
            explode_objects(1, 2);
            stats[bodies[2].stats].damage -= 15;
            return 0;
        }
        if (hit_test(n, 4, 20)) {
            free_object(n);
            explode_objects(1, 4);
            stats[bodies[4].stats].damage -= 15;
            return 0;
        }
        if (hit_test(n, 0, 15)) {
            free_object(n);
            explode_objects(1, 0);
            stats[bodies[0].stats].damage -= 10;
            return 0;
        }
    }
    cx = (bodies[n].x + bodies[n].vx >> 16) / 32;
    cy = (bodies[n].y + bodies[n].vy >> 16) / 40;
    h = map[(cy * map_w + cx) * 6 + 5];
    ground_check(n);
    if (h > 0) {
        free_object(n);
        explode_objects(1, n);
        return 0;
    }
    if (map[(cy * map_w + cx) * 6 + 4] == 15 || map[(cy * map_w + cx) * 6 + 4] == 16) {
        bodies[map[(cy * map_w + cx) * 6 + 1]].f24 = 5;
        map[(cy * map_w + cx) * 6 + 4] = 0;
        free_object(n);
        explode_objects(1, n);
    }
    bodies[n].x += bodies[n].vx;
    bodies[n].y += bodies[n].vy;
    bodies[n].vx = fixmul(bodies[n].vx, 0xff3b);
    bodies[n].vy = fixmul(bodies[n].vy, 0xff3b);
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x178f6 -------------------------------------------------------------------------------- */

/* differs: 16 rows, the two damage blocks (the game loads bodies[r].stats into EAX and merges
   racer 4's block into racer 2's; ours loads EDX and does not). mine_tick, sign_tick, bomb_tick,
   fn_16e46 and cactus_tick differ only in their jumps into this function's epilogue. */
int dropped2_tick(short n)
{
    int h;
    short k;

    sprites[objects[n].sprite].mode = 2;
    if (objects[n].period == -1)
        return 0;
    bodies[n].f24--;
    if (bodies[n].f24 <= 0) {
        free_object(n);
        explode_objects(1, n);
        return 0;
    }
    if (bodies[n].speed < 0x20000) {
        free_object(n);
        explode_objects(1, n);
        return 0;
    }
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = 0;
    if (hit_test(n, 2, 10)) {
        free_object(n);
        explode_objects(1, n);
        stats[bodies[2].stats].damage -= 10;
        return 0;
    }
    if (hit_test(n, 4, 10)) {
        free_object(n);
        explode_objects(1, n);
        stats[bodies[4].stats].damage -= 10;
        return 0;
    }
    h = aim_at_player(n, 2);
    if (h - bodies[n].stats && h != -1)
        bodies[n].stats = turn_tbl[h][bodies[n].stats];
    bodies[n].vx = fixmul(bodies[n].speed, cos_tbl[bodies[n].stats]);
    bodies[n].vy = fixmul(bodies[n].speed, sin_tbl[bodies[n].stats]);
    bodies[n].x += bodies[n].vx;
    bodies[n].y += bodies[n].vy;
    bodies[n].speed = fixmul(bodies[n].speed, 0xffbe);
    objects[n].anim = 16;
    objects[n].frame = bodies[n].stats;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}


/* ---- 0x17b61 -------------------------------------------------------------------------------- */

int dropped0_tick(short n)
{
    int h;
    short k;

    sprites[objects[n].sprite].mode = 2;
    if (objects[n].period == -1)
        return 0;
    bodies[n].f24--;
    if (bodies[n].f24 <= 0) {
        free_object(n);
        explode_objects(1, n);
        return 0;
    }
    if (bodies[n].speed < 0x20000) {
        free_object(n);
        explode_objects(1, n);
        return 0;
    }
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = 0;
    if (hit_test(n, 0, 10)) {
        free_object(n);
        explode_objects(1, n);
        stats[bodies[0].stats].damage -= 10;
        return 0;
    }
    if (hit_test(n, 4, 10)) {
        free_object(n);
        explode_objects(1, n);
        stats[bodies[4].stats].damage -= 10;
        return 0;
    }
    h = aim_at_player(n, 0);
    if (h - bodies[n].stats && h != -1)
        bodies[n].stats = turn_tbl[h][bodies[n].stats];
    bodies[n].vx = fixmul(bodies[n].speed, cos_tbl[bodies[n].stats]);
    bodies[n].vy = fixmul(bodies[n].speed, sin_tbl[bodies[n].stats]);
    bodies[n].x += bodies[n].vx;
    bodies[n].y += bodies[n].vy;
    bodies[n].speed = fixmul(bodies[n].speed, 0xffbe);
    objects[n].anim = 16;
    objects[n].frame = bodies[n].stats;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}


/* ---- 0x17c74 -------------------------------------------------------------------------------- */

int dropped4_tick(short n)
{
    int h;
    short k;

    sprites[objects[n].sprite].mode = 2;
    if (objects[n].period == -1)
        return 0;
    bodies[n].f24--;
    if (bodies[n].f24 <= 0) {
        free_object(n);
        explode_objects(1, n);
        return 0;
    }
    if (bodies[n].speed < 0x20000) {
        free_object(n);
        explode_objects(1, n);
        return 0;
    }
    if (n < 0)
        rc_error("illegal object : %d ", (int)n);
    bodies[n].f1c = 0;
    if (hit_test(n, 2, 10)) {
        free_object(n);
        explode_objects(1, n);
        stats[bodies[2].stats].damage -= 10;
        return 0;
    }
    if (hit_test(n, 4, 10)) {
        free_object(n);
        explode_objects(1, n);
        stats[bodies[4].stats].damage -= 10;
        return 0;
    }
    h = aim_at_player(n, 4);
    if (h - bodies[n].stats && h != -1)
        bodies[n].stats = turn_tbl[h][bodies[n].stats];
    bodies[n].vx = fixmul(bodies[n].speed, cos_tbl[bodies[n].stats]);
    bodies[n].vy = fixmul(bodies[n].speed, sin_tbl[bodies[n].stats]);
    bodies[n].x += bodies[n].vx;
    bodies[n].y += bodies[n].vy;
    bodies[n].speed = fixmul(bodies[n].speed, 0xffbe);
    objects[n].anim = 16;
    objects[n].frame = bodies[n].stats;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

