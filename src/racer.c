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
   h at [esp+0x10] (0x14371) and x1 at [esp+0x14] (0x1440b), ours the other way round (h's store and reload, x1's store
   and two reloads). Prologue, the four products, all five tests, the frame size and the epilogue
   are byte-identical. 71 -> 5: sin_tbl[h] as the second product and (h + 0x28) & 0x1f as the
   third/fourth index (11 rows, the racer_contact convention), test 5's y-check as `bodies[b].y > yl`
   (14 rows: the game loads bodies[b].y into a register once and compares twice), and the corner
   declaration order below. Standalone probes plateaued at 24 rows; it only comes to 5 inside
   racer.o with the natural statement order and the tail y1, x2, y2, x3, y3, x4, x1, y4 (the whole
   file, not hit_test alone, picks the allocation - .work/probe/ht sweeps incl. 3600 declaration
   orders and 1400 with h moved into the tail, which always breaks the frame). h/x1 slot numbering
   is not steerable from C; GOTCHAS dead end.
   2026-11-26, the start_lights lever and others, all >= 5 rows: merging locals whose lifetimes do
   not overlap (size/x/h/a hosting a later one, singles and pairs, 838 files, ht3); declaring
   subsets in an inner block from h, fx or x1 on (2021, ht4); x1, y1 or h inline (ht5); extra
   holders for bodies[b].x/.y or (h + 0x28) & 0x1f and a dead local (ht6); racer.c compiled with
   up to eight of rrr.c's preceding and three following functions (36, ht7: always 5); h/x1 types
   (ht8); the 384 corner orders (ht9: only the natural one is 5); the four tests' operand orders
   (625, hta).  2026-11-28 round 2 (.work/pi/ht2/v.py, v3.py, v5.py, 22 variants): `register` is
   INERT in this compiler - `register int h`, `register int x1`, both together, and `register` on
   each of the three parameters all compile to the control's exact bytes with the same two slots
   (CONFIRMED: fndiff's row text identical). `x2 = (bodies[b].x + fx) - gx`, `y1 = (bodies[b].y +
   fy) + gy`, all four corners parenthesised, the two period tests as two `if`s and test 5's x side
   as `bodies[b].x > xl` hold 5 rows byte-identical; `x1 = bodies[b].x + (fx + gx)` is 110, so the
   game really does (x + fx) + gx; moving `h = ...` between the box lines or after `x = size << 16`
   is 104/136. Band fact (CONFIRMED, same probe): the local band and the temporary band are
   separate - declaring `h` LAST moves its slot to [esp+0xc] (temps' top) at 75 rows, declaring it
   just before `x1` costs `x1` its slot (it stays in a register) at 84 - and none of the ten further
   declaration orders tried here lands the game's h/x1 numbering either.  2026-11-28 vbatch
   (.work/pi/d2/v3.py): `h` read after both free tests 147, the
   heading load moved in front of the box lines 136, `bodies[b].x + (fx + gx)` 141, `h` declared last
   75 - so the C's statement order is the game's (the game stores `h` at 0x14371, before the two
   period tests at 0x14384) - while `xl` before `xh`, `x` inlined into the four box lines, and the
   declaration split after `gy` all hold 5 rows without moving the two slots. */

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
