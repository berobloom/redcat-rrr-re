/* shadeobj2.c: the object callbacks, 0x16945-0x17d93. One module, LIKELY: their
   'illegal object : %d ' block (0x51784-0x51798) is one string block - no gap between it and the
   strings of the functions that use it, padding byte 0x5179b before heap.c's - and mine/sign/bomb/
   cactus_tick, fn_16e46 and dropped0/2/4_tick all jump into dropped2_tick's epilogue at 0x17b59/
   0x17b5b, which WCC386 only does inside one object. The name is a placeholder (nothing here passes
   a __FILE__, and the string carries none). The boundary that costs bytes is follower_tick's
   (src/shadeobj.c): free_object_tick and the 0x1696c-0x16ad9 callbacks can sit on either side, both
   give the same bytes. */

#include "rrr.h"

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

/* differs: 16 rows, all from one instruction. In the FIRST damage block the game loads
   bodies[2].stats into EAX and computes the stats index (x*3 << 4) in EDX; ours loads EDX, copies
   to EAX, computes there and copies back (`mov edx,eax`, 2 bytes). That also moves the tail merge
   with the second (racer 4) block back by 8 bytes: the game's racer-4 block ends at
   `mov eax, [bodies[4].stats]` + `jmp 0x179c3`, ours repeats the multiply chain and joins at
   0x179cd. Net: our epilogue sits 10 bytes late (0x26d into the function instead of 0x263), so
   mine_tick, sign_tick, bomb_tick, fn_16e46 and cactus_tick - which are otherwise byte-identical -
   miss the `jmp 0x17b59` into this epilogue, and dropped0_tick/dropped4_tick show the same 16 rows.
   Fixing that one register choice is worth 8 functions.
   Nothing tried moves it: index locals (int/short, every declaration order), (stats + i)->damage,
   *(stats + i), bodies[i].stats + 0, * 1, (int) casts, = x - 10, += -10, + -10, 0x0a, an
   else-if chain, swapping the two blocks, a propagated `r = 2; r = 4;` local for both the
   hit_test and the damage index, `--bodies[n].f24`, moving `bodies[n].f1c = 0` before the
   `if (n < 0)` check; and no compiler option (-ox -oe, -oa, -oi, -oe alone) changes it.
   2026-11-23 synthetic probe mechanism (S.OBJ, .work/probe): with ONE damage statement Watcom
   emits the game's load-EAX shape (s4); adding a SECOND same-shape damage statement anywhere in
   the function flips the first to our load-EDX+copy shape, even when the two cannot merge
   (different constants, s6; aim tail removed, s5). rocket_tick's game code has the flipped
   (load-EDX) shape and matches as is, so the game's dropped2 source must keep the first
   statement natural while carrying a second damage site: writing block 2's damage in assign
   form (`x = x - 10`) keeps block 1 in the game's shape (s8) but block 2 then compiles to
   load-sub-store, not the game's `jmp` into block 1's RMW `sub [edx+0x6fbe8],0xa`; pointer
   spellings (`*(int *)((char *)stats + s*48 + 40) -= 10`) fold to the same tree and flip it
   back; `register` locals, bare-reference statements and paren variants change nothing. Look
   for a block-2 statement shape that is structurally distinct for the allocator yet still
   codegens to immediate-RMW `sub mem, imm`; nothing tried does.
   2026-11-27, 34 more spellings (.work/probe/d2, inplace.py), all still 16 rows: (short)10 /
   10-0 / 0x0a / (10) / + -10 on either side, bodies[(short)2] and bodies[2+0], hit_test's `!= 0`,
   `if (1)` around either damage statement, `{ }` braces, the period test as == 0xffff, `(short)n
   < 0`, 131072, 0x0000, `f24 = f24 - 1`, local racer index `r = 2;`/`r = 4;` (either block or
   both), and the local-index holder `s = bodies[X].stats; stats[s].damage -= 10;` - the last two
   do move the shape (580/574 bytes, 9/10 rows) but lose the game's 629-byte length and put the
   shared epilogue in the wrong place, which is what the five 0-row callbacks jump to.
   Declaration order and an extra dead short/int local move nothing at all. */
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

