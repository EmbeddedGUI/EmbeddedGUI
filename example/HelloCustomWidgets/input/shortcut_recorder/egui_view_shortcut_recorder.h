#ifndef _EGUI_VIEW_SHORTCUT_RECORDER_H_
#define _EGUI_VIEW_SHORTCUT_RECORDER_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD  0
#define EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET 1
#define EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR  2
#define EGUI_VIEW_SHORTCUT_RECORDER_PART_NONE   0xFF

#define EGUI_VIEW_SHORTCUT_RECORDER_MAX_PRESETS 4

typedef struct egui_view_shortcut_recorder_preset egui_view_shortcut_recorder_preset_t;
struct egui_view_shortcut_recorder_preset
{
    const char *label;
    const char *meta;
    uint8_t key_code;
    uint8_t is_shift;
    uint8_t is_ctrl;
};

typedef void (*egui_view_on_shortcut_recorder_changed_listener_t)(egui_view_t *self, uint8_t part, uint8_t preset_index);

typedef struct egui_view_shortcut_recorder egui_view_shortcut_recorder_t;
struct egui_view_shortcut_recorder
{
    egui_view_t base;
    egui_view_on_shortcut_recorder_changed_listener_t on_changed;
    const egui_view_shortcut_recorder_preset_t *presets;
    const egui_font_t *font;
    const egui_font_t *meta_font;
    const char *title;
    const char *helper;
    const char *footer_prefix;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t listening_color;
    egui_color_t preview_color;
    egui_color_t danger_color;
    uint8_t preset_count;
    uint8_t current_part;
    uint8_t current_preset;
    uint8_t compact_mode;
    uint8_t read_only_mode;
    uint8_t listening;
    uint8_t has_binding;
    uint8_t binding_key_code;
    uint8_t binding_is_shift;
    uint8_t binding_is_ctrl;
    uint8_t pressed_part;
    uint8_t pressed_preset;
};

void egui_view_shortcut_recorder_init(egui_view_t *self);
void egui_view_shortcut_recorder_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_shortcut_recorder_set_meta_font(egui_view_t *self, const egui_font_t *font);
void egui_view_shortcut_recorder_set_header(egui_view_t *self, const char *title, const char *helper, const char *footer_prefix);
void egui_view_shortcut_recorder_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                             egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t listening_color, egui_color_t preview_color,
                                             egui_color_t danger_color);
void egui_view_shortcut_recorder_set_presets(egui_view_t *self, const egui_view_shortcut_recorder_preset_t *presets, uint8_t preset_count);
void egui_view_shortcut_recorder_set_binding(egui_view_t *self, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl);
void egui_view_shortcut_recorder_commit_binding(egui_view_t *self, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl);
void egui_view_shortcut_recorder_clear_binding(egui_view_t *self);
uint8_t egui_view_shortcut_recorder_has_binding(egui_view_t *self);
uint8_t egui_view_shortcut_recorder_get_key_code(egui_view_t *self);
uint8_t egui_view_shortcut_recorder_get_is_shift(egui_view_t *self);
uint8_t egui_view_shortcut_recorder_get_is_ctrl(egui_view_t *self);
void egui_view_shortcut_recorder_get_binding_text(egui_view_t *self, char *buffer, int size);
void egui_view_shortcut_recorder_set_listening(egui_view_t *self, uint8_t listening);
uint8_t egui_view_shortcut_recorder_is_listening(egui_view_t *self);
void egui_view_shortcut_recorder_apply_preset(egui_view_t *self, uint8_t preset_index);
void egui_view_shortcut_recorder_set_current_part(egui_view_t *self, uint8_t part);
uint8_t egui_view_shortcut_recorder_get_current_part(egui_view_t *self);
void egui_view_shortcut_recorder_set_current_preset(egui_view_t *self, uint8_t preset_index);
uint8_t egui_view_shortcut_recorder_get_current_preset(egui_view_t *self);
void egui_view_shortcut_recorder_set_compact_mode(egui_view_t *self, uint8_t compact_mode);
void egui_view_shortcut_recorder_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode);
void egui_view_shortcut_recorder_set_on_changed_listener(egui_view_t *self, egui_view_on_shortcut_recorder_changed_listener_t listener);
uint8_t egui_view_shortcut_recorder_get_part_region(egui_view_t *self, uint8_t part, uint8_t preset_index, egui_region_t *region);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_SHORTCUT_RECORDER_H_ */
