/* raceanim.c: the race's animation sets (0x15c02-0x167a9).
   init_anims loads the animation image file. Each of the seven sets (a -1-terminated list of anim
   numbers) has four functions: load_<set> and free_<set> page its RC images in or out, and the
   _half pair does the same after giving every odd frame its even neighbour's image, so half the
   images are needed. The game calls only nine of the 28. The check at the top of each is the
   library's "please call init_anims () first" guard with the original's line numbers (#line keeps
   __LINE__ as the game's). */

#include "rrr.h"

#define ANIMS_CHECK() \
    if (!anims_inited) \
        rc_error(" %s(%d) : please call init_anims () first", "raceanim.c", __LINE__)

/* ---- 0x15c02 -------------------------------------------------------------------------------- */

int init_anims(void)
{
    if (fn_25a9d(anim_list))
        rc_error("not enough memory for animations");
    if (fn_26782(fn_218e4(anim_img_path, "rc.img"), 0))
        rc_error("not enough memory for animation data", anim_err);
    mem_images_reset();
    anims_inited = 1;
    return 0;
}

/* ---- 0x15c66 -------------------------------------------------------------------------------- */

int load_object_anims(void)
{
#line 1891 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set(object_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x15cf1 -------------------------------------------------------------------------------- */

int free_object_anims(void)
{
#line 1906 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set(object_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x15d34 -------------------------------------------------------------------------------- */

int load_object_anims_half(void)
{
#line 1916 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set_half(object_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x15dbf -------------------------------------------------------------------------------- */

int free_object_anims_half(void)
{
#line 1931 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set_half(object_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x15e02 -------------------------------------------------------------------------------- */

int load_rc_drive_anims(void)
{
#line 1941 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set(rc_drive_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x15e8d -------------------------------------------------------------------------------- */

int free_rc_drive_anims(void)
{
#line 1956 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set(rc_drive_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x15ed0 -------------------------------------------------------------------------------- */

int load_rc_drive_anims_half(void)
{
#line 1966 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set_half(rc_drive_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x15f5b -------------------------------------------------------------------------------- */

int free_rc_drive_anims_half(void)
{
#line 1981 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set_half(rc_drive_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x15f9e -------------------------------------------------------------------------------- */

int load_bull_drive_anims(void)
{
#line 1991 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set(bull_drive_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x16029 -------------------------------------------------------------------------------- */

int free_bull_drive_anims(void)
{
#line 2006 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set(bull_drive_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x1606c -------------------------------------------------------------------------------- */

int load_bull_drive_anims_half(void)
{
#line 2016 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set_half(bull_drive_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x160f7 -------------------------------------------------------------------------------- */

int free_bull_drive_anims_half(void)
{
#line 2031 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set_half(bull_drive_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x1613a -------------------------------------------------------------------------------- */

int load_muis_drive_anims(void)
{
#line 2041 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set(muis_drive_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x161c5 -------------------------------------------------------------------------------- */

int free_muis_drive_anims(void)
{
#line 2056 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set(muis_drive_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x16208 -------------------------------------------------------------------------------- */

int load_muis_drive_anims_half(void)
{
#line 2066 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set_half(muis_drive_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x16293 -------------------------------------------------------------------------------- */

int free_muis_drive_anims_half(void)
{
#line 2081 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set_half(muis_drive_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x162d6 -------------------------------------------------------------------------------- */

int load_rc_crash_anims(void)
{
#line 2091 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set(rc_crash_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x16361 -------------------------------------------------------------------------------- */

int free_rc_crash_anims(void)
{
#line 2106 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set(rc_crash_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x163a4 -------------------------------------------------------------------------------- */

int load_rc_crash_anims_half(void)
{
#line 2116 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set_half(rc_crash_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x1642f -------------------------------------------------------------------------------- */

int free_rc_crash_anims_half(void)
{
#line 2131 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set_half(rc_crash_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x16472 -------------------------------------------------------------------------------- */

int load_bull_crash_anims(void)
{
#line 2141 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set(bull_crash_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x164fd -------------------------------------------------------------------------------- */

int free_bull_crash_anims(void)
{
#line 2156 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set(bull_crash_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x16540 -------------------------------------------------------------------------------- */

int load_bull_crash_anims_half(void)
{
#line 2166 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set_half(bull_crash_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x165cb -------------------------------------------------------------------------------- */

int free_bull_crash_anims_half(void)
{
#line 2181 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set_half(bull_crash_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x1660e -------------------------------------------------------------------------------- */

int load_muis_crash_anims(void)
{
#line 2191 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set(muis_crash_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x16699 -------------------------------------------------------------------------------- */

int free_muis_crash_anims(void)
{
#line 2206 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set(muis_crash_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}

/* ---- 0x166dc -------------------------------------------------------------------------------- */

int load_muis_crash_anims_half(void)
{
#line 2216 "raceanim.c"
    ANIMS_CHECK();
    disable_mem_images(mem_images);
    mem_images_reset();
    mem_images_add_set_half(muis_crash_anims, anim_list);
    if (load_mem_images(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), mem_images)) {
        disable_mem_images(mem_images);
        return 2000;
    }
    return 0;
}

/* ---- 0x16767 -------------------------------------------------------------------------------- */

int free_muis_crash_anims_half(void)
{
#line 2231 "raceanim.c"
    ANIMS_CHECK();
    mem_images_reset();
    mem_images_add_set_half(muis_crash_anims, anim_list);
    disable_mem_images(mem_images);
    return 0;
}
