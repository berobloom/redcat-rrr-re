/* init.c: the map-cell setup of the race: the computer racers' waypoint path (0x1517a) and the
   bonus objects in the cells (0x15597). CONFIRMED name: both functions pass __FILE__ "init.c" to
   check_waypoint_nr (0x1b69d) and rc_error, with the game's own line numbers 398..557.
   STRONGLY SUPPORTED boundary: the game shares one epilogue between find_waypoints and
   find_bonus_objects (find_bonus_objects' last jmp 0x15665 goes to find_waypoints' pop/ret at
   0x15565), and Watcom shares an epilogue only inside one object - so the two are one module
   (MATCHING.md 4, open question 27). */

#include "rrr.h"

/* ---- 0x1517a find_waypoints ---------------------------------------------------------------
 * The map's cells carry the computer racers' waypoint path: cell +2 = waypoint
 * number, +3 = which racer (1 RedCat, 2 Brutus, 4 Snitch; 3 = none). Each
 * racer's list is indexed by waypoint number and holds its centre in 16.16:
 * x = (i % map_w) * 32 + 16, y = (i / map_w) * 40 + 20 (Snitch: +16, as the
 * port's log says). The lists are filled with -1 first, and afterwards every
 * number up to 150 must be used in order (an empty entry followed by a used
 * one is a hole -> "Missing waypoint", return -1). init.c lines 398..456.
 *
 * The wayp.nr value: an `unsigned short w` and no n local, the number written
 * `(w & 0xff)` wherever it is used. With an int w and an n local Watcom keeps
 * four 32-bit values live, spills the shifted x/y to [esp], keeps n*8 instead
 * of indexing with a scale-8 SIB, and compares the hole-check entries against
 * the immediate -1 instead of hoisting -1 into EBX: 190 rows. This spelling
 * keeps BX as the 16-bit holder (its load is `mov bx, word [...]`, its mask
 * `xor bh, bh`), ESI for the int copy and EBP for the shifted value: 0 rows.
 *
 * differs: no instruction row is left - bytes +0..+1009 (0x1517a-0x1556b, the
 * ret) are identical. What differs is the block after it, find_bonus_objects'
 * ten-entry jump table: the game pads it with three bytes (`lea eax,[eax]`) at
 * 0x1556c and we with two (`mov eax,eax`) at +1010, so we are one byte short
 * and its table lands at +1012 instead of +1013. Watcom aligns a jump table to
 * a 4-byte boundary inside its own object, so the pad length follows from what
 * the object holds before find_waypoints: fn_15166 (20 B, 0 mod 4) in front of
 * it keeps the two-byte pad, init_objects 0x148db (1635 B, 3 mod 4) would give
 * three. That is the module question below, not a C spelling.
 * -------------------------------------------------------------------------------------------- */

int find_waypoints(void)
{
    int i;
    unsigned short w;

    waypoint_count0 = 0;
    waypoint_count4 = 0;
    waypoint_count2 = 0;
    for (i = 0; i < 150; i++) {
        waypoint0[i].x = -1;
        waypoint0[i].y = -1;
        waypoint2[i].x = -1;
        waypoint4[i].x = -1;
        waypoint2[i].y = -1;
        waypoint4[i].y = -1;
    }
    for (i = 0; i < map_w * map_h; i++) {
        w = *(unsigned short *)(map + i * 6 + 2);
        switch (w >> 8) {
        case 1:
            waypoint0[check_waypoint_nr((w & 0xff), "init.c", 398)].x = ((i % map_w) * 0x20 + 0x10) << 16;
            waypoint0[check_waypoint_nr((w & 0xff), "init.c", 399)].y = (i / map_w * 0x28 + 0x14) << 16;
            debug_printf("REDCAT wayp.nr %d found at (%d,%d) in tile %d\n", (w & 0xff),
                         (short)(waypoint0[(w & 0xff)].x >> 16), (short)(waypoint0[(w & 0xff)].y >> 16), i);
            waypoint_count0++;
            break;
        case 2:
            waypoint2[check_waypoint_nr((w & 0xff), "init.c", 405)].x = ((i % map_w) * 0x20 + 0x10) << 16;
            waypoint2[check_waypoint_nr((w & 0xff), "init.c", 406)].y = (i / map_w * 0x28 + 0x14) << 16;
            debug_printf("BULL wayp.nr %d found at (%d,%d) in tile %d\n", (w & 0xff),
                         (short)(waypoint2[(w & 0xff)].x >> 16), (short)(waypoint2[(w & 0xff)].y >> 16), i);
            waypoint_count2++;
            break;
        case 4:
            waypoint4[check_waypoint_nr((w & 0xff), "init.c", 412)].x = ((i % map_w) * 0x20 + 0x10) << 16;
            waypoint4[check_waypoint_nr((w & 0xff), "init.c", 413)].y = (i / map_w * 0x28 + 0x10) << 16;
            debug_printf("MUIS wayp.nr %d found at (%d,%d) in tile %d\n", (w & 0xff),
                         (short)(waypoint4[(w & 0xff)].x >> 16), (short)(waypoint4[(w & 0xff)].y >> 16), i);
            waypoint_count4++;
            break;
        }
    }
    debug_printf("%d waypoints found for REDCAT\n", waypoint_count0);
    debug_printf("%d waypoints found for BULL\n", waypoint_count2);
    debug_printf("%d waypoints found for MUIS\n", waypoint_count4);
    if (waypoint_count2 > 150 || waypoint_count4 > 150 || waypoint_count0 > 150)
        rc_error("%s(%d): To many waypoints in map-file! RC:%d BULL:%d SNITCH:%d\n",
                 "init.c", 425, waypoint_count0, waypoint_count2, waypoint_count4);
    if (waypoint_count2 <= 0 || waypoint_count4 <= 0 || waypoint_count0 <= 0)
        rc_error("%s(%d): No waypoints in map-file for object: RC:%d BULL:%d SNITCH:%d\n",
                 "init.c", 429, waypoint_count0, waypoint_count2, waypoint_count4);
    for (i = 0; i < 149; i++) {
        if ((waypoint0[i].x == -1 || waypoint0[i].y == -1)
            && (waypoint0[i + 1].x != -1 || waypoint0[i + 1].y != -1)) {
            rc_error("%s(%d) Missing waypoint for REDCAT: %d\n", "init.c", 436, i);
            return -1;
        }
        if ((waypoint2[i].x == -1 || waypoint2[i].y == -1)
            && (waypoint2[i + 1].x != -1 || waypoint2[i + 1].y != -1)) {
            rc_error("%s(%d) Missing waypoint for BULL: %d\n", "init.c", 442, i);
            return -1;
        }
        if ((waypoint4[i].x == -1 || waypoint4[i].y == -1)
            && (waypoint4[i + 1].x != -1 || waypoint4[i + 1].y != -1)) {
            rc_error("%s(%d) Missing waypoint for MUIS: %d\n", "init.c", 448, i);
            return -1;
        }
    }
    return 0;
}


/* ---- 0x15597 find_bonus_objects --------------------------------------------------------
 * Every map cell (+4 = kind) of 10..19 gets its object: 10 bomb, 12 mine,
 * 13 boost pickup, 15/16 cactus, 18 finish sign (anim 11), 19 start sign
 * (anim 10); 11, 14 and 17 are nothing. Cell centre in 16.16: x = (i %
 * map_w) * 32 + 16, y = (i / map_w) * 40 + 20, and the object number goes
 * back into cell byte +1. The three "boost object" messages are the
 * original's copy-paste (init.c lines 483..557). Kinds 11, 14 and 17 are in the
 * jump table (so the switch has ten arms) but do nothing: the game writes an
 * explicit `default: break;`, without it Watcom puts the merged
 * check_object_nr+store+loop-increment block after the last case instead of
 * after the case-13 body (32 rows -> 0). Body order in the source is 13, 12,
 * 10, 19, 18, 15, 16, straight from the jump table at 0x1556f.
 *
 * One row left, and it is find_waypoints' epilogue: the game's last jmp goes to
 * 0x15565 (find_waypoints' pop/ret, shared across the two functions), ours to an
 * earlier merged epilogue. It MATCHes once find_waypoints does.
 * -------------------------------------------------------------------------------------------- */

void find_bonus_objects(void)
{
    int i, n;

    for (i = 0; i < map_w * map_h; i++) {
        if (free_objects < 0)
            rc_error("(%s):%d Number of free objects cannot be negative : %d", "init.c", 470, free_objects);
        switch (map[i * 6 + 4]) {
        case 13:
            n = new_boost_pickup(((i % map_w) * 0x20 + 0x10) << 16, (i / map_w * 0x28 + 0x14) << 16);
            if (n == -1)
                rc_error("(%s):%d Cannot initialise boost object.", "init.c", 483);
            map[i * 6 + 1] = check_object_nr(n, "init.c", 485);
            break;
        case 12:
            n = new_mine(((i % map_w) * 0x20 + 0x10) << 16, (i / map_w * 0x28 + 0x14) << 16);
            if (n == -1)
                rc_error("(%s):%d Cannot initialise boost object.", "init.c", 494);
            map[i * 6 + 1] = check_object_nr(n, "init.c", 496);
            break;
        case 10:
            n = new_bomb(((i % map_w) * 0x20 + 0x10) << 16, (i / map_w * 0x28 + 0x14) << 16);
            if (n == -1)
                rc_error("(%s):%d Cannot initialise boost object.", "init.c", 506);
            map[i * 6 + 1] = check_object_nr(n, "init.c", 508);
            break;
        case 19:
            n = new_sign(((i % map_w) * 0x20 + 0x10) << 16, (i / map_w * 0x28 + 0x14) << 16, 10);
            if (n == -1)
                rc_error("(%s):%d Cannot initialise startsign object.", "init.c", 518);
            map[i * 6 + 1] = check_object_nr(n, "init.c", 521);
            break;
        case 18:
            n = new_sign(((i % map_w) * 0x20 + 0x10) << 16, (i / map_w * 0x28 + 0x14) << 16, 11);
            if (n == -1)
                rc_error("(%s):%d Cannot initialise finish sign object.", "init.c", 531);
            map[i * 6 + 1] = check_object_nr(n, "init.c", 533);
            break;
        case 15:
            n = new_cactus(((i % map_w) * 0x20 + 0x10) << 16, (i / map_w * 0x28 + 0x14) << 16);
            if (n == -1)
                rc_error("(%s):%d Cannot initialise cactus1 sign object.", "init.c", 543);
            map[i * 6 + 1] = check_object_nr(n, "init.c", 545);
            break;
        case 16:
            n = new_cactus(((i % map_w) * 0x20 + 0x10) << 16, (i / map_w * 0x28 + 0x14) << 16);
            if (n == -1)
                rc_error("(%s):%d Cannot initialise cactus2 sign object.", "init.c", 555);
            map[i * 6 + 1] = check_object_nr(n, "init.c", 557);
            break;
        default:
            break;
        }
    }
}
