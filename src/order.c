/*
 * order.c (the game's own name, from its error strings): the draw-order list, 0x1195f-0x11bb3 (right after
 * new2io.c's draw_view; blit_sprite, assembly, follows at 0x11bb4). A module of its own: its strings start on their own boundary and its functions share no
 * code with main.c's (GOTCHAS, "One object per module").
 */
#include "rrr.h"

/* ---- 0x1195f ------------------------------------------------------------------------------ */

int order_clear(void)
{
    int i;
    for (i = 0; i < 306; i++) {
        order[i].sprite = -1;
        order[i].next = -1;
        order[i].key = -1;
        order[i].prev = -1;
    }
    order_head = -1;
    return 0;
}

/* ---- 0x119a0 -------------------------------------------------------------------------------- */

int order_alloc(void)
{
    int i;
    for (i = 0; i < 306; i++) {
        if (order[i].sprite == -1) {
            order[i].sprite = -2;
            return i;
        }
    }
    return -1;
}

/* ---- 0x119ce -------------------------------------------------------------------------------- */

int order_insert_before(int e, int n)
{
    int p;
    p = order[e].prev;
    if (p != -1)
        order[p].next = n;
    if (e == order_head)
        order_head = n;
    order[n].next = e;
    order[n].prev = p;
    order[e].prev = n;
    return 0;
}

/* ---- 0x11a1a -------------------------------------------------------------------------------- */

int order_add(int sprite, int key)
{
    int prev, e, n;

    prev = -1;
    e = order_head;
    n = order_alloc();
    if (n < 0)
        rc_error(" %s(%d) : illegal free order element", "order.c", 71);
    if (e != -1) {
        for (e = order_head; e >= 0; e = order[e].next) {
            if (key > order[e].key) {
                order_insert_before(e, n);
                order[n].sprite = sprite;
                order[n].key = key;
                return 1;
            }
            prev = e;
        }
        if (prev != -1) {
            order[prev].next = n;
            order[n].next = -1;
            order[n].prev = prev;
            order[n].sprite = sprite;
            order[n].key = key;
            return 1;
        }
    } else {
        order[n].sprite = sprite;
        order[n].key = key;
        order[n].prev = -1;
        order_head = n;
        order[n].next = -1;
        return 1;
    }
    return 0;
}

/* ---- 0x11ae3 -------------------------------------------------------------------------------- */

int order_has(int sprite)
{
    int e;
    for (e = order_head; e != -1; e = order[e].next)
        if (order[e].sprite == sprite)
            return 1;
    return 0;
}

/* ---- 0x11b0e -------------------------------------------------------------------------------- */

int sort_draw_order(void)
{
    int i;
    order_clear();
    for (i = 0; i < 306; i++)
        if (!order_add(i, (1000 - (bodies[i].x >> 16)) * 10 + (bodies[i].y >> 16) * -1000
                          + (bodies[i].z >> 16) * -1 - bodies[i].f1c))
            rc_error(" %s(%d) : illegal add_to_order", "order.c", 135);
    for (i = 0; i < 306; i++)
        if (!order_has(i))
            rc_error(" %s(%d) : illegal order, %d missing", "order.c", 141, i);
    return 0;
}
