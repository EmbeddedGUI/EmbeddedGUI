#ifndef _EGUI_VIEW_WINDOW_H_
#define _EGUI_VIEW_WINDOW_H_

#include "egui_view_group.h"
#include "egui_view_label.h"

/**
 * @brief Framed window container with a title bar, optional close control, and child content area.
 *
 * The widget is implemented as a group that internally owns three children:
 * a centered title label, a clickable close label, and a content group placed
 * below the header. Users normally add application widgets through the content
 * group so window chrome and client content stay separated.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Listener fired when the close control is clicked. Without a listener, the window simply marks itself gone. */
typedef void (*egui_view_window_close_cb_t)(egui_view_t *self);

typedef struct egui_view_window egui_view_window_t;
struct egui_view_window
{
    /* Outer grouped widget that draws window chrome and forwards events. */
    egui_view_group_t base;

    /* Centered title label shown inside the header bar. */
    egui_view_label_t title_label;
    /* Right-aligned close glyph that behaves like a button. */
    egui_view_label_t close_label;
    /* Inner container where user content should be attached. */
    egui_view_group_t content;
    /* Height of the title bar that stays above the content area. */
    egui_dim_t header_height;
    /* Background color used for the title bar. */
    egui_color_t header_color;
    /* Fill color used for the client area below the header. */
    egui_color_t content_bg_color;
    /* Borrowed glyph string shown by the close label. */
    const char *close_icon;
    /* Optional override font for the close glyph. */
    const egui_font_t *close_icon_font;
    /* Callback fired instead of the default hide-on-close behavior. */
    egui_view_window_close_cb_t on_close;
};

// ============== Window Params ==============
typedef struct egui_view_window_params egui_view_window_params_t;
struct egui_view_window_params
{
    /* Outer region occupied by the whole window. */
    egui_region_t region;
    /* Header height used for title-bar layout. */
    egui_dim_t header_height;
    /* Borrowed title text shown in the header label. */
    const char *title;
};

/** Build a window parameter block with region, header height, and borrowed title text. */
#define EGUI_VIEW_WINDOW_PARAMS_INIT(_name, _x, _y, _w, _h, _hdr_h, _title)                                                                                    \
    static const egui_view_window_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .header_height = (_hdr_h), .title = (_title)}

/** Apply region, header height, and title from one parameter block. */
void egui_view_window_apply_params(egui_view_t *self, const egui_view_window_params_t *params);
/** Initialize a window widget and immediately apply its parameter block. */
void egui_view_window_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_window_params_t *params);

/** Set the title string shown in the header. The title pointer is borrowed through the internal label. */
void egui_view_window_set_title(egui_view_t *self, const char *title);
/** Return the borrowed title string shown in the header. */
const char *egui_view_window_get_title(egui_view_t *self);
/** Set the header height and relayout the title, close control, and content area. */
void egui_view_window_set_header_height(egui_view_t *self, egui_dim_t height);
/** Return the header height in pixels. */
egui_dim_t egui_view_window_get_header_height(egui_view_t *self);
/** Set the title-bar background color. */
void egui_view_window_set_header_color(egui_view_t *self, egui_color_t color);
/** Return the title-bar background color. */
egui_color_t egui_view_window_get_header_color(egui_view_t *self);
/** Set the client-area background color. */
void egui_view_window_set_content_bg_color(egui_view_t *self, egui_color_t color);
/** Return the client-area background color. */
egui_color_t egui_view_window_get_content_bg_color(egui_view_t *self);
/** Set the close glyph string. Passing NULL or an empty string hides the close control even if a font is available. */
void egui_view_window_set_close_icon(egui_view_t *self, const char *icon);
/** Return the borrowed close glyph string. */
const char *egui_view_window_get_close_icon(egui_view_t *self);
/** Override the font used for the close glyph. Passing NULL restores the auto-selected icon font. A close icon is shown only when both font and text exist. */
void egui_view_window_set_close_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the close glyph font override, or NULL when automatic icon-font resolution is used. */
const egui_font_t *egui_view_window_get_close_icon_font(egui_view_t *self);
/** Return the internal content group where application children are attached. */
egui_view_t *egui_view_window_get_content(egui_view_t *self);
/** Add one child to the internal content group below the header instead of attaching it directly to the outer window. */
void egui_view_window_add_content(egui_view_t *self, egui_view_t *child);
/** Register the callback fired when the close control is clicked. */
void egui_view_window_set_on_close(egui_view_t *self, egui_view_window_close_cb_t callback);
/** Return the callback fired when the close control is clicked. */
egui_view_window_close_cb_t egui_view_window_get_on_close(egui_view_t *self);
/** Draw the window chrome, including header background, close hotspot feedback, and content background. */
void egui_view_window_on_draw(egui_view_t *self);
/** Initialize the grouped window widget with title label, close control, and content container. */
void egui_view_window_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_WINDOW_H_ */
