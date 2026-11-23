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

/* differs (145 rows): Watcom shares what the game keeps apart. The game reloads
 * bodies[racer].stats from scratch for every one of the four checks in the fallback
 * (mov edx,ebx; *5; load; *48) and duplicates the two check pairs, one per kind branch,
 * sharing only the spawn call; ours hoists the loads and merges the checks (854 bytes vs
 * 1254). Case bodies go in the game's order (2 before 1 - the jump table at 0x1af53 has
 * f4=1 at index 2 but its body sits at 0x1b196, after the f4=2 body at 0x1b157), which
 * took 164 rows to 145; the game also keeps ESI for i*24 where we use ECX, and passes
 * kind in EAX->EDX through a register the other way round. No wrong address (nmrefs).
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
                n = 0;
            } else {
                if (stats[bodies[racer].stats].drops == 0)
                    return;
                if (stats[bodies[racer].stats].damage < 0)
                    return;
                n = 4;
            }
            spawn_dropped_object(racer, n);
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
                    stats[bodies[script4[rrrio.world][i].f8].stats].heading);
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
        }
    }
}
