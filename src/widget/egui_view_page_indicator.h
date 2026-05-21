#ifndef _EGUI_VIEW_PAGE_INDICATOR_H_
#define _EGUI_VIEW_PAGE_INDICATOR_H_

#include "egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_page_indicator egui_view_page_indicator_t;

typedef enum
{
    /** Draw page marks as filled circles. */
    EGUI_VIEW_PAGE_INDICATOR_MARK_STYLE_DOT = 0,
    /** Draw page marks as icon-font glyphs from `icons[]`. */
    EGUI_VIEW_PAGE_INDICATOR_MARK_STYLE_ICON = 1,
} egui_view_page_indicator_mark_style_t;

/**
 * @brief Page-position indicator that can render dots or icon marks.
 *
 * The widget stores only page-count state and presentation fields. It does not
 * manage page switching by itself, so callers update `current_index` when the
 * surrounding pager or activity changes page.
 */
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
    uint8_t mark_style;
    const char *const *icons;
    const egui_font_t *icon_font;
};

// ============== PageIndicator Params ==============
typedef struct egui_view_page_indicator_params egui_view_page_indicator_params_t;
/**
 * @brief Construction-time parameter block for one page indicator.
 */
struct egui_view_page_indicator_params
{
    egui_region_t region;
    uint8_t total_count;
    uint8_t current_index;
};

/** Build a page-indicator parameter block with region and initial page state. */
#define EGUI_VIEW_PAGE_INDICATOR_PARAMS_INIT(_name, _x, _y, _w, _h, _total, _current)                                                                          \
    static const egui_view_page_indicator_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .total_count = (_total), .current_index = (_current)}

/** Apply a page-indicator parameter block after initialization. */
void egui_view_page_indicator_apply_params(egui_view_t *self, const egui_view_page_indicator_params_t *params);
/** Initialize a page indicator and immediately apply its parameter block. */
void egui_view_page_indicator_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_page_indicator_params_t *params);

/** Set how many page marks should be drawn. */
void egui_view_page_indicator_set_total_count(egui_view_t *self, uint8_t total_count);
/** Return how many page marks are currently stored. */
uint8_t egui_view_page_indicator_get_total_count(egui_view_t *self);
/** Select the active page mark. Out-of-range values are clamped to the last page. */
void egui_view_page_indicator_set_current_index(egui_view_t *self, uint8_t current_index);
/** Return the current active page mark index. */
uint8_t egui_view_page_indicator_get_current_index(egui_view_t *self);
/** Choose between simple dots and icon-font marks. */
void egui_view_page_indicator_set_mark_style(egui_view_t *self, egui_view_page_indicator_mark_style_t style);
/** Return the current page mark rendering style. */
egui_view_page_indicator_mark_style_t egui_view_page_indicator_get_mark_style(egui_view_t *self);
/** Set the icon array used when the mark style is `ICON`. */
void egui_view_page_indicator_set_icons(egui_view_t *self, const char *const *icons);
/** Return the borrowed icon array used by icon-style page marks. */
const char *const *egui_view_page_indicator_get_icons(egui_view_t *self);
/** Override the icon font used for icon-style page marks. */
void egui_view_page_indicator_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the icon font override, or NULL when automatic icon-font resolution is used. */
const egui_font_t *egui_view_page_indicator_get_icon_font(egui_view_t *self);
/** Default draw hook used by the page-indicator API table. */
void egui_view_page_indicator_on_draw(egui_view_t *self);
/** Initialize the page-indicator widget. */
void egui_view_page_indicator_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_PAGE_INDICATOR_H_ */
