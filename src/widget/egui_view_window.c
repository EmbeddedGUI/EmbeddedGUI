#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_window.h"
#include "core/egui_core.h"
#include "egui_view_icon_font.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
#include "canvas/egui_canvas_gradient.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#include "shadow/egui_shadow.h"
#endif

/**
 * @file egui_view_window.c
 * @brief Window container that combines header chrome with an inner content group.
 *
 * Reading notes:
 * - the outer group
 * owns title, close, and content subviews so window users only add content widgets;
 * - layout reserves optional space for the close glyph and keeps the title
 * centered in the remaining header area;
 * - the default close behavior hides the window, but applications can replace it with their own callback.
 */

/** Choose an icon font size that fits one square header control area. */
static const egui_font_t *egui_view_window_get_auto_icon_font(egui_dim_t area_size)
{
    return egui_view_icon_font_get_auto(area_size, 18, 22);
}

/** Resolve the effective close-icon font, falling back to an automatic size from the header height. */
static const egui_font_t *egui_view_window_resolve_close_icon_font(const egui_view_window_t *local)
{
    if (local->close_icon_font != NULL)
    {
        return local->close_icon_font;
    }

    return egui_view_window_get_auto_icon_font(local->header_height);
}

/** Report whether the close control currently has both glyph text and a font to draw with. */
static uint8_t egui_view_window_has_close_icon(const egui_view_window_t *local)
{
    const egui_font_t *icon_font = egui_view_window_resolve_close_icon_font(local);
    return (icon_font != NULL && local->close_icon != NULL && local->close_icon[0] != '\0') ? 1U : 0U;
}

/** Recompute title, close-button, and content-group rectangles after size or icon changes. */
static void egui_view_window_update_layout(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_window_t);

    egui_dim_t header_height = local->header_height;
    egui_dim_t window_width = self->region.size.width;
    egui_dim_t window_height = self->region.size.height;
    egui_dim_t close_width = egui_view_window_has_close_icon(local) ? header_height : 0;
    egui_dim_t title_width;
    egui_dim_t content_height;

    if (close_width * 3 > window_width)
    {
        close_width = window_width / 3;
    }

    title_width = window_width - close_width * 2;
    if (title_width < 0)
    {
        title_width = 0;
    }

    content_height = window_height - header_height;
    if (content_height < 0)
    {
        content_height = 0;
    }

    egui_view_set_position(EGUI_VIEW_OF(&local->title_label), close_width, 0);
    egui_view_set_size(EGUI_VIEW_OF(&local->title_label), title_width, header_height);

    egui_view_set_position(EGUI_VIEW_OF(&local->close_label), window_width - close_width, 0);
    egui_view_set_size(EGUI_VIEW_OF(&local->close_label), close_width, header_height);
    egui_view_label_set_font(EGUI_VIEW_OF(&local->close_label), egui_view_window_resolve_close_icon_font(local));

    egui_view_set_position(EGUI_VIEW_OF(&local->content), 0, header_height);
    egui_view_set_size(EGUI_VIEW_OF(&local->content), window_width, content_height);
}

/** Forward close clicks to the user callback, or hide the window when no callback is installed. */
static void egui_view_window_on_close_click(egui_view_t *self)
{
    egui_view_t *window_view = EGUI_VIEW_PARENT(self);
    egui_view_window_t *local;
    if (window_view == NULL)
    {
        return;
    }

    local = (egui_view_window_t *)window_view;

    if (local->on_close != NULL)
    {
        local->on_close(window_view);
        return;
    }

    egui_view_set_gone(window_view, 1);
}

/** Replace the borrowed title text shown by the internal header label. */
void egui_view_window_set_title(egui_view_t *self, const char *title)
{
    EGUI_LOCAL_INIT(egui_view_window_t);
    egui_view_label_set_text(EGUI_VIEW_OF(&local->title_label), title);
    egui_view_invalidate(self);
}

const char *egui_view_window_get_title(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_window_t);
    return egui_view_label_get_text(EGUI_VIEW_OF(&local->title_label));
}

/** Change the header height, then relayout the title, close control, and content area. */
void egui_view_window_set_header_height(egui_view_t *self, egui_dim_t height)
{
    EGUI_LOCAL_INIT(egui_view_window_t);
    if (local->header_height != height)
    {
        local->header_height = height;
        egui_view_window_update_layout(self);
        egui_view_invalidate(self);
    }
}

egui_dim_t egui_view_window_get_header_height(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_window_t);
    return local->header_height;
}

void egui_view_window_set_header_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_window_t);
    if (local->header_color.full == color.full)
    {
        return;
    }

    local->header_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_window_get_header_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_window_t);
    return local->header_color;
}

void egui_view_window_set_content_bg_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_window_t);
    if (local->content_bg_color.full == color.full)
    {
        return;
    }

    local->content_bg_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_window_get_content_bg_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_window_t);
    return local->content_bg_color;
}

/** Replace the close glyph string and update layout because icon presence changes reserved header space. */
void egui_view_window_set_close_icon(egui_view_t *self, const char *icon)
{
    EGUI_LOCAL_INIT(egui_view_window_t);

    if (local->close_icon == icon)
    {
        return;
    }

    local->close_icon = icon;
    egui_view_label_set_text(EGUI_VIEW_OF(&local->close_label), local->close_icon);
    egui_view_window_update_layout(self);
    egui_view_invalidate(self);
}

const char *egui_view_window_get_close_icon(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_window_t);
    return local->close_icon;
}

/** Override the close-icon font and recompute layout because icon visibility may change. */
void egui_view_window_set_close_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_window_t);

    if (local->close_icon_font == font)
    {
        return;
    }

    local->close_icon_font = font;
    egui_view_window_update_layout(self);
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_window_get_close_icon_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_window_t);
    return local->close_icon_font;
}

egui_view_t *egui_view_window_get_content(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_window_t);
    return EGUI_VIEW_OF(&local->content);
}

/** Add one child into the window's content group instead of the outer chrome container. */
void egui_view_window_add_content(egui_view_t *self, egui_view_t *child)
{
    EGUI_LOCAL_INIT(egui_view_window_t);
    egui_view_group_add_child(EGUI_VIEW_OF(&local->content), child);
    egui_view_invalidate(self);
}

/** Store the callback that intercepts close-button clicks. */
void egui_view_window_set_on_close(egui_view_t *self, egui_view_window_close_cb_t callback)
{
    EGUI_LOCAL_INIT(egui_view_window_t);
    local->on_close = callback;
}

egui_view_window_close_cb_t egui_view_window_get_on_close(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_window_t);
    return local->on_close;
}

/** Draw window chrome: header background, close-button press feedback, and content background. */
void egui_view_window_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_window_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    // Draw header background
#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(local->header_color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = local->header_color},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = stops,
        };
        egui_canvas_draw_rectangle_fill_gradient(canvas, region.location.x, region.location.y, region.size.width, local->header_height, &grad);
    }
#else
    egui_canvas_draw_rectangle_fill(canvas, region.location.x, region.location.y, region.size.width, local->header_height, local->header_color, EGUI_ALPHA_100);
#endif

    if (egui_view_get_pressed(EGUI_VIEW_OF(&local->close_label)))
    {
        egui_view_t *close_view = EGUI_VIEW_OF(&local->close_label);
        egui_dim_t inset = close_view->region.size.width / 5;
        egui_dim_t hotspot_w = close_view->region.size.width - inset * 2;
        egui_dim_t hotspot_h = close_view->region.size.height - inset * 2;
        egui_dim_t hotspot_r = hotspot_h / 2;

        if (hotspot_w > 0 && hotspot_h > 0)
        {
            egui_canvas_draw_round_rectangle_fill(canvas, close_view->region.location.x + inset, close_view->region.location.y + inset, hotspot_w, hotspot_h,
                                                  hotspot_r, EGUI_COLOR_WHITE, 36);
        }
    }

    // Draw content background
    egui_canvas_draw_rectangle_fill(canvas, region.location.x, region.location.y + local->header_height, region.size.width,
                                    region.size.height - local->header_height, local->content_bg_color, EGUI_ALPHA_100);
}

/* Use group behavior for child management and events, but keep custom window chrome drawing. */
const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_window_t) = {
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = egui_view_group_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_group_calculate_layout,
        .request_layout = egui_view_group_request_layout,
        .draw = egui_view_group_draw,
        .on_attach_to_window = egui_view_group_on_attach_to_window,
        .on_draw = egui_view_window_on_draw,
        .on_detach_from_window = egui_view_group_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_group_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

/** Initialize the outer group plus its built-in title label, close label, and content group. */
void egui_view_window_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_window_t);
    // call super init.
    egui_view_group_init(self, core);

    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_window_t);

    // init local data.
    local->header_height = 30;
    local->header_color = EGUI_THEME_PRIMARY_DARK;
    local->content_bg_color = EGUI_THEME_SURFACE;
    local->close_icon = EGUI_ICON_MS_CROSS;
    local->close_icon_font = NULL;
    local->on_close = NULL;

    // Init title label
    egui_view_label_init(EGUI_VIEW_OF(&local->title_label), core);
    egui_view_label_set_font(EGUI_VIEW_OF(&local->title_label), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&local->title_label), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&local->title_label), EGUI_ALIGN_CENTER);

    // Init close icon label
    egui_view_label_init(EGUI_VIEW_OF(&local->close_label), core);
    egui_view_label_set_text(EGUI_VIEW_OF(&local->close_label), local->close_icon);
    egui_view_label_set_font(EGUI_VIEW_OF(&local->close_label), egui_view_window_resolve_close_icon_font(local));
    egui_view_label_set_font_color(EGUI_VIEW_OF(&local->close_label), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&local->close_label), EGUI_ALIGN_CENTER);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&local->close_label), egui_view_window_on_close_click);

    // Init content group
    egui_view_group_init(EGUI_VIEW_OF(&local->content), core);

    // Add title and content as children of the window
    egui_view_group_add_child(self, EGUI_VIEW_OF(&local->title_label));
    egui_view_group_add_child(self, EGUI_VIEW_OF(&local->close_label));
    egui_view_group_add_child(self, EGUI_VIEW_OF(&local->content));

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
    {
        static const egui_shadow_t window_shadow = {
                .width = EGUI_THEME_SHADOW_WIDTH_MD,
                .ofs_x = 0,
                .ofs_y = EGUI_THEME_SHADOW_OFS_Y_MD,
                .spread = 0,
                .opa = EGUI_THEME_SHADOW_OPA,
                .color = EGUI_COLOR_BLACK,
                .corner_radius = 0,
        };
        egui_view_set_shadow(self, &window_shadow);
    }
#endif

    egui_view_set_view_name(self, "egui_view_window");
}

/** Apply geometry, header height, and title text from one parameter block. */
void egui_view_window_apply_params(egui_view_t *self, const egui_view_window_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_window_t);

    self->region = params->region;
    local->header_height = params->header_height;

    // Set title
    egui_view_label_set_text(EGUI_VIEW_OF(&local->title_label), params->title);

    egui_view_window_update_layout(self);

    egui_view_invalidate(self);
}

/** Convenience helper that initializes the window before applying params. */
void egui_view_window_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_window_params_t *params)
{
    egui_view_window_init(self, core);
    egui_view_window_apply_params(self, params);
}
