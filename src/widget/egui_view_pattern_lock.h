#ifndef _EGUI_VIEW_PATTERN_LOCK_H_
#define _EGUI_VIEW_PATTERN_LOCK_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_PATTERN_LOCK_GRID_SIZE 3
#define EGUI_VIEW_PATTERN_LOCK_MAX_NODES (EGUI_VIEW_PATTERN_LOCK_GRID_SIZE * EGUI_VIEW_PATTERN_LOCK_GRID_SIZE)

typedef void (*egui_view_on_pattern_complete_listener_t)(egui_view_t *self, uint8_t node_count);
typedef void (*egui_view_on_pattern_finish_listener_t)(egui_view_t *self, const uint8_t *nodes, uint8_t node_count, uint8_t valid);

typedef struct egui_view_pattern_lock egui_view_pattern_lock_t;
struct egui_view_pattern_lock
{
    egui_view_t base;

    uint8_t min_nodes;
    uint8_t node_count;
    uint8_t nodes[EGUI_VIEW_PATTERN_LOCK_MAX_NODES];
    uint8_t selected[EGUI_VIEW_PATTERN_LOCK_MAX_NODES];
    uint8_t is_tracking;
    uint8_t is_completed;
    uint8_t show_error;
    uint8_t touch_expand;

    egui_dim_t cursor_x;
    egui_dim_t cursor_y;

    egui_alpha_t alpha;
    egui_color_t bg_color;
    egui_color_t border_color;
    egui_color_t node_color;
    egui_color_t active_node_color;
    egui_color_t line_color;
    egui_color_t error_color;

    egui_view_on_pattern_complete_listener_t on_pattern_complete;
    egui_view_on_pattern_finish_listener_t on_pattern_finish;
};

typedef struct egui_view_pattern_lock_params egui_view_pattern_lock_params_t;
struct egui_view_pattern_lock_params
{
    egui_region_t region;
    uint8_t min_nodes;
    uint8_t touch_expand;
};

#define EGUI_VIEW_PATTERN_LOCK_PARAMS_INIT(_name, _x, _y, _w, _h, _min_nodes, _touch_expand)                                                                   \
    static const egui_view_pattern_lock_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .min_nodes = (_min_nodes), .touch_expand = (_touch_expand)}

void egui_view_pattern_lock_apply_params(egui_view_t *self, const egui_view_pattern_lock_params_t *params);
void egui_view_pattern_lock_init_with_params(egui_view_t *self, const egui_view_pattern_lock_params_t *params);

void egui_view_pattern_lock_set_min_nodes(egui_view_t *self, uint8_t min_nodes);
uint8_t egui_view_pattern_lock_get_min_nodes(egui_view_t *self);
uint8_t egui_view_pattern_lock_get_node_count(egui_view_t *self);
const uint8_t *egui_view_pattern_lock_get_nodes(egui_view_t *self);
void egui_view_pattern_lock_clear_pattern(egui_view_t *self);
void egui_view_pattern_lock_set_touch_expand(egui_view_t *self, uint8_t touch_expand);
uint8_t egui_view_pattern_lock_get_touch_expand(egui_view_t *self);

void egui_view_pattern_lock_set_on_pattern_complete_listener(egui_view_t *self, egui_view_on_pattern_complete_listener_t listener);
void egui_view_pattern_lock_set_on_pattern_finish_listener(egui_view_t *self, egui_view_on_pattern_finish_listener_t listener);
void egui_view_pattern_lock_set_bg_color(egui_view_t *self, egui_color_t color);
void egui_view_pattern_lock_set_border_color(egui_view_t *self, egui_color_t color);
void egui_view_pattern_lock_set_node_color(egui_view_t *self, egui_color_t color);
void egui_view_pattern_lock_set_active_node_color(egui_view_t *self, egui_color_t color);
void egui_view_pattern_lock_set_line_color(egui_view_t *self, egui_color_t color);
void egui_view_pattern_lock_set_error_color(egui_view_t *self, egui_color_t color);

void egui_view_pattern_lock_on_draw(egui_view_t *self);
void egui_view_pattern_lock_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_PATTERN_LOCK_H_ */
