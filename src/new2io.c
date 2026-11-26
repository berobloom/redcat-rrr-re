/*
 * new2io.c (the game's own name, from its error strings): drawing the race view, 0x1150e-0x1195e
 * (draw_view's jump table first, at 0x1150e, right after main.c's last ret). Its strings start on
 * their own boundary (0x50c30, main.c's end at 0x50c2f).
 */
#include "rrr.h"

/* ---- 0x11526 -------------------------------------------------------------------------------- */

/* differs: 51 rows, register homes only - and they do not move. Tile loop: the game keeps col in
   ESI and px in ECX (ours the other way round, which costs `mov edx,ecx` before draw_tile); in the
   clip it holds &sprites[n] in EBP/EDI and spills cx, ox and x + w to slots; in the blit arms it
   spills ww and cx where we hold ox, oy and ww in registers. Matched on the way: the tile loops as
   do-while, tx/ty/sx/sy and the stack locals in the game's declaration order, mode 4 and 5 falling
   through into 6 (no break: a mode-4 sprite is blended three times). Tried, all in place
   (inplace.py, .work/probe/dv2): 128 tile-loop x clip x cx-mutations (strength-reduced px, hoisted
   row base, increment orders, reversed clip tests, hoisted y+h/x+w and x/y/w/h locals, ?: forms),
   72 declaration-order swaps and shuffles (nothing: 51 rows, the levers' "declaration order acts
   only through ties"), pointer locals (&sprites[n], used everywhere or only in the clip: 167-268
   rows, the game re-materialises n << 4 per block), hoisted blit subexpressions (102-212), and nine
   `#pragma aux draw_tile` forms (parm registers or narrower modify lists: 64-143). The pragma
   probes also say the game's draw_tile call was plain push-based cdecl - no register parameters.
   Both sides have 263 instructions, frame 0x2c and 11 spill slots, so the frame is not the
   constraint; the game spends its three pushed registers on col/py/temps and we spend them on
   px/ox/oy, and no spelling shifts the tie (watcom-levers "9.5b frame and evaluation-order rules",
   (b): IR counts, not C counts).
   2026-11-26, the start_lights lever (how many C variables carry the values): every merge of a
   sprite-loop local (cx/cy/ox/oy/ww/hh/n/e) into a tile-loop one (row/col/px/py/tx/ty) costs
   80-200 rows (48 singles, .work/probe/dv3); a tile-value local, tx/ty in the tile index and
   moving the sx/sy saves give 62-176 (dv4). */
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
