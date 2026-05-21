#ifndef _EGUI_VIEW_LYRIC_SCROLLER_H_
#define _EGUI_VIEW_LYRIC_SCROLLER_H_

#include "core/egui_timer.h"
#include "egui_view_group.h"
#include "egui_view_label.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_lyric_scroller egui_view_lyric_scroller_t;
/** Clipping group that hosts one label child and scrolls it horizontally when the text overflows. */
struct egui_view_lyric_scroller
{
    egui_view_group_t base;

    egui_view_label_t label;   /* Internal label child that renders the text. */
    egui_timer_t scroll_timer; /* Timer that advances the horizontal offset. */

    egui_dim_t text_width;        /* Measured width of the current text. */
    egui_dim_t text_height;       /* Measured height of the current text. */
    egui_dim_t scroll_offset_x;   /* Current horizontal scroll offset applied to the label. */
    egui_dim_t max_scroll_offset; /* Furthest offset before the text reaches the right-end pause point. */
    egui_dim_t scroll_step;       /* Pixels moved per timer tick. */

    uint16_t interval_ms;           /* Timer period between scroll steps. */
    uint16_t pause_duration_ms;     /* Pause length at each edge, in milliseconds. */
    uint16_t pause_ticks_remaining; /* Remaining pause time expressed in timer ticks. */

    int8_t scroll_direction; /* 1 = moving toward the overflow end, -1 = returning toward the start. */
    uint8_t is_attached;     /* Non-zero while the widget is attached to a window/core. */
};

typedef struct egui_view_lyric_scroller_params egui_view_lyric_scroller_params_t;
/** Construction-time parameters for a lyric scroller. */
struct egui_view_lyric_scroller_params
{
    egui_region_t region;
    const char *text;
    const egui_font_t *font;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_dim_t scroll_step;
    uint16_t interval_ms;
    uint16_t pause_duration_ms;
};

#define EGUI_VIEW_LYRIC_SCROLLER_PARAMS_INIT_COLOR(_name, _x, _y, _w, _h, _text, _font, _color, _alpha)                                                        \
    static const egui_view_lyric_scroller_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                            \
                                                            .text = (_text),                                                                                   \
                                                            .font = (const egui_font_t *)(_font),                                                              \
                                                            .color = _color,                                                                                   \
                                                            .alpha = (_alpha),                                                                                 \
                                                            .scroll_step = 1,                                                                                  \
                                                            .interval_ms = 50,                                                                                 \
                                                            .pause_duration_ms = 400}

#if defined(_MSC_VER)
#define EGUI_VIEW_LYRIC_SCROLLER_PARAMS_INIT(_name, _x, _y, _w, _h, _text, _font, _color, _alpha)                                                              \
    EGUI_VIEW_LYRIC_SCROLLER_PARAMS_INIT_COLOR(_name, _x, _y, _w, _h, _text, _font, _color##_INIT, _alpha)
#else
#define EGUI_VIEW_LYRIC_SCROLLER_PARAMS_INIT(_name, _x, _y, _w, _h, _text, _font, _color, _alpha)                                                              \
    EGUI_VIEW_LYRIC_SCROLLER_PARAMS_INIT_COLOR(_name, _x, _y, _w, _h, _text, _font, (_color), _alpha)
#endif

#define EGUI_VIEW_LYRIC_SCROLLER_PARAMS_INIT_SIMPLE(_name, _x, _y, _w, _h, _text)                                                                              \
    static const egui_view_lyric_scroller_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                            \
                                                            .text = (_text),                                                                                   \
                                                            .font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT,                                             \
                                                            .color = EGUI_THEME_TEXT_PRIMARY_INIT,                                                             \
                                                            .alpha = EGUI_ALPHA_100,                                                                           \
                                                            .scroll_step = 1,                                                                                  \
                                                            .interval_ms = 50,                                                                                 \
                                                            .pause_duration_ms = 400}

/** Apply region, text, font, and scrolling parameters from one parameter block. */
void egui_view_lyric_scroller_apply_params(egui_view_t *self, const egui_view_lyric_scroller_params_t *params);
/** Initialize a lyric scroller and immediately apply its parameter block. */
void egui_view_lyric_scroller_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_lyric_scroller_params_t *params);

/** Set the text shown by the internal label, reset the scroll position, and recalculate overflow. The string pointer is borrowed. */
void egui_view_lyric_scroller_set_text(egui_view_t *self, const char *text);
/** Return the borrowed text pointer shown by the internal label, or NULL when unset or self is NULL. */
const char *egui_view_lyric_scroller_get_text(egui_view_t *self);
/** Set the internal label font, reset the scroll position, and recalculate overflow. */
void egui_view_lyric_scroller_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the internal label font pointer, or NULL when unset or self is NULL. */
const egui_font_t *egui_view_lyric_scroller_get_font(egui_view_t *self);
/** Set the internal label text color and alpha. */
void egui_view_lyric_scroller_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
/** Return the internal label text color. Returns zero color when self is NULL. */
egui_color_t egui_view_lyric_scroller_get_font_color(egui_view_t *self);
/** Return the internal label text alpha. Returns 0 when self is NULL. */
egui_alpha_t egui_view_lyric_scroller_get_font_alpha(egui_view_t *self);
/** Set how many pixels the text moves per tick. Values below 1 clamp to 1. */
void egui_view_lyric_scroller_set_scroll_step(egui_view_t *self, egui_dim_t scroll_step);
/** Return how many pixels the text moves per timer tick. Returns 0 when self is NULL. */
egui_dim_t egui_view_lyric_scroller_get_scroll_step(egui_view_t *self);
/** Set the timer interval between scroll steps. Values of 0 clamp to 1 ms. */
void egui_view_lyric_scroller_set_interval_ms(egui_view_t *self, uint16_t interval_ms);
/** Return the timer interval between scroll steps in milliseconds. Returns 0 when self is NULL. */
uint16_t egui_view_lyric_scroller_get_interval_ms(egui_view_t *self);
/** Set how long the text pauses at each end before reversing direction. */
void egui_view_lyric_scroller_set_pause_duration_ms(egui_view_t *self, uint16_t pause_duration_ms);
/** Return how long the text pauses at each end before reversing direction. Returns 0 when self is NULL. */
uint16_t egui_view_lyric_scroller_get_pause_duration_ms(egui_view_t *self);
/** Return the measured text width. Returns 0 when self is NULL. */
egui_dim_t egui_view_lyric_scroller_get_text_width(egui_view_t *self);
/** Return the measured text height. Returns 0 when self is NULL. */
egui_dim_t egui_view_lyric_scroller_get_text_height(egui_view_t *self);
/** Return the current horizontal scroll offset applied to the internal label. Returns 0 when self is NULL. */
egui_dim_t egui_view_lyric_scroller_get_scroll_offset_x(egui_view_t *self);
/** Return the maximum horizontal scroll offset for the current text and viewport. Returns 0 when self is NULL. */
egui_dim_t egui_view_lyric_scroller_get_max_scroll_offset(egui_view_t *self);
/** Reset the scroll offset to the start without changing text or style. */
void egui_view_lyric_scroller_restart(egui_view_t *self);
/** Start scrolling when the widget is attached and the text actually overflows the viewport. */
void egui_view_lyric_scroller_start(egui_view_t *self);
/** Stop the scrolling timer while keeping the current offset. */
void egui_view_lyric_scroller_stop(egui_view_t *self);
/** Return whether the internal scroll timer is currently running. Returns 0 when self is NULL. */
uint8_t egui_view_lyric_scroller_is_scrolling(egui_view_t *self);
/** Initialize the clipping group and its internal label child. */
void egui_view_lyric_scroller_init(egui_view_t *self, egui_core_t *core);

#ifdef __cplusplus
}
#endif

#endif
