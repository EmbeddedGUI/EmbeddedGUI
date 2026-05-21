#ifndef _EGUI_VIEW_TAB_BAR_H_
#define _EGUI_VIEW_TAB_BAR_H_

#include "egui_view.h"
#include "font/egui_font.h"

/**
 * @brief Horizontal tab selector that divides its width into equal segments.
 *
 * Each tab can show text only or an icon plus text. The widget stores only
 * borrowed tab metadata and the currently selected index, leaving page content
 * switching to caller-provided listeners.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TAB_BAR_PRESSED_NONE 0xFF

/** Listener fired when the active tab changes. */
typedef void (*egui_view_on_tab_changed_listener_t)(egui_view_t *self, uint8_t index);

typedef struct egui_view_tab_bar egui_view_tab_bar_t;
struct egui_view_tab_bar
{
    egui_view_t base;

    /* Listener fired after the selected tab actually changes. */
    egui_view_on_tab_changed_listener_t on_tab_changed;
    /* Borrowed tab label array. */
    const char **tab_texts;
    /* Optional borrowed icon array parallel to `tab_texts`. */
    const char **tab_icons;
    /* Number of visible tabs. */
    uint8_t tab_count;
    /* Currently selected tab index. */
    uint8_t current_index;
    /* Tab index captured by the current pointer gesture. */
    uint8_t pressed_index;
    /* Shared alpha used for labels, icons, and indicator. */
    egui_alpha_t alpha;
    /* Text color for inactive tabs. */
    egui_color_t text_color;
    /* Text color for the active tab. */
    egui_color_t active_text_color;
    /* Color of the underline indicator. */
    egui_color_t indicator_color;
    /* Font used for tab labels. */
    const egui_font_t *font;
    /* Optional icon font override for tab icons. */
    const egui_font_t *icon_font;
    /* Vertical spacing between icon and text in mixed tabs. */
    egui_dim_t icon_text_gap;
};

// ============== TabBar Params ==============
typedef struct egui_view_tab_bar_params egui_view_tab_bar_params_t;
struct egui_view_tab_bar_params
{
    egui_region_t region;
    const char **tab_texts;
    uint8_t tab_count;
};

/** Build a tab-bar parameter block with region and borrowed tab labels. */
#define EGUI_VIEW_TAB_BAR_PARAMS_INIT(_name, _x, _y, _w, _h, _texts, _count)                                                                                   \
    static const egui_view_tab_bar_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .tab_texts = (_texts), .tab_count = (_count)}

/** Apply a tab-bar parameter block after initialization. */
void egui_view_tab_bar_apply_params(egui_view_t *self, const egui_view_tab_bar_params_t *params);
/** Initialize a tab bar and immediately apply its parameter block. */
void egui_view_tab_bar_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_tab_bar_params_t *params);

/** Replace the tab text array and tab count. */
void egui_view_tab_bar_set_tabs(egui_view_t *self, const char **tab_texts, uint8_t tab_count);
/** Return the borrowed tab text array. */
const char **egui_view_tab_bar_get_tabs(egui_view_t *self);
/** Select the active tab. This fires the change listener on real changes. */
void egui_view_tab_bar_set_current_index(egui_view_t *self, uint8_t index);
/** Return the index of the currently selected tab. */
uint8_t egui_view_tab_bar_get_current_index(egui_view_t *self);
/** Return the number of tabs in the bar. */
uint8_t egui_view_tab_bar_get_tab_count(egui_view_t *self);
/** Return the text color used for inactive tabs. */
egui_color_t egui_view_tab_bar_get_text_color(egui_view_t *self);
/** Return the text color used for the active tab. */
egui_color_t egui_view_tab_bar_get_active_text_color(egui_view_t *self);
/** Return the underline indicator color. */
egui_color_t egui_view_tab_bar_get_indicator_color(egui_view_t *self);
/** Return the shared alpha value. */
egui_alpha_t egui_view_tab_bar_get_alpha(egui_view_t *self);
/** Return the vertical gap between icon and text in mixed tabs. */
egui_dim_t egui_view_tab_bar_get_icon_text_gap(egui_view_t *self);
/** Register the callback fired when the active tab changes. */
void egui_view_tab_bar_set_on_tab_changed_listener(egui_view_t *self, egui_view_on_tab_changed_listener_t listener);
/** Return the callback fired when the active tab changes. */
egui_view_on_tab_changed_listener_t egui_view_tab_bar_get_on_tab_changed_listener(egui_view_t *self);
/** Override the font used for tab labels. */
void egui_view_tab_bar_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the font used for tab labels. */
const egui_font_t *egui_view_tab_bar_get_font(egui_view_t *self);
/** Set the optional icon array that parallels the tab text array. */
void egui_view_tab_bar_set_tab_icons(egui_view_t *self, const char **tab_icons);
/** Return the optional borrowed icon array. */
const char **egui_view_tab_bar_get_tab_icons(egui_view_t *self);
/** Override the icon font used when tabs show icons. */
void egui_view_tab_bar_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the explicit icon font override, or NULL when auto-selection is used. */
const egui_font_t *egui_view_tab_bar_get_icon_font(egui_view_t *self);
/** Set the vertical gap between the icon and text in mixed icon-text tabs. */
void egui_view_tab_bar_set_icon_text_gap(egui_view_t *self, egui_dim_t gap);
/** Default draw hook used by the tab-bar API table. */
void egui_view_tab_bar_on_draw(egui_view_t *self);
/** Initialize the clickable tab-bar widget. */
void egui_view_tab_bar_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TAB_BAR_H_ */
