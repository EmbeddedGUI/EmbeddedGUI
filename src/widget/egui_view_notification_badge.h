#ifndef _EGUI_VIEW_NOTIFICATION_BADGE_H_
#define _EGUI_VIEW_NOTIFICATION_BADGE_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_notification_badge egui_view_notification_badge_t;

typedef enum
{
    EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT = 0,
    EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_ICON = 1,
} egui_view_notification_badge_content_style_t;

struct egui_view_notification_badge
{
    egui_view_t base;

    uint16_t count;
    uint8_t max_display;
    egui_color_t badge_color;
    egui_color_t text_color;
    const egui_font_t *font;
    uint8_t content_style;
    const char *icon;
    const egui_font_t *icon_font;
    char text_buffer[8];
};

// ============== Notification Badge Params ==============
typedef struct egui_view_notification_badge_params egui_view_notification_badge_params_t;
struct egui_view_notification_badge_params
{
    egui_region_t region;
    uint16_t count;
};

#define EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT(_name, _x, _y, _w, _h, _count)                                                                                \
    static const egui_view_notification_badge_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .count = (_count)}

void egui_view_notification_badge_apply_params(egui_view_t *self, const egui_view_notification_badge_params_t *params);
void egui_view_notification_badge_init_with_params(egui_view_t *self, const egui_view_notification_badge_params_t *params);

void egui_view_notification_badge_set_count(egui_view_t *self, uint16_t count);
uint16_t egui_view_notification_badge_get_count(egui_view_t *self);
void egui_view_notification_badge_set_max_display(egui_view_t *self, uint8_t max);
void egui_view_notification_badge_set_badge_color(egui_view_t *self, egui_color_t color);
void egui_view_notification_badge_set_text_color(egui_view_t *self, egui_color_t color);
void egui_view_notification_badge_set_font(egui_view_t *self, const egui_font_t *font);
void egui_view_notification_badge_set_content_style(egui_view_t *self, egui_view_notification_badge_content_style_t style);
void egui_view_notification_badge_set_icon(egui_view_t *self, const char *icon);
void egui_view_notification_badge_set_icon_font(egui_view_t *self, const egui_font_t *font);
void egui_view_notification_badge_on_draw(egui_view_t *self);
void egui_view_notification_badge_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_NOTIFICATION_BADGE_H_ */
