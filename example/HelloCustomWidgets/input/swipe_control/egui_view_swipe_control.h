#ifndef _EGUI_VIEW_SWIPE_CONTROL_H_
#define _EGUI_VIEW_SWIPE_CONTROL_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION 0
#define EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE      1
#define EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION   2
#define EGUI_VIEW_SWIPE_CONTROL_PART_NONE         0xFF

#define EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE  0
#define EGUI_VIEW_SWIPE_CONTROL_REVEAL_START 1
#define EGUI_VIEW_SWIPE_CONTROL_REVEAL_END   2

typedef struct egui_view_swipe_control_item egui_view_swipe_control_item_t;
struct egui_view_swipe_control_item
{
    const char *eyebrow;
    const char *title;
    const char *description;
    const char *footer;
    egui_color_t surface_color;
    egui_color_t accent_color;
};

typedef struct egui_view_swipe_control_action egui_view_swipe_control_action_t;
struct egui_view_swipe_control_action
{
    const char *label;
    const char *hint;
    egui_color_t fill_color;
    egui_color_t text_color;
};

typedef void (*egui_view_on_swipe_control_changed_listener_t)(egui_view_t *self, uint8_t reveal_state, uint8_t current_part);

typedef struct egui_view_swipe_control egui_view_swipe_control_t;
struct egui_view_swipe_control
{
    egui_view_t base;
    egui_view_on_swipe_control_changed_listener_t on_changed;
    const egui_view_swipe_control_item_t *item;
    const egui_view_swipe_control_action_t *start_action;
    const egui_view_swipe_control_action_t *end_action;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    const char *title;
    const char *helper;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t inactive_color;
    uint8_t current_part;
    uint8_t reveal_state;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t pressed_part;
    uint8_t dragging;
    egui_dim_t gesture_start_x;
    egui_dim_t gesture_start_y;
};

void egui_view_swipe_control_init(egui_view_t *self);
void egui_view_swipe_control_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_swipe_control_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_swipe_control_set_title(egui_view_t *self, const char *title);
void egui_view_swipe_control_set_helper(egui_view_t *self, const char *helper);
void egui_view_swipe_control_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                         egui_color_t muted_text_color, egui_color_t inactive_color);
void egui_view_swipe_control_set_item(egui_view_t *self, const egui_view_swipe_control_item_t *item);
const egui_view_swipe_control_item_t *egui_view_swipe_control_get_item(egui_view_t *self);
void egui_view_swipe_control_set_actions(egui_view_t *self, const egui_view_swipe_control_action_t *start_action,
                                         const egui_view_swipe_control_action_t *end_action);
void egui_view_swipe_control_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_swipe_control_get_current_part(egui_view_t *self);
void egui_view_swipe_control_set_reveal_state(egui_view_t *self, uint8_t reveal_state);
uint8_t egui_view_swipe_control_get_reveal_state(egui_view_t *self);
void egui_view_swipe_control_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_swipe_control_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_swipe_control_set_on_changed_listener(egui_view_t *self, egui_view_on_swipe_control_changed_listener_t listener);
uint8_t egui_view_swipe_control_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region);
uint8_t egui_view_swipe_control_handle_navigation_key(egui_view_t *self, uint8_t key_code);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SWIPE_CONTROL_H_ */
