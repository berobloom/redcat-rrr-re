/* heap.c: the race's object pool and the objects it hands out, 0x17e0b-0x18885 (the
   'heap.c' guards at 0x17e87, 0x17faa and 0x180d3; heap.c's strings follow shadeobj.c's).
   Where the module ends is LIKELY: fn_18886 and ground_check (0x1889d) are left in rrr.c until
   map.c's start is known. */

#include "rrr.h"

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

/* ---- 0x17e87 -------------------------------------------------------------------------------- */

int spawn_puffs(int count, int racer, int kind)
{
    int n;

#line 72 "heap.c"
    if (count <= 0)
        rc_error(" %s(%d) : illegal amount of objects requested (%d)\n", "heap.c", __LINE__, count);
    while (count-- > 0) {
        n = alloc_object();
        if (n == -1)
            return -1;
        objects[n].period = 2;
        if (kind > 50) {
            objects[n].anim = 7;
            objects[n].callback = (void (*)(void))fn_16e46;
        } else {
            objects[n].anim = 17;
            objects[n].callback = (void (*)(void))obj_callback_1709c;
        }
        objects[n].frame = 0;
        bodies[n].x = ((rand() & 15) - 7 << 16) + bodies[racer].x;
        bodies[n].y = ((rand() & 15) - 7 << 16) + bodies[racer].y;
        bodies[n].z = ((rand() & 7) - 3 << 16) + bodies[racer].z;
        bodies[n].vx = 0;
        bodies[n].vy = 0;
        bodies[n].vz = 0;
        bodies[n].f24 = 6;
    }
    return 0;
}


/* ---- 0x17faa -------------------------------------------------------------------------------- */

int explode_objects(int count, int racer)
{
    int n;

#line 108 "heap.c"
    if (count <= 0)
        rc_error(" %s(%d) : illegal amount of objects requested (%d)\n", "heap.c", __LINE__, count);
    while (count-- > 0) {
        n = alloc_object();
        if (n == -1)
            return -1;
        objects[n].period = 1;
        objects[n].callback = (void (*)(void))obj_callback_1709c;
        objects[n].anim = 17;
        objects[n].frame = 0;
        bodies[n].x = bodies[racer].x;
        bodies[n].y = bodies[racer].y;
        bodies[n].z = bodies[racer].z;
        bodies[n].vx = 0;
        bodies[n].vy = 0;
        bodies[n].vz = 0;
        bodies[n].f24 = 18;
        play_fx(1, abs((bodies[n].x >> 16) - (bodies[camera_racer].x >> 16))
                 + abs((bodies[n].y >> 16) - (bodies[camera_racer].y >> 16)));
    }
    return 0;
}


/* ---- 0x180d3 -------------------------------------------------------------------------------- */

int spawn_trail(int count, int racer)
{
    int n;

#line 140 "heap.c"
    if (count <= 0)
        rc_error(" %s(%d) : illegal amount of objects requested (%d)\n", "heap.c", __LINE__, count);
    while (count-- > 0) {
        n = alloc_object();
        if (n == -1)
            return -1;
        objects[n].period = 1;
        objects[n].callback = (void (*)(void))fn_1733e;
        objects[n].anim = 3;
        objects[n].frame = stats[bodies[racer].stats].heading;
        bodies[n].x = bodies[racer].x;
        bodies[n].y = bodies[racer].y;
        bodies[n].z = bodies[racer].z;
        bodies[n].vx = 0;
        bodies[n].vy = 0;
        bodies[n].vz = bodies[racer].vz;
        bodies[n].f24 = 6;
    }
    return 0;
}


/* ---- 0x181ba -------------------------------------------------------------------------------- */

int new_boost_pickup(int x, int y)
{
    int n;

    n = alloc_object();
    if (n == -1)
        return -1;
    objects[n].period = 2;
    objects[n].callback = (void (*)(void))obj_callback_16a63;
    objects[n].anim = 8;
    objects[n].frame = rand() % animset[objects[n].anim].count;
    bodies[n].z = 0;
    bodies[n].vx = 0;
    bodies[n].vy = 0;
    bodies[n].vz = 0;
    bodies[n].x = x;
    bodies[n].y = y;
    return n;
}


/* ---- 0x18276 -------------------------------------------------------------------------------- */

int new_cactus(int x, int y)
{
    int n;

    n = alloc_object();
    if (n == -1)
        return -1;
    objects[n].period = 1;
    objects[n].callback = (void (*)(void))cactus_tick;
    objects[n].anim = 12;
    objects[n].frame = 0;
    bodies[n].z = 0;
    bodies[n].vx = 0;
    bodies[n].vy = 0;
    bodies[n].vz = 0;
    bodies[n].f24 = -1;
    bodies[n].x = x + 0x100000;
    bodies[n].y = y - 0xa0000;
    return n;
}


/* ---- 0x18324 -------------------------------------------------------------------------------- */

int fn_18324(int x, int y)
{
    int n;

    n = alloc_object();
    if (n == -1)
        return -1;
    objects[n].period = 2;
    objects[n].callback = (void (*)(void))obj_callback_16ad9;
    objects[n].anim = 9;
    objects[n].frame = rand() % animset[objects[n].anim].count;
    bodies[n].z = 0;
    bodies[n].vx = 0;
    bodies[n].vy = 0;
    bodies[n].vz = 0;
    bodies[n].x = x;
    bodies[n].y = y;
    return n;
}


/* ---- 0x1836d -------------------------------------------------------------------------------- */

int new_mine(int x, int y)
{
    int n;

    n = alloc_object();
    if (n == -1)
        return -1;
    debug_printf("object %d created mijn \n", n);
    objects[n].period = 3;
    objects[n].callback = (void (*)(void))mine_tick;
    objects[n].anim = 6;
    objects[n].frame = rand() % animset[objects[n].anim].count;
    bodies[n].f24 = 10;
    bodies[n].z = 0;
    bodies[n].vx = 0;
    bodies[n].vy = 0;
    bodies[n].vz = 0;
    bodies[n].x = x;
    bodies[n].y = y;
    return n;
}


/* ---- 0x18445 -------------------------------------------------------------------------------- */

int new_sign(int x, int y, int anim)
{
    int n;

    n = alloc_object();
    if (n == -1)
        return -1;
    objects[n].period = 3;
    objects[n].callback = (void (*)(void))sign_tick;
    objects[n].anim = anim;
    objects[n].frame = rand() % animset[objects[n].anim].count;
    bodies[n].z = 0;
    bodies[n].vx = 0;
    bodies[n].vy = 0;
    bodies[n].vz = 0;
    bodies[n].x = x;
    bodies[n].y = y;
    return n;
}


/* ---- 0x18502 -------------------------------------------------------------------------------- */

int new_bomb(int x, int y)
{
    int n;

    n = alloc_object();
    if (n == -1)
        return -1;
    objects[n].period = 3;
    objects[n].callback = (void (*)(void))bomb_tick;
    objects[n].anim = 5;
    objects[n].frame = rand() % animset[objects[n].anim].count;
    bodies[n].z = 0;
    bodies[n].vx = 0;
    bodies[n].vy = 0;
    bodies[n].vz = 0;
    bodies[n].x = x;
    bodies[n].y = y;
    bodies[n].f24 = (rand() & 0xff) + 100;
    return n;
}


/* ---- 0x185d4 -------------------------------------------------------------------------------- */

int fire_rocket(int racer)
{
    int n;

    n = alloc_object();
    if (n == -1)
        return -1;
    objects[n].period = 1;
    objects[n].callback = (void (*)(void))rocket_tick;
    objects[n].anim = 15;
    objects[n].frame = stats[bodies[racer].stats].heading;
    bodies[n].stats = stats[bodies[racer].stats].heading;
    bodies[n].x = fixmul(0x190000, cos_tbl[stats[bodies[racer].stats].heading]) + bodies[racer].x;
    bodies[n].y = fixmul(0x190000, sin_tbl[stats[bodies[racer].stats].heading]) + bodies[racer].y;
    bodies[n].z = bodies[racer].z + 0x140000;
    bodies[n].vx = fixmul(0x190000, cos_tbl[stats[bodies[racer].stats].heading]);
    bodies[n].vy = fixmul(0x190000, sin_tbl[stats[bodies[racer].stats].heading]);
    bodies[n].f24 = 100;
    return n;
}


/* ---- 0x18759 -------------------------------------------------------------------------------- */

/* differs: 8 rows, only the base/index choice: the game keeps n * 16 in EAX (the alloc_object
   result, shifted in place) and loads the objects pointer into ECX/EDX per store, ours copies n
   to ECX and shifts that, so every store encodes base/index the other way round. This switch is
   the best shape found: if/else-if, else-if with !kind, and (*(objects + n)) all give 11 rows and
   299 bytes (the game is 301); a struct object pointer local, a short kind and a second n copy
   are worse. */
int spawn_dropped_object(int racer, int kind)
{
    int n;

    n = alloc_object();
    if (n == -1)
        return -1;
    objects[n].period = 1;
    switch (kind) {
    case 2:
        objects[n].callback = (void (*)(void))dropped2_tick;
        break;
    case 0:
        objects[n].callback = (void (*)(void))dropped0_tick;
        break;
    default:
        objects[n].callback = (void (*)(void))dropped4_tick;
        break;
    }
    objects[n].anim = 16;
    objects[n].frame = stats[bodies[racer].stats].heading;
    bodies[n].stats = stats[bodies[racer].stats].heading;
    bodies[n].x = fixmul(0x190000, cos_tbl[bodies[n].stats]) + bodies[racer].x;
    bodies[n].y = fixmul(0x190000, sin_tbl[bodies[n].stats]) + bodies[racer].y;
    bodies[n].z = 0x280000;
    bodies[n].speed = 0x190000;
    bodies[n].f24 = 150;
    return n;
}

