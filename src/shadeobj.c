/* shadeobj.c: 0x167aa-0x16944, one function. CONFIRMED boundary: its __FILE__ string 'shadeobj.c'
   starts a fresh string block at 0x51750 (one padding byte, 0x5174f, after raceanim.c's last
   string), 'illegal shade object : %d following :%d' follows it with no gap, and the callbacks'
   'illegal object : %d ' starts a new block at 0x51784 (padding byte 0x51783) - so the module ends
   with follower_tick. That is also why the game gives follower_tick its own pop/ret at 0x1693a
   while smoke_tick (0x16e46) jumps into dropped2_tick's epilogue at 0x17b5b: one object shares an
   epilogue, two do not. The callbacks are in src/shadeobj2.c. */

#include "rrr.h"

/* ---- 0x167aa -------------------------------------------------------------------------------- */

/* differs: 9 rows, two Watcom choices, nothing else:
   (1) the game sign-extends the short parameter in place - `cwde`, then `mov esi,eax` + `dec eax`
       and its `shl esi,4` before the call; we do `movsx esi,ax` + `lea eax,[esi-1]` and shl after;
   (2) the `frame = 32` store indexes with EDX and keeps objects in EAX, we mirror.
   41 -> 13 rows came from the damage/fuel test: the game's fall-through arm is the 32 store, so it
   wrote the negated-and form `!(damage > 0 && fuel > 0)`; `damage <= 0 || fuel <= 0` costs 30 rows
   more and `if (damage > 0 && fuel > 0) <heading> else <32>` 20. The last 4 rows are the module
   split (its own epilogue, header above). Then nothing pays: k/t/param
   types (int, long, unsigned, char, unsigned short), every declaration order, `frame = 0x20` /
   `(short)32` / braces / `| 0`, `heading` cast spellings, `t = n - 1` as two statements,
   (int)/(short)/L/`n + -1`/`--n` in the check_object_nr call, an extra local, and the mode, period,
   anim and bodies-copy statements moved - ~60 variants, each compiled alone in .work/probe/ft2.
   2026-11-29 (.work/pi/ft/v.py, v2.py, 20 vbatch variants in-module): argint, framecast, framensu,
   testzero, statsholder, objptr, pixel, framesub, paramcopy, paramhead, hex32, cast32, hexmask,
   modecast, periodcast, argsub, nlocal, tnocall, bodiesFirst, kdeclblock - none below 9, and
   argint, framecast, testzero, hex32, cast32, hexmask, modecast, argsub, tnocall all hold 9.
   (framensu, framesub and the first testzero scored 63 because my edit text dropped the `else` arm -
   invalid probes, not evidence.) New,
   CONFIRMED (side-by-side dump .work/pi/lo/dump.py on .work/src/SHADEOBJ.OBJ): (a) a named int copy
   of the short parameter does NOT buy the game's in-place extension - `int m = n;` then `m - 1` and
   `objects[m]` is 24 rows (paramcopy, paramhead; `register int m` 24); (b) the `frame = 32` store's
   instruction is byte-identical (`mov word ptr [edx+eax+0xa],0x20` 0x16896 game / 0x16899 ours), so
   its 3 rows are only which of EAX/EDX holds the re-derived `n << 4` and which holds `objects` - no
   RHS spelling (32, 0x20, (short)32), guard spelling (`0 < damage && 0 < fuel`) or index cast
   moves it; (c) both sides keep `n << 4` in EBX live into the else arm and reuse it there
   (`mov [ebx+eax+0xa],dx` 0x168b0 game, 0x168b3 ours) - the then arm re-derives it. Row 9 is the
   head's 2-byte length gap (game `cwde`+`mov esi,eax`+`dec eax` = 4 B, ours `movsx esi,ax`+
   `lea eax,[esi-1]` = 6 B), which leaves the game's `push edx` at 0x16945 without a partner - that
   push is NOT a missing statement, 0x16945 is `free_object_tick` (rrr_names.txt line 40) and the
   game's `ret` sits at 0x16944.
   2026-11-28 round 2 (.work/pi/adr/v2.py, v3.py, 9 in-module vbatch variants): kr (old-style
   `int follower_tick(n) short n;`) and oframe (`(&objects[n])->frame`) hold 9; krint (old-style
   `int n;`) 66, oframeh (a `struct body *b` in the else arm plus `(&objects[n])`) 80, modefirst
   (the mode store before the call) 32, andmask (`heading % 32`) 38, hstore (an `int h` holder in
   the else arm) 49; krhstore 49; modefirstof did not compile (E1081). So K&R parameter promotion
   is not the route to the game's in-place `cwde` either - floor 9 rows. */
int follower_tick(short n)
{
    short k;
    int t;

#line 16 "shadeobj.c"
    t = check_object_nr(n - 1, "shadeobj.c", __LINE__);
    sprites[objects[n].sprite].mode = 6;
    if (objects[n].period == -1)
        return 0;
    if (t < 0 || t > 5)
        rc_error("illegal shade object : %d following :%d", (int)n, t);
    bodies[n].x = bodies[t].x;
    bodies[n].y = bodies[t].y;
    bodies[n].z = 0;
    bodies[n].f1c = -100000;
    objects[n].anim = 4;
    if (!(stats[bodies[t].stats].damage > 0 && stats[bodies[t].stats].fuel > 0))
        objects[n].frame = 32;
    else
        objects[n].frame = stats[bodies[t].stats].heading & 31;
    k = animset[objects[n].anim].steps[objects[n].frame].frame;
    objects[n].x = (bodies[n].x >> 16) - frames[k].f8 - 16;
    objects[n].y = (bodies[n].y >> 16) - (bodies[n].z >> 16) - frames[k].fa;
    return 0;
}


