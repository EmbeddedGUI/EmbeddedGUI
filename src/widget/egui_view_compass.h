#ifndef _EGUI_VIEW_COMPASS_H_
#define _EGUI_VIEW_COMPASS_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_compass egui_view_compass_t;
struct egui_view_compass
{
    egui_view_t base;

    int16_t heading; // 0-359 degrees
    egui_dim_t stroke_width;
    egui_color_t dial_color;
    egui_color_t north_color;
    egui_color_t needle_color;
    egui_color_t text_color;
    uint8_t show_degree;
    const egui_font_t *font;
};

// ============== Compass Params ==============
typedef struct egui_view_compass_params egui_view_compass_params_t;
struct egui_view_compass_params
{
    egui_region_t region;
    int16_t heading;
};

#define EGUI_VIEW_COMPASS_PARAMS_INIT(_name, _x, _y, _w, _h, _heading)                                                                                         \
    static const egui_view_compass_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .heading = (_heading)}

/** Apply a compass parameter block after initialization. */
void egui_view_compass_apply_params(egui_view_t *self, const egui_view_compass_params_t *params);
/** Initialize a compass and immediately apply its parameter block. */
void egui_view_compass_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_compass_params_t *params);

/** Set the heading in degrees. Negative and overflow values are normalized into 0-359. */
void egui_view_compass_set_heading(egui_view_t *self, int16_t heading);
/** Return the stored heading in degrees. */
int16_t egui_view_compass_get_heading(egui_view_t *self);
/** Show or hide the numeric degree label drawn in the center. */
void egui_view_compass_set_show_degree(egui_view_t *self, uint8_t show);
/** Return whether the numeric degree label is shown in the center. */
uint8_t egui_view_compass_get_show_degree(egui_view_t *self);
/** Set the dial circle stroke width. */
void egui_view_compass_set_stroke_width(egui_view_t *self, egui_dim_t stroke_width);
/** Return the dial circle stroke width. */
egui_dim_t egui_view_compass_get_stroke_width(egui_view_t *self);
/** Set the dial and non-north tick color. */
void egui_view_compass_set_dial_color(egui_view_t *self, egui_color_t color);
/** Return the dial and non-north tick color. */
egui_color_t egui_view_compass_get_dial_color(egui_view_t *self);
/** Set the north needle and north tick color. */
void egui_view_compass_set_north_color(egui_view_t *self, egui_color_t color);
/** Return the north needle and north tick color. */
egui_color_t egui_view_compass_get_north_color(egui_view_t *self);
/** Set the south needle color. */
void egui_view_compass_set_needle_color(egui_view_t *self, egui_color_t color);
/** Return the south needle color. */
egui_color_t egui_view_compass_get_needle_color(egui_view_t *self);
/** Set the numeric degree label color. */
void egui_view_compass_set_text_color(egui_view_t *self, egui_color_t color);
/** Return the numeric degree label color. */
egui_color_t egui_view_compass_get_text_color(egui_view_t *self);
/** Set the font used for the numeric degree label. */
void egui_view_compass_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the font used for the numeric degree label. */
const egui_font_t *egui_view_compass_get_font(egui_view_t *self);
/** Default draw hook used by the compass API table. */
void egui_view_compass_on_draw(egui_view_t *self);
/** Initialize a compass widget with numeric heading display enabled. */
void egui_view_compass_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_COMPASS_H_ */
