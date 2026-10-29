/* Headless check of the format readers against a game directory:
   opens every archive, parses every sprite, PCX, tileset and map.

   usage: rc_dump <data dir> [-v]          exit status 0 when all parse */
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "archive.h"
#include "gfx8.h"

static const char *const archives[] = {
    "INSTALL", "INSTMNU", "INTRO", "INTROSND", "MENU", "RC", "RCS",
    "REKEN", "REKENSND", "RRRANIM", "RRRMENU", "RRRMSND", "SNDINST",
    "SNDRCS", "XVGALIB",
};

static int ends_with(const char *s, const char *suf)
{
    size_t n = strlen(s), m = strlen(suf);
    return n >= m && strcasecmp(s + n - m, suf) == 0;
}

int main(int argc, char **argv)
{
    int verbose = argc > 2 && strcmp(argv[2], "-v") == 0;
    int errors = 0;
    size_t i;
    int k;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <data dir> [-v]\n", argv[0]);
        return 2;
    }
    for (i = 0; i < sizeof archives / sizeof *archives; i++) {
        rc_archive a;
        int sprites = 0, pcx = 0, other = 0;

        if (rc_archive_open(&a, argv[1], archives[i]) != 0) {
            printf("%-9s missing or malformed\n", archives[i]);
            errors++;
            continue;
        }
        for (k = 0; k < a.count; k++) {
            uint32_t size;
            const uint8_t *d = rc_archive_get(&a, k, &size);
            const char *n = a.entries[k].name;

            if (!a.named) {
                rc_sprite s;
                if (rc_sprite_parse(&s, d, size) == 0)
                    sprites++;
                else
                    other++;
            } else if (ends_with(n, ".pcx")) {
                rc_pcx p;
                if (rc_pcx_decode(&p, d, size) == 0) {
                    pcx++;
                    if (verbose)
                        printf("    %s %dx%d\n", n, p.w, p.h);
                    rc_pcx_free(&p);
                } else {
                    printf("    %s: bad PCX\n", n);
                    errors++;
                }
            } else {
                other++;
            }
        }
        printf("%-9s %-7s %4d entries: %d sprites, %d pcx, %d other\n",
               archives[i], a.named ? "named" : "indexed", a.count, sprites,
               pcx, other);
        rc_archive_close(&a);
    }

    {
        rc_archive rcs;
        if (rc_archive_open(&rcs, argv[1], "RCS") == 0) {
            for (k = 0; k < RC_NUM_MAPS; k++) {
                uint32_t ms, ts;
                const uint8_t *md = rc_archive_find(&rcs, rc_map_files[k], &ms);
                const uint8_t *td = rc_archive_find(&rcs, rc_map_tilesets[k],
                                                    &ts);
                rc_map m;
                rc_tileset t;
                if (!md || !td || rc_map_parse(&m, md, ms) != 0 ||
                    rc_tileset_parse(&t, td, ts) != 0) {
                    printf("map %2d %s: failed\n", k, rc_map_files[k]);
                    errors++;
                    continue;
                }
                if (verbose)
                    printf("map %2d %-12s %-10s %3dx%-3d\n", k,
                           rc_map_files[k], rc_map_tilesets[k], m.w, m.h);
                rc_map_free(&m);
            }
            printf("maps: %d checked\n", RC_NUM_MAPS);
            rc_archive_close(&rcs);
        }
    }
    printf(errors ? "%d errors\n" : "all ok\n", errors);
    return errors != 0;
}
