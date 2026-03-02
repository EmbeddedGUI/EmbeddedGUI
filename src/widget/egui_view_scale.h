#ifndef _EGUI_VIEW_SCALE_H_
#define _EGUI_VIEW_SCALE_H_

#include "egui_view.h"
#include "core/egui_theme.h"
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

void egui_view_scale_apply_params(egui_view_t *self, const egui_view_scale_params_t *params);
void egui_view_scale_init_with_params(egui_view_t *self, const egui_view_scale_params_t *params);

void egui_view_scale_set_range(egui_view_t *self, int16_t min, int16_t max);
void egui_view_scale_set_value(egui_view_t *self, int16_t value);
void egui_view_scale_set_ticks(egui_view_t *self, uint8_t major_count, uint8_t minor_count);
void egui_view_scale_set_orientation(egui_view_t *self, uint8_t is_horizontal);
void egui_view_scale_set_tick_color(egui_view_t *self, egui_color_t color);
void egui_view_scale_set_label_color(egui_view_t *self, egui_color_t color);
void egui_view_scale_set_indicator_color(egui_view_t *self, egui_color_t color);
void egui_view_scale_show_labels(egui_view_t *self, uint8_t show);
void egui_view_scale_show_indicator(egui_view_t *self, uint8_t show);
void egui_view_scale_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_scale_on_draw(egui_view_t *self);
void egui_view_scale_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SCALE_H_ */
