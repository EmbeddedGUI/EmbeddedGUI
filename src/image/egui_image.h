#ifndef _EGUI_IMAGE_H_
#define _EGUI_IMAGE_H_

#include "core/egui_canvas.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_image_api egui_image_api_t;
struct egui_image_api
{
    int (*get_point)(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);
    int (*get_point_resize)(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color, egui_alpha_t *alpha);
    void (*draw_image)(const egui_image_t *self, egui_dim_t x, egui_dim_t y);
    void (*draw_image_resize)(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height);
};

struct egui_image
{
    const void *res; // image resource

    const egui_image_api_t *api; // virtual api
};

#define EGUI_IMAGE_DEFINE_STATIC(_type, _name, _res)                                                                                                           \
    extern const egui_image_api_t _type##_api_table;                                                                                                           \
    static _type _name = {.res = _res, .api = &_type##_api_table}

#define EGUI_IMAGE_SUB_DEFINE_STATIC(_type, _name, _res)                                                                                                       \
    extern const egui_image_api_t _type##_api_table;                                                                                                           \
    static _type _name = {.base = {.res = _res, .api = &_type##_api_table}}

#define EGUI_IMAGE_SUB_DEFINE_CONST(_type, _name, _res)                                                                                                              \
    extern const egui_image_api_t _type##_api_table;                                                                                                           \
    const _type _name = {.base = {.res = _res, .api = &_type##_api_table}}

#define EGUI_IMAGE_SUB_DEFINE(_type, _name, _res)                                                                                                              \
    extern const egui_image_api_t _type##_api_table;                                                                                                           \
    _type _name = {.base = {.res = _res, .api = &_type##_api_table}}

void egui_image_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y);
void egui_image_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height);
void egui_image_init(egui_image_t *self, const void *res);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_H_ */
