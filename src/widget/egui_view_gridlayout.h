#ifndef _EGUI_VIEW_GRIDLAYOUT_H_
#define _EGUI_VIEW_GRIDLAYOUT_H_

#include "egui_view_group.h"

/**
 * @brief Grid container that arranges visible children into fixed-width columns.
 *
 * Children are assigned to cells in append order. Each row height is driven by
 * the tallest visible child in that row, and each child is centered inside its
 * own column cell horizontally.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_gridlayout egui_view_gridlayout_t;
struct egui_view_gridlayout
{
    egui_view_group_t base;

    /* Number of columns used when partitioning the container width. */
    uint8_t col_count;
    /* Stored alignment hint for future layout-policy expansion. */
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

/** Build a grid-layout parameter block with region, column count, and alignment hint. */
#define EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(_name, _x, _y, _w, _h, _cols, _align)                                                                                 \
    static const egui_view_gridlayout_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .col_count = (_cols), .align_type = (_align)}

/** Apply a grid-layout parameter block after initialization. */
void egui_view_gridlayout_apply_params(egui_view_t *self, const egui_view_gridlayout_params_t *params);
/** Initialize a grid layout and immediately apply its parameter block. */
void egui_view_gridlayout_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_gridlayout_params_t *params);

/** Set the number of columns. A value of 0 is normalized to 1. */
void egui_view_gridlayout_set_col_count(egui_view_t *self, uint8_t col_count);
/** Return the configured number of grid columns. */
uint8_t egui_view_gridlayout_get_col_count(egui_view_t *self);
/** Store an alignment hint for the grid. The current built-in layout helper still centers each child inside its cell. */
void egui_view_gridlayout_set_align_type(egui_view_t *self, uint8_t align_type);
/** Return the stored alignment hint. */
uint8_t egui_view_gridlayout_get_align_type(egui_view_t *self);
/** Position visible children into rows and columns using the current container width. */
void egui_view_gridlayout_layout_childs(egui_view_t *self);
/** Initialize a group container with a default two-column grid configuration. */
void egui_view_gridlayout_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_GRIDLAYOUT_H_ */
