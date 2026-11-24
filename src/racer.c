/* racer.c - racer collision geometry (address order, module boundary unknown yet).
 * These four are one object in the game: racer_bounce's tail jumps into racer_contact's
 * epilogue at 0x142d6, and Watcom shares an epilogue only within one object - so everything
 * between them (on_screen, hit_test) is in that object too, and main (0x111e8) is not: the
 * game's own C shares an epilogue only 6 times in 0x10010-0x1B915 and never between main and
 * here, while our single rrr.c object does (discoveries.md "The game's own C is many objects").
 */
#include "rrr.h"

/* ---- 0x1404a -------------------------------------------------------------------------------- */

/* Byte-identical 2026-11-23. Two things carried it:
   - the corners are declared AND written in the order x1, y2, x2, y1, which gives the game's ten
     slots exactly (y1 [esp+0x10], x2 0x20, y2 4, x3 0, y3 0x18, x4 0x1c, dy2 in EBP, y4 in ESI);
     the other 23 declaration orders score 6-12 rows here (inplace.py), and with the old declaration
     order the 23 other statement orders scored 14-52 in src/rrr.c.
   - this file: the game keeps its own epilogue here (0x142d6) and racer_bounce's tail jumps into
     it, while in src/rrr.c Watcom merged ours into main's pop/ret, which the game never does.
   The first corner test passes x1, y2, x2, y1, not x1, y1, x2, y1 as this function said until
   2026-11-23: left_of's matched body reads its args as EDX=y1, EBX=x2, ECX=y2, stack x3 then y3,
   and the game's four loads are x1, y2, x2, y1 - the old spelling tested a corner that is not
   there (2 rows). The corners are the racer's four 50 px box corners: the heading vector and it
   rotated by (h+0x28)&0x1f (= 90 deg), so dx2 = -dy1 and dy2 = dx1 in value. */
int racer_contact(int a, int b)
{
    int dx1, dy1, dx2, dy2, x1, y2, x2, y1, x3, y3, y4, x4;

    dx1 = fixmul(0x320000, cos_tbl[stats[bodies[b].stats].heading]);
    dy1 = fixmul(0x320000, sin_tbl[stats[bodies[b].stats].heading]);
    dx2 = fixmul(0x320000, cos_tbl[(stats[bodies[b].stats].heading + 0x28) & 0x1f]);
    dy2 = fixmul(0x320000, sin_tbl[(stats[bodies[b].stats].heading + 0x28) & 0x1f]);
    x1 = (short) (((bodies[b].x + dx1) + dx2) >> 16);
    y2 = (short) (((bodies[b].y + dy1) + dy2) >> 16);
    x2 = (short) (((bodies[b].x + dx1) - dx2) >> 16);
    y1 = (short) (((bodies[b].y + dy1) - dy2) >> 16);
    if (abs(bodies[a].z - bodies[b].z) > 0x3c0000)
        return 0;
    if (abs((bodies[a].x >> 16) - (bodies[b].x >> 16))
            + abs((bodies[a].y >> 16) - (bodies[b].y >> 16)) > 300)
        return 0;
    if (!left_of((short) (bodies[a].x >> 16), (short) (bodies[a].y >> 16), x1, y2, x2, y1))
        return 0;
    x3 = (short) (((bodies[b].x - dx1) - dx2) >> 16);
    y3 = (short) (((bodies[b].y - dy1) - dy2) >> 16);
    if (!left_of((short) (bodies[a].x >> 16), (short) (bodies[a].y >> 16), x2, y1, x3, y3))
        return 0;
    x4 = (short) (((bodies[b].x - dx1) + dx2) >> 16);
    y4 = (short) (((bodies[b].y - dy1) + dy2) >> 16);
    if (!left_of((short) (bodies[a].x >> 16), (short) (bodies[a].y >> 16), x3, y3, x4, y4))
        return 0;
    return left_of((short) (bodies[a].x >> 16), (short) (bodies[a].y >> 16), x4, y4, x1, y2);
}

/* ---- 0x142dc -------------------------------------------------------------------------------- */

int on_screen(int x, int y)
{
    if (x < 40 || x >= screen_w - 40)
        return 0;
    if (y < 40 || y >= screen_h - 40)
        return 0;
    return 1;
}


/* ---- 0x1430e -------------------------------------------------------------------------------- */

/* differs: 5 rows (550 vs 550 bytes) - two swapped stack slots and nothing else: the game keeps
   h at [esp+0x14] and x1 at [esp+0x10], ours the other way round (h's store and reload, x1's store
   and two reloads). Prologue, the four products, all five tests, the frame size and the epilogue
   are byte-identical. 71 -> 5: sin_tbl[h] as the second product and (h + 0x28) & 0x1f as the
   third/fourth index (11 rows, the racer_contact convention), test 5's y-check as `bodies[b].y > yl`
   (14 rows: the game loads bodies[b].y into a register once and compares twice), and the corner
   declaration order below. Standalone probes plateaued at 24 rows; it only comes to 5 inside
   racer.o with the natural statement order and the tail y1, x2, y2, x3, y3, x4, x1, y4 (the whole
   file, not hit_test alone, picks the allocation - .work/probe/ht sweeps incl. 3600 declaration
   orders and 1400 with h moved into the tail, which always breaks the frame). h/x1 slot numbering
   is not steerable from C; GOTCHAS dead end. */

/* hit_test(a, b, size) (discoveries.md): 1..5 when one of racer b's four corners, 30 px along
   and across its heading, or its centre lies strictly inside object a's box (bodies[a].x/y +/-
   size in 16.16); 0 when either object is free. Heights are not compared. The four products are
   (30 << 16) x cos_tbl[h], sin_tbl[h], cos_tbl[(h + 0x28) & 0x1f] and sin_tbl[(h + 0x28) & 0x1f]
   - the same +0x28 wrap (32 headings, 8 = a quarter turn) that racer_contact uses; for h >= 24 an
   unmasked cos_tbl[h + 8] would read into sin_tbl, which holds the same values there. */
int hit_test(int a, int b, int size)
{
    int x, xl, xh, yl, yh, h, fx, fy, gx, gy, y1, x2, y2, x3, y3, x4, x1, y4;

    x = size << 16;
    xh = bodies[a].x + x;
    xl = bodies[a].x - x;
    yl = bodies[a].y - x;
    yh = bodies[a].y + x;
    h = stats[bodies[b].stats].heading;
    if (objects[a].period == -1 || objects[b].period == -1)
        return 0;
    fx = fixmul(0x1e0000, cos_tbl[h]);
    fy = fixmul(0x1e0000, sin_tbl[h]);
    gx = fixmul(0x1e0000, cos_tbl[(h + 0x28) & 0x1f]);
    gy = fixmul(0x1e0000, sin_tbl[(h + 0x28) & 0x1f]);
    x1 = bodies[b].x + fx + gx;
    y1 = bodies[b].y + fy + gy;
    x2 = bodies[b].x + fx - gx;
    y2 = bodies[b].y + fy - gy;
    x3 = bodies[b].x - fx - gx;
    y3 = bodies[b].y - fy - gy;
    x4 = bodies[b].x - fx + gx;
    y4 = bodies[b].y - fy + gy;
    if (x1 > xl && x1 < xh && y1 > yl && y1 < yh)
        return 1;
    if (x2 > xl && x2 < xh && y2 > yl && y2 < yh)
        return 2;
    if (x3 > xl && x3 < xh && y3 > yl && y3 < yh)
        return 3;
    if (x4 > xl && x4 < xh && y4 > yl && y4 < yh)
        return 4;
    if (xl < bodies[b].x && bodies[b].x < xh && bodies[b].y > yl && bodies[b].y < yh)
        return 5;
    return 0;
}


/* ---- 0x14534 -------------------------------------------------------------------------------- */

/* Byte-identical 2026-11-23 once these four are one object: 0 instruction rows differed before,
   only the tail jmp's target (we shared an epilogue with the function before on_screen, the game
   jumps into racer_contact's at 0x142d6). Locals' declaration order picks which heading lands in
   EBP: `hb, ha` here. */
void racer_bounce(int a, int b)
{
    int hb, ha, s;

    ha = merge_tbl[stats[bodies[a].stats].heading][stats[bodies[b].stats].heading];
    hb = merge_tbl[stats[bodies[b].stats].heading][stats[bodies[a].stats].heading];
    if (abs(stats[bodies[a].stats].heading - ha) > 8)
        bodies[a].speed = 0;
    else
        stats[bodies[a].stats].heading = ha;
    if (abs(stats[bodies[b].stats].heading - hb) > 8)
        bodies[b].speed = 0;
    else
        stats[bodies[b].stats].heading = hb;
    spawn_puffs(10, a, 100);
    s = (bodies[a].speed + bodies[b].speed) >> 1;
    bodies[a].speed = s;
    bodies[b].speed = s;
}
