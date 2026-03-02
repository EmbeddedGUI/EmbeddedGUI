#ifndef _EGUI_VIEW_TILEVIEW_H_
#define _EGUI_VIEW_TILEVIEW_H_

#include "egui_view_group.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TILEVIEW_MAX_TILES    9
#define EGUI_VIEW_TILEVIEW_SWIPE_THRESH 20

typedef struct egui_view_tile_pos
{
    uint8_t col;
    uint8_t row;
} egui_view_tile_pos_t;

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

void egui_view_tileview_apply_params(egui_view_t *self, const egui_view_tileview_params_t *params);
void egui_view_tileview_init_with_params(egui_view_t *self, const egui_view_tileview_params_t *params);

void egui_view_tileview_add_tile(egui_view_t *self, egui_view_t *tile_view, uint8_t col, uint8_t row);
void egui_view_tileview_set_current(egui_view_t *self, uint8_t col, uint8_t row);
void egui_view_tileview_set_on_changed(egui_view_t *self, egui_view_tileview_changed_cb_t callback);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
int egui_view_tileview_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event);
int egui_view_tileview_on_touch_event(egui_view_t *self, egui_motion_event_t *event);
#endif

void egui_view_tileview_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TILEVIEW_H_ */
