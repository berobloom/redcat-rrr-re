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
 * What is left is one allocation choice: the game gives the script index i a home register, ECX
 * (0x1af8d `mov ecx,eax`, straight after the call), and keeps no register for the byte offset
 * i*24, re-deriving it wherever an arm needs it (0x1b11a `imul esi,eax,0x18` for the switch, again
 * `imul eax,ecx,0x18` at 0x1b1fd inside racer 2's rocket arm). That is why its arms each re-read
 * bodies[]/stats[]/0x751a4 and call aim_at_player once more (nmrefs), why its two wait bodies are
 * two copies (0x1b2c7, 0x1b407) and why one fire_rocket call serves both rocket arms (0x1b24e).
 * We give i no home and keep the offset live instead, so our arms share it and our bodies merge.
 * Guard-clause vs `&&` conditions in the arms (all three), an `int h` for the aim argument, the
 * switch expression in an `int act` local: all exactly 64. `default: break;` instead of
 * `case -1: case 0:` costs 4, and writing case 4's `else` arm first costs 13.
 *
 * 2026-11-25 (.work/probe/awa3, ~45 more spellings): both effects appear at once if the racer test
 * is `switch (racer)` instead of the if/else-if chain - s1.c 1184 B / 43 rows, and with an `int h`
 * for the aim argument (w4.c) 1197 B / 39, the closest this function has been. It is not the
 * original text: WCC then lowers the racer test as a range tree (`cmp ebx,2`/`jb`, `cmp ebx,2`/`jbe`,
 * `cmp ebx,4`/`je` plus a second epilogue copy at 0x1b14d) where the game has the two `cmp`/`jne` of
 * an if/else-if chain, and no spelling of that chain (nested, else-if, separate ifs - 385 rows) ever
 * gives i a home. In that switch context, `i = j = ai_script_find(...)` and a second index local
 * (m1/m2 - WCC copy-propagates them into one), `h2`/`h4` per arm, guard clauses, `default: break;`,
 * `int act`, `int s`: 39-73. Kept here: the if/else-if chain, the shape the game's dispatch has.
 *
 * 2026-11-26: the rebuilt index chains are local to ONE block. The jump tables (0x1af53 racer 2,
 * 0x1af6b racer 4: f4+1 = 0..5 -> 1b465 1b465 1b196/1b389 1b157/1b34a 1b25e 1b2c7/1b407) show
 * racer 2's rocket arm 0x1b196 is reached only from its table, yet it alone rebuilds
 * stats[bodies[racer].stats] for each of its four uses and script2[rrrio.world][i] from
 * rrrio.world with imul ecx(i),0x18; racer 4's rocket arm 0x1b389, the boost arm 0x1b25e and
 * racer 2's drop arm reuse those values as ours do. Declaring i register/long/short/split, a world
 * local, reusing n for i, a racer copy (aw5), and `->`/pointer spellings of that arm alone (aw6)
 * leave it at 64 (n reused: 61 rows but 1004 bytes).
 * 2026-11-26 (later, .work/probe/aw7): the top level does not move i's home either: `if (i < 0)
 * {fallback} else {script}` compiles to the same 1151 bytes, and `if (i >= 0) {script} else
 * {fallback}` gives 480 rows. The f24-chain lever (an unsigned if/else-if chain written as a
 * switch) has nothing to act on here: the dispatch already is a switch on f4.
 *
 * 2026-11-28 (round 1, .work/pi/awa/v.py, v2.py, 17 spellings, control 64): nothing is below 64.
 * The one global the game references more often than we do is `rrrio.world` (0x751a4 = rrrio
 * 0x75178 + 0x2c): caching it in a local and indexing `scriptN[w][i]` costs 124 rows (188), so the
 * game's extra read is WCC re-materialising `scriptN[rrrio.world][i]` per statement, not a variable
 * of its source. `h = aim_at_player(racer, ...); stats[bodies[racer].stats].heading = h;` in the
 * rocket arm (either racer, or both) leaves 64 unchanged - its one extra aim_at_player call and one
 * fewer fire_rocket call (nmrefs) come from WCC merging only the `fire_rocket(racer); play_fx(20,
 * 0);` tail of its two rocket arms, which no spelling of the body asks for. Also 64: the `-1/0` arm
 * last, `.f8` read into a local before spawn_dropped_object, the `speed`/`wait2` and `vz`/`speed`
 * swaps inside the arms. Worse: `case 1:` first 150, `case 3:` first 149, `wait2 = 0;` before the
 * case-4 test 70, `.f14` into a local 65. `(scriptN[rrrio.world] + i)->f8` never scored - the
 * variant's replacement text was missing its '('.
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
