#ifndef _EGUI_VIEW_PAGE_INDICATOR_H_
#define _EGUI_VIEW_PAGE_INDICATOR_H_

#include "egui_view.h"
#include "core/egui_theme.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_page_indicator egui_view_page_indicator_t;
struct egui_view_page_indicator
{
    egui_view_t base;

    uint8_t total_count;
    uint8_t current_index;
    egui_dim_t dot_radius;
    egui_dim_t dot_spacing;
    egui_alpha_t alpha;
    egui_color_t active_color;
    egui_color_t inactive_color;
};

// ============== PageIndicator Params ==============
typedef struct egui_view_page_indicator_params egui_view_page_indicator_params_t;
struct egui_view_page_indicator_params
{
    egui_region_t region;
    uint8_t total_count;
    uint8_t current_index;
};

#define EGUI_VIEW_PAGE_INDICATOR_PARAMS_INIT(_name, _x, _y, _w, _h, _total, _current)                                                                          \
    static const egui_view_page_indicator_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .total_count = (_total), .current_index = (_current)}

void egui_view_page_indicator_apply_params(egui_view_t *self, const egui_view_page_indicator_params_t *params);
void egui_view_page_indicator_init_with_params(egui_view_t *self, const egui_view_page_indicator_params_t *params);

void egui_view_page_indicator_set_total_count(egui_view_t *self, uint8_t total_count);
void egui_view_page_indicator_set_current_index(egui_view_t *self, uint8_t current_index);
void egui_view_page_indicator_on_draw(egui_view_t *self);
void egui_view_page_indicator_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_PAGE_INDICATOR_H_ */
