#ifndef _EGUI_VIEW_CARD_H_
#define _EGUI_VIEW_CARD_H_

#include "egui_view_group.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_card egui_view_card_t;
/**
 * @brief Group-style container that draws card chrome before its children.
 *
 * The widget inherits the child-management behavior from `egui_view_group_t`,
 * then adds themed background, border, and optional shadow styling on top.
 */
struct egui_view_card
{
    egui_view_group_t base;

    egui_dim_t corner_radius;
    egui_dim_t border_width;
    egui_color_t border_color;
    egui_color_t bg_color;
    egui_alpha_t bg_alpha;
    uint8_t bg_color_custom; // 1 when callers override the theme-provided fill color.
};

// ============== Card Params ==============
typedef struct egui_view_card_params egui_view_card_params_t;
/**
 * @brief Construction-time parameter block for one card container.
 */
struct egui_view_card_params
{
    egui_region_t region;
    egui_dim_t corner_radius;
};

/** Build a card parameter block with region and corner radius. */
#define EGUI_VIEW_CARD_PARAMS_INIT(_name, _x, _y, _w, _h, _radius)                                                                                             \
    static const egui_view_card_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .corner_radius = (_radius)}

/** Apply a card parameter block after initialization. */
void egui_view_card_apply_params(egui_view_t *self, const egui_view_card_params_t *params);
/** Initialize a card and immediately apply its parameter block. */
void egui_view_card_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_card_params_t *params);

/** Set the corner radius used by the card background and border. */
void egui_view_card_set_corner_radius(egui_view_t *self, egui_dim_t radius);
/** Return the locally stored card corner radius. */
egui_dim_t egui_view_card_get_corner_radius(egui_view_t *self);
/** Set the border width and border color around the card. */
void egui_view_card_set_border(egui_view_t *self, egui_dim_t width, egui_color_t color);
/** Return the locally stored card border width. */
egui_dim_t egui_view_card_get_border_width(egui_view_t *self);
/** Return the locally stored card border color. */
egui_color_t egui_view_card_get_border_color(egui_view_t *self);
/** Override the themed fill color and remember it as a custom background. */
void egui_view_card_set_bg_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
/** Return the locally stored card background color. */
egui_color_t egui_view_card_get_bg_color(egui_view_t *self);
/** Return the locally stored card background alpha. */
egui_alpha_t egui_view_card_get_bg_alpha(egui_view_t *self);
/** Return whether the background color has been explicitly overridden. */
uint8_t egui_view_card_get_bg_color_custom(egui_view_t *self);
/** Add one child view to the internal group container. */
void egui_view_card_add_child(egui_view_t *self, egui_view_t *child);
/** Run the group layout helper for current children using a simple horizontal or vertical flow. */
void egui_view_card_layout_childs(egui_view_t *self, uint8_t is_horizontal, uint8_t align_type);
/** Default draw hook used by the card API table. */
void egui_view_card_on_draw(egui_view_t *self);
/** Initialize a group-style card with themed background, border, and shadow defaults. */
void egui_view_card_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CARD_H_ */
