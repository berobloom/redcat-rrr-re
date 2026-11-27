/* drive.c: player_tick 0x11eb5, RedCat driving. It used to hold bull_tick 0x12826 and muis_tick
   0x130f8 too; since 2026-11-27 those are src/bull.c and src/muis.c (their notes say why and what is
   not proven). Placeholder module name, same reasoning as tick.c. */

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

   differs: 136 rows (143 in one drive.c with the other two ticks - the cross-function tail merge).
   Frame map (stackmap.py): the game's is 0x10 with go at [esp], vv [esp+4], m [esp+8], h [esp+0xc]
   and want in EBP; ours is 0x14 with go [esp+4], want [esp+8], vv [esp+0xc], m [esp+0x10] and h in
   EDI - one slot too many and the same four values in another order, and 71 of the 136 rows are
   homes. No C spelling moves the homes. The game re-derives the bodies[] index per
   block. Rest: the
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
    short k;
    unsigned f, want;
    int vv, m, t, d, go, h, g;

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
    objects[n].x = (short)(bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}

