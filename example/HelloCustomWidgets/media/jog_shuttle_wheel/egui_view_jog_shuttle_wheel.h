#ifndef _EGUI_VIEW_JOG_SHUTTLE_WHEEL_H_
#define _EGUI_VIEW_JOG_SHUTTLE_WHEEL_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_JOG_SHUTTLE_WHEEL_MAX_MODES 8

typedef struct egui_view_jog_shuttle_wheel_mode egui_view_jog_shuttle_wheel_mode_t;
struct egui_view_jog_shuttle_wheel_mode
{
    const char *preset;
    const char *status;
    const char *summary;
    const char *footer;
    uint8_t ring_index;
    uint8_t accent_mode;
};

typedef struct egui_view_jog_shuttle_wheel egui_view_jog_shuttle_wheel_t;
struct egui_view_jog_shuttle_wheel
{
    egui_view_t base;
    const egui_view_jog_shuttle_wheel_mode_t *modes;
    const egui_font_t *font;
    uint8_t mode_count;
    uint8_t current_index;
    uint8_t last_zone;
};

void egui_view_jog_shuttle_wheel_set_modes(egui_view_t *self, const egui_view_jog_shuttle_wheel_mode_t *modes, uint8_t mode_count);
void egui_view_jog_shuttle_wheel_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_jog_shuttle_wheel_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif
