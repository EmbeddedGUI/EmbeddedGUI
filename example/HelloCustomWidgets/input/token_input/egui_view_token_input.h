#ifndef _EGUI_VIEW_TOKEN_INPUT_H_
#define _EGUI_VIEW_TOKEN_INPUT_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS    8
#define EGUI_VIEW_TOKEN_INPUT_MAX_TOKEN_LEN 23
#define EGUI_VIEW_TOKEN_INPUT_MAX_DRAFT_LEN 23

#define EGUI_VIEW_TOKEN_INPUT_PART_INPUT 0xFE
#define EGUI_VIEW_TOKEN_INPUT_PART_NONE  0xFF

typedef void (*egui_view_on_token_input_changed_listener_t)(egui_view_t *self, uint8_t token_count, uint8_t part);

typedef struct egui_view_token_input egui_view_token_input_t;
struct egui_view_token_input
{
    egui_view_t base;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    egui_view_on_token_input_changed_listener_t on_changed;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t shadow_color;
    char placeholder[EGUI_VIEW_TOKEN_INPUT_MAX_TOKEN_LEN + 1];
    char draft_text[EGUI_VIEW_TOKEN_INPUT_MAX_DRAFT_LEN + 1];
    char tokens[EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS][EGUI_VIEW_TOKEN_INPUT_MAX_TOKEN_LEN + 1];
    uint8_t token_count;
    uint8_t draft_len;
    uint8_t current_part;
    uint8_t pressed_part;
    uint8_t pressed_remove;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t restore_input_focus;
};

void egui_view_token_input_init(egui_view_t *self);
void egui_view_token_input_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_token_input_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_token_input_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t shadow_color);
void egui_view_token_input_set_placeholder(egui_view_t *self, const char *placeholder);
void egui_view_token_input_set_tokens(egui_view_t *self, const char **tokens, uint8_t count);
uint8_t egui_view_token_input_add_token(egui_view_t *self, const char *text);
uint8_t egui_view_token_input_remove_token(egui_view_t *self, uint8_t index);
uint8_t egui_view_token_input_get_token_count(egui_view_t *self);
const char *egui_view_token_input_get_token(egui_view_t *self, uint8_t index);
const char *egui_view_token_input_get_draft_text(egui_view_t *self);
void egui_view_token_input_clear_draft(egui_view_t *self);
void egui_view_token_input_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_token_input_get_current_part(egui_view_t *self);
void egui_view_token_input_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_token_input_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_token_input_set_on_changed_listener(egui_view_t *self, egui_view_on_token_input_changed_listener_t listener);
uint8_t egui_view_token_input_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region);
uint8_t egui_view_token_input_get_remove_region(egui_view_t *self, uint8_t index, egui_region_t *region);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TOKEN_INPUT_H_ */
