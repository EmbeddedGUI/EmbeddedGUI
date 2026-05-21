#ifndef _EGUI_VIEW_TILEVIEW_H_
#define _EGUI_VIEW_TILEVIEW_H_

#include "egui_view_group.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of registered tiles in one tileview instance. */
#define EGUI_VIEW_TILEVIEW_MAX_TILES    9
/** Minimum swipe distance before tile navigation is considered. */
#define EGUI_VIEW_TILEVIEW_SWIPE_THRESH 20

typedef struct egui_view_tile_pos
{
    uint8_t col;
    uint8_t row;
} egui_view_tile_pos_t;

/** Listener fired after tileview navigation changes the active tile position. */
typedef void (*egui_view_tileview_changed_cb_t)(egui_view_t *self, uint8_t col, uint8_t row);

typedef struct egui_view_tileview egui_view_tileview_t;
struct egui_view_tileview
{
    egui_view_group_t base;

    egui_view_t *tiles[EGUI_VIEW_TILEVIEW_MAX_TILES];
    egui_view_tile_pos_t tile_positions[EGUI_VIEW_TILEVIEW_MAX_TILES];
    uint8_t tile_count;
    uint8_t current_col;
    uint8_t current_row;

    egui_dim_t touch_down_x;
    egui_dim_t touch_down_y;

    egui_view_tileview_changed_cb_t on_changed;
};

// ============== TileView Params ==============
typedef struct egui_view_tileview_params egui_view_tileview_params_t;
struct egui_view_tileview_params
{
    egui_region_t region;
};

#define EGUI_VIEW_TILEVIEW_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_tileview_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

/** Apply a tileview parameter block after initialization. */
void egui_view_tileview_apply_params(egui_view_t *self, const egui_view_tileview_params_t *params);
/** Initialize a tileview and immediately apply its parameter block. */
void egui_view_tileview_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_tileview_params_t *params);

/** Register one child tile at a logical `(col, row)` position in the swipe grid. Inactive tiles stay parked off-screen. */
void egui_view_tileview_add_tile(egui_view_t *self, egui_view_t *tile_view, uint8_t col, uint8_t row);
/** Return the number of registered tiles. Returns 0 when self is NULL. */
uint8_t egui_view_tileview_get_tile_count(egui_view_t *self);
/** Jump to an existing tile position immediately. Missing positions are ignored. */
void egui_view_tileview_set_current(egui_view_t *self, uint8_t col, uint8_t row);
/** Return the current tile column. Returns 0 when self is NULL. */
uint8_t egui_view_tileview_get_current_col(egui_view_t *self);
/** Return the current tile row. Returns 0 when self is NULL. */
uint8_t egui_view_tileview_get_current_row(egui_view_t *self);
/** Register the callback fired when the active tile changes. */
void egui_view_tileview_set_on_changed(egui_view_t *self, egui_view_tileview_changed_cb_t callback);
/** Return the registered tile-change callback, or NULL when unset or self is NULL. */
egui_view_tileview_changed_cb_t egui_view_tileview_get_on_changed(egui_view_t *self);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/** Intercept touch once movement grows into a swipe gesture that should not be handled by child widgets. */
int egui_view_tileview_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event);
/** Handle swipe-based tile navigation. The dominant axis decides whether to move horizontally or vertically. */
int egui_view_tileview_on_touch_event(egui_view_t *self, egui_motion_event_t *event);
#endif

/** Initialize the swipe-driven tileview container. Tile changes are immediate rather than animated. */
void egui_view_tileview_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TILEVIEW_H_ */
