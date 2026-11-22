/* raceanim.c: the race's animation sets (0x15c02-0x167a9).
   init_anims loads the animation image file; each set (seven of them) has four functions that
   start or stop it (fn_1bd71 / fn_1bda1 in the animation library), with or without reloading
   rc.img/rc.ind. The check at the top of each is the library's "please call init_anims () first"
   guard with the original's line numbers (#line keeps __LINE__ as the game's). */

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
    fn_1bcb7();
    anims_inited = 1;
    return 0;
}

/* ---- 0x15c66 -------------------------------------------------------------------------------- */

int fn_15c66(void)
{
#line 1891 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bd71(anim_set_57620, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x15cf1 -------------------------------------------------------------------------------- */

int fn_15cf1(void)
{
#line 1906 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bd71(anim_set_57620, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x15d34 -------------------------------------------------------------------------------- */

int fn_15d34(void)
{
#line 1916 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bda1(anim_set_57620, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x15dbf -------------------------------------------------------------------------------- */

int fn_15dbf(void)
{
#line 1931 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bda1(anim_set_57620, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x15e02 -------------------------------------------------------------------------------- */

int fn_15e02(void)
{
#line 1941 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bd71(anim_set_57670, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x15e8d -------------------------------------------------------------------------------- */

int fn_15e8d(void)
{
#line 1956 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bd71(anim_set_57670, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x15ed0 -------------------------------------------------------------------------------- */

int fn_15ed0(void)
{
#line 1966 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bda1(anim_set_57670, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x15f5b -------------------------------------------------------------------------------- */

int fn_15f5b(void)
{
#line 1981 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bda1(anim_set_57670, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x15f9e -------------------------------------------------------------------------------- */

int fn_15f9e(void)
{
#line 1991 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bd71(anim_set_5767c, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x16029 -------------------------------------------------------------------------------- */

int fn_16029(void)
{
#line 2006 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bd71(anim_set_5767c, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x1606c -------------------------------------------------------------------------------- */

int fn_1606c(void)
{
#line 2016 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bda1(anim_set_5767c, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x160f7 -------------------------------------------------------------------------------- */

int fn_160f7(void)
{
#line 2031 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bda1(anim_set_5767c, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x1613a -------------------------------------------------------------------------------- */

int fn_1613a(void)
{
#line 2041 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bd71(anim_set_57684, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x161c5 -------------------------------------------------------------------------------- */

int fn_161c5(void)
{
#line 2056 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bd71(anim_set_57684, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x16208 -------------------------------------------------------------------------------- */

int fn_16208(void)
{
#line 2066 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bda1(anim_set_57684, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x16293 -------------------------------------------------------------------------------- */

int fn_16293(void)
{
#line 2081 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bda1(anim_set_57684, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x162d6 -------------------------------------------------------------------------------- */

int fn_162d6(void)
{
#line 2091 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bd71(anim_set_5768c, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x16361 -------------------------------------------------------------------------------- */

int fn_16361(void)
{
#line 2106 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bd71(anim_set_5768c, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x163a4 -------------------------------------------------------------------------------- */

int fn_163a4(void)
{
#line 2116 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bda1(anim_set_5768c, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x1642f -------------------------------------------------------------------------------- */

int fn_1642f(void)
{
#line 2131 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bda1(anim_set_5768c, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x16472 -------------------------------------------------------------------------------- */

int fn_16472(void)
{
#line 2141 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bd71(anim_set_57710, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x164fd -------------------------------------------------------------------------------- */

int fn_164fd(void)
{
#line 2156 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bd71(anim_set_57710, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x16540 -------------------------------------------------------------------------------- */

int fn_16540(void)
{
#line 2166 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bda1(anim_set_57710, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x165cb -------------------------------------------------------------------------------- */

int fn_165cb(void)
{
#line 2181 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bda1(anim_set_57710, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x1660e -------------------------------------------------------------------------------- */

int fn_1660e(void)
{
#line 2191 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bd71(anim_set_57754, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x16699 -------------------------------------------------------------------------------- */

int fn_16699(void)
{
#line 2206 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bd71(anim_set_57754, anim_list);
    fn_2670e(anim_buf);
    return 0;
}

/* ---- 0x166dc -------------------------------------------------------------------------------- */

int fn_166dc(void)
{
#line 2216 "raceanim.c"
    ANIMS_CHECK();
    fn_2670e(anim_buf);
    fn_1bcb7();
    fn_1bda1(anim_set_57754, anim_list);
    if (fn_2643f(fn_218e4(anim_img_path, "rc.img"), fn_218e4(anim_ind_path, "rc.ind"), anim_buf)) {
        fn_2670e(anim_buf);
        return 2000;
    }
    return 0;
}

/* ---- 0x16767 -------------------------------------------------------------------------------- */

int fn_16767(void)
{
#line 2231 "raceanim.c"
    ANIMS_CHECK();
    fn_1bcb7();
    fn_1bda1(anim_set_57754, anim_list);
    fn_2670e(anim_buf);
    return 0;
}
