/* muis.c: muis_tick 0x130f8, Snitch driving normally. Split out of drive.c on 2026-11-27: as one
   object it ended by jmping into bull_tick's epilogue, and the game ends it with its own (0x138e1
   `xor eax,eax; add esp,0xc; pop x6; ret`) - 2030 of 2037 bytes becomes 2036 of 2037. Whether the
   three normal ticks really are three objects is open (their game epilogues differ anyway, so
   non-sharing alone proves nothing - see the note in src/bull.c). Module name is a placeholder: no
   rc_error or debug_printf call, so no string block. */

#include "rrr.h"

/* ---- 0x130f8 -------------------------------------------------------------------------------- */

/* Snitch driving normally. Roughly three fifths of this is muis_crash_tick 0x1a42a below - the
   waypoint/ai_unstick/direction_to_waypoint block, the v56018-v5602c tick state, the f24/want pair,
   the throttle/brake/turn handling and the z/friction tail - and the rest is what only the living
   have: the damage and fuel test at the head that sets crash4 and hands the racer over to the crash
   tick, the drive sound (play_fx 7 with the distance to RedCat as its volume), puffs whose kind is
   the current damage instead of a fixed 100, no racer-contact test at all, and an anim number picked
   by how fast the body is rising or falling (0x20/0x40 climbing, 0x60/0x80 falling) instead of a
   frame from the heading. The boost limit is 0x1a0000, not Snitch's wreck 0x170000.

   differs: 24 rows, 2036 of 2037 bytes. The frame map already equals the game's (`stackmap.py`: frame
   0xc, slots [esp], [esp+4], [esp+8], no hole); 14 of the 24 rows are homes and the other 10 sit at
   four sites - the heading test (the game's `mov ebx,edx; sub ebx,[eax+@]` at 0x1327b), play_fx's
   first abs (0x13446), the index chain of the turn_tbl store (0x13266-0x1329a) and the tail's y
   difference, where the game keeps the `movsx edx,dx` of 0x138cd that WCC drops for us. All fourteen
   spellings that fix one site cost many rows elsewhere (.work/scratch/muis_tick/gen.py): the
   arithmetic heading test gives 86/95/119 depending on spelling (it shrinks the frame to 8 and moves
   `m` into EBP), the `(short)` cast on the y difference gives 76, and mirroring play_fx's first abs
   gives 29. So the heading test stays `h != ...` and the y line stays uncast: 24 rows is the
   knife-edge where the whole function's allocation lines up. src/nearmiss.txt. */
int muis_tick(short n)
{
    short k;
    int d, m, h, go;
    unsigned want, f;

    bodies[n].f1c = 0;
    sprites[objects[n].sprite].mode = 1;
    go = 0;
    m = 0x1f;
    if (objects[n].period == -1)
        return 0;
    if (stats[bodies[n].stats].damage < 0) {
        crash4 = 1;
        return 0;
    }
    if (stats[bodies[n].stats].fuel < 0) {
        crash4 = 1;
        return 0;
    }
    d = fixdiv(0x1e0000, stats[bodies[n].stats].mass);
    if (lights_done) {
        if (!finished4) {
            if (abs((bodies[4].x >> 16) - (waypoint4[waypoint_index4].x >> 16))
                    + abs((bodies[4].y >> 16) - (waypoint4[waypoint_index4].y >> 16)) < 0x28) {
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
            else
                v56018 = 0;
        }
    }
    if (v56018 || v5601c)
        v56028 = 1;
    if (!finished4 && !wait4) {
        f = stats[bodies[n].stats].f24;
        switch (f) {
        case 0:
            if (v5602c && stats[bodies[n].stats].boosts
                    && bodies[n].speed < 0x1a0000) {
                go = 1;
                want = 3;
            }
            break;
        case 3:
            if (!v5602c || bodies[n].speed < 0x1a0000) {
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
                if (v56018) {
                    if (bodies[n].speed < stats[bodies[n].stats].top_speed) {
                        if (bodies[n].speed < 0x20000)
                            play_fx(7, abs((bodies[0].x >> 16) - (bodies[n].x >> 16))
                                       + abs((bodies[n].y >> 16) - (bodies[0].y >> 16)));
                        v71ddc++;
                        bodies[n].speed += d;
                        if (v71ddc > 3) {
                            spawn_puffs(1, 4, stats[bodies[n].stats].damage);
                            v71ddc = 0;
                        }
                    }
                }
                if (v5601c) {
                    if (bodies[n].speed > -0x40000)
                        bodies[n].speed -= d;
                }
                if (v56024)
                    stats[bodies[n].stats].heading =
                        (stats[bodies[n].stats].heading + 1) & m;
                if (v56020)
                    stats[bodies[n].stats].heading =
                        (stats[bodies[n].stats].heading + m) & m;
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
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

