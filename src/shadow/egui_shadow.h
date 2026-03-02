#ifndef _EGUI_SHADOW_H_
#define _EGUI_SHADOW_H_

#include "core/egui_common.h"
#include "core/egui_region.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

struct egui_shadow
{
    egui_dim_t width;         // shadow blur width in pixels
    egui_dim_t ofs_x;         // horizontal offset
    egui_dim_t ofs_y;         // vertical offset
    egui_dim_t spread;        // shadow spread (expand beyond widget)
    egui_alpha_t opa;         // shadow opacity
    egui_color_t color;       // shadow color
    egui_dim_t corner_radius; // corner radius (0 = square, >0 = rounded)
};

#define EGUI_SHADOW_PARAM_INIT(_name, _width, _ofs_x, _ofs_y, _color, _opa)                                                                                    \
    static const egui_shadow_t _name = {.width = _width, .ofs_x = _ofs_x, .ofs_y = _ofs_y, .spread = 0, .opa = _opa, .color = _color, .corner_radius = 0}

#define EGUI_SHADOW_PARAM_INIT_ROUND(_name, _width, _ofs_x, _ofs_y, _color, _opa, _corner_radius)                                                              \
    static const egui_shadow_t _name = {                                                                                                                       \
            .width = _width, .ofs_x = _ofs_x, .ofs_y = _ofs_y, .spread = 0, .opa = _opa, .color = _color, .corner_radius = _corner_radius}

#define EGUI_SHADOW_PARAM_INIT_FULL(_name, _width, _ofs_x, _ofs_y, _spread, _color, _opa, _corner_radius)                                                      \
    static const egui_shadow_t _name = {                                                                                                                       \
            .width = _width, .ofs_x = _ofs_x, .ofs_y = _ofs_y, .spread = _spread, .opa = _opa, .color = _color, .corner_radius = _corner_radius}

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW

#define EGUI_SHADOW_LUT_SIZE 64

void egui_shadow_draw(const egui_shadow_t *shadow, egui_region_t *view_region);
void egui_shadow_get_region(const egui_shadow_t *shadow, egui_region_t *view_region, egui_region_t *shadow_region);

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_SHADOW_H_ */
