#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_chart_pie.h"
#include "egui_view_circle_dirty.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

typedef egui_dim_t (*egui_view_chart_pie_get_font_height_fn)(egui_view_chart_pie_t *local);
typedef void (*egui_view_chart_pie_draw_legend_text_fn)(egui_view_chart_pie_t *local, const char *text, egui_region_t *text_rect);

struct egui_view_chart_pie_text_ops
{
    egui_view_chart_pie_get_font_height_fn get_font_height;
    egui_view_chart_pie_draw_legend_text_fn draw_legend_text;
};

typedef struct egui_view_chart_pie_geometry
{
    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t radius;
} egui_view_chart_pie_geometry_t;

#define EGUI_VIEW_CHART_PIE_ANGLE_CULL_PAD_DEG 2

static egui_dim_t egui_view_chart_pie_get_font_height_basic(egui_view_chart_pie_t *local)
{
    (void)local;
    return egui_chart_get_font_height((const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
}

static egui_dim_t egui_view_chart_pie_get_font_height_rich(egui_view_chart_pie_t *local)
{
    if (local == NULL || local->font == NULL)
    {
        return 8;
    }

    return EGUI_FONT_STD_GET_FONT_HEIGHT(local->font);
}

static void egui_view_chart_pie_draw_legend_text_basic(egui_view_chart_pie_t *local, const char *text, egui_region_t *text_rect)
{
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    if (local == NULL || text == NULL || text_rect == NULL)
    {
        return;
    }

    if (font != NULL)
    {
        egui_canvas_draw_text_in_rect(font, text, text_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, local->text_color, EGUI_ALPHA_100);
    }
}

static void egui_view_chart_pie_draw_legend_text_rich(egui_view_chart_pie_t *local, const char *text, egui_region_t *text_rect)
{
    if (local == NULL || local->font == NULL || text == NULL || text_rect == NULL)
    {
        return;
    }

    egui_canvas_draw_text_in_rect(local->font, text, text_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, local->text_color, EGUI_ALPHA_100);
}

static const egui_view_chart_pie_text_ops_t egui_view_chart_pie_basic_text_ops = {
        .get_font_height = egui_view_chart_pie_get_font_height_basic,
        .draw_legend_text = egui_view_chart_pie_draw_legend_text_basic,
};

static const egui_view_chart_pie_text_ops_t egui_view_chart_pie_rich_text_ops = {
        .get_font_height = egui_view_chart_pie_get_font_height_rich,
        .draw_legend_text = egui_view_chart_pie_draw_legend_text_rich,
};

static egui_dim_t egui_view_chart_pie_get_font_height(egui_view_chart_pie_t *local)
{
    if (local == NULL || local->text_ops == NULL || local->text_ops->get_font_height == NULL)
    {
        return 8;
    }

    return local->text_ops->get_font_height(local);
}

static int egui_view_chart_pie_rect_intersects_work_region(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t *work = egui_canvas_get_base_view_work_region();
    egui_dim_t rect_x2;
    egui_dim_t rect_y2;
    egui_dim_t work_x2;
    egui_dim_t work_y2;

    if (work == NULL || width <= 0 || height <= 0 || egui_region_is_empty(work))
    {
        return 0;
    }

    rect_x2 = x + width;
    rect_y2 = y + height;
    work_x2 = work->location.x + work->size.width;
    work_y2 = work->location.y + work->size.height;
    return !(rect_x2 <= work->location.x || x >= work_x2 || rect_y2 <= work->location.y || y >= work_y2);
}

static uint16_t egui_view_chart_pie_isqrt32(uint32_t n)
{
    uint32_t result = 0;
    uint32_t bit = 1u << 30;

    while (bit > n)
    {
        bit >>= 2;
    }

    while (bit != 0)
    {
        if (n >= result + bit)
        {
            n -= result + bit;
            result = (result >> 1) + bit;
        }
        else
        {
            result >>= 1;
        }
        bit >>= 2;
    }

    return (uint16_t)result;
}

static int16_t egui_view_chart_pie_integer_atan2_deg(int32_t dy, int32_t dx)
{
    int32_t abs_y;
    int32_t abs_x;
    int16_t angle;

    if (dx == 0 && dy == 0)
    {
        return 0;
    }
    if (dx == 0)
    {
        return (dy > 0) ? 90 : 270;
    }
    if (dy == 0)
    {
        return (dx > 0) ? 0 : 180;
    }

    abs_y = (dy < 0) ? -dy : dy;
    abs_x = (dx < 0) ? -dx : dx;

    if (abs_x >= abs_y)
    {
        angle = (int16_t)((int32_t)45 * abs_y / abs_x);
    }
    else
    {
        angle = 90 - (int16_t)((int32_t)45 * abs_x / abs_y);
    }

    if (dx < 0)
    {
        angle = 180 - angle;
    }
    if (dy < 0)
    {
        angle = 360 - angle;
    }

    return angle % 360;
}

static int egui_view_chart_pie_point_in_region(const egui_region_t *region, egui_dim_t x, egui_dim_t y)
{
    if (region == NULL || egui_region_is_empty((egui_region_t *)region))
    {
        return 0;
    }

    return x >= region->location.x && x < (region->location.x + region->size.width) && y >= region->location.y &&
           y < (region->location.y + region->size.height);
}

static int egui_view_chart_pie_angle_in_slice(int16_t start_angle, uint16_t sweep, int16_t angle)
{
    int16_t padded_start;
    uint16_t padded_sweep;

    if (sweep >= 360U)
    {
        return 1;
    }

    padded_start = egui_view_circle_dirty_normalize_angle((int32_t)start_angle - EGUI_VIEW_CHART_PIE_ANGLE_CULL_PAD_DEG);
    padded_sweep = sweep + (uint16_t)(EGUI_VIEW_CHART_PIE_ANGLE_CULL_PAD_DEG * 2);
    if (padded_sweep >= 360U)
    {
        return 1;
    }

    return (uint16_t)egui_view_circle_dirty_normalize_angle((int32_t)angle - padded_start) <= padded_sweep;
}

static int egui_view_chart_pie_point_in_slice(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, uint16_t sweep, egui_dim_t x,
                                              egui_dim_t y)
{
    int32_t dx = (int32_t)x - center_x;
    int32_t dy = (int32_t)y - center_y;
    int32_t outer_radius = radius + 1;

    if (dx * dx + dy * dy > outer_radius * outer_radius)
    {
        return 0;
    }

    return egui_view_chart_pie_angle_in_slice(start_angle, sweep, egui_view_chart_pie_integer_atan2_deg(dy, dx));
}

static int egui_view_chart_pie_point_on_segment(egui_dim_t ax, egui_dim_t ay, egui_dim_t bx, egui_dim_t by, egui_dim_t px, egui_dim_t py)
{
    return px >= EGUI_MIN(ax, bx) && px <= EGUI_MAX(ax, bx) && py >= EGUI_MIN(ay, by) && py <= EGUI_MAX(ay, by);
}

static int32_t egui_view_chart_pie_segment_cross(egui_dim_t ax, egui_dim_t ay, egui_dim_t bx, egui_dim_t by, egui_dim_t px, egui_dim_t py)
{
    return ((int32_t)bx - ax) * ((int32_t)py - ay) - ((int32_t)by - ay) * ((int32_t)px - ax);
}

static int egui_view_chart_pie_segments_intersect(egui_dim_t ax, egui_dim_t ay, egui_dim_t bx, egui_dim_t by, egui_dim_t cx, egui_dim_t cy, egui_dim_t dx,
                                                  egui_dim_t dy)
{
    int32_t d1 = egui_view_chart_pie_segment_cross(ax, ay, bx, by, cx, cy);
    int32_t d2 = egui_view_chart_pie_segment_cross(ax, ay, bx, by, dx, dy);
    int32_t d3 = egui_view_chart_pie_segment_cross(cx, cy, dx, dy, ax, ay);
    int32_t d4 = egui_view_chart_pie_segment_cross(cx, cy, dx, dy, bx, by);

    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) && ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)))
    {
        return 1;
    }

    if (d1 == 0 && egui_view_chart_pie_point_on_segment(ax, ay, bx, by, cx, cy))
    {
        return 1;
    }
    if (d2 == 0 && egui_view_chart_pie_point_on_segment(ax, ay, bx, by, dx, dy))
    {
        return 1;
    }
    if (d3 == 0 && egui_view_chart_pie_point_on_segment(cx, cy, dx, dy, ax, ay))
    {
        return 1;
    }
    if (d4 == 0 && egui_view_chart_pie_point_on_segment(cx, cy, dx, dy, bx, by))
    {
        return 1;
    }

    return 0;
}

static int egui_view_chart_pie_segment_intersects_region(egui_dim_t x0, egui_dim_t y0, egui_dim_t x1, egui_dim_t y1, const egui_region_t *region)
{
    egui_dim_t left;
    egui_dim_t top;
    egui_dim_t right;
    egui_dim_t bottom;

    if (region == NULL || egui_region_is_empty((egui_region_t *)region))
    {
        return 0;
    }

    if (egui_view_chart_pie_point_in_region(region, x0, y0) || egui_view_chart_pie_point_in_region(region, x1, y1))
    {
        return 1;
    }

    left = region->location.x;
    top = region->location.y;
    right = left + region->size.width - 1;
    bottom = top + region->size.height - 1;

    return egui_view_chart_pie_segments_intersect(x0, y0, x1, y1, left, top, right, top) ||
           egui_view_chart_pie_segments_intersect(x0, y0, x1, y1, right, top, right, bottom) ||
           egui_view_chart_pie_segments_intersect(x0, y0, x1, y1, right, bottom, left, bottom) ||
           egui_view_chart_pie_segments_intersect(x0, y0, x1, y1, left, bottom, left, top);
}

static int egui_view_chart_pie_slice_circle_edge_hits_region(const egui_region_t *work, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius,
                                                             int16_t start_angle, uint16_t sweep)
{
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x1;
    egui_dim_t y1;
    int32_t outer_radius;
    int32_t outer_radius_sq;
    uint8_t i;

    if (work == NULL || egui_region_is_empty((egui_region_t *)work))
    {
        return 0;
    }

    x0 = work->location.x;
    y0 = work->location.y;
    x1 = x0 + work->size.width - 1;
    y1 = y0 + work->size.height - 1;
    outer_radius = radius + 1;
    outer_radius_sq = outer_radius * outer_radius;

    for (i = 0; i < 2; i++)
    {
        egui_dim_t x = (i == 0) ? x0 : x1;
        int32_t dx = (int32_t)x - center_x;
        int32_t remain = outer_radius_sq - dx * dx;

        if (remain >= 0)
        {
            uint16_t y_off = egui_view_chart_pie_isqrt32((uint32_t)remain);
            egui_dim_t y_top = center_y - (egui_dim_t)y_off;
            egui_dim_t y_bottom = center_y + (egui_dim_t)y_off;

            if (y_top >= y0 && y_top <= y1 && egui_view_chart_pie_point_in_slice(center_x, center_y, radius, start_angle, sweep, x, y_top))
            {
                return 1;
            }
            if (y_bottom >= y0 && y_bottom <= y1 && egui_view_chart_pie_point_in_slice(center_x, center_y, radius, start_angle, sweep, x, y_bottom))
            {
                return 1;
            }
        }
    }

    for (i = 0; i < 2; i++)
    {
        egui_dim_t y = (i == 0) ? y0 : y1;
        int32_t dy = (int32_t)y - center_y;
        int32_t remain = outer_radius_sq - dy * dy;

        if (remain >= 0)
        {
            uint16_t x_off = egui_view_chart_pie_isqrt32((uint32_t)remain);
            egui_dim_t x_left = center_x - (egui_dim_t)x_off;
            egui_dim_t x_right = center_x + (egui_dim_t)x_off;

            if (x_left >= x0 && x_left <= x1 && egui_view_chart_pie_point_in_slice(center_x, center_y, radius, start_angle, sweep, x_left, y))
            {
                return 1;
            }
            if (x_right >= x0 && x_right <= x1 && egui_view_chart_pie_point_in_slice(center_x, center_y, radius, start_angle, sweep, x_right, y))
            {
                return 1;
            }
        }
    }

    return 0;
}

static int egui_view_chart_pie_slice_intersects_work_region(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, uint16_t sweep)
{
    egui_region_t slice_region;
    egui_region_t *work = egui_canvas_get_base_view_work_region();
    egui_dim_t mid_radius;
    egui_dim_t edge_radius;
    egui_dim_t start_x;
    egui_dim_t start_y;
    egui_dim_t end_x;
    egui_dim_t end_y;

    if (work == NULL || egui_region_is_empty(work))
    {
        return 0;
    }

    if (sweep >= 360U)
    {
        return egui_view_chart_pie_rect_intersects_work_region(center_x - radius, center_y - radius, radius * 2 + 1, radius * 2 + 1);
    }

    if (!egui_view_chart_pie_rect_intersects_work_region(center_x - radius, center_y - radius, radius * 2 + 1, radius * 2 + 1))
    {
        return 0;
    }

    mid_radius = (radius + 1) / 2;
    if (!egui_view_circle_dirty_compute_arc_region(center_x, center_y, mid_radius, mid_radius, start_angle, sweep, &slice_region))
    {
        return 0;
    }

    if (!egui_view_chart_pie_rect_intersects_work_region(slice_region.location.x, slice_region.location.y, slice_region.size.width, slice_region.size.height))
    {
        return 0;
    }

    if (egui_view_chart_pie_point_in_region(work, center_x, center_y))
    {
        return 1;
    }

    {
        egui_dim_t x0 = work->location.x;
        egui_dim_t y0 = work->location.y;
        egui_dim_t x1 = x0 + work->size.width - 1;
        egui_dim_t y1 = y0 + work->size.height - 1;

        if (egui_view_chart_pie_point_in_slice(center_x, center_y, radius, start_angle, sweep, x0, y0) ||
            egui_view_chart_pie_point_in_slice(center_x, center_y, radius, start_angle, sweep, x1, y0) ||
            egui_view_chart_pie_point_in_slice(center_x, center_y, radius, start_angle, sweep, x0, y1) ||
            egui_view_chart_pie_point_in_slice(center_x, center_y, radius, start_angle, sweep, x1, y1))
        {
            return 1;
        }
    }

    edge_radius = radius + 1;
    egui_view_circle_dirty_get_circle_point(center_x, center_y, edge_radius, egui_view_circle_dirty_normalize_angle(start_angle), &start_x, &start_y);
    egui_view_circle_dirty_get_circle_point(center_x, center_y, edge_radius, egui_view_circle_dirty_normalize_angle((int32_t)start_angle + sweep), &end_x, &end_y);

    if (egui_view_chart_pie_segment_intersects_region(center_x, center_y, start_x, start_y, work) ||
        egui_view_chart_pie_segment_intersects_region(center_x, center_y, end_x, end_y, work))
    {
        return 1;
    }

    return egui_view_chart_pie_slice_circle_edge_hits_region(work, center_x, center_y, radius, start_angle, sweep);
}

static uint32_t egui_view_chart_pie_get_total_value(egui_view_chart_pie_t *local)
{
    uint32_t total = 0;
    uint8_t i;

    if (local == NULL || local->pie_slice_count == 0 || local->pie_slices == NULL)
    {
        return 0;
    }

    for (i = 0; i < local->pie_slice_count; i++)
    {
        total += local->pie_slices[i].value;
    }

    return total;
}

static int egui_view_chart_pie_compute_geometry(egui_view_chart_pie_t *local, egui_region_t *region, egui_view_chart_pie_geometry_t *geometry)
{
    egui_dim_t font_h;
    egui_dim_t avail_x;
    egui_dim_t avail_y;
    egui_dim_t avail_w;
    egui_dim_t avail_h;

    if (local == NULL || region == NULL || geometry == NULL)
    {
        return 0;
    }

    font_h = egui_view_chart_pie_get_font_height(local);

    avail_x = region->location.x;
    avail_y = region->location.y;
    avail_w = region->size.width;
    avail_h = region->size.height;

    if (local->legend_pos == EGUI_CHART_LEGEND_RIGHT)
    {
        avail_w -= font_h * 4;
    }
    else if (local->legend_pos == EGUI_CHART_LEGEND_BOTTOM)
    {
        avail_h -= font_h + 4;
    }
    else if (local->legend_pos == EGUI_CHART_LEGEND_TOP)
    {
        avail_y += font_h + 4;
        avail_h -= font_h + 4;
    }

    geometry->center_x = avail_x + avail_w / 2;
    geometry->center_y = avail_y + avail_h / 2;
    geometry->radius = EGUI_MIN(avail_w, avail_h) / 2 - 2;

    return (geometry->radius > 0) ? 1 : 0;
}

static int egui_view_chart_pie_is_work_region_inside_solid_circle(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius)
{
    egui_region_t *work = egui_canvas_get_base_view_work_region();
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x1;
    egui_dim_t y1;
    int32_t solid_radius;
    int32_t solid_radius_sq;

    if (work == NULL || egui_region_is_empty(work))
    {
        return 0;
    }

    solid_radius = (radius > 1) ? (radius - 1) : radius;
    if (solid_radius <= 0)
    {
        return 0;
    }

    x0 = work->location.x;
    y0 = work->location.y;
    x1 = x0 + work->size.width - 1;
    y1 = y0 + work->size.height - 1;
    solid_radius_sq = solid_radius * solid_radius;

#define EGUI_VIEW_CHART_PIE_CORNER_INSIDE(_x, _y)                                                                                                             \
    (((int32_t)(_x) - center_x) * ((int32_t)(_x) - center_x) + ((int32_t)(_y) - center_y) * ((int32_t)(_y) - center_y) <= solid_radius_sq)

    return EGUI_VIEW_CHART_PIE_CORNER_INSIDE(x0, y0) && EGUI_VIEW_CHART_PIE_CORNER_INSIDE(x1, y0) && EGUI_VIEW_CHART_PIE_CORNER_INSIDE(x0, y1) &&
           EGUI_VIEW_CHART_PIE_CORNER_INSIDE(x1, y1);

#undef EGUI_VIEW_CHART_PIE_CORNER_INSIDE
}

// ============== Pie Chart Drawing ==============

static void egui_view_chart_pie_draw_pie(egui_view_chart_pie_t *local, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, uint32_t total)
{
    if (local->pie_slice_count == 0 || local->pie_slices == NULL || total == 0 || radius <= 0)
    {
        return;
    }

    if (!egui_view_chart_pie_rect_intersects_work_region(center_x - radius, center_y - radius, radius * 2 + 1, radius * 2 + 1))
    {
        return;
    }

    // Draw all slices as arcs with inner_r=1 to avoid center pixel artifact.
    // Then fill center pixel with first slice color.
    // Use cumulative proportion to avoid rounding error accumulation.
    int16_t start_angle = 270;
    uint32_t cumulative_value = 0;
    for (uint8_t i = 0; i < local->pie_slice_count; i++)
    {
        cumulative_value += local->pie_slices[i].value;
        int16_t end_angle;
        if (i == local->pie_slice_count - 1)
        {
            end_angle = 630; // ensure exact full circle
        }
        else
        {
            end_angle = 270 + (int16_t)((uint32_t)cumulative_value * 360 / total);
        }

        int16_t sweep = end_angle - start_angle;
        if (sweep < 1 && local->pie_slices[i].value > 0)
        {
            end_angle = start_angle + 1;
            sweep = 1;
        }

        if (end_angle <= start_angle)
        {
            start_angle = end_angle;
            continue;
        }

        if (!egui_view_chart_pie_slice_intersects_work_region(center_x, center_y, radius, start_angle, (uint16_t)sweep))
        {
            start_angle = end_angle;
            continue;
        }

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t color_light = egui_rgb_mix(local->pie_slices[i].color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t stops[2] = {
                    {.position = 0, .color = color_light},
                    {.position = 255, .color = local->pie_slices[i].color},
            };
            egui_gradient_t grad = {
                    .type = EGUI_GRADIENT_TYPE_RADIAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = stops,
            };
            egui_canvas_draw_arc_ring_fill_gradient(center_x, center_y, radius, 1, start_angle, end_angle, &grad);
        }
#else
        egui_canvas_draw_arc_fill(center_x, center_y, radius, start_angle, end_angle, local->pie_slices[i].color, EGUI_ALPHA_100);
#endif

        start_angle = end_angle;
    }

    // Fill center pixel with first slice gradient center color
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t center_color = egui_rgb_mix(local->pie_slices[0].color, EGUI_COLOR_WHITE, 80);
        egui_canvas_draw_rectangle_fill(center_x, center_y, 1, 1, center_color, EGUI_ALPHA_100);
    }
#else
    egui_canvas_draw_rectangle_fill(center_x, center_y, 1, 1, local->pie_slices[0].color, EGUI_ALPHA_100);
#endif
}

// ============== Pie Legend Drawing ==============

static void egui_view_chart_pie_draw_legend(egui_view_chart_pie_t *local, egui_region_t *region)
{
    egui_dim_t font_h = egui_view_chart_pie_get_font_height(local);
    egui_dim_t swatch_size = font_h > 2 ? font_h - 2 : 4;
    egui_dim_t item_gap = 6;
    egui_dim_t text_w = font_h * 3;

    uint8_t count = local->pie_slice_count;
    if (count == 0)
    {
        return;
    }

    // Calculate legend starting position
    egui_dim_t lx, ly;
    if (local->legend_pos == EGUI_CHART_LEGEND_BOTTOM)
    {
        egui_dim_t total_w = count * (swatch_size + 2 + text_w + item_gap) - item_gap;
        lx = region->location.x + (region->size.width - total_w) / 2;
        if (lx < region->location.x)
        {
            lx = region->location.x;
        }
        ly = region->location.y + region->size.height - font_h - 2;
    }
    else if (local->legend_pos == EGUI_CHART_LEGEND_TOP)
    {
        egui_dim_t total_w = count * (swatch_size + 2 + text_w + item_gap) - item_gap;
        lx = region->location.x + (region->size.width - total_w) / 2;
        if (lx < region->location.x)
        {
            lx = region->location.x;
        }
        ly = region->location.y + 1;
    }
    else if (local->legend_pos == EGUI_CHART_LEGEND_RIGHT)
    {
        // For pie, legend goes to the right of the pie area
        egui_dim_t pie_w = region->size.width - font_h * 4;
        lx = region->location.x + pie_w + 6;
        ly = region->location.y + (region->size.height - count * (font_h + 2)) / 2;
        if (ly < region->location.y)
        {
            ly = region->location.y;
        }
    }
    else
    {
        return;
    }

    for (uint8_t i = 0; i < count; i++)
    {
        egui_color_t color = local->pie_slices[i].color;
        const char *name = local->pie_slices[i].name;

        if (name == NULL)
        {
            continue;
        }

        // Draw color swatch
        if (egui_view_chart_pie_rect_intersects_work_region(lx, ly + 1, swatch_size, swatch_size))
        {
            egui_canvas_draw_rectangle_fill(lx, ly + 1, swatch_size, swatch_size, color, EGUI_ALPHA_100);
        }

        // Draw name text
        EGUI_REGION_DEFINE(text_rect, lx + swatch_size + 2, ly, text_w, font_h);
        if (local->text_ops != NULL && local->text_ops->draw_legend_text != NULL &&
            egui_view_chart_pie_rect_intersects_work_region(text_rect.location.x, text_rect.location.y, text_rect.size.width, text_rect.size.height))
        {
            local->text_ops->draw_legend_text(local, name, &text_rect);
        }

        // Advance position
        if (local->legend_pos == EGUI_CHART_LEGEND_RIGHT)
        {
            ly += font_h + 2;
        }
        else
        {
            lx += swatch_size + 2 + text_w + item_gap;
        }
    }
}

// ============== Main Draw ==============

void egui_view_chart_pie_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chart_pie_t);

    egui_region_t region;
    egui_view_chart_pie_geometry_t geometry;
    uint32_t total;
    int has_geometry;
    egui_view_get_work_region(self, &region);

    if (region.size.width < 10 || region.size.height < 10)
    {
        return;
    }

    total = egui_view_chart_pie_get_total_value(local);
    has_geometry = (total > 0) ? egui_view_chart_pie_compute_geometry(local, &region, &geometry) : 0;

    if (!(has_geometry && egui_view_chart_pie_is_work_region_inside_solid_circle(geometry.center_x, geometry.center_y, geometry.radius)))
    {
        // Draw background only when the current tile is not fully covered by the pie.
        egui_canvas_draw_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, local->bg_color, EGUI_ALPHA_100);
    }

    // Draw pie
    if (has_geometry)
    {
        egui_view_chart_pie_draw_pie(local, geometry.center_x, geometry.center_y, geometry.radius, total);
    }

    // Draw legend
    if (local->legend_pos != EGUI_CHART_LEGEND_NONE)
    {
        egui_view_chart_pie_draw_legend(local, &region);
    }
}

// ============== API Table ==============

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_chart_pie_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_chart_pie_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

// ============== Init / Params ==============

void egui_view_chart_pie_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_chart_pie_t);
    // call super init
    egui_view_init(self);
    // update api
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_chart_pie_t);

    // init local data
    local->pie_slices = NULL;
    local->pie_slice_count = 0;

    // legend
    local->legend_pos = EGUI_CHART_LEGEND_NONE;

    // style
    local->bg_color = EGUI_THEME_SURFACE;
    local->text_color = EGUI_THEME_TEXT_SECONDARY;
    local->font = NULL;
    local->text_ops = &egui_view_chart_pie_basic_text_ops;

    egui_view_set_view_name(self, "egui_view_chart_pie");
}

void egui_view_chart_pie_apply_params(egui_view_t *self, const egui_view_chart_pie_params_t *params)
{
    self->region = params->region;
    egui_view_invalidate(self);
}

void egui_view_chart_pie_init_with_params(egui_view_t *self, const egui_view_chart_pie_params_t *params)
{
    egui_view_chart_pie_init(self);
    egui_view_chart_pie_apply_params(self, params);
}

// ============== Setters ==============

void egui_view_chart_pie_set_slices(egui_view_t *self, const egui_chart_pie_slice_t *slices, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_chart_pie_t);
    local->pie_slices = slices;
    local->pie_slice_count = count;
    egui_view_invalidate(self);
}

void egui_view_chart_pie_set_legend_pos(egui_view_t *self, uint8_t pos)
{
    EGUI_LOCAL_INIT(egui_view_chart_pie_t);
    if (local->legend_pos != pos)
    {
        local->legend_pos = pos;
        egui_view_invalidate(self);
    }
}

void egui_view_chart_pie_set_colors(egui_view_t *self, egui_color_t bg, egui_color_t text)
{
    EGUI_LOCAL_INIT(egui_view_chart_pie_t);
    local->bg_color = bg;
    local->text_color = text;
    egui_view_invalidate(self);
}

void egui_view_chart_pie_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_chart_pie_t);
    local->font = font;
    local->text_ops = (font == NULL) ? &egui_view_chart_pie_basic_text_ops : &egui_view_chart_pie_rich_text_ops;
    egui_view_invalidate(self);
}
