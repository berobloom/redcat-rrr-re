/*
 * new2io.c (the game's own name, from its error strings): drawing the race view, 0x1150e-0x1195e
 * (draw_view's jump table first, at 0x1150e, right after main.c's last ret). Its strings start on
 * their own boundary (0x50c30, main.c's end at 0x50c2f).
 */
#include "rrr.h"

/* ---- 0x11526 -------------------------------------------------------------------------------- */

/* differs: 51 rows, all register and stack-slot choice, 6 bytes short of the game: in the tile
   loop the game keeps col in ESI and px in ECX (ours the other way round); in the clip it puts
   &sprites[n] in EBP/EDI and spills cx, x + w and w to the stack, ours keeps them in registers.
   Matched on the way: the tile loops as do-while, tx/ty/sx/sy and the stack locals in the game's
   declaration order, mode 4 and 5 falling through into 6 (no break: a mode-4 sprite is blended
   three times). Tried: strength-reduced col * 32 (exact size, 124 rows), reusing the sprite
   variables in the tile loop, w/x locals and ?: in the width clip, operand orders, increment
   orders. */
void draw_view(short dy, short dx)
{
    int cx, hh, ww, e, sx, ty, tx, sy, n, ox, oy, cy, row, col, px, py;

    tx = tile_x;
    ty = tile_y;
    sx = scroll_x;
    sy = scroll_y;
    row = 0;
    py = 0;
    do {
        px = 0;
        col = 0;
        do {
            draw_tile(map[((tile_y + row) * map_w + (tile_x + col)) * 6], px, py);
            col++;
            px += 32;
        } while (col <= view_tiles_w);
        row++;
        py += 40;
    } while (row <= view_tiles_h);
    scroll_x = tx * 32;
    scroll_y = ty * 40;
    e = order_head;
    if (e < 0)
        rc_error(" %s(%d) : illegal ordering.", "new2io.c", 71);
    do {
        n = order[e].sprite;
        e = order[e].next;
        if (n < 0 || n > num_sprites)
            rc_error(" %s(%d) : illegal sprite %d, order element %d.", "new2io.c", 80, n, e);
        cx = sprites[n].x - scroll_x;
        if (cx < 0) {
            ox = -cx;
            if (ox >= sprites[n].w)
                continue;
            cx = 0;
        } else
            ox = 0;
        cy = sprites[n].y - scroll_y;
        if (cy < 0) {
            oy = -cy;
            if (oy >= sprites[n].h)
                continue;
            cy = 0;
        } else
            oy = 0;
        if (scroll_y + (view_tiles_h + 1) * 40 > sprites[n].y + sprites[n].h)
            hh = sprites[n].h;
        else
            hh = scroll_y + (view_tiles_h + 1) * 40 - sprites[n].y;
        if ((view_tiles_w + 1) * 32 + scroll_x > sprites[n].x + sprites[n].w)
            ww = sprites[n].w;
        else
            ww = (view_tiles_w + 1) * 32 + scroll_x - sprites[n].x;
        if (ww <= 0 || hh <= 0)
            continue;
        switch (sprites[n].mode) {
        case 1:
        case 2:
            blit_sprite(screen_buf, sprites[n].img + ox + sprites[n].w * oy, cx, cy, ww - ox, hh - oy,
                        sprites[n].w - (ww - ox), screen_w);
            break;
        case 4:
            blit_sprite_trans(screen_buf, sprites[n].img + ox + sprites[n].w * oy, trans, cx, cy, ww - ox,
                              hh - oy, sprites[n].w - (ww - ox), screen_w);
        case 5:
            blit_sprite_trans(screen_buf, sprites[n].img + ox + sprites[n].w * oy, trans, cx, cy, ww - ox,
                              hh - oy, sprites[n].w - (ww - ox), screen_w);
        case 6:
            blit_sprite_trans(screen_buf, sprites[n].img + ox + sprites[n].w * oy, trans, cx, cy, ww - ox,
                              hh - oy, sprites[n].w - (ww - ox), screen_w);
            break;
        default:
            rc_error(" %s (%d) : unknown type %d sprite %d", "new2io.c", 165, sprites[n].mode, n);
        }
    } while (e >= 0);
    scroll_x = sx;
    scroll_y = sy;
    switch (view_size) {
    case 0:
        present_view(dx, dx + 600, dy, dy + 400, 20, 400);
        break;
    case 1:
        present_view(dx, dx + 540, dy, dy + 360, 50, 380);
        break;
    case 2:
        present_view(dx, dx + 480, dy, dy + 320, 80, 360);
        break;
    }
}
