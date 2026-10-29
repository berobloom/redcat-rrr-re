/* %8.8d.SMD: a 255-byte name, a record count, then the records at a stride
   of 0x5b bytes of which only the first 27 are used.  REKEN appends 27
   bytes and on the next append rewrites the earlier records at the full
   stride, so all records but the last fill 0x5b bytes; the bytes after the
   27 were whatever the buffer held, zeros here. */
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "smd.h"

static int stricmp_ascii(const char *a, const char *b)  /* REKEN 0x1a807 */
{
    for (;; a++, b++) {
        int x = tolower((unsigned char)*a), y = tolower((unsigned char)*b);
        if (x != y || !y)
            return x - y;
    }
}

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

static int is_smd(const char *f)
{
    size_t n = strlen(f);
    return n > 4 && stricmp_ascii(f + n - 4, ".smd") == 0;
}

static uint8_t *read_all(const char *path, long *size)
{
    FILE *f = fopen(path, "rb");
    uint8_t *d = NULL;
    long n;

    if (!f)
        return NULL;
    if (fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) >= 0 &&
        fseek(f, 0, SEEK_SET) == 0 && (d = malloc((size_t)n + 1)) &&
        fread(d, 1, (size_t)n, f) != (size_t)n) {
        free(d);
        d = NULL;
    }
    fclose(f);
    *size = d ? n : 0;
    return d;
}

static void parse(rc_smd *p, const uint8_t *d, long size)
{
    int i, n = size >= RC_SMD_NAME + 4 ? get32(d + RC_SMD_NAME) : 0;

    if (n <= 0 || n > 100000)
        return;
    p->rec = calloc((size_t)n, sizeof *p->rec);
    if (!p->rec)
        return;
    p->count = n;
    for (i = 0; i < n; i++) {
        uint8_t r[RC_SMD_STRIDE] = { 0 };   /* a short read leaves zeros */
        long at = RC_SMD_NAME + 4 + (long)i * RC_SMD_STRIDE;
        if (at < size)
            memcpy(r, d + at, (size_t)(size - at < RC_SMD_STRIDE ?
                                       size - at : RC_SMD_STRIDE));
        p->rec[i].module = get32(r);
        p->rec[i].asked = get32(r + 4);
        p->rec[i].correct = get32(r + 8);
        memcpy(p->rec[i].date, r + 12, 11);
        p->rec[i].date[10] = 0;
        p->rec[i].groep = get32(r + 23);
    }
}

/* smd_find: the files in directory order; when none matches, one past
   the number of the last file seen.  Here: one past the highest. */
int rc_smd_load(rc_smd *p, const char *dir, const char *name)
{
    DIR *dp = opendir(dir);
    struct dirent *e;
    int highest = 0;

    memset(p, 0, sizeof *p);
    if (!dp) {
        p->number = 1;
        return 0;
    }
    while ((e = readdir(dp))) {
        char path[1024], stored[RC_SMD_NAME + 1];
        uint8_t *d;
        long size;
        int num;

        if (!is_smd(e->d_name))
            continue;
        num = atoi(e->d_name);
        if (num > highest)
            highest = num;
        snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
        if (!(d = read_all(path, &size)))
            continue;
        memset(stored, 0, sizeof stored);
        memcpy(stored, d, (size_t)(size < RC_SMD_NAME ? size : RC_SMD_NAME));
        if (stricmp_ascii(stored, name) == 0) {
            p->number = num;
            parse(p, d, size);
            free(d);
            closedir(dp);
            return 0;
        }
        free(d);
    }
    closedir(dp);
    p->number = highest + 1;
    return 0;
}

void rc_smd_free(rc_smd *p)
{
    free(p->rec);
    memset(p, 0, sizeof *p);
}

/* smd_append (REKEN 0x19f07) */
int rc_smd_append(const char *dir, const char *name, int module, int asked,
                  int correct, int groep, const char *date)
{
    rc_smd p;
    char path[1024];
    uint8_t head[RC_SMD_NAME + 4] = { 0 }, r[RC_SMD_STRIDE];
    FILE *f;
    int i, ok;

    rc_smd_load(&p, dir, name);
    snprintf(path, sizeof path, "%s/%8.8d.SMD", dir, p.number);
    if (!(f = fopen(path, "wb"))) {
        rc_smd_free(&p);
        return -1;
    }
    strncpy((char *)head, name, RC_SMD_NAME - 1);
    put32(head + RC_SMD_NAME, p.count + 1);
    ok = fwrite(head, sizeof head, 1, f) == 1;
    for (i = 0; i <= p.count && ok; i++) {
        const rc_smd_record *q = i < p.count ? &p.rec[i] : NULL;
        memset(r, 0, sizeof r);
        put32(r, q ? q->module : module);
        put32(r + 4, q ? q->asked : asked);
        put32(r + 8, q ? q->correct : correct);
        strncpy((char *)r + 12, q ? q->date : date, 10);
        put32(r + 23, q ? q->groep : groep);
        ok = fwrite(r, q ? RC_SMD_STRIDE : RC_SMD_WRITTEN, 1, f) == 1;
    }
    if (fclose(f) != 0)
        ok = 0;
    rc_smd_free(&p);
    return ok ? 0 : -1;
}

/* 0x19cb6: day, month and year of localtime(), the month as tm_mon */
void rc_smd_date(char out[11])
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    char s[32];

    snprintf(s, sizeof s, "%2.2d/%2.2d/%4d", tm->tm_mday, tm->tm_mon,
             tm->tm_year + 1900);
    memcpy(out, s, 10);
    out[10] = 0;
}

int rc_smd_last(const rc_smd *p, int module)
{
    int i;

    for (i = p->count - 1; i >= 0; i--)
        if (p->rec[i].module == module)
            return p->rec[i].asked ? p->rec[i].correct * 100 /
                p->rec[i].asked : 0;
    return -1;
}

int rc_smd_total(const rc_smd *p, int module, int n)
{
    int i, k = 0, asked = 0, correct = 0, limit = p->count;

    if (!p->count)
        return -1;
    if (n > 0 && n < limit)
        limit = n;
    for (i = 0; i < p->count; i++)
        if (p->rec[i].module == module && k < limit) {
            k++;
            correct += p->rec[i].correct;
            asked += p->rec[i].asked;
        }
    return asked ? correct * 100 / asked : 0;
}

int rc_smd_count(const rc_smd *p, int module)
{
    int i, k = 0;

    for (i = 0; i < p->count; i++)
        k += p->rec[i].module == module;
    return k;
}
