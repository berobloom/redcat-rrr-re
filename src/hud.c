/* hud.c (our name; the original's is not known): the race's panel, from hud_count 0x19391.
   It is in a module after map.c (0x18b8d) and before rrrio's strings; where it starts and
   ends is not known yet. Inside rrr.c Watcom moved hud_count's entry block next to its caller
   fn_15166 (0x15166, two modules earlier), so it cannot be in the same object as that. */

#include "rrr.h"

/* ---- 0x19391 -------------------------------------------------------------------------------- */

void hud_count(int what)
{
    int lo, hi, x, y;

    switch (what) {
    case 13:
        lo = animset[18].steps[stats[bodies[0].stats].boosts % 10].frame;
        hi = animset[18].steps[stats[bodies[0].stats].boosts / 10 % 10].frame;
        x = 389;
        y = 415;
        break;
    case 23:
        lo = animset[18].steps[stats[bodies[0].stats].rockets % 10].frame;
        hi = animset[18].steps[stats[bodies[0].stats].rockets / 10 % 10].frame;
        x = 494;
        y = 415;
        break;
    }
    draw_frame(screen_buf, frames[hi].img, x, y, frames[hi].w, frames[hi].h);
    draw_frame(screen_buf, frames[lo].img, x + 16, y, frames[lo].w, frames[lo].h);
}
