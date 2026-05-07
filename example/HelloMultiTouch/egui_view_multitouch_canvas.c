#include "egui_view_multitouch_canvas.h"

#include <stdio.h>

#include "egui.h"

#define MT_BG_COLOR      EGUI_COLOR_MAKE(14, 18, 24)
#define MT_PANEL_COLOR   EGUI_COLOR_MAKE(24, 31, 42)
#define MT_GRID_COLOR    EGUI_COLOR_MAKE(48, 58, 72)
#define MT_TEXT_COLOR    EGUI_COLOR_MAKE(222, 231, 242)
#define MT_MUTED_COLOR   EGUI_COLOR_MAKE(143, 157, 176)
#define MT_LINK_COLOR    EGUI_COLOR_MAKE(98, 218, 155)
#define MT_HEADER_HEIGHT 42
#define MT_STATUS_HEIGHT 36
#define MT_CANVAS_MARGIN 8

static const egui_color_t mt_trace_colors[] = {
        EGUI_COLOR_MAKE(64, 169, 255), EGUI_COLOR_MAKE(255, 177, 66),  EGUI_COLOR_MAKE(98, 218, 155),
        EGUI_COLOR_MAKE(244, 91, 105), EGUI_COLOR_MAKE(183, 132, 255),
};

static int16_t mt_abs16(int16_t v)
{
    return v < 0 ? (int16_t)-v : v;
}

static int16_t mt_isqrt32(int32_t value)
{
    int32_t bit = 1L << 30;
    int32_t result = 0;

    if (value <= 0)
    {
        return 0;
    }

    while (bit > value)
    {
        bit >>= 2;
    }

    while (bit != 0)
    {
        if (value >= result + bit)
        {
            value -= result + bit;
            result = (result >> 1) + bit;
        }
        else
        {
            result >>= 1;
        }
        bit >>= 2;
    }

    return (int16_t)result;
}

static int16_t mt_distance(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2)
{
    int32_t dx = (int32_t)x2 - (int32_t)x1;
    int32_t dy = (int32_t)y2 - (int32_t)y1;
    return mt_isqrt32(dx * dx + dy * dy);
}

static egui_color_t mt_trace_color(uint8_t index)
{
    return mt_trace_colors[index % EGUI_ARRAY_SIZE(mt_trace_colors)];
}

static void mt_get_canvas_region(egui_view_t *self, egui_region_t *canvas_region)
{
    egui_dim_t width;
    egui_dim_t height;

    if (canvas_region == NULL)
    {
        return;
    }

    if (self == NULL)
    {
        egui_region_init_empty(canvas_region);
        return;
    }

    width = self->region_screen.size.width - MT_CANVAS_MARGIN * 2;
    height = self->region_screen.size.height - MT_HEADER_HEIGHT - MT_STATUS_HEIGHT - MT_CANVAS_MARGIN * 2;
    if (width < 0)
    {
        width = 0;
    }
    if (height < 0)
    {
        height = 0;
    }

    egui_region_init(canvas_region, self->region_screen.location.x + MT_CANVAS_MARGIN, self->region_screen.location.y + MT_HEADER_HEIGHT + MT_CANVAS_MARGIN,
                     width, height);
}

static uint8_t mt_event_pointer_count(const egui_motion_event_t *event)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    if (event->pointer_count > EGUI_CONFIG_TOUCH_MAX_POINTS)
    {
        return EGUI_CONFIG_TOUCH_MAX_POINTS;
    }
    return event->pointer_count;
#else
    EGUI_UNUSED(event);
    return 1;
#endif
}

static egui_location_t mt_event_location_at(const egui_motion_event_t *event, uint8_t index)
{
    egui_location_t location = event->location;

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    if (index < event->pointer_count)
    {
        location = event->locations[index];
    }
#else
    EGUI_UNUSED(index);
#endif
    return location;
}

static uint8_t mt_collect_canvas_points(const egui_motion_event_t *event, const egui_region_t *canvas_region, egui_location_t *points)
{
    uint8_t point_count;
    uint8_t canvas_point_count = 0;

    if (event == NULL || canvas_region == NULL || points == NULL || egui_region_is_empty((egui_region_t *)canvas_region))
    {
        return 0;
    }

    point_count = mt_event_pointer_count(event);
    for (uint8_t i = 0; i < point_count; i++)
    {
        egui_location_t location = mt_event_location_at(event, i);

        if (!egui_region_pt_in_rect(canvas_region, location.x, location.y))
        {
            continue;
        }

        points[canvas_point_count] = location;
        canvas_point_count++;
        if (canvas_point_count >= EGUI_CONFIG_TOUCH_MAX_POINTS)
        {
            break;
        }
    }

    return canvas_point_count;
}

static void mt_trace_append(multitouch_trace_point_t *trace, uint16_t *count, egui_dim_t x, egui_dim_t y)
{
    if (trace == NULL || count == NULL)
    {
        return;
    }

    if (*count > 0 && trace[*count - 1U].x == x && trace[*count - 1U].y == y)
    {
        return;
    }

    if (*count < MULTITOUCH_TRACE_MAX_POINTS)
    {
        trace[*count].x = x;
        trace[*count].y = y;
        (*count)++;
        return;
    }

    for (uint16_t i = 1; i < MULTITOUCH_TRACE_MAX_POINTS; i++)
    {
        trace[i - 1U] = trace[i];
    }
    trace[MULTITOUCH_TRACE_MAX_POINTS - 1U].x = x;
    trace[MULTITOUCH_TRACE_MAX_POINTS - 1U].y = y;
}

static void mt_trace_clear(egui_view_multitouch_canvas_t *local)
{
    for (uint8_t i = 0; i < EGUI_CONFIG_TOUCH_MAX_POINTS; i++)
    {
        local->trace_counts[i] = 0;
    }
}

static void mt_draw_trace(egui_canvas_t *canvas, const multitouch_trace_point_t *trace, uint16_t count, egui_color_t color)
{
    if (count == 0)
    {
        return;
    }

    if (count == 1)
    {
        egui_canvas_draw_circle_fill(canvas, trace[0].x, trace[0].y, 3, color, EGUI_ALPHA_100);
        return;
    }

    for (uint16_t i = 1; i < count; i++)
    {
        egui_canvas_draw_line_round_cap_hq(canvas, trace[i - 1U].x, trace[i - 1U].y, trace[i].x, trace[i].y, 5, color, EGUI_ALPHA_90);
    }
}

static uint8_t mt_has_any_trace(const egui_view_multitouch_canvas_t *local)
{
    for (uint8_t i = 0; i < EGUI_CONFIG_TOUCH_MAX_POINTS; i++)
    {
        if (local->trace_counts[i] > 0)
        {
            return 1;
        }
    }
    return 0;
}

static const char *mt_event_name(uint8_t type)
{
    switch (type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        return "DOWN";
    case EGUI_MOTION_EVENT_ACTION_UP:
        return "UP";
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        return "MOVE";
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        return "CANCEL";
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    case EGUI_MOTION_EVENT_ACTION_POINTER_DOWN:
        return "POINTER_DOWN";
    case EGUI_MOTION_EVENT_ACTION_POINTER_UP:
        return "POINTER_UP";
#endif
    default:
        return "NONE";
    }
}

static void mt_draw_header(egui_canvas_t *canvas, egui_view_t *self, egui_view_multitouch_canvas_t *local)
{
    char text[64];
    egui_region_t rect;

    egui_canvas_draw_rectangle_fill(canvas, self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width, MT_HEADER_HEIGHT,
                                    MT_PANEL_COLOR, EGUI_ALPHA_100);
    egui_canvas_draw_text(canvas, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, "Multi-touch Canvas", self->region_screen.location.x + 10,
                          self->region_screen.location.y + 8, MT_TEXT_COLOR, EGUI_ALPHA_100);

    snprintf(text, sizeof(text), "%s  points:%u", mt_event_name(local->last_event_type), (unsigned)local->active_points);
    egui_region_init(&rect, self->region_screen.location.x + 158, self->region_screen.location.y + 7, self->region_screen.size.width - 168, 22);
    egui_canvas_draw_text_in_rect(canvas, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, text, &rect, EGUI_ALIGN_RIGHT, MT_MUTED_COLOR, EGUI_ALPHA_100);
}

static void mt_draw_status(egui_canvas_t *canvas, egui_view_t *self, egui_view_multitouch_canvas_t *local)
{
    char text[80];
    egui_dim_t y = self->region_screen.location.y + self->region_screen.size.height - MT_STATUS_HEIGHT;

    egui_canvas_draw_rectangle_fill(canvas, self->region_screen.location.x, y, self->region_screen.size.width, MT_STATUS_HEIGHT, MT_PANEL_COLOR,
                                    EGUI_ALPHA_100);
    snprintf(text, sizeof(text), "events:%u  scale:%d%%  dist:%d", (unsigned)local->event_count, (int)local->scale_percent, (int)local->pinch_current_distance);
    egui_canvas_draw_text(canvas, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, text, self->region_screen.location.x + 10, y + 9, MT_TEXT_COLOR,
                          EGUI_ALPHA_100);
}

static void mt_draw_grid(egui_canvas_t *canvas, egui_region_t *canvas_region)
{
    egui_canvas_draw_rectangle_fill(canvas, canvas_region->location.x, canvas_region->location.y, canvas_region->size.width, canvas_region->size.height,
                                    MT_BG_COLOR, EGUI_ALPHA_100);

    for (egui_dim_t x = canvas_region->location.x + 20; x < canvas_region->location.x + canvas_region->size.width; x += 20)
    {
        egui_canvas_draw_line(canvas, x, canvas_region->location.y, x, canvas_region->location.y + canvas_region->size.height - 1, 1, MT_GRID_COLOR,
                              EGUI_ALPHA_40);
    }
    for (egui_dim_t y = canvas_region->location.y + 20; y < canvas_region->location.y + canvas_region->size.height; y += 20)
    {
        egui_canvas_draw_line(canvas, canvas_region->location.x, y, canvas_region->location.x + canvas_region->size.width - 1, y, 1, MT_GRID_COLOR,
                              EGUI_ALPHA_40);
    }

    egui_canvas_draw_rectangle(canvas, canvas_region->location.x, canvas_region->location.y, canvas_region->size.width, canvas_region->size.height, 1,
                               MT_GRID_COLOR, EGUI_ALPHA_100);
}

static void mt_draw_touch_points(egui_canvas_t *canvas, egui_view_multitouch_canvas_t *local)
{
    if (local->active_points >= 2)
    {
        egui_dim_t cx = (egui_dim_t)(((int32_t)local->last_points[0].x + (int32_t)local->last_points[1].x) / 2);
        egui_dim_t cy = (egui_dim_t)(((int32_t)local->last_points[0].y + (int32_t)local->last_points[1].y) / 2);
        egui_dim_t ring_radius = (egui_dim_t)(mt_abs16(local->pinch_current_distance) / 2);

        if (ring_radius > 6)
        {
            egui_canvas_draw_circle(canvas, cx, cy, ring_radius, 2, MT_LINK_COLOR, EGUI_ALPHA_70);
        }
        egui_canvas_draw_line_round_cap_hq(canvas, local->last_points[0].x, local->last_points[0].y, local->last_points[1].x, local->last_points[1].y, 3,
                                           MT_LINK_COLOR, EGUI_ALPHA_80);
    }

    for (uint8_t i = 0; i < local->active_points && i < EGUI_CONFIG_TOUCH_MAX_POINTS; i++)
    {
        egui_color_t color = mt_trace_color(i);
        egui_canvas_draw_circle_fill(canvas, local->last_points[i].x, local->last_points[i].y, 10, color, EGUI_ALPHA_60);
        egui_canvas_draw_circle(canvas, local->last_points[i].x, local->last_points[i].y, 14, 2, color, EGUI_ALPHA_100);
    }
}

static void egui_view_multitouch_canvas_on_draw(egui_view_t *self)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_view_multitouch_canvas_t *local = (egui_view_multitouch_canvas_t *)self;
    egui_region_t canvas_region;
    const egui_region_t *prev_clip = NULL;
    egui_region_t trace_clip;

    mt_get_canvas_region(self, &canvas_region);

    egui_canvas_draw_rectangle_fill(canvas, self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                    self->region_screen.size.height, MT_BG_COLOR, EGUI_ALPHA_100);
    mt_draw_header(canvas, self, local);
    mt_draw_grid(canvas, &canvas_region);

    if (!mt_has_any_trace(local))
    {
        egui_canvas_draw_text(canvas, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, "Drag one finger, pinch two, or paint with five.",
                              canvas_region.location.x + 12, canvas_region.location.y + 16, MT_MUTED_COLOR, EGUI_ALPHA_100);
    }

    prev_clip = egui_canvas_get_extra_clip(canvas);
    if (prev_clip != NULL)
    {
        egui_region_t prev_clip_copy = *prev_clip;
        egui_region_intersect(&prev_clip_copy, &canvas_region, &trace_clip);
    }
    else
    {
        trace_clip = canvas_region;
    }
    egui_canvas_set_extra_clip(canvas, &trace_clip);
    egui_canvas_calc_work_region(canvas, &self->region_screen);

    for (uint8_t i = 0; i < EGUI_CONFIG_TOUCH_MAX_POINTS; i++)
    {
        mt_draw_trace(canvas, local->traces[i], local->trace_counts[i], mt_trace_color(i));
    }
    mt_draw_touch_points(canvas, local);

    if (prev_clip != NULL)
    {
        egui_canvas_set_extra_clip(canvas, prev_clip);
    }
    else
    {
        egui_canvas_clear_extra_clip(canvas);
    }
    egui_canvas_calc_work_region(canvas, &self->region_screen);
    mt_draw_status(canvas, self, local);
}

static int egui_view_multitouch_canvas_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    egui_view_multitouch_canvas_t *local = (egui_view_multitouch_canvas_t *)self;
    egui_region_t canvas_region;
    egui_location_t canvas_points[EGUI_CONFIG_TOUCH_MAX_POINTS];
    uint8_t canvas_point_count;

    if (event == NULL)
    {
        return 0;
    }

    mt_get_canvas_region(self, &canvas_region);
    canvas_point_count = mt_collect_canvas_points(event, &canvas_region, canvas_points);

    if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        if (canvas_point_count == 0)
        {
            local->is_tracking = 0;
            return 0;
        }
        local->is_tracking = 1;
    }
    else if (!local->is_tracking)
    {
        return 0;
    }

    local->event_count++;
    local->last_event_type = event->type;
    local->active_points = canvas_point_count;
    for (uint8_t i = 0; i < local->active_points; i++)
    {
        local->last_points[i] = canvas_points[i];
    }
    for (uint8_t i = local->active_points; i < EGUI_CONFIG_TOUCH_MAX_POINTS; i++)
    {
        local->last_points[i].x = 0;
        local->last_points[i].y = 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        mt_trace_clear(local);
        local->is_pinching = 0;
        local->pinch_start_distance = 0;
        local->pinch_current_distance = 0;
        local->scale_percent = 100;
        if (local->active_points > 0)
        {
            mt_trace_append(local->traces[0], &local->trace_counts[0], local->last_points[0].x, local->last_points[0].y);
        }
        break;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        for (uint8_t i = 0; i < local->active_points; i++)
        {
            mt_trace_append(local->traces[i], &local->trace_counts[i], local->last_points[i].x, local->last_points[i].y);
        }
        if (local->active_points >= 2)
        {
            local->pinch_current_distance = mt_distance(local->last_points[0].x, local->last_points[0].y, local->last_points[1].x, local->last_points[1].y);
            if (local->pinch_start_distance > 0)
            {
                local->scale_percent = (int16_t)((int32_t)local->pinch_current_distance * 100 / local->pinch_start_distance);
            }
        }
        break;
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    case EGUI_MOTION_EVENT_ACTION_POINTER_DOWN:
        for (uint8_t i = 0; i < local->active_points; i++)
        {
            mt_trace_append(local->traces[i], &local->trace_counts[i], local->last_points[i].x, local->last_points[i].y);
        }
        if (local->active_points >= 2)
        {
            local->is_pinching = 1;
            local->pinch_start_distance = mt_distance(local->last_points[0].x, local->last_points[0].y, local->last_points[1].x, local->last_points[1].y);
            if (local->pinch_start_distance <= 0)
            {
                local->pinch_start_distance = 1;
            }
            local->pinch_current_distance = local->pinch_start_distance;
            local->scale_percent = 100;
        }
        break;
    case EGUI_MOTION_EVENT_ACTION_POINTER_UP:
        local->is_pinching = 0;
        if (local->active_points > 1)
        {
            local->active_points--;
        }
        break;
#endif
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->is_tracking = 0;
        local->active_points = 0;
        local->is_pinching = 0;
        break;
    default:
        break;
    }

    egui_view_invalidate(self);
    return 1;
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_multitouch_canvas_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_multitouch_canvas_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_multitouch_canvas_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
};

void egui_view_multitouch_canvas_init(egui_view_t *self, egui_core_t *core)
{
    egui_view_multitouch_canvas_t *local = (egui_view_multitouch_canvas_t *)self;

    egui_view_init(self, core);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_multitouch_canvas_t);
    self->is_clickable = 1;

    mt_trace_clear(local);
    local->active_points = 0;
    local->is_tracking = 0;
    local->is_pinching = 0;
    local->last_event_type = EGUI_MOTION_EVENT_NONE;
    local->event_count = 0;
    for (uint8_t i = 0; i < EGUI_CONFIG_TOUCH_MAX_POINTS; i++)
    {
        local->last_points[i].x = 0;
        local->last_points[i].y = 0;
    }
    local->pinch_start_distance = 0;
    local->pinch_current_distance = 0;
    local->scale_percent = 100;
}
