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
    /** Show a numeric count such as `3` or `99+`. */
    EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT = 0,
    /** Show one icon glyph instead of a numeric count. */
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

/** Apply the region and initial count from one parameter block. */
void egui_view_notification_badge_apply_params(egui_view_t *self, const egui_view_notification_badge_params_t *params);
/** Initialize a notification badge and immediately apply its parameter block. */
void egui_view_notification_badge_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_notification_badge_params_t *params);

/** Set the underlying notification count. In count mode, values above `max_display` render as `max+`. */
void egui_view_notification_badge_set_count(egui_view_t *self, uint16_t count);
/** Return the stored notification count. */
uint16_t egui_view_notification_badge_get_count(egui_view_t *self);
/** Set the largest numeric value rendered directly. Values below 1 clamp to 1. */
void egui_view_notification_badge_set_max_display(egui_view_t *self, uint8_t max);
/** Return the largest numeric value rendered directly. */
uint8_t egui_view_notification_badge_get_max_display(egui_view_t *self);
/** Set the badge background color. */
void egui_view_notification_badge_set_badge_color(egui_view_t *self, egui_color_t color);
/** Return the badge background color. */
egui_color_t egui_view_notification_badge_get_badge_color(egui_view_t *self);
/** Set the text or icon tint color drawn inside the badge. */
void egui_view_notification_badge_set_text_color(egui_view_t *self, egui_color_t color);
/** Return the text or icon tint color drawn inside the badge. */
egui_color_t egui_view_notification_badge_get_text_color(egui_view_t *self);
/** Override the font used for numeric-count rendering. */
void egui_view_notification_badge_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the numeric-count font override, or NULL when default font is used. */
const egui_font_t *egui_view_notification_badge_get_font(egui_view_t *self);
/** Switch between count mode and icon mode. */
void egui_view_notification_badge_set_content_style(egui_view_t *self, egui_view_notification_badge_content_style_t style);
/** Return the current badge content mode. */
egui_view_notification_badge_content_style_t egui_view_notification_badge_get_content_style(egui_view_t *self);
/** Borrow the icon glyph string used when content style is `ICON`. */
void egui_view_notification_badge_set_icon(egui_view_t *self, const char *icon);
/** Return the borrowed icon glyph string used in `ICON` mode. */
const char *egui_view_notification_badge_get_icon(egui_view_t *self);
/** Override the icon font used in `ICON` mode. */
void egui_view_notification_badge_set_icon_font(egui_view_t *self, const egui_font_t *font);
/** Return the icon font override, or NULL when automatic icon-font resolution is used. */
const egui_font_t *egui_view_notification_badge_get_icon_font(egui_view_t *self);
/** Default draw hook used by the notification-badge API table. */
void egui_view_notification_badge_on_draw(egui_view_t *self);
/** Initialize the badge widget with count mode, a default notifications icon, and `max_display = 99`. */
void egui_view_notification_badge_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_NOTIFICATION_BADGE_H_ */
