/* hiscore.c (our name; the original's is not known): 0x1b67b-0x1b915, the last module of the
   game's own C. A module of its own: its strings start after padding behind rrrio's
   "RRRIO.INF", and inside rrr.c Watcom merged high_score's and high_load's epilogues with
   other functions' (5 rows each), which the game's code does not. */

#include "rrr.h"

/* ---- 0x1b67b -------------------------------------------------------------------------------- */

int check_object_nr(int n, char *file, int line)
{
    if (n < 0 || n > 255)
        rc_error(" %s(%d) : objectnr %d error\n", file, line, n);
    return n;
}

/* ---- 0x1b69d -------------------------------------------------------------------------------- */

int check_waypoint_nr(int n, char *file, int line)
{
    if (n < 0 || n > 150)
        rc_error(" %s(%d) : Waypointnr %d error\n", file, line, n);
    return n;
}

/* ---- 0x1b6bf -------------------------------------------------------------------------------- */

void high_score(char *name, int score)
{
    int i, j;
    FILE *f;

    high_load();
    debug_printf("Save score %d for %s\n", score, name);
    for (i = 0; i < 10; i++) {
        if (high[i].score < score) {
            debug_printf("New place = %d\n", i);
            for (j = 9; j > i; j--) {
                debug_printf("%d to %d\n", j - 1, j);
                strcpy(high[j].name, high[j - 1].name);
                high[j].score = high[j - 1].score;
                high[j].world = high[j - 1].world;
            }
            strcpy(high[i].name, name);
            high[i].score = score;
            high[i].world = rrrio.world;
            f = fopen("RRRHIGH.DAT", "r+b");
            if (f == NULL) {
                f = fopen("RRRHIGH.DAT", "wb");
                if (f == NULL)
                    return;
            }
            fwrite(high, 1, sizeof(high), f);
            fclose(f);
            return;
        }
    }
}

/* ---- 0x1b82c -------------------------------------------------------------------------------- */

int high_load(void)
{
    FILE *f;
    int i;

    f = fopen("RRRHIGH.DAT", "rb");
    if (f == NULL) {
        for (i = 0; i < 10; i++) {
            strcpy(high[i].name, high_names[rand() % 3]);
            high[i].score = 10 - i;
            high[i].world = rand() % 6;
        }
        strcpy(high[0].name, high_names[0]);
        return 0;
    }
    fread(high, 1, sizeof(high), f);
    fclose(f);
    return 1;
}

