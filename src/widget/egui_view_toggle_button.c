#include <stdio.h>
#include <assert.h>

#include "egui_view_toggle_button.h"
#include "core/egui_core.h"
#include "egui_view_icon_font.h"
#include "font/egui_font.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
#include "canvas/egui_canvas_gradient.h"
#endif

/**
 * @file egui_view_toggle_button.c
 * @brief Full-surface toggle button with optional icon and text content.
 *
 * Reading notes:
 * - the frame color reflects the persistent on or off state while press feedback is only an overlay;
 * - content layout centers icon and text as one combined block when both are present;
 * - touch handling flips the state only after a complete press-and-release inside the button.
 */

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_toggle_button_t);

/*
 * Unlike checkbox and radio button widgets, this control treats the whole button face as the toggle target.
 * It manages its own pressed state so the frame can show a full-surface press overlay.
 */

/** Resolve the label font, falling back to the default configured font. */
static const egui_font_t *egui_view_toggle_button_get_text_font(const egui_view_toggle_button_t *local)
{
    if (local->font != NULL)
    {
        return local->font;
    }

    return (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
}

/** Draw icon and text content centered within the button's work region. */
static void egui_view_toggle_button_draw_content(egui_canvas_t *canvas, egui_view_toggle_button_t *local, const egui_region_t *region, egui_color_t text_color)
{
    const char *icon = local->icon;
    const char *text = local->text;
    const egui_font_t *text_font = egui_view_toggle_button_get_text_font(local);
    egui_region_t draw_region = *region;

    if (!EGUI_VIEW_ICON_TEXT_VALID(icon) && !EGUI_VIEW_TEXT_VALID(text))
    {
        return;
    }

    if (!EGUI_VIEW_ICON_TEXT_VALID(icon))
    {
        if (text_font != NULL)
        {
            egui_canvas_draw_text_in_rect(canvas, text_font, text, &draw_region, EGUI_ALIGN_CENTER, text_color, EGUI_ALPHA_100);
        }
        return;
    }

    if (!EGUI_VIEW_TEXT_VALID(text))
    {
        const egui_font_t *icon_font = EGUI_VIEW_ICON_FONT_RESOLVE(local->icon_font, EGUI_MIN(region->size.width, region->size.height), 20, 26);
        if (icon_font != NULL)
        {
            egui_canvas_draw_text_in_rect(canvas, icon_font, icon, &draw_region, EGUI_ALIGN_CENTER, text_color, EGUI_ALPHA_100);
        }
        return;
    }

    {
        const egui_font_t *icon_font = EGUI_VIEW_ICON_FONT_RESOLVE(local->icon_font, region->size.height, 20, 26);
        egui_dim_t icon_width = 0;
        egui_dim_t icon_height = 0;
        egui_dim_t text_width = 0;
        egui_dim_t text_height = 0;
        egui_dim_t content_width;
        egui_dim_t start_x;
        egui_dim_t gap = local->icon_text_gap;
        egui_region_t icon_region;
        egui_region_t text_region;

        if (icon_font == NULL)
        {
            if (text_font != NULL)
            {
                egui_canvas_draw_text_in_rect(canvas, text_font, text, &draw_region, EGUI_ALIGN_CENTER, text_color, EGUI_ALPHA_100);
            }
            return;
        }

        if (icon_font->api != NULL && icon_font->api->get_str_size != NULL)
        {
            egui_font_get_str_size_with_canvas(icon_font, canvas, icon, 0, 0, &icon_width, &icon_height);
        }
        if (text_font != NULL && text_font->api != NULL && text_font->api->get_str_size != NULL)
        {
            egui_font_get_str_size_with_canvas(text_font, canvas, text, 0, 0, &text_width, &text_height);
        }
        (void)icon_height;
        (void)text_height;

        if (icon_width <= 0)
        {
            icon_width = EGUI_MIN(region->size.height, 20);
        }
        if (text_width <= 0)
        {
            gap = 0;
        }
        if (gap < 0)
        {
            gap = 0;
        }

        // Measure icon and label first, then center the combined content block as one unit.
        content_width = icon_width + gap + text_width;
        if (content_width < 0)
        {
            content_width = 0;
        }

        start_x = region->location.x + (region->size.width - content_width) / 2;
        if (start_x < region->location.x)
        {
            start_x = region->location.x;
        }

        icon_region.location.x = start_x;
        icon_region.location.y = region->location.y;
        icon_region.size.width = EGUI_MIN(icon_width, region->size.width);
        icon_region.size.height = region->size.height;

        text_region.location.x = icon_region.location.x + icon_region.size.width + gap;
        text_region.location.y = region->location.y;
        text_region.size.width = region->location.x + region->size.width - text_region.location.x;
        text_region.size.height = region->size.height;

        egui_canvas_draw_text_in_rect(canvas, icon_font, icon, &icon_region, EGUI_ALIGN_CENTER, text_color, EGUI_ALPHA_100);
        if (text_region.size.width > 0 && text_width > 0 && text_font != NULL)
        {
            egui_canvas_draw_text_in_rect(canvas, text_font, text, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color, EGUI_ALPHA_100);
        }
    }
}

/** Draw the button frame for the current toggle and pressed state, then return the content color. */
static egui_color_t egui_view_toggle_button_draw_frame(egui_view_t *self, egui_view_toggle_button_t *local, egui_region_t *region)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_color_t bg_color;

    egui_view_get_work_region(self, region);

    bg_color = local->is_toggled ? local->on_color : local->off_color;
    if (!egui_view_get_enable(self))
    {
        bg_color = EGUI_THEME_DISABLED;
    }

    // Background color reflects the logical toggle state before content is rendered.
#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(bg_color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t tb_stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = bg_color},
        };
        egui_gradient_t tb_grad = {
                .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = tb_stops,
        };
        egui_canvas_draw_round_rectangle_fill_gradient(canvas, region->location.x, region->location.y, region->size.width, region->size.height,
                                                       local->corner_radius, &tb_grad);
    }
#else
    egui_canvas_draw_round_rectangle_fill(canvas, region->location.x, region->location.y, region->size.width, region->size.height, local->corner_radius,
                                          bg_color, EGUI_ALPHA_100);
#endif

    // Press feedback is a transient overlay, separate from the persistent on/off background color.
    if (self->is_pressed)
    {
        egui_canvas_draw_round_rectangle_fill(canvas, region->location.x, region->location.y, region->size.width, region->size.height, local->corner_radius,
                                              EGUI_THEME_PRESS_OVERLAY, EGUI_THEME_PRESS_OVERLAY_ALPHA);
    }

    if (!egui_view_get_enable(self))
    {
        return EGUI_THEME_TEXT_SECONDARY;
    }

    if (local->is_toggled)
    {
        return local->text_color;
    }

    return EGUI_THEME_TEXT_PRIMARY;
}

/** Store the callback fired after the toggle state changes. */
void egui_view_toggle_button_set_on_toggled_listener(egui_view_t *self, egui_view_on_toggled_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    local->on_toggled = listener;
}

egui_view_on_toggled_listener_t egui_view_toggle_button_get_on_toggled_listener(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    return local->on_toggled;
}

/** Update the stored state, notify listeners, and redraw only when it really changes. */
void egui_view_toggle_button_set_toggled(egui_view_t *self, uint8_t is_toggled)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    if (is_toggled != local->is_toggled)
    {
        local->is_toggled = is_toggled;
        if (local->on_toggled)
        {
            local->on_toggled(self, is_toggled);
        }
        egui_view_invalidate(self);
    }
}

/** Return whether the button is currently toggled on. */
uint8_t egui_view_toggle_button_get_toggled(egui_view_t *self)
{
    return egui_view_toggle_button_is_toggled(self);
}

/** Return whether the button is currently toggled on. */
uint8_t egui_view_toggle_button_is_toggled(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    return local->is_toggled;
}

/** Replace the optional icon pointer shown by the button. */
void egui_view_toggle_button_set_icon(egui_view_t *self, const char *icon)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);

    if (local->icon == icon)
    {
        return;
    }

    local->icon = icon;
    egui_view_invalidate(self);
}

const char *egui_view_toggle_button_get_icon(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    return local->icon;
}

/** Replace the borrowed label text shown by the button. */
void egui_view_toggle_button_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    local->text = text;
    egui_view_invalidate(self);
}

const char *egui_view_toggle_button_get_text(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    return local->text;
}

/** Override the font used for label text. */
void egui_view_toggle_button_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    if (local->font == font)
    {
        return;
    }
    local->font = font;
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_toggle_button_get_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    return local->font;
}

/** Override the icon font used when the button shows a glyph string. */
void egui_view_toggle_button_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);

    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_toggle_button_get_icon_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    return local->icon_font;
}

/** Change the horizontal spacing between icon and text when both exist. */
void egui_view_toggle_button_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);

    if (local->icon_text_gap == gap)
    {
        return;
    }

    local->icon_text_gap = gap;
    egui_view_invalidate(self);
}

egui_dim_t egui_view_toggle_button_get_icon_text_gap(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    return local->icon_text_gap;
}

/** Override the content color used while the button is toggled on. */
void egui_view_toggle_button_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    local->text_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_toggle_button_get_text_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    return local->text_color;
}

/** Draw the frame first and then paint icon and text content on top. */
void egui_view_toggle_button_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_region_t region;
    egui_color_t text_color = egui_view_toggle_button_draw_frame(self, local, &region);

    egui_view_toggle_button_draw_content(canvas, local, &region, text_color);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
/** Flip the stored on/off state when the normal click pipeline completes. */
static int egui_view_toggle_button_perform_click(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);

    egui_view_toggle_button_set_toggled(self, !local->is_toggled);
    return 1;
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/** Convert pointer press and release events into full-face pressed state plus toggle commits. */
int egui_view_toggle_button_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    int is_inside = egui_region_pt_in_rect(&self->region_screen, event->location.x, event->location.y);

    if (self->is_enable == false)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        // Press state tracks the full button face, not a smaller sub-region.
        egui_view_set_pressed(self, is_inside);
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        if (self->is_pressed != is_inside)
        {
            egui_view_set_pressed(self, is_inside);
        }
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_UP:
    {
        int was_pressed = self->is_pressed;
        egui_view_set_pressed(self, false);
        if (was_pressed && is_inside)
        {
            // Only a complete press-and-release inside the button commits the state flip.
            egui_view_toggle_button_set_toggled(self, !local->is_toggled);
        }
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
    {
        egui_view_set_pressed(self, false);
        break;
    }
    default:
        break;
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

/* Use default view hooks except for toggle-button drawing and full-face touch handling. */
const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_toggle_button_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_toggle_button_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_toggle_button_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .perform_click = egui_view_toggle_button_perform_click,
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

/** Initialize default colors, fonts, and full-surface toggle behavior for one button. */
void egui_view_toggle_button_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_toggle_button_t);
    // Initialize the base view first so touch dispatch and invalidation are available.
    egui_view_init(self, core);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_toggle_button_t);

    // Defaults describe a pill-like button with no content yet and an "off" state.
    local->on_toggled = NULL;
    local->is_toggled = 0;
    local->icon = NULL;
    local->text = NULL;
    local->font = NULL;
    local->icon_font = NULL;
    local->text_color = EGUI_THEME_TEXT;
    local->on_color = EGUI_THEME_PRIMARY;
    local->off_color = EGUI_THEME_TRACK_OFF;
    local->corner_radius = EGUI_THEME_RADIUS_MD;
    local->icon_text_gap = 6;
    self->is_clickable = true;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    self->is_focusable = true;
#endif

    egui_view_set_view_name(self, "egui_view_toggle_button");
}

/** Apply geometry, text, and initial toggled state from one parameter block. */
void egui_view_toggle_button_apply_params(egui_view_t *self, const egui_view_toggle_button_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);

    self->region = params->region;

    local->text = params->text;
    local->is_toggled = params->is_toggled;

    egui_view_invalidate(self);
}

/** Convenience helper that chains toggle-button init and params. */
void egui_view_toggle_button_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_toggle_button_params_t *params)
{
    egui_view_toggle_button_init(self, core);
    egui_view_toggle_button_apply_params(self, params);
}
