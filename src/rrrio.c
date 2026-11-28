/* rrrio.c (our name; the original's is not known): 0x1ab7f-0x1af52, from rrrio_save to
   ai_script_find; crashmuis.c's muis_crash_tick ends at 0x1ab7f, and ai.c's two switch tables start
   at 0x1af53. Its CONST runs 0x51984-0x51a59, from just after map.c's padding up to guard.c's: first
   the literals of the two name tables below (Watcom puts initialisers' literals ahead of the
   functions'), then "wb", "RRRIO.INF", "rb". Its _DATA holds the tables at 0x57c04, right after
   stage_table (0x57b8c, 120 B; whether that is this module's too is not known). */

#include "rrr.h"

/* 0x57c04: the four REKEN games' names, 18 chars padded, and a blank one; nothing in RRR reads it */
char *game_names[5] = {
    "Handig rekenen    ", "Wat is waar?      ", "Memory            ", "Grote getallenspel",
    "                  "
};

/* 0x57c18 */
char *world_names[6] = {
    "De Welkomstrace ", "De Rotsvlakte   ", "De Snelweg      ", "De Puzzelkloof  ",
    "De Cactusvallei ", "De Laatste Kans "
};

/* ---- 0x1ab7f -------------------------------------------------------------------------------- */

void rrrio_save(void)
{
    FILE *f = fopen("RRRIO.INF", "wb");
    fwrite(&rrrio, 0x228, 1, f);
    fclose(f);
}

/* ---- 0x1abb6 -------------------------------------------------------------------------------- */

int rrrio_load(void)
{
    FILE *f;
    int i;

    f = fopen("RRRIO.INF", "rb");
    if (f != NULL) {
        fread(&rrrio, 0x228, 1, f);
        if (rrrio.version == 3) {
            fclose(f);
            rrrio.race_done = 0;
            return 1;
        }
    }
    rrrio.version = 3;
    rrrio.damage = 100;
    rrrio.boosts = 5;
    rrrio.f10 = 0;
    rrrio.rockets = 10;
    rrrio.f18 = 0;
    rrrio.f1c = 0;
    rrrio.fuel = 75;
    rrrio.stage = 0;
    rrrio.groep = 6;
    rrrio.world = 0;
    rrrio.sound = 3;
    rrrio.joystick = 0;
    rrrio.joy[0] = -1;
    rrrio.joy[1] = -1;
    rrrio.joy[4] = -1;
    rrrio.joy[5] = -1;
    rrrio.joy[2] = -1;
    rrrio.joy[3] = -1;
    rrrio.keys[0] = 0x1c;
    rrrio.keys[1] = 0x36;
    rrrio.keys[2] = 0x4b;
    rrrio.keys[3] = 0x4d;
    rrrio.keys[4] = 0x48;
    rrrio.keys[5] = 0x50;
    for (i = 0; i < 4; i++)
        rrrio.names[i][0] = 0;
    rrrio.race_done = 0;
    rrrio.start_order[0] = 0;
    rrrio.start_order[1] = 2;
    rrrio.start_order[2] = 4;
    rrrio.f208[0] = 0;
    rrrio.f208[1] = 0;
    rrrio.f208[2] = 0;
    rrrio.points[0] = 0;
    rrrio.points[1] = 0;
    rrrio.points[2] = 0;
    rrrio.f220 = 0;
    rrrio.f224 = 0;
    rrrio_save();
    return 0;
}

/* ---- 0x1ad9a -------------------------------------------------------------------------------- */

int next_module(int set, int world, int groep)
{
    int i;

    if (set || world == -1)
        world = rrrio.world;
    if (set || groep == -1)
        groep = rrrio.groep;
    i = rrrio.stage;
    if (!rrrio.race_done && i > 0)
        return i;
    do
        i++;
    while (i <= 4 && module_table[world][i - 1] == 0);
    if (i > 4 || i < 1)
        return -1;
    if (i == 4 && groep == 3)
        i = groep;
    if (set)
        rrrio.stage = i;
    return i;
}


/* ---- 0x1ae20 -------------------------------------------------------------------------------- */

int next_module_set(void)
{
    return next_module(1, -1, -1);
}


/* ---- 0x1ae36 -------------------------------------------------------------------------------- */

int next_module_of(int world, int groep)
{
    return next_module(0, world, groep);
}


/* ---- 0x1ae44 -------------------------------------------------------------------------------- */

int stage_map_index(void)
{
    return stage_table[rrrio.world][rrrio.stage] - 1;
}

/* ---- 0x1ae67 -------------------------------------------------------------------------------- */

/* the spelling matters: a `for (i = stage; i < 5; i++) if (...) break;` is rotated by -ol and
   tests at the bottom; the game tests at the top */
int stage_is_last(void)
{
    int i = rrrio.stage;
    while (i <= 4 && stage_table[rrrio.world][i] == 0)
        i++;
    return i == 4;
}

/* ---- 0x1aea5 -------------------------------------------------------------------------------- */

int stage_is_first(void)
{
    int i = rrrio.stage;
    while (i > 0 && stage_table[rrrio.world - 1][i] != 0)
        i--;
    return i == rrrio.stage;
}

/* ---- 0x1aedf -------------------------------------------------------------------------------- */

int ai_script_find(int racer, int n)
{
    int i;

    if (racer == 2) {
        for (i = 0; i < 150; i++) {
            if (script2[rrrio.world][i].key == -1)
                break;
            if (n < script2[rrrio.world][i].key)
                break;
            if (n == script2[rrrio.world][i].key)
                return i;
        }
    } else if (racer == 4) {
        for (i = 0; i < 150; i++) {
            if (script4[rrrio.world][i].key == -1)
                break;
            if (n < script4[rrrio.world][i].key)
                break;
            if (n == script4[rrrio.world][i].key)
                return i;
        }
    }
    return -1;
}
