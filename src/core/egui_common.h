#ifndef _EGUI_COMMON_H_
#define _EGUI_COMMON_H_

#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "egui_predefs.h"
#include "config/egui_config.h"
#include "egui_typedef.h"
#include "egui_oop.h"
#include "utils/egui_fixmath.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_RESOURCE_TYPE_INTERNAL 0 /* Resource is code */
#define EGUI_RESOURCE_TYPE_EXTERNAL 1 /* Resource is flash/file */

/* Apparently this is needed by several Windows compilers */
// #if !defined(__MACH__)
#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif /* NULL */
// #endif /* ! Mac OS X - breaks precompiled headers */

/**
 * \brief           Get maximal value between 2 values
 * \param[in]       x: First value
 * \param[in]       y: Second value
 * \retval          Maximal value
 * \hideinitializer
 */
#define EGUI_MAX(x, y) ((x) > (y) ? (x) : (y))

/**
 * \brief           Get minimal value between 2 values
 * \param[in]       x: First value
 * \param[in]       y: Second value
 * \retval          Minimal value
 * \hideinitializer
 */
#define EGUI_MIN(x, y) ((x) < (y) ? (x) : (y))

/**
 * \brief           Get absolute value of input
 * \param[in]       x: Input value
 * \retval          Absolute value of input
 * \hideinitializer
 */
#define EGUI_ABS(x) ((x) >= 0 ? (x) : -(x))

#define EGUI_SWAP(a, b) (a = (a) + (b), b = (a) - (b), a = (a) - (b))

#define EGUI_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define EGUI_UNUSED(_x) (void)(_x)

#if defined(_MSC_VER)
#define __EGUI_WEAK__
#define __EGUI_UNUSED_ATTR__
#define __EGUI_MAY_ALIAS_ATTR__
#define __EGUI_OPTIMIZE_SIZE_ATTR__
#elif defined(__GNUC__) || defined(__clang__)
#define __EGUI_WEAK__               __attribute__((weak))
#define __EGUI_UNUSED_ATTR__        __attribute__((unused))
#define __EGUI_MAY_ALIAS_ATTR__     __attribute__((__may_alias__))
#define __EGUI_OPTIMIZE_SIZE_ATTR__ __attribute__((optimize("Os")))
#else
#define __EGUI_WEAK__
#define __EGUI_UNUSED_ATTR__
#define __EGUI_MAY_ALIAS_ATTR__
#define __EGUI_OPTIMIZE_SIZE_ATTR__
#endif

#define __EGUI_STATIC_INLINE__ static inline __EGUI_UNUSED_ATTR__

#if defined(_MSC_VER) && defined(_M_X64)
#define __EGUI_MSVC_ALTERNATE_NAME(_symbol, _fallback) __pragma(comment(linker, "/alternatename:" #_symbol "=" #_fallback))
#else
#define __EGUI_MSVC_ALTERNATE_NAME(_symbol, _fallback)
#endif

/*---------------------- Graphic LCD color definitions -----------------------*/

/* The most simple macro to create a color from R,G and B values.
 * Keep the scalar full value canonical; endian only changes memory byte order. */
#if EGUI_CONFIG_COLOR_DEPTH == 8
#define EGUI_COLOR_MAKE_INIT(_r8, _g8, _b8) {((uint8_t)((((uint16_t)(_r8)) + ((uint16_t)(_g8)) + ((uint16_t)(_b8))) / 3U))}
#elif EGUI_CONFIG_COLOR_DEPTH == 16
#define EGUI_COLOR_MAKE_INIT(_r8, _g8, _b8) {((uint16_t)(((((uint16_t)(_r8)) & 0xF8U) << 8) | ((((uint16_t)(_g8)) & 0xFCU) << 3) | (((uint16_t)(_b8)) >> 3)))}
#elif EGUI_CONFIG_COLOR_DEPTH == 32
#define EGUI_COLOR_MAKE_INIT(_r8, _g8, _b8) {((uint32_t)((((uint32_t)(_r8)) << 16) | (((uint32_t)(_g8)) << 8) | ((uint32_t)(_b8))))}
#endif

#define EGUI_COLOR_MAKE(_r8, _g8, _b8) ((egui_color_t)EGUI_COLOR_MAKE_INIT(_r8, _g8, _b8))

#define EGUI_COLOR_HEX(c)      EGUI_COLOR_MAKE(((uint32_t)((uint32_t)c >> 16) & 0xFF), ((uint32_t)((uint32_t)c >> 8) & 0xFF), ((uint32_t)c & 0xFF))
#define EGUI_COLOR_HEX_INIT(c) EGUI_COLOR_MAKE_INIT(((uint32_t)((uint32_t)c >> 16) & 0xFF), ((uint32_t)((uint32_t)c >> 8) & 0xFF), ((uint32_t)c & 0xFF))

#define EGUI_COLOR_NAMED_INIT(_color) _color##_INIT

#if EGUI_CONFIG_COLOR_DEPTH == 8
#define EGUI_COLOR_RGB888_TRANS(_color)                                                                                                                        \
    EGUI_COLOR_MAKE(((egui_color_rgb888_t *)&_color)->color.red, ((egui_color_rgb888_t *)&_color)->color.green, ((egui_color_rgb888_t *)&_color)->color.blue)  \
            .full
#define EGUI_COLOR_RGB565_TRANS(_color)                                                                                                                        \
    EGUI_COLOR_MAKE((((egui_color_rgb565_t *)&_color)->color.red << 3) | (((egui_color_rgb565_t *)&_color)->color.red >> 2),                                   \
                    (((egui_color_rgb565_t *)&_color)->color.green << 2) | (((egui_color_rgb565_t *)&_color)->color.green >> 4),                               \
                    (((egui_color_rgb565_t *)&_color)->color.blue << 3) | (((egui_color_rgb565_t *)&_color)->color.blue >> 2))                                 \
            .full
#elif EGUI_CONFIG_COLOR_DEPTH == 16
#define EGUI_COLOR_RGB888_TRANS(_color)                                                                                                                        \
    EGUI_COLOR_MAKE(((egui_color_rgb888_t *)&_color)->color.red, ((egui_color_rgb888_t *)&_color)->color.green, ((egui_color_rgb888_t *)&_color)->color.blue)  \
            .full
#define EGUI_COLOR_RGB565_TRANS(_color) _color
#elif EGUI_CONFIG_COLOR_DEPTH == 32
#define EGUI_COLOR_RGB888_TRANS(_color) _color
#define EGUI_COLOR_RGB565_TRANS(_color)                                                                                                                        \
    EGUI_COLOR_MAKE((((egui_color_rgb565_t *)&_color)->color.red << 3) | (((egui_color_rgb565_t *)&_color)->color.red >> 2),                                   \
                    (((egui_color_rgb565_t *)&_color)->color.green << 2) | (((egui_color_rgb565_t *)&_color)->color.green >> 4),                               \
                    (((egui_color_rgb565_t *)&_color)->color.blue << 3) | (((egui_color_rgb565_t *)&_color)->color.blue >> 2))                                 \
            .full
#endif

/* GLCD RGB color definitions                            */
#define EGUI_COLOR_BLACK           EGUI_COLOR_MAKE(0, 0, 0)
#define EGUI_COLOR_BLACK_INIT      EGUI_COLOR_MAKE_INIT(0, 0, 0)
#define EGUI_COLOR_NAVY            EGUI_COLOR_MAKE(0, 0, 128)
#define EGUI_COLOR_NAVY_INIT       EGUI_COLOR_MAKE_INIT(0, 0, 128)
#define EGUI_COLOR_DARK_GREEN      EGUI_COLOR_MAKE(0, 128, 0)
#define EGUI_COLOR_DARK_GREEN_INIT EGUI_COLOR_MAKE_INIT(0, 128, 0)
#define EGUI_COLOR_DARK_CYAN       EGUI_COLOR_MAKE(0, 128, 128)
#define EGUI_COLOR_DARK_CYAN_INIT  EGUI_COLOR_MAKE_INIT(0, 128, 128)
#define EGUI_COLOR_MAROON          EGUI_COLOR_MAKE(128, 0, 0)
#define EGUI_COLOR_MAROON_INIT     EGUI_COLOR_MAKE_INIT(128, 0, 0)
#define EGUI_COLOR_PURPLE          EGUI_COLOR_MAKE(128, 0, 128)
#define EGUI_COLOR_PURPLE_INIT     EGUI_COLOR_MAKE_INIT(128, 0, 128)
#define EGUI_COLOR_OLIVE           EGUI_COLOR_MAKE(128, 128, 0)
#define EGUI_COLOR_OLIVE_INIT      EGUI_COLOR_MAKE_INIT(128, 128, 0)
#define EGUI_COLOR_LIGHT_GREY      EGUI_COLOR_MAKE(192, 192, 192)
#define EGUI_COLOR_LIGHT_GREY_INIT EGUI_COLOR_MAKE_INIT(192, 192, 192)
#define EGUI_COLOR_DARK_GREY       EGUI_COLOR_MAKE(128, 128, 128)
#define EGUI_COLOR_DARK_GREY_INIT  EGUI_COLOR_MAKE_INIT(128, 128, 128)
#define EGUI_COLOR_BLUE            EGUI_COLOR_MAKE(0, 0, 255)
#define EGUI_COLOR_BLUE_INIT       EGUI_COLOR_MAKE_INIT(0, 0, 255)
#define EGUI_COLOR_GREEN           EGUI_COLOR_MAKE(0, 255, 0)
#define EGUI_COLOR_GREEN_INIT      EGUI_COLOR_MAKE_INIT(0, 255, 0)
#define EGUI_COLOR_CYAN            EGUI_COLOR_MAKE(0, 255, 255)
#define EGUI_COLOR_CYAN_INIT       EGUI_COLOR_MAKE_INIT(0, 255, 255)
#define EGUI_COLOR_RED             EGUI_COLOR_MAKE(255, 0, 0)
#define EGUI_COLOR_RED_INIT        EGUI_COLOR_MAKE_INIT(255, 0, 0)
#define EGUI_COLOR_MAGENTA         EGUI_COLOR_MAKE(255, 0, 255)
#define EGUI_COLOR_MAGENTA_INIT    EGUI_COLOR_MAKE_INIT(255, 0, 255)
#define EGUI_COLOR_YELLOW          EGUI_COLOR_MAKE(255, 255, 0)
#define EGUI_COLOR_YELLOW_INIT     EGUI_COLOR_MAKE_INIT(255, 255, 0)
#define EGUI_COLOR_WHITE           EGUI_COLOR_MAKE(255, 255, 255)
#define EGUI_COLOR_WHITE_INIT      EGUI_COLOR_MAKE_INIT(255, 255, 255)
#define EGUI_COLOR_ORANGE          EGUI_COLOR_MAKE(255, 128, 0)
#define EGUI_COLOR_ORANGE_INIT     EGUI_COLOR_MAKE_INIT(255, 128, 0)

enum
{
    EGUI_ALPHA_TRANSP = 0,
    EGUI_ALPHA_0 = 0,
    EGUI_ALPHA_10 = 25,
    EGUI_ALPHA_20 = 51,
    EGUI_ALPHA_30 = 76,
    EGUI_ALPHA_40 = 102,
    EGUI_ALPHA_50 = 127,
    EGUI_ALPHA_60 = 153,
    EGUI_ALPHA_70 = 178,
    EGUI_ALPHA_80 = 204,
    EGUI_ALPHA_90 = 229,
    EGUI_ALPHA_100 = 255,
    EGUI_ALPHA_COVER = 255,
};

#define EGUI_ALIGN_HCENTER 0x01
#define EGUI_ALIGN_LEFT    0x02
#define EGUI_ALIGN_RIGHT   0x04
#define EGUI_ALIGN_HMASK   0x0F

#define EGUI_ALIGN_VCENTER 0x10
#define EGUI_ALIGN_TOP     0x20
#define EGUI_ALIGN_BOTTOM  0x40
#define EGUI_ALIGN_VMASK   0xF0

#define EGUI_ALIGN_CENTER       (EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER)
#define EGUI_ALIGN_TOP_LEFT     (EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP)
#define EGUI_ALIGN_TOP_MID      (EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP)
#define EGUI_ALIGN_TOP_RIGHT    (EGUI_ALIGN_RIGHT | EGUI_ALIGN_TOP)
#define EGUI_ALIGN_BOTTOM_LEFT  (EGUI_ALIGN_LEFT | EGUI_ALIGN_BOTTOM)
#define EGUI_ALIGN_BOTTOM_MID   (EGUI_ALIGN_HCENTER | EGUI_ALIGN_BOTTOM)
#define EGUI_ALIGN_BOTTOM_RIGHT (EGUI_ALIGN_RIGHT | EGUI_ALIGN_BOTTOM)

#define EGUI_LAYOUT_VERTICAL   0
#define EGUI_LAYOUT_HORIZONTAL 1

struct egui_mem_monitor
{
    size_t used_size;
    size_t total_size;
    size_t free_cnt;
    size_t free_size;
    size_t free_biggest_size;
    size_t used_cnt;
    size_t max_used;
    uint8_t used_pct;
    uint8_t frag_pct;
};

/*----------------------------------------------------------------------------*
 * Colour definitions                                                         *

 * *----------------------------------------------------------------------------*/

/*!
 * \brief the colour type for gray8 (8bit gray scale)
 *
 */
typedef union egui_color_gray8_t
{
    uint8_t full;
} egui_color_gray8_t;

/*!
 * \brief the colour type for rgb565
 *
 */
typedef union egui_color_rgb565_t
{
    uint16_t full;
    struct
    {
        uint16_t blue : 5;
        uint16_t green : 6;
        uint16_t red : 5;
    } color;
} egui_color_rgb565_t;

/*!
 * \brief the colour type for brga8888
 *
 * \details In most cases four equal-sized pieces of adjacent memory are used,
 *          one for each channel, and a 0 in a channel indicates black color or
 *          transparent alpha, while all-1 bits indicates white or fully opaque
 *          alpha. By far the most common format is to store 8 bits (one byte)
 *          for each channel, which is 32 bits for each pixel.
 *
 *          (source: https://en.wikipedia.org/wiki/RGBA_color_model#ARGB32)
 */
typedef union egui_color_bgra8888_t
{
    uint32_t full;
    uint8_t chChannel[4];
    struct
    {
        uint32_t blue : 8;
        uint32_t green : 8;
        uint32_t red : 8;
        uint32_t alpha : 8;
    } color;
} egui_color_bgra8888_t;

/*!
 * \brief the colour type for rgb888 (compliant with ccca888 and bgra8888)
 *
 * \details In most cases four equal-sized pieces of adjacent memory are used,
 *          one for each channel, and a 0 in a channel indicates black color or
 *          transparent alpha, while all-1 bits indicates white or fully opaque
 *          alpha. By far the most common format is to store 8 bits (one byte)
 *          for each channel, which is 32 bits for each pixel.
 *
 *          (source: https://en.wikipedia.org/wiki/RGBA_color_model#ARGB32)
 */
typedef union egui_color_rgb888_t
{
    uint32_t full;
    struct
    {
        uint32_t blue : 8;
        uint32_t green : 8;
        uint32_t red : 8;
        uint32_t : 8;
    } color;
} egui_color_rgb888_t;

/*
 * IMPORTANT: The following colour-type-free macros are only used in examples.
 *            They only cover a small range of APIs. They are not guaranteed and
 *            subject to change. Please do NOT use them in your applications.
 */

#if EGUI_CONFIG_COLOR_DEPTH == 8
#define egui_color_t     egui_color_gray8_t
#define egui_color_int_t uint8_t
#elif EGUI_CONFIG_COLOR_DEPTH == 16
#define egui_color_t     egui_color_rgb565_t
#define egui_color_int_t uint16_t
#elif EGUI_CONFIG_COLOR_DEPTH == 32
#define egui_color_t     egui_color_rgb888_t
#define egui_color_int_t uint32_t
#else
#error "Invalid EGUI_CONFIG_COLOR_DEPTH in egui_conf.h! Set it to 8, 16 or 32!"
#endif

typedef uint8_t egui_alpha_t; /*!< Alpha value in range 0-255 */

/** Convert a 0-100 percentage to the 0-255 alpha scale used by the renderer. */
__EGUI_STATIC_INLINE__ egui_alpha_t egui_alpha_make(int32_t percent)
{
    if (percent <= 0)
    {
        return EGUI_ALPHA_0;
    }
    if (percent >= 100)
    {
        return EGUI_ALPHA_100;
    }
    return (egui_alpha_t)(((uint32_t)percent * (uint32_t)EGUI_ALPHA_100) / 100U);
}

#define EGUI_ALPHA_MAKE(_percent) egui_alpha_make((int32_t)(_percent))

#define EGUI_DIM_MAX 0x7FF0  /*!< Maximum value for egui_dim_t */
#define egui_dim_t   int16_t /*!< GUI dimensions in units of pixels */
#if EGUI_CONFIG_REDUCE_MARGIN_PADDING_SIZE
#define egui_dim_margin_padding_t int8_t
#else
#define egui_dim_margin_padding_t egui_dim_t
#endif // EGUI_CONFIG_REDUCE_MARGIN_PADDING_SIZE

#define egui_id_t uint16_t /*!< GUI resource identifier */

#define egui_uintptr_t uintptr_t

extern const uint8_t egui_alpha_change_table_2[4];
extern const uint8_t egui_alpha_change_table_4[16];

/** Combine two alpha factors the same way layered rendering composes opacity. */
static __EGUI_UNUSED_ATTR__ egui_alpha_t egui_color_alpha_mix(egui_alpha_t alpha_0, egui_alpha_t alpha_1)
{
    if (alpha_0 == EGUI_ALPHA_100)
    {
        return alpha_1;
    }
    else if (alpha_1 == EGUI_ALPHA_100)
    {
        return alpha_0;
    }
    else
    {
        // return ((uint16_t)alpha_0 * alpha_1) / EGUI_ALPHA_100;
        return ((uint16_t)alpha_0 * alpha_1 + 128) >> 8; // For speed, +128 for rounding
    }
}

/** Blend one foreground color over one background color with a single alpha factor. */
static __EGUI_UNUSED_ATTR__ egui_color_t egui_rgb_mix(egui_color_t back_color, egui_color_t fore_color, egui_alpha_t fore_alpha)
{
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    /* Early-out for fully opaque / fully transparent */
    if (fore_alpha > 251)
    {
        return fore_color;
    }
    if (fore_alpha < 4)
    {
        return back_color;
    }
    /* Parallel RGB565 blend: pack R+B and G into a 32-bit word.
     * Layout after (c | c<<16) & 0x07E0F81F:  [---GGGGG G00000][RRRRR 00000BBBBB]
     * 5-bit alpha (>> 3) keeps products within channel gaps. */
    uint16_t bg = back_color.full;
    uint16_t fg = fore_color.full;
    uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
    uint32_t fg_rb_g = (fg | ((uint32_t)fg << 16)) & 0x07E0F81FUL;
    uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)fore_alpha >> 3) >> 5)) & 0x07E0F81FUL;
    egui_color_t ret;
    ret.full = (uint16_t)(result | (result >> 16));
    return ret;
#else
    egui_color_t ret;
    ret.color.red = (((uint16_t)(fore_alpha) * (fore_color.color.red)) + (uint16_t)(255 - fore_alpha) * back_color.color.red + 128) >> 8; // +128 for rounding
    ret.color.green =
            (((uint16_t)(fore_alpha) * (fore_color.color.green)) + (uint16_t)(255 - fore_alpha) * back_color.color.green + 128) >> 8; // +128 for rounding
    ret.color.blue =
            (((uint16_t)(fore_alpha) * (fore_color.color.blue)) + (uint16_t)(255 - fore_alpha) * back_color.color.blue + 128) >> 8; // +128 for rounding
    return ret;
#endif
}

/** Pointer-based variant of `egui_rgb_mix()` for hot paths that already work on addresses. */
static __EGUI_UNUSED_ATTR__ void egui_rgb_mix_ptr(egui_color_t *p_back_color, egui_color_t *p_fore_color, egui_color_t *p_out_color, egui_alpha_t fore_alpha)
{
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    if (fore_alpha > 251)
    {
        p_out_color->full = p_fore_color->full;
        return;
    }
    if (fore_alpha < 4)
    {
        p_out_color->full = p_back_color->full;
        return;
    }
    uint16_t bg = p_back_color->full;
    uint16_t fg = p_fore_color->full;
    uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
    uint32_t fg_rb_g = (fg | ((uint32_t)fg << 16)) & 0x07E0F81FUL;
    uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)fore_alpha >> 3) >> 5)) & 0x07E0F81FUL;
    p_out_color->full = (uint16_t)(result | (result >> 16));
#else
    p_out_color->color.red =
            (((uint16_t)(fore_alpha) * (p_fore_color->color.red)) + (uint16_t)(255 - fore_alpha) * p_back_color->color.red + 128) >> 8; // +128 for rounding
    p_out_color->color.green =
            (((uint16_t)(fore_alpha) * (p_fore_color->color.green)) + (uint16_t)(255 - fore_alpha) * p_back_color->color.green + 128) >> 8; // +128 for rounding
    p_out_color->color.blue =
            (((uint16_t)(fore_alpha) * (p_fore_color->color.blue)) + (uint16_t)(255 - fore_alpha) * p_back_color->color.blue + 128) >> 8; // +128 for rounding
#endif
}

/** Blend one BGRA8888 foreground pixel over one RGB565 background pixel. */
void egui_argb8888_mix_rgb565(egui_color_rgb565_t *p_back_color, egui_color_bgra8888_t *p_fore_color, egui_color_rgb565_t *p_out_color);
/** Blend one BGRA8888 foreground pixel over one BGRA8888 background pixel. */
void egui_argb8888_mix_argb8888(egui_color_bgra8888_t *p_back_color, egui_color_bgra8888_t *p_fore_color, egui_color_bgra8888_t *p_out_color);

egui_color_t egui_rgb_mix(egui_color_t back_color, egui_color_t fore_color, egui_alpha_t fore_alpha);
void egui_rgb_mix_ptr(egui_color_t *p_back_color, egui_color_t *p_fore_color, egui_color_t *p_out_color, egui_alpha_t fore_alpha);

/** Compute the child top-left offset inside a parent rectangle for the given alignment flags. */
void egui_common_align_get_x_y(egui_dim_t parent_width, egui_dim_t parent_height, egui_dim_t child_width, egui_dim_t child_height, uint8_t align_type,
                               egui_dim_t *x, egui_dim_t *y);

/** Compatibility wrapper around `egui_api_memcpy()`. */
void egui_memcpy(void *dest, const void *src, uint32_t n);
/** Allocate memory using the active core-aware allocator. */
void *egui_malloc(egui_core_t *core, int size);
/** Free memory that was allocated by `egui_malloc()` or `egui_api_malloc()`. */
void egui_free(egui_core_t *core, void *ptr);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_COMMON_H_ */
