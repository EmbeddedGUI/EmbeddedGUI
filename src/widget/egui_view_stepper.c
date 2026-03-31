#include "egui_view_stepper.h"
#include "egui_view_icon_font.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

static const egui_font_t *egui_view_stepper_get_icon_font(egui_view_stepper_t *local, egui_dim_t area_size)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    return egui_view_icon_font_get_auto(area_size, 18, 22);
}

void egui_view_stepper_set_total_steps(egui_view_t *self, uint8_t total_steps)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    if (total_steps == 0)
    {
        total_steps = 1;
    }
    if (local->total_count == total_steps)
    {
        return;
    }
    egui_view_page_indicator_set_total_count(self, total_steps);
}

uint8_t egui_view_stepper_get_total_steps(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    return local->total_count;
}

void egui_view_stepper_set_current_step(egui_view_t *self, uint8_t current_step)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    if (local->total_count > 0 && current_step >= local->total_count)
    {
        current_step = (uint8_t)(local->total_count - 1);
    }
    if (local->current_index == current_step)
    {
        return;
    }
    egui_view_page_indicator_set_current_index(self, current_step);
}

uint8_t egui_view_stepper_get_current_step(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    return local->current_index;
}

void egui_view_stepper_set_mark_style(egui_view_t *self, egui_view_stepper_mark_style_t style)
{
    EGUI_LOCAL_INIT(egui_view_stepper_t);
    if (local->mark_style == (uint8_t)style)
    {
        return;
    }

    local->mark_style = (uint8_t)style;
    egui_view_invalidate(self);
}

void egui_view_stepper_set_completed_icon(egui_view_t *self, const char *icon)
{
    EGUI_LOCAL_INIT(egui_view_stepper_t);
    if (local->completed_icon == icon)
    {
        return;
    }

    local->completed_icon = icon;
    egui_view_invalidate(self);
}

void egui_view_stepper_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_stepper_t);
    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

static void egui_view_stepper_draw_icon_marks(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_stepper_t);
    egui_view_page_indicator_t *indicator = &local->indicator;
    egui_region_t region;
    egui_dim_t node_diameter;
    egui_dim_t node_radius;
    egui_dim_t start_x;
    egui_dim_t step_gap;
    egui_dim_t center_y;
    egui_dim_t connector_thickness;
    egui_color_t active_color;
    egui_color_t inactive_color;
    uint8_t i;

    egui_view_get_work_region(self, &region);

    if (indicator->total_count == 0 || region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    node_diameter = region.size.height - EGUI_MAX(region.size.height / 4, 6);
    if (node_diameter < 10)
    {
        node_diameter = EGUI_MAX(region.size.height - 2, 8);
    }
    if (indicator->total_count > 1)
    {
        egui_dim_t max_slot = (region.size.width - 8) / (indicator->total_count - 1);
        if (node_diameter > max_slot)
        {
            node_diameter = EGUI_MAX(max_slot, 8);
        }
    }

    node_radius = EGUI_MAX(node_diameter / 2, 3);
    center_y = region.location.y + region.size.height / 2;

    if (indicator->total_count == 1)
    {
        start_x = region.location.x + region.size.width / 2;
        step_gap = 0;
    }
    else
    {
        egui_dim_t span = region.size.width - node_radius * 2;
        if (span < node_diameter)
        {
            span = node_diameter;
        }
        step_gap = span / (indicator->total_count - 1);
        start_x = region.location.x + (region.size.width - (step_gap * (indicator->total_count - 1) + node_radius * 2)) / 2 + node_radius;
    }

    active_color = egui_view_get_enable(self) ? indicator->active_color : EGUI_THEME_DISABLED;
    inactive_color = egui_view_get_enable(self) ? indicator->inactive_color : EGUI_THEME_DISABLED;
    connector_thickness = EGUI_MAX(region.size.height / 10, 2);

    if (indicator->total_count > 1)
    {
        for (i = 0; i < (uint8_t)(indicator->total_count - 1); i++)
        {
            egui_dim_t x1 = start_x + i * step_gap;
            egui_dim_t x2 = start_x + (i + 1) * step_gap;
            egui_dim_t left = x1 + node_radius;
            egui_dim_t width = x2 - x1 - node_radius * 2;
            egui_color_t connector_color = (i < indicator->current_index) ? active_color : inactive_color;

            if (width <= 0)
            {
                continue;
            }

            egui_canvas_draw_round_rectangle_fill(left, center_y - connector_thickness / 2, width, connector_thickness, connector_thickness / 2,
                                                  connector_color, indicator->alpha);
        }
    }

    for (i = 0; i < indicator->total_count; i++)
    {
        egui_dim_t cx = start_x + i * step_gap;

        if (i < indicator->current_index)
        {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
            {
                egui_color_t badge_light = egui_rgb_mix(active_color, EGUI_COLOR_WHITE, 60);
                egui_gradient_stop_t badge_stops[2] = {
                        {.position = 0, .color = badge_light},
                        {.position = 255, .color = active_color},
                };
                egui_gradient_t badge_grad = {
                        .type = EGUI_GRADIENT_TYPE_RADIAL,
                        .stop_count = 2,
                        .alpha = indicator->alpha,
                        .stops = badge_stops,
                        .center_x = 0,
                        .center_y = 0,
                        .radius = node_radius,
                };
                egui_canvas_draw_circle_fill_gradient(cx, center_y, node_radius, &badge_grad);
            }
#else
            egui_canvas_draw_circle_fill(cx, center_y, node_radius, active_color, indicator->alpha);
#endif
            if (local->completed_icon != NULL)
            {
                const egui_font_t *icon_font = egui_view_stepper_get_icon_font(local, node_radius * 2);
                if (icon_font == NULL || local->completed_icon[0] == '\0')
                {
                    continue;
                }

                egui_region_t icon_region = {
                        {cx - node_radius, center_y - node_radius},
                        {node_radius * 2, node_radius * 2},
                };
                egui_canvas_draw_text_in_rect(icon_font, local->completed_icon, &icon_region, EGUI_ALIGN_CENTER, EGUI_COLOR_WHITE, indicator->alpha);
            }
        }
        else if (i == indicator->current_index)
        {
            egui_dim_t inner_radius = EGUI_MAX(node_radius / 3, 2);
            egui_canvas_draw_circle(cx, center_y, node_radius, 2, active_color, indicator->alpha);
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
            egui_canvas_draw_circle_fill_hq(cx, center_y, inner_radius, active_color, indicator->alpha);
#else
            egui_canvas_draw_circle_fill(cx, center_y, inner_radius, active_color, indicator->alpha);
#endif
        }
        else
        {
            egui_dim_t pending_radius = EGUI_MAX(node_radius / 3, 2);
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
            egui_canvas_draw_circle_fill_hq(cx, center_y, pending_radius, inactive_color, indicator->alpha);
#else
            egui_canvas_draw_circle_fill(cx, center_y, pending_radius, inactive_color, indicator->alpha);
#endif
        }
    }
}

void egui_view_stepper_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_stepper_t);
    if (local->mark_style == EGUI_VIEW_STEPPER_MARK_STYLE_ICON)
    {
        egui_view_stepper_draw_icon_marks(self);
        return;
    }

    egui_view_page_indicator_on_draw(self);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_stepper_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_stepper_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_stepper_init(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_stepper_t);
    egui_view_page_indicator_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_stepper_t);
    local->mark_style = EGUI_VIEW_STEPPER_MARK_STYLE_DOT;
    local->completed_icon = EGUI_ICON_MS_DONE;
    local->icon_font = NULL;
    egui_view_set_view_name(self, "egui_view_stepper");
}

void egui_view_stepper_apply_params(egui_view_t *self, const egui_view_stepper_params_t *params)
{
    uint8_t total_steps = params->total_steps;
    uint8_t current_step = params->current_step;
    if (total_steps == 0)
    {
        total_steps = 1;
    }
    if (current_step >= total_steps)
    {
        current_step = (uint8_t)(total_steps - 1U);
    }
    egui_view_page_indicator_params_t indicator_params = {
            .region = params->region,
            .total_count = total_steps,
            .current_index = current_step,
    };
    egui_view_page_indicator_apply_params(self, &indicator_params);
}

void egui_view_stepper_init_with_params(egui_view_t *self, const egui_view_stepper_params_t *params)
{
    egui_view_stepper_init(self);
    egui_view_stepper_apply_params(self, params);
}
