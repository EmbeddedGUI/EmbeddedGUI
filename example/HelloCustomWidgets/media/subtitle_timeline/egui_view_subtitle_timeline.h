#ifndef _EGUI_VIEW_SUBTITLE_TIMELINE_H_
#define _EGUI_VIEW_SUBTITLE_TIMELINE_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_SUBTITLE_TIMELINE_MAX_CUES 8

typedef struct egui_view_subtitle_timeline_cue egui_view_subtitle_timeline_cue_t;
struct egui_view_subtitle_timeline_cue
{
    const char *speaker;
    const char *status;
    const char *line;
    const char *footer;
    uint8_t emphasis;
    uint8_t active_index;
};

typedef struct egui_view_subtitle_timeline egui_view_subtitle_timeline_t;
struct egui_view_subtitle_timeline
{
    egui_view_t base;
    const egui_view_subtitle_timeline_cue_t *cues;
    const egui_font_t *font;
    uint8_t cue_count;
    uint8_t current_index;
    uint8_t last_zone;
};

void egui_view_subtitle_timeline_set_cues(
        egui_view_t *self,
        const egui_view_subtitle_timeline_cue_t *cues,
        uint8_t cue_count);
void egui_view_subtitle_timeline_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_subtitle_timeline_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif
