#ifndef _EGUI_VIEW_STEPPER_H_
#define _EGUI_VIEW_STEPPER_H_

#include "egui_view_page_indicator.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_stepper egui_view_stepper_t;

typedef enum
{
    EGUI_VIEW_STEPPER_MARK_STYLE_DOT = 0,
    EGUI_VIEW_STEPPER_MARK_STYLE_ICON = 1,
} egui_view_stepper_mark_style_t;

struct egui_view_stepper
{
    egui_view_page_indicator_t indicator;
    uint8_t mark_style;
    const char *completed_icon;
    const egui_font_t *icon_font;
};

typedef struct egui_view_stepper_params egui_view_stepper_params_t;
struct egui_view_stepper_params
{
    egui_region_t region;
    uint8_t total_steps;
    uint8_t current_step;
};

#define EGUI_VIEW_STEPPER_PARAMS_INIT(_name, _x, _y, _w, _h, _total, _current)                                                                                 \
    static const egui_view_stepper_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .total_steps = (_total), .current_step = (_current)}

void egui_view_stepper_apply_params(egui_view_t *self, const egui_view_stepper_params_t *params);
void egui_view_stepper_init_with_params(egui_view_t *self, const egui_view_stepper_params_t *params);

void egui_view_stepper_set_total_steps(egui_view_t *self, uint8_t total_steps);
uint8_t egui_view_stepper_get_total_steps(egui_view_t *self);
void egui_view_stepper_set_current_step(egui_view_t *self, uint8_t current_step);
uint8_t egui_view_stepper_get_current_step(egui_view_t *self);
void egui_view_stepper_set_mark_style(egui_view_t *self, egui_view_stepper_mark_style_t style);
void egui_view_stepper_set_completed_icon(egui_view_t *self, const char *icon);
void egui_view_stepper_set_icon_font(egui_view_t *self, const egui_font_t *font);
void egui_view_stepper_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_STEPPER_H_ */
