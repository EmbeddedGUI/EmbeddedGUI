#ifndef _EGUI_VIEW_IMAGE_H_
#define _EGUI_VIEW_IMAGE_H_

#include "egui_view.h"
#include "image/egui_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Draw the source image at its original size from the top-left corner. */
#define EGUI_VIEW_IMAGE_TYPE_NORMAL 0
/** Stretch the source image to fill the view work region. */
#define EGUI_VIEW_IMAGE_TYPE_RESIZE 1

typedef struct egui_view_image egui_view_image_t;
/**
 * @brief Basic image widget that either draws at source size or stretches.
 *
 * The widget borrows an `egui_image_t` resource pointer and optionally applies
 * a single tint color when drawing alpha-style image assets.
 */
struct egui_view_image
{
    egui_view_t base;

    uint8_t image_type;

    const egui_image_t *image;

    egui_color_t image_color;
    egui_alpha_t image_color_alpha;

#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
    int16_t angle_deg; /* Rotation angle in degrees, 0 = no rotation. */
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
    int16_t scale_q8; /* Scale factor Q8 (256 = 1x, 512 = 2x, 128 = 0.5x). */
#endif
};

// ============== Image Params ==============
typedef struct egui_view_image_params egui_view_image_params_t;
/**
 * @brief Construction-time parameter block for one image widget.
 */
struct egui_view_image_params
{
    egui_region_t region;
    const egui_image_t *image;
};

/** Build an image parameter block with region and resource pointer. */
#define EGUI_VIEW_IMAGE_PARAMS_INIT(_name, _x, _y, _w, _h, _image)                                                                                             \
    static const egui_view_image_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .image = (_image)}

/** Apply a simple image parameter block after initialization. */
void egui_view_image_apply_params(egui_view_t *self, const egui_view_image_params_t *params);
/** Initialize an image view and immediately apply its parameter block. */
void egui_view_image_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_image_params_t *params);

/** Default draw hook used by the image view API table. */
void egui_view_image_on_draw(egui_view_t *self);
/** Choose original-size drawing or stretched drawing. */
void egui_view_image_set_image_type(egui_view_t *self, int image_type);
/** Return the current image draw mode. Returns 0 when self is NULL. */
int egui_view_image_get_image_type(egui_view_t *self);
/** Set the image resource rendered by this view. */
void egui_view_image_set_image(egui_view_t *self, egui_image_t *image);
/** Return the current image resource pointer, or NULL when none is set. */
const egui_image_t *egui_view_image_get_image(egui_view_t *self);
/** Apply a tint color. Use alpha `0` to disable the tint path entirely. */
void egui_view_image_set_image_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
/** Return the configured tint color. Returns zero when self is NULL. */
egui_color_t egui_view_image_get_image_color(egui_view_t *self);
/** Return the configured tint alpha. Returns 0 when self is NULL. */
egui_alpha_t egui_view_image_get_image_alpha(egui_view_t *self);

/** Initialize the base image widget. */
void egui_view_image_init(egui_view_t *self, egui_core_t *core);

#if EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
/** Set the scale factor in Q8 format (256 = 1x, 512 = 2x, 128 = 0.5x). Triggers a redraw. */
void egui_view_image_set_scale(egui_view_t *self, int16_t scale_q8);
/** Return the current scale factor. */
int16_t egui_view_image_get_scale(egui_view_t *self);
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
/** Set the rotation angle in degrees (0-359). Triggers a redraw. */
void egui_view_image_set_angle(egui_view_t *self, int16_t angle_deg);
/** Return the current rotation angle. */
int16_t egui_view_image_get_angle(egui_view_t *self);
/** Convenience setter for both angle and scale in one call. */
void egui_view_image_set_transform(egui_view_t *self, int16_t angle_deg, int16_t scale_q8);
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_IMAGE_H_ */
