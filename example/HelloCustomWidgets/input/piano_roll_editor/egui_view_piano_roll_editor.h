#ifndef _EGUI_VIEW_PIANO_ROLL_EDITOR_H_
#define _EGUI_VIEW_PIANO_ROLL_EDITOR_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_PIANO_ROLL_EDITOR_MAX_STATES 8

typedef struct egui_view_piano_roll_editor_state egui_view_piano_roll_editor_state_t;
struct egui_view_piano_roll_editor_state
{
    const char *clip;
    const char *mode;
    const char *summary;
    const char *footer;
    uint8_t focus_row;
    uint8_t play_col;
    uint8_t loop_start;
    uint8_t loop_width;
    uint8_t accent_mode;
};

typedef struct egui_view_piano_roll_editor egui_view_piano_roll_editor_t;
struct egui_view_piano_roll_editor
{
    egui_view_t base;
    const egui_view_piano_roll_editor_state_t *states;
    uint8_t state_count;
    uint8_t current_index;
    uint8_t last_zone;
};

void egui_view_piano_roll_editor_set_states(
        egui_view_t *self,
        const egui_view_piano_roll_editor_state_t *states,
        uint8_t state_count);
void egui_view_piano_roll_editor_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif
