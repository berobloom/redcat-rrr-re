/* The screen after a race (RRR.EXE podium 0x15a9f). */
#ifndef RC_PODIUM_H
#define RC_PODIUM_H

#include <stdint.h>

#include "archive.h"
#include "gfx8.h"

typedef struct {
    rc_pcx pic;                 /* rbs.pcx .. sbr.pcx */
    rc_font font;               /* RCS Ascii20.img */
} rc_podium;

/* order: object numbers 0, 2, 4 by place; points per racer (RedCat, Brutus,
   Snitch); name: the pupil's.  Draws into fb (640 x 480) and pal. */
int rc_podium_draw(rc_podium *p, const rc_archive *rcs, int world,
                   const int32_t order[3], const int32_t points[3],
                   const char *name, uint8_t *fb, rc_rgb *pal);
void rc_podium_free(rc_podium *p);

#endif
