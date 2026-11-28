/* hud.c (our name; the original's is not known): the race's panel, from hud_count 0x19391.
   It is in a module after map.c (0x18b8d) and before rrrio's strings; where it starts and
   ends is not known yet. Inside rrr.c Watcom moved hud_count's entry block next to its caller
   draw_hud_counts (0x15166, two modules earlier), so it cannot be in the same object as that. */

#include "rrr.h"

/* ---- 0x1918d -------------------------------------------------------------------------------- */

void hud_update(void)
{
    int v;

    v = stats[bodies[0].stats].damage / 10;
    if (v != hud_damage) {
        hud_damage = v;
        if (v > 9)
            v = 9;
        else if (v < 0)
            v = 0;
        draw_frame(screen_buf, frames[animset[21].steps[v].frame].img, 217, 438,
                   frames[animset[21].steps[v].frame].w, frames[animset[21].steps[v].frame].h);
    }
    v = (short)(stats[bodies[0].stats].fuel >> 16) / 16;
    if (v != hud_fuel) {
        hud_fuel = v;
        if (v > 8)
            v = 8;
        else if (v < 0)
            v = 0;
        draw_frame(screen_buf, frames[animset[20].steps[v].frame].img, 110, 422,
                   frames[animset[20].steps[v].frame].w, frames[animset[20].steps[v].frame].h);
    }
    v = abs((short)(fixdiv(fixmul(bodies[0].speed, 11), 20) >> 16));
    if (v != hud_speed) {
        hud_speed = v;
        if (v > 10)
            v = 10;
        else if (v < 0)
            v = 0;
        draw_frame(screen_buf, frames[animset[19].steps[v].frame].img, 20, 414,
                   frames[animset[19].steps[v].frame].w, frames[animset[19].steps[v].frame].h);
    }
    if (hud_boosts != stats[bodies[0].stats].boosts) {
        hud_boosts = stats[bodies[0].stats].boosts;
        hud_count(13);
    }
    if (hud_rockets != stats[bodies[0].stats].rockets) {
        hud_rockets = stats[bodies[0].stats].rockets;
        hud_count(23);
    }
}


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
