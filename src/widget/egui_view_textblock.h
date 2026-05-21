#ifndef _EGUI_VIEW_TEXTBLOCK_H_
#define _EGUI_VIEW_TEXTBLOCK_H_

#include "egui_view.h"
#include "font/egui_font.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#include "core/egui_timer.h"
#endif

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_textblock egui_view_textblock_t;
struct egui_view_textblock
{
    egui_view_t base;

    egui_dim_t line_space;
    egui_dim_t max_lines;
    egui_dim_t content_line_count;
    egui_dim_t layout_width;

    uint8_t align_type;
    uint8_t is_auto_height;
    uint8_t is_auto_wrap_enabled;
    uint8_t is_scroll_enabled;
    egui_alpha_t alpha;
    egui_color_t color;

    const egui_font_t *font;
    const char *text;

    // Scroll fields
    egui_dim_t scroll_offset_x;
    egui_dim_t scroll_offset_y;
    egui_dim_t content_width;
    egui_dim_t content_height;

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_dim_t touch_last_x;
    egui_dim_t touch_last_y;
    uint8_t is_touch_dragging;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    uint8_t is_scrollbar_enabled : 1;
    uint8_t is_scrollbar_v_dragging : 1;
    uint8_t is_scrollbar_h_dragging : 1;
    uint8_t reserved_sb : 5;
#endif

    // Border fields
    uint8_t is_border_enabled : 1;
    uint8_t reserved_border : 7;
    egui_dim_t border_radius;
    egui_color_t border_color;

    // Edit mode fields
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    uint8_t is_editable : 1;
    uint8_t cursor_visible : 1;
    uint8_t reserved_edit : 6;
    uint16_t cursor_pos;
    uint16_t text_len;
    uint16_t max_length;
    char edit_buf[EGUI_CONFIG_TEXTBLOCK_EDIT_MAX_LENGTH + 1];
    egui_timer_t cursor_timer;
    egui_color_t cursor_color;
#endif
};

// ============== TextBlock Params ==============
typedef struct egui_view_textblock_params egui_view_textblock_params_t;
struct egui_view_textblock_params
{
    egui_region_t region;
    uint8_t align_type;
    const char *text;
    const egui_font_t *font;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_dim_t line_space;
    egui_dim_t max_lines;
    uint8_t is_auto_wrap_enabled;
    uint8_t is_scroll_enabled;
};

#define EGUI_VIEW_TEXTBLOCK_PARAMS_INIT_COLOR(_name, _x, _y, _w, _h, _text, _font, _color, _alpha)                                                             \
    static const egui_view_textblock_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                                 \
                                                       .align_type = EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP,                                                         \
                                                       .is_auto_wrap_enabled = 1,                                                                              \
                                                       .is_scroll_enabled = 1,                                                                                 \
                                                       .text = (_text),                                                                                        \
                                                       .font = (const egui_font_t *)(_font),                                                                   \
                                                       .color = _color,                                                                                        \
                                                       .alpha = (_alpha),                                                                                      \
                                                       .line_space = 2,                                                                                        \
                                                       .max_lines = 0}

#if defined(_MSC_VER)
#define EGUI_VIEW_TEXTBLOCK_PARAMS_INIT(_name, _x, _y, _w, _h, _text, _font, _color, _alpha)                                                                   \
    EGUI_VIEW_TEXTBLOCK_PARAMS_INIT_COLOR(_name, _x, _y, _w, _h, _text, _font, _color##_INIT, _alpha)
#else
#define EGUI_VIEW_TEXTBLOCK_PARAMS_INIT(_name, _x, _y, _w, _h, _text, _font, _color, _alpha)                                                                   \
    EGUI_VIEW_TEXTBLOCK_PARAMS_INIT_COLOR(_name, _x, _y, _w, _h, _text, _font, (_color), _alpha)
#endif

#define EGUI_VIEW_TEXTBLOCK_PARAMS_INIT_SIMPLE(_name, _x, _y, _w, _h, _text)                                                                                   \
    static const egui_view_textblock_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                                 \
                                                       .align_type = EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP,                                                         \
                                                       .is_auto_wrap_enabled = 1,                                                                              \
                                                       .is_scroll_enabled = 1,                                                                                 \
                                                       .text = (_text),                                                                                        \
                                                       .font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT,                                                  \
                                                       .color = EGUI_COLOR_WHITE_INIT,                                                                         \
                                                       .alpha = EGUI_ALPHA_100,                                                                                \
                                                       .line_space = 2,                                                                                        \
                                                       .max_lines = 0}

/** Apply a textblock parameter block after initialization. */
void egui_view_textblock_apply_params(egui_view_t *self, const egui_view_textblock_params_t *params);
/** Initialize a textblock and immediately apply its parameter block. */
void egui_view_textblock_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_textblock_params_t *params);

/** Default draw hook used by the textblock API table. */
void egui_view_textblock_on_draw(egui_view_t *self);
/** Set extra spacing inserted between rendered lines. */
void egui_view_textblock_set_line_space(egui_view_t *self, egui_dim_t line_space);
/** Return extra spacing inserted between rendered lines. Returns 0 when self is NULL. */
egui_dim_t egui_view_textblock_get_line_space(egui_view_t *self);
/** Limit how many lines are rendered. Use 0 for no explicit line limit. */
void egui_view_textblock_set_max_lines(egui_view_t *self, egui_dim_t max_lines);
/** Return the configured maximum rendered line count. Returns 0 when self is NULL. */
egui_dim_t egui_view_textblock_get_max_lines(egui_view_t *self);
/** Resize the view height to fit measured text content automatically. */
void egui_view_textblock_set_auto_height(egui_view_t *self, uint8_t is_auto_height);
/** Return whether auto-height mode is enabled. Returns 0 when self is NULL. */
uint8_t egui_view_textblock_get_auto_height(egui_view_t *self);
/** Enable or disable automatic line wrapping to the work-region width. */
void egui_view_textblock_set_auto_wrap(egui_view_t *self, uint8_t enabled);
/** Return whether automatic line wrapping is enabled. Returns 0 when self is NULL. */
uint8_t egui_view_textblock_get_auto_wrap(egui_view_t *self);
/** Enable or disable drag scrolling when content is larger than the viewport. */
void egui_view_textblock_set_scroll_enabled(egui_view_t *self, uint8_t enabled);
/** Return whether drag scrolling is enabled. Returns 0 when self is NULL. */
uint8_t egui_view_textblock_get_scroll_enabled(egui_view_t *self);
/** Override the font used to measure and draw the text. */
void egui_view_textblock_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the configured text font, or NULL when unset or self is NULL. */
const egui_font_t *egui_view_textblock_get_font(egui_view_t *self);
/** Set the text color and alpha. */
void egui_view_textblock_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
/** Return the configured text color. Returns zero color when self is NULL. */
egui_color_t egui_view_textblock_get_font_color(egui_view_t *self);
/** Return the configured text alpha. Returns 0 when self is NULL. */
egui_alpha_t egui_view_textblock_get_font_alpha(egui_view_t *self);
/** Set horizontal and vertical alignment flags used when content does not need scrolling. */
void egui_view_textblock_set_align_type(egui_view_t *self, uint8_t align_type);
/** Return the stored horizontal and vertical alignment flags. Returns 0 when self is NULL. */
uint8_t egui_view_textblock_get_align_type(egui_view_t *self);
/** Replace the current text content and recompute layout metrics. */
void egui_view_textblock_set_text(egui_view_t *self, const char *text);
/** Return the current borrowed text pointer, or NULL when unset or self is NULL. */
const char *egui_view_textblock_get_text(egui_view_t *self);
/** Return the measured content width. Returns 0 when self is NULL. */
egui_dim_t egui_view_textblock_get_content_width(egui_view_t *self);
/** Return the measured content height. Returns 0 when self is NULL. */
egui_dim_t egui_view_textblock_get_content_height(egui_view_t *self);
/** Return the measured content line count. Returns 0 when self is NULL. */
egui_dim_t egui_view_textblock_get_content_line_count(egui_view_t *self);
/** Measure arbitrary text using the current font and wrapping settings. Returns 0 when no font is set. */
int egui_view_textblock_get_text_size(egui_view_t *self, const char *text, egui_dim_t max_width, egui_dim_t *width, egui_dim_t *height);
/** Initialize the multi-line textblock widget. */
void egui_view_textblock_init(egui_view_t *self, egui_core_t *core);

// Border API
/** Enable or disable the optional rounded border frame. */
void egui_view_textblock_set_border_enabled(egui_view_t *self, uint8_t enabled);
/** Return whether the optional rounded border frame is enabled. Returns 0 when self is NULL. */
uint8_t egui_view_textblock_get_border_enabled(egui_view_t *self);
/** Set the corner radius of the optional border frame. */
void egui_view_textblock_set_border_radius(egui_view_t *self, egui_dim_t radius);
/** Return the optional border frame corner radius. Returns 0 when self is NULL. */
egui_dim_t egui_view_textblock_get_border_radius(egui_view_t *self);
/** Set the color of the optional border frame. */
void egui_view_textblock_set_border_color(egui_view_t *self, egui_color_t color);
/** Return the optional border frame color. Returns zero color when self is NULL. */
egui_color_t egui_view_textblock_get_border_color(egui_view_t *self);

// Scrollbar API
/** Enable or disable built-in scrollbars when scrollbar support is compiled in. */
void egui_view_textblock_set_scrollbar_enabled(egui_view_t *self, uint8_t enabled);
/** Return whether built-in scrollbars are enabled. Returns 0 when self is NULL or scrollbar support is compiled out. */
uint8_t egui_view_textblock_get_scrollbar_enabled(egui_view_t *self);

// Edit mode API
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
/** Turn the textblock into an editable text area backed by an internal edit buffer. */
void egui_view_textblock_set_editable(egui_view_t *self, uint8_t is_editable);
/** Return whether editable mode is enabled. */
uint8_t egui_view_textblock_get_editable(egui_view_t *self);
/** Insert one single-byte character at the cursor. */
void egui_view_textblock_insert_char(egui_view_t *self, char c);
/** Delete the character before the cursor. */
void egui_view_textblock_delete_char(egui_view_t *self);
/** Move the cursor by byte index. Values past the end are clamped. */
void egui_view_textblock_set_cursor_pos(egui_view_t *self, uint16_t pos);
/** Return the current cursor byte index. Returns 0 when self is NULL. */
uint16_t egui_view_textblock_get_cursor_pos(egui_view_t *self);
/** Set the caret color used in editable mode. */
void egui_view_textblock_set_cursor_color(egui_view_t *self, egui_color_t color);
/** Return the caret color used in editable mode. Returns zero color when self is NULL. */
egui_color_t egui_view_textblock_get_cursor_color(egui_view_t *self);
/** Return the current internal edit buffer. */
const char *egui_view_textblock_get_edit_text(egui_view_t *self);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TEXTBLOCK_H_ */
