#ifndef _EGUI_ROTATION_H_
#define _EGUI_ROTATION_H_

#include "egui_common.h"
#include "egui_display_driver.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Transform one rendered PFB tile from logical screen space into native panel space.
 * The function updates the tile rectangle in place and rotates/copies the tile pixels into `dst`.
 * `src` and `dst` may alias only for the 180-degree case.
 *
 * @param rotation   Current rotation
 * @param phys_w     Native panel width before rotation
 * @param phys_h     Native panel height before rotation
 * @param x          Logical X coordinate (in/out, rewritten to native-panel space)
 * @param y          Logical Y coordinate (in/out, rewritten to native-panel space)
 * @param w          Logical tile width (in/out, may swap with `h` for 90/270 rotation)
 * @param h          Logical tile height (in/out, may swap with `w` for 90/270 rotation)
 * @param src        Source PFB pixels in logical row-major order
 * @param dst        Destination pixel buffer in native-panel row-major order
 * @param dst_size   Available capacity of `dst` in pixels
 */
#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
void egui_rotation_transform_pfb(egui_display_rotation_t rotation, int16_t phys_w, int16_t phys_h, int16_t *x, int16_t *y, int16_t *w, int16_t *h,
                                 const egui_color_int_t *src, egui_color_int_t *dst, int dst_size);
#else
__EGUI_STATIC_INLINE__ void egui_rotation_transform_pfb(egui_display_rotation_t rotation, int16_t phys_w, int16_t phys_h, int16_t *x, int16_t *y, int16_t *w,
                                                        int16_t *h, const egui_color_int_t *src, egui_color_int_t *dst, int dst_size)
{
    EGUI_UNUSED(rotation);
    EGUI_UNUSED(phys_w);
    EGUI_UNUSED(phys_h);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(w);
    EGUI_UNUSED(h);
    EGUI_UNUSED(src);
    EGUI_UNUSED(dst);
    EGUI_UNUSED(dst_size);
}
#endif

/**
 * Transform one touch coordinate from native panel space back into logical screen space.
 * This is used when the port keeps the panel unrotated and the core emulates rotation in software.
 *
 * @param rotation  Current rotation
 * @param phys_w    Native panel width before rotation
 * @param phys_h    Native panel height before rotation
 * @param x         Native-panel touch X coordinate (in/out, rewritten to logical space)
 * @param y         Native-panel touch Y coordinate (in/out, rewritten to logical space)
 */
#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
void egui_rotation_transform_touch(egui_display_rotation_t rotation, int16_t phys_w, int16_t phys_h, egui_dim_t *x, egui_dim_t *y);
#else
__EGUI_STATIC_INLINE__ void egui_rotation_transform_touch(egui_display_rotation_t rotation, int16_t phys_w, int16_t phys_h, egui_dim_t *x, egui_dim_t *y)
{
    EGUI_UNUSED(rotation);
    EGUI_UNUSED(phys_w);
    EGUI_UNUSED(phys_h);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
}
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ROTATION_H_ */
