/* map.c: the map cells under a racer, 0x18886-0x1918c ('map.c' guard in cell_effect).
   cell_checks ends in a jump to ground_check's epilogue, so the two are one object; the start at
   fn_18886 is LIKELY (it follows heap.c's last function and is only called from here). */

#include "rrr.h"

/* ---- 0x18886 -------------------------------------------------------------------------------- */

int fn_18886(int v, int lo, int hi)
{
    if (v < lo)
        return -1;
    if (v > hi)
        return 1;
    return 0;
}


/* ---- 0x1889d -------------------------------------------------------------------------------- */

/* differs: 35 rows, register choice (the game keeps cx in EBP and ox on the stack); tried seven
   declaration orders, ox/oy inlined, separate h variables, clamping per branch (this one). */
int ground_check(int n)
{
    int h, cx, cy, ox, oy, d;

    cx = ((bodies[n].x + bodies[n].vx) >> 16) / 32;
    cy = ((bodies[n].y + bodies[n].vy) >> 16) / 40;
    h = map[(cy * map_w + cx) * 6 + 5];
    if (h * 1000 << 16 > bodies[n].z) {
        ox = (bodies[n].x >> 16) / 32;
        h = map[(cy * map_w + ox) * 6 + 5];
        if (h * 1000 << 16 <= bodies[n].z) {
            d = fn_18886(bodies[n].vy, -0x30000, 0x30000);
            if (!key_left && !key_right) {
                if (d == -1)
                    stats[bodies[n].stats].heading = turn_tbl[0][stats[bodies[n].stats].heading];
                else if (d == 1)
                    stats[bodies[n].stats].heading = turn_tbl[16][stats[bodies[n].stats].heading];
            }
            bodies[n].vx = 0;
            if (h < 0)
                h = 0;
            return h;
        } else {
            oy = (bodies[n].y >> 16) / 40;
            h = map[(oy * map_w + cx) * 6 + 5];
            if (h * 1000 << 16 <= bodies[n].z) {
                d = fn_18886(bodies[n].vx, -0x30000, 0x30000);
                if (!key_left && !key_right) {
                    if (d == -1)
                        stats[bodies[n].stats].heading = turn_tbl[8][stats[bodies[n].stats].heading];
                    else if (d == 1)
                        stats[bodies[n].stats].heading = turn_tbl[24][stats[bodies[n].stats].heading];
                }
                bodies[n].vy = 0;
                if (h < 0)
                    h = 0;
                return h;
            } else {
                h = map[(oy * map_w + ox) * 6 + 5];
                bodies[n].vx = 0;
                bodies[n].vy = 0;
                if (h < 0)
                    h = 0;
                return h;
            }
        }
    }
    return h;
}


/* ---- 0x18ae5 -------------------------------------------------------------------------------- */

void cell_checks(int n)
{
    int r, cx, cy;

    r = rand() % 100;
    cx = (bodies[n].x >> 16) / 32;
    cy = (bodies[n].y >> 16) / 40;
    cell_effect(n, cx, cy, r);
    cell_effect(n, cx + 1, cy, r);
    cell_effect(n, cx, cy + 1, r);
    cell_effect(n, cx - 1, cy, r);
    cell_effect(n, cx, cy - 1, r);
}

/* ---- 0x18b8d -------------------------------------------------------------------------------- */

/* Byte-identical 2026-11-23. Two spellings carry it:
   - case 16 repeats case 15's test and both bodies verbatim. The game's case 16 (0x19123) re-tests
     `damage <= 0` / `fuel <= 0` and jumps into case 15's blocks (0x19099 and 0x190cd), so Watcom
     must see the same text: any other test for 16 (>= 1 &&, > 0 &&, !(<= 0 ||), nested ifs, early
     break/return, swapped case order) folds or duplicates the blocks - 87 rows for `>= 1 &&`.
   - the case-22 store reads `map[(x + y * map_w) * 6 + 4]`: operand order picks the SIB pair, the
     game has lea [edi + ebx] (x as base) and `y * map_w + x` gives [ebx + edi] (1 row).
   The empty `case 10` gives the game's switch tree (without it the tree splits at 15). */
void cell_effect(int n, int x, int y, int r)
{
    int type, obj;

    type = map[(y * map_w + x) * 6 + 4];
    if (type < 0 || type > 21) {
#line 244 "map.c"
        rc_error("%s(%d) : Unknown object found in map : %d", __FILE__, __LINE__, type);
        return;
    }
    if (type == 11) {
        if (stats[bodies[n].stats].top_speed <= 0xc0000)
            stats[bodies[n].stats].top_speed += 0x30000;
    } else {
        if (stats[bodies[n].stats].top_speed > 0xc0000)
            stats[bodies[n].stats].top_speed -= 0x30000;
    }
    switch (type) {
    case 13:
        obj = map[(y * map_w + x) * 6 + 1];
        if (bodies[n].z < 0x140000) {
            play_fx(2, abs((bodies[n].x >> 16) - (bodies[obj].x >> 16))
                     + abs((bodies[n].y >> 16) - (bodies[obj].y >> 16)));
            free_object(obj);
            stats[bodies[n].stats].boosts++;
            hud_count(13);
            map[(y * map_w + x) * 6 + 4] = 0;
        }
        break;
    case 22:
        obj = map[(y * map_w + x) * 6 + 1];
        if (bodies[n].z < 0x140000) {
            play_fx(2, abs((bodies[n].x >> 16) - (bodies[obj].x >> 16))
                     + abs((bodies[n].y >> 16) - (bodies[obj].y >> 16)));
            free_object(obj);
            stats[bodies[n].stats].f18++;
            map[(x + y * map_w) * 6 + 4] = 0;
        }
        break;
    case 12:
        obj = map[(y * map_w + x) * 6 + 1];
        if (bodies[n].z < 0x140000 && bodies[obj].f24 <= 0) {
            play_fx(1, abs((bodies[n].x >> 16) - (bodies[obj].x >> 16))
                     + abs((bodies[n].y >> 16) - (bodies[obj].y >> 16)));
            explode_objects(1, obj);
            free_object(obj);
            stats[bodies[n].stats].damage -= 10;
            map[(y * map_w + x) * 6 + 4] = 0;
            bodies[n].vz += fixmul(bodies[n].speed, 0x1999);
            if ((stats[bodies[n].stats].damage <= 0 || stats[bodies[n].stats].fuel <= 0) && n == 0)
                play_fx(23, 0);
        }
        break;
    case 14:
        if (bodies[n].z < 0x140000 && bodies[n].speed > 0x40000) {
            bodies[n].vz += fixmul(bodies[n].speed, 0x1999);
            if (n == 0 && (stats[bodies[0].stats].damage <= 0 || stats[bodies[0].stats].fuel <= 0)) {
                if (r > 20)
                    play_fx(26, 0);
            } else if (n == 2 && (stats[bodies[2].stats].damage <= 0 || stats[bodies[2].stats].fuel <= 0)) {
                if (r > 20)
                    play_fx(27, 0);
            } else if (n == 4 && (stats[bodies[4].stats].damage <= 0 || stats[bodies[2].stats].fuel <= 0)) {
                if (r > 20)
                    play_fx(28, 0);
            }
        }
        break;
    case 15:
        if (stats[bodies[n].stats].damage <= 0 || stats[bodies[n].stats].fuel <= 0) {
            if (n == 0)
                play_fx(23, 0);
            stats[bodies[n].stats].damage -= 2;
        } else if (bodies[n].z < 0x1e0000) {
            bodies[map[(y * map_w + x) * 6 + 1]].f24 = 5;
            stats[bodies[n].stats].damage--;
            map[(y * map_w + x) * 6 + 4] = 0;
            play_fx(25, 0);
        }
        break;
    case 16:
        if (stats[bodies[n].stats].damage <= 0 || stats[bodies[n].stats].fuel <= 0) {
            if (n == 0)
                play_fx(23, 0);
            stats[bodies[n].stats].damage -= 2;
        } else if (bodies[n].z < 0x1e0000) {
            bodies[map[(y * map_w + x) * 6 + 1]].f24 = 5;
            stats[bodies[n].stats].damage--;
            map[(y * map_w + x) * 6 + 4] = 0;
            play_fx(25, 0);
        }
        break;
    case 10:
        break;
    case 20:
        switch (n) {
        case 0:
            finished0 = 1;
            break;
        case 2:
            finished2 = 1;
            break;
        case 4:
            finished4 = 1;
            break;
        }
        break;
    }
}

