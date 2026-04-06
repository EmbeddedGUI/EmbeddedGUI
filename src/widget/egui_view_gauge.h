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

void egui_view_gauge_apply_params(egui_view_t *self, const egui_view_gauge_params_t *params);
void egui_view_gauge_init_with_params(egui_view_t *self, const egui_view_gauge_params_t *params);

void egui_view_gauge_set_value(egui_view_t *self, uint8_t value);
void egui_view_gauge_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_gauge_set_text_color(egui_view_t *self, egui_color_t color);
void egui_view_gauge_on_draw(egui_view_t *self);
void egui_view_gauge_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_GAUGE_H_ */
