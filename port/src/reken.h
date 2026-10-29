/* The arithmetic part (REKEN.EXE): sums, prizes, the shared session record.
   No SDL: the caller shows the sum and the prize and feeds the answer.

   Addresses in the comments are REKEN.EXE's (LE image); the evidence is in
   research/discoveries.md "The arithmetic part" and "The session". */
#ifndef RC_REKEN_H
#define RC_REKEN_H

#include <stdint.h>

#define RC_SUMS_PER_ROUND 8     /* module 1, 0x11ea2 */
#define RC_SESSION_SIZE 0x228   /* RRRIO.INF */

enum {                          /* [0x46c86] */
    RC_PRIZE_BOOSTS,            /* +5, max 99 */
    RC_PRIZE_ROCKETS,           /* +5, max 99 */
    RC_PRIZE_REPAIR_FULL,       /* repair to 100 */
    RC_PRIZE_REPAIR,            /* +30, max 100 */
    RC_PRIZE_FUEL5, RC_PRIZE_FUEL10, RC_PRIZE_FUEL15    /* max 150 */
};

/* RRRIO.INF, the record RRRMENU, REKEN and RRR share (file-formats.md). */
typedef struct {
    int32_t version;            /* 3 */
    int32_t repair;             /* +0x04, "damage points", 100 = intact */
    int32_t boosts;             /* +0x0c */
    int32_t rockets;            /* +0x14 */
    int32_t fuel;               /* +0x20 */
    int32_t stage;              /* +0x24: map column = module last played */
    int32_t groep;              /* +0x28: 3..8 */
    int32_t world;              /* +0x2c: 0..5 */
    int32_t speech;             /* +0x30 */
    int32_t race_done;          /* +0x1f8 */
    uint8_t raw[RC_SESSION_SIZE];   /* the whole file, other fields kept */
} rc_session;

void rc_session_default(rc_session *s);         /* 0x10ac9's defaults */
/* 0 if the bytes are a valid record (version 3), else defaults and -1. */
int rc_session_parse(rc_session *s, const uint8_t *d, uint32_t size);
void rc_session_store(rc_session *s);           /* fields back into raw */

/* The module played after this stage (0x10cad): 1..4, or -1 when the world
   is done.  With take set, it becomes the new stage. */
int rc_next_module(rc_session *s, int take);

/* The race of a stage (0x44f5c = RRR 0x57b8c): map 0..22, or -1 when the
   stage has none. */
int rc_stage_map(int world, int stage);

typedef struct {
    uint32_t seed;              /* Watcom rand() state */
    int level;                  /* [0x46c84]: groep 6,7,8 -> 0,1,2; 3,4,5 */
    int fuel, repair, boosts, rockets;
    int prize;
    char text[100];             /* 0x5371c */
    int answer;                 /* [0x537e4] */
    int asked, correct;         /* [0x537e8] */
} rc_reken;

void rc_reken_start(rc_reken *r, const rc_session *s, uint32_t seed);
void rc_reken_finish(const rc_reken *r, rc_session *s);        /* 0x199cf */

/* Module 1 "Handig rekenen": the next sum and its prize (0x11ea2 and the
   start of 0x11cfd).  Returns 0 after the last sum. */
int rc_module1_next(rc_reken *r);
/* The pupil's answer: 1 if right; the prize is then added at once (the
   original counts it up one unit per 200 ms). */
int rc_reken_answer(rc_reken *r, int answer);

/* The generators, for tests: type 0 x, 1 :, 2 +, 3 - (0x4539c's codes). */
void rc_sum_make(rc_reken *r, int type);
void rc_prize_choose(rc_reken *r);               /* 0x18e7f */
int rc_rand(rc_reken *r);                        /* 0x22bd2 */

#endif
