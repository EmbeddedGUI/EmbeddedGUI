#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_chart_common.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#include "egui_view_group.h"
#endif

// ============== Axis Base Init ==============

void egui_chart_axis_base_init_defaults(egui_chart_axis_base_t *ab)
{
    ab->series = NULL;
    ab->series_count = 0;

    // default axis X
    ab->axis_x.min_value = 0;
    ab->axis_x.max_value = 100;
    ab->axis_x.tick_step = 0;
    ab->axis_x.tick_count = 5;
    ab->axis_x.show_grid = 1;
    ab->axis_x.show_labels = 1;
    ab->axis_x.show_axis = 1;
    ab->axis_x.title = NULL;

    // default axis Y
    ab->axis_y.min_value = 0;
    ab->axis_y.max_value = 100;
    ab->axis_y.tick_step = 0;
    ab->axis_y.tick_count = 5;
    ab->axis_y.show_grid = 1;
    ab->axis_y.show_labels = 1;
    ab->axis_y.show_axis = 1;
    ab->axis_y.title = NULL;

    // legend
    ab->legend_pos = EGUI_CHART_LEGEND_NONE;

    // style
    ab->bg_color = EGUI_THEME_SURFACE;
    ab->axis_color = EGUI_THEME_TEXT_PRIMARY;
    ab->grid_color = EGUI_THEME_BORDER;
    ab->text_color = EGUI_THEME_TEXT_SECONDARY;
    ab->font = NULL;

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    ab->zoom_enabled = 0;
    ab->is_panning = 0;
    ab->is_pinching = 0;
    ab->view_x_min = ab->axis_x.min_value;
    ab->view_x_max = ab->axis_x.max_value;
    ab->view_y_min = ab->axis_y.min_value;
    ab->view_y_max = ab->axis_y.max_value;
    ab->pinch_start_dist = 0;
    ab->pinch_start_dx_abs = 0;
    ab->pinch_start_dy_abs = 0;
#endif
}

// ============== Internal Helpers ==============

void egui_chart_int_to_str(int16_t value, char *buf, int buf_size)
{
    int pos = 0;
    int16_t v = value;
    char tmp[8];
    int tmp_len = 0;

    if (buf_size < 2)
    {
        if (buf_size > 0)
        {
            buf[0] = '\0';
        }
        return;
    }

    if (v < 0)
    {
        buf[pos++] = '-';
        v = -v;
    }

    // Extract digits in reverse
    if (v == 0)
    {
        tmp[tmp_len++] = '0';
    }
    else
    {
        while (v > 0 && tmp_len < (int)sizeof(tmp))
        {
            tmp[tmp_len++] = '0' + (v % 10);
            v /= 10;
        }
    }

    // Reverse copy to buf
    for (int i = tmp_len - 1; i >= 0 && pos < buf_size - 1; i--)
    {
        buf[pos++] = tmp[i];
    }
    buf[pos] = '\0';
}

egui_dim_t egui_chart_get_font_height(const egui_font_t *font)
{
    if (font == NULL)
    {
        return 8; // fallback
    }
    return EGUI_FONT_STD_GET_FONT_HEIGHT(font);
}

static uint8_t egui_chart_count_int_chars(int16_t value)
{
    uint8_t chars = 0;
    int16_t v = value;
    if (v <= 0)
    {
        chars++;
        v = -v;
    }
    while (v > 0)
    {
        chars++;
        v /= 10;
    }
    return chars;
}

static egui_dim_t egui_chart_get_y_label_width(egui_chart_axis_base_t *ab, egui_dim_t font_h)
{
    uint8_t min_chars = egui_chart_count_int_chars(ab->axis_y.min_value);
    uint8_t max_chars = egui_chart_count_int_chars(ab->axis_y.max_value);
    uint8_t chars = (min_chars > max_chars) ? min_chars : max_chars;
    return (egui_dim_t)((font_h * chars + 1) / 2 + 6);
}

// ============== Viewport Helpers ==============

void egui_chart_get_view_x(egui_chart_axis_base_t *ab, int16_t *out_min, int16_t *out_max)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    if (ab->zoom_enabled)
    {
        *out_min = ab->view_x_min;
        *out_max = ab->view_x_max;
        return;
    }
#endif
    *out_min = ab->axis_x.min_value;
    *out_max = ab->axis_x.max_value;
}

void egui_chart_get_view_y(egui_chart_axis_base_t *ab, int16_t *out_min, int16_t *out_max)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    if (ab->zoom_enabled)
    {
        *out_min = ab->view_y_min;
        *out_max = ab->view_y_max;
        return;
    }
#endif
    *out_min = ab->axis_y.min_value;
    *out_max = ab->axis_y.max_value;
}

// ============== Coordinate Mapping ==============

egui_dim_t egui_chart_map_x(egui_chart_axis_base_t *ab, int16_t data_x, egui_dim_t plot_x, egui_dim_t plot_w)
{
    int16_t min_val, max_val;
    egui_chart_get_view_x(ab, &min_val, &max_val);
    int32_t range = (int32_t)max_val - (int32_t)min_val;
    if (range <= 0)
    {
        return plot_x;
    }
    int32_t offset = (int32_t)data_x - (int32_t)min_val;
    // Use (plot_w - 1) as pixel range: min_val maps to plot_x, max_val maps to plot_x + plot_w - 1
    return plot_x + (egui_dim_t)(offset * (int32_t)(plot_w - 1) / range);
}

egui_dim_t egui_chart_map_y(egui_chart_axis_base_t *ab, int16_t data_y, egui_dim_t plot_y, egui_dim_t plot_h)
{
    int16_t min_val, max_val;
    egui_chart_get_view_y(ab, &min_val, &max_val);
    int32_t range = (int32_t)max_val - (int32_t)min_val;
    if (range <= 0)
    {
        return plot_y;
    }
    int32_t offset = (int32_t)data_y - (int32_t)min_val;
    // Use (plot_h - 1) as pixel range: min_val maps to plot_y + plot_h - 1, max_val maps to plot_y
    return plot_y + (plot_h - 1) - (egui_dim_t)(offset * (int32_t)(plot_h - 1) / range);
}

// ============== Plot Area Calculation ==============

void egui_chart_calc_plot_area(egui_chart_axis_base_t *ab, egui_region_t *region, egui_region_t *plot_area)
{
    const egui_font_t *font = ab->font ? ab->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t font_h = egui_chart_get_font_height(font);

    egui_dim_t margin_left = 0;
    egui_dim_t margin_bottom = 0;
    egui_dim_t margin_top = 2;
    egui_dim_t margin_right = 2;

    // Y axis labels on the left
    if (ab->axis_y.show_labels)
    {
        margin_left = egui_chart_get_y_label_width(ab, font_h) + 2;
    }
    else if (ab->axis_y.show_axis)
    {
        margin_left = 2;
    }

    // X axis labels at the bottom
    if (ab->axis_x.show_labels)
    {
        margin_bottom = font_h + 4; // font height + tick mark height
        // Reserve right margin for last X label that extends beyond plot area
        if (margin_right < font_h)
        {
            margin_right = font_h;
        }
    }
    else if (ab->axis_x.show_axis)
    {
        margin_bottom = 2;
    }

    // Legend space
    if (ab->legend_pos == EGUI_CHART_LEGEND_TOP)
    {
        margin_top += font_h + 4;
    }
    else if (ab->legend_pos == EGUI_CHART_LEGEND_BOTTOM)
    {
        margin_bottom += font_h + 4;
    }
    else if (ab->legend_pos == EGUI_CHART_LEGEND_RIGHT)
    {
        margin_right += font_h * 4; // approximate width for legend items
    }

    plot_area->location.x = region->location.x + margin_left;
    plot_area->location.y = region->location.y + margin_top;
    plot_area->size.width = region->size.width - margin_left - margin_right;
    plot_area->size.height = region->size.height - margin_top - margin_bottom;

    if (plot_area->size.width < 10)
    {
        plot_area->size.width = 10;
    }
    if (plot_area->size.height < 10)
    {
        plot_area->size.height = 10;
    }
}

// ============== Axis Drawing ==============

void egui_chart_draw_axes(egui_chart_axis_base_t *ab, egui_region_t *region, egui_region_t *plot_area)
{
    const egui_font_t *font = ab->font ? ab->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t font_h = egui_chart_get_font_height(font);

    // Get viewport range for tick iteration
    int16_t view_x_min, view_x_max, view_y_min, view_y_max;
    egui_chart_get_view_x(ab, &view_x_min, &view_x_max);
    egui_chart_get_view_y(ab, &view_y_min, &view_y_max);

    // Draw X axis line
    if (ab->axis_x.show_axis)
    {
        egui_canvas_draw_hline(plot_area->location.x, plot_area->location.y + plot_area->size.height, plot_area->size.width, ab->axis_color, EGUI_ALPHA_100);
    }

    // Draw Y axis line
    if (ab->axis_y.show_axis)
    {
        egui_canvas_draw_vline(plot_area->location.x, plot_area->location.y, plot_area->size.height, ab->axis_color, EGUI_ALPHA_100);
    }

    // X axis ticks, grid, labels
    if (ab->axis_x.show_labels || ab->axis_x.show_grid)
    {
        char label_buf[8];

        if (ab->axis_x.is_categorical && ab->series_count > 0 && ab->series[0].point_count > 0)
        {
            // Categorical mode: divide plot into equal slots, labels centered on each slot
            uint8_t n_cat = ab->series[0].point_count;
            egui_dim_t slot_w = plot_area->size.width / n_cat;

            for (uint8_t i = 0; i < n_cat; i++)
            {
                egui_dim_t slot_center = plot_area->location.x + slot_w * i + slot_w / 2;

                // Tick mark at slot center
                if (ab->axis_x.show_axis)
                {
                    egui_canvas_draw_vline(slot_center, plot_area->location.y + plot_area->size.height, 3, ab->axis_color, EGUI_ALPHA_100);
                }

                // Grid line between slots (at slot left boundary, skip first)
                if (ab->axis_x.show_grid && i > 0)
                {
                    egui_dim_t boundary = plot_area->location.x + slot_w * i;
                    egui_canvas_draw_vline(boundary, plot_area->location.y, plot_area->size.height, ab->grid_color, EGUI_ALPHA_30);
                }

                // Label (use data point x value)
                if (ab->axis_x.show_labels)
                {
                    egui_chart_int_to_str(ab->series[0].points[i].x, label_buf, sizeof(label_buf));
                    EGUI_REGION_DEFINE(label_rect, slot_center - 12, plot_area->location.y + plot_area->size.height + 3, 24, font_h);
                    egui_canvas_draw_text_in_rect(font, label_buf, &label_rect, EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP, ab->text_color, EGUI_ALPHA_100);
                }
            }
        }
        else
        {
            // Continuous mode: ticks at regular value intervals
            int16_t step = ab->axis_x.tick_step;
            if (step <= 0)
            {
                int16_t range = view_x_max - view_x_min;
                uint8_t count = ab->axis_x.tick_count > 0 ? ab->axis_x.tick_count : 5;
                step = range / count;
                if (step <= 0)
                {
                    step = 1;
                }
            }

            // Align start to tick step boundary
            int16_t start_v = ((view_x_min + step - 1) / step) * step;
            if (view_x_min <= 0 && start_v > view_x_min)
            {
                start_v = (view_x_min / step) * step;
                if (start_v < view_x_min)
                {
                    start_v += step;
                }
            }

            for (int16_t v = start_v; v <= view_x_max; v += step)
            {
                egui_dim_t px = egui_chart_map_x(ab, v, plot_area->location.x, plot_area->size.width);

                // Tick mark
                if (ab->axis_x.show_axis)
                {
                    egui_canvas_draw_vline(px, plot_area->location.y + plot_area->size.height, 3, ab->axis_color, EGUI_ALPHA_100);
                }

                // Grid line
                if (ab->axis_x.show_grid)
                {
                    egui_canvas_draw_vline(px, plot_area->location.y, plot_area->size.height, ab->grid_color, EGUI_ALPHA_30);
                }

                // Label
                if (ab->axis_x.show_labels)
                {
                    egui_chart_int_to_str(v, label_buf, sizeof(label_buf));
                    EGUI_REGION_DEFINE(label_rect, px - 12, plot_area->location.y + plot_area->size.height + 3, 24, font_h);
                    egui_canvas_draw_text_in_rect(font, label_buf, &label_rect, EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP, ab->text_color, EGUI_ALPHA_100);
                }
            }
        }
    }

    // Y axis ticks, grid, labels
    if (ab->axis_y.show_labels || ab->axis_y.show_grid)
    {
        int16_t step = ab->axis_y.tick_step;
        if (step <= 0)
        {
            int16_t range = view_y_max - view_y_min;
            uint8_t count = ab->axis_y.tick_count > 0 ? ab->axis_y.tick_count : 5;
            step = range / count;
            if (step <= 0)
            {
                step = 1;
            }
        }

        // Align start to tick step boundary
        int16_t start_v = ((view_y_min + step - 1) / step) * step;
        if (view_y_min <= 0 && start_v > view_y_min)
        {
            start_v = (view_y_min / step) * step;
            if (start_v < view_y_min)
            {
                start_v += step;
            }
        }

        char label_buf[8];
        for (int16_t v = start_v; v <= view_y_max; v += step)
        {
            egui_dim_t py = egui_chart_map_y(ab, v, plot_area->location.y, plot_area->size.height);

            // Tick mark
            if (ab->axis_y.show_axis)
            {
                egui_canvas_draw_hline(plot_area->location.x - 3, py, 3, ab->axis_color, EGUI_ALPHA_100);
            }

            // Grid line
            if (ab->axis_y.show_grid)
            {
                egui_canvas_draw_hline(plot_area->location.x, py, plot_area->size.width, ab->grid_color, EGUI_ALPHA_30);
            }

            // Label
            if (ab->axis_y.show_labels)
            {
                egui_chart_int_to_str(v, label_buf, sizeof(label_buf));
                egui_dim_t label_w = egui_chart_get_y_label_width(ab, font_h);
                egui_dim_t label_y = py - font_h / 2;
                if (label_y < region->location.y)
                {
                    label_y = region->location.y;
                }
                if (label_y + font_h > region->location.y + region->size.height)
                {
                    label_y = region->location.y + region->size.height - font_h;
                }
                EGUI_REGION_DEFINE(label_rect, plot_area->location.x - label_w - 4, label_y, label_w, font_h);
                egui_canvas_draw_text_in_rect(font, label_buf, &label_rect, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, ab->text_color, EGUI_ALPHA_100);
            }
        }
    }
}

// ============== Legend Drawing (series) ==============

void egui_chart_draw_legend_series(egui_chart_axis_base_t *ab, egui_region_t *region, egui_region_t *plot_area)
{
    const egui_font_t *font = ab->font ? ab->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t font_h = egui_chart_get_font_height(font);
    egui_dim_t swatch_size = font_h > 2 ? font_h - 2 : 4;
    egui_dim_t item_gap = 6;
    egui_dim_t text_w = font_h * 3;

    uint8_t count = ab->series_count;
    if (count == 0)
    {
        return;
    }

    // Calculate legend starting position
    egui_dim_t lx, ly;
    if (ab->legend_pos == EGUI_CHART_LEGEND_BOTTOM)
    {
        egui_dim_t total_w = count * (swatch_size + 2 + text_w + item_gap) - item_gap;
        lx = region->location.x + (region->size.width - total_w) / 2;
        if (lx < region->location.x)
        {
            lx = region->location.x;
        }
        ly = region->location.y + region->size.height - font_h - 2;
    }
    else if (ab->legend_pos == EGUI_CHART_LEGEND_TOP)
    {
        egui_dim_t total_w = count * (swatch_size + 2 + text_w + item_gap) - item_gap;
        lx = region->location.x + (region->size.width - total_w) / 2;
        if (lx < region->location.x)
        {
            lx = region->location.x;
        }
        ly = region->location.y + 1;
    }
    else if (ab->legend_pos == EGUI_CHART_LEGEND_RIGHT)
    {
        lx = plot_area->location.x + plot_area->size.width + 6;
        ly = plot_area->location.y;
    }
    else
    {
        return;
    }

    for (uint8_t i = 0; i < count; i++)
    {
        egui_color_t color = ab->series[i].color;
        const char *name = ab->series[i].name;

        if (name == NULL)
        {
            continue;
        }

        // Draw color swatch
        egui_canvas_draw_rectangle_fill(lx, ly + 1, swatch_size, swatch_size, color, EGUI_ALPHA_100);

        // Draw name text
        EGUI_REGION_DEFINE(text_rect, lx + swatch_size + 2, ly, text_w, font_h);
        egui_canvas_draw_text_in_rect(font, name, &text_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, ab->text_color, EGUI_ALPHA_100);

        // Advance position
        if (ab->legend_pos == EGUI_CHART_LEGEND_RIGHT)
        {
            ly += font_h + 2;
        }
        else
        {
            lx += swatch_size + 2 + text_w + item_gap;
        }
    }
}

// ============== Zoom / Touch Handling ==============

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH

static int32_t chart_iabs32(int32_t v)
{
    return (v >= 0) ? v : -v;
}

// Integer square root (for pinch distance calculation)
static int32_t chart_isqrt(int32_t n)
{
    if (n <= 0)
    {
        return 0;
    }
    int32_t x = n;
    int32_t y = (x + 1) / 2;
    while (y < x)
    {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

void egui_chart_clamp_viewport(egui_chart_axis_base_t *ab)
{
    int16_t ax_min = ab->axis_x.min_value;
    int16_t ax_max = ab->axis_x.max_value;
    int16_t ay_min = ab->axis_y.min_value;
    int16_t ay_max = ab->axis_y.max_value;

    // Minimum visible range (1/8 of full range)
    int16_t min_x_range = (ax_max - ax_min) / 8;
    int16_t min_y_range = (ay_max - ay_min) / 8;
    if (min_x_range < 2)
    {
        min_x_range = 2;
    }
    if (min_y_range < 2)
    {
        min_y_range = 2;
    }

    // Enforce minimum range
    if (ab->view_x_max - ab->view_x_min < min_x_range)
    {
        int16_t center = (ab->view_x_min + ab->view_x_max) / 2;
        ab->view_x_min = center - min_x_range / 2;
        ab->view_x_max = center + min_x_range / 2;
    }
    if (ab->view_y_max - ab->view_y_min < min_y_range)
    {
        int16_t center = (ab->view_y_min + ab->view_y_max) / 2;
        ab->view_y_min = center - min_y_range / 2;
        ab->view_y_max = center + min_y_range / 2;
    }

    // Clamp to axis bounds
    if (ab->view_x_min < ax_min)
    {
        ab->view_x_max += (ax_min - ab->view_x_min);
        ab->view_x_min = ax_min;
    }
    if (ab->view_x_max > ax_max)
    {
        ab->view_x_min -= (ab->view_x_max - ax_max);
        ab->view_x_max = ax_max;
    }
    if (ab->view_x_min < ax_min)
    {
        ab->view_x_min = ax_min;
    }

    if (ab->view_y_min < ay_min)
    {
        ab->view_y_max += (ay_min - ab->view_y_min);
        ab->view_y_min = ay_min;
    }
    if (ab->view_y_max > ay_max)
    {
        ab->view_y_min -= (ab->view_y_max - ay_max);
        ab->view_y_max = ay_max;
    }
    if (ab->view_y_min < ay_min)
    {
        ab->view_y_min = ay_min;
    }
}

int egui_chart_is_zoomed(egui_chart_axis_base_t *ab)
{
    return (ab->view_x_min > ab->axis_x.min_value || ab->view_x_max < ab->axis_x.max_value || ab->view_y_min > ab->axis_y.min_value ||
            ab->view_y_max < ab->axis_y.max_value);
}

void egui_chart_apply_zoom(egui_chart_axis_base_t *ab, int zoom_in)
{
    int32_t x_range = (int32_t)ab->view_x_max - (int32_t)ab->view_x_min;
    int32_t y_range = (int32_t)ab->view_y_max - (int32_t)ab->view_y_min;

    int32_t dx, dy;
    if (zoom_in)
    {
        dx = x_range / 10;
        dy = y_range / 10;
    }
    else
    {
        dx = -(x_range / 8);
        dy = -(y_range / 8);
    }

    if (dx == 0)
    {
        dx = zoom_in ? 1 : -1;
    }
    if (dy == 0)
    {
        dy = zoom_in ? 1 : -1;
    }

    ab->view_x_min += (int16_t)dx;
    ab->view_x_max -= (int16_t)dx;
    ab->view_y_min += (int16_t)dy;
    ab->view_y_max -= (int16_t)dy;

    egui_chart_clamp_viewport(ab);
}

void egui_chart_apply_zoom_axis(egui_chart_axis_base_t *ab, int zoom_in, int axis_x, int axis_y)
{
    int32_t x_range = (int32_t)ab->view_x_max - (int32_t)ab->view_x_min;
    int32_t y_range = (int32_t)ab->view_y_max - (int32_t)ab->view_y_min;

    int32_t dx = 0;
    int32_t dy = 0;
    if (axis_x)
    {
        dx = zoom_in ? (x_range / 10) : -(x_range / 8);
        if (dx == 0)
        {
            dx = zoom_in ? 1 : -1;
        }
    }
    if (axis_y)
    {
        dy = zoom_in ? (y_range / 10) : -(y_range / 8);
        if (dy == 0)
        {
            dy = zoom_in ? 1 : -1;
        }
    }

    ab->view_x_min += (int16_t)dx;
    ab->view_x_max -= (int16_t)dx;
    ab->view_y_min += (int16_t)dy;
    ab->view_y_max -= (int16_t)dy;

    egui_chart_clamp_viewport(ab);
}

int egui_chart_axis_on_touch_event(egui_view_t *self, egui_chart_axis_base_t *ab, egui_motion_event_t *event)
{
    if (!ab->zoom_enabled)
    {
        return egui_view_on_touch_event(self, event);
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_SCROLL:
    {
        if (event->scroll_delta > 0)
        {
            egui_chart_apply_zoom(ab, 1);
        }
        else if (event->scroll_delta < 0)
        {
            egui_chart_apply_zoom(ab, 0);
        }
        egui_view_invalidate(self);
        return 1;
    }

    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        ab->is_panning = 0;
        ab->is_pinching = 0;
        ab->pan_last_x = event->location.x;
        ab->pan_last_y = event->location.y;
        if (egui_chart_is_zoomed(ab))
        {
            return 1;
        }
        return 0;
    }

    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        if (event->pointer_count >= 2 && !ab->is_panning)
        {
            int32_t dx = (int32_t)event->location2.x - (int32_t)event->location.x;
            int32_t dy = (int32_t)event->location2.y - (int32_t)event->location.y;
            int32_t dx_abs = chart_iabs32(dx);
            int32_t dy_abs = chart_iabs32(dy);

            if (dx_abs <= 0)
            {
                dx_abs = 1;
            }
            if (dy_abs <= 0)
            {
                dy_abs = 1;
            }

            if (ab->is_pinching && ab->pinch_start_dx_abs > 0 && ab->pinch_start_dy_abs > 0)
            {
                int32_t start_x_range = (int32_t)ab->pinch_start_view_x_max - (int32_t)ab->pinch_start_view_x_min;
                int32_t start_y_range = (int32_t)ab->pinch_start_view_y_max - (int32_t)ab->pinch_start_view_y_min;

                int32_t new_x_range = start_x_range * ab->pinch_start_dx_abs / dx_abs;
                int32_t new_y_range = start_y_range * ab->pinch_start_dy_abs / dy_abs;

                if (new_x_range < 2)
                {
                    new_x_range = 2;
                }
                if (new_y_range < 2)
                {
                    new_y_range = 2;
                }

                int16_t cx = (ab->pinch_start_view_x_min + ab->pinch_start_view_x_max) / 2;
                int16_t cy = (ab->pinch_start_view_y_min + ab->pinch_start_view_y_max) / 2;

                ab->view_x_min = cx - (int16_t)(new_x_range / 2);
                ab->view_x_max = cx + (int16_t)(new_x_range / 2);
                ab->view_y_min = cy - (int16_t)(new_y_range / 2);
                ab->view_y_max = cy + (int16_t)(new_y_range / 2);

                egui_chart_clamp_viewport(ab);
                egui_view_invalidate(self);
            }
            return 1;
        }

        // Single-finger move = pan (only when zoomed)
        if (egui_chart_is_zoomed(ab) && event->pointer_count <= 1)
        {
            egui_dim_t dx_px = event->location.x - ab->pan_last_x;
            egui_dim_t dy_px = event->location.y - ab->pan_last_y;

            egui_region_t region;
            egui_view_get_work_region(self, &region);
            egui_region_t plot_area;
            egui_chart_calc_plot_area(ab, &region, &plot_area);

            int32_t x_range = (int32_t)ab->view_x_max - (int32_t)ab->view_x_min;
            int32_t y_range = (int32_t)ab->view_y_max - (int32_t)ab->view_y_min;

            if (plot_area.size.width > 0 && plot_area.size.height > 0)
            {
                int16_t data_dx = (int16_t)(-(int32_t)dx_px * x_range / plot_area.size.width);
                int16_t data_dy = (int16_t)((int32_t)dy_px * y_range / plot_area.size.height);

                if (data_dx != 0 || data_dy != 0)
                {
                    if (!ab->is_panning)
                    {
                        ab->is_panning = 1;
                        if (self->parent != NULL)
                        {
                            egui_view_group_request_disallow_intercept_touch_event((egui_view_t *)self->parent, 1);
                        }
                    }

                    ab->view_x_min += data_dx;
                    ab->view_x_max += data_dx;
                    ab->view_y_min += data_dy;
                    ab->view_y_max += data_dy;
                    egui_chart_clamp_viewport(ab);
                    egui_view_invalidate(self);
                }
            }

            ab->pan_last_x = event->location.x;
            ab->pan_last_y = event->location.y;
            return 1;
        }
        return 0;
    }

    case EGUI_MOTION_EVENT_ACTION_POINTER_DOWN:
    {
        ab->is_pinching = 1;
        ab->is_panning = 0;
        int32_t dx = (int32_t)event->location2.x - (int32_t)event->location.x;
        int32_t dy = (int32_t)event->location2.y - (int32_t)event->location.y;
        ab->pinch_start_dx_abs = chart_iabs32(dx);
        ab->pinch_start_dy_abs = chart_iabs32(dy);
        if (ab->pinch_start_dx_abs <= 0)
        {
            ab->pinch_start_dx_abs = 1;
        }
        if (ab->pinch_start_dy_abs <= 0)
        {
            ab->pinch_start_dy_abs = 1;
        }
        ab->pinch_start_dist = chart_isqrt(dx * dx + dy * dy);
        ab->pinch_start_view_x_min = ab->view_x_min;
        ab->pinch_start_view_x_max = ab->view_x_max;
        ab->pinch_start_view_y_min = ab->view_y_min;
        ab->pinch_start_view_y_max = ab->view_y_max;

        if (self->parent != NULL)
        {
            egui_view_group_request_disallow_intercept_touch_event((egui_view_t *)self->parent, 1);
        }
        return 1;
    }

    case EGUI_MOTION_EVENT_ACTION_POINTER_UP:
    {
        ab->is_pinching = 0;
        return 1;
    }

    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
    {
        ab->is_panning = 0;
        ab->is_pinching = 0;
        return egui_chart_is_zoomed(ab) ? 1 : 0;
    }

    default:
        break;
    }

    return 0;
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
