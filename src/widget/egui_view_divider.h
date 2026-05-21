#ifndef _EGUI_VIEW_DIVIDER_H_
#define _EGUI_VIEW_DIVIDER_H_

#include "egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_divider egui_view_divider_t;
/**
 * @brief Minimal divider widget that paints one solid or gradient-filled strip.
 */
struct egui_view_divider
{
    egui_view_t base;

    egui_alpha_t alpha;
    egui_color_t color;
};

// ============== Divider Params ==============
typedef struct egui_view_divider_params egui_view_divider_params_t;
/**
 * @brief Construction-time parameter block for one divider strip.
 */
struct egui_view_divider_params
{
    egui_region_t region;
    egui_color_t color;
};

/** Build a divider parameter block with region and color. */
#define EGUI_VIEW_DIVIDER_PARAMS_INIT(_name, _x, _y, _w, _h, _color)                                                                                           \
    static const egui_view_divider_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .color = _color}

/** Apply a divider parameter block after initialization. */
void egui_view_divider_apply_params(egui_view_t *self, const egui_view_divider_params_t *params);
/** Initialize a divider and immediately apply its parameter block. */
void egui_view_divider_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_divider_params_t *params);

/** Set the divider fill color. */
void egui_view_divider_set_color(egui_view_t *self, egui_color_t color);
/** Return the divider fill color. */
egui_color_t egui_view_divider_get_color(egui_view_t *self);
/** Set the divider fill alpha. */
void egui_view_divider_set_alpha(egui_view_t *self, egui_alpha_t alpha);
/** Return the divider fill alpha. */
egui_alpha_t egui_view_divider_get_alpha(egui_view_t *self);
/** Default draw hook used by the divider API table. */
void egui_view_divider_on_draw(egui_view_t *self);
/** Initialize a simple divider rectangle with the theme border color. */
void egui_view_divider_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_DIVIDER_H_ */
