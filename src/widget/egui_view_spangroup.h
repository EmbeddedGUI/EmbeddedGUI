#ifndef _EGUI_VIEW_SPANGROUP_H_
#define _EGUI_VIEW_SPANGROUP_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SPANGROUP_MAX_SPANS 8

typedef struct egui_view_span
{
    const char *text;
    const egui_font_t *font;
    egui_color_t color;
} egui_view_span_t;

typedef struct egui_view_spangroup egui_view_spangroup_t;
struct egui_view_spangroup
{
    egui_view_t base;

    egui_view_span_t spans[EGUI_VIEW_SPANGROUP_MAX_SPANS];
    uint8_t span_count;
    uint8_t line_spacing;
    uint8_t align;
};

// ============== Spangroup Params ==============
typedef struct egui_view_spangroup_params egui_view_spangroup_params_t;
struct egui_view_spangroup_params
{
    egui_region_t region;
};

#define EGUI_VIEW_SPANGROUP_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_spangroup_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

int egui_view_spangroup_add_span(egui_view_t *self, const char *text, const egui_font_t *font, egui_color_t color);
void egui_view_spangroup_clear(egui_view_t *self);
void egui_view_spangroup_set_align(egui_view_t *self, uint8_t align);
void egui_view_spangroup_set_line_spacing(egui_view_t *self, uint8_t spacing);

void egui_view_spangroup_apply_params(egui_view_t *self, const egui_view_spangroup_params_t *params);
void egui_view_spangroup_init_with_params(egui_view_t *self, const egui_view_spangroup_params_t *params);
void egui_view_spangroup_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SPANGROUP_H_ */
