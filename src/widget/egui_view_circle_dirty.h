#ifndef _EGUI_VIEW_CIRCLE_DIRTY_H_
#define _EGUI_VIEW_CIRCLE_DIRTY_H_

#include "egui_view.h"
#include "utils/egui_fixmath.h"

#define EGUI_VIEW_CIRCLE_DIRTY_AA_PAD 2

__EGUI_STATIC_INLINE__ int16_t egui_view_circle_dirty_normalize_angle(int32_t angle)
{
    angle %= 360;
    if (angle < 0)
    {
        angle += 360;
    }
    return (int16_t)angle;
}

__EGUI_STATIC_INLINE__ void egui_view_circle_dirty_get_circle_point(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t angle_deg,
                                                                    egui_dim_t *x, egui_dim_t *y)
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

__EGUI_STATIC_INLINE__ void egui_view_circle_dirty_expand_bounds(int *has_bounds, egui_dim_t x, egui_dim_t y, egui_dim_t *min_x, egui_dim_t *min_y,
                                                                 egui_dim_t *max_x, egui_dim_t *max_y)
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

__EGUI_STATIC_INLINE__ uint8_t egui_view_circle_dirty_is_angle_in_sweep(int16_t start_angle, uint16_t sweep, int16_t angle)
{
    uint16_t delta;

    if (sweep >= 360U)
    {
        return 1;
    }

    delta = (uint16_t)egui_view_circle_dirty_normalize_angle((int32_t)angle - start_angle);
    return (delta <= sweep) ? 1 : 0;
}

__EGUI_STATIC_INLINE__ uint8_t egui_view_circle_dirty_compute_arc_region(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t mid_radius,
                                                                         egui_dim_t expand_radius, int16_t start_angle, uint16_t sweep,
                                                                         egui_region_t *dirty_region)
{
    static const int16_t critical_angles[] = {0, 90, 180, 270};

    egui_dim_t min_x;
    egui_dim_t min_y;
    egui_dim_t max_x;
    egui_dim_t max_y;
    egui_dim_t point_x;
    egui_dim_t point_y;
    int16_t end_angle;
    int16_t start_norm;
    uint8_t i;
    int has_bounds;

    if (dirty_region == NULL)
    {
        return 0;
    }

    egui_region_init_empty(dirty_region);

    if (mid_radius < 0 || expand_radius < 0 || sweep == 0U)
    {
        return 0;
    }

    if (sweep >= 360U)
    {
        dirty_region->location.x = center_x - mid_radius - expand_radius;
        dirty_region->location.y = center_y - mid_radius - expand_radius;
        dirty_region->size.width = mid_radius * 2 + expand_radius * 2 + 1;
        dirty_region->size.height = mid_radius * 2 + expand_radius * 2 + 1;
        return egui_region_is_empty(dirty_region) ? 0 : 1;
    }

    has_bounds = 0;
    end_angle = start_angle + (int16_t)sweep;
    start_norm = egui_view_circle_dirty_normalize_angle(start_angle);

    egui_view_circle_dirty_get_circle_point(center_x, center_y, mid_radius, start_norm, &point_x, &point_y);
    egui_view_circle_dirty_expand_bounds(&has_bounds, point_x, point_y, &min_x, &min_y, &max_x, &max_y);

    egui_view_circle_dirty_get_circle_point(center_x, center_y, mid_radius, egui_view_circle_dirty_normalize_angle(end_angle), &point_x, &point_y);
    egui_view_circle_dirty_expand_bounds(&has_bounds, point_x, point_y, &min_x, &min_y, &max_x, &max_y);

    for (i = 0; i < EGUI_ARRAY_SIZE(critical_angles); i++)
    {
        if (!egui_view_circle_dirty_is_angle_in_sweep(start_norm, sweep, critical_angles[i]))
        {
            continue;
        }

        egui_view_circle_dirty_get_circle_point(center_x, center_y, mid_radius, critical_angles[i], &point_x, &point_y);
        egui_view_circle_dirty_expand_bounds(&has_bounds, point_x, point_y, &min_x, &min_y, &max_x, &max_y);
    }

    if (!has_bounds)
    {
        return 0;
    }

    dirty_region->location.x = min_x - expand_radius;
    dirty_region->location.y = min_y - expand_radius;
    dirty_region->size.width = max_x - min_x + expand_radius * 2 + 1;
    dirty_region->size.height = max_y - min_y + expand_radius * 2 + 1;

    return egui_region_is_empty(dirty_region) ? 0 : 1;
}

__EGUI_STATIC_INLINE__ void egui_view_circle_dirty_union_region(egui_region_t *dirty_region, const egui_region_t *other)
{
    if (dirty_region == NULL || other == NULL || egui_region_is_empty((egui_region_t *)other))
    {
        return;
    }

    if (egui_region_is_empty(dirty_region))
    {
        egui_region_copy(dirty_region, other);
        return;
    }

    egui_region_union(dirty_region, other, dirty_region);
}

__EGUI_STATIC_INLINE__ void egui_view_circle_dirty_add_circle_region(egui_region_t *dirty_region, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius,
                                                                     egui_dim_t pad)
{
    egui_region_t region;

    if (dirty_region == NULL || radius < 0)
    {
        return;
    }

    region.location.x = center_x - radius - pad;
    region.location.y = center_y - radius - pad;
    region.size.width = radius * 2 + pad * 2 + 1;
    region.size.height = radius * 2 + pad * 2 + 1;

    egui_view_circle_dirty_union_region(dirty_region, &region);
}

__EGUI_STATIC_INLINE__ void egui_view_circle_dirty_add_line_region(egui_region_t *dirty_region, egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2,
                                                                   egui_dim_t stroke_width, egui_dim_t pad)
{
    egui_region_t region;
    egui_dim_t expand;
    egui_dim_t min_x;
    egui_dim_t min_y;
    egui_dim_t max_x;
    egui_dim_t max_y;

    if (dirty_region == NULL)
    {
        return;
    }

    expand = (stroke_width + 1) / 2 + pad;
    min_x = EGUI_MIN(x1, x2) - expand;
    min_y = EGUI_MIN(y1, y2) - expand;
    max_x = EGUI_MAX(x1, x2) + expand;
    max_y = EGUI_MAX(y1, y2) + expand;

    region.location.x = min_x;
    region.location.y = min_y;
    region.size.width = max_x - min_x + 1;
    region.size.height = max_y - min_y + 1;

    egui_view_circle_dirty_union_region(dirty_region, &region);
}

__EGUI_STATIC_INLINE__ void egui_view_circle_dirty_add_rect_region(egui_region_t *dirty_region, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height,
                                                                   egui_dim_t pad)
{
    egui_region_t region;

    if (dirty_region == NULL || width <= 0 || height <= 0)
    {
        return;
    }

    region.location.x = x - pad;
    region.location.y = y - pad;
    region.size.width = width + pad * 2;
    region.size.height = height + pad * 2;

    egui_view_circle_dirty_union_region(dirty_region, &region);
}

#endif /* _EGUI_VIEW_CIRCLE_DIRTY_H_ */
