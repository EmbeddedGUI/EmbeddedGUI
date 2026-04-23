#include "egui_view_circle_dirty.h"
#include "core/egui_trig_lut.h"

/**
 * @file egui_view_circle_dirty.c
 * @brief Shared dirty-region helpers for circular and radial widgets.
 *
 * Circular widgets rarely need a full redraw
 * when only one arc segment, hand,
 * or marker moves. These helpers provide lightweight bounding boxes that are
 * cheap to compute and good enough for
 * invalidation.
 */

/**
 * @brief Sample the sine lookup table and return a signed Q15 result.
 *
 * The LUT stores the first quadrant only, so the helper mirrors the degree
 *
 * into that range and then restores the final sign.
 */
static int32_t egui_view_circle_dirty_sin_q15(int32_t deg)
{
    deg = ((deg % 360) + 360) % 360;
    int32_t sign = 1;

    if (deg > 180)
    {
        deg = 360 - deg;
        sign = -1;
    }
    if (deg > 90)
    {
        deg = 180 - deg;
    }

    return egui_trig_float_to_q15(egui_trig_sin_lut[deg]) * sign;
}

/** Compute cosine in degrees by reusing the sine helper with a 90 degree shift. */
static int32_t egui_view_circle_dirty_cos_q15(int32_t deg)
{
    return egui_view_circle_dirty_sin_q15(deg + 90);
}

/**
 * @brief Scale one integer radius by a Q15 factor with symmetric rounding.
 *
 * This keeps circle-point coordinates stable when angles move across
 *
 * quadrants instead of always truncating toward zero.
 */
static egui_dim_t egui_view_circle_dirty_scale_q15(egui_dim_t radius, int32_t q15)
{
    int32_t scaled = (int32_t)radius * q15;

    if (scaled >= 0)
    {
        scaled += 1 << 14;
    }
    else
    {
        scaled -= 1 << 14;
    }

    return (egui_dim_t)(scaled >> 15);
}

/** Wrap any signed angle into the canonical degree range used by the helpers. */
int16_t egui_view_circle_dirty_normalize_angle(int32_t angle)
{
    angle %= 360;
    if (angle < 0)
    {
        angle += 360;
    }
    return (int16_t)angle;
}

/**
 * @brief Convert polar circle coordinates into screen coordinates.
 *
 * The angle convention matches the rest of egui's circular widgets: `0`
 * degrees
 * points right and positive angles advance clockwise on screen.
 */
void egui_view_circle_dirty_get_circle_point(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t angle_deg, egui_dim_t *x, egui_dim_t *y)
{
    *x = center_x + egui_view_circle_dirty_scale_q15(radius, egui_view_circle_dirty_cos_q15(angle_deg));
    *y = center_y + egui_view_circle_dirty_scale_q15(radius, egui_view_circle_dirty_sin_q15(angle_deg));
}

/** Grow a sampled-point bounding box, bootstrapping it from the first point. */
static void egui_view_circle_dirty_expand_bounds(int *has_bounds, egui_dim_t x, egui_dim_t y, egui_dim_t *min_x, egui_dim_t *min_y, egui_dim_t *max_x,
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

/**
 * @brief Test whether one axis angle lies inside a clockwise sweep.
 *
 * `compute_arc_region` uses this to decide whether the arc crosses any of the
 *
 * four cardinal extrema that can expand the bounding box.
 */
static uint8_t egui_view_circle_dirty_is_angle_in_sweep(int16_t start_angle, uint16_t sweep, int16_t angle)
{
    uint16_t delta;

    if (sweep >= 360U)
    {
        return 1;
    }

    delta = (uint16_t)egui_view_circle_dirty_normalize_angle((int32_t)angle - start_angle);
    return (delta <= sweep) ? 1 : 0;
}

/**
 * @brief Approximate the dirty region for an arc by sampling its extrema.
 *
 * The box is built from the arc endpoints plus any cardinal directions
 * crossed
 * by the sweep. `expand_radius` lets callers pad for stroke width, thumb size,
 * or anti-aliased edges without duplicating that math at each call
 * site.
 */
uint8_t egui_view_circle_dirty_compute_arc_region(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t mid_radius, egui_dim_t expand_radius,
                                                  int16_t start_angle, uint16_t sweep, egui_region_t *dirty_region)
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

    // A full sweep degenerates to the bounding box of the entire stroked circle.
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

    // The two arc endpoints are always part of the final bounds.
    egui_view_circle_dirty_get_circle_point(center_x, center_y, mid_radius, start_norm, &point_x, &point_y);
    egui_view_circle_dirty_expand_bounds(&has_bounds, point_x, point_y, &min_x, &min_y, &max_x, &max_y);

    egui_view_circle_dirty_get_circle_point(center_x, center_y, mid_radius, egui_view_circle_dirty_normalize_angle(end_angle), &point_x, &point_y);
    egui_view_circle_dirty_expand_bounds(&has_bounds, point_x, point_y, &min_x, &min_y, &max_x, &max_y);

    // Cardinal angles contribute local extrema on x or y and must be included when crossed.
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

/** Merge one non-empty region into the accumulated dirty box. */
void egui_view_circle_dirty_union_region(egui_region_t *dirty_region, const egui_region_t *other)
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

/** Add the padded bounding box of one circle to the accumulated dirty region. */
void egui_view_circle_dirty_add_circle_region(egui_region_t *dirty_region, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t pad)
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

/**
 * @brief Add the padded bounding box of a stroked line segment.
 *
 * This is intentionally axis-aligned because it is used for invalidation, not
 * for
 * exact hit testing or rasterization.
 */
void egui_view_circle_dirty_add_line_region(egui_region_t *dirty_region, egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width,
                                            egui_dim_t pad)
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

/** Add a rectangle plus extra padding to the accumulated dirty region. */
void egui_view_circle_dirty_add_rect_region(egui_region_t *dirty_region, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t pad)
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
