/* crashmuis.c: muis_crash_tick 0x1a42a, Snitch's wreck driving. Its own object because the game
   gives each crash tick its own epilogue (see the note in src/tick.c). Module name is a
   placeholder: no rc_error or debug_printf call, so no string block and no __FILE__ to read it
   from. */

#include "rrr.h"

/* ---- 0x1a42a -------------------------------------------------------------------------------- */

/* Snitch's wreck driving: the crash tick that 0x14f3e installs when his damage runs out. Written
   from the disassembly; every racer has its own copy (player 0x19497, Brutus 0x19c8c), each about
   1900 bytes, and they are not the same C: the three instruction streams share under half their
   instructions. */
/* 2026-11-26 (latest): 40 -> 32. The tail truncates both coordinates to short as the game does
   (movsx at 0x1ab26 and 0x1ab5a): `(short)(bodies[n].x >> 16)` and `(short)((bodies[n].y >> 16) -
   (bodies[n].z >> 16))`, with the declarations `short k; unsigned f, want; int d, h, go, m;`. The
   same behaviour: x >> 16 is clamped to 0x18e0 and both results are stored to short fields. One
   cast alone costs (61-68 rows); abs() written body - waypoint, as the game subtracts at
   0x1a4e8/0x1a50b, costs 5, and `h - heading` 35 (16 forms x 61 declaration orders,
   .work/probe/mc9).
   Earlier: 46 -> 40. The f24 chain is a switch: the game's `test; jbe A; cmp 3; je B;
   jmp join` with both arms out of line (0x1a63b-0x1a644) is how WCC lowers `switch (f) { case 0:
   ... case 3: ... }` on an unsigned f, and that block now matches. The want chain is the same
   kind of switch (0x1a699-0x1a6a2), and written as one it matches too, but it costs elsewhere (42
   rows at best: the want == 3 body's bodies/stats registers mirror, 12 body orders x 288
   declaration orders, .work/probe/mc7-mc8), so it stays the if/else-if here.
   Earlier the same day: 67 -> 46 rows with two text changes found in place (inplace.py, .work/probe/mc2-mc6):
   the declarations `short k; int go, d, m, h; unsigned want, f;` (16 of 576 orders x chain
   spellings reach 46; the old order gave go/m each other's stack slot once the chains moved) and
   the want chain as `if (!want) f24 = 0; else if (want == 3) {...}` - want is unsigned, so this is
   the old `want > 0` test negated. The arithmetic heading test `h - heading` does give the game's
   `sub` at 0x1a589 but costs 1 row overall. Left: the f24 and want chains' out-of-line arms (the
   f24 spellings nest/eq3first/!f/switch: 46-71), the sprites base in EBP, the tail's registers.
   Earlier: */
/* differs: 67 rows, one block-placement decision plus register homes. The game puts both arms of
   the f24 chain and of the want chain out of line (test + jbe arm, cmp 3 + je body, jmp join at
   0x1a63d-0x1a644 and 0x1a699-0x1a6a2) and keeps both bodies as jump targets; ours puts the bodies
   in the fall-through slot. 36 spellings of the two chains (11 x 5 minus duplicates, solo-compiled,
   .work/probe/mct/sweep.py) do not move it; nor do five (short) casts that try to force the game's
   movsx truncation of (bodies[].x >> 16) in the tail. What the game also has and we do not: the
   sprites base in EBP across the function, the turn_tbl result in ECX, and one more register in the
   epilogue. src/nearmiss.txt (SKIP). */
int muis_crash_tick(short n)
{
    unsigned f, want;
    short k;
    int d, h, go, m;

    bodies[n].f1c = 0;
    sprites[objects[n].sprite].mode = 1;
    go = 0;
    m = 0x1f;
    if (objects[n].period == -1)
        return 0;
    d = fixdiv(0x1e0000, stats[bodies[n].stats].mass);
    if (lights_done) {
        if (!finished4) {
            if (abs((waypoint4[waypoint_index4].x >> 16) - (bodies[4].x >> 16))
                    + abs((waypoint4[waypoint_index4].y >> 16) - (bodies[4].y >> 16)) < 0x28) {
                ai_waypoint_action(n, waypoint_index4);
                if (!finished4 && waypoint_count4 - 1 > waypoint_index4)
                    waypoint_index4++;
            }
            if (!ai_unstick(n)) {
                h = direction_to_waypoint(4, waypoint4[waypoint_index4].x,
                                          waypoint4[waypoint_index4].y);
                if (h != stats[bodies[n].stats].heading && h != -1)
                    stats[bodies[n].stats].heading =
                        turn_tbl[h][stats[bodies[n].stats].heading];
            }
            if (!finished4)
                v56018 = 1;
            else {
                v56018 = 0;
                objects[n].frame = 2;
            }
        }
    }
    if (v56018 || v5601c)
        v56028 = 1;
    if (!finished4 && !wait4) {
        f = stats[bodies[n].stats].f24;
        switch (f) {
        case 0:
            if (v5602c && stats[bodies[n].stats].boosts
                    && bodies[n].speed < 0x170000) {
                go = 1;
                want = 3;
            }
            break;
        case 3:
            if (!v5602c || bodies[n].speed < 0x170000) {
                go = 1;
                want = 0;
            }
            break;
        }
        if (go) {
            if (!want)
                stats[bodies[n].stats].f24 = 0;
            else if (want == 3) {
                bodies[n].speed += d << 4;
                stats[bodies[n].stats].f24 = want;
                stats[bodies[n].stats].boosts--;
            }
        } else {
            f = stats[bodies[n].stats].f24;
            if (f <= 0) {
            if (v56018) {
                if (bodies[n].speed < stats[bodies[n].stats].top_speed) {
                    v71ddc++;
                    bodies[n].speed += d;
                    if (v71ddc > 3) {
                        spawn_puffs(1, 4, 100);
                        v71ddc = 0;
                    }
                }
            }
            if (v5601c) {
                if (bodies[n].speed > -0x40000)
                    bodies[n].speed -= d;
            }
            if (v56024)
                stats[bodies[n].stats].heading = (stats[bodies[n].stats].heading + 1) & m;
            if (v56020)
                stats[bodies[n].stats].heading = (stats[bodies[n].stats].heading + m) & m;
            }
        }
    } else if (wait4)
        ai_waypoint_action(n, waypoint_index4 - 1);
    if (!v56028 || bodies[n].speed > stats[bodies[n].stats].top_speed) {
        bodies[n].speed = fixmul(bodies[n].speed, 0xf851);
        if (bodies[n].speed < 0x3333)
            bodies[n].speed = 0;
    }
    if (!finished4) {
        bodies[n].vx = fixmul(bodies[n].speed, cos_tbl[stats[bodies[n].stats].heading]);
        bodies[n].vy = fixmul(bodies[n].speed, sin_tbl[stats[bodies[n].stats].heading]);
    } else {
        bodies[n].vx = 0;
        bodies[n].vy = 0;
        bodies[n].speed = 0;
        objects[n].frame = 2;
    }
    bodies[n].z += bodies[n].vz;
    if (bodies[n].vz > -0x140000)
        bodies[n].vz -= fixmul(0x4000, abs(bodies[n].vz)) + 0x4000;
    if (bodies[n].z < 0) {
        bodies[n].vx -= fixmul(bodies[n].vx, 0x2666);
        bodies[n].vy -= fixmul(bodies[n].vy, 0x2666);
        bodies[n].z = 0;
        if (abs(bodies[n].vz) > 0x10000)
            bodies[n].vz = -fixmul(0x4ccc, bodies[n].vz);
        else
            bodies[n].vz = 0;
    }
    if (lights_done && !finished4)
        ai_unstick(n);
    ground_check(n);
    bodies[n].x += bodies[n].vx;
    bodies[n].y += bodies[n].vy;
    cell_checks(n);
    if (bodies[n].x > 0x18e00000)
        bodies[n].x = 0x18e00000;
    else if (bodies[n].x < 0)
        bodies[n].x = 0;
    if (bodies[n].y > 0x1f180000)
        bodies[n].y = 0x1f180000;
    else if (bodies[n].y < 0)
        bodies[n].y = 0;
    objects[n].anim = (stats[bodies[n].stats].heading >> 1) + 0x46;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (short)(bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (short)((bodies[n].y >> 16) - (bodies[n].z >> 16)) - frames[k].fa;
    return 0;
}
