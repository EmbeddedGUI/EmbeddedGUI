#include <stdio.h>
#include <assert.h>

#include "egui_view_activity_ring.h"
#include "utils/egui_fixmath.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

#define EGUI_VIEW_ACTIVITY_RING_DIRTY_AA_PAD 2

static int16_t egui_view_activity_ring_normalize_angle(int32_t angle)
{
    angle %= 360;
    if (angle < 0)
    {
        angle += 360;
    }
    return (int16_t)angle;
}

static void egui_view_activity_ring_get_circle_point(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t angle_deg, egui_dim_t *x,
                                                     egui_dim_t *y)
{
    egui_float_t angle_rad;
    egui_float_t cos_val;
    egui_float_t sin_val;

    angle_rad = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(angle_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));
    cos_val = EGUI_FLOAT_COS(angle_rad);
    sin_val = EGUI_FLOAT_SIN(angle_rad);

    *x = center_x + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(radius), cos_val));
    *y = center_y + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(radius), sin_val));
}

static void egui_view_activity_ring_expand_bounds(int *has_bounds, egui_dim_t x, egui_dim_t y, egui_dim_t *min_x, egui_dim_t *min_y, egui_dim_t *max_x,
                                                  egui_dim_t *max_y)
{
    if (!(*has_bounds))
    {
        *min_x = x;
        *min_y = y;
        *max_x = x;
        *max_y = y;
        *has_bounds = 1;
        return;
    }

    if (x < *min_x)
    {
        *min_x = x;
    }
    if (x > *max_x)
    {
        *max_x = x;
    }
    if (y < *min_y)
    {
        *min_y = y;
    }
    if (y > *max_y)
    {
        *max_y = y;
    }
}

static uint8_t egui_view_activity_ring_is_angle_in_sweep(int16_t start_angle, uint16_t sweep, int16_t angle)
{
    uint16_t delta;

    if (sweep >= 360U)
    {
        return 1;
    }

    delta = (uint16_t)egui_view_activity_ring_normalize_angle((int32_t)angle - start_angle);
    return (delta <= sweep) ? 1 : 0;
}

static uint8_t egui_view_activity_ring_get_ring_geometry(egui_view_t *self, egui_view_activity_ring_t *local, uint8_t ring_index, egui_dim_t *center_x,
                                                         egui_dim_t *center_y, egui_dim_t *mid_radius, egui_dim_t *expand_radius)
{
    egui_region_t region;
    egui_dim_t w;
    egui_dim_t h;
    egui_dim_t outer_radius;
    egui_dim_t cur_radius;

    if (ring_index >= local->ring_count)
    {
        return 0;
    }

    egui_view_get_work_region(self, &region);
    w = region.size.width;
    h = region.size.height;
    outer_radius = EGUI_MIN(w, h) / 2 - local->stroke_width / 2 - 1;
    if (outer_radius <= 0)
    {
        return 0;
    }

    cur_radius = outer_radius - ring_index * (local->stroke_width + local->ring_gap);
    if (cur_radius <= 0)
    {
        return 0;
    }

    *center_x = region.location.x + w / 2;
    *center_y = region.location.y + h / 2;
    *mid_radius = cur_radius - local->stroke_width / 2;
    if (*mid_radius < 0)
    {
        *mid_radius = 0;
    }
    *expand_radius = local->stroke_width / 2 + EGUI_VIEW_ACTIVITY_RING_DIRTY_AA_PAD;

    return 1;
}

static uint8_t egui_view_activity_ring_get_value_dirty_region(egui_view_t *self, egui_view_activity_ring_t *local, uint8_t ring_index, uint8_t old_value,
                                                              uint8_t new_value, egui_region_t *dirty_region)
{
    static const int16_t critical_angles[] = {0, 90, 180, 270};

    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t mid_radius;
    egui_dim_t expand_radius;
    egui_dim_t min_x;
    egui_dim_t min_y;
    egui_dim_t max_x;
    egui_dim_t max_y;
    egui_dim_t point_x;
    egui_dim_t point_y;
    uint16_t old_sweep;
    uint16_t new_sweep;
    uint16_t sweep;
    int16_t dirty_start_angle;
    int16_t dirty_start_norm;
    int16_t dirty_end_angle;
    uint8_t i;
    int has_bounds;

    if (dirty_region == NULL)
    {
        return 0;
    }

    egui_region_init_empty(dirty_region);

    if (!egui_view_activity_ring_get_ring_geometry(self, local, ring_index, &center_x, &center_y, &mid_radius, &expand_radius))
    {
        return 0;
    }

    old_sweep = (uint16_t)((uint32_t)old_value * 360U / 100U);
    new_sweep = (uint16_t)((uint32_t)new_value * 360U / 100U);
    if (old_sweep == new_sweep)
    {
        return 0;
    }

    dirty_start_angle = local->start_angle + (old_sweep < new_sweep ? (int16_t)old_sweep : (int16_t)new_sweep);
    dirty_end_angle = local->start_angle + (old_sweep > new_sweep ? (int16_t)old_sweep : (int16_t)new_sweep);
    sweep = (uint16_t)(dirty_end_angle - dirty_start_angle);

    if (sweep >= 360U)
    {
        min_x = center_x - mid_radius - expand_radius;
        min_y = center_y - mid_radius - expand_radius;
        max_x = center_x + mid_radius + expand_radius;
        max_y = center_y + mid_radius + expand_radius;
    }
    else
    {
        has_bounds = 0;
        dirty_start_norm = egui_view_activity_ring_normalize_angle(dirty_start_angle);

        egui_view_activity_ring_get_circle_point(center_x, center_y, mid_radius, dirty_start_norm, &point_x, &point_y);
        egui_view_activity_ring_expand_bounds(&has_bounds, point_x, point_y, &min_x, &min_y, &max_x, &max_y);

        egui_view_activity_ring_get_circle_point(center_x, center_y, mid_radius, egui_view_activity_ring_normalize_angle(dirty_end_angle), &point_x, &point_y);
        egui_view_activity_ring_expand_bounds(&has_bounds, point_x, point_y, &min_x, &min_y, &max_x, &max_y);

        for (i = 0; i < EGUI_ARRAY_SIZE(critical_angles); i++)
        {
            if (!egui_view_activity_ring_is_angle_in_sweep(dirty_start_norm, sweep, critical_angles[i]))
            {
                continue;
            }

            egui_view_activity_ring_get_circle_point(center_x, center_y, mid_radius, critical_angles[i], &point_x, &point_y);
            egui_view_activity_ring_expand_bounds(&has_bounds, point_x, point_y, &min_x, &min_y, &max_x, &max_y);
        }

        if (!has_bounds)
        {
            return 0;
        }

        min_x -= expand_radius;
        min_y -= expand_radius;
        max_x += expand_radius;
        max_y += expand_radius;
    }

    dirty_region->location.x = min_x;
    dirty_region->location.y = min_y;
    dirty_region->size.width = max_x - min_x + 1;
    dirty_region->size.height = max_y - min_y + 1;

    return egui_region_is_empty(dirty_region) ? 0 : 1;
}

void egui_view_activity_ring_set_value(egui_view_t *self, uint8_t ring_index, uint8_t value)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    uint8_t old_value;
    egui_region_t dirty_region;

    if (ring_index >= EGUI_VIEW_ACTIVITY_RING_MAX_RINGS)
    {
        return;
    }
    if (value > 100)
    {
        value = 100;
    }
    if (value != local->values[ring_index])
    {
        old_value = local->values[ring_index];
        local->values[ring_index] = value;
        if (ring_index >= local->ring_count)
        {
            return;
        }

        if (self->region_screen.size.width <= 0 || self->region_screen.size.height <= 0)
        {
            egui_view_invalidate(self);
            return;
        }

        if (egui_view_has_pending_dirty(self))
        {
            egui_view_invalidate_full(self);
            return;
        }

        if (egui_view_activity_ring_get_value_dirty_region(self, local, ring_index, old_value, value, &dirty_region))
        {
            egui_view_invalidate_region(self, &dirty_region);
        }
        else
        {
            egui_view_invalidate(self);
        }
    }
}

uint8_t egui_view_activity_ring_get_value(egui_view_t *self, uint8_t ring_index)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (ring_index >= EGUI_VIEW_ACTIVITY_RING_MAX_RINGS)
    {
        return 0;
    }
    return local->values[ring_index];
}

void egui_view_activity_ring_set_ring_count(egui_view_t *self, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (count > EGUI_VIEW_ACTIVITY_RING_MAX_RINGS)
    {
        count = EGUI_VIEW_ACTIVITY_RING_MAX_RINGS;
    }
    if (count != local->ring_count)
    {
        local->ring_count = count;
        egui_view_invalidate(self);
    }
}

void egui_view_activity_ring_set_ring_color(egui_view_t *self, uint8_t ring_index, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (ring_index >= EGUI_VIEW_ACTIVITY_RING_MAX_RINGS)
    {
        return;
    }
    local->ring_colors[ring_index] = color;
    egui_view_invalidate(self);
}

void egui_view_activity_ring_set_ring_bg_color(egui_view_t *self, uint8_t ring_index, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (ring_index >= EGUI_VIEW_ACTIVITY_RING_MAX_RINGS)
    {
        return;
    }
    local->ring_bg_colors[ring_index] = color;
    egui_view_invalidate(self);
}

void egui_view_activity_ring_set_stroke_width(egui_view_t *self, egui_dim_t stroke_width)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (stroke_width != local->stroke_width)
    {
        local->stroke_width = stroke_width;
        egui_view_invalidate(self);
    }
}

void egui_view_activity_ring_set_ring_gap(egui_view_t *self, egui_dim_t ring_gap)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (ring_gap != local->ring_gap)
    {
        local->ring_gap = ring_gap;
        egui_view_invalidate(self);
    }
}

void egui_view_activity_ring_set_start_angle(egui_view_t *self, int16_t start_angle)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (start_angle != local->start_angle)
    {
        local->start_angle = start_angle;
        egui_view_invalidate(self);
    }
}

void egui_view_activity_ring_set_show_round_cap(egui_view_t *self, uint8_t show)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);
    if (show != local->show_round_cap)
    {
        local->show_round_cap = show;
        egui_view_invalidate(self);
    }
}

void egui_view_activity_ring_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_activity_ring_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;
    egui_dim_t center_x = region.location.x + w / 2;
    egui_dim_t center_y = region.location.y + h / 2;
    egui_dim_t outer_radius = EGUI_MIN(w, h) / 2 - local->stroke_width / 2 - 1;

    if (outer_radius <= 0)
    {
        return;
    }

    uint8_t i;
    for (i = 0; i < local->ring_count; i++)
    {
        egui_dim_t cur_radius = outer_radius - i * (local->stroke_width + local->ring_gap);
        int16_t draw_start_angle = egui_view_activity_ring_normalize_angle(local->start_angle);
        if (cur_radius <= 0)
        {
            break;
        }

        egui_dim_t inner_r = cur_radius - local->stroke_width;
        if (inner_r < 0)
            inner_r = 0;

        // Background arc (full 360)
        // In Enhanced mode use ring fill so geometry matches the progress ring fill below
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_gradient_stop_t bg_stops[2] = {
                    {.position = 0, .color = local->ring_bg_colors[i]},
                    {.position = 255, .color = local->ring_bg_colors[i]},
            };
            egui_gradient_t bg_grad = {
                    .type = EGUI_GRADIENT_TYPE_RADIAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = bg_stops,
            };
            egui_canvas_draw_arc_ring_fill_gradient(center_x, center_y, cur_radius, inner_r, 0, 360, &bg_grad);
        }
#else
        egui_canvas_draw_arc(center_x, center_y, cur_radius, 0, 360, local->stroke_width, local->ring_bg_colors[i], EGUI_ALPHA_100);
#endif

        // Progress arc
        if (local->values[i] > 0)
        {
            int16_t draw_end_angle = draw_start_angle + (int16_t)((uint32_t)local->values[i] * 360 / 100);
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
            {
                egui_color_t color_light = egui_rgb_mix(local->ring_colors[i], EGUI_COLOR_WHITE, 80);
                egui_gradient_stop_t stops[2] = {
                        {.position = 0, .color = color_light},
                        {.position = 255, .color = local->ring_colors[i]},
                };
                egui_gradient_t grad = {
                        .type = EGUI_GRADIENT_TYPE_RADIAL,
                        .stop_count = 2,
                        .alpha = EGUI_ALPHA_100,
                        .stops = stops,
                };
                egui_canvas_draw_arc_ring_fill_gradient_round_cap(center_x, center_y, cur_radius, inner_r, draw_start_angle, draw_end_angle, &grad,
                                                                  local->show_round_cap ? EGUI_ARC_CAP_BOTH : EGUI_ARC_CAP_NONE);
            }
#else
            {
                if (local->show_round_cap)
                {
                    egui_canvas_draw_arc_round_cap_hq(center_x, center_y, cur_radius, draw_start_angle, draw_end_angle, local->stroke_width,
                                                      local->ring_colors[i], EGUI_ALPHA_100);
                }
                else
                {
                    egui_canvas_draw_arc(center_x, center_y, cur_radius, draw_start_angle, draw_end_angle, local->stroke_width, local->ring_colors[i],
                                         EGUI_ALPHA_100);
                }
            }
#endif
        }
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_activity_ring_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_activity_ring_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_activity_ring_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_activity_ring_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_activity_ring_t);

    // init local data.
    local->ring_count = 3;
    local->stroke_width = 8;
    local->ring_gap = 4;
    local->start_angle = -90;
    local->show_round_cap = 1;

    local->values[0] = 0;
    local->values[1] = 0;
    local->values[2] = 0;

    local->ring_colors[0] = EGUI_THEME_DANGER;
    local->ring_colors[1] = EGUI_THEME_SUCCESS;
    local->ring_colors[2] = EGUI_THEME_PRIMARY;

    local->ring_bg_colors[0] = EGUI_THEME_TRACK_BG;
    local->ring_bg_colors[1] = EGUI_THEME_TRACK_BG;
    local->ring_bg_colors[2] = EGUI_THEME_TRACK_BG;

    egui_view_set_view_name(self, "egui_view_activity_ring");
}

void egui_view_activity_ring_apply_params(egui_view_t *self, const egui_view_activity_ring_params_t *params)
{
    self->region = params->region;
    egui_view_invalidate(self);
}

void egui_view_activity_ring_init_with_params(egui_view_t *self, const egui_view_activity_ring_params_t *params)
{
    egui_view_activity_ring_init(self);
    egui_view_activity_ring_apply_params(self, params);
}
