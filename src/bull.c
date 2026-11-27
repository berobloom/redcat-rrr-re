/* bull.c: bull_tick 0x12826, Brutus driving normally. Split out of drive.c on 2026-11-27: as one
   object our bull_tick ended by jmping into muis_tick's epilogue, while the game ends it with its own
   (0x130e5 `mov eax,ebx; xor eax,ebx; add esp,0x10; pop x6; ret`). That the game's three normal ticks
   share no epilogue is NOT proof of three objects: WCC shares only identical ones and the game's three
   differ anyway (player_tick 0x1281a `xor eax,eax; add esp,0x10`, bull_tick 0x130ec `xor eax,ebx;
   add esp,0x10`, muis_tick 0x138e1 `xor eax,eax; add esp,0xc`) - ours could join only because our
   bull_tick frame is 0xc where the game's is 0x10 (see src/nearmiss.txt). CONFIRMED is the effect:
   7 rows off player_tick (143 -> 136) and muis_tick at 2036 of the game's 2037 bytes. Module name is a
   placeholder: no rc_error or debug_printf call, so no string block.
   muis_tick 0x130f8 is in src/muis.c for the same reason. */

#include "rrr.h"

/* ---- 0x12826 -------------------------------------------------------------------------------- */

/* Brutus driving normally: bull_crash_tick 0x19c8c's head and AI, muis_tick 0x130f8's shape below,
   and the two things only the living Brutus does - the engine sound play_fx(6, distance to RedCat)
   and the racer-contact test, whose bump sound is play_fx(13, |speed| / 65536 * 10). Boost limit
   0x1c0000, puffs of kind damage, crash2 when damage or fuel runs out.

   differs: 50 rows (was 208 -> 172 -> 133), 2246 of 2258 bytes, nmrefs refs all the same.
   2026-11-26 (.work/probe/btx): the game computes vx and vy TWICE - once unconditionally, then
   again inside `if (!finished2)` with the zero stores in the else (0x12d1e-0x12e06: two
   cos_tbl/sin_tbl fixmul pairs; the other five ticks have one). A redundancy in the game's own
   source, copied: 133 -> 111 rows and +77 bytes. Then `switch (want)`, the racer_contact test with
   its true arm first, `damage < 0 || fuel < 0` as one test (both jump to the game's one
   `crash2 = 1; return 0` block at 0x128a5) and one declaration per line in this order: 50.
   Older notes, 2026-11-25: the heading test is the game's
   arithmetic truth test `if (h - stats[bodies[n].stats].heading && h != -1)` (the game computes the
   difference and tests it - `mov ebx,edx; sub ebx,[eax+@]` at 0x12a3e - not `h != ...`, worth 35 rows
   here, the opposite of muis_tick below), and the go==0 arm's f24 guard is `if (f <= 0)` (unsigned,
   `test` + `ja`). What is left is the same allocation wall as the crash ticks: the game re-derives
   `n` and the bodies[] index per block (the byte shift between the two grows across the function),
   plus the f24/want arm placement (src/nearmiss.txt). */
int bull_tick(short n)
{
    int m;
    unsigned want;
    int d;
    int h;
    short k;
    int go;
    unsigned f;

    bodies[n].f1c = 0;
    sprites[objects[n].sprite].mode = 1;
    go = 0;
    m = 0x1f;
    if (objects[n].period == -1)
        return 0;
    if (stats[bodies[n].stats].damage < 0 || stats[bodies[n].stats].fuel < 0) {
        crash2 = 1;
        return 0;
    }
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
            else
                v55ff8 = 0;
        }
    }
    if (v55ff8 || v55ffc)
        v56008 = 1;
    if (!finished2 && !wait2) {
        f = stats[bodies[n].stats].f24;
        switch (f) {
        case 0:
            if (v5600c && stats[bodies[n].stats].boosts
                    && bodies[n].speed < 0x1c0000) {
                go = 1;
                want = 3;
            }
            break;
        case 3:
            if (!v5600c || bodies[n].speed < 0x1c0000) {
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
            f = stats[bodies[n].stats].f24;
            if (f <= 0) {
                if (v55ff8) {
                    if (bodies[n].speed < stats[bodies[n].stats].top_speed) {
                        if (bodies[n].speed < 0x20000)
                            play_fx(6, abs((bodies[n].x >> 16) - (bodies[0].x >> 16))
                                       + abs((bodies[n].y >> 16) - (bodies[0].y >> 16)));
                        v71918++;
                        bodies[n].speed += d;
                        if (v71918 > 3) {
                            spawn_puffs(1, 2, stats[bodies[n].stats].damage);
                            v71918 = 0;
                        }
                    }
                }
                if (v55ffc) {
                    if (bodies[n].speed > -0x40000)
                        bodies[n].speed -= d;
                }
                if (v56004)
                    stats[bodies[n].stats].heading =
                        (stats[bodies[n].stats].heading + 1) & m;
                if (v56000)
                    stats[bodies[n].stats].heading =
                        (stats[bodies[n].stats].heading + m) & m;
            }
        }
    } else if (wait2)
        ai_waypoint_action(n, waypoint_index2 - 1);
    if (!v56008 || bodies[n].speed > stats[bodies[n].stats].top_speed) {
        bodies[n].speed = fixmul(bodies[n].speed, 0xf851);
        if (bodies[n].speed < 0x3333)
            bodies[n].speed = 0;
    }
    bodies[n].vx = fixmul(bodies[n].speed, cos_tbl[stats[bodies[n].stats].heading]);
    bodies[n].vy = fixmul(bodies[n].speed, sin_tbl[stats[bodies[n].stats].heading]);
    if (!finished2) {
        bodies[n].vx = fixmul(bodies[n].speed, cos_tbl[stats[bodies[n].stats].heading]);
        bodies[n].vy = fixmul(bodies[n].speed, sin_tbl[stats[bodies[n].stats].heading]);
    } else {
        bodies[n].vx = 0;
        bodies[n].vy = 0;
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
    if (racer_contact(n, 4)) {
        if (!v55ff4) {
            racer_bounce(n, 4);
            play_fx(13, (short)(abs(bodies[n].speed) >> 16) * 10);
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
    if (bodies[n].vz > 0x30000) {
        if (bodies[n].vz > 0x40000)
            h = 0x40;
        else
            h = 0x20;
    } else if (bodies[n].vz < -0x50000) {
        if (bodies[n].vz < -0x70000)
            h = 0x80;
        else
            h = 0x60;
    } else
        h = 0;
    objects[n].frame = stats[bodies[n].stats].heading + h;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (short)(bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (short)((bodies[n].y >> 16) - (bodies[n].z >> 16)) - frames[k].fa;
    return 0;
}

