/* RRRHIGH.DAT: ten {name, score, world} entries, best first.  RRR adds
   RedCat's world points after each race (0x1b6bf); RRRMENU shows the
   table on Einde (0x119cc).  Both load it with 0x1b82c / 0x136eb. */
#ifndef RC_HIGHSCORE_H
#define RC_HIGHSCORE_H

#include <stdint.h>

#define RC_HIGH_ENTRIES 10
#define RC_HIGH_SIZE 0x26       /* bytes per entry */
#define RC_HIGH_NAME 30

typedef struct {
    char name[RC_HIGH_NAME + 1];
    int32_t score;              /* +0x1e */
    int32_t world;              /* +0x22 */
} rc_high_entry;

typedef struct {
    rc_high_entry e[RC_HIGH_ENTRIES];
    uint8_t raw[RC_HIGH_ENTRIES * RC_HIGH_SIZE];   /* the file, kept */
} rc_high;

/* The file in dir, else the made-up table: scores 10..1 by a random racer
   in a random world, the first RedCat's.  rnd gives rand()'s values. */
void rc_high_load(rc_high *h, const char *dir, int (*rnd)(void *), void *ctx);
/* The score in before the first lower one; the file written.  Returns the
   place 0..9, or -1 when it is not in the table. */
int rc_high_add(rc_high *h, const char *dir, const char *name, int score,
                int world);

#endif
