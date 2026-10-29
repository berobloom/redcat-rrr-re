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
#define RC_BIG_LINES 4          /* module 4: work lines shown */
#define RC_BIG_HISTORY 64       /* module 4: sums TERUG can undo (port limit) */

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
    int32_t speech;             /* +0x30: the menu's sound mode, 3 / 1 / 0 */
    int32_t stat18, stat1c;     /* +0x18, +0x1c: the player's stats +0x10, +0x14 */
    int32_t race_done;          /* +0x1f8 */
    int32_t order[3];           /* +0x1fc: finishing order, object numbers */
    int32_t progress[3];        /* +0x208: each racer's next waypoint */
    int32_t points[3];          /* +0x214: per racer, 3 / 2 / 1 a race */
    uint8_t raw[RC_SESSION_SIZE];   /* the whole file, other fields kept */
} rc_session;

void rc_session_default(rc_session *s);         /* 0x10ac9's defaults */
/* 0 if the bytes are a valid record (version 3), else defaults and -1. */
int rc_session_parse(rc_session *s, const uint8_t *d, uint32_t size);
void rc_session_store(rc_session *s);           /* fields back into raw */

/* The module played after this stage (0x10cad): 1..4, or -1 when the world
   is done.  With take set, it becomes the new stage. */
int rc_next_module(rc_session *s, int take);

/* REKEN's choice (0x10903): rc_next_module taken, module 4 played as 3 by
   groep 3; -1 sends the pupil back to the menu. */
int rc_pick_module(rc_session *s);

/* Other fields of the record, by offset (+0x34 joystick, ...). */
int32_t rc_session_get(const rc_session *s, int offset);
void rc_session_set(rc_session *s, int offset, int32_t v);

/* RRRMENU 0x1099b, a world (re)started: repair 100, the start order 0, 2, 4,
   no waypoints or points. */
void rc_world_reset(rc_session *s);

/* RRRMENU at start-up (0x10f95): after the last race of a world, the next
   world (after 5: 0) from stage 0 with repair 100, the start order 0, 2, 4
   and no points (0x1099b).  1 if it moved on. */
int rc_world_next(rc_session *s);

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

/* Module 4 "Grote getallenspel": operators [0x45c2c] */
enum { RC_OP_NONE = -1, RC_OP_TIMES, RC_OP_DIVIDE, RC_OP_PLUS, RC_OP_MINUS };

typedef struct {                /* 0x5bcf4, 16-byte records */
    int a, b, ia, ib;
} rc_big_step;

typedef struct {
    uint32_t seed;              /* Watcom rand() state */
    int level;                  /* [0x46c84]: groep 6,7,8 -> 0,1,2; 3,4,5 */
    int speech;                 /* [0x46c87] (RRRIO +0x30) */
    int fuel, repair, boosts, rockets;
    int prize;
    /* REKENSND sample numbers of what the drawn rand() picks to say: the
       prize (0x18e7f) and "goed" / "fout" (0x1910f, 0x19471) */
    int prize_say, verdict_say;
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
    /* module 4 */
    int count;                  /* [0x45c30]: 4 numbers, 3 for groep 4, 5 */
    int num[4], start[4];       /* 0x5bf94 (-1 used), 0x5bfb4 */
    int target;                 /* [0x5c104] */
    int a, b, ia, ib, op;       /* [0x5bf84], [0x5bf88], [0x5bf74], [0x5bf78] */
    int entered, want_b;        /* 0x165fb's step and ebp */
    char line[RC_BIG_HISTORY][50];  /* 0x5bfc4, 51-byte records +0x32 flag */
    int line_on[RC_BIG_HISTORY];
    int lines;                  /* [0x45c38] */
    rc_big_step step[RC_BIG_HISTORY];
    int steps;                  /* [0x45c28] */
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

/* Module 4 "Grote getallenspel": the next round's numbers and target, and
   its prize (0x17c57, 0x17b95).  Returns 0 after the last round. */
int rc_module4_next(rc_reken *r);
/* The buttons (0x165fb): a number slot, an operator, '=', TERUG and
   OPNIEUW.  Returns RC_BIG_NONE (an empty slot), RC_BIG_REFUSED (the
   error sound), RC_BIG_TAKEN, or after '=' with one number left
   RC_BIG_RIGHT / RC_BIG_WRONG (the round is over and the prize, if any,
   added). */
enum { RC_BIG_NONE, RC_BIG_REFUSED, RC_BIG_TAKEN, RC_BIG_RIGHT, RC_BIG_WRONG };
int rc_module4_number(rc_reken *r, int slot);
int rc_module4_op(rc_reken *r, int op);
int rc_module4_equals(rc_reken *r);
int rc_module4_back(rc_reken *r);               /* TERUG */
void rc_module4_again(rc_reken *r);             /* OPNIEUW */
/* The sum being entered, as 0x161d3 shows it ("a op b "). */
void rc_module4_entry(const rc_reken *r, char *s, int size);

/* The generators, for tests: type 0 x, 1 :, 2 +, 3 - (0x4539c's codes). */
void rc_sum_make(rc_reken *r, int type);
void rc_module2_make(rc_reken *r);               /* 0x12844 */
void rc_module3_deal(rc_reken *r);               /* 0x14d5a, 0x14378 */
void rc_module4_make(rc_reken *r);               /* 0x17c57's round */
void rc_prize_choose(rc_reken *r);               /* 0x18e7f */
int rc_rand(rc_reken *r);                        /* 0x22bd2 */

#endif
