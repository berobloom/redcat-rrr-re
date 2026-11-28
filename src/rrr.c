/*
 * RRR.EXE, the race: the game's own C, rebuilt function by function.
 *
 * Compiled by src/check.sh with Watcom C/C++ 10.5, `wcc386 -bt=dos -3r -s -ox` (MATCHING.md), and compared
 * with RRR.EXE function by function. Functions are in address order. The original was several
 * modules (sndman.c, main.c, new2io.c, order.c, shade.c, init.c, raceanim.c, heap.c, map.c, ...;
 * research/discoveries.md); a module moves to its own file in src/ once its boundary is known
 * (main.c, order.c, new2io.c, map.c, racer.c, hud.c, ... so far; racer.c and map.c only partly), the rest
 * stays here, so each function is compared on its own. Types,
 * globals and prototypes are in rrr.h; names and layouts are this project's, with their evidence
 * in reverse-engineering/analysis/rrr_names.txt.
 */
#include "rrr.h"

/* ---- 0x138ed camera_follow -----------------------------------------------------------------
 * The view eases 1/16 of the way per frame toward the racer minus the view
 * size plus a lead of 128 px horizontally and 160 px vertically, the lead
 * following the sign of the racer's own velocity; scroll_x/scroll_y are then
 * clamped to 0 .. map size - view size and tile_x/tile_y recomputed.
 * scroll_step_x/scroll_step_y (0x56060, 0x56064) are written here and by
 * fn_13a78 and read nowhere in RRR.EXE.
 *
 * near miss: 62 -> 19 rows. The C is established: vw/vh/tx/ty short (the word
 * loads of view_tiles_w/h are Watcom narrowing the short locals' assignments),
 * x/y int with (short) casts, int vx/vy locals (the game tests registers),
 * both ternaries inline in tx/ty (the (5*sy)<<5 shape of sy*160 says no sy
 * local), statements vw,vh,x,y,vx,vy,tx,ty. Prologue, [eax*8] indexing, x/y
 * normalize-at-def/plain-reload, both easings, both clamps (map_h - vh) * 40
 * and the vh*20 shape then match line for line, and our epilogue jmps the
 * shared turn_tbl_init epilogue like the game's. What is left is Watcom
 * allocation ties, and the declaration order resolves one but never both:
 * one family of orders spills tx to [esp+8] (frame 0xc, x@0, y@4, the game's)
 * but mirrors vx/vy (ours EDX/ESI, the game ESI/EDX; sx, ty and the step
 * registers ripple); the order below assigns vx/vy right but keeps tx in ESI
 * (frame 0x8) and y's reload, the step registers and the map_w clamp ripple.
 * ~200 spellings: 50+ declaration orders (random sweeps, all 55
 * transpositions of the one below, statement interleavings, casts,
 * short/int/unsigned mixes, an unsigned short alias for view_tiles_w/h,
 * vw * 16, sx * 128, (map_w - vw) * 32, renamed locals, an int txs
 * intermediate, sx/sy locals, register keyword, int t). Same class as
 * draw_view 0x11526's homes (watcom-levers "register tie"); take the next
 * line.
 * -------------------------------------------------------------------------------------------- */

void camera_follow(int racer)
{
    int x;
    int sx;
    unsigned t;
    short tx;
    short vh;
    int vx;
    short ty;
    short vw;
    int vy;
    int sy;
    int y;

    vw = view_tiles_w + 1;
    vh = view_tiles_h + 1;
    x = (short)(bodies[racer].x >> 16);
    y = (short)((bodies[racer].y - bodies[racer].z) >> 16);
    vx = bodies[racer].vx;
    vy = bodies[racer].vy;
    tx = (short)(x - (vw << 4) + ((vx < 0 ? -1 : vx > 0) << 7));
    ty = (short)(y - vh * 20 + (vy < 0 ? -1 : vy > 0) * 160);
    scroll_step_x = (tx - scroll_x) >> 4;
    if (tx != scroll_x)
        scroll_x += scroll_step_x;
    scroll_step_y = (ty - scroll_y) >> 4;
    if (ty != scroll_y)
        scroll_y += scroll_step_y;
    if (scroll_x < 0)
        scroll_x = 0;
    t = (map_w - vw) << 5;
    if (scroll_x > t)
        scroll_x = t;
    if (scroll_y < 0)
        scroll_y = 0;
    t = (map_h - vh) * 40;
    if (scroll_y > t)
        scroll_y = t;
    tile_x = scroll_x / 32;
    tile_y = scroll_y / 40;
}

/* ---- 0x13a78 --------------------------------------------------------------------------------
 * camera_follow's twin for a caller that already knows the pixel position: same view size, same
 * 1/16 easing into scroll_x/scroll_y, same clamps and tile_x/tile_y; the lead is 64 px by 80 px
 * instead of 128 by 160, and the caller (fn_13bb8, which draws the view from scroll_x/scroll_y
 * straight after) hands it objects[racer].x/y rather than a body.
 *
 * differs: 15 rows, all register mirrors - the game keeps the vx sign in EAX and shifts it there
 * and puts vw << 4 in EBX (we load vw into EAX and move the sign to EBX), the same mirror for the
 * vy sign (its -1 goes to EBX), and in the map_h clamp it extends vh, still in DX, into EAX and
 * loads map_h into EDX, we the other way round; we end up 1 byte longer. Refs agree (nmrefs), so
 * the C is right and the allocation is Watcom's. Tried: 14 declaration orders of vw/vh/tx/ty/t,
 * the lead as (sign << 6), sign * 64, (sign * 5) << 4, (x - A) + B and (x + B) - A, the two
 * leads as separate statements, a hoisted w = vw << 4, sx/sy locals, the clamps as * 32, * 5 * 8,
 * * 8 * 5, * 40u and with unsigned casts, a separate (map_w - vw) local, vw as int (both spill
 * differently) and this function compiled in an object of its own with camera_follow and
 * fn_13bb8 (camera_follow stays at its 62 rows there, the same mirror). 2026-11-28, 12 more in-module
 * variants (.work/pi/gc/v13.py, control 15): the ground_check CSE lever - `vw = view_tiles_w + 1;` /
 * `vh = view_tiles_h + 1;` written again before the clamp that reads them (68 / 65 / 75), `t` split
 * into two locals (16), the x limit as `* 32` (15, byte-identical), one merged short declaration
 * (15, byte-identical), the declaration order swapped (20), each lead written with its sign term first
 * (21 / 16 / 22), the leads as six plain statements (53), vw/vh as int (50). Nothing below 15, so the
 * three mirrors stay Watcom's.
 * -------------------------------------------------------------------------------------------- */

void fn_13a78(int x, int y, int vx, int vy)
{
    short vw, vh;
    short tx, ty;
    unsigned t;

    vw = view_tiles_w + 1;
    vh = view_tiles_h + 1;
    tx = x - (vw << 4) + ((vx < 0 ? -1 : vx > 0) << 6);
    ty = y - vh * 20 + (vy < 0 ? -1 : vy > 0) * 80;
    scroll_step_x = (tx - scroll_x) >> 4;
    if (tx != scroll_x)
        scroll_x += scroll_step_x;
    scroll_step_y = (ty - scroll_y) >> 4;
    if (ty != scroll_y)
        scroll_y += scroll_step_y;
    if (scroll_x < 0)
        scroll_x = 0;
    t = (map_w - vw) << 5;
    if (scroll_x > t)
        scroll_x = t;
    if (scroll_y < 0)
        scroll_y = 0;
    t = (map_h - vh) * 40;
    if (scroll_y > t)
        scroll_y = t;
    tile_x = scroll_x / 32;
    tile_y = scroll_y / 40;
}

/* ---- 0x13bb8 -------------------------------------------------------------------------------- */

void fn_13bb8(int racer)
{
    objects_tick();
    fn_13a78(objects[racer].x, objects[racer].y, (short)(bodies[racer].vx >> 16), (short)(bodies[racer].vy >> 16));
    draw_view(scroll_y % 40, scroll_x % 32);
}


/* ---- 0x13c34 -------------------------------------------------------------------------------- */

/* MATCH: the game's cos value is not a 4-byte local - it sits at [esp+8], in a 12-byte object below
   the gap that WCC leaves under its scalar band (i [esp+1Ch], a [esp+18h], y [esp+14h]; discoveries
   "Toolchain: how WCC lays out a function's stack frame"). So x goes in element 2 of a local int[3],
   and the angle is written twice instead of kept in a `double t`: WCC CSEs it (the FP instructions are
   the game's) and a `double t` would grow the frame to 0x24. `long v[3]` and a local
   `struct { int x, y, z; }` used as .z give the same 134 bytes - the bytes cannot say which the game
   had. What does NOT work: x as a local, in any of 156 spellings (all 120 declaration orders of
   i, a, y, x, t and 36 statement/declaration combinations stop at 2 rows, x always at [esp+10]). */
void cos_sin_init(void)
{
    int i, y;
    float a;
    int v[3];
    for (a = 0; a < 32; a++) {
        v[2] = cos((a + 8.0) * -0.19634954087500001) * 65536.0;
        i = a;
        y = sin((a + 8.0) * -0.19634954087500001) * 65536.0;
        cos_tbl[i] = v[2];
        sin_tbl[i] = y;
    }
}

/* ---- 0x13cba -------------------------------------------------------------------------------- */

void dir_tbl_init(void)
{
    int r, c, d;
    for (r = 0; r < 33; r++) {
        for (c = 0; c < 33; c++) {
            d = atan2(c + -16.0, r + -16.0) * 5.0929581782756487 + 16.0 + 0.5;
            dir_tbl[r][c] = (32 - d + 24) & 31;
            debug_printf("%2d ", dir_tbl[r][c]);
        }
        debug_printf("\n");
    }
}

/* ---- 0x13d60 -------------------------------------------------------------------------------- */

int direction_to_waypoint(int racer, int x, int y)
{
    int i, j;
    float dy, dx, d;

    dx = bodies[racer].x * (1.0 / 65536) - x * (1.0 / 65536);
    dy = bodies[racer].y * (1.0 / 65536) - y * (1.0 / 65536);
    d = sqrt(dx * dx + dy * dy);
    if (d == 0)
        return 0;
    i = (int)((dx / d + 1) * 16.0) % 33;
    j = (int)((dy / d + 1) * 16.0) % 33;
    return dir_tbl[i][j];
}


/* ---- 0x13e3d -------------------------------------------------------------------------------- */

/* both coordinates are the racer's own, so dx = dy = 0 and this always returns 0 (one fild, fsub st(0)) */
int aim_at_player(int racer, int target)
{
    int i, j;
    float dy, dx, d;

    dx = bodies[racer].x * (1.0 / 65536) - bodies[racer].x * (1.0 / 65536);
    dy = bodies[racer].y * (1.0 / 65536) - bodies[racer].y * (1.0 / 65536);
    d = sqrt(dx * dx + dy * dy);
    if (d == 0)
        return 0;
    i = (int)((dx / d + 1) * 16.0) % 33;
    j = (int)((dy / d + 1) * 16.0) % 33;
    return dir_tbl[i][j];
}


/* ---- 0x13f00 -------------------------------------------------------------------------------- */

void turn_tbl_init(void)
{
    int cur, tgt, d;
    for (cur = 0; cur < 32; cur++) {
        for (tgt = 0; tgt < 32; tgt++) {
            d = tgt - cur;
            if (d == 0)
                turn_tbl[tgt][cur] = cur;
            else if (d > 0) {
                if (d > 16)
                    turn_tbl[tgt][cur] = (cur + 31) & 31;
                else
                    turn_tbl[tgt][cur] = (cur + 1) & 31;
            } else {
                if (d < -16)
                    turn_tbl[tgt][cur] = (cur + 1) & 31;
                else
                    turn_tbl[tgt][cur] = (cur + 31) & 31;
            }
        }
    }
}

/* ---- 0x13f91 -------------------------------------------------------------------------------- */

void merge_tbl_init(void)
{
    int cur, tgt, d;
    for (cur = 0; cur < 32; cur++) {
        for (tgt = 0; tgt < 32; tgt++) {
            d = tgt - cur;
            if (d == 0)
                merge_tbl[tgt][cur] = cur;
            else if (d > 0) {
                if (d > 16)
                    merge_tbl[tgt][cur] = (cur + 1) & 31;
                else
                    merge_tbl[tgt][cur] = (cur + 31) & 31;
            } else {
                if (d < -16)
                    merge_tbl[tgt][cur] = (cur + 31) & 31;
                else
                    merge_tbl[tgt][cur] = (cur + 1) & 31;
            }
        }
    }
}

/* ---- 0x1401d -------------------------------------------------------------------------------- */

/* SKIP: 8 rows, one register mirror. Both sides compute (y1-y3)*(x3-x2) in EDX and
   (x1-x3)*(y3-y2) in EAX and return their difference with setg; the game saves x1 in ESI at entry
   (`mov esi,eax`) and uses EAX for the x3 - x2 temp, ours keeps x1 in EAX and uses ESI for the
   temp. 4 bytes shorter. Tried: one expression in four operand orders, a, b locals, if/return,
   > between the products, compound assignments on y1 and x1 (this one), a copy t = x1, the two
   halves swapped, longhand instead of compound, `y1 > x1`, temporaries for the two products.
   2026-11-29 (.work/pi/left_of/v.py, v2.py, 21 variants scored serially, none below 8; eleven tie
   at 8 with our same 41 bytes): `int t = x1;`/`int t; t = x1;` used for the second product, the
   first product in a local, the second in a local, four locals for the four differences, one
   return expression, `(y1 - y3) * (x3 - x2) - (x1 - x3) * (y3 - y2) > 0` 22, `y1 > x1` after two
   plain products 11, `0 < y1 - x1`, parenthesised longhand, `register int x1` on the first
   parameter, the multiplication operands flipped in each product, x3 and y3 held in locals, the
   subtractions flipped (`x1 = x3 - x1; x1 *= y2 - y3;`) 10, and the four statements in three
   interleavings - `x1 -= x3;` second (8, same bytes), third (14) and last (14). What the mirror
   costs, byte for byte (.work/pi/lo/dump.py): the game moves x1 out of its home register at entry
   (`mov esi,eax` 0x1401e) and moves it back for the second multiply (`mov eax,esi` 0x14037), where
   we keep x1 in EAX and give ESI to the x3 temp; that is the whole 4-byte gap. CONFIRMED on our own
   compile: the 3rd parameter sits in EBX and the 4th in ECX here (first product reads EBX, second
   ECX) - the same as the game's, so the C's parameter mapping is the game's. */
int left_of(int x1, int y1, int x2, int y2, int x3, int y3)
{
    y1 -= y3;
    y1 *= x3 - x2;
    x1 -= x3;
    x1 *= y3 - y2;
    return y1 - x1 > 0;
}

/* ---- 0x1467c -------------------------------------------------------------------------------- */

int ai_unstick(int racer)
{
    if (bodies[racer].vx == 0 && bodies[racer].vy == 0) {
        if (racer == 2) {
            stuck2++;
            if (stuck2 > 20) {
                stats[bodies[racer].stats].heading = (rand() % 40 + stats[bodies[racer].stats].heading + 8) & 31;
                stuck2 = 0;
                return 1;
            }
        } else if (racer == 4) {
            stuck4++;
            if (stuck4 > 20) {
                stats[bodies[racer].stats].heading = (rand() % 40 + stats[bodies[racer].stats].heading + 8) & 31;
                stuck4 = 0;
                return 1;
            }
        }
    }
    return 0;
}

/* ---- 0x1475f -------------------------------------------------------------------------------- */

void camera_start(int racer)
{
    scroll_x = (short)(bodies[racer].x >> 16) - view_tiles_w * 32 / 2;
    if (scroll_x < 0)
        scroll_x = 0;
    if ((map_w - (view_tiles_w + 1)) * 32 < scroll_x)
        scroll_x = (map_w - (view_tiles_w + 1)) * 32;
    scroll_y = (short)(bodies[racer].y >> 16) - view_tiles_h * 40 / 2;
    if (scroll_y < 0)
        scroll_y = 0;
    if ((map_h - (view_tiles_h + 1)) * 40 < scroll_y)
        scroll_y = (map_h - (view_tiles_h + 1)) * 40;
    tile_x = scroll_x / 32;
    tile_y = scroll_y / 40;
}

/* ---- 0x14861 -------------------------------------------------------------------------------- */

void clear_objects(void)
{
    int i;
    for (i = 0; i < num_objects; i++) {
        bodies[i].f1c = 0;
        objects[i].period = -1;
        objects[i].sprite = i;
        sprites[i].x = sprites[i].y = 4000;
        objects[i].anim = 0;
        objects[i].frame = 0;
    }
}
