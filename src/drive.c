/* drive.c: the three racers' normal per-frame callbacks, 0x11eb5-0x138ec (player_tick, bull_tick,
   muis_tick). Placeholder module name, same reasoning as tick.c. The split from the crash ticks is
   because the game lays the three out back to back at 0x11eb5/0x12826/0x130f8 and the three crash
   ticks back to back at 0x19497/0x19c8c/0x1a42a, with other modules in between. */

#include "rrr.h"

/* ---- 0x11eb5 -------------------------------------------------------------------------------- */

/* RedCat driving: the only tick that reads the keyboard, so it is player_crash_tick 0x19497 below
   with the AI blocks swapped for input. It refreshes the six per-frame key flags, advances
   waypoint_index0 when the body nears its waypoint, then on key edges fires the rocket
   (fire_rocket 0x185d4, one per key press) and the boost (+speed and +vz, one boost item), drives
   while any steering key is held, and steers through steer_tbl 0x56030 applied twice through
   turn_tbl. Its puff counter is keymap[2] 0x71454, the keymap slot no keyboard action reads.
   Unlike the other ticks it asks ground_check for the ground height and only scrapes along when the
   body is under it, its landing sound is play_fx(21, ...) when the bounce is hard, and hitting a
   racer plays play_fx(13, ...) plus - one time in ten against racer 4, one in five against racer 2
   - play_fx(4, 0). Its go/want pair is dead code like the crash tick's, and the game's own oddity is
   copied with it: the second bounce test compares rand() % 100 with 20 at 0x12696 and then jumps
   into racer 4's comparison with 10, so the 20 is dead.

   differs: 193 -> 189 rows, and 71 of them are register homes (nmclass.py): the game's frame is 0x10
   with go at [esp], vv [esp+4], m [esp+8], h [esp+0xc] and want in EBP, ours is 0x14 with go [esp+4],
   want [esp+8], vv [esp+0xc], m [esp+0x10] and h in EDI, and the game re-derives the bodies[] index per
   block. No C spelling moves the homes (the drive.c/tick.c module split is already in). Rest: the
   f24/want arm placement (test + `jbe else`, `cmp 3` + `je body` at 0x120cd, the body out of line - the
   same one decision that costs 22 rows in muis_crash_tick) and the bounce-test chain.
   2026-11-28: the waypoint-distance abs() pair wants its operands mirrored like bull_crash_tick's -
   `abs((bodies[0].x >> 16) - (waypoint0[waypoint_index0].x >> 16))` (0x1202f loads bodies[0].x first,
   0x12035 waypoint0[] second, `sub edx,eax`): -4 rows, the same word mirrored costs +7 in
   muis_crash_tick. Nothing else moved: chained zero stores for the six key flags, `bodies[n].f1c = 0`
   in the game's place (0x11f02, after v55fe8), clearing all seven in one chain, both spellings of the
   `want` chain (`want > 0 && want != 3` then `else if (want == 3)`, and `want == 3` first: 200 and 194),
   and four spellings of the friction test - the game's `imul eax,edi,0x3e8` + `shl eax,0x10` against our
   WCC shift/sub chain for the folded 65536000 (`shl 5; sub; shl 2; add; shl 0x13`, the only nmrefs
   constant difference in the function); `q = g * 1000` as its own local costs 30+ because it adds a
   stack slot to an already full frame. */
int player_tick(short n)
{
    unsigned want, f;
    short k;
    int g, m, h, vv, go, t, d;

    key_forward = 0;
    key_back = 0;
    key_right = 0;
    key_left = 0;
    v55fe0 = 0;
    v55fe4 = 0;
    bodies[n].f1c = 0;
    v55fe8 = 0;
    sprites[objects[n].sprite].mode = 1;
    m = 0x1f;
    h = 1;
    vv = 1;
    go = 0;
    if (objects[n].period == -1)
        return 0;
    if (stats[bodies[n].stats].damage < 0) {
        crash0 = 1;
        return 0;
    }
    if (stats[bodies[n].stats].fuel < 0) {
        crash0 = 1;
        return 0;
    }
    d = fixdiv(0x1e0000, stats[bodies[n].stats].mass);
    if (lights_done) {
        key_forward = key_pressed(keymap[5]);
        key_back = key_pressed(keymap[4]);
        key_right = key_pressed(keymap[6]);
        key_left = key_pressed(keymap[3]);
        v55fe8 = key_pressed(keymap[1]);
        v55fe4 = key_pressed(keymap[0]);
    }
    if (key_forward || key_back || key_left || key_right)
        v55fe0 = 1;
    if (abs((bodies[0].x >> 16) - (waypoint0[waypoint_index0].x >> 16))
            + abs((bodies[0].y >> 16) - (waypoint0[waypoint_index0].y >> 16)) < 0x78) {
        if (!finished0 && waypoint_count0 - 1 > waypoint_index0)
            waypoint_index0++;
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
    } else if (!finished0) {
        f = stats[bodies[n].stats].f24;
        if (f <= 0) {
            if (v55fe4) {
                if (!v55fc8) {
                    v55fc8 = 1;
                    if (stats[bodies[n].stats].rockets) {
                        fire_rocket(n);
                        stats[bodies[n].stats].rockets--;
                        play_fx(20, 0);
                    }
                }
            } else
                v55fc8 = 0;
            if (v55fe8) {
                if (!v55fcc) {
                    if (bodies[n].z < 0x280000) {
                        v55fcc = 1;
                        if (stats[bodies[n].stats].boosts) {
                            bodies[n].speed += d << 4;
                            bodies[n].vz += d << 6;
                            play_fx(16, 0);
                            stats[bodies[n].stats].boosts--;
                        }
                    }
                }
            } else
                v55fcc = 0;
            if (v55fe0) {
                if (bodies[n].speed <= stats[bodies[n].stats].top_speed) {
                    if (bodies[n].speed < 0x3333)
                        play_fx(15, 0);
                    bodies[n].speed += d;
                    keymap[2]++;
                    stats[bodies[n].stats].fuel -= 0x666;
                    if (keymap[2] > 3) {
                        spawn_puffs(1, 0, stats[bodies[n].stats].damage);
                        keymap[2] = 0;
                    }
                } else if (bodies[n].speed > stats[bodies[n].stats].top_speed + 0x40000)
                    spawn_trail(1, 0);
            }
            if (key_left)
                h = 0;
            if (key_right)
                h = 2;
            if (key_forward)
                vv = 0;
            if (key_back)
                vv = 2;
            t = steer_tbl[vv][h];
            if (t != -1) {
                h = stats[bodies[n].stats].heading;
                h = turn_tbl[t & m][h] & m;
                stats[bodies[n].stats].heading = h;
                h = turn_tbl[t & m][h] & m;
                stats[bodies[n].stats].heading = h;
            }
            if (!v55fe0 || bodies[n].speed > stats[bodies[n].stats].top_speed) {
                bodies[n].speed = fixmul(bodies[n].speed, 0xf851);
                if (bodies[n].speed < 0x3333)
                    bodies[n].speed = 0;
            }
        }
    }
    if (!finished0) {
        bodies[n].vx = fixmul(bodies[n].speed, cos_tbl[stats[bodies[n].stats].heading]);
        bodies[n].vy = fixmul(bodies[n].speed, sin_tbl[stats[bodies[n].stats].heading]);
    } else {
        bodies[n].vx = 0;
        bodies[n].vy = 0;
    }
    g = ground_check(n);
    bodies[n].z += bodies[n].vz;
    if (bodies[n].vz > -0x140000)
        bodies[n].vz -= fixmul(0x4000, abs(bodies[n].vz)) + 0x4000;
    if (g * 1000 * 0x10000 > bodies[n].z) {
        bodies[n].vx -= fixmul(bodies[n].vx, 0x2666);
        bodies[n].vy -= fixmul(bodies[n].vy, 0x2666);
        bodies[n].z = 0;
        if (abs(bodies[n].vz) > 0x10000) {
            bodies[n].vz = -fixmul(0x4ccc, bodies[n].vz);
            if (abs(bodies[n].vz) > 0x20000)
                play_fx(21, (short)(abs(bodies[n].vz) >> 16) * 10);
        } else
            bodies[n].vz = 0;
    }
    bodies[n].x += bodies[n].vx;
    bodies[n].y += bodies[n].vy;
    if (racer_contact(n, 4)) {
        if (!v55fc4) {
            racer_bounce(n, 4);
            play_fx(13, (short)(abs(bodies[n].speed) >> 16) * 10);
            if (rand() % 100 < 10)
                play_fx(4, 0);
            v55fc4 = 1;
        }
    } else if (racer_contact(n, 2)) {
        if (!v55fc4) {
            racer_bounce(n, 2);
            play_fx(13, (short)(abs(bodies[n].speed) >> 16) * 10);
            if (rand() % 100 < 20)
                play_fx(4, 0);
            v55fc4 = 1;
        }
    } else
        v55fc4 = 0;
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
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x12826 -------------------------------------------------------------------------------- */

/* Brutus driving normally: bull_crash_tick 0x19c8c's head and AI, muis_tick 0x130f8's shape below,
   and the two things only the living Brutus does - the engine sound play_fx(6, distance to RedCat)
   and the racer-contact test, whose bump sound is play_fx(13, |speed| / 65536 * 10). Boost limit
   0x1c0000, puffs of kind damage, crash2 when damage or fuel runs out.

   differs: 172 rows (was 208). Two spellings found 2026-11-25: the heading test is the game's
   arithmetic truth test `if (h - stats[bodies[n].stats].heading && h != -1)` (the game computes the
   difference and tests it - `mov ebx,edx; sub ebx,[eax+@]` at 0x12a3e - not `h != ...`, worth 35 rows
   here, the opposite of muis_tick below), and the go==0 arm's f24 guard is `if (f <= 0)` (unsigned,
   `test` + `ja`). What is left is the same allocation wall as the crash ticks: the game re-derives
   `n` and the bodies[] index per block (the byte shift between the two grows across the function),
   plus the f24/want arm placement (src/nearmiss.txt). */
int bull_tick(short n)
{
    unsigned f, want;
    int m, d, h, go;
    short k;

    bodies[n].f1c = 0;
    sprites[objects[n].sprite].mode = 1;
    go = 0;
    m = 0x1f;
    if (objects[n].period == -1)
        return 0;
    if (stats[bodies[n].stats].damage < 0) {
        crash2 = 1;
        return 0;
    }
    if (stats[bodies[n].stats].fuel < 0) {
        crash2 = 1;
        return 0;
    }
    d = fixdiv(0x1e0000, stats[bodies[n].stats].mass);
    if (lights_done) {
        if (!finished2) {
            if (abs((waypoint2[waypoint_index2].x >> 16) - (bodies[2].x >> 16))
                    + abs((waypoint2[waypoint_index2].y >> 16) - (bodies[2].y >> 16)) < 0x28) {
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
        if (f <= 0) {
            if (v5600c && stats[bodies[n].stats].boosts
                    && bodies[n].speed < 0x1c0000) {
                go = 1;
                want = 3;
            }
        } else if (f == 3) {
            if (!v5600c || bodies[n].speed < 0x1c0000) {
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
    if (!racer_contact(n, 4))
        v55ff4 = 0;
    else if (!v55ff4) {
        racer_bounce(n, 4);
        play_fx(13, (short)(abs(bodies[n].speed) >> 16) * 10);
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
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

/* ---- 0x130f8 -------------------------------------------------------------------------------- */

/* Snitch driving normally. Roughly three fifths of this is muis_crash_tick 0x1a42a below - the
   waypoint/ai_unstick/direction_to_waypoint block, the v56018-v5602c tick state, the f24/want pair,
   the throttle/brake/turn handling and the z/friction tail - and the rest is what only the living
   have: the damage and fuel test at the head that sets crash4 and hands the racer over to the crash
   tick, the drive sound (play_fx 7 with the distance to RedCat as its volume), puffs whose kind is
   the current damage instead of a fixed 100, no racer-contact test at all, and an anim number picked
   by how fast the body is rising or falling (0x20/0x40 climbing, 0x60/0x80 falling) instead of a
   frame from the heading. The boost limit is 0x1a0000, not Snitch's wreck 0x170000.

   differs: 86 rows (was 87). Here the heading test must stay `if (h != stats[...].heading &&
   h != -1)`: the game's `mov ebx,edx; sub ebx,[eax+@]` at 0x1327b does come out of `h - ...` and that
   then matches the game's size to the byte (2037 = 2037), but 86 -> 158 rows, because the spelling
   moves the f24/want arm placement that costs 22 rows in muis_crash_tick too. bull_tick 0x12826 above
   wants the other spelling (-36 there). Rest: the f24/want chain arms (both out of line in the game,
   fall-through in ours), the distance-to-RedCat pair spilled through [esp] in the game where we keep
   it in a register, and the want<=0 arm. src/nearmiss.txt. */
int muis_tick(short n)
{
    short k;
    unsigned f, want;
    int m, d, go, h;

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
            else
                v56018 = 0;
        }
    }
    if (v56018 || v5601c)
        v56028 = 1;
    if (!finished4 && !wait4) {
        f = stats[bodies[n].stats].f24;
        if (f <= 0) {
            if (v5602c && stats[bodies[n].stats].boosts
                    && bodies[n].speed < 0x1a0000) {
                go = 1;
                want = 3;
            }
        } else if (f == 3) {
            if (!v5602c || bodies[n].speed < 0x1a0000) {
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
                if (v56018) {
                    if (bodies[n].speed < stats[bodies[n].stats].top_speed) {
                        if (bodies[n].speed < 0x20000)
                            play_fx(7, abs((bodies[n].x >> 16) - (bodies[0].x >> 16))
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
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

