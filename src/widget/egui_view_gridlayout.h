#ifndef _EGUI_VIEW_GRIDLAYOUT_H_
#define _EGUI_VIEW_GRIDLAYOUT_H_

#include "egui_view_group.h"
#include "core/egui_theme.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_gridlayout egui_view_gridlayout_t;
struct egui_view_gridlayout
{
    egui_view_group_t base;

    uint8_t col_count;
    uint8_t align_type;
};

// ============== GridLayout Params ==============
typedef struct egui_view_gridlayout_params egui_view_gridlayout_params_t;
struct egui_view_gridlayout_params
{
    egui_region_t region;
    uint8_t col_count;
    uint8_t align_type;
};

#define EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(_name, _x, _y, _w, _h, _cols, _align)                                                                                 \
    static const egui_view_gridlayout_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .col_count = (_cols), .align_type = (_align)}

void egui_view_gridlayout_apply_params(egui_view_t *self, const egui_view_gridlayout_params_t *params);
void egui_view_gridlayout_init_with_params(egui_view_t *self, const egui_view_gridlayout_params_t *params);

void egui_view_gridlayout_set_col_count(egui_view_t *self, uint8_t col_count);
void egui_view_gridlayout_set_align_type(egui_view_t *self, uint8_t align_type);
void egui_view_gridlayout_layout_childs(egui_view_t *self);
void egui_view_gridlayout_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_GRIDLAYOUT_H_ */
