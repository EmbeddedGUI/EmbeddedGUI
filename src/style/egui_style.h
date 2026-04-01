#ifndef _EGUI_STYLE_H_
#define _EGUI_STYLE_H_

#include "core/egui_common.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations to avoid circular includes */
struct egui_gradient;
#ifndef EGUI_GRADIENT_T_DEFINED
#define EGUI_GRADIENT_T_DEFINED
typedef struct egui_gradient egui_gradient_t;
#endif

/* Style property flags */
#define EGUI_STYLE_PROP_BG_COLOR    (1 << 0)
#define EGUI_STYLE_PROP_BG_GRADIENT (1 << 1)
#define EGUI_STYLE_PROP_BORDER      (1 << 2)
#define EGUI_STYLE_PROP_RADIUS      (1 << 3)
#define EGUI_STYLE_PROP_PADDING     (1 << 4)
#define EGUI_STYLE_PROP_TEXT_COLOR  (1 << 5)
#define EGUI_STYLE_PROP_TEXT_FONT   (1 << 6)
#define EGUI_STYLE_PROP_SHADOW      (1 << 7)
#define EGUI_STYLE_PROP_ALPHA       (1 << 8)

/* Part identifiers for composite widgets */
typedef enum
{
    EGUI_PART_MAIN = 0,
    EGUI_PART_INDICATOR,
    EGUI_PART_KNOB,
    EGUI_PART_SELECTED,
    EGUI_PART_MAX = 4,
} egui_part_t;

/* State identifiers */
typedef enum
{
    EGUI_STATE_NORMAL = 0,
    EGUI_STATE_PRESSED,
    EGUI_STATE_DISABLED,
    EGUI_STATE_FOCUSED,
    EGUI_STATE_CHECKED,
    EGUI_STATE_MAX = 5,
} egui_state_t;

/* Style structure - designed to be const (ROM-resident), ~32 bytes */
typedef struct egui_style
{
    uint16_t flags;

    /* Background */
    egui_color_t bg_color;
    egui_alpha_t bg_alpha;
    const egui_gradient_t *bg_gradient;

    /* Border */
    egui_color_t border_color;
    egui_alpha_t border_alpha;
    egui_dim_t border_width;

    /* Shape */
    egui_dim_t radius;

    /* Padding */
    egui_dim_t pad_top;
    egui_dim_t pad_bottom;
    egui_dim_t pad_left;
    egui_dim_t pad_right;

    /* Text */
    egui_color_t text_color;
    egui_alpha_t text_alpha;
    const egui_font_t *text_font;

    /* Shadow */
    const egui_shadow_t *shadow;
} egui_style_t;

/* Widget style descriptor: [part][state] lookup table */
typedef struct egui_widget_style_desc
{
    uint8_t part_count;
    const egui_style_t *const *styles; /* flat array: styles[part * EGUI_STATE_MAX + state] */
} egui_widget_style_desc_t;

/* O(1) style lookup */
static inline const egui_style_t *egui_style_get(const egui_widget_style_desc_t *desc, egui_part_t part, egui_state_t state)
{
    if (desc == NULL || part >= desc->part_count || state >= EGUI_STATE_MAX)
    {
        return NULL;
    }
    return desc->styles[part * EGUI_STATE_MAX + state];
}

/* Get current state enum from view state bits */
egui_state_t egui_style_get_view_state(const void *view);

/* Convenience: get style for a view's current state */
static inline const egui_style_t *egui_style_get_current(const egui_widget_style_desc_t *desc, egui_part_t part, const void *view)
{
    return egui_style_get(desc, part, egui_style_get_view_state(view));
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_STYLE_H_ */
