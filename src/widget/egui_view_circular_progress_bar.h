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
    const egui_font_t *font; // NULL = auto-select based on size
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

void egui_view_circular_progress_bar_apply_params(egui_view_t *self, const egui_view_circular_progress_bar_params_t *params);
void egui_view_circular_progress_bar_init_with_params(egui_view_t *self, const egui_view_circular_progress_bar_params_t *params);

void egui_view_circular_progress_bar_set_on_progress_listener(egui_view_t *self, egui_view_on_progress_changed_listener_t listener);
void egui_view_circular_progress_bar_set_process(egui_view_t *self, uint8_t process);
void egui_view_circular_progress_bar_set_stroke_width(egui_view_t *self, egui_dim_t stroke_width);
void egui_view_circular_progress_bar_set_progress_color(egui_view_t *self, egui_color_t color);
void egui_view_circular_progress_bar_set_bk_color(egui_view_t *self, egui_color_t color);
void egui_view_circular_progress_bar_set_text_color(egui_view_t *self, egui_color_t color);
void egui_view_circular_progress_bar_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_circular_progress_bar_on_draw(egui_view_t *self);
void egui_view_circular_progress_bar_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_CIRCULAR_PROGRESS_BAR_H_ */
