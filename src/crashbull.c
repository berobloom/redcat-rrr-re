/* crashbull.c: bull_crash_tick 0x19c8c, Brutus' wreck driving. Its own object because the game
   gives each crash tick its own epilogue (see the note in src/tick.c). Module name is a
   placeholder: no rc_error or debug_printf call, so no string block and no __FILE__ to read it
   from. */

#include "rrr.h"

/* ---- 0x19c8c -------------------------------------------------------------------------------- */

/* Brutus' wreck driving. Same shape as muis_crash_tick 0x1a42a below with racer 2's globals, a
   0x190000 boost limit instead of 0x170000 and anim set 0x36 instead of 0x46 - but it also has the
   racer-racer contact test the other crash ticks do without (0x55ff4 records that it bounced),
   which is why it is the bigger of the two. */
/* differs: 153 rows (was 185), first-pass C - the logic reads complete, the layout does not. Register homes
   (the game keeps `n` re-derived and reloads bodies[]/stats[] per block, so the byte shift between
   the two grows across the function), the go/want body placement (same out-of-line arms as
   muis_crash_tick 0x1a42a, see its note), and the extra racer_contact/racer_bounce block's argument
   registers. Two spellings here are NOT the ones that suit muis_crash_tick: the mirrored abs()
   operands below (-2 here, +7 there) and the arithmetic heading test `if (h - stats[...].heading
   && h != -1)` (-5 here, +5 there). src/nearmiss.txt. */
int bull_crash_tick(short n)
{
    int go, m, d, h;
    unsigned want, f;
    short k;

    bodies[n].f1c = 0;
    sprites[objects[n].sprite].mode = 1;
    go = 0;
    m = 0x1f;
    if (objects[n].period == -1)
        return 0;
    d = fixdiv(0x1e0000, stats[bodies[n].stats].mass);
    if (lights_done) {
        if (!finished2) {
            if (abs((bodies[2].x >> 16) - (waypoint2[waypoint_index2].x >> 16))
                    + abs((bodies[2].y >> 16) - (waypoint2[waypoint_index2].y >> 16)) < 0x28) {
                ai_waypoint_action(n, waypoint_index2);
                if (!finished2 && waypoint_count2 - 1 > waypoint_index2)
                    waypoint_index2++;
            }
            if (!ai_unstick(n)) {
                h = direction_to_waypoint(2, waypoint2[waypoint_index2].x,
                                          waypoint2[waypoint_index2].y);
                if (h - stats[bodies[n].stats].heading && h != -1)
                    stats[bodies[n].stats].heading =
                        turn_tbl[h][stats[bodies[n].stats].heading];
            }
            if (!finished2)
                v55ff8 = 1;
            else {
                v55ff8 = 0;
                objects[n].frame = 2;
            }
        }
    }
    if (v55ff8 || v55ffc)
        v56008 = 1;
    if (!finished2 && !wait2) {
        f = stats[bodies[n].stats].f24;
        if (f <= 0) {
            if (v5600c && stats[bodies[n].stats].boosts
                    && bodies[n].speed < 0x190000) {
                go = 1;
                want = 3;
            }
        } else if (f == 3) {
            if (!v5600c || bodies[n].speed < 0x190000) {
                go = 1;
                want = 0;
            }
        }
        if (go) {
            if (want > 0) {
                if (want == 3) {
                    bodies[n].speed += d << 4;
                    stats[bodies[n].stats].f24 = want;
                    stats[bodies[n].stats].boosts--;
                }
            } else
                stats[bodies[n].stats].f24 = 0;
        } else {
            f = stats[bodies[n].stats].f24;
            if (f <= 0) {
            if (v55ff8) {
                if (bodies[n].speed < stats[bodies[n].stats].top_speed) {
                    v71918++;
                    bodies[n].speed += d;
                    if (v71918 > 3) {
                        spawn_puffs(1, 2, 100);
                        v71918 = 0;
                    }
                }
            }
            if (v55ffc) {
                if (bodies[n].speed > -0x40000)
                    bodies[n].speed -= d;
            }
            if (v56004)
                stats[bodies[n].stats].heading = (stats[bodies[n].stats].heading + 1) & m;
            if (v56000)
                stats[bodies[n].stats].heading = (stats[bodies[n].stats].heading + m) & m;
            }
        }
    } else if (wait2)
        ai_waypoint_action(n, waypoint_index2 - 1);
    if (!v56008 || bodies[n].speed > stats[bodies[n].stats].top_speed) {
        bodies[n].speed = fixmul(bodies[n].speed, 0xf851);
        if (bodies[n].speed < 0x3333)
            bodies[n].speed = 0;
    }
    if (!finished2) {
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
    if (lights_done && !finished2)
        ai_unstick(n);
    ground_check(n);
    bodies[n].x += bodies[n].vx;
    bodies[n].y += bodies[n].vy;
    if (!racer_contact(n, 4))
        v55ff4 = 0;
    else if (!v55ff4) {
        racer_bounce(n, 4);
        v55ff4 = 1;
    }
    cell_checks(n);
    if (bodies[n].x > 0x18e00000)
        bodies[n].x = 0x18e00000;
    else if (bodies[n].x < 0)
        bodies[n].x = 0;
    if (bodies[n].y > 0x1f180000)
        bodies[n].y = 0x1f180000;
    else if (bodies[n].y < 0)
        bodies[n].y = 0;
    objects[n].anim = (stats[bodies[n].stats].heading >> 1) + 0x36;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

