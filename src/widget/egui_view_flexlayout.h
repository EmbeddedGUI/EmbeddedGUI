#ifndef _EGUI_VIEW_FLEXLAYOUT_H_
#define _EGUI_VIEW_FLEXLAYOUT_H_

#include "egui_view_group.h"

#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_FLEXLAYOUT

typedef enum
{
    EGUI_FLEX_DIRECTION_ROW = 0,
    EGUI_FLEX_DIRECTION_COLUMN = 1,
} egui_flex_direction_t;

typedef enum
{
    EGUI_FLEX_WRAP_NOWRAP = 0,
    EGUI_FLEX_WRAP_WRAP = 1,
} egui_flex_wrap_t;

typedef enum
{
    EGUI_FLEX_JUSTIFY_START = 0,
    EGUI_FLEX_JUSTIFY_END = 1,
    EGUI_FLEX_JUSTIFY_CENTER = 2,
    EGUI_FLEX_JUSTIFY_SPACE_BETWEEN = 3,
    EGUI_FLEX_JUSTIFY_SPACE_AROUND = 4,
    EGUI_FLEX_JUSTIFY_SPACE_EVENLY = 5,
} egui_flex_justify_t;

typedef enum
{
    EGUI_FLEX_ALIGN_START = 0,
    EGUI_FLEX_ALIGN_END = 1,
    EGUI_FLEX_ALIGN_CENTER = 2,
    EGUI_FLEX_ALIGN_STRETCH = 3,
} egui_flex_align_t;

typedef struct egui_view_flexlayout egui_view_flexlayout_t;
struct egui_view_flexlayout
{
    egui_view_group_t base;
    uint8_t direction;       /* egui_flex_direction_t */
    uint8_t wrap;            /* egui_flex_wrap_t */
    uint8_t justify_content; /* egui_flex_justify_t */
    uint8_t align_items;     /* egui_flex_align_t */
    uint8_t align_content;   /* egui_flex_justify_t — multi-line cross-axis */
    egui_dim_t row_gap;
    egui_dim_t col_gap;
};

/** Initialize the flexlayout container with default properties. */
void egui_view_flexlayout_init(egui_view_t *self, egui_core_t *core);
/** Set main-axis direction: ROW (default) or COLUMN. */
void egui_view_flexlayout_set_direction(egui_view_t *self, uint8_t direction);
/** Return the configured main-axis direction. */
uint8_t egui_view_flexlayout_get_direction(egui_view_t *self);
/** Enable or disable wrapping when children overflow the main axis. */
void egui_view_flexlayout_set_wrap(egui_view_t *self, uint8_t wrap);
/** Return whether wrapping is enabled. */
uint8_t egui_view_flexlayout_get_wrap(egui_view_t *self);
/** Set how children are distributed along the main axis within each line. */
void egui_view_flexlayout_set_justify_content(egui_view_t *self, uint8_t justify);
/** Return the configured main-axis distribution mode. */
uint8_t egui_view_flexlayout_get_justify_content(egui_view_t *self);
/** Set how children are aligned along the cross axis within each line. */
void egui_view_flexlayout_set_align_items(egui_view_t *self, uint8_t align);
/** Return the configured per-line cross-axis alignment mode. */
uint8_t egui_view_flexlayout_get_align_items(egui_view_t *self);
/** Set how multiple lines are distributed along the cross axis. */
void egui_view_flexlayout_set_align_content(egui_view_t *self, uint8_t align);
/** Return the configured multi-line cross-axis distribution mode. */
uint8_t egui_view_flexlayout_get_align_content(egui_view_t *self);
/** Set the spacing between rows and between columns. */
void egui_view_flexlayout_set_gap(egui_view_t *self, egui_dim_t row_gap, egui_dim_t col_gap);
/** Return the configured spacing between flex rows. */
egui_dim_t egui_view_flexlayout_get_row_gap(egui_view_t *self);
/** Return the configured spacing between flex columns. */
egui_dim_t egui_view_flexlayout_get_col_gap(egui_view_t *self);
/** Run the flex layout pass, computing and applying positions for all visible children. */
void egui_view_flexlayout_layout_childs(egui_view_t *self);

#endif /* EGUI_CONFIG_FUNCTION_FLEXLAYOUT */

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_FLEXLAYOUT_H_ */
