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
/* differs: 42 rows (was 185 -> 153 -> 128), 1947 of the game's 1950 bytes; nmrefs 147/147 refs
   the same. What took it from 128 to 42 (2026-11-26, .work/probe/bcx), each behaviour-neutral:
   `go = 0;` first, so the game's `xor ebp, ebp` holds go in EBP and serves the f1c store (the
   other crash ticks keep it third); the racer_contact test with its true arm first (the game's
   `je` to `v55ff4 = 0`); both tail coordinates `(short)` (the game's `sar; movsx`); the f24
   field tested directly in the `go == 0` arm, `waypoint_index2 < waypoint_count2 - 1`,
   `-0x140000 < bodies[n].vz`, and one declaration per line in this order. Short x alone costs
   60+ rows: it pays only with the other spellings and a fresh declaration sweep. Left: the
   re-derived `n*40`/`stats*48` index temporaries in the f24 switch, the boost arm and the
   finished2 zeroing block take EAX/ECX/EDX in another order (the same kind of tie as
   muis_crash_tick's last 6), and the `go == 0` arm's f24 test, which the game loads into EDX
   (`mov edx, [..]; test edx, edx`) - the `f = ...; if (f <= 0)` spelling that would do that
   scores 90 under all 5,040 orders. src/nearmiss.txt. */
int bull_crash_tick(short n)
{
    int go;
    unsigned want;
    int m;
    int h;
    short k;
    int d;
    unsigned f;

    go = 0;
    bodies[n].f1c = 0;
    sprites[objects[n].sprite].mode = 1;
    m = 0x1f;
    if (objects[n].period == -1)
        return 0;
    d = fixdiv(0x1e0000, stats[bodies[n].stats].mass);
    if (lights_done) {
        if (!finished2) {
            if (abs((bodies[2].x >> 16) - (waypoint2[waypoint_index2].x >> 16))
                    + abs((bodies[2].y >> 16) - (waypoint2[waypoint_index2].y >> 16)) < 0x28) {
                ai_waypoint_action(n, waypoint_index2);
                if (!finished2 && waypoint_index2 < waypoint_count2 - 1)
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
        switch (f) {
        case 0:
            if (v5600c && stats[bodies[n].stats].boosts
                    && bodies[n].speed < 0x190000) {
                go = 1;
                want = 3;
            }
            break;
        case 3:
            if (!v5600c || bodies[n].speed < 0x190000) {
                go = 1;
                want = 0;
            }
            break;
        }
        if (go) {
            switch (want) {
            case 0:
                stats[bodies[n].stats].f24 = 0;
                break;
            case 3:
                    bodies[n].speed += d << 4;
                    stats[bodies[n].stats].f24 = want;
                    stats[bodies[n].stats].boosts--;
                break;
            }
        } else {
            if (stats[bodies[n].stats].f24 <= 0) {
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
    if (-0x140000 < bodies[n].vz)
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
    if (racer_contact(n, 4)) {
        if (!v55ff4) {
            racer_bounce(n, 4);
            v55ff4 = 1;
        }
    } else
        v55ff4 = 0;
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
    objects[n].x = (short)(bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (short)((bodies[n].y >> 16) - (bodies[n].z >> 16)) - frames[k].fa;
    return 0;
}
