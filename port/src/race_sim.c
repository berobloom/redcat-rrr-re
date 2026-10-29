/* race_sim: run races without a screen and report how the racers do.

   usage: race_sim <data dir> [map 0..22 | all] [frames] [trace every N]

   The player gets no input, so this shows whether Brutus and Snitch get
   around each map: their waypoint progress and finishing frame. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archive.h"
#include "gfx8.h"
#include "race.h"

#define RC_IMAGES 1056

static rc_hotspot hotspot[RC_IMAGES + 1];
static int trace_every;

static int run(rc_archive *rcs, int map, int frames)
{
    static const int start_order[RC_RACERS] = { 0, 1, 2 };
    static rc_race r;
    rc_input none;
    uint32_t size;
    const uint8_t *d = rc_archive_find(rcs, rc_map_files[map], &size);
    int f, k, done[RC_RACERS] = { 0 };

    memset(&none, 0, sizeof none);
    if (!d || rc_race_init(&r, d, size, map, hotspot, start_order) != 0) {
        printf("map %2d %-12s failed to load\n", map, rc_map_files[map]);
        return 1;
    }
    r.started = 1;
    for (f = 1; f <= frames; f++) {
        rc_race_tick(&r, &none);
        if (trace_every && f % trace_every == 0)
            for (k = RC_BULL; k <= RC_MUIS; k++) {
                const rc_body *b = &r.body[2 * k];
                printf("%6d %s wp %3d xy %5d %5d z %3d speed %5.2f h %2d\n", f,
                       k == RC_BULL ? "bull" : "muis", r.wp_next[k],
                       b->x >> 16, b->y >> 16, b->z >> 16, b->speed / 65536.0,
                       r.stats[k].heading);
            }
        for (k = RC_BULL; k <= RC_MUIS; k++)
            if (r.finished[k] && !done[k])
                done[k] = f;
    }
    printf("map %2d %-12s", map, rc_map_files[map]);
    for (k = RC_BULL; k <= RC_MUIS; k++)
        printf("  %s wp %3d/%3d %s%6d", k == RC_BULL ? "Brutus" : "Snitch",
               r.wp_next[k], r.wp_count[k], done[k] ? "finished" : "   still",
               done[k] ? done[k] : f - 1);
    printf("\n");
    rc_race_free(&r);
    return !(done[RC_BULL] && done[RC_MUIS]);
}

int main(int argc, char **argv)
{
    rc_archive rcs, rc;
    int i, map = -1, frames = 20000, fails = 0;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <data dir> [map|all] [frames]\n", argv[0]);
        return 2;
    }
    if (argc > 2 && strcmp(argv[2], "all") != 0)
        map = atoi(argv[2]);
    if (argc > 3)
        frames = atoi(argv[3]);
    if (argc > 4)
        trace_every = atoi(argv[4]);
    if (rc_archive_open(&rcs, argv[1], "RCS") != 0 ||
        rc_archive_open(&rc, argv[1], "RC") != 0) {
        fprintf(stderr, "RCS/RC archives not found in %s\n", argv[1]);
        return 1;
    }
    for (i = 1; i <= RC_IMAGES && i - 1 < rc.count; i++) {
        rc_sprite s;
        uint32_t size;
        const uint8_t *d = rc_archive_get(&rc, i - 1, &size);
        if (d && rc_sprite_parse(&s, d, size) == 0) {
            hotspot[i].hx = (int16_t)s.x;
            hotspot[i].hy = (int16_t)s.y;
        }
    }
    if (map >= 0)
        return run(&rcs, map, frames);
    for (i = 0; i < RC_NUM_MAPS; i++)
        fails += run(&rcs, i, frames);
    return fails != 0;
}
