#ifndef _EGUI_VIEW_WINDOW_H_
#define _EGUI_VIEW_WINDOW_H_

#include "egui_view_group.h"
#include "egui_view_label.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_window_close_cb_t)(egui_view_t *self);

typedef struct egui_view_window egui_view_window_t;
struct egui_view_window
{
    egui_view_group_t base;

    egui_view_label_t title_label;
    egui_view_group_t content;
    egui_dim_t header_height;
    egui_color_t header_color;
    egui_color_t content_bg_color;
    egui_view_window_close_cb_t on_close;
};

// ============== Window Params ==============
typedef struct egui_view_window_params egui_view_window_params_t;
struct egui_view_window_params
{
    egui_region_t region;
    egui_dim_t header_height;
    const char *title;
};

#define EGUI_VIEW_WINDOW_PARAMS_INIT(_name, _x, _y, _w, _h, _hdr_h, _title)                                                                                    \
    static const egui_view_window_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .header_height = (_hdr_h), .title = (_title)}

void egui_view_window_apply_params(egui_view_t *self, const egui_view_window_params_t *params);
void egui_view_window_init_with_params(egui_view_t *self, const egui_view_window_params_t *params);

void egui_view_window_set_title(egui_view_t *self, const char *title);
void egui_view_window_set_header_height(egui_view_t *self, egui_dim_t height);
void egui_view_window_add_content(egui_view_t *self, egui_view_t *child);
void egui_view_window_set_on_close(egui_view_t *self, egui_view_window_close_cb_t callback);
void egui_view_window_on_draw(egui_view_t *self);
void egui_view_window_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_WINDOW_H_ */
