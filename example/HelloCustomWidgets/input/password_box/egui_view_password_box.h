#ifndef _EGUI_VIEW_PASSWORD_BOX_H_
#define _EGUI_VIEW_PASSWORD_BOX_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_PASSWORD_BOX_MAX_TEXT_LEN 31

#define EGUI_VIEW_PASSWORD_BOX_PART_FIELD  0
#define EGUI_VIEW_PASSWORD_BOX_PART_REVEAL 1
#define EGUI_VIEW_PASSWORD_BOX_PART_NONE   0xFF

typedef void (*egui_view_on_password_box_changed_listener_t)(egui_view_t *self, const char *text, uint8_t revealed, uint8_t part);

typedef struct egui_view_password_box egui_view_password_box_t;
struct egui_view_password_box
{
    egui_view_t base;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    const egui_font_t *icon_font;
    egui_view_on_password_box_changed_listener_t on_changed;
    const char *label;
    const char *helper;
    const char *placeholder;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    char text[EGUI_VIEW_PASSWORD_BOX_MAX_TEXT_LEN + 1];
    char masked_text[EGUI_VIEW_PASSWORD_BOX_MAX_TEXT_LEN + 1];
    uint8_t text_len;
    uint8_t cursor_pos;
    uint8_t current_part;
    uint8_t pressed_part;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t revealed;
    uint8_t cursor_visible;
    egui_dim_t scroll_offset_x;
    egui_timer_t cursor_timer;
};

void egui_view_password_box_init(egui_view_t *self);
void egui_view_password_box_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_password_box_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_password_box_set_icon_font(egui_view_t *self, const egui_font_t *font);
void egui_view_password_box_set_label(egui_view_t *self, const char *label);
void egui_view_password_box_set_helper(egui_view_t *self, const char *helper);
void egui_view_password_box_set_placeholder(egui_view_t *self, const char *placeholder);
void egui_view_password_box_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                        egui_color_t muted_text_color, egui_color_t accent_color);
void egui_view_password_box_set_text(egui_view_t *self, const char *text);
const char *egui_view_password_box_get_text(egui_view_t *self);
void egui_view_password_box_clear(egui_view_t *self);
void egui_view_password_box_insert_char(egui_view_t *self, char ch);
void egui_view_password_box_delete_char(egui_view_t *self);
void egui_view_password_box_delete_forward(egui_view_t *self);
void egui_view_password_box_move_cursor_left(egui_view_t *self);
void egui_view_password_box_move_cursor_right(egui_view_t *self);
void egui_view_password_box_move_cursor_home(egui_view_t *self);
void egui_view_password_box_move_cursor_end(egui_view_t *self);
void egui_view_password_box_set_cursor_pos(egui_view_t *self, uint8_t cursor_pos);
void egui_view_password_box_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_password_box_get_current_part(egui_view_t *self);
void egui_view_password_box_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_password_box_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_password_box_set_revealed(egui_view_t *self, uint8_t revealed);
uint8_t egui_view_password_box_get_revealed(egui_view_t *self);
void egui_view_password_box_set_on_changed_listener(egui_view_t *self, egui_view_on_password_box_changed_listener_t listener);
uint8_t egui_view_password_box_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region);
uint8_t egui_view_password_box_handle_navigation_key(egui_view_t *self, uint8_t key_code);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_PASSWORD_BOX_H_ */
