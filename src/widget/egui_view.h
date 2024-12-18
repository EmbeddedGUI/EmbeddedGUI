#ifndef _EGUI_VIEW_H_
#define _EGUI_VIEW_H_

#include "core/egui_region.h"
#include "utils/egui_dlist.h"
#include "core/egui_canvas.h"
#include "core/egui_motion_event.h"
#include "background/egui_background.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif


typedef struct egui_view_api egui_view_api_t;
struct egui_view_api
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    int (*dispatch_touch_event)(egui_view_t *self, egui_motion_event_t *event);
    int (*on_touch_event)(egui_view_t *self, egui_motion_event_t *event);
    // to reduce code size, just put the function pointer here, and implement it in egui_view_group.c
    int (*on_intercept_touch_event)(egui_view_t *self, egui_motion_event_t *event);
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    void (*compute_scroll)(egui_view_t *self);
    void (*calculate_layout)(egui_view_t *self);
    void (*request_layout)(egui_view_t *self);
    void (*draw)(egui_view_t *self);
    void (*on_attach_to_window)(egui_view_t *self);
    void (*on_draw)(egui_view_t *self);
    void (*on_detach_from_window)(egui_view_t *self);
};

#define EGUI_VIEW_API_TABLE_NAME(_name) _name##_api_table

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
#if !EGUI_CONFIG_AC5
#define EGUI_VIEW_API_DEFINE(_name, _dispatch_touch_event, _on_touch_event, _on_intercept_touch_event, _compute_scroll, _calculate_layout, _request_layout,    \
                             _draw, _on_attach_to_window, _on_draw, _on_detach_from_window)                                                                    \
    const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(_name) = {                                                                                                  \
            .dispatch_touch_event = _dispatch_touch_event == NULL ? egui_view_dispatch_touch_event : _dispatch_touch_event,                                    \
            .on_touch_event = _on_touch_event == NULL ? egui_view_on_touch_event : _on_touch_event,                                                            \
            .on_intercept_touch_event = _on_intercept_touch_event == NULL ? egui_view_on_intercept_touch_event : _on_intercept_touch_event,                    \
            .compute_scroll = _compute_scroll == NULL ? egui_view_compute_scroll : _compute_scroll,                                                            \
            .calculate_layout = _calculate_layout == NULL ? egui_view_calculate_layout : _calculate_layout,                                                    \
            .request_layout = _request_layout == NULL ? egui_view_request_layout : _request_layout,                                                            \
            .draw = _draw == NULL ? egui_view_draw : _draw,                                                                                                    \
            .on_attach_to_window = _on_attach_to_window == NULL ? egui_view_on_attach_to_window : _on_attach_to_window,                                        \
            .on_draw = _on_draw == NULL ? egui_view_on_draw : _on_draw,                                                                                        \
            .on_detach_from_window = _on_detach_from_window == NULL ? egui_view_on_detach_from_window : _on_detach_from_window,                                \
    };
#else
#define EGUI_VIEW_API_DEFINE(_name, _dispatch_touch_event, _on_touch_event, _on_intercept_touch_event, _compute_scroll, _calculate_layout, _request_layout,    \
                             _draw, _on_attach_to_window, _on_draw, _on_detach_from_window)                                                                    \
           egui_view_api_t EGUI_VIEW_API_TABLE_NAME(_name);
		
#define EGUI_VIEW_API_INIT(_name, _dispatch_touch_event, _on_touch_event, _on_intercept_touch_event, _compute_scroll, _calculate_layout, _request_layout,    \
                             _draw, _on_attach_to_window, _on_draw, _on_detach_from_window)                                                                    \
            EGUI_VIEW_API_TABLE_NAME(_name).compute_scroll = _compute_scroll == NULL ? egui_view_compute_scroll : _compute_scroll;                                                           \
            EGUI_VIEW_API_TABLE_NAME(_name).calculate_layout = _calculate_layout == NULL ? egui_view_calculate_layout : _calculate_layout;                                                    \
            EGUI_VIEW_API_TABLE_NAME(_name).request_layout = _request_layout == NULL ? egui_view_request_layout : _request_layout;                                                            \
            EGUI_VIEW_API_TABLE_NAME(_name).draw = _draw == NULL ? egui_view_draw : _draw;                                                                                                    \
            EGUI_VIEW_API_TABLE_NAME(_name).on_attach_to_window = _on_attach_to_window == NULL ? egui_view_on_attach_to_window : _on_attach_to_window;                                        \
            EGUI_VIEW_API_TABLE_NAME(_name).on_draw = _on_draw == NULL ? egui_view_on_draw : _on_draw;                                                                                        \
            EGUI_VIEW_API_TABLE_NAME(_name).on_detach_from_window = _on_detach_from_window == NULL ? egui_view_on_detach_from_window : _on_detach_from_window;  
#endif // EGUI_CONFIG_AC5						
#else
#if !EGUI_CONFIG_AC5
#define EGUI_VIEW_API_DEFINE(_name, _dispatch_touch_event, _on_touch_event, _on_intercept_touch_event, _compute_scroll, _calculate_layout, _request_layout,    \
                             _draw, _on_attach_to_window, _on_draw, _on_detach_from_window)                                                                    \
    const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(_name) = { \
            .compute_scroll = _compute_scroll == NULL ? egui_view_compute_scroll : _compute_scroll,                                                            \
            .calculate_layout = _calculate_layout == NULL ? egui_view_calculate_layout : _calculate_layout,                                                    \
            .request_layout = _request_layout == NULL ? egui_view_request_layout : _request_layout,                                                            \
            .draw = _draw == NULL ? egui_view_draw : _draw,                                                                                                    \
            .on_attach_to_window = _on_attach_to_window == NULL ? egui_view_on_attach_to_window : _on_attach_to_window,                                        \
            .on_draw = _on_draw == NULL ? egui_view_on_draw : _on_draw,                                                                                        \
            .on_detach_from_window = _on_detach_from_window == NULL ? egui_view_on_detach_from_window : _on_detach_from_window,                                \
    };

#else
#define EGUI_VIEW_API_DEFINE(_name, _dispatch_touch_event, _on_touch_event, _on_intercept_touch_event, _compute_scroll, _calculate_layout, _request_layout,    \
                             _draw, _on_attach_to_window, _on_draw, _on_detach_from_window)                                                                    \
           egui_view_api_t EGUI_VIEW_API_TABLE_NAME(_name);
		
#define EGUI_VIEW_API_INIT(_name, _dispatch_touch_event, _on_touch_event, _on_intercept_touch_event, _compute_scroll, _calculate_layout, _request_layout,    \
                             _draw, _on_attach_to_window, _on_draw, _on_detach_from_window)                                                                    \
            EGUI_VIEW_API_TABLE_NAME(_name).compute_scroll = _compute_scroll == NULL ? egui_view_compute_scroll : _compute_scroll;                                                           \
            EGUI_VIEW_API_TABLE_NAME(_name).calculate_layout = _calculate_layout == NULL ? egui_view_calculate_layout : _calculate_layout;                                                    \
            EGUI_VIEW_API_TABLE_NAME(_name).request_layout = _request_layout == NULL ? egui_view_request_layout : _request_layout;                                                            \
            EGUI_VIEW_API_TABLE_NAME(_name).draw = _draw == NULL ? egui_view_draw : _draw;                                                                                                    \
            EGUI_VIEW_API_TABLE_NAME(_name).on_attach_to_window = _on_attach_to_window == NULL ? egui_view_on_attach_to_window : _on_attach_to_window;                                        \
            EGUI_VIEW_API_TABLE_NAME(_name).on_draw = _on_draw == NULL ? egui_view_on_draw : _on_draw;                                                                                        \
            EGUI_VIEW_API_TABLE_NAME(_name).on_detach_from_window = _on_detach_from_window == NULL ? egui_view_on_detach_from_window : _on_detach_from_window;                                
						
#endif // EGUI_CONFIG_AC5		
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

typedef struct egui_view_padding egui_view_padding_t;
struct egui_view_padding
{
    egui_dim_margin_padding_t left;
    egui_dim_margin_padding_t right;
    egui_dim_margin_padding_t top;
    egui_dim_margin_padding_t bottom;
};

typedef struct egui_view_margin egui_view_margin_t;
struct egui_view_margin
{
    egui_dim_margin_padding_t left;
    egui_dim_margin_padding_t right;
    egui_dim_margin_padding_t top;
    egui_dim_margin_padding_t bottom;
};

typedef int (*egui_view_on_touch_listener_t)(egui_view_t *self, egui_motion_event_t *event);
typedef void (*egui_view_on_click_listener_t)(egui_view_t *self);

struct egui_view
{
    // id is used to identify the view, it should be unique in the view tree, and it is only for debugging purpose.
    uint16_t id; // ID of the view

    uint8_t is_enable : 1;         // whether the view is enabled
    uint8_t is_visible : 1;        // whether the view is visible
    uint8_t is_gone : 1;        // whether the view is gone
    uint8_t is_pressed : 1;        // whether the view is pressed
    uint8_t is_clickable : 1;      // whether the view is clickable
    uint8_t is_request_layout : 1; // whether the view is requested to layout

    egui_alpha_t alpha; // alpha of the view

    egui_view_padding_t padding;
    egui_view_margin_t margin;

    egui_dnode_t node; // used for linked list

    egui_view_group_t *parent; // parent view

    egui_region_t region; // size of the region

    egui_region_t region_screen; // size of the region in screen coordinate

    egui_background_t *background; // background

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_view_on_touch_listener_t on_touch_listener; // touch listener

    egui_view_on_click_listener_t on_click_listener; // clcick listener
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    const char *name; // name of the view
#endif

    const egui_view_api_t *api; // api of the view
};

void egui_view_invalidate(egui_view_t *self);
void egui_view_set_background(egui_view_t *self, egui_background_t *background);
void egui_view_set_parent(egui_view_t *self, egui_view_group_t *parent);
void egui_view_set_alpha(egui_view_t *self, egui_alpha_t alpha);
void egui_view_get_raw_pos(egui_view_t *self, egui_location_t *location);
void egui_view_layout(egui_view_t *self, egui_region_t *region);
void egui_view_scroll_to(egui_view_t *self, egui_dim_t x, egui_dim_t y);
void egui_view_scroll_by(egui_view_t *self, egui_dim_t x, egui_dim_t y);
void egui_view_get_work_region(egui_view_t *self, egui_region_t *region);

void egui_view_set_on_click_listener(egui_view_t *self, egui_view_on_click_listener_t listener);
void egui_view_set_on_touch_listener(egui_view_t *self, egui_view_on_touch_listener_t listener);

void egui_view_set_enable(egui_view_t *self, int is_enable);
int egui_view_get_enable(egui_view_t *self);
void egui_view_set_clickable(egui_view_t *self, int is_clickable);
int egui_view_get_clickable(egui_view_t *self);
void egui_view_set_position(egui_view_t *self, egui_dim_t x, egui_dim_t y);
void egui_view_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height);
void egui_view_set_pressed(egui_view_t *self, int is_pressed);
int egui_view_get_pressed(egui_view_t *self);
void egui_view_set_visible(egui_view_t *self, int is_visible);
int egui_view_get_visible(egui_view_t *self);
void egui_view_set_gone(egui_view_t *self, int is_gone);
int egui_view_get_gone(egui_view_t *self);
void egui_view_set_padding(egui_view_t *self, egui_dim_margin_padding_t left, egui_dim_margin_padding_t right, egui_dim_margin_padding_t top, egui_dim_margin_padding_t bottom);
void egui_view_set_padding_all(egui_view_t *self, egui_dim_margin_padding_t padding);
void egui_view_set_margin(egui_view_t *self, egui_dim_margin_padding_t left, egui_dim_margin_padding_t right, egui_dim_margin_padding_t top, egui_dim_margin_padding_t bottom);
void egui_view_set_margin_all(egui_view_t *self, egui_dim_margin_padding_t margin);

void egui_view_set_position(egui_view_t *self, egui_dim_t x, egui_dim_t y);
void egui_view_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height);

void egui_view_set_view_name(egui_view_t *self, const char *name);

int egui_view_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event);
int egui_view_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event);
int egui_view_perform_click(egui_view_t *self);
int egui_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event);

void egui_view_on_attach_to_window(egui_view_t *self);
void egui_view_on_draw(egui_view_t *self);
void egui_view_on_detach_from_window(egui_view_t *self);
void egui_view_draw(egui_view_t *self);
void egui_view_request_layout(egui_view_t *self);
void egui_view_compute_scroll(egui_view_t *self);
void egui_view_calculate_layout(egui_view_t *self);
void egui_view_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_H_ */
