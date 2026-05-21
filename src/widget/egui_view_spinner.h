#ifndef _EGUI_VIEW_SPINNER_H_
#define _EGUI_VIEW_SPINNER_H_

#include "egui_view.h"
#include "core/egui_timer.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_spinner egui_view_spinner_t;
/**
 * @brief Indeterminate loading indicator rendered as a rotating arc.
 *
 * The widget owns a timer that advances `rotation_angle` while spinning. It is
 * intentionally simple: one color, one stroke width, one arc length.
 */
struct egui_view_spinner
{
    egui_view_t base;

    int16_t arc_length;
    int16_t rotation_angle;
    egui_dim_t stroke_width;
    egui_color_t color;
    uint8_t is_spinning;
    egui_timer_t spin_timer;
};

// ============== Spinner Params ==============
typedef struct egui_view_spinner_params egui_view_spinner_params_t;
/**
 * @brief Construction-time parameter block for one spinner widget.
 */
struct egui_view_spinner_params
{
    egui_region_t region;
};

/** Build a spinner parameter block with region only. */
#define EGUI_VIEW_SPINNER_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_spinner_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

/** Apply a spinner parameter block after initialization. */
void egui_view_spinner_apply_params(egui_view_t *self, const egui_view_spinner_params_t *params);
/** Initialize a spinner and immediately apply its parameter block. */
void egui_view_spinner_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_spinner_params_t *params);

/** Start the rotation timer. The spinner is idle until this is called. */
void egui_view_spinner_start(egui_view_t *self);
/** Stop the rotation timer and freeze the current angle. */
void egui_view_spinner_stop(egui_view_t *self);
/** Return whether the rotation timer is currently active. */
int egui_view_spinner_is_spinning(egui_view_t *self);
/** Set the color of the rotating arc. */
void egui_view_spinner_set_color(egui_view_t *self, egui_color_t color);
/** Return the color of the rotating arc. */
egui_color_t egui_view_spinner_get_color(egui_view_t *self);
/** Default draw hook used by the spinner API table. */
void egui_view_spinner_on_draw(egui_view_t *self);
/** Initialize a spinner that is idle by default. */
void egui_view_spinner_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SPINNER_H_ */
