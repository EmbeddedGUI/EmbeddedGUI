#ifndef _EGUI_VIEW_SLIDER_H_
#define _EGUI_VIEW_SLIDER_H_

#include "egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Listener fired when the slider value changes. */
typedef void (*egui_view_on_value_changed_listener_t)(egui_view_t *self, uint8_t value);

typedef struct egui_view_slider egui_view_slider_t;
/**
 * @brief Draggable horizontal value control backed by one thumb and track.
 *
 * The widget stores one logical value and converts it through the shared
 * linear-value helper so drawing and touch math stay in sync.
 */
struct egui_view_slider
{
    egui_view_t base;

    egui_view_on_value_changed_listener_t on_value_changed;

    uint8_t value;
    uint8_t is_dragging;
    uint8_t max_value;
    egui_color_t track_color;
    egui_color_t active_color;
    egui_color_t thumb_color;
};

// ============== Slider Params ==============
typedef struct egui_view_slider_params egui_view_slider_params_t;
/**
 * @brief Construction-time parameter block for one slider widget.
 */
struct egui_view_slider_params
{
    egui_region_t region;
    uint8_t value;
};

/** Build a slider parameter block with region and initial value. */
#define EGUI_VIEW_SLIDER_PARAMS_INIT(_name, _x, _y, _w, _h, _val)                                                                                              \
    static const egui_view_slider_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .value = (_val)}

/** Apply a slider parameter block after initialization. */
void egui_view_slider_apply_params(egui_view_t *self, const egui_view_slider_params_t *params);
/** Initialize a slider and immediately apply its parameter block. */
void egui_view_slider_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_slider_params_t *params);

/** Register the callback fired when the value changes. */
void egui_view_slider_set_on_value_changed_listener(egui_view_t *self, egui_view_on_value_changed_listener_t listener);
/** Return the callback fired when the value changes. */
egui_view_on_value_changed_listener_t egui_view_slider_get_on_value_changed_listener(egui_view_t *self);
/** Set the slider value. The default range is `0..100`. */
void egui_view_slider_set_value(egui_view_t *self, uint8_t value);
/** Return the current slider value. */
uint8_t egui_view_slider_get_value(egui_view_t *self);
/** Return the maximum slider value. */
uint8_t egui_view_slider_get_max_value(egui_view_t *self);
/** Return non-zero when the slider is currently being dragged. */
int egui_view_slider_get_is_dragging(egui_view_t *self);
/** Return the background track color. */
egui_color_t egui_view_slider_get_track_color(egui_view_t *self);
/** Return the filled active track color. */
egui_color_t egui_view_slider_get_active_color(egui_view_t *self);
/** Return the thumb circle color. */
egui_color_t egui_view_slider_get_thumb_color(egui_view_t *self);
/** Default draw hook used by the slider API table. */
void egui_view_slider_on_draw(egui_view_t *self);
/** Initialize the slider widget with default colors and range. */
void egui_view_slider_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SLIDER_H_ */
