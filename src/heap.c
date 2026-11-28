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

/* differs: 8 rows (306 vs the game's 301 bytes). The game's code is an if/else-if chain (`cmp edx,2;
   jne`, then `test edx,edx; jne`) that keeps n * 16 in EAX through all three arms and loads the
   objects pointer into EDX in each; this switch gives the game's registers but Watcom's compare
   tree (`test edx,edx; jbe` first) and a default arm that recomputes n << 4. Every if-shaped
   spelling (if/else-if, !kind, braces, nested, goto, switch-then-if, if-then-switch) gives the
   game's tree at 299 bytes but mirrors the registers (n * 16 copied to ECX, objects into EAX): 11
   rows. 2026-11-26, all >= 8: int/unsigned/long n, long kind, `if ((n = alloc_object()) == -1)`
   (128, .work/probe/sd2), six switch orders and hybrids (sd3), a second copy of n in the arms and
   the period store moved (sd4). Earlier: (*(objects + n)) and if/else-if 11 rows, a struct object
   pointer local, a short kind and a second n copy worse.
   2026-11-29 (.work/pi/spawn/v.py..v4.py, 31 variants scored, floor 7 rows): ifchain, ifkind,
   ifelsSwitch, sw0first, swdefirst, ifcmpne, swDefIf, swDefIf0, sw2d0, sw0d2, swd02, swucast,
   swnobreak, ifsw2, swcase4, sw2d0u, sw2d0cast, ifsw2sw, sw2case0only, ifk3, ifshort, ifchain_full,
   ifchain_nohi, sw2d0_dfl, sw2d0_regn, chain_u, chain_reg, sw0DefIf, sw2def2, chain_else_if_neg,
   sw2d0_sw0. Best 7 = switch with the arms written case 2 / default / case 0 (sw2d0; also 7 with
   `(unsigned)kind` or `register int n`), 1 row off the current text so NOT applied. CONFIRMED the
   two halves are independent (side-by-side dump: .work/pi/spawn/one.py): the switch keeps the
   game's period-store pair (`mov ecx,[@]` 0x1876f, `shl eax,4` 0x18775, `mov ,1` 0x18778) and the
   game's arm stores (`mov edx,[@]` + `mov [edx+eax+0xc]`), and only the tree is Watcom's range tree
   (`test edx,edx` + `jbe` 0x1877e, `cmp edx,2` + `jne` 0x18782) where the game tests for equality
   (`cmp edx,2` + `jne` 0x1877e, `test edx,edx` + `jne` 0x18793); every if-chain lands the game's
   two tests at 0x1877f/0x18793 but pays the period store as `mov ecx,eax` + `mov eax,[@]` +
   `shl ecx,4` and three arms `mov eax,[@]` + `mov [ecx+eax+0xc]` - so WCC's choice of which
   register holds n<<4 and which holds the objects base at 0x1876f IS decided by the dispatch
   construct that follows it (same statements, switch vs if, first difference 6 instructions
   earlier). Which arm recomputes n<<4 is set by the arm order: case 2 / default / case 0 puts the
   5-byte `mov eax,edi` + `shl eax,4` (0x18797, 0x1879f) where difflib aligns best (8 -> 7 rows).
   2026-11-28 round 2 (.work/pi/sd2/v.py, v2.py: 8 variants scored - 5 on src/heap.c, control 8, and
   3 on a sw2d0 base, control 7; none below 7): the arm order case 2 / default / case 0 (sw2d0, 7),
   the same with default's `break` dropped (sw2d0_nobreak, 7) and the same switching on a local
   `k = kind` (klocal2, 7); `if (kind == 2)` in front of a `switch (kind)` holding only
   `default:` and `case 0:` 12, `default: if (kind == 0) .. else ..` inside the switch 11 (11 with
   `!kind` too), the period store moved after a full if-chain 12, `switch ((short)kind)` 8.
   CONFIRMED again: the equality tree comes only with the mirrored registers - the hybrid keeps the
   switch's registers in its first arm and pays the mirror in the other two. */
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

