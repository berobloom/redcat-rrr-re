/* The arithmetic part (REKEN.EXE): sums, prizes, the shared session record.
   No SDL: the caller shows the sum and the prize and feeds the answer.

   Addresses in the comments are REKEN.EXE's (LE image); the evidence is in
   research/discoveries.md "The arithmetic part" and "The session". */
#ifndef RC_REKEN_H
#define RC_REKEN_H

#include <stdint.h>

#define RC_SUMS_PER_ROUND 8     /* module 1, 0x11ea2 */
#define RC_SESSION_SIZE 0x228   /* RRRIO.INF */
#define RC_MEMORY_CELLS 30      /* module 3: the largest grid, 5 x 6 */

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

/* Module 3 "Memory": a card of the grid (0x58f98, 58-byte records) */
enum { RC_CARD_GONE = -1, RC_CARD_SOM, RC_CARD_ANTWOORD };
typedef struct {
    char text[50];              /* the sum; a SOM card shows it */
    int value;                  /* +0x32: an ANTWOORD card shows it */
    int kind;                   /* +0x36 */
} rc_card;

/* 0x456e4: the grid per level */
typedef struct {
    int cols, rows;             /* [0x457a4], [0x457a8] */
    int x, y, w, h, gap_x, gap_y;   /* [0x457b0].. */
} rc_grid;

typedef struct {
    uint32_t seed;              /* Watcom rand() state */
    int level;                  /* [0x46c84]: groep 6,7,8 -> 0,1,2; 3,4,5 */
    int fuel, repair, boosts, rockets;
    int prize;
    char text[100];             /* 0x5371c */
    int answer;                 /* [0x537e4] */
    int asked, correct;         /* [0x537e8] */
    /* module 2 */
    char choice[3][100];        /* 0x537ec: choice 0 is the true sum */
    int choice_value[3];        /* 0x53be4 */
    int true_row;               /* [0x53c80]: the row choice 0 is shown in */
    /* module 3: asked counts the pairs tried ([0x5bcf0]), correct the
       pairs found ([0x5bcec]) */
    rc_card card[RC_MEMORY_CELLS];
    int cells, left;            /* [0x457ac]; cards still on the grid */
    int first, second;          /* [0x58f90], [0x58f94]: turned cells, -1 */
} rc_reken;

void rc_reken_start(rc_reken *r, const rc_session *s, uint32_t seed);
void rc_reken_finish(const rc_reken *r, rc_session *s);        /* 0x199cf */

/* Module 1 "Handig rekenen": the next sum and its prize (0x11ea2 and the
   start of 0x11cfd).  Returns 0 after the last sum. */
int rc_module1_next(rc_reken *r);
/* The pupil's answer: 1 if right; the prize is then added at once (the
   original counts it up one unit per 200 ms). */
int rc_reken_answer(rc_reken *r, int answer);

/* Module 2 "Wat is waar?": three sums with their results, one of them
   true (0x13630, 0x12844 and the start of 0x13229).  Returns 0 after the
   last round. */
int rc_module2_next(rc_reken *r);
/* The choice shown in row 0..2 (0x12ee0). */
int rc_module2_choice(const rc_reken *r, int row);
/* The row picked: 1 if it shows the true sum; the prize as in module 1. */
int rc_module2_answer(rc_reken *r, int row);

/* Module 3 "Memory": deal the grid (0x14d5a, 0x14378) and choose the
   first prize (0x14967). */
void rc_module3_start(rc_reken *r);
const rc_grid *rc_module3_grid(int level);
/* A click on a cell (0x14967): RC_PICK_NONE when nothing happens,
   RC_PICK_REFUSED when the card shakes (an ANTWOORD first, or a second
   SOM), RC_PICK_TURNED when it turns over. */
enum { RC_PICK_NONE, RC_PICK_REFUSED, RC_PICK_TURNED };
int rc_module3_pick(rc_reken *r, int cell);
/* After the second card: 1 if the values match (the pair leaves the grid,
   the prize is added and, while cards are left, a new one chosen); 0 if
   not.  first and second are -1 again afterwards. */
int rc_module3_judge(rc_reken *r);

/* The generators, for tests: type 0 x, 1 :, 2 +, 3 - (0x4539c's codes). */
void rc_sum_make(rc_reken *r, int type);
void rc_module2_make(rc_reken *r);               /* 0x12844 */
void rc_module3_deal(rc_reken *r);               /* 0x14d5a, 0x14378 */
void rc_prize_choose(rc_reken *r);               /* 0x18e7f */
int rc_rand(rc_reken *r);                        /* 0x22bd2 */

#endif
