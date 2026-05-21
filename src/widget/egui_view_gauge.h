#ifndef _EGUI_VIEW_GAUGE_H_
#define _EGUI_VIEW_GAUGE_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_gauge egui_view_gauge_t;
typedef struct egui_view_gauge_text_ops egui_view_gauge_text_ops_t;
struct egui_view_gauge
{
    egui_view_t base;

    uint8_t value; // 0-100
    egui_dim_t stroke_width;
    egui_dim_t needle_width; // needle line width, independent of arc stroke
    int16_t start_angle;     // default 150 (~7 o'clock, 0=3 o'clock clockwise)
    int16_t sweep_angle;     // default 240 (sweep 240 degrees, gap at bottom)
    egui_color_t bk_color;
    egui_color_t progress_color;
    egui_color_t needle_color;
    egui_color_t text_color;
    const egui_font_t *font; // NULL falls back to EGUI_CONFIG_FONT_DEFAULT
    const egui_view_gauge_text_ops_t *text_ops;
};

// ============== Gauge Params ==============
typedef struct egui_view_gauge_params egui_view_gauge_params_t;
struct egui_view_gauge_params
{
    egui_region_t region;
    uint8_t value;
};

#define EGUI_VIEW_GAUGE_PARAMS_INIT(_name, _x, _y, _w, _h, _val)                                                                                               \
    static const egui_view_gauge_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .value = (_val)}

/** Apply a gauge parameter block after initialization. */
void egui_view_gauge_apply_params(egui_view_t *self, const egui_view_gauge_params_t *params);
/** Initialize a gauge and immediately apply its parameter block. */
void egui_view_gauge_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_gauge_params_t *params);

/** Set the gauge percentage in the 0-100 range. */
void egui_view_gauge_set_value(egui_view_t *self, uint8_t value);
/** Return the current gauge percentage. */
uint8_t egui_view_gauge_get_value(egui_view_t *self);
/** Set the arc stroke width. */
void egui_view_gauge_set_stroke_width(egui_view_t *self, egui_dim_t stroke_width);
/** Return the arc stroke width. */
egui_dim_t egui_view_gauge_get_stroke_width(egui_view_t *self);
/** Set the needle line width. */
void egui_view_gauge_set_needle_width(egui_view_t *self, egui_dim_t needle_width);
/** Return the needle line width. */
egui_dim_t egui_view_gauge_get_needle_width(egui_view_t *self);
/** Set the start angle in degrees. */
void egui_view_gauge_set_start_angle(egui_view_t *self, int16_t start_angle);
/** Return the start angle in degrees. */
int16_t egui_view_gauge_get_start_angle(egui_view_t *self);
/** Set the sweep angle in degrees. */
void egui_view_gauge_set_sweep_angle(egui_view_t *self, int16_t sweep_angle);
/** Return the sweep angle in degrees. */
int16_t egui_view_gauge_get_sweep_angle(egui_view_t *self);
/** Set the background track color. */
void egui_view_gauge_set_bk_color(egui_view_t *self, egui_color_t color);
/** Return the background track color. */
egui_color_t egui_view_gauge_get_bk_color(egui_view_t *self);
/** Set the progress arc fill color. */
void egui_view_gauge_set_progress_color(egui_view_t *self, egui_color_t color);
/** Return the progress arc fill color. */
egui_color_t egui_view_gauge_get_progress_color(egui_view_t *self);
/** Set the needle and tick color. */
void egui_view_gauge_set_needle_color(egui_view_t *self, egui_color_t color);
/** Return the needle and tick color. */
egui_color_t egui_view_gauge_get_needle_color(egui_view_t *self);
/** Override the font used for the centered value text. Passing NULL restores the default renderer. */
void egui_view_gauge_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the explicit centered value text font, or NULL when the compact default renderer is used. */
const egui_font_t *egui_view_gauge_get_font(egui_view_t *self);
/** Set the color used for the centered value text. */
void egui_view_gauge_set_text_color(egui_view_t *self, egui_color_t color);
/** Return the color used for the centered value text. */
egui_color_t egui_view_gauge_get_text_color(egui_view_t *self);
/** Default draw hook used by the gauge API table. */
void egui_view_gauge_on_draw(egui_view_t *self);
/** Initialize a gauge with a needle, tick marks, and centered value text. */
void egui_view_gauge_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_GAUGE_H_ */
