/* hiscore.c (our name; the original's is not known): 0x1b469-0x1b915, the last module of the
   game's own C. Its strings start after padding behind rrrio's "RRRIO.INF" (with the guards at
   0x1b67b), and inside rrr.c Watcom merged high_score's and high_load's epilogues with other
   functions' (5 rows each), which the game's code does not. set_keys and joystick_to_keys have
   no strings, so whether they open this module or close the one before is not known; they
   match in either. */

#include "rrr.h"

/* ---- 0x1b469 -------------------------------------------------------------------------------- */

void set_keys(void)
{
    if (rrrio.joystick && fn_2779e(1) == 0) {
        joy_on = 1;
        keymap[5] = 0x68;
        keymap[4] = 0x69;
        keymap[3] = 0x66;
        joymap[2] = rrrio.joy[0];
        keymap[6] = 0x67;
        joymap[1] = rrrio.joy[4];
        keymap[1] = 0x64;
        joymap[0] = rrrio.joy[5];
        keymap[0] = 0x65;
        joymap[3] = rrrio.joy[1];
        return;
    }
    keymap[5] = rrrio.keys[4];
    keymap[4] = rrrio.keys[5];
    keymap[3] = rrrio.keys[2];
    keymap[6] = rrrio.keys[3];
    keymap[1] = rrrio.keys[0];
    keymap[0] = rrrio.keys[1];
}


/* ---- 0x1b536 -------------------------------------------------------------------------------- */

void joystick_to_keys(void)
{
    int b, x, y;

    if (rrrio.joystick && joy_on) {
        b = fn_27837(1);
        x = fn_2784e(1);
        y = fn_278a4(1);
        keystate[keymap[1]] = 0;
        keystate[keymap[0]] = 0;
        keystate[keymap[3]] = 0;
        keystate[keymap[6]] = 0;
        keystate[keymap[5]] = 0;
        keystate[keymap[4]] = 0;
        if (rrrio.f224 & b)
            keystate[keymap[1]] = 1;
        if (b & rrrio.f220)
            keystate[keymap[0]] = 1;
        if (x < joymap[2])
            keystate[keymap[3]] = 1;
        if (x > joymap[1])
            keystate[keymap[6]] = 1;
        if (y < joymap[3])
            keystate[keymap[5]] = 1;
        if (y > joymap[0])
            keystate[keymap[4]] = 1;
    }
}


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

