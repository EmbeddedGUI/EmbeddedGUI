#ifndef _EGUI_VIEW_SCENE_CROSSFADER_H_
#define _EGUI_VIEW_SCENE_CROSSFADER_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SCENE_CROSSFADER_MAX_STATES 8

typedef struct egui_view_scene_crossfader_state egui_view_scene_crossfader_state_t;
struct egui_view_scene_crossfader_state
{
    const char *preset;
    const char *mode;
    const char *summary;
    const char *footer;
    const char *left_tag;
    const char *right_tag;
    uint8_t mix_percent;
    uint8_t left_level;
    uint8_t right_level;
    uint8_t curve_bias;
    uint8_t accent_mode;
};

typedef struct egui_view_scene_crossfader egui_view_scene_crossfader_t;
struct egui_view_scene_crossfader
{
    egui_view_t base;
    const egui_view_scene_crossfader_state_t *states;
    uint8_t state_count;
    uint8_t current_index;
    uint8_t last_zone;
};

void egui_view_scene_crossfader_set_states(egui_view_t *self, const egui_view_scene_crossfader_state_t *states, uint8_t state_count);
void egui_view_scene_crossfader_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif
