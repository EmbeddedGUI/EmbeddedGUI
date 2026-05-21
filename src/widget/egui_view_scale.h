#ifndef _EGUI_VIEW_SCALE_H_
#define _EGUI_VIEW_SCALE_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_scale egui_view_scale_t;
struct egui_view_scale
{
    egui_view_t base;

    int16_t range_min;
    int16_t range_max;
    int16_t value;
    uint8_t major_tick_count;
    uint8_t minor_tick_count;
    uint8_t major_tick_len;
    uint8_t minor_tick_len;
    uint8_t is_horizontal;
    uint8_t show_labels;
    uint8_t show_indicator;
    egui_color_t tick_color;
    egui_color_t label_color;
    egui_color_t indicator_color;
    const egui_font_t *font;
};

// ============== Scale Params ==============
typedef struct egui_view_scale_params egui_view_scale_params_t;
struct egui_view_scale_params
{
    egui_region_t region;
    int16_t range_min;
    int16_t range_max;
    uint8_t major_tick_count;
};

#define EGUI_VIEW_SCALE_PARAMS_INIT(_name, _x, _y, _w, _h, _min, _max, _major)                                                                                 \
    static const egui_view_scale_params_t _name = {                                                                                                            \
            .region = {{(_x), (_y)}, {(_w), (_h)}}, .range_min = (_min), .range_max = (_max), .major_tick_count = (_major)}

/** Apply region, numeric range, and major tick count from one parameter block. */
void egui_view_scale_apply_params(egui_view_t *self, const egui_view_scale_params_t *params);
/** Initialize a scale view and immediately apply its parameter block. */
void egui_view_scale_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_scale_params_t *params);

/** Set the numeric range shown by the scale. If `max <= min`, the draw hook has nothing meaningful to render. */
void egui_view_scale_set_range(egui_view_t *self, int16_t min, int16_t max);
/** Return the lower bound of the numeric scale range. */
int16_t egui_view_scale_get_range_min(egui_view_t *self);
/** Return the upper bound of the numeric scale range. */
int16_t egui_view_scale_get_range_max(egui_view_t *self);
/** Set the current value marker. Values outside the configured range are clamped. */
void egui_view_scale_set_value(egui_view_t *self, int16_t value);
/** Return the current value marker. */
int16_t egui_view_scale_get_value(egui_view_t *self);
/** Set the number of major ticks and the number of minor ticks inserted between each adjacent major pair. */
void egui_view_scale_set_ticks(egui_view_t *self, uint8_t major_count, uint8_t minor_count);
/** Return the number of major ticks. */
uint8_t egui_view_scale_get_major_tick_count(egui_view_t *self);
/** Return the number of minor ticks inserted between each adjacent major pair. */
uint8_t egui_view_scale_get_minor_tick_count(egui_view_t *self);
/** Switch between horizontal and vertical drawing. */
void egui_view_scale_set_orientation(egui_view_t *self, uint8_t is_horizontal);
/** Return non-zero when the scale is drawn horizontally. */
uint8_t egui_view_scale_get_is_horizontal(egui_view_t *self);
/** Set the color used for major and minor tick lines. */
void egui_view_scale_set_tick_color(egui_view_t *self, egui_color_t color);
/** Return the color used for major and minor tick lines. */
egui_color_t egui_view_scale_get_tick_color(egui_view_t *self);
/** Set the color used for numeric labels when labels are enabled. */
void egui_view_scale_set_label_color(egui_view_t *self, egui_color_t color);
/** Return the color used for numeric labels when labels are enabled. */
egui_color_t egui_view_scale_get_label_color(egui_view_t *self);
/** Set the color used for the current-value indicator line. */
void egui_view_scale_set_indicator_color(egui_view_t *self, egui_color_t color);
/** Return the color used for the current-value indicator line. */
egui_color_t egui_view_scale_get_indicator_color(egui_view_t *self);
/** Show or hide numeric labels. Labels still require a non-NULL font to render. */
void egui_view_scale_show_labels(egui_view_t *self, uint8_t show);
/** Return whether numeric labels are enabled. */
uint8_t egui_view_scale_get_show_labels(egui_view_t *self);
/** Show or hide the current-value indicator line. */
void egui_view_scale_show_indicator(egui_view_t *self, uint8_t show);
/** Return whether the current-value indicator line is enabled. */
uint8_t egui_view_scale_get_show_indicator(egui_view_t *self);
/** Set the font used for numeric labels. */
void egui_view_scale_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the font used for numeric labels. */
const egui_font_t *egui_view_scale_get_font(egui_view_t *self);
/** Default draw hook used by the scale API table. */
void egui_view_scale_on_draw(egui_view_t *self);
/** Initialize the ruler-style scale widget with a default `0..100` horizontal range. */
void egui_view_scale_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SCALE_H_ */
