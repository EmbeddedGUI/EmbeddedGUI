#ifndef _EGUI_VIEW_HEART_RATE_H_
#define _EGUI_VIEW_HEART_RATE_H_

#include "egui_view.h"
#include "core/egui_timer.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_heart_rate egui_view_heart_rate_t;
struct egui_view_heart_rate
{
    egui_view_t base;

    uint8_t bpm;
    uint8_t animate;
    egui_color_t heart_color;
    egui_color_t text_color;
    const egui_font_t *font;
    char text_buffer[8];

    // Internal animation state
    egui_timer_t anim_timer;
    uint8_t ecg_offset; // 0-31: current scroll position in ECG template
    uint8_t beat_phase; // 0-15: heart scale animation (>0 means heart is enlarged)
};

// ============== Heart Rate Params ==============
typedef struct egui_view_heart_rate_params egui_view_heart_rate_params_t;
struct egui_view_heart_rate_params
{
    egui_region_t region;
    uint8_t bpm;
};

#define EGUI_VIEW_HEART_RATE_PARAMS_INIT(_name, _x, _y, _w, _h, _bpm)                                                                                          \
    static const egui_view_heart_rate_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .bpm = (_bpm)}

/** Apply a heart rate parameter block after initialization. */
void egui_view_heart_rate_apply_params(egui_view_t *self, const egui_view_heart_rate_params_t *params);
/** Initialize a heart rate view and immediately apply its parameter block. */
void egui_view_heart_rate_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_heart_rate_params_t *params);

/** Set beats per minute. A value of 0 also stops the timer-driven pulse animation. */
void egui_view_heart_rate_set_bpm(egui_view_t *self, uint8_t bpm);
/** Return the stored beats-per-minute value. */
uint8_t egui_view_heart_rate_get_bpm(egui_view_t *self);
/** Enable or disable the internal ECG and heart pulse animation timer. */
void egui_view_heart_rate_set_animate(egui_view_t *self, uint8_t enable);
/** Return whether the internal ECG and heart pulse animation timer is enabled. */
uint8_t egui_view_heart_rate_get_animate(egui_view_t *self);
/** Set the color shared by the heart icon and ECG waveform. */
void egui_view_heart_rate_set_heart_color(egui_view_t *self, egui_color_t color);
/** Return the color shared by the heart icon and ECG waveform. */
egui_color_t egui_view_heart_rate_get_heart_color(egui_view_t *self);
/** Set the initial ECG scroll phase for deterministic previews or synchronized demos. */
void egui_view_heart_rate_set_pulse_phase(egui_view_t *self, uint8_t phase);
/** Return the current ECG template offset in the 0-31 range. */
uint8_t egui_view_heart_rate_get_ecg_offset(egui_view_t *self);
/** Default draw hook used by the heart rate API table. */
void egui_view_heart_rate_on_draw(egui_view_t *self);
/** Initialize a heart rate indicator with animation disabled by default. */
void egui_view_heart_rate_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_HEART_RATE_H_ */
