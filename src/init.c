/* init.c: the racers and bonus objects of a race: the object/body/stats setup (0x148db), the map
   cells' waypoint path (0x1517a) and the bonus objects in them (0x15597).
   CONFIRMED name: find_waypoints and find_bonus_objects pass __FILE__ "init.c" to check_waypoint_nr
   (0x1b69d) and rc_error, with the game's own line numbers 398..557.
   STRONGLY SUPPORTED boundary: the game shares one epilogue between find_waypoints and
   find_bonus_objects (find_bonus_objects' last jmp 0x15665 goes to find_waypoints' pop/ret at
   0x15565), and Watcom shares an epilogue only inside one object - so the two are one module
   (MATCHING.md 4, open question 27).
   STRONGLY SUPPORTED, 2026-11-27: init_objects 0x148db is in this module too - its only string,
   'player_direction : %d wp: %d\n' (0x50e04, pushed at 0x14b15), sits at the head of the init.c
   string block: 0x50e04 + 29 + 1 = 0x50e22, where 'init.c' starts, and the four bytes before
   0x50e04 are zero (the end of the previous module's block, and the two doubles 0x50de3/0x50deb
   that direction_to_waypoint 0x13d60 uses sit just before them). */

#include "rrr.h"

/* ---- 0x148db init_objects -------------------------------------------------------------
 * The race's objects, bodies and stats: players in objects[0]/[2]/[4] with bodies[0]/[2]/[4]
 * and stats[0]/[1]/[2], their followers in objects[1]/[3]/[5], and objects[6..305] left in the
 * free pool. The starting position of every racer is its list's first waypoint, taken from the
 * list of the *grid slot* it starts in (rrrio.start_order[3], default 0/2/4: bodies[0] takes
 * waypoint0[0], bodies[2] waypoint2[0], bodies[4] waypoint4[0]). stats[i].mass is 16.16 too
 * (0x3c0000 = 60 for RedCat and Snitch, 0x5a0000 = 90 for Brutus); the computer racers' fuel,
 * boosts, rockets and the +0x18/+0x20 counters are the same 3 and their top speed comes from a
 * per-map table (0x57b18) indexed by map_no. direction_to_waypoint gets EAX = body number,
 * EDX/EBX = the target x/y (open question 30).
 *
 * The player's stats block is written drops, top_speed, f24, fuel, DAMAGE, boosts, f10, vx, f14,
 * vy, rockets, vz: damage stored right after fuel (not last) is what makes Watcom emit the game's
 * `cmp [stats+0x28],0` reload instead of reusing the value it just stored, and it lands our length
 * on the game's 1635 bytes with every address aligned (16 rows -> 7).
 *
 * differs (7 rows, 1635 = 1635 bytes, addresses aligned from +0 to the end):
 *   +607 the player's `rrrio.fuel << 16` product: the game keeps it in EBX and loads rrrio.boosts
 *        into EDX after it, we keep it in EDX (5 rows). Tried: * 0x10000/* 65536/< 16, moving the
 *        fuel, damage, boosts, rockets and the three bodies stores (192 statement permutations of
 *        the player, BULL and MUIS stats blocks through inplace.py, .work/probe/io/y*.c, best 7),
 *        `!(d > 0 && f > 0)` vs `<= 0 ||` vs `< 1` and an extra store: nothing moves it.
 *   +1169 the MUIS heading store: same tie, the game's result sits in EBX so the store floats
 *        past the racer_top_speeds[] load, ours is forced early. BULL, whose text is identical
 *        apart from the constants, matches - the game's two blocks are the mirror image of ours.
 *   Layout, not behaviour: find_waypoints' three __FILE__ strings and the rc_error texts land 1-2
 *        bytes off the game's (nmrefs: ours 0x50f4b/0x50f91/0x50fb9/0x50fdf, game
 *        0x50f4a/0x50f8f/0x50fb7/0x50fdd), because the strings of this module's other functions
 *        (clear_objects, start_crash, switch_anims, fn_15166) still sit in rrr.c's pool. It costs
 *        no match (objmatch compares those fixups by target); it goes away with the module split.
 * -------------------------------------------------------------------------------------------- */

void init_objects(void)
{
    int i;

    clear_objects();
    bodies[0].x = waypoint0[0].x;
    bodies[0].y = waypoint0[0].y;
    bodies[2].x = waypoint2[0].x;
    bodies[2].y = waypoint2[0].y;
    bodies[4].x = waypoint4[0].x;
    bodies[4].y = waypoint4[0].y;
    for (i = 0; i < 3; i++) {
        switch (rrrio.start_order[i]) {
        case 0:
            if (i == 0) {
                bodies[0].x = waypoint0[0].x;
                bodies[0].y = waypoint0[0].y;
            }
            if (i == 1) {
                bodies[0].x = waypoint2[0].x;
                bodies[0].y = waypoint2[0].y;
            }
            if (i == 2) {
                bodies[0].x = waypoint4[0].x;
                bodies[0].y = waypoint4[0].y;
            }
            break;
        case 2:
            if (i == 0) {
                bodies[2].x = waypoint0[0].x;
                bodies[2].y = waypoint0[0].y;
            }
            if (i == 1) {
                bodies[2].x = waypoint2[0].x;
                bodies[2].y = waypoint2[0].y;
            }
            if (i == 2) {
                bodies[2].x = waypoint4[0].x;
                bodies[2].y = waypoint4[0].y;
            }
            break;
        case 4:
            if (i == 0) {
                bodies[4].x = waypoint0[0].x;
                bodies[4].y = waypoint0[0].y;
            }
            if (i == 1) {
                bodies[4].x = waypoint2[0].x;
                bodies[4].y = waypoint2[0].y;
            }
            if (i == 2) {
                bodies[4].x = waypoint4[0].x;
                bodies[4].y = waypoint4[0].y;
            }
            break;
        default:
            break;
        }
    }
    waypoint_index0 = 1;
    waypoint_index2 = 1;
    waypoint_index4 = 1;
    objects[0].anim = 2;
    objects[0].frame = 0;
    objects[0].period = 1;
    objects[0].callback = (void (*)(void))player_tick;
    bodies[0].z = 0;
    bodies[0].speed = 0;
    bodies[0].stats = 0;
    stats[0].mass = 0x3c0000;
    stats[bodies[0].stats].heading = direction_to_waypoint(0,
        waypoint0[waypoint_index0].x, waypoint0[waypoint_index0].y);
    debug_printf("player_direction : %d wp: %d\n", stats[bodies[0].stats].heading,
        waypoint_index0 + 1);
    stats[bodies[0].stats].drops = 0;
    stats[bodies[0].stats].top_speed = 0x100000;
    stats[bodies[0].stats].f24 = 0;
    stats[bodies[0].stats].fuel = rrrio.fuel << 0x10;
    stats[bodies[0].stats].damage = rrrio.damage;
    stats[bodies[0].stats].boosts = rrrio.boosts;
    stats[bodies[0].stats].f10 = rrrio.f18;
    bodies[0].vx = 0;
    stats[bodies[0].stats].f14 = rrrio.f1c;
    bodies[0].vy = 0;
    stats[bodies[0].stats].rockets = rrrio.rockets;
    bodies[0].vz = 0;
    if (!(stats[bodies[0].stats].damage > 0 && stats[bodies[0].stats].fuel > 0))
        crash0 = 1;
    objects[2].anim = 0;
    objects[2].frame = 0;
    objects[2].period = 1;
    objects[2].callback = (void (*)(void))bull_tick;
    bodies[2].z = 0;
    bodies[2].speed = 0;
    bodies[2].stats = 1;
    stats[1].fuel = 0x640000;
    stats[1].boosts = 3;
    stats[1].f18 = 3;
    stats[1].rockets = 3;
    stats[1].drops = 3;
    stats[1].mass = 0x5a0000;
    stats[bodies[2].stats].heading = direction_to_waypoint(2,
        waypoint2[waypoint_index2].x, waypoint2[waypoint_index2].y);
    stats[bodies[2].stats].f24 = 0;
    stats[bodies[2].stats].damage = 100;
    bodies[2].vx = 0;
    stats[bodies[2].stats].top_speed = racer_top_speeds[map_no];
    bodies[2].vy = 0;
    bodies[2].vz = 0;
    objects[4].anim = 1;
    objects[4].frame = 0;
    objects[4].period = 1;
    objects[4].callback = (void (*)(void))muis_tick;
    bodies[4].z = 0;
    bodies[4].speed = 0;
    bodies[4].stats = 2;
    stats[2].fuel = 0x640000;
    stats[2].boosts = 3;
    stats[2].f18 = 3;
    stats[2].rockets = 3;
    stats[2].drops = 3;
    stats[2].mass = 0x3c0000;
    stats[bodies[4].stats].heading = direction_to_waypoint(4,
        waypoint4[waypoint_index4].x, waypoint4[waypoint_index4].y);
    stats[bodies[4].stats].f24 = 0;
    stats[bodies[4].stats].damage = 100;
    stats[bodies[4].stats].top_speed = racer_top_speeds[map_no];
    bodies[4].vx = 0;
    bodies[4].vy = 0;
    bodies[4].vz = 0;
    objects[1].anim = 4;
    objects[1].frame = 0;
    objects[1].period = 1;
    objects[1].callback = (void (*)(void))follower_tick;
    bodies[1].x = bodies[0].x;
    bodies[1].y = bodies[0].y;
    bodies[1].z = bodies[0].y;
    bodies[1].stats = -1;
    objects[3].anim = 4;
    objects[3].frame = 0;
    objects[3].period = 1;
    objects[3].callback = (void (*)(void))follower_tick;
    bodies[3].x = bodies[2].x;
    bodies[3].y = bodies[2].y;
    bodies[3].z = bodies[2].y;
    bodies[3].stats = -1;
    objects[5].anim = 4;
    objects[5].frame = 0;
    objects[5].period = 1;
    objects[5].callback = (void (*)(void))follower_tick;
    bodies[5].x = bodies[4].x;
    bodies[5].y = bodies[4].y;
    bodies[5].z = bodies[4].y;
    bodies[5].stats = -1;
    for (i = 6; i < 306; i++) {
        objects[i].anim = 7;
        objects[i].frame = 0;
        objects[i].period = -1;
        objects[i].callback = (void (*)(void))free_object_tick;
        bodies[i].stats = -1;
        bodies[i].x = 0;
        bodies[i].y = 0;
        bodies[i].z = -1;
    }
}

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
