/* IND/IMG archives (reverse-engineering/file-formats.md). */
#ifndef RC_ARCHIVE_H
#define RC_ARCHIVE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char name[21];          /* named archives; "" in indexed ones */
    uint32_t offset, size;
} rc_entry;

typedef struct {
    int named;
    int count;
    rc_entry *entries;
    uint8_t *img;
    size_t img_size;
} rc_archive;

/* Open DIR/BASE.IND + BASE.IMG, matching the names case-insensitively.
   Returns 0 on success. */
int rc_archive_open(rc_archive *a, const char *dir, const char *base);
void rc_archive_close(rc_archive *a);

/* Entry bytes by name (named archives) or index; NULL if absent. */
const uint8_t *rc_archive_find(const rc_archive *a, const char *name,
                               uint32_t *size);
const uint8_t *rc_archive_get(const rc_archive *a, int index, uint32_t *size);

/* fopen() DIR/NAME ignoring case, as DOS did. */
void *rc_fopen_nocase(const char *dir, const char *name, const char *mode);

#endif
