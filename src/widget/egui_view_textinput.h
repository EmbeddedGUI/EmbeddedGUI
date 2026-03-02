#ifndef _EGUI_VIEW_TEXTINPUT_H_
#define _EGUI_VIEW_TEXTINPUT_H_

#include "egui_view.h"
#include "core/egui_theme.h"
#include "core/egui_timer.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*egui_view_textinput_on_text_changed_t)(egui_view_t *self, const char *text);
typedef void (*egui_view_textinput_on_submit_t)(egui_view_t *self, const char *text);

typedef struct egui_view_textinput egui_view_textinput_t;
struct egui_view_textinput
{
    egui_view_t base;

    char text[EGUI_CONFIG_TEXTINPUT_MAX_LENGTH + 1];
    uint8_t text_len;
    uint8_t cursor_pos;
    uint8_t max_length;

    uint8_t cursor_visible : 1;
    uint8_t reserved : 7;

    egui_timer_t cursor_timer;

    const egui_font_t *font;
    egui_color_t text_color;
    egui_alpha_t text_alpha;
    egui_color_t placeholder_color;
    egui_alpha_t placeholder_alpha;
    egui_color_t cursor_color;
    uint8_t align_type;

    const char *placeholder;
    egui_dim_t scroll_offset_x;

    egui_view_textinput_on_text_changed_t on_text_changed;
    egui_view_textinput_on_submit_t on_submit;
};

void egui_view_textinput_set_text(egui_view_t *self, const char *text);
const char *egui_view_textinput_get_text(egui_view_t *self);
void egui_view_textinput_clear(egui_view_t *self);
void egui_view_textinput_insert_char(egui_view_t *self, char c);
void egui_view_textinput_delete_char(egui_view_t *self);
void egui_view_textinput_delete_forward(egui_view_t *self);
void egui_view_textinput_set_cursor_pos(egui_view_t *self, uint8_t pos);
void egui_view_textinput_move_cursor_left(egui_view_t *self);
void egui_view_textinput_move_cursor_right(egui_view_t *self);
void egui_view_textinput_move_cursor_home(egui_view_t *self);
void egui_view_textinput_move_cursor_end(egui_view_t *self);
void egui_view_textinput_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_textinput_set_text_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
void egui_view_textinput_set_placeholder(egui_view_t *self, const char *placeholder);
void egui_view_textinput_set_placeholder_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
void egui_view_textinput_set_cursor_color(egui_view_t *self, egui_color_t color);
void egui_view_textinput_set_max_length(egui_view_t *self, uint8_t max_length);
void egui_view_textinput_set_on_text_changed(egui_view_t *self, egui_view_textinput_on_text_changed_t listener);
void egui_view_textinput_set_on_submit(egui_view_t *self, egui_view_textinput_on_submit_t listener);
void egui_view_textinput_on_draw(egui_view_t *self);
void egui_view_textinput_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TEXTINPUT_H_ */
