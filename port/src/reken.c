/* The arithmetic part (REKEN.EXE): see reken.h.

   The generators follow the disassembly call for call, rand() included, so
   a seed gives the original's sums (checked against the original code under
   Unicorn by port/tools/check_reken.py). */
#include <stdio.h>
#include <string.h>

#include "reken.h"

/* 0x4539c: sum types per level, -1 = unused slot */
static const int8_t sum_types[6][4] = {
    { 0, 1, 2, 3 }, { 0, 1, 2, 3 }, { 0, 1, 2, 3 },
    { 2, 3, -1, -1 }, { 0, 2, 3, -1 }, { 0, 1, 2, 3 },
};

/* 0x451aa: word lists the generators index at different starts */
static const int16_t words[26] = {
    1, -1, 5, 10, 15, 100, 100, 100, 100, 100, 5, 10, 5, 10, 5, 10, 5, 10,
    5, 10, 10, 100, 100, 100, 100, 100,
};
#define W_SIGN   (words + 0)    /* 0x451aa */
#define W_MULT   (words + 2)    /* 0x451ae */
#define W_DIV0   (words + 10)   /* 0x451be */
#define W_DIV1   (words + 11)   /* 0x451c0 */
#define W_SUB    (words + 20)   /* 0x451d2 */
#define W_SUB2   (words + 21)   /* 0x451d4 */

/* 0x45388: small factors for level 0 products */
static const uint8_t small_factor[20] = {
    2, 3, 2, 4, 6, 5, 7, 8, 9, 4, 5, 6, 2, 5, 6, 3, 2, 4, 5, 3,
};

typedef struct { int lo, hi, n; } range1;
typedef struct { int lo1, hi1, lo2, hi2, n, n2; } range2;

/* 0x452b0, rows of 9 bytes: row 2L for one operand, 2L + 1 for the other */
static const range1 add_rows[12] = {
    { 3, 10, 2 }, { 20, 90, 2 }, { 3, 20, 2 }, { 20, 200, 2 },
    { 8, 30, 2 }, { 20, 200, 3 }, { 2, 6, 1 }, { 1, 6, 1 },
    { 2, 10, 1 }, { 1, 10, 1 }, { 2, 30, 1 }, { 1, 30, 1 },
};
/* 0x4531c, rows of 18 bytes */
static const range2 mul_rows[6] = {
    { 2, 10, 2, 50, 2, 0 }, { 2, 8, 1, 40, 2, 0 }, { 2, 8, 1, 40, 3, 0 },
    { 1, 10, 1, 10, 1, 0 }, { 1, 10, 1, 10, 1, 0 }, { 3, 16, 1, 10, 1, 0 },
};
/* 0x4524a, rows of 17 bytes */
static const range2 div_rows[6] = {
    { 1, 6, 2, 10, 2, 0 }, { 8, 20, 4, 20, 3, 0 }, { 8, 20, 4, 20, 3, 0 },
    { 1, 4, 1, 5, 1, 0 }, { 1, 4, 1, 5, 1, 0 }, { 2, 8, 2, 6, 1, 0 },
};
/* 0x451de, rows of 18 bytes */
static const range2 sub_rows[6] = {
    { 2, 10, 2, 10, 2, 0 }, { 1, 30, 1, 30, 2, 2 }, { 1, 30, 1, 30, 3, 4 },
    { 3, 10, 3, 10, 1, 1 }, { 3, 10, 3, 10, 1, 1 }, { 3, 30, 3, 30, 1, 1 },
};

/* 0x46c8c, 0x46cac: prizes when repair is full / when fuel or repair is low */
static const uint8_t prize_full[16] = { 5, 6, 5, 1, 4, 0, 6, 1, 6, 4, 5, 6, 0, 1, 1, 1 };
static const uint8_t prize_low[16] = { 3, 6, 3, 5, 3, 6, 6, 6, 1, 3, 5, 3, 3, 5, 6, 3 };

/* 0x45000: exercise modules 1..4 per world */
static const uint8_t world_modules[6][4] = {
    { 1, 0, 1, 0 }, { 0, 1, 1, 0 }, { 1, 1, 1, 0 },
    { 1, 0, 1, 1 }, { 0, 1, 1, 1 }, { 1, 1, 1, 1 },
};

/* 0x44f5c: map number + 1 per world and stage, 0 = no race */
static const uint8_t stage_maps[6][5] = {
    { 1, 2, 0, 3, 0 }, { 4, 0, 5, 6, 0 }, { 7, 8, 9, 10, 0 },
    { 11, 12, 0, 13, 14 }, { 15, 0, 16, 17, 18 }, { 19, 20, 21, 22, 23 },
};

int rc_stage_map(int world, int stage)
{
    if (world < 0 || world > 5 || stage < 0 || stage > 4)
        return -1;
    return stage_maps[world][stage] - 1;
}

int rc_rand(rc_reken *r)
{
    r->seed = r->seed * 0x41c64e6du + 0x3039;
    return (r->seed >> 16) & 0x7fff;
}

/* 0x111e0 */
static int between(rc_reken *r, int lo, int hi)
{
    return lo + rc_rand(r) % (hi - lo);
}

static int iabs(int v) { return v < 0 ? -v : v; }

static void sum_add(rc_reken *r, int L)         /* 0x1122e */
{
    if (L >= 3) {
        int a = between(r, add_rows[2 * L].lo, add_rows[2 * L].hi);
        int b = between(r, add_rows[2 * L + 1].lo, add_rows[2 * L + 1].hi);
        snprintf(r->text, sizeof r->text, "%d + %d = ", a, b);
        r->answer = a + b;
    } else if (rc_rand(r) % 2 == 0) {
        /* two round numbers, each split in two, the four parts shuffled */
        const range1 *row = &add_rows[2 * L];
        int v1 = W_MULT[rc_rand(r) % row->n] * between(r, row->lo, row->hi);
        int v2 = W_MULT[rc_rand(r) % row->n] * between(r, row->lo, row->hi);
        int p1 = v1 - (rc_rand(r) % (v1 - 5) + 2), p2 = iabs(p1 - v1);
        int q1 = v2 - (rc_rand(r) % (v2 - 5) + 2), q2 = iabs(q1 - v2);
        int t[4];

        switch (rc_rand(r) % 4) {
        case 0: t[0] = p2; t[1] = q1; t[2] = q2; t[3] = p1; break;
        case 1: t[0] = q1; t[1] = p2; t[2] = p1; t[3] = q2; break;
        case 2: t[0] = q2; t[1] = p1; t[2] = q1; t[3] = p2; break;
        default: t[0] = p1; t[1] = q2; t[2] = p2; t[3] = q1; break;
        }
        snprintf(r->text, sizeof r->text, "%d + %d + %d + %d = ",
                 t[0], t[1], t[2], t[3]);
        r->answer = v1 + v2;
    } else {
        /* one round number split in two */
        const range1 *row = &add_rows[2 * L + 1];
        int v = W_MULT[rc_rand(r) % row->n] * between(r, row->lo, row->hi);
        int b = v - (rc_rand(r) % (v - 5) + 2);
        int a = iabs(v - b);

        snprintf(r->text, sizeof r->text, "%d + %d = ", a, b);
        r->answer = a + b;
    }
}

static void sum_mul(rc_reken *r, int L)         /* 0x11496 */
{
    const range2 *row = &mul_rows[L];
    int a, b;

    switch (L) {
    case 0:
        a = small_factor[rc_rand(r) % 20];
        b = W_MULT[rc_rand(r) % row->n];
        b *= between(r, row->lo2, row->hi2);
        break;
    case 1:
    case 2:
        a = between(r, row->lo1, row->hi1);
        a += between(r, row->lo1, row->hi1);
        b = W_MULT[rc_rand(r) % row->n];
        b *= between(r, row->lo2, row->hi2);
        break;
    case 4:
    case 5:
        a = between(r, row->lo1, row->hi1);
        b = between(r, row->lo2, row->hi2);
        if (b < 1)
            b = 1;
        break;
    default:
        return;                 /* level 3 has no products */
    }
    snprintf(r->text, sizeof r->text, "%d x %d = ", a, b);
    r->answer = a * b;
}

static void sum_div(rc_reken *r, int L)         /* 0x11619 */
{
    const range2 *row = &div_rows[L];
    int a, b, m;

    switch (L) {
    case 0:
        a = between(r, row->lo1, row->hi1);
        b = between(r, row->lo2, row->hi2);
        m = W_DIV0[rc_rand(r) % row->n];
        a *= m;
        break;
    case 1:
    case 2:
        a = between(r, row->lo1, row->hi1);
        m = W_DIV1[rc_rand(r) % row->n];
        b = between(r, row->lo2, row->hi2) * m;
        break;
    case 4:
    case 5:
        a = between(r, row->lo1, row->hi1);
        b = between(r, row->lo2, row->hi2);
        break;
    default:
        return;                 /* level 3 has no divisions */
    }
    snprintf(r->text, sizeof r->text, "%d : %d = ", a * b, b);
    r->answer = a;
}

static void sum_sub(rc_reken *r, int L)         /* 0x1179e */
{
    const range2 *row = &sub_rows[L];
    int a, b, k;

    switch (L) {
    case 0:
        a = W_SUB[rc_rand(r) % row->n];
        a *= between(r, row->lo1, row->hi1);
        b = W_SUB[rc_rand(r) % row->n];
        b *= between(r, row->lo2, row->hi2);
        if (a == b)
            b += 5;
        break;
    case 1:
    case 2:
        rc_rand(r);
        rc_rand(r);
        a = (L == 1 ? W_SUB : W_SUB2)[rc_rand(r) % row->n];
        a *= between(r, row->lo1, row->hi1);
        k = rc_rand(r) % row->n2;
        k *= W_SIGN[rc_rand(r) % 2];
        if (L == 1)
            k *= rc_rand(r) % 2;
        b = W_SUB[rc_rand(r) % row->n];
        b = b * between(r, row->lo2, row->hi2) - k;
        if (a == b)
            b -= 5;
        break;
    default:
        a = between(r, row->lo1, row->hi1);
        b = between(r, row->lo2, row->hi2);
        if (a == b)
            b -= rc_rand(r) % 2 + 1;
        break;
    }
    if (a > b) {
        snprintf(r->text, sizeof r->text, "%d - %d = ", a, b);
        r->answer = a - b;
    } else {
        snprintf(r->text, sizeof r->text, "%d - %d = ", b, a);
        r->answer = b - a;
    }
}

void rc_sum_make(rc_reken *r, int type)
{
    switch (type) {
    case 0: sum_mul(r, r->level); break;
    case 1: sum_div(r, r->level); break;
    case 2: sum_add(r, r->level); break;
    case 3: sum_sub(r, r->level); break;
    }
}

void rc_prize_choose(rc_reken *r)
{
    if (r->repair == 100)
        r->prize = prize_full[rc_rand(r) % 16];
    if (r->fuel < 60 || r->repair != 100)
        r->prize = prize_low[rc_rand(r) % 16];
    if (r->repair == 0)
        r->prize = RC_PRIZE_REPAIR_FULL;
    if (r->prize == RC_PRIZE_REPAIR && r->repair == 100)
        r->prize = rc_rand(r) % 2;
    if (r->prize == RC_PRIZE_REPAIR_FULL && r->repair == 100)
        r->prize = rc_rand(r) % 2;
    if (r->prize == RC_PRIZE_ROCKETS && r->rockets == 99)
        r->prize = RC_PRIZE_BOOSTS;
    if (r->prize == RC_PRIZE_BOOSTS && r->boosts == 99)
        r->prize = RC_PRIZE_ROCKETS;
    if (r->prize <= RC_PRIZE_REPAIR)
        rc_rand(r);             /* which spoken phrase goes with it */
}

static void prize_award(rc_reken *r)            /* 0x1910f */
{
    static const int fuel_units[3] = { 5, 10, 15 };
    int i;

    switch (r->prize) {
    case RC_PRIZE_BOOSTS:
        for (i = 0; i < 5; i++)
            if (r->boosts < 99)
                r->boosts++;
        break;
    case RC_PRIZE_ROCKETS:
        for (i = 0; i < 5; i++)
            if (r->rockets < 99)
                r->rockets++;
        break;
    case RC_PRIZE_REPAIR_FULL:
        r->repair = 100;
        break;
    case RC_PRIZE_REPAIR:
        if (r->repair < 100) {
            r->repair += 30;
            if (r->repair > 100)
                r->repair = 100;
        }
        break;
    default:
        for (i = 0; i < fuel_units[r->prize - RC_PRIZE_FUEL5]; i++)
            if (r->fuel < 150)
                r->fuel++;
        break;
    }
}

int rc_module1_next(rc_reken *r)
{
    int n = 0, i;

    if (r->asked >= RC_SUMS_PER_ROUND)
        return 0;
    for (i = 0; i < 4; i++)
        if (sum_types[r->level][i] != -1)
            n++;
    rc_sum_make(r, sum_types[r->level][rc_rand(r) % n]);
    rc_prize_choose(r);
    r->asked++;
    return 1;
}

static int answered(rc_reken *r, int right)
{
    rc_rand(r);                 /* the spoken "goed" or "fout" (0x1910f, 0x19471) */
    if (right) {
        prize_award(r);
        r->correct++;
    }
    return right;
}

int rc_reken_answer(rc_reken *r, int answer)
{
    return answered(r, answer == r->answer);
}

/* Module 2 "Wat is waar?" */

/* 0x45480: the results' range per level */
static const int16_t result_range[6][2] = {
    { 15, 50 }, { 15, 100 }, { 15, 150 }, { 2, 9 }, { 2, 9 }, { 2, 9 },
};

/* 0x454b0: the three sums' types (0x4539c's codes) per level, five rows */
static const uint8_t choice_types[6][5][3] = {
    { { 0, 1, 2 }, { 1, 0, 3 }, { 3, 1, 0 }, { 2, 1, 3 }, { 2, 0, 3 } },
    { { 0, 1, 2 }, { 1, 0, 3 }, { 3, 1, 0 }, { 2, 1, 3 }, { 3, 0, 2 } },
    { { 0, 1, 2 }, { 1, 0, 3 }, { 3, 1, 0 }, { 2, 1, 3 }, { 3, 0, 2 } },
    { { 2, 3, 2 }, { 3, 2, 3 }, { 2, 3, 2 }, { 3, 3, 2 }, { 3, 2, 2 } },
    { { 0, 2, 3 }, { 2, 0, 3 }, { 3, 0, 2 }, { 2, 0, 3 }, { 3, 0, 2 } },
    { { 0, 1, 2 }, { 1, 0, 3 }, { 3, 1, 0 }, { 2, 1, 3 }, { 3, 0, 2 } },
};

#define TRIES 4000

static int result_between(rc_reken *r)          /* 0x1202a */
{
    return between(r, result_range[r->level][0], result_range[r->level][1]);
}

/* Each generator writes choice i and its value.  The target R is
   choice_value[0] (0x53be4): the drawn result for choice 0, which then
   holds choice 0's own value, so choices 1 and 2 are made around that. */

static void choice_add(rc_reken *r, int i)      /* 0x12068 */
{
    int R = r->choice_value[0], a, b;

    if (r->level >= 3) {
        a = rc_rand(r) % R + 1;
        b = iabs(R - a + rc_rand(r) % 3 + 1);
    } else {
        a = iabs(R / 2 - (rc_rand(r) % 4 + 1));
        b = iabs(R / 2 - (rc_rand(r) % 4 + 1));
    }
    snprintf(r->choice[i], sizeof r->choice[i], "%d + %d", a, b);
    r->choice_value[i] = a + b;
}

/* 0x121d9: a product within tol of R, but not R */
static void near_product(rc_reken *r, int *a, int *b, int R, int tol)
{
    int x = 0, y = 0, n, d;

    for (n = 0; n < TRIES; n++) {
        x = result_between(r);
        y = result_between(r);
        d = iabs(x * y - R);
        if (d != 0 && d <= tol) {
            *a = x;
            *b = y;
            return;
        }
    }
    *a = x + 1;
    *b = y;
}

static void choice_mul(rc_reken *r, int i)      /* 0x12288 */
{
    int R = r->choice_value[0], a, b;

    if (r->level >= 3) {
        near_product(r, &a, &b, R, 1);
    } else {
        a = rc_rand(r) % 20 + 1;
        b = R / a;
        if (b < 5)
            b += rc_rand(r) % 4 + 1;
        else
            b -= rc_rand(r) % 4 + 1;
    }
    snprintf(r->choice[i], sizeof r->choice[i], "%d x %d", a, b);
    r->choice_value[i] = a * b;
}

/* 0x123cf: a division a : b in the result range whose quotient is within
   tol of R, but not R */
static void near_quotient(rc_reken *r, int *a, int *b, int R, int tol)
{
    int lo = result_range[r->level][0], hi = result_range[r->level][1];
    int qlo = R - tol, qhi = R + tol, n, q, d;

    if ((unsigned)qlo < (unsigned)lo)           /* unsigned compares */
        qlo = lo;
    if ((unsigned)qhi > (unsigned)hi)
        qhi = hi;
    /* qhi < qlo (R past hi + tol) still draws, from qlo up (idiv keeps the
       remainder's sign).  qhi == qlo divides by zero in the original, and a
       span holding only R loops forever there; the port takes the fallback
       for both. */
    n = qhi == qlo || (iabs(qhi - qlo) == 1 && qlo == R) ? TRIES : 0;
    for (; n < TRIES; n++) {
        do
            q = qlo + rc_rand(r) % (qhi - qlo);
        while (q == R);
        d = result_between(r);
        if (q * d >= lo && q * d <= hi) {
            *a = q * d;
            *b = d;
            return;
        }
    }
    *a = R == 4 ? 9 : 8;
    *b = R == 4 ? 3 : 2;
}

static void choice_div(rc_reken *r, int i)      /* 0x124ec */
{
    int R = r->choice_value[0], a, b;

    if (r->level >= 3) {
        near_quotient(r, &a, &b, R, 2);
    } else {
        b = rc_rand(r) % 6 + 1;
        a = iabs(R - (rc_rand(r) % 4 + 1)) * b;
    }
    snprintf(r->choice[i], sizeof r->choice[i], "%d : %d", a, b);
    r->choice_value[i] = a / b;
}

static void choice_sub(rc_reken *r, int i)      /* 0x12614 */
{
    int R = r->choice_value[0], a, b;

    if (r->level >= 3) {
        a = rc_rand(r) % R + 1;
        b = iabs(rc_rand(r) % 3 + (R - a) + 1);
        if (a == b)
            a++;
        if (a > b && i != 0 && a - b == R)
            a++;
        else if (a < b && i != 0 && b - a == R)
            b++;
    } else {
        a = iabs(R - (rc_rand(r) % 4 + 1));
        b = rc_rand(r) % 100 + 20;
        a += b;
        if (a == b)
            a -= 5;
    }
    if (a > b) {
        snprintf(r->choice[i], sizeof r->choice[i], "%d - %d", a, b);
        r->choice_value[i] = a - b;
    } else {
        snprintf(r->choice[i], sizeof r->choice[i], "%d - %d", b, a);
        r->choice_value[i] = b - a;
    }
}

void rc_module2_make(rc_reken *r)
{
    const uint8_t *types;
    int i;

    result_between(r);          /* [0x53bd4], [0x53bd8]: overwritten */
    result_between(r);
    r->choice_value[0] = result_between(r);
    types = choice_types[r->level][rc_rand(r) % 5];
    for (i = 0; i < 3; i++)
        switch (types[i]) {
        case 0: choice_mul(r, i); break;
        case 1: choice_div(r, i); break;
        case 2: choice_add(r, i); break;
        case 3: choice_sub(r, i); break;
        }
}

int rc_module2_next(rc_reken *r)
{
    int i;

    if (r->asked >= RC_SUMS_PER_ROUND)
        return 0;
    rc_module2_make(r);
    rc_prize_choose(r);
    r->true_row = rc_rand(r) % 3;
    /* groep 6..8 read "a op b = R" in each row; groep 3..5 the bare sums,
       with R in a box beside them */
    if (r->level <= 2)
        for (i = 0; i < 3; i++) {
            size_t n = strlen(r->choice[i]);
            snprintf(r->choice[i] + n, sizeof r->choice[i] - n, " = %2d",
                     r->choice_value[0]);
        }
    r->answer = r->choice_value[0];
    r->asked++;
    return 1;
}

int rc_module2_choice(const rc_reken *r, int row)
{
    return row == r->true_row ? 0 : row == 0 ? r->true_row : row;
}

int rc_module2_answer(rc_reken *r, int row)
{
    return answered(r, row == r->true_row);
}

/* The session record */

static int32_t get32(const uint8_t *p)
{
    return (int32_t)(p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24);
}

static void put32(uint8_t *p, int32_t v)
{
    p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = (uint32_t)v >> 24;
}

static void session_fields(rc_session *s)
{
    s->version = get32(s->raw);
    s->repair = get32(s->raw + 0x04);
    s->boosts = get32(s->raw + 0x0c);
    s->rockets = get32(s->raw + 0x14);
    s->fuel = get32(s->raw + 0x20);
    s->stage = get32(s->raw + 0x24);
    s->groep = get32(s->raw + 0x28);
    s->world = get32(s->raw + 0x2c);
    s->speech = get32(s->raw + 0x30);
    s->race_done = get32(s->raw + 0x1f8);
}

void rc_session_store(rc_session *s)
{
    put32(s->raw, s->version);
    put32(s->raw + 0x04, s->repair);
    put32(s->raw + 0x0c, s->boosts);
    put32(s->raw + 0x14, s->rockets);
    put32(s->raw + 0x20, s->fuel);
    put32(s->raw + 0x24, s->stage);
    put32(s->raw + 0x28, s->groep);
    put32(s->raw + 0x2c, s->world);
    put32(s->raw + 0x30, s->speech);
    put32(s->raw + 0x1f8, s->race_done);
}

void rc_session_default(rc_session *s)
{
    static const int32_t keys[6] = { 0x1c, 0x36, 0x4b, 0x4d, 0x48, 0x50 };
    int i;

    memset(s->raw, 0, sizeof s->raw);
    put32(s->raw, 3);
    put32(s->raw + 0x04, 100);
    put32(s->raw + 0x0c, 5);
    put32(s->raw + 0x14, 10);
    put32(s->raw + 0x20, 75);
    put32(s->raw + 0x28, 6);
    put32(s->raw + 0x30, 3);
    for (i = 0; i < 6; i++)
        put32(s->raw + 0x38 + 4 * i, -1);
    for (i = 0; i < 6; i++)
        put32(s->raw + 0x50 + 4 * i, keys[i]);
    put32(s->raw + 0x200, 2);   /* start order 0, 2, 4 */
    put32(s->raw + 0x204, 4);
    session_fields(s);
}

int rc_session_parse(rc_session *s, const uint8_t *d, uint32_t size)
{
    if (size < RC_SESSION_SIZE || get32(d) != 3) {
        rc_session_default(s);
        return -1;
    }
    memcpy(s->raw, d, RC_SESSION_SIZE);
    session_fields(s);
    return 0;
}

int rc_next_module(rc_session *s, int take)
{
    int m = s->stage;

    if (s->race_done || s->stage < 1) {
        for (m = s->stage + 1; m <= 4; m++)
            if (world_modules[s->world][m - 1])
                break;
        if (m > 4 || m < 1)
            return -1;
        if (take)
            s->stage = m;
    }
    return m;
}

static int groep_level(int groep)
{
    return groep >= 6 && groep <= 8 ? groep - 6 : groep >= 3 && groep <= 5 ? groep : 0;
}

void rc_reken_start(rc_reken *r, const rc_session *s, uint32_t seed)
{
    memset(r, 0, sizeof *r);
    r->seed = seed;
    r->level = groep_level(s->groep);           /* 0x198c7 */
    r->fuel = s->fuel;
    r->repair = s->repair < 0 ? 0 : s->repair;
    r->boosts = s->boosts;
    r->rockets = s->rockets;
}

void rc_reken_finish(const rc_reken *r, rc_session *s)
{
    s->fuel = r->fuel;
    s->repair = r->repair;
    s->boosts = r->boosts;
    s->rockets = r->rockets;
    s->groep = r->level <= 2 ? r->level + 6 : r->level;
    rc_session_store(s);
}
