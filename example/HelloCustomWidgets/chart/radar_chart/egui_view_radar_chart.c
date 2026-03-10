#include <stdlib.h>

#include "egui_view_radar_chart.h"
#include "utils/egui_fixmath.h"

static uint8_t egui_view_radar_chart_clamp_axis_count(uint8_t count)
{
    if (count > EGUI_VIEW_RADAR_CHART_MAX_AXES)
    {
        return EGUI_VIEW_RADAR_CHART_MAX_AXES;
    }
    return count;
}

static uint8_t egui_view_radar_chart_clamp_value_set_count(uint8_t count)
{
    if (count > EGUI_VIEW_RADAR_CHART_MAX_VALUE_SETS)
    {
        return EGUI_VIEW_RADAR_CHART_MAX_VALUE_SETS;
    }
    return count;
}

static void egui_view_radar_chart_get_point(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t angle_deg, egui_dim_t *x, egui_dim_t *y)
{
    egui_float_t angle_rad = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(angle_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));
    egui_float_t cos_val = EGUI_FLOAT_COS(angle_rad);
    egui_float_t sin_val = EGUI_FLOAT_SIN(angle_rad);
    *x = center_x + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(radius), cos_val));
    *y = center_y + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(radius), sin_val));
}

static void egui_view_radar_chart_build_polygon_points(
        egui_dim_t center_x,
        egui_dim_t center_y,
        egui_dim_t radius,
        uint8_t axis_count,
        const uint8_t *values,
        egui_dim_t *points)
{
    uint8_t i;
    for (i = 0; i < axis_count; i++)
    {
        egui_dim_t point_x;
        egui_dim_t point_y;
        egui_dim_t point_radius = radius;
        int16_t angle = -90 + (int16_t)((int32_t)360 * i / axis_count);
        if (values != NULL)
        {
            point_radius = (egui_dim_t)((int32_t)radius * values[i] / 100);
        }
        egui_view_radar_chart_get_point(center_x, center_y, point_radius, angle, &point_x, &point_y);
        points[i * 2] = point_x;
        points[i * 2 + 1] = point_y;
    }
}

void egui_view_radar_chart_set_axis_labels(egui_view_t *self, const char **axis_labels, uint8_t axis_count)
{
    EGUI_LOCAL_INIT(egui_view_radar_chart_t);
    local->axis_labels = axis_labels;
    local->axis_count = egui_view_radar_chart_clamp_axis_count(axis_count);
    if (local->current_value_set >= local->value_set_count)
    {
        local->current_value_set = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_radar_chart_set_value_set(egui_view_t *self, uint8_t set_index, const uint8_t *values)
{
    EGUI_LOCAL_INIT(egui_view_radar_chart_t);
    if (set_index >= EGUI_VIEW_RADAR_CHART_MAX_VALUE_SETS)
    {
        return;
    }
    local->value_sets[set_index] = values;
    if (local->value_set_count <= set_index)
    {
        local->value_set_count = set_index + 1;
    }
    local->value_set_count = egui_view_radar_chart_clamp_value_set_count(local->value_set_count);
    egui_view_invalidate(self);
}

void egui_view_radar_chart_set_current_value_set(egui_view_t *self, uint8_t set_index)
{
    EGUI_LOCAL_INIT(egui_view_radar_chart_t);
    if (local->value_set_count == 0 || set_index >= local->value_set_count)
    {
        return;
    }
    if (local->current_value_set == set_index)
    {
        return;
    }
    local->current_value_set = set_index;
    egui_view_invalidate(self);
}

uint8_t egui_view_radar_chart_get_current_value_set(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_radar_chart_t);
    return local->current_value_set;
}

void egui_view_radar_chart_set_compare_values(egui_view_t *self, const uint8_t *values)
{
    EGUI_LOCAL_INIT(egui_view_radar_chart_t);
    local->compare_values = values;
    egui_view_invalidate(self);
}

void egui_view_radar_chart_set_show_compare(egui_view_t *self, uint8_t show_compare)
{
    EGUI_LOCAL_INIT(egui_view_radar_chart_t);
    local->show_compare = show_compare ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_radar_chart_set_grid_levels(egui_view_t *self, uint8_t grid_levels)
{
    EGUI_LOCAL_INIT(egui_view_radar_chart_t);
    if (grid_levels == 0)
    {
        grid_levels = 1;
    }
    local->grid_levels = grid_levels;
    egui_view_invalidate(self);
}

void egui_view_radar_chart_set_label_padding(egui_view_t *self, uint8_t label_padding)
{
    EGUI_LOCAL_INIT(egui_view_radar_chart_t);
    if (label_padding < 8)
    {
        label_padding = 8;
    }
    local->label_padding = label_padding;
    egui_view_invalidate(self);
}

void egui_view_radar_chart_set_show_axis_labels(egui_view_t *self, uint8_t show_axis_labels)
{
    EGUI_LOCAL_INIT(egui_view_radar_chart_t);
    local->show_axis_labels = show_axis_labels ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_radar_chart_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_radar_chart_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_radar_chart_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t primary_fill_color,
        egui_color_t primary_line_color,
        egui_color_t secondary_fill_color,
        egui_color_t secondary_line_color,
        egui_color_t grid_color,
        egui_color_t text_color,
        egui_color_t muted_text_color)
{
    EGUI_LOCAL_INIT(egui_view_radar_chart_t);
    local->surface_color = surface_color;
    local->primary_fill_color = primary_fill_color;
    local->primary_line_color = primary_line_color;
    local->secondary_fill_color = secondary_fill_color;
    local->secondary_line_color = secondary_line_color;
    local->grid_color = grid_color;
    local->axis_color = grid_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    egui_view_invalidate(self);
}

static void egui_view_radar_chart_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_radar_chart_t);
    egui_region_t region;
    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t outer_radius;
    egui_dim_t label_radius;
    egui_dim_t background_radius;
    egui_dim_t chart_padding;
    egui_dim_t label_width;
    egui_dim_t label_height;
    egui_dim_t polygon_points[EGUI_VIEW_RADAR_CHART_MAX_AXES * 2];
    egui_dim_t compare_points[EGUI_VIEW_RADAR_CHART_MAX_AXES * 2];
    egui_dim_t current_points[EGUI_VIEW_RADAR_CHART_MAX_AXES * 2];
    const uint8_t *current_values;
    const egui_font_t *font;
    egui_color_t plate_color;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (local->axis_count < 3 || local->axis_count > EGUI_VIEW_RADAR_CHART_MAX_AXES)
    {
        return;
    }
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    font = local->font ? local->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    center_x = region.location.x + region.size.width / 2;
    center_y = region.location.y + region.size.height / 2;
    chart_padding = local->show_axis_labels ? 20 : 8;
    outer_radius = EGUI_MIN(region.size.width, region.size.height) / 2 - chart_padding;
    if (outer_radius <= 8)
    {
        return;
    }
    label_radius = outer_radius + (local->show_axis_labels ? local->label_padding : 10);
    background_radius = outer_radius + 5;
    label_width = EGUI_MAX(region.size.width / 4, 28);
    label_height = 13;
    current_values = local->value_sets[local->current_value_set];
    if (current_values == NULL)
    {
        return;
    }

    plate_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    egui_canvas_draw_circle_fill(center_x, center_y, background_radius, plate_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
    egui_canvas_draw_circle(center_x, center_y, outer_radius + 1, 1, local->grid_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));

    for (i = 1; i <= local->grid_levels; i++)
    {
        egui_dim_t level_radius = (egui_dim_t)((int32_t)outer_radius * i / local->grid_levels);
        egui_color_t level_color = (i == local->grid_levels) ? local->axis_color : local->grid_color;
        egui_alpha_t level_alpha = (i == local->grid_levels) ? egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30) : egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10);
        egui_view_radar_chart_build_polygon_points(center_x, center_y, level_radius, local->axis_count, NULL, polygon_points);
        egui_canvas_draw_polygon(polygon_points, local->axis_count, 1, level_color, level_alpha);
    }

    for (i = 0; i < local->axis_count; i++)
    {
        egui_dim_t axis_x;
        egui_dim_t axis_y;
        int16_t angle = -90 + (int16_t)((int32_t)360 * i / local->axis_count);
        egui_view_radar_chart_get_point(center_x, center_y, outer_radius, angle, &axis_x, &axis_y);
        egui_canvas_draw_line(center_x, center_y, axis_x, axis_y, 1, local->axis_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
    }

    if (local->show_compare && local->compare_values != NULL)
    {
        egui_view_radar_chart_build_polygon_points(center_x, center_y, outer_radius, local->axis_count, local->compare_values, compare_points);
        egui_canvas_draw_polygon_fill(compare_points, local->axis_count, local->secondary_fill_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10));
        egui_canvas_draw_polygon(compare_points, local->axis_count, 1, local->secondary_line_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
        for (i = 0; i < local->axis_count; i++)
        {
            egui_canvas_draw_circle_fill(compare_points[i * 2], compare_points[i * 2 + 1], 3, local->secondary_line_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10));
            egui_canvas_draw_circle_fill(compare_points[i * 2], compare_points[i * 2 + 1], 1, local->secondary_line_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));
        }
    }

    egui_view_radar_chart_build_polygon_points(center_x, center_y, outer_radius, local->axis_count, current_values, current_points);
    egui_canvas_draw_polygon_fill(current_points, local->axis_count, local->primary_fill_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
    egui_canvas_draw_polygon(current_points, local->axis_count, 2, local->primary_line_color, self->alpha);
    for (i = 0; i < local->axis_count; i++)
    {
        egui_canvas_draw_circle_fill(current_points[i * 2], current_points[i * 2 + 1], 5, local->primary_line_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_circle_fill(current_points[i * 2], current_points[i * 2 + 1], 3, local->primary_line_color, self->alpha);
    }

    egui_canvas_draw_circle_fill(center_x, center_y, 4, local->primary_line_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_90));

    if (local->show_axis_labels && local->axis_labels != NULL)
    {
        for (i = 0; i < local->axis_count; i++)
        {
            egui_dim_t label_x;
            egui_dim_t label_y;
            egui_region_t text_region;
            int16_t angle = -90 + (int16_t)((int32_t)360 * i / local->axis_count);
            const char *text_label = local->axis_labels[i] ? local->axis_labels[i] : "";
            egui_view_radar_chart_get_point(center_x, center_y, label_radius, angle, &label_x, &label_y);
            text_region.location.x = label_x - label_width / 2;
            text_region.location.y = label_y - label_height / 2;
            text_region.size.width = label_width;
            text_region.size.height = label_height;
            if (angle > 120 || angle < -120)
            {
                text_region.location.x -= 8;
            }
            else if (angle > -60 && angle < 60)
            {
                text_region.location.x += 8;
            }
            if (angle >= 60 && angle <= 120)
            {
                text_region.location.y += 4;
            }
            else if (angle <= -60 && angle >= -120)
            {
                text_region.location.y -= 4;
            }
            if (text_region.location.x < region.location.x)
            {
                text_region.location.x = region.location.x;
            }
            if (text_region.location.x + text_region.size.width > region.location.x + region.size.width)
            {
                text_region.location.x = region.location.x + region.size.width - text_region.size.width;
            }
            if (text_region.location.y < region.location.y)
            {
                text_region.location.y = region.location.y;
            }
            if (text_region.location.y + text_region.size.height > region.location.y + region.size.height)
            {
                text_region.location.y = region.location.y + region.size.height - text_region.size.height;
            }
            egui_canvas_draw_text_in_rect(font, text_label, &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);
        }
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_radar_chart_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_radar_chart_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_radar_chart_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_radar_chart_t);
    uint8_t i;
    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_radar_chart_t);
    egui_view_set_padding_all(self, 4);

    local->axis_labels = NULL;
    for (i = 0; i < EGUI_VIEW_RADAR_CHART_MAX_VALUE_SETS; i++)
    {
        local->value_sets[i] = NULL;
    }
    local->compare_values = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x0F172A);
    local->primary_fill_color = EGUI_COLOR_HEX(0x38BDF8);
    local->primary_line_color = EGUI_COLOR_HEX(0x7DD3FC);
    local->secondary_fill_color = EGUI_COLOR_HEX(0xF59E0B);
    local->secondary_line_color = EGUI_COLOR_HEX(0xFBBF24);
    local->grid_color = EGUI_COLOR_HEX(0x334155);
    local->axis_color = EGUI_COLOR_HEX(0x64748B);
    local->text_color = EGUI_COLOR_HEX(0xCBD5E1);
    local->muted_text_color = EGUI_COLOR_HEX(0x94A3B8);
    local->axis_count = 0;
    local->value_set_count = 0;
    local->current_value_set = 0;
    local->grid_levels = 4;
    local->label_padding = 16;
    local->show_compare = 0;
    local->show_axis_labels = 1;
}
