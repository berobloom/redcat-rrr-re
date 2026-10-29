/* RRR.EXE's animation table (0x57798), REKEN.EXE's (0x4660c) and RRRMENU's
   (0x457bc), generated into anims_tab.c, menu_anims_tab.c and
   rrrmenu_anims_tab.c by port/tools/gen_anims.py.  Race
   frame kinds: 1, 2 opaque; 4, 5, 6 drawn through the TRANS.DAT blend
   table.  Image i is RC.IMG (race), MENU.IMG (REKEN) or RRRANIM.IMG
   (RRRMENU) entry i - 1. */
#ifndef RC_ANIMS_H
#define RC_ANIMS_H

typedef struct {
    short kind, image;
} rc_frame;

typedef struct {
    short count, flag;      /* flag: 1 in every entry; meaning unknown */
    const rc_frame *frames;
} rc_anim;

#define RC_ANIM_COUNT 87

extern const rc_anim rc_anims[RC_ANIM_COUNT];

#define RC_MENU_ANIM_COUNT 59

extern const rc_anim rc_menu_anims[RC_MENU_ANIM_COUNT];

#define RC_RRRMENU_ANIM_COUNT 9

extern const rc_anim rc_rrrmenu_anims[RC_RRRMENU_ANIM_COUNT];

#endif
