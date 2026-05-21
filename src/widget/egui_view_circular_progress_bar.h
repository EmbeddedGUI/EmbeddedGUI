#ifndef _EGUI_VIEW_CIRCULAR_PROGRESS_BAR_H_
#define _EGUI_VIEW_CIRCULAR_PROGRESS_BAR_H_

#include "egui_view.h"
#include "egui_view_progress_bar.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_circular_progress_bar egui_view_circular_progress_bar_t;
typedef struct egui_view_circular_progress_bar_text_ops egui_view_circular_progress_bar_text_ops_t;
struct egui_view_circular_progress_bar
{
    egui_view_t base;

    egui_view_on_progress_changed_listener_t on_progress_changed;

    uint8_t process;
    egui_dim_t stroke_width;
    int16_t start_angle;
    egui_color_t bk_color;
    egui_color_t progress_color;
    egui_color_t text_color;
    const egui_font_t *font; // NULL falls back to EGUI_CONFIG_FONT_DEFAULT
    const egui_view_circular_progress_bar_text_ops_t *text_ops;
};

// ============== Circular Progress Bar Params ==============
typedef struct egui_view_circular_progress_bar_params egui_view_circular_progress_bar_params_t;
struct egui_view_circular_progress_bar_params
{
    egui_region_t region;
    uint8_t process;
};

#define EGUI_VIEW_CIRCULAR_PROGRESS_BAR_PARAMS_INIT(_name, _x, _y, _w, _h, _val)                                                                               \
    static const egui_view_circular_progress_bar_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .process = (_val)}

/** Apply a circular progress bar parameter block after initialization. */
void egui_view_circular_progress_bar_apply_params(egui_view_t *self, const egui_view_circular_progress_bar_params_t *params);
/** Initialize a circular progress bar and immediately apply its parameter block. */
void egui_view_circular_progress_bar_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_circular_progress_bar_params_t *params);

/** Register the callback fired when the displayed progress changes. */
void egui_view_circular_progress_bar_set_on_progress_listener(egui_view_t *self, egui_view_on_progress_changed_listener_t listener);
/** Return the callback fired when the displayed progress changes. */
egui_view_on_progress_changed_listener_t egui_view_circular_progress_bar_get_on_progress_listener(egui_view_t *self);
/** Set the progress in the 0-100 range and fire the listener on real changes. */
void egui_view_circular_progress_bar_set_process(egui_view_t *self, uint8_t process);
/** Return the current progress value (0-100). */
uint8_t egui_view_circular_progress_bar_get_process(egui_view_t *self);
/** Set the ring thickness used for both the track and progress arc. */
void egui_view_circular_progress_bar_set_stroke_width(egui_view_t *self, egui_dim_t stroke_width);
/** Return the current arc stroke width in pixels. */
egui_dim_t egui_view_circular_progress_bar_get_stroke_width(egui_view_t *self);
/** Set the color of the filled progress arc. */
void egui_view_circular_progress_bar_set_progress_color(egui_view_t *self, egui_color_t color);
/** Return the color of the filled progress arc. */
egui_color_t egui_view_circular_progress_bar_get_progress_color(egui_view_t *self);
/** Set the color of the background track arc. */
void egui_view_circular_progress_bar_set_bk_color(egui_view_t *self, egui_color_t color);
/** Return the color of the background track arc. */
egui_color_t egui_view_circular_progress_bar_get_bk_color(egui_view_t *self);
/** Set the color of the centered progress text. */
void egui_view_circular_progress_bar_set_text_color(egui_view_t *self, egui_color_t color);
/** Return the color of the centered progress text. */
egui_color_t egui_view_circular_progress_bar_get_text_color(egui_view_t *self);
/** Override the text font. Passing NULL restores the default percent renderer. */
void egui_view_circular_progress_bar_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the optional text font override, or NULL when the basic renderer is used. */
const egui_font_t *egui_view_circular_progress_bar_get_font(egui_view_t *self);
/** Default draw hook used by the circular progress bar API table. */
void egui_view_circular_progress_bar_on_draw(egui_view_t *self);
/** Initialize a circular progress indicator with centered text. */
void egui_view_circular_progress_bar_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CIRCULAR_PROGRESS_BAR_H_ */
