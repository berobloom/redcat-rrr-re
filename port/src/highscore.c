#include <stdio.h>
#include <string.h>

#include "highscore.h"

static const char *const racers[3] = { "RedCat", "Brutus", "Snitch" };

static int32_t get32(const uint8_t *d)
{
    return (int32_t)(d[0] | d[1] << 8 | d[2] << 16 | (uint32_t)d[3] << 24);
}

static void put32(uint8_t *d, int32_t v)
{
    d[0] = (uint8_t)v;
    d[1] = (uint8_t)(v >> 8);
    d[2] = (uint8_t)(v >> 16);
    d[3] = (uint8_t)(v >> 24);
}

static void fields(rc_high *h)
{
    int i;

    for (i = 0; i < RC_HIGH_ENTRIES; i++) {
        const uint8_t *r = h->raw + i * RC_HIGH_SIZE;
        memcpy(h->e[i].name, r, RC_HIGH_NAME);
        h->e[i].name[RC_HIGH_NAME] = 0;
        h->e[i].score = get32(r + 0x1e);
        h->e[i].world = get32(r + 0x22);
    }
}

/* strcpy into the entry: only the name and its NUL change, so a shorter
   name leaves the old one's tail behind (the file keeps it) */
static void set_name(rc_high *h, int i, const char *name)
{
    size_t n = strlen(name);

    if (n > RC_HIGH_NAME - 1)
        n = RC_HIGH_NAME - 1;
    memcpy(h->raw + i * RC_HIGH_SIZE, name, n);
    h->raw[i * RC_HIGH_SIZE + n] = 0;
}

void rc_high_load(rc_high *h, const char *dir, int (*rnd)(void *), void *ctx)
{
    char path[1100];
    FILE *f;
    int i;

    memset(h, 0, sizeof *h);
    snprintf(path, sizeof path, "%s/RRRHIGH.DAT", dir);
    if ((f = fopen(path, "rb"))) {
        size_t n = fread(h->raw, 1, sizeof h->raw, f);
        (void)n;                /* a short file: the rest stays zero */
        fclose(f);
    } else {
        for (i = 0; i < RC_HIGH_ENTRIES; i++) {
            set_name(h, i, racers[rnd(ctx) % 3]);
            put32(h->raw + i * RC_HIGH_SIZE + 0x1e, RC_HIGH_ENTRIES - i);
            put32(h->raw + i * RC_HIGH_SIZE + 0x22, rnd(ctx) % 6);
        }
        set_name(h, 0, racers[0]);
    }
    fields(h);
}

int rc_high_add(rc_high *h, const char *dir, const char *name, int score,
                int world)
{
    char path[1100];
    FILE *f;
    int i, place;

    for (place = 0; place < RC_HIGH_ENTRIES; place++)
        if (h->e[place].score < score)
            break;
    if (place == RC_HIGH_ENTRIES)
        return -1;
    /* 0x1b72a: each entry down one, name by strcpy, score and world */
    for (i = RC_HIGH_ENTRIES - 1; i > place; i--) {
        set_name(h, i, h->e[i - 1].name);
        memcpy(h->raw + i * RC_HIGH_SIZE + 0x1e,
               h->raw + (i - 1) * RC_HIGH_SIZE + 0x1e, 8);
    }
    set_name(h, place, name);
    put32(h->raw + place * RC_HIGH_SIZE + 0x1e, score);
    put32(h->raw + place * RC_HIGH_SIZE + 0x22, world);
    fields(h);
    snprintf(path, sizeof path, "%s/RRRHIGH.DAT", dir);
    if (!(f = fopen(path, "r+b")) && !(f = fopen(path, "wb")))
        return place;
    if (fwrite(h->raw, sizeof h->raw, 1, f) != 1)
        fprintf(stderr, "cannot write %s\n", path);
    fclose(f);
    return place;
}
