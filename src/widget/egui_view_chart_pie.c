#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_chart_pie.h"
#include "egui_view_circle_dirty.h"
#include "resource/egui_resource.h"

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

typedef struct egui_view_chart_pie_work_region_info
{
    egui_region_t *work;
    int contains_center;
    int inside_solid_circle;
    int has_angle_window;
    int16_t angle_start;
    uint16_t angle_sweep;
} egui_view_chart_pie_work_region_info_t;

#define EGUI_VIEW_CHART_PIE_ANGLE_CULL_PAD_DEG   0
#define EGUI_VIEW_CHART_PIE_ANGLE_WINDOW_PAD_DEG 5

static int egui_view_chart_pie_is_work_region_inside_solid_circle(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius);
static int egui_view_chart_pie_work_region_intersects_outer_circle(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius);
static int16_t egui_view_chart_pie_integer_atan2_deg(int32_t dy, int32_t dx);

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

static int egui_view_chart_pie_sweep_contains_angle(int16_t start_angle, uint16_t sweep, int16_t angle)
{
    if (sweep >= 360U)
    {
        return 1;
    }

    return (uint16_t)egui_view_circle_dirty_normalize_angle((int32_t)angle - start_angle) <= sweep;
}

static void egui_view_chart_pie_sort_angles(int16_t *angles, uint8_t count)
{
    for (uint8_t i = 1; i < count; i++)
    {
        int16_t value = angles[i];
        uint8_t j = i;

        while (j > 0 && angles[j - 1] > value)
        {
            angles[j] = angles[j - 1];
            j--;
        }
        angles[j] = value;
    }
}

static void egui_view_chart_pie_add_angle_candidate(int16_t *angles, uint8_t *count, int16_t angle)
{
    uint8_t i;
    int16_t normalized;

    if (angles == NULL || count == NULL)
    {
        return;
    }

    normalized = egui_view_circle_dirty_normalize_angle(angle);
    for (i = 0; i < *count; i++)
    {
        if (angles[i] == normalized)
        {
            return;
        }
    }

    angles[*count] = normalized;
    (*count)++;
}

static int egui_view_chart_pie_get_region_angle_window(const egui_region_t *work, egui_dim_t center_x, egui_dim_t center_y, int16_t *out_start,
                                                       uint16_t *out_sweep)
{
    int16_t angles[8];
    uint8_t count = 0;
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x1;
    egui_dim_t y1;
    int32_t max_gap = -1;
    uint8_t gap_index = 0;

    if (work == NULL || out_start == NULL || out_sweep == NULL || egui_region_is_empty((egui_region_t *)work))
    {
        return 0;
    }

    x0 = work->location.x;
    y0 = work->location.y;
    x1 = x0 + work->size.width - 1;
    y1 = y0 + work->size.height - 1;

    egui_view_chart_pie_add_angle_candidate(angles, &count, egui_view_chart_pie_integer_atan2_deg(y0 - center_y, x0 - center_x));
    egui_view_chart_pie_add_angle_candidate(angles, &count, egui_view_chart_pie_integer_atan2_deg(y0 - center_y, x1 - center_x));
    egui_view_chart_pie_add_angle_candidate(angles, &count, egui_view_chart_pie_integer_atan2_deg(y1 - center_y, x0 - center_x));
    egui_view_chart_pie_add_angle_candidate(angles, &count, egui_view_chart_pie_integer_atan2_deg(y1 - center_y, x1 - center_x));

    if (x0 <= center_x && center_x <= x1)
    {
        if (y0 <= center_y)
        {
            egui_view_chart_pie_add_angle_candidate(angles, &count, 270);
        }
        if (y1 >= center_y)
        {
            egui_view_chart_pie_add_angle_candidate(angles, &count, 90);
        }
    }

    if (y0 <= center_y && center_y <= y1)
    {
        if (x0 <= center_x)
        {
            egui_view_chart_pie_add_angle_candidate(angles, &count, 180);
        }
        if (x1 >= center_x)
        {
            egui_view_chart_pie_add_angle_candidate(angles, &count, 0);
        }
    }

    if (count == 0)
    {
        return 0;
    }

    egui_view_chart_pie_sort_angles(angles, count);

    for (uint8_t i = 0; i < count; i++)
    {
        int32_t cur = angles[i];
        int32_t next = (i + 1u < count) ? angles[i + 1u] : (angles[0] + 360);
        int32_t gap = next - cur;

        if (gap > max_gap)
        {
            max_gap = gap;
            gap_index = i;
        }
    }

    *out_start = angles[(gap_index + 1u) % count];
    *out_sweep = (uint16_t)(360 - max_gap);

    if (*out_sweep < 360U && EGUI_VIEW_CHART_PIE_ANGLE_WINDOW_PAD_DEG > 0)
    {
        *out_start = egui_view_circle_dirty_normalize_angle((int32_t)(*out_start) - EGUI_VIEW_CHART_PIE_ANGLE_WINDOW_PAD_DEG);
        *out_sweep = EGUI_MIN((uint16_t)360U, (uint16_t)(*out_sweep + (EGUI_VIEW_CHART_PIE_ANGLE_WINDOW_PAD_DEG * 2)));
    }

    return 1;
}

static int egui_view_chart_pie_arc_intersects_arc(int16_t start_a, uint16_t sweep_a, int16_t start_b, uint16_t sweep_b)
{
    int16_t end_a;
    int16_t end_b;

    if (sweep_a >= 360U || sweep_b >= 360U)
    {
        return 1;
    }

    if (sweep_a == 0U)
    {
        return egui_view_chart_pie_sweep_contains_angle(start_b, sweep_b, start_a);
    }

    if (sweep_b == 0U)
    {
        return egui_view_chart_pie_sweep_contains_angle(start_a, sweep_a, start_b);
    }

    end_a = egui_view_circle_dirty_normalize_angle((int32_t)start_a + sweep_a);
    end_b = egui_view_circle_dirty_normalize_angle((int32_t)start_b + sweep_b);
    return egui_view_chart_pie_sweep_contains_angle(start_a, sweep_a, start_b) || egui_view_chart_pie_sweep_contains_angle(start_a, sweep_a, end_b) ||
           egui_view_chart_pie_sweep_contains_angle(start_b, sweep_b, start_a) || egui_view_chart_pie_sweep_contains_angle(start_b, sweep_b, end_a);
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
    egui_dim_t seg_min_x;
    egui_dim_t seg_max_x;
    egui_dim_t seg_min_y;
    egui_dim_t seg_max_y;

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
    seg_min_x = EGUI_MIN(x0, x1);
    seg_max_x = EGUI_MAX(x0, x1);
    seg_min_y = EGUI_MIN(y0, y1);
    seg_max_y = EGUI_MAX(y0, y1);

    if (seg_max_x < left || seg_min_x > right || seg_max_y < top || seg_min_y > bottom)
    {
        return 0;
    }

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

static int egui_view_chart_pie_slice_bbox_intersects_work_region(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle,
                                                                 uint16_t sweep, egui_dim_t *start_x, egui_dim_t *start_y, egui_dim_t *end_x, egui_dim_t *end_y)
{
    static const int16_t critical_angles[] = {0, 90, 180, 270};
    egui_dim_t edge_radius;
    egui_dim_t bbox_min_x;
    egui_dim_t bbox_min_y;
    egui_dim_t bbox_max_x;
    egui_dim_t bbox_max_y;
    uint8_t i;
    int16_t start_norm;
    int16_t end_norm;

    if (start_x == NULL || start_y == NULL || end_x == NULL || end_y == NULL)
    {
        return 0;
    }

    if (sweep >= 360U)
    {
        *start_x = center_x - radius;
        *start_y = center_y;
        *end_x = center_x + radius;
        *end_y = center_y;
        return egui_view_chart_pie_rect_intersects_work_region(center_x - radius, center_y - radius, radius * 2 + 1, radius * 2 + 1);
    }

    edge_radius = radius + 1;
    start_norm = egui_view_circle_dirty_normalize_angle(start_angle);
    end_norm = egui_view_circle_dirty_normalize_angle((int32_t)start_angle + sweep);

    egui_view_circle_dirty_get_circle_point(center_x, center_y, edge_radius, start_norm, start_x, start_y);
    egui_view_circle_dirty_get_circle_point(center_x, center_y, edge_radius, end_norm, end_x, end_y);

    bbox_min_x = EGUI_MIN(center_x, EGUI_MIN(*start_x, *end_x));
    bbox_min_y = EGUI_MIN(center_y, EGUI_MIN(*start_y, *end_y));
    bbox_max_x = EGUI_MAX(center_x, EGUI_MAX(*start_x, *end_x));
    bbox_max_y = EGUI_MAX(center_y, EGUI_MAX(*start_y, *end_y));

    for (i = 0; i < EGUI_ARRAY_SIZE(critical_angles); i++)
    {
        egui_dim_t axis_x;
        egui_dim_t axis_y;

        if (!egui_view_chart_pie_sweep_contains_angle(start_norm, sweep, critical_angles[i]))
        {
            continue;
        }

        egui_view_circle_dirty_get_circle_point(center_x, center_y, edge_radius, critical_angles[i], &axis_x, &axis_y);
        bbox_min_x = EGUI_MIN(bbox_min_x, axis_x);
        bbox_min_y = EGUI_MIN(bbox_min_y, axis_y);
        bbox_max_x = EGUI_MAX(bbox_max_x, axis_x);
        bbox_max_y = EGUI_MAX(bbox_max_y, axis_y);
    }

    return egui_view_chart_pie_rect_intersects_work_region(bbox_min_x - 1, bbox_min_y - 1, bbox_max_x - bbox_min_x + 3, bbox_max_y - bbox_min_y + 3);
}

static int egui_view_chart_pie_slice_intersects_work_region(const egui_view_chart_pie_work_region_info_t *work_info, egui_dim_t center_x, egui_dim_t center_y,
                                                            egui_dim_t radius, int16_t start_angle, uint16_t sweep)
{
    egui_region_t *work = (work_info != NULL) ? work_info->work : egui_canvas_get_base_view_work_region();
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

    if (work_info != NULL)
    {
        if (work_info->contains_center)
        {
            return 1;
        }

        if (work_info->has_angle_window && !egui_view_chart_pie_arc_intersects_arc(start_angle, sweep, work_info->angle_start, work_info->angle_sweep))
        {
            return 0;
        }

        if (work_info->inside_solid_circle && work_info->has_angle_window)
        {
            return 1;
        }
    }

    if (!egui_view_chart_pie_slice_bbox_intersects_work_region(center_x, center_y, radius, start_angle, sweep, &start_x, &start_y, &end_x, &end_y))
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

    if (egui_view_chart_pie_segment_intersects_region(center_x, center_y, start_x, start_y, work) ||
        egui_view_chart_pie_segment_intersects_region(center_x, center_y, end_x, end_y, work))
    {
        return 1;
    }

    if ((work_info != NULL && work_info->inside_solid_circle && sweep <= 180U) ||
        (work_info == NULL && sweep <= 180U && egui_view_chart_pie_is_work_region_inside_solid_circle(center_x, center_y, radius)))
    {
        return 0;
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

#define EGUI_VIEW_CHART_PIE_CORNER_INSIDE(_x, _y)                                                                                                              \
    (((int32_t)(_x) - center_x) * ((int32_t)(_x) - center_x) + ((int32_t)(_y) - center_y) * ((int32_t)(_y) - center_y) <= solid_radius_sq)

    return EGUI_VIEW_CHART_PIE_CORNER_INSIDE(x0, y0) && EGUI_VIEW_CHART_PIE_CORNER_INSIDE(x1, y0) && EGUI_VIEW_CHART_PIE_CORNER_INSIDE(x0, y1) &&
           EGUI_VIEW_CHART_PIE_CORNER_INSIDE(x1, y1);

#undef EGUI_VIEW_CHART_PIE_CORNER_INSIDE
}

static int egui_view_chart_pie_work_region_intersects_outer_circle(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius)
{
    egui_region_t *work = egui_canvas_get_base_view_work_region();
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x1;
    egui_dim_t y1;
    egui_dim_t nearest_x;
    egui_dim_t nearest_y;
    int32_t dx;
    int32_t dy;
    int32_t outer_radius;

    if (work == NULL || egui_region_is_empty(work))
    {
        return 0;
    }

    x0 = work->location.x;
    y0 = work->location.y;
    x1 = x0 + work->size.width - 1;
    y1 = y0 + work->size.height - 1;

    if (center_x < x0)
    {
        nearest_x = x0;
    }
    else if (center_x > x1)
    {
        nearest_x = x1;
    }
    else
    {
        nearest_x = center_x;
    }

    if (center_y < y0)
    {
        nearest_y = y0;
    }
    else if (center_y > y1)
    {
        nearest_y = y1;
    }
    else
    {
        nearest_y = center_y;
    }

    dx = (int32_t)nearest_x - center_x;
    dy = (int32_t)nearest_y - center_y;
    outer_radius = radius + 1;
    return dx * dx + dy * dy <= outer_radius * outer_radius;
}

static void egui_view_chart_pie_get_work_region_info(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_view_chart_pie_work_region_info_t *info)
{
    if (info == NULL)
    {
        return;
    }

    info->work = egui_canvas_get_base_view_work_region();
    info->contains_center = 0;
    info->inside_solid_circle = 0;
    info->has_angle_window = 0;
    info->angle_start = 0;
    info->angle_sweep = 0;

    if (info->work == NULL || egui_region_is_empty(info->work))
    {
        return;
    }

    info->contains_center = egui_view_chart_pie_point_in_region(info->work, center_x, center_y);
    info->inside_solid_circle = egui_view_chart_pie_is_work_region_inside_solid_circle(center_x, center_y, radius);
    if (!info->contains_center)
    {
        info->has_angle_window = egui_view_chart_pie_get_region_angle_window(info->work, center_x, center_y, &info->angle_start, &info->angle_sweep);
    }
}

// ============== Pie Chart Drawing ==============

static void egui_view_chart_pie_draw_pie(egui_view_chart_pie_t *local, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, uint32_t total)
{
    egui_view_chart_pie_work_region_info_t work_info;
    uint32_t angle_scale_q7 = 0;
    int use_angle_scale_q7 = 0;
    int use_angle_window_skip_path = 0;
    int window_wraps = 0;
    int16_t window_start_ext = 0;
    int16_t window_end_ext = 0;
    int16_t window_wrap_end = 0;

    if (local->pie_slice_count == 0 || local->pie_slices == NULL || total == 0 || radius <= 0)
    {
        return;
    }

    if (!egui_view_chart_pie_rect_intersects_work_region(center_x - radius, center_y - radius, radius * 2 + 1, radius * 2 + 1))
    {
        return;
    }

    if (!egui_view_chart_pie_work_region_intersects_outer_circle(center_x, center_y, radius))
    {
        return;
    }

    egui_view_chart_pie_get_work_region_info(center_x, center_y, radius, &work_info);
    if (total <= 65535U)
    {
        angle_scale_q7 = ((uint32_t)360 << 7) / total;
        use_angle_scale_q7 = 1;
    }
    if (!work_info.contains_center && work_info.has_angle_window)
    {
        int32_t ext_end;

        use_angle_window_skip_path = 1;
        window_start_ext = work_info.angle_start;
        if (window_start_ext < 270)
        {
            window_start_ext += 360;
        }

        ext_end = (int32_t)window_start_ext + work_info.angle_sweep;
        if (ext_end > 630)
        {
            window_wraps = 1;
            window_end_ext = 630;
            window_wrap_end = (int16_t)(ext_end - 360);
        }
        else
        {
            window_end_ext = (int16_t)ext_end;
        }
    }

    // Draw all slices as solid arcs so adjacent sectors share the same fill path.
    // Then cover the center with the first slice color to avoid the tiny spoke artifact.
    // Use cumulative proportion to avoid rounding error accumulation.
    int16_t start_angle = 270;
    uint32_t cumulative_value = 0;
    for (uint8_t i = 0; i < local->pie_slice_count; i++)
    {
        cumulative_value += local->pie_slices[i].value;
        int16_t end_angle;
        int16_t draw_end_angle;
        int16_t cull_end_angle;
        uint16_t cull_sweep;
        if (i == local->pie_slice_count - 1)
        {
            end_angle = 630; // ensure exact full circle
        }
        else if (use_angle_scale_q7)
        {
            end_angle = 270 + (int16_t)(((cumulative_value * angle_scale_q7) + 64U) >> 7);
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

        draw_end_angle = end_angle;
        if (i != local->pie_slice_count - 1 && draw_end_angle < 630)
        {
            draw_end_angle++;
        }

        cull_end_angle = end_angle;
        if (i != local->pie_slice_count - 1 && cull_end_angle < 629)
        {
            cull_end_angle += 2;
        }
        cull_sweep = (uint16_t)(cull_end_angle - start_angle);

        if (use_angle_window_skip_path)
        {
            if (!window_wraps)
            {
                if (cull_end_angle < window_start_ext)
                {
                    start_angle = end_angle;
                    continue;
                }
                if (start_angle > window_end_ext)
                {
                    break;
                }
            }
            else if (start_angle > window_wrap_end && end_angle < window_start_ext)
            {
                start_angle = end_angle;
                continue;
            }
        }
        else if (!egui_view_chart_pie_slice_intersects_work_region(&work_info, center_x, center_y, radius, start_angle, cull_sweep))
        {
            start_angle = end_angle;
            continue;
        }

        egui_canvas_draw_arc_fill_basic(center_x, center_y, radius, start_angle, draw_end_angle, local->pie_slices[i].color, EGUI_ALPHA_100);
        start_angle = end_angle;
    }

    if (!work_info.contains_center)
    {
        return;
    }

    // Cover the center neighborhood so the per-slice AA edges do not leave a visible spoke.
    egui_canvas_draw_circle_fill(center_x, center_y, 1, local->pie_slices[0].color, EGUI_ALPHA_100);
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
