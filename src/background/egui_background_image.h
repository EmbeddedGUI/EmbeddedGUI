#ifndef _EGUI_BACKGROUND_IMAGE_H_
#define _EGUI_BACKGROUND_IMAGE_H_

#include "egui_background.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Build one image-background parameter block from a single image pointer. */
#define EGUI_BACKGROUND_IMAGE_PARAM_INIT(_name, _img)                                                                                                          \
    static const egui_background_image_param_t _name = {                                                                                                       \
            .img = _img,                                                                                                                                       \
    }

extern const egui_background_api_t egui_background_image_t_api_table;

/** Build one static image-background object that reuses a shared parameter table. */
#define EGUI_BACKGROUND_IMAGE_STATIC_CONST_INIT(_name, _params)                                                                                                \
    static const egui_background_image_t _name = {.base = {.api = &egui_background_image_t_api_table, .params = _params}}

typedef struct egui_background_image_param egui_background_image_param_t;

/**
 * @brief Parameter block for one image background state.
 */
struct egui_background_image_param
{
    egui_image_t *img; // borrowed image pointer drawn to fill the target region
};

typedef struct egui_background_image egui_background_image_t;

/**
 * @brief Concrete background subtype that stretches an image over the view.
 */
struct egui_background_image
{
    egui_background_t base;
};

/**
 * @brief Initialize one image-background parameter block from a borrowed image pointer.
 */
void egui_background_image_param_init(egui_background_image_param_t *param, egui_image_t *img);

/**
 * @brief Draw the selected image background stretched to the supplied region.
 */
void egui_background_image_on_draw(egui_background_t *self, egui_canvas_t *canvas, egui_region_t *region, const void *param);

/**
 * @brief Initialize an image-backed background implementation.
 */
void egui_background_image_init(egui_background_t *self);

/**
 * @brief Initialize an image-backed background and immediately bind its state table.
 */
void egui_background_image_init_with_params(egui_background_t *self, const egui_background_params_t *params);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BACKGROUND_IMAGE_H_ */
