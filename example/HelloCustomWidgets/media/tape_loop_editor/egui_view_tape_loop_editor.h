#ifndef _EGUI_VIEW_TAPE_LOOP_EDITOR_H_
#define _EGUI_VIEW_TAPE_LOOP_EDITOR_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TAPE_LOOP_EDITOR_MAX_STATES 8

typedef struct egui_view_tape_loop_editor_state egui_view_tape_loop_editor_state_t;
struct egui_view_tape_loop_editor_state
{
    const char *preset;
    const char *status;
    const char *summary;
    const char *footer;
    uint8_t focus_zone;
    uint8_t accent_mode;
};

typedef struct egui_view_tape_loop_editor egui_view_tape_loop_editor_t;
struct egui_view_tape_loop_editor
{
    egui_view_t base;
    const egui_view_tape_loop_editor_state_t *states;
    const egui_font_t *font;
    uint8_t state_count;
    uint8_t current_index;
    uint8_t last_zone;
};

void egui_view_tape_loop_editor_set_states(
        egui_view_t *self,
        const egui_view_tape_loop_editor_state_t *states,
        uint8_t state_count);
void egui_view_tape_loop_editor_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_tape_loop_editor_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif
