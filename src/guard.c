/* guard.c (our name; the original's is not known): 0x1b469-0x1b6bf. check_object_nr's and
   check_waypoint_nr's strings open a CONST of their own (0x51a5c, after padding behind rrrio's
   "RRRIO.INF") and end on a 4-byte boundary, 0x51a98, where hiscore's high_names strings begin:
   Watcom puts an initialiser's literals ahead of every function's, so in one object they would
   come first, and the game has the guards' strings first. set_keys and joystick_to_keys have no
   strings; they are here because hiscore's code starts at high_score, 0x1b6bf, and an object's code
   is contiguous. Whether they open this module or close the one before is not known. */

#include "rrr.h"

/* ---- 0x1b469 -------------------------------------------------------------------------------- */

void set_keys(void)
{
    if (rrrio.joystick && fg_initjoy(1) == 0) {
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
        b = fg_button(1);
        x = fg_getxjoy(1);
        y = fg_getyjoy(1);
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
