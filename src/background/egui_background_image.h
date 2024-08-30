#ifndef _EGUI_BACKGROUND_IMAGE_H_
#define _EGUI_BACKGROUND_IMAGE_H_

#include "egui_background.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_BACKGROUND_IMAGE_PARAM_INIT(_name, _img)                                                                                                          \
    static const egui_background_image_param_t _name = {                                                                                                       \
            .img = _img,                                                                                                                                       \
    }

extern const egui_background_api_t egui_background_image_t_api_table;

#define EGUI_BACKGROUND_IMAGE_STATIC_CONST_INIT(_name, _params)                                                                                                \
    static const egui_background_image_t _name = {.base = {.api = &egui_background_image_t_api_table, .params = _params}}

typedef struct egui_background_image_param egui_background_image_param_t;
struct egui_background_image_param
{
    egui_image_t *img;
};

typedef struct egui_background_image egui_background_image_t;
struct egui_background_image
{
    egui_background_t base;
};

void egui_background_image_param_init(egui_background_image_param_t *param, egui_image_t *img);

void egui_background_image_draw(egui_background_t *self, egui_region_t *region, int is_disabled, int is_pressed);
void egui_background_image_init(egui_background_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BACKGROUND_IMAGE_H_ */
