#include <stdio.h>
#include <assert.h>

#include "egui_view_radio_button.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

void egui_view_radio_button_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    local->text = text;
    egui_view_invalidate(self);
}

void egui_view_radio_button_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_radio_button_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    local->text_color = color;
    egui_view_invalidate(self);
}

// ============== Radio Group ==============
void egui_view_radio_group_init(egui_view_radio_group_t *group)
{
    egui_slist_init(&group->buttons);
    group->on_changed = NULL;
}

void egui_view_radio_group_add(egui_view_radio_group_t *group, egui_view_t *button)
{
    egui_view_radio_button_t *rb = (egui_view_radio_button_t *)button;
    rb->group = group;
    egui_slist_append(&group->buttons, &rb->group_node);
}

void egui_view_radio_group_set_on_changed_listener(egui_view_radio_group_t *group, egui_view_on_radio_changed_listener_t listener)
{
    group->on_changed = listener;
}

// ============== Radio Button ==============
void egui_view_radio_button_set_checked(egui_view_t *self, uint8_t is_checked)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);

    if (is_checked == local->is_checked)
    {
        return;
    }

    local->is_checked = is_checked;

    // If checking this button and it belongs to a group,
    // uncheck all others in the group
    if (is_checked && local->group != NULL)
    {
        egui_snode_t *sn;
        int index = 0;
        int selected_index = 0;

        EGUI_SLIST_FOR_EACH_NODE(&local->group->buttons, sn)
        {
            egui_view_radio_button_t *rb = egui_slist_container_of(sn, egui_view_radio_button_t, group_node);
            if (rb != local && rb->is_checked)
            {
                rb->is_checked = 0;
                egui_view_invalidate(EGUI_VIEW_OF(rb));
            }
            if (rb == local)
            {
                selected_index = index;
            }
            index++;
        }

        // Fire group listener
        if (local->group->on_changed)
        {
            local->group->on_changed(self, selected_index);
        }
    }

    egui_view_invalidate(self);
}

static void egui_view_radio_button_on_click(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);

    // Only allow checking (not unchecking by clicking same button)
    if (!local->is_checked)
    {
        egui_view_radio_button_set_checked(self, 1);
    }
}

void egui_view_radio_button_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    // Radio button is a circle. Use min(width, height) as diameter.
    egui_dim_t size = EGUI_MIN(region.size.width, region.size.height);
    egui_dim_t center_x = region.location.x + size / 2;
    egui_dim_t center_y = region.location.y + size / 2;
    egui_dim_t outer_radius = size / 2 - 1;
    egui_dim_t stroke = 2;

    if (local->text != NULL)
    {
        center_x = region.location.x + size / 2 + 1;
    }

    egui_color_t outer_color = local->is_checked ? local->dot_color : local->circle_color;
    egui_color_t ring_color = egui_view_get_enable(self) ? outer_color : EGUI_THEME_DISABLED;

    // Draw outer circle (always)
    egui_canvas_draw_circle(center_x, center_y, outer_radius, stroke, ring_color, local->alpha);

    if (local->is_checked)
    {
        // Draw inner filled circle (about 50% of outer radius)
        egui_dim_t inner_radius = outer_radius * 5 / 10;
        egui_color_t fill_color = egui_view_get_enable(self) ? local->dot_color : EGUI_THEME_DISABLED;
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t dot_light = egui_rgb_mix(fill_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t dot_stops[2] = {
                    {.position = 0, .color = dot_light},
                    {.position = 255, .color = fill_color},
            };
            egui_gradient_t dot_grad = {
                    .type = EGUI_GRADIENT_TYPE_RADIAL,
                    .stop_count = 2,
                    .alpha = local->alpha,
                    .stops = dot_stops,
                    .center_x = 0,
                    .center_y = 0,
                    .radius = inner_radius,
            };
            egui_canvas_draw_circle_fill_gradient(center_x, center_y, inner_radius, &dot_grad);
        }
#else
        egui_canvas_draw_circle_fill(center_x, center_y, inner_radius, fill_color, local->alpha);
#endif
    }

    if (local->text != NULL)
    {
        const egui_font_t *font = local->font ? local->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
        egui_color_t text_color = egui_view_get_enable(self) ? local->text_color : EGUI_THEME_DISABLED;

        egui_region_t text_region;
        text_region.location.x = center_x + outer_radius + local->text_gap;
        text_region.location.y = region.location.y;
        text_region.size.width = region.location.x + region.size.width - text_region.location.x;
        text_region.size.height = region.size.height;
        if (text_region.size.width > 0)
        {
            egui_canvas_draw_text_in_rect(font, local->text, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color, local->alpha);
        }
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_radio_button_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_radio_button_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_radio_button_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_radio_button_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_radio_button_t);

    // init local data.
    local->group_node.next = NULL;
    local->group = NULL;
    local->is_checked = false;
    local->alpha = EGUI_ALPHA_100;
    local->circle_color = EGUI_THEME_BORDER;
    local->dot_color = EGUI_THEME_PRIMARY;
    local->text = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->text_color = EGUI_THEME_TEXT;
    local->text_gap = 6;

    egui_view_set_on_click_listener(self, egui_view_radio_button_on_click);

    egui_view_set_view_name(self, "egui_view_radio_button");
}

void egui_view_radio_button_apply_params(egui_view_t *self, const egui_view_radio_button_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);

    self->region = params->region;

    local->is_checked = params->is_checked;
    local->text = params->text;

    egui_view_invalidate(self);
}

void egui_view_radio_button_init_with_params(egui_view_t *self, const egui_view_radio_button_params_t *params)
{
    egui_view_radio_button_init(self);
    egui_view_radio_button_apply_params(self, params);
}
