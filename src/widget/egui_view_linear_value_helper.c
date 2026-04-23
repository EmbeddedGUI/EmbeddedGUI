#include "egui_view_linear_value_helper.h"
#include "style/egui_theme.h"

/**
 * @file egui_view_linear_value_helper.c
 * @brief Shared geometry and coordinate conversion helpers for horizontal value controls.
 *
 * Slider, progress bar, and related widgets reuse these helpers so their track
 * thickness, knob placement, and `0..100` conversions stay consistent.
 */

/**
 * @brief Clamp a derived knob radius into the theme's supported size band.
 *
 * Reusing the theme radii keeps slider and progress-bar knobs visually aligned
 * even when the source widget region is unusually short or tall.
 */
static egui_dim_t egui_view_linear_value_clamp_radius(egui_dim_t radius)
{
    if (radius < EGUI_THEME_RADIUS_SM)
    {
        radius = EGUI_THEME_RADIUS_SM;
    }
    if (radius > EGUI_THEME_RADIUS_LG)
    {
        radius = EGUI_THEME_RADIUS_LG;
    }
    return radius;
}

/**
 * @brief Derive track and knob metrics for one horizontal value widget region.
 *
 * `reserve_knob_margin` is the main behavioral switch: sliders reserve room so
 * the thumb can stay fully inside the widget bounds, while progress bars can
 * use the full width when they do not need that margin.
 */
uint8_t egui_view_linear_value_get_metrics(const egui_region_t *region, uint8_t reserve_knob_margin, egui_view_linear_value_metrics_t *metrics)
{
    egui_dim_t track_height;
    egui_dim_t knob_radius;
    egui_dim_t knob_margin;

    if (region == NULL || metrics == NULL || region->size.width <= 0 || region->size.height <= 0)
    {
        return 0;
    }

    // The stock track thickness comes from the theme but is clamped when the widget is shorter than that.
    track_height = EGUI_THEME_TRACK_THICKNESS;
    if (!reserve_knob_margin && track_height > region->size.height)
    {
        track_height = region->size.height;
    }
    if (track_height <= 0)
    {
        return 0;
    }

    knob_radius = egui_view_linear_value_clamp_radius((region->size.height / 2) - 1);
    knob_margin = reserve_knob_margin ? (knob_radius + 1) : knob_radius;

    if (reserve_knob_margin)
    {
        // Reserve one knob radius on both sides so the thumb center can reach both ends without clipping.
        if (region->size.width <= 2 * knob_margin)
        {
            return 0;
        }

        metrics->usable_width = region->size.width - 2 * knob_margin;
        metrics->start_x = region->location.x + knob_margin;
    }
    else
    {
        metrics->usable_width = region->size.width;
        metrics->start_x = region->location.x;
    }

    metrics->track_height = track_height;
    metrics->track_y = region->location.y + (region->size.height - track_height) / 2;
    metrics->track_radius = track_height / 2;
    metrics->knob_radius = knob_radius;
    metrics->knob_margin = knob_margin;
    metrics->center_y = region->location.y + region->size.height / 2;
    return 1;
}

/**
 * @brief Convert a percentage value into the absolute x position on the track.
 *
 * The mapping is inclusive at both ends: `0` lands on `start_x` and `100`
 * lands on the far edge of the usable span.
 */
egui_dim_t egui_view_linear_value_get_x(const egui_view_linear_value_metrics_t *metrics, uint8_t value)
{
    if (metrics == NULL)
    {
        return 0;
    }

    return metrics->start_x + (egui_dim_t)((uint32_t)metrics->usable_width * value / 100);
}

/**
 * @brief Clamp an absolute x coordinate so the knob stays inside the usable span.
 *
 * Callers usually apply this after converting widths or drag positions into a
 * thumb center coordinate.
 */
egui_dim_t egui_view_linear_value_clamp_x(const egui_view_linear_value_metrics_t *metrics, egui_dim_t x)
{
    egui_dim_t min_x;
    egui_dim_t max_x;

    if (metrics == NULL)
    {
        return x;
    }

    min_x = metrics->start_x + metrics->knob_radius;
    max_x = metrics->start_x + metrics->usable_width - metrics->knob_radius;

    if (x < min_x)
    {
        x = min_x;
    }
    if (x > max_x)
    {
        x = max_x;
    }
    return x;
}

/**
 * @brief Convert a local x coordinate back into a `0..100` percentage value.
 *
 * `local_x` is expected to use the widget's local coordinate space, so the
 * helper subtracts the knob margin before projecting into the usable span.
 */
uint8_t egui_view_linear_value_get_value_from_local_x(const egui_view_linear_value_metrics_t *metrics, egui_dim_t local_x)
{
    egui_dim_t offset;

    if (metrics == NULL || metrics->usable_width <= 0)
    {
        return 0;
    }

    offset = local_x - metrics->knob_margin;
    if (offset < 0)
    {
        offset = 0;
    }
    if (offset > metrics->usable_width)
    {
        offset = metrics->usable_width;
    }

    return (uint8_t)((uint32_t)offset * 100 / metrics->usable_width);
}
