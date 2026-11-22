/*
 * shade.c (the game's own name, from its error strings): the palette blend table for translucent
 * sprites, 0x11c11-0x11eb4. blit_sprite at 0x11bb4, just before it, is assembly (enter, NOP
 * padding, all registers pushed), not this compiler's output.
 */
#include "rrr.h"

/* ---- 0x11c11 -------------------------------------------------------------------------------- */

unsigned char nearest_colour(unsigned char r, unsigned char g, unsigned char b)
{
    short i, best_i, best, d;
    short dr, dg, db;

    best = 0x7fff;
    for (i = 0; i < 256; i++) {
        dr = abs(palette[i][0] - r);
        dg = abs(palette[i][1] - g);
        db = abs(palette[i][2] - b);
        d = dr * dr + dg * dg + db * db;
        if (d < best) {
            best = d;
            best_i = i;
        }
    }
    return best_i;
}

/* ---- 0x11ca2 -------------------------------------------------------------------------------- */

void trans_generate(void)
{
    short b, a, g, r;

    trans = malloc(0xffff);
    if (!trans)
        rc_error("not enough memory for transparenct sprites, choos lower speed level");
    for (a = 0; a < 255; a++) {
        for (b = 0; b < 255; b++) {
            if (b == a) {
                trans[a * 256 + b] = a;
            } else {
                r = (palette[a][0] + palette[b][0]) / 2;
                g = (palette[a][1] + palette[b][1]) / 2;
                trans[a * 256 + b] = nearest_colour(r, g, (palette[a][2] + palette[b][2]) / 2);
            }
        }
        trans[a * 256 + 255] = a;
    }
}

/* ---- 0x11db8 -------------------------------------------------------------------------------- */

int trans_save(char *name)
{
    FILE *f;
    short a, b;

    f = fopen(name, "wb");
    if (!f)
        return 1000;
    for (a = 0; a < 255; a++)
        for (b = 0; b < 255; b++)
            fwrite(&trans[a * 256 + b], 1, 1, f);
    fclose(f);
    return 0;
}

/* ---- 0x11e2a -------------------------------------------------------------------------------- */

short trans_load(char *name)
{
    FILE *f;
    short a, b;

    f = fopen(name, "rb");
    if (!f)
        return 1000;
    trans = malloc(0xffff);
    if (!trans)
        rc_error(" %s(%d) : not enough memory for transparent sprites", "shade.c", 90);
    for (a = 0; a < 255; a++)
        for (b = 0; b < 255; b++)
            fread(&trans[a * 256 + b], 1, 1, f);
    fclose(f);
    return 0;
}
