/* shadeobj2.c: the object callbacks, 0x16945-0x17d93. One module, LIKELY: their
   'illegal object : %d ' block (0x51784-0x51798) is one string block - no gap between it and the
   strings of the functions that use it, padding byte 0x5179b before heap.c's - and mine/sign/bomb/
   cactus_tick, smoke_tick and dropped0/2/4_tick all jump into dropped2_tick's epilogue at 0x17b59/
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

int still_smoke_tick(short n)
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

int boost_pickup_tick(short n)
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

int orb_pickup_tick(short n)
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

int smoke_tick(short n)
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

int explosion_tick(short n)
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

int trail_tick(short n)
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

/* differs: 2 rows, one register choice in the last x store: the game does `sub edi,0x10` +
   `mov [objects+4],di`, ours `lea esi,[edi-0x10]` + `mov [objects+4],si`, for the same
   `(bodies[n].x >> 16) - frames[k].f8 - 16`. 619 = 619 bytes.
   What took 16 -> 2 (and took dropped0_tick 0x17b61, dropped4_tick 0x17c74, mine_tick 0x16b53,
   sign_tick 0x16c5d, bomb_tick 0x16d30, smoke_tick 0x16e46 and cactus_tick 0x17236 to MATCH): the
   FIRST damage block reads the stats index through a local the function uses again later -
   `h = bodies[2].stats; stats[h].damage -= 10;` with the same `int h` the aim block uses - instead
   of `stats[bodies[2].stats].damage -= 10;`. With the literal index WCC colours the two blocks'
   index temps differently (first load-EDX-then-copy, second load-EAX), the two `*3 << 4` chains are
   not identical text, the second block cannot jmp into the first's chain, and this function lands 10
   bytes long, so the epilogue at 0x17b59 that the five callbacks jmp to sits wrong. Sharing `h` makes
   both chains the same virtual value: the chain is emitted once, racer 4's block ends at
   `mov eax,[bodies[4].stats]` + `jmp 0x179c3` like the game's, and dropped0/dropped4 tail-merge into
   this function's aim call (0x17a11) and epilogue (0x17b5b) the way the game's do. Both must use `h`:
   a separate `int s` (628 B, 44 rows) or `short s` (619 B but `mov ax,[..]` + `cwtl`, 21 rows) or
   holders in both blocks (622, 43) all fail; `short` is what merges the chains but truncates the
   load, and the second block keeps its literal index (622, 43). 34 older spellings of the two
   damage statements (.work/probe/d2) and ~70 of the tail (.work/probe/d8-d11: parens and operand
   order on the x and y lines, `- (frames[k].f8 + 16)`, `h = bodies[n].x >> 16` holders, `(short)`
   casts, extra dead locals, declaration order, pragma aux clobber lists) leave those 2 rows.  2026-11-28: 16 more through vbatch (.work/pi/d2/v.py, v2.py) - `animset[16]` 38
   rows, `steps[bodies[n].stats]` 29, both 48, a holder for the whole x value 40, the chain `h =
   bodies[n].x >> 16; h -= frames[k].f8; h -= 16;` 51, `- (frames[k].f8 + 16)` 6, `bodies[n].speed`
   cached in the dead `h` 19, `bodies[n].stats` cached in `h` 54, the aim test's operand order 6, `int
   k` 12, `stats[h].damage = stats[h].damage - 10;` 53, `bodies[n].f24 = bodies[n].f24 - 1;` 30, the y
   line's operand order 40, and `rc_error("illegal object : %d ", n)` without the `(int)` cast 2 (the
   same bytes). Nothing beats 2: the pair moves with any id change in the function but never to the
   game's `sub` + `mov ,di`, and splitprobe 2026-11-28 (.work/pi/split/probe/dropped2_tick.txt) pads
   all 37 statement starts at n = 6, 14, 20 and never goes under control, so no block split decides
   it. 2026-11-28b (round 2, .work/pi/d2r2/v.py-v4.py): 28 more spellings of the x line - four `h`
   holder forms (`objects[n].x = (h = (bodies[n].x >> 16) - frames[k].f8) - 16;` and `hpre` compile
   byte-identical to the control, so `h` is CSEd away), `-16 + (...)`, `(short)` casts, `- 16` before
   `frames[k].f8` (5 rows), an `int x` in a nested block (10), a `struct frame *fp = frames + k` for
   both lines (38), the y line first (38), `h` for both lines (41) and a `short k`/`int h` decl swap
   (2). Seven tie at 2 with identical bytes; floor 2 again.

   Why the two rows are an allocation choice, not this statement's C (CONFIRMED 2026-11-28): in the
   game's module 0x16c00-0x17d60 the same x-line text appears nine times - seven compile to
   `lea r,[r-0x10]` (0x16c22, 0x16d03, 0x16e0d, 0x171eb, 0x17323, 0x1757b, 0x178b1) and two to
   `sub r,0x10` (smoke_tick 0x17052 and here 0x17b23) - and our build agrees with every one of them, so
   the identical C in smoke_tick gives the in-place `sub eax,0x10` (ours 0x1704d-0x17052: `sar eax,0x10`
   / `sub eax,edi` / `sub eax,0x10`) while the identical C in cactus_tick gives `lea ebx,[edi-0x10]`
   (ours and the game's both at 0x17323). SPECULATIVE the tie is decided by which register holds the
   accumulator (EAX folds it, EDI gets a fresh destination), which no spelling of this statement or of
   the locals reaches: the 16 + 70 + 34 + 28 variants only move the pair. That question stays open. */
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
        h = bodies[2].stats;
        stats[h].damage -= 10;
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
        h = bodies[0].stats;
        stats[h].damage -= 10;
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
        h = bodies[2].stats;
        stats[h].damage -= 10;
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

