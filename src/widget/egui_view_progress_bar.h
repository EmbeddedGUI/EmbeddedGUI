#ifndef _EGUI_VIEW_PROGRESS_BAR_H_
#define _EGUI_VIEW_PROGRESS_BAR_H_

#include "egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Listener fired when the progress value changes. */
typedef void (*egui_view_on_progress_changed_listener_t)(egui_view_t *self, uint8_t progress);

typedef struct egui_view_progress_bar egui_view_progress_bar_t;
/**
 * @brief Horizontal progress indicator with optional thumb-style control knob.
 *
 * The widget stores one progress value in the `0..100` range and uses shared
 * linear-value helpers to compute track, fill, and knob geometry.
 */
struct egui_view_progress_bar
{
    egui_view_t base;

    egui_view_on_progress_changed_listener_t on_progress_changed;

    uint8_t process;
    uint8_t is_show_control;
    egui_color_t control_color;
    egui_color_t bk_color;
    egui_color_t progress_color;
};

// ============== Progress Bar Params ==============
typedef struct egui_view_progress_bar_params egui_view_progress_bar_params_t;
/**
 * @brief Construction-time parameter block for one progress bar.
 */
struct egui_view_progress_bar_params
{
    egui_region_t region;
    uint8_t process;
};

/** Build a progress-bar parameter block with region and initial percentage. */
#define EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT(_name, _x, _y, _w, _h, _val)                                                                                        \
    static const egui_view_progress_bar_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .process = (_val)}

/** Apply a progress-bar parameter block after initialization. */
void egui_view_progress_bar_apply_params(egui_view_t *self, const egui_view_progress_bar_params_t *params);
/** Initialize a progress bar and immediately apply its parameter block. */
void egui_view_progress_bar_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_progress_bar_params_t *params);

/** Register the callback fired when the progress changes. */
void egui_view_progress_bar_set_on_progress_listener(egui_view_t *self, egui_view_on_progress_changed_listener_t listener);
/** Return the callback fired when the progress changes. */
egui_view_on_progress_changed_listener_t egui_view_progress_bar_get_on_progress_listener(egui_view_t *self);
/** Set the progress percentage in the range `0..100`. */
void egui_view_progress_bar_set_process(egui_view_t *self, uint8_t process);
/** Return the current progress percentage (0..100). */
uint8_t egui_view_progress_bar_get_process(egui_view_t *self);
/** Return the background track color. */
egui_color_t egui_view_progress_bar_get_bk_color(egui_view_t *self);
/** Return the fill color used for the completed portion of the track. */
egui_color_t egui_view_progress_bar_get_progress_color(egui_view_t *self);
/** Return the color of the optional control knob. */
egui_color_t egui_view_progress_bar_get_control_color(egui_view_t *self);
/** Return 1 when the control knob is shown, 0 otherwise. */
uint8_t egui_view_progress_bar_get_is_show_control(egui_view_t *self);
/** Default draw hook used by the progress-bar API table. */
void egui_view_progress_bar_on_draw(egui_view_t *self);
/** Initialize the base progress bar widget. */
void egui_view_progress_bar_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_PROGRESS_BAR_H_ */
