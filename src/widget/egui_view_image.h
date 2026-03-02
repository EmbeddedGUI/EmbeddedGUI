#ifndef _EGUI_VIEW_IMAGE_H_
#define _EGUI_VIEW_IMAGE_H_

#include "egui_view.h"
#include "core/egui_theme.h"
#include "image/egui_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_IMAGE_TYPE_NORMAL 0
#define EGUI_VIEW_IMAGE_TYPE_RESIZE 1

typedef struct egui_view_image egui_view_image_t;
struct egui_view_image
{
    egui_view_t base;

    uint8_t image_type;

    const egui_image_t *image;

    egui_color_t image_color;
    egui_alpha_t image_color_alpha;
};

// ============== Image Params ==============
typedef struct egui_view_image_params egui_view_image_params_t;
struct egui_view_image_params
{
    egui_region_t region;
    const egui_image_t *image;
};

#define EGUI_VIEW_IMAGE_PARAMS_INIT(_name, _x, _y, _w, _h, _image)                                                                                             \
    static const egui_view_image_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .image = (_image)}

void egui_view_image_apply_params(egui_view_t *self, const egui_view_image_params_t *params);
void egui_view_image_init_with_params(egui_view_t *self, const egui_view_image_params_t *params);

void egui_view_image_on_draw(egui_view_t *self);
void egui_view_image_set_image_type(egui_view_t *self, int image_type);
void egui_view_image_set_image(egui_view_t *self, egui_image_t *image);
void egui_view_image_set_image_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);

void egui_view_image_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_IMAGE_H_ */
