/* tick.c: player_crash_tick 0x19497, RedCat's crash tick (the crash ticks that 0x14f3e installs
   when a racer's damage or fuel runs out: player 0x19497, Brutus 0x19c8c in src/crashbull.c,
   Snitch 0x1a42a in src/crashmuis.c). One function per file: the game gives each of the three its
   own epilogue (open question 27 lists the six epilogue shares of the C range and these are not
   among them), so they cannot be one object - Watcom shares an epilogue only inside an object, and
   with all three in one file ours ends each in a jmp to the previous one's.
   The module name is a placeholder: no rc_error or debug_printf call, so no string block and no
   __FILE__ to read it from. */

#include "rrr.h"

/* ---- 0x19497 -------------------------------------------------------------------------------- */

/* RedCat's wreck driving: the only crash tick that reads the keyboard. It refreshes the player's
   per-frame input flags (0x55fd0-0x55fe8, which player_tick 0x11eb5 then acts on), fires the rocket
   and the boost on key edges, and steers through steer_tbl 0x56030 - a 3x3 [vertical][horizontal]
   key matrix holding a heading step, applied twice through turn_tbl. Where the other two crash
   ticks test `stats[].f24` to decide whether to drive at all, this one reads the keys; the go/want
   pair of the other two is still here, but nothing sets it, so the whole `if (go)` arm at 0x19672
   is dead code the game carries anyway. It also asks ground_check what the ground height is and
   only scrapes along when the body is above it. */
/* differs: 276 rows. Register homes (m in a stack slot at [esp+8] and vv at [esp+0xc] in the game,
   we keep m in EBP) plus the go/want body placement of muis_crash_tick 0x1a42a's family; the go/want
   pair is dead code here that the game carries anyway (0x19672-0x196a6). 6 of the old 282 rows came
   from the object: alone in its file, its tail stops jmping into another crash tick's epilogue. */
int player_crash_tick(short n)
{
    short k;
    unsigned want, f;
    int g, vh, d, h, t;
    int vv, m, go;

    key_forward = 0;
    key_back = 0;
    key_right = 0;
    key_left = 0;
    v55fe0 = 0;
    v55fe4 = 0;
    bodies[n].f1c = 0;
    v55fe8 = 0;
    sprites[objects[n].sprite].mode = 1;
    go = 0;
    m = 0x1f;
    vv = 1;
    if (objects[n].period == -1)
        return 0;
    d = fixdiv(0x410000, stats[bodies[n].stats].mass);
    if (lights_done) {
        key_forward = key_pressed(keymap[5]);
        key_back = key_pressed(keymap[4]);
        key_right = key_pressed(keymap[6]);
        key_left = key_pressed(keymap[3]);
        v55fe8 = key_pressed(keymap[1]);
        v55fe4 = key_pressed(keymap[0]);
    }
    if (abs((waypoint0[waypoint_index0].x >> 16) - (bodies[0].x >> 16))
            + abs((waypoint0[waypoint_index0].y >> 16) - (bodies[0].y >> 16)) < 0x78) {
        if (!finished0 && waypoint_count0 - 1 > waypoint_index0)
            waypoint_index0++;
    }
    if (key_forward || key_back || key_left || key_right)
        v55fe0 = 1;
    if (go) {
        switch (want) {
        case 0:
            stats[bodies[n].stats].f24 = 0;
            break;
        case 3:
            stats[bodies[n].stats].f24 = 3;
            bodies[n].speed += d << 4;
            stats[bodies[n].stats].boosts--;

            break;
        }
    } else if (finished0) {
        bodies[n].speed = 0;
        objects[n].frame = 2;
    } else if (!stats[bodies[n].stats].f24) {
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
                    bodies[n].speed += d << 3;
                    v55fcc = 1;
                    bodies[n].vz += d << 4;
                }
            }
        } else
            v55fcc = 0;
        if (v55fe0 && bodies[n].speed <= stats[bodies[n].stats].top_speed) {
            v75174++;
            bodies[n].speed += d;
            stats[bodies[n].stats].fuel -= 0x666;
            if (v75174 > 3)
                v75174 = 0;
        }
        h = 1;
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
            bodies[n].speed = fixmul(bodies[n].speed, 0xe666);
            if (bodies[n].speed < 0x20000) {
                bodies[n].speed = 0;
                objects[n].frame = 2;
            }
        }
    }
    bodies[n].vx = fixmul(bodies[n].speed, cos_tbl[stats[bodies[n].stats].heading]);
    bodies[n].vy = fixmul(bodies[n].speed, sin_tbl[stats[bodies[n].stats].heading]);
    g = ground_check(n);
    bodies[n].z += bodies[n].vz;
    if (bodies[n].vz > -0x140000)
        bodies[n].vz -= fixmul(0x4000, abs(bodies[n].vz)) + 0x4000;
    if (g * 1000 * 0x10000 > bodies[n].z) {
        bodies[n].vx -= fixmul(bodies[n].vx, 0x2666);
        bodies[n].vy -= fixmul(bodies[n].vy, 0x2666);
        bodies[n].z = 0;
        if (abs(bodies[n].vz) > 0x10000)
            bodies[n].vz = -fixmul(0x4ccc, bodies[n].vz);
        else
            bodies[n].vz = 0;
    }
    bodies[n].x += bodies[n].vx;
    bodies[n].y += bodies[n].vy;
    if (racer_contact(n, 4)) {
        if (!v55fc4) {
            racer_bounce(n, 4);
            v55fc4 = 1;
        }
    } else if (racer_contact(n, 2)) {
        if (!v55fc4) {
            racer_bounce(n, 2);
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
    if (bodies[n].z > 0x80000)
        objects[n].anim = 0;
    objects[n].frame = stats[bodies[n].stats].heading + 0x16;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (short)((bodies[n].y >> 16) - (bodies[n].z >> 16)) - frames[k].fa;
    return 0;
}

