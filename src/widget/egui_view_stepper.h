#ifndef _EGUI_VIEW_STEPPER_H_
#define _EGUI_VIEW_STEPPER_H_

#include "egui_view_page_indicator.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_stepper egui_view_stepper_t;

typedef enum
{
    /** Reuse the simple page-indicator dot drawing. */
    EGUI_VIEW_STEPPER_MARK_STYLE_DOT = 0,
    /** Draw connectors and completed-step badges with one icon glyph. */
    EGUI_VIEW_STEPPER_MARK_STYLE_ICON = 1,
} egui_view_stepper_mark_style_t;

/**
 * @brief Progress step indicator built on top of `egui_view_page_indicator_t`.
 *
 * The embedded `indicator` keeps the shared count, current index, and colors.
 * Stepper-specific fields only extend how completed steps are rendered.
 */
struct egui_view_stepper
{
    egui_view_page_indicator_t indicator;
    uint8_t mark_style;
    const char *completed_icon;
    const egui_font_t *icon_font;
};

typedef struct egui_view_stepper_params egui_view_stepper_params_t;
/**
 * @brief Construction-time parameter block for one stepper widget.
 */
struct egui_view_stepper_params
{
    egui_region_t region;
    uint8_t total_steps;
    uint8_t current_step;
};

/** Build a stepper parameter block with region and initial step state. */
#define EGUI_VIEW_STEPPER_PARAMS_INIT(_name, _x, _y, _w, _h, _total, _current)                                                                                 \
    static const egui_view_stepper_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .total_steps = (_total), .current_step = (_current)}

/** Apply a stepper parameter block after initialization. */
void egui_view_stepper_apply_params(egui_view_t *self, const egui_view_stepper_params_t *params);
/** Initialize a stepper and immediately apply its parameter block. */
void egui_view_stepper_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_stepper_params_t *params);

/** Set the total step count. The count is clamped to at least 1. */
void egui_view_stepper_set_total_steps(egui_view_t *self, uint8_t total_steps);
/** Return the total number of steps currently shown. */
uint8_t egui_view_stepper_get_total_steps(egui_view_t *self);
/** Set the current active step. Values past the end are clamped. */
void egui_view_stepper_set_current_step(egui_view_t *self, uint8_t current_step);
/** Return the current active step index. */
uint8_t egui_view_stepper_get_current_step(egui_view_t *self);
/** Choose between plain dots and icon-enhanced completed-step marks. */
void egui_view_stepper_set_mark_style(egui_view_t *self, egui_view_stepper_mark_style_t style);
/** Return the current completed-step mark rendering style. */
egui_view_stepper_mark_style_t egui_view_stepper_get_mark_style(egui_view_t *self);
/** Set the icon glyph drawn for completed steps when using icon mark style. */
void egui_view_stepper_set_completed_icon(egui_view_t *self, const char *icon);
/** Return the borrowed icon glyph drawn for completed steps. */
const char *egui_view_stepper_get_completed_icon(egui_view_t *self);
/** Override the icon font used for completed-step marks. */
void egui_view_stepper_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the icon font override, or NULL when automatic icon-font resolution is used. */
const egui_font_t *egui_view_stepper_get_icon_font(egui_view_t *self);
/** Initialize the page-indicator-based stepper widget. */
void egui_view_stepper_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_STEPPER_H_ */
