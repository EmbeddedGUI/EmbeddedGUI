#ifndef _EGUI_ROTATION_H_
#define _EGUI_ROTATION_H_

#include "egui_common.h"
#include "egui_display_driver.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_SOFTWARE_ROTATION

/**
 * Transform PFB coordinates and pixel data from logical (rotated) space
 * to physical (display hardware) space.
 *
 * Called by egui_core_draw_data() when software rotation is active.
 *
 * @param rotation   Current rotation
 * @param phys_w     Physical display width
 * @param phys_h     Physical display height
 * @param x          Logical X coordinate (in/out, modified to physical)
 * @param y          Logical Y coordinate (in/out, modified to physical)
 * @param w          Logical width (in/out, modified to physical)
 * @param h          Logical height (in/out, modified to physical)
 * @param src        Source PFB data (logical layout)
 * @param dst        Destination buffer (physical layout). For 180 deg, can be same as src.
 * @param dst_size   Size of dst buffer in pixels
 */
void egui_rotation_transform_pfb(egui_display_rotation_t rotation, int16_t phys_w, int16_t phys_h, int16_t *x, int16_t *y, int16_t *w, int16_t *h,
                                 const egui_color_int_t *src, egui_color_int_t *dst, int dst_size);

/**
 * Transform touch coordinates from physical to logical space.
 *
 * @param rotation  Current rotation
 * @param phys_w    Physical display width
 * @param phys_h    Physical display height
 * @param x         Physical touch X coordinate (in/out, modified to logical)
 * @param y         Physical touch Y coordinate (in/out, modified to logical)
 */
void egui_rotation_transform_touch(egui_display_rotation_t rotation, int16_t phys_w, int16_t phys_h, egui_dim_t *x, egui_dim_t *y);

#endif /* EGUI_CONFIG_SOFTWARE_ROTATION */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ROTATION_H_ */
