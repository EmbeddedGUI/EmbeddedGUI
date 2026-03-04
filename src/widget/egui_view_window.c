#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_window.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#include "shadow/egui_shadow.h"
#endif

void egui_view_window_set_title(egui_view_t *self, const char *title)
{
    EGUI_LOCAL_INIT(egui_view_window_t);
    egui_view_label_set_text(EGUI_VIEW_OF(&local->title_label), title);
    egui_view_invalidate(self);
}

void egui_view_window_set_header_height(egui_view_t *self, egui_dim_t height)
{
    EGUI_LOCAL_INIT(egui_view_window_t);
    if (local->header_height != height)
    {
        local->header_height = height;
        egui_view_set_size(EGUI_VIEW_OF(&local->title_label), self->region.size.width, height);
        egui_view_set_position(EGUI_VIEW_OF(&local->content), 0, height);
        egui_view_set_size(EGUI_VIEW_OF(&local->content), self->region.size.width, self->region.size.height - height);
        egui_view_invalidate(self);
    }
}

void egui_view_window_add_content(egui_view_t *self, egui_view_t *child)
{
    EGUI_LOCAL_INIT(egui_view_window_t);
    egui_view_group_add_child(EGUI_VIEW_OF(&local->content), child);
    egui_view_invalidate(self);
}

void egui_view_window_set_on_close(egui_view_t *self, egui_view_window_close_cb_t callback)
{
    EGUI_LOCAL_INIT(egui_view_window_t);
    local->on_close = callback;
}

void egui_view_window_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_window_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    // Draw header background
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
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
        egui_canvas_draw_rectangle_fill_gradient(region.location.x, region.location.y, region.size.width, local->header_height, &grad);
    }
#else
    egui_canvas_draw_rectangle_fill(region.location.x, region.location.y, region.size.width, local->header_height, local->header_color, EGUI_ALPHA_100);
#endif

    // Draw content background
    egui_canvas_draw_rectangle_fill(region.location.x, region.location.y + local->header_height, region.size.width, region.size.height - local->header_height,
                                    local->content_bg_color, EGUI_ALPHA_100);
}

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

void egui_view_window_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_window_t);
    // call super init.
    egui_view_group_init(self);

    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_window_t);

    // init local data.
    local->header_height = 30;
    local->header_color = EGUI_THEME_PRIMARY_DARK;
    local->content_bg_color = EGUI_THEME_SURFACE;
    local->on_close = NULL;

    // Init title label
    egui_view_label_init(EGUI_VIEW_OF(&local->title_label));
    egui_view_label_set_font(EGUI_VIEW_OF(&local->title_label), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&local->title_label), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&local->title_label), EGUI_ALIGN_CENTER);
    egui_view_set_position(EGUI_VIEW_OF(&local->title_label), 0, 0);

    // Init content group
    egui_view_group_init(EGUI_VIEW_OF(&local->content));

    // Add title and content as children of the window
    egui_view_group_add_child(self, EGUI_VIEW_OF(&local->title_label));
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

void egui_view_window_apply_params(egui_view_t *self, const egui_view_window_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_window_t);

    self->region = params->region;
    local->header_height = params->header_height;

    // Set title
    egui_view_label_set_text(EGUI_VIEW_OF(&local->title_label), params->title);

    // Layout title label
    egui_view_set_position(EGUI_VIEW_OF(&local->title_label), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&local->title_label), params->region.size.width, params->header_height);

    // Layout content area
    egui_view_set_position(EGUI_VIEW_OF(&local->content), 0, params->header_height);
    egui_view_set_size(EGUI_VIEW_OF(&local->content), params->region.size.width, params->region.size.height - params->header_height);

    egui_view_invalidate(self);
}

void egui_view_window_init_with_params(egui_view_t *self, const egui_view_window_params_t *params)
{
    egui_view_window_init(self);
    egui_view_window_apply_params(self, params);
}
