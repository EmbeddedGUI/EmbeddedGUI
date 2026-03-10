#ifndef _EGUI_VIEW_EQUALIZER_CURVE_EDITOR_H_
#define _EGUI_VIEW_EQUALIZER_CURVE_EDITOR_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_EQUALIZER_CURVE_EDITOR_MAX_BANDS 8

typedef struct egui_view_equalizer_curve_editor_band egui_view_equalizer_curve_editor_band_t;
struct egui_view_equalizer_curve_editor_band
{
    const char *preset;
    const char *status;
    const char *summary;
    const char *footer;
    int8_t gains[5];
    uint8_t focus_index;
    uint8_t accent_mode;
};

typedef struct egui_view_equalizer_curve_editor egui_view_equalizer_curve_editor_t;
struct egui_view_equalizer_curve_editor
{
    egui_view_t base;
    const egui_view_equalizer_curve_editor_band_t *bands;
    const egui_font_t *font;
    uint8_t band_count;
    uint8_t current_index;
    uint8_t last_zone;
};

void egui_view_equalizer_curve_editor_set_bands(
        egui_view_t *self,
        const egui_view_equalizer_curve_editor_band_t *bands,
        uint8_t band_count);
void egui_view_equalizer_curve_editor_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_equalizer_curve_editor_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif
