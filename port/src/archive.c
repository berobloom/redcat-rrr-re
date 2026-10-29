#include "archive.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

void *rc_fopen_nocase(const char *dir, const char *name, const char *mode)
{
    char path[1024];
    DIR *d = opendir(dir);
    struct dirent *e;
    FILE *f = NULL;

    if (!d)
        return NULL;
    while ((e = readdir(d)) != NULL) {
        if (strcasecmp(e->d_name, name) == 0) {
            snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
            f = fopen(path, mode);
            break;
        }
    }
    closedir(d);
    return f;
}

static uint8_t *slurp(const char *dir, const char *name, size_t *size)
{
    FILE *f = rc_fopen_nocase(dir, name, "rb");
    uint8_t *buf;
    long n;

    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = malloc(n ? n : 1);
    if (buf && fread(buf, 1, n, f) != (size_t)n) {
        free(buf);
        buf = NULL;
    }
    fclose(f);
    *size = n;
    return buf;
}

static uint32_t rd32(const uint8_t *p)
{
    return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24;
}

int rc_archive_open(rc_archive *a, const char *dir, const char *base)
{
    char name[64];
    size_t ind_size;
    uint8_t *ind;
    int i;

    memset(a, 0, sizeof *a);
    snprintf(name, sizeof name, "%s.ind", base);
    ind = slurp(dir, name, &ind_size);
    if (!ind || ind_size < 2)
        goto fail;
    snprintf(name, sizeof name, "%s.img", base);
    a->img = slurp(dir, name, &a->img_size);
    if (!a->img)
        goto fail;

    a->count = ind[0] | ind[1] << 8;
    if (ind_size == 2 + 24 * (size_t)a->count)
        a->named = 1;
    else if (ind_size != 2 + 4 * (size_t)a->count)
        goto fail;
    a->entries = calloc(a->count ? a->count : 1, sizeof *a->entries);
    for (i = 0; i < a->count; i++) {
        rc_entry *e = &a->entries[i];
        if (a->named) {
            const uint8_t *r = ind + 2 + 24 * i;
            memcpy(e->name, r, 20);         /* NUL inside; junk after it */
            e->name[20] = 0;
            e->offset = rd32(r + 20);
        } else {
            e->offset = rd32(ind + 2 + 4 * i);
        }
    }
    for (i = 0; i < a->count; i++) {
        uint32_t end = i + 1 < a->count ? a->entries[i + 1].offset
                                        : (uint32_t)a->img_size;
        if (a->entries[i].offset > end || end > a->img_size)
            goto fail;
        a->entries[i].size = end - a->entries[i].offset;
    }
    free(ind);
    return 0;
fail:
    free(ind);
    rc_archive_close(a);
    return -1;
}

void rc_archive_close(rc_archive *a)
{
    free(a->entries);
    free(a->img);
    memset(a, 0, sizeof *a);
}

const uint8_t *rc_archive_get(const rc_archive *a, int index, uint32_t *size)
{
    if (index < 0 || index >= a->count)
        return NULL;
    if (size)
        *size = a->entries[index].size;
    return a->img + a->entries[index].offset;
}

const uint8_t *rc_archive_find(const rc_archive *a, const char *name,
                               uint32_t *size)
{
    int i;

    for (i = 0; i < a->count; i++)
        if (strcasecmp(a->entries[i].name, name) == 0)
            return rc_archive_get(a, i, size);
    return NULL;
}
