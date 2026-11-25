/* ai.c - the computer racers' own code (address order, module boundary unknown yet).
 * ai_waypoint_action gets its own object because its switch jump tables sit in the
 * gap before it (0x1af53, 0x1af6b in the game), which objmatch would otherwise read
 * as the tail of ai_script_find (0x1aedf) and score as a diff. */
#include "rrr.h"

/* ---- 0x1af83 ai_waypoint_action ------------------------------------------------------------
 * A computer racer has reached a waypoint. If that waypoint number is in its
 * script, run the scripted action (f4: 1 rocket aimed with aim_at_player, 2
 * drop the kind in f8, 3 boost, 4 wait); otherwise the random fallback
 * (discoveries.md): 10% drop a mine or bomb (kind 0 or 4, half each), else 10%
 * boost. Both script tables are empty in the EXE, so the fallback is all that
 * ever runs. The two racer branches are separate copies of the same switch over
 * script2 and script4, which is why Watcom computes [0x751a4] * 720 + i * 24
 * once and adds the two table bases as displacements.
 * -------------------------------------------------------------------------------------------- */

/* differs (145 -> 64 rows, 1151 of the game's 1254 bytes): the content was missing, not
 * the spelling. Three things were:
 * 1. The switch has a SIXTH arm. Both jump tables (0x1af53, 0x1af6b) hold six entries for
 *    f4+1 = 0..5, so f4 = 4 exists: wait. Its body (0x1b2c7 for racer 2, 0x1b407 for racer 4)
 *    is `if (waypoint_index0 - 1 < script[world][i].f14) { wait2/4 = 1; bodies[racer].speed
 *    = 0; stats[...].heading = aim_at_player(racer, 0); } else wait2/4 = 0;` - 90 bytes each,
 *    -77 rows. Both tables' f4 = 3 arm points at the ONE boost body at 0x1b25e, so case 3 is
 *    written in both switches and Watcom emits it once (like cell_effect's duplicated case).
 * 2. Racer 4's rocket arm reads BRUTUS's script: 0x1b3bf is `mov edx, [eax + 0x57c98]` =
 *    script2[world][i].f8; script4's f8 (0x5bd48) is only used in that switch's drop arm
 *    (0x1b37e). CONFIRMED - writing script2 there is what makes check.sh's fixup agree.
 * 3. The fallback's drop kind is an immediate in the call's second register: the mine arm does
 *    `mov eax,ebx; xor edx,edx` (0x1b010) and jmps to the one `call spawn_dropped_object` at
 *    0x1b06b, which the bomb arm reaches through `mov edx,4` (0x1b064); both then jmp to the
 *    shared `play_fx(20, 0); return` tail at 0x1b2be. Two literal calls with their own
 *    spawn+play_fx+return are what scores best (64): writing the kind into the parameter `n`
 *    and sharing the call, the shape the game's code suggests, gives 68.
 * What is left is allocation, again: the game re-derives the bodies[]/stats[] index inside every
 * check in the case bodies (nmrefs: it reads bodies 3x, stats 1x and the script block 0x751a4 1x
 * more, and calls aim_at_player once more, where we call fire_rocket once more) and holds the
 * script byte offset i*24 in ESI (0x1b11a `imul esi,eax,0x18`) where we hold it in ECX. Guard-clause vs `&&` conditions in the arms (all three), an `int h` for the aim
 * argument, the switch expression in an `int act` local: all exactly 64. `default: break;`
 * instead of `case -1: case 0:` costs 4, and writing case 4's `else` arm first costs 13.
 */

void ai_waypoint_action(int racer, int n)
{
    int i = ai_script_find(racer, n);

    if (i < 0) {
        if (rand() % 100 < 10) {
            if (rand() % 100 < 50) {
                if (stats[bodies[racer].stats].drops == 0)
                    return;
                if (stats[bodies[racer].stats].damage < 0)
                    return;
                spawn_dropped_object(racer, 0);
                play_fx(20, 0);
                return;
            } else {
                if (stats[bodies[racer].stats].drops == 0)
                    return;
                if (stats[bodies[racer].stats].damage < 0)
                    return;
                spawn_dropped_object(racer, 4);
            }
            play_fx(20, 0);
            return;
        }
        if (rand() % 100 >= 10)
            return;
        if (stats[bodies[racer].stats].boosts == 0)
            return;
        bodies[racer].speed += 8 << 16;
        bodies[racer].vz += 8 << 16;
        play_fx(16, 0);
        stats[bodies[racer].stats].boosts--;
        play_fx(17, 0);
        return;
    }
    if (racer == 2) {
                switch (script2[rrrio.world][i].f4) {
        case 2:
            if (stats[bodies[racer].stats].drops != 0
                && stats[bodies[racer].stats].damage >= 0) {
                spawn_dropped_object(racer, script2[rrrio.world][i].f8);
                play_fx(20, 0);
            }
            break;
        case 1:
            if (stats[bodies[racer].stats].rockets != 0
                && stats[bodies[racer].stats].damage >= 0) {
                stats[bodies[racer].stats].heading = aim_at_player(racer,
                    stats[bodies[script2[rrrio.world][i].f8].stats].heading);
                fire_rocket(racer);
                play_fx(20, 0);
            }
            break;
        case -1:
        case 0:
            break;
        case 3:
            if (stats[bodies[racer].stats].boosts != 0) {
                bodies[racer].speed += 8 << 16;
                bodies[racer].vz += 8 << 16;
                play_fx(16, 0);
                stats[bodies[racer].stats].boosts--;
                play_fx(17, 0);
            }
            break;
        case 4:
            if (waypoint_index0 - 1 < script2[rrrio.world][i].f14) {
                wait2 = 1;
                bodies[racer].speed = 0;
                stats[bodies[racer].stats].heading = aim_at_player(racer, 0);
            } else {
                wait2 = 0;
            }
            break;
        }
    } else if (racer == 4) {
                switch (script4[rrrio.world][i].f4) {
        case 2:
            if (stats[bodies[racer].stats].drops != 0
                && stats[bodies[racer].stats].damage >= 0) {
                spawn_dropped_object(racer, script4[rrrio.world][i].f8);
                play_fx(20, 0);
            }
            break;
        case 1:
            if (stats[bodies[racer].stats].rockets != 0
                && stats[bodies[racer].stats].damage >= 0) {
                stats[bodies[racer].stats].heading = aim_at_player(racer,
                    stats[bodies[script2[rrrio.world][i].f8].stats].heading);
                fire_rocket(racer);
                play_fx(20, 0);
            }
            break;
        case -1:
        case 0:
            break;
        case 3:
            if (stats[bodies[racer].stats].boosts != 0) {
                bodies[racer].speed += 8 << 16;
                bodies[racer].vz += 8 << 16;
                play_fx(16, 0);
                stats[bodies[racer].stats].boosts--;
                play_fx(17, 0);
            }
            break;
        case 4:
            if (waypoint_index0 - 1 < script4[rrrio.world][i].f14) {
                wait4 = 1;
                bodies[racer].speed = 0;
                stats[bodies[racer].stats].heading = aim_at_player(racer, 0);
            } else {
                wait4 = 0;
            }
            break;
        }
    }
}
