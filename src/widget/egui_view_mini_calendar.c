#include <assert.h>
#include <string.h>

#include "egui_view_mini_calendar.h"
#include "utils/egui_sprintf.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12)
    {
        return 30;
    }
    uint8_t d = days[month - 1];
    if (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
    {
        d = 29;
    }
    return d;
}

// Sakamoto's algorithm - returns 0=Sunday..6=Saturday
static uint8_t day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 1 || month > 12)
    {
        return 0;
    }
    uint16_t y = year;
    if (month < 3)
    {
        y--;
    }
    return (uint8_t)((y + y / 4 - y / 100 + y / 400 + t[month - 1] + day) % 7);
}

static uint8_t is_weekend_col(uint8_t col, uint8_t first_day_of_week)
{
    // Map column back to actual day of week
    uint8_t dow = (col + first_day_of_week) % 7;
    return (dow == 0 || dow == 6) ? 1 : 0; // Sunday=0 or Saturday=6
}

static const char *const egui_view_mini_calendar_default_weekdays_sun[] = {"S", "M", "T", "W", "T", "F", "S"};
static const char *const egui_view_mini_calendar_default_weekdays_mon[] = {"M", "T", "W", "T", "F", "S", "S"};

static const char *const *egui_view_mini_calendar_get_weekday_labels(egui_view_mini_calendar_t *local)
{
    if (local->weekday_labels[0] != NULL)
    {
        return local->weekday_labels;
    }

    return local->first_day_of_week == 1 ? egui_view_mini_calendar_default_weekdays_mon : egui_view_mini_calendar_default_weekdays_sun;
}

static uint8_t egui_view_mini_calendar_hit_day(egui_view_t *self, egui_view_mini_calendar_t *local, egui_dim_t touch_x, egui_dim_t touch_y)
{
    egui_dim_t w = self->region.size.width;
    egui_dim_t h = self->region.size.height;
    egui_dim_t cell_w = w / 7;
    egui_dim_t header_h = h / 8;
    egui_dim_t cell_h = (h - header_h * 2) / 6;
    egui_dim_t local_x = touch_x - self->region_screen.location.x;
    egui_dim_t local_y = touch_y - self->region_screen.location.y;
    egui_dim_t grid_y_start = header_h * 2;
    uint8_t first_dow;
    uint8_t start_col;
    uint8_t pos;
    uint8_t total_days;
    uint8_t col;
    uint8_t row;

    if (local_x < 0 || local_y < grid_y_start || local_x >= w || local_y >= h || cell_w == 0 || cell_h == 0)
    {
        return 0;
    }

    col = (uint8_t)(local_x / cell_w);
    row = (uint8_t)((local_y - grid_y_start) / cell_h);
    if (col >= 7 || row >= 6)
    {
        return 0;
    }

    first_dow = day_of_week(local->year, local->month, 1);
    start_col = (first_dow - local->first_day_of_week + 7) % 7;
    pos = row * 7 + col;
    total_days = days_in_month(local->year, local->month);
    if (pos < start_col || pos >= start_col + total_days)
    {
        return 0;
    }

    return (uint8_t)(pos - start_col + 1);
}

static int egui_view_mini_calendar_get_day_cell_region(egui_view_t *self, egui_view_mini_calendar_t *local, uint8_t day, egui_region_t *out_region)
{
    egui_region_t region;
    egui_dim_t w;
    egui_dim_t h;
    egui_dim_t cell_w;
    egui_dim_t header_h;
    egui_dim_t cell_h;
    uint8_t first_dow;
    uint8_t start_col;
    uint8_t total_days;
    uint8_t pos;
    uint8_t col;
    uint8_t row;

    if (out_region == NULL)
    {
        return 0;
    }

    egui_view_get_work_region(self, &region);
    w = region.size.width;
    h = region.size.height;
    cell_w = w / 7;
    header_h = h / 8;
    cell_h = (h - header_h * 2) / 6;
    total_days = days_in_month(local->year, local->month);

    if (day < 1 || day > total_days || cell_w <= 0 || cell_h <= 0)
    {
        egui_region_init_empty(out_region);
        return 0;
    }

    first_dow = day_of_week(local->year, local->month, 1);
    start_col = (first_dow - local->first_day_of_week + 7) % 7;
    pos = start_col + day - 1;
    col = pos % 7;
    row = pos / 7;

    out_region->location.x = region.location.x + col * cell_w;
    out_region->location.y = region.location.y + header_h * 2 + row * cell_h;
    out_region->size.width = cell_w;
    out_region->size.height = cell_h;

    return 1;
}

static void egui_view_mini_calendar_local_region_to_screen(egui_view_t *self, const egui_region_t *local_region, egui_region_t *screen_region)
{
    screen_region->location.x = self->region_screen.location.x + local_region->location.x;
    screen_region->location.y = self->region_screen.location.y + local_region->location.y;
    screen_region->size.width = local_region->size.width;
    screen_region->size.height = local_region->size.height;
}

void egui_view_mini_calendar_set_date(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day)
{
    EGUI_LOCAL_INIT(egui_view_mini_calendar_t);
    local->year = year;
    local->month = month;
    local->day = day;
    egui_view_invalidate(self);
}

void egui_view_mini_calendar_set_today(egui_view_t *self, uint8_t day)
{
    EGUI_LOCAL_INIT(egui_view_mini_calendar_t);
    local->today_day = day;
    egui_view_invalidate(self);
}

void egui_view_mini_calendar_set_first_day_of_week(egui_view_t *self, uint8_t day)
{
    EGUI_LOCAL_INIT(egui_view_mini_calendar_t);
    local->first_day_of_week = day;
    egui_view_invalidate(self);
}

void egui_view_mini_calendar_set_weekday_labels(egui_view_t *self, const char *const *labels)
{
    EGUI_LOCAL_INIT(egui_view_mini_calendar_t);

    if (labels == NULL)
    {
        memset(local->weekday_labels, 0, sizeof(local->weekday_labels));
    }
    else
    {
        uint8_t i;

        for (i = 0; i < 7; i++)
        {
            local->weekday_labels[i] = labels[i];
        }
    }

    egui_view_invalidate(self);
}

void egui_view_mini_calendar_set_on_date_selected_listener(egui_view_t *self, egui_view_on_date_selected_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_mini_calendar_t);
    local->on_date_selected = listener;
}

void egui_view_mini_calendar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_mini_calendar_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t x = region.location.x;
    egui_dim_t y = region.location.y;
    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;

    egui_dim_t cell_w = w / 7;
    egui_dim_t header_h = h / 8;
    egui_dim_t cell_h = (h - header_h * 2) / 6;

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(local->bg_color, EGUI_COLOR_WHITE, 40);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = local->bg_color},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = stops,
        };
        egui_canvas_draw_round_rectangle_fill_gradient(x, y, w, h, EGUI_THEME_RADIUS_MD, &grad);
    }
#else
    egui_canvas_draw_round_rectangle_fill(x, y, w, h, EGUI_THEME_RADIUS_MD, local->bg_color, EGUI_ALPHA_100);
#endif
    egui_canvas_draw_round_rectangle(x, y, w, h, EGUI_THEME_RADIUS_MD, EGUI_THEME_STROKE_WIDTH, EGUI_THEME_BORDER, EGUI_ALPHA_100);

    const egui_font_t *font = local->font ? local->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    // Row 1: Month title "2026-02"
    char buf[16];
    {
        int pos = 0;
        pos += egui_sprintf_int_pad(&buf[pos], (int)sizeof(buf) - pos, local->year, 4, '0');
        pos += egui_sprintf_char(&buf[pos], (int)sizeof(buf) - pos, '-');
        pos += egui_sprintf_int_pad(&buf[pos], (int)sizeof(buf) - pos, local->month, 2, '0');
    }
    egui_region_t title_rect = {{x, y}, {w, header_h}};
    egui_canvas_draw_text_in_rect(font, buf, &title_rect, EGUI_ALIGN_CENTER, local->header_color, EGUI_ALPHA_100);

    // Row 2: Weekday headers
    const char *const *weekdays = egui_view_mini_calendar_get_weekday_labels(local);

    uint8_t col;
    for (col = 0; col < 7; col++)
    {
        egui_region_t cell_rect = {{x + col * cell_w, y + header_h}, {cell_w, header_h}};
        egui_canvas_draw_text_in_rect(font, weekdays[col], &cell_rect, EGUI_ALIGN_CENTER, local->header_color, EGUI_ALPHA_100);
    }

    // Date grid (6 rows x 7 cols)
    uint8_t first_dow = day_of_week(local->year, local->month, 1);
    uint8_t start_col = (first_dow - local->first_day_of_week + 7) % 7;
    uint8_t total_days = days_in_month(local->year, local->month);

    uint8_t d;
    for (d = 1; d <= total_days; d++)
    {
        uint8_t pos = start_col + d - 1;
        egui_region_t day_rect;
        egui_region_t day_screen_rect;
        col = pos % 7;

        if (!egui_view_mini_calendar_get_day_cell_region(self, local, d, &day_rect))
        {
            continue;
        }

        egui_view_mini_calendar_local_region_to_screen(self, &day_rect, &day_screen_rect);
        if (!egui_canvas_is_region_active(&day_screen_rect))
        {
            continue;
        }

        egui_dim_t cx = day_rect.location.x + day_rect.size.width / 2;
        egui_dim_t cy = day_rect.location.y + day_rect.size.height / 2;
        egui_dim_t r = cell_h / 2 - 1;
        if (r < 1)
        {
            r = 1;
        }

        egui_sprintf_int(buf, sizeof(buf), d);

        if (d == local->today_day)
        {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
            {
                egui_color_t color_light = egui_rgb_mix(local->today_color, EGUI_COLOR_WHITE, 80);
                egui_gradient_stop_t stops[2] = {
                        {.position = 0, .color = color_light},
                        {.position = 255, .color = local->today_color},
                };
                egui_gradient_t grad = {
                        .type = EGUI_GRADIENT_TYPE_RADIAL,
                        .stop_count = 2,
                        .alpha = EGUI_ALPHA_100,
                        .stops = stops,
                };
                egui_canvas_draw_circle_fill_gradient(cx, cy, r, &grad);
            }
#else
            egui_canvas_draw_circle_fill(cx, cy, r, local->today_color, EGUI_ALPHA_100);
#endif
            egui_canvas_draw_text_in_rect(font, buf, &day_rect, EGUI_ALIGN_CENTER, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
        }
        else if (d == local->day)
        {
            egui_canvas_draw_circle(cx, cy, r, 1, local->selected_color, EGUI_ALPHA_100);
            egui_canvas_draw_text_in_rect(font, buf, &day_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
        }
        else if (is_weekend_col(col, local->first_day_of_week))
        {
            egui_canvas_draw_text_in_rect(font, buf, &day_rect, EGUI_ALIGN_CENTER, local->weekend_color, EGUI_ALPHA_100);
        }
        else
        {
            egui_canvas_draw_text_in_rect(font, buf, &day_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_mini_calendar_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_mini_calendar_t);
    uint8_t hit_day;

    if (self->is_enable == false)
    {
        return 0;
    }

    hit_day = egui_view_mini_calendar_hit_day(self, local, event->location.x, event->location.y);

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        local->pressed_day = hit_day;
        egui_view_set_pressed_with_region(self, hit_day != 0, NULL);
        break;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        egui_view_set_pressed_with_region(self, local->pressed_day != 0 && local->pressed_day == hit_day, NULL);
        break;
    case EGUI_MOTION_EVENT_ACTION_UP:
    {
        int was_pressed = self->is_pressed;
        uint8_t pressed_day = local->pressed_day;
        egui_region_t day_region;

        egui_view_set_pressed_with_region(self, false, NULL);
        local->pressed_day = 0;
        if (was_pressed && pressed_day != 0 && pressed_day == hit_day && hit_day != local->day)
        {
            if (egui_view_mini_calendar_get_day_cell_region(self, local, local->day, &day_region))
            {
                egui_view_invalidate_region(self, &day_region);
            }

            local->day = hit_day;

            if (egui_view_mini_calendar_get_day_cell_region(self, local, hit_day, &day_region))
            {
                egui_view_invalidate_region(self, &day_region);
            }

            if (local->on_date_selected)
            {
                local->on_date_selected(self, hit_day);
            }
        }
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_day = 0;
        egui_view_set_pressed_with_region(self, false, NULL);
        break;
    default:
        break;
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_mini_calendar_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_mini_calendar_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_mini_calendar_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_mini_calendar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_mini_calendar_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_mini_calendar_t);

    // init local data.
    local->bg_color = EGUI_THEME_SURFACE;
    local->year = 2026;
    local->month = 1;
    local->day = 1;
    local->today_day = 0;
    local->pressed_day = 0;
    local->first_day_of_week = 0;
    local->header_color = EGUI_THEME_TEXT_PRIMARY;
    local->text_color = EGUI_THEME_TEXT_PRIMARY;
    local->today_color = EGUI_THEME_PRIMARY;
    local->selected_color = EGUI_THEME_SECONDARY;
    local->weekend_color = EGUI_THEME_TEXT_SECONDARY;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    memset(local->weekday_labels, 0, sizeof(local->weekday_labels));
    local->on_date_selected = NULL;

    egui_view_set_view_name(self, "egui_view_mini_calendar");
}

void egui_view_mini_calendar_apply_params(egui_view_t *self, const egui_view_mini_calendar_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_mini_calendar_t);

    self->region = params->region;

    local->year = params->year;
    local->month = params->month;
    local->day = params->day;

    egui_view_invalidate(self);
}

void egui_view_mini_calendar_init_with_params(egui_view_t *self, const egui_view_mini_calendar_params_t *params)
{
    egui_view_mini_calendar_init(self);
    egui_view_mini_calendar_apply_params(self, params);
}
