#ifndef _EGUI_VIEW_LYRIC_SCROLLER_H_
#define _EGUI_VIEW_LYRIC_SCROLLER_H_

#include "core/egui_timer.h"
#include "egui_view_group.h"
#include "egui_view_label.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_lyric_scroller egui_view_lyric_scroller_t;
struct egui_view_lyric_scroller
{
    egui_view_group_t base;

    egui_view_label_t label;
    egui_timer_t scroll_timer;

    egui_dim_t text_width;
    egui_dim_t text_height;
    egui_dim_t scroll_offset_x;
    egui_dim_t max_scroll_offset;
    egui_dim_t scroll_step;

    uint16_t interval_ms;
    uint16_t pause_duration_ms;
    uint16_t pause_ticks_remaining;

    int8_t scroll_direction;
    uint8_t is_attached;
};

typedef struct egui_view_lyric_scroller_params egui_view_lyric_scroller_params_t;
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

#define EGUI_VIEW_LYRIC_SCROLLER_PARAMS_INIT(_name, _x, _y, _w, _h, _text, _font, _color, _alpha)                                                              \
    static const egui_view_lyric_scroller_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                            \
                                                            .text = (_text),                                                                                   \
                                                            .font = (const egui_font_t *)(_font),                                                              \
                                                            .color = (_color),                                                                                 \
                                                            .alpha = (_alpha),                                                                                 \
                                                            .scroll_step = 1,                                                                                  \
                                                            .interval_ms = 50,                                                                                 \
                                                            .pause_duration_ms = 400}

#define EGUI_VIEW_LYRIC_SCROLLER_PARAMS_INIT_SIMPLE(_name, _x, _y, _w, _h, _text)                                                                              \
    static const egui_view_lyric_scroller_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}},                                                            \
                                                            .text = (_text),                                                                                   \
                                                            .font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT,                                             \
                                                            .color = EGUI_THEME_TEXT_PRIMARY,                                                                  \
                                                            .alpha = EGUI_ALPHA_100,                                                                           \
                                                            .scroll_step = 1,                                                                                  \
                                                            .interval_ms = 50,                                                                                 \
                                                            .pause_duration_ms = 400}

void egui_view_lyric_scroller_apply_params(egui_view_t *self, const egui_view_lyric_scroller_params_t *params);
void egui_view_lyric_scroller_init_with_params(egui_view_t *self, const egui_view_lyric_scroller_params_t *params);

void egui_view_lyric_scroller_set_text(egui_view_t *self, const char *text);
void egui_view_lyric_scroller_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_lyric_scroller_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha);
void egui_view_lyric_scroller_set_scroll_step(egui_view_t *self, egui_dim_t scroll_step);
void egui_view_lyric_scroller_set_interval_ms(egui_view_t *self, uint16_t interval_ms);
void egui_view_lyric_scroller_set_pause_duration_ms(egui_view_t *self, uint16_t pause_duration_ms);
void egui_view_lyric_scroller_restart(egui_view_t *self);
void egui_view_lyric_scroller_start(egui_view_t *self);
void egui_view_lyric_scroller_stop(egui_view_t *self);
void egui_view_lyric_scroller_init(egui_view_t *self);

#ifdef __cplusplus
}
#endif

#endif
