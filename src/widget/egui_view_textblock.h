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

    uint8_t align_type;
    uint8_t is_auto_height;
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
};

#define EGUI_VIEW_TEXTBLOCK_PARAMS_INIT(_name, _x, _y, _w, _h, _text, _font, _color, _alpha)                                                                   \
    static const egui_view_textblock_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                                 \
                                                       .align_type = EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP,                                                         \
                                                       .text = (_text),                                                                                        \
                                                       .font = (const egui_font_t *)(_font),                                                                   \
                                                       .color = (_color),                                                                                      \
                                                       .alpha = (_alpha),                                                                                      \
                                                       .line_space = 2,                                                                                        \
                                                       .max_lines = 0}

#define EGUI_VIEW_TEXTBLOCK_PARAMS_INIT_SIMPLE(_name, _x, _y, _w, _h, _text)                                                                                   \
    static const egui_view_textblock_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                                 \
                                                       .align_type = EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP,                                                         \
                                                       .text = (_text),                                                                                        \
                                                       .font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT,                                                  \
                                                       .color = EGUI_COLOR_WHITE,                                                                              \
                                                       .alpha = EGUI_ALPHA_100,                                                                                \
                                                       .line_space = 2,                                                                                        \
                                                       .max_lines = 0}

void egui_view_textblock_apply_params(egui_view_t *self, const egui_view_textblock_params_t *params);
void egui_view_textblock_init_with_params(egui_view_t *self, const egui_view_textblock_params_t *params);

void egui_view_textblock_on_draw(egui_view_t *self);
void egui_view_textblock_set_line_space(egui_view_t *self, egui_dim_t line_space);
void egui_view_textblock_set_max_lines(egui_view_t *self, egui_dim_t max_lines);
void egui_view_textblock_set_auto_height(egui_view_t *self, uint8_t is_auto_height);
void egui_view_textblock_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_textblock_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
void egui_view_textblock_set_align_type(egui_view_t *self, uint8_t align_type);
void egui_view_textblock_set_text(egui_view_t *self, const char *text);
int egui_view_textblock_get_text_size(egui_view_t *self, const char *text, egui_dim_t max_width, egui_dim_t *width, egui_dim_t *height);
void egui_view_textblock_init(egui_view_t *self);

// Border API
void egui_view_textblock_set_border_enabled(egui_view_t *self, uint8_t enabled);
void egui_view_textblock_set_border_radius(egui_view_t *self, egui_dim_t radius);
void egui_view_textblock_set_border_color(egui_view_t *self, egui_color_t color);

// Scrollbar API
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
void egui_view_textblock_set_scrollbar_enabled(egui_view_t *self, uint8_t enabled);
#endif

// Edit mode API
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
void egui_view_textblock_set_editable(egui_view_t *self, uint8_t is_editable);
uint8_t egui_view_textblock_get_editable(egui_view_t *self);
void egui_view_textblock_insert_char(egui_view_t *self, char c);
void egui_view_textblock_delete_char(egui_view_t *self);
void egui_view_textblock_set_cursor_pos(egui_view_t *self, uint16_t pos);
void egui_view_textblock_set_cursor_color(egui_view_t *self, egui_color_t color);
const char *egui_view_textblock_get_edit_text(egui_view_t *self);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TEXTBLOCK_H_ */
