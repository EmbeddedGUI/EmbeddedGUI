#ifndef _EGUI_VIEW_LINEARLAYOUT_H_
#define _EGUI_VIEW_LINEARLAYOUT_H_

#include "egui_view_group.h"
#include "core/egui_theme.h"
#include "core/egui_scroller.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_linearlayout egui_view_linearlayout_t;
struct egui_view_linearlayout
{
    egui_view_group_t base;

    uint8_t align_type;
    uint8_t is_auto_width;
    uint8_t is_auto_height;
    uint8_t is_orientation_horizontal;
};

// ============== LinearLayout Params ==============
typedef struct egui_view_linearlayout_params egui_view_linearlayout_params_t;
struct egui_view_linearlayout_params
{
    egui_region_t region;
    uint8_t align_type;
    uint8_t is_orientation_horizontal;
};

#define EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT(_name, _x, _y, _w, _h, _align)                                                                                      \
    static const egui_view_linearlayout_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .align_type = (_align), .is_orientation_horizontal = 0}

#define EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT_H(_name, _x, _y, _w, _h, _align)                                                                                    \
    static const egui_view_linearlayout_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .align_type = (_align), .is_orientation_horizontal = 1}

void egui_view_linearlayout_apply_params(egui_view_t *self, const egui_view_linearlayout_params_t *params);
void egui_view_linearlayout_init_with_params(egui_view_t *self, const egui_view_linearlayout_params_t *params);

void egui_view_linearlayout_set_align_type(egui_view_t *self, uint8_t align_type);
void egui_view_linearlayout_set_auto_width(egui_view_t *self, uint8_t is_auto_width);
void egui_view_linearlayout_set_auto_height(egui_view_t *self, uint8_t is_auto_height);
void egui_view_linearlayout_set_orientation(egui_view_t *self, uint8_t is_horizontal);
uint8_t egui_view_linearlayout_is_align_type(egui_view_t *self);
uint8_t egui_view_linearlayout_is_auto_width(egui_view_t *self);
uint8_t egui_view_linearlayout_is_auto_height(egui_view_t *self);
uint8_t egui_view_linearlayout_is_orientation_horizontal(egui_view_t *self);
void egui_view_linearlayout_layout_childs(egui_view_t *self);
void egui_view_linearlayout_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_LINEARLAYOUT_H_ */
