#ifndef _EGUI_VIEW_LINEARLAYOUT_H_
#define _EGUI_VIEW_LINEARLAYOUT_H_

#include "egui_view_group.h"
#include "core/egui_scroller.h"
#include "font/egui_font.h"

/**
 * @brief Simple linear container that arranges children in one row or one column.
 *
 * The linearlayout delegates most heavy lifting to the shared group-layout
 * helper, but stores the direction, alignment, and optional auto-size flags
 * that control the next layout pass.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_linearlayout egui_view_linearlayout_t;
struct egui_view_linearlayout
{
    egui_view_group_t base;

    /* Alignment flags consumed by the shared group-layout helper. */
    uint8_t align_type;
    /* When nonzero, width grows to fit child content during layout. */
    uint8_t is_auto_width;
    /* When nonzero, height grows to fit child content during layout. */
    uint8_t is_auto_height;
    /* `0` for vertical flow, `1` for horizontal flow. */
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

/** Build a vertical linearlayout parameter block. */
#define EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT(_name, _x, _y, _w, _h, _align)                                                                                      \
    static const egui_view_linearlayout_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .align_type = (_align), .is_orientation_horizontal = 0}

/** Build a horizontal linearlayout parameter block. */
#define EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT_H(_name, _x, _y, _w, _h, _align)                                                                                    \
    static const egui_view_linearlayout_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .align_type = (_align), .is_orientation_horizontal = 1}

/** Apply a linearlayout parameter block after initialization. */
void egui_view_linearlayout_apply_params(egui_view_t *self, const egui_view_linearlayout_params_t *params);
/** Initialize a linearlayout and immediately apply its parameter block. */
void egui_view_linearlayout_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_linearlayout_params_t *params);

/** Set child alignment used by the next layout pass. */
void egui_view_linearlayout_set_align_type(egui_view_t *self, uint8_t align_type);
/** Enable or disable automatic width growth based on child content. */
void egui_view_linearlayout_set_auto_width(egui_view_t *self, uint8_t is_auto_width);
/** Enable or disable automatic height growth based on child content. */
void egui_view_linearlayout_set_auto_height(egui_view_t *self, uint8_t is_auto_height);
/** Set vertical (`0`) or horizontal (`1`) child flow. */
void egui_view_linearlayout_set_orientation(egui_view_t *self, uint8_t is_horizontal);
/** Return the currently configured alignment flags. */
uint8_t egui_view_linearlayout_is_align_type(egui_view_t *self);
/** Return the currently configured alignment flags. */
uint8_t egui_view_linearlayout_get_align_type(egui_view_t *self);
/** Return non-zero when auto width is enabled. */
uint8_t egui_view_linearlayout_is_auto_width(egui_view_t *self);
/** Return non-zero when auto width is enabled. */
uint8_t egui_view_linearlayout_get_auto_width(egui_view_t *self);
/** Return non-zero when auto height is enabled. */
uint8_t egui_view_linearlayout_is_auto_height(egui_view_t *self);
/** Return non-zero when auto height is enabled. */
uint8_t egui_view_linearlayout_get_auto_height(egui_view_t *self);
/** Return non-zero when the layout direction is horizontal. */
uint8_t egui_view_linearlayout_is_orientation_horizontal(egui_view_t *self);
/** Return non-zero when the layout direction is horizontal. */
uint8_t egui_view_linearlayout_get_orientation(egui_view_t *self);
/** Recalculate child positions using the current direction, alignment, and auto-size flags. */
void egui_view_linearlayout_layout_childs(egui_view_t *self);
/** Initialize the linearlayout container base. */
void egui_view_linearlayout_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_LINEARLAYOUT_H_ */
