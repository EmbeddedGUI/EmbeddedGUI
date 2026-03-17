#include "egui_view_calendar_view.h"

#define EGUI_VIEW_CALENDAR_VIEW_STANDARD_RADIUS        10
#define EGUI_VIEW_CALENDAR_VIEW_STANDARD_FILL_ALPHA    94
#define EGUI_VIEW_CALENDAR_VIEW_STANDARD_BORDER_ALPHA  60
#define EGUI_VIEW_CALENDAR_VIEW_STANDARD_PAD_X         10
#define EGUI_VIEW_CALENDAR_VIEW_STANDARD_PAD_Y         8
#define EGUI_VIEW_CALENDAR_VIEW_STANDARD_LABEL_HEIGHT  10
#define EGUI_VIEW_CALENDAR_VIEW_STANDARD_LABEL_GAP     4
#define EGUI_VIEW_CALENDAR_VIEW_STANDARD_HEADER_HEIGHT 18
#define EGUI_VIEW_CALENDAR_VIEW_STANDARD_WEEK_HEIGHT   10
#define EGUI_VIEW_CALENDAR_VIEW_STANDARD_HELPER_GAP    5
#define EGUI_VIEW_CALENDAR_VIEW_STANDARD_HELPER_HEIGHT 10

#define EGUI_VIEW_CALENDAR_VIEW_COMPACT_RADIUS         8
#define EGUI_VIEW_CALENDAR_VIEW_COMPACT_FILL_ALPHA     92
#define EGUI_VIEW_CALENDAR_VIEW_COMPACT_BORDER_ALPHA   56
#define EGUI_VIEW_CALENDAR_VIEW_COMPACT_PAD_X          7
#define EGUI_VIEW_CALENDAR_VIEW_COMPACT_PAD_Y          6
#define EGUI_VIEW_CALENDAR_VIEW_COMPACT_HEADER_HEIGHT  14
#define EGUI_VIEW_CALENDAR_VIEW_COMPACT_SUMMARY_HEIGHT 18

typedef struct egui_view_calendar_view_metrics egui_view_calendar_view_metrics_t;
struct egui_view_calendar_view_metrics
{
    egui_region_t content_region;
    egui_region_t label_region;
    egui_region_t header_region;
    egui_region_t helper_region;
    egui_region_t week_region;
    egui_region_t grid_region;
    egui_region_t summary_region;
    egui_region_t prev_region;
    egui_region_t next_region;
    egui_region_t title_region;
    uint8_t show_label;
    uint8_t show_helper;
    uint8_t show_grid;
};

static const char *calendar_view_month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char *calendar_view_weekday_names_sun[] = {"S", "M", "T", "W", "T", "F", "S"};
static const char *calendar_view_weekday_names_mon[] = {"M", "T", "W", "T", "F", "S", "S"};

static uint8_t calendar_view_is_leap_year(uint16_t year)
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 1 : 0;
}

static uint8_t calendar_view_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month < 1 || month > 12)
    {
        return 30;
    }
    if (month == 2 && calendar_view_is_leap_year(year))
    {
        return 29;
    }
    return days[month - 1];
}

static uint8_t calendar_view_day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    static const int offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint16_t y = year;

    if (month < 1 || month > 12)
    {
        return 0;
    }
    if (month < 3)
    {
        y--;
    }
    return (uint8_t)((y + y / 4 - y / 100 + y / 400 + offsets[month - 1] + day) % 7);
}

static void calendar_view_normalize_display_month(uint16_t *year, uint8_t *month)
{
    if (*year < 1)
    {
        *year = 1;
    }

    if (*month < 1)
    {
        *month = 1;
    }
    else if (*month > 12)
    {
        *month = 12;
    }
}

static void calendar_view_normalize_day_pair(uint16_t year, uint8_t month, uint8_t *start_day, uint8_t *end_day)
{
    uint8_t max_day = calendar_view_days_in_month(year, month);
    uint8_t temp;

    if (*start_day < 1)
    {
        *start_day = 1;
    }
    else if (*start_day > max_day)
    {
        *start_day = max_day;
    }

    if (*end_day < 1)
    {
        *end_day = 1;
    }
    else if (*end_day > max_day)
    {
        *end_day = max_day;
    }

    if (*start_day > *end_day)
    {
        temp = *start_day;
        *start_day = *end_day;
        *end_day = temp;
    }
}

static egui_color_t calendar_view_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static uint8_t calendar_view_region_contains_point(const egui_region_t *region, egui_dim_t x, egui_dim_t y)
{
    return (x >= region->location.x && x < region->location.x + region->size.width && y >= region->location.y && y < region->location.y + region->size.height)
                   ? 1
                   : 0;
}

static void calendar_view_format_month_title(uint16_t year, uint8_t month, char *buffer)
{
    buffer[0] = calendar_view_month_names[month - 1][0];
    buffer[1] = calendar_view_month_names[month - 1][1];
    buffer[2] = calendar_view_month_names[month - 1][2];
    buffer[3] = ' ';
    buffer[4] = (char)('0' + (year / 1000) % 10);
    buffer[5] = (char)('0' + (year / 100) % 10);
    buffer[6] = (char)('0' + (year / 10) % 10);
    buffer[7] = (char)('0' + year % 10);
    buffer[8] = '\0';
}

static void calendar_view_format_range_summary(uint16_t year, uint8_t month, uint8_t start_day, uint8_t end_day, char *buffer)
{
    uint8_t pos = 0;

    EGUI_UNUSED(year);
    buffer[pos++] = calendar_view_month_names[month - 1][0];
    buffer[pos++] = calendar_view_month_names[month - 1][1];
    buffer[pos++] = calendar_view_month_names[month - 1][2];
    buffer[pos++] = ' ';
    buffer[pos++] = (char)('0' + (start_day / 10) % 10);
    buffer[pos++] = (char)('0' + start_day % 10);
    if (start_day != end_day)
    {
        buffer[pos++] = '-';
        buffer[pos++] = (char)('0' + (end_day / 10) % 10);
        buffer[pos++] = (char)('0' + end_day % 10);
    }
    buffer[pos] = '\0';
}

static void calendar_view_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                    egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void calendar_view_draw_chevron(egui_view_t *self, const egui_region_t *region, egui_color_t color, uint8_t direction)
{
    egui_dim_t cx = region->location.x + region->size.width / 2;
    egui_dim_t cy = region->location.y + region->size.height / 2;
    egui_alpha_t alpha = egui_color_alpha_mix(self->alpha, 90);

    if (direction == 0)
    {
        egui_canvas_draw_line(cx + 2, cy - 3, cx - 1, cy, 1, color, alpha);
        egui_canvas_draw_line(cx - 1, cy, cx + 2, cy + 3, 1, color, alpha);
    }
    else
    {
        egui_canvas_draw_line(cx - 2, cy - 3, cx + 1, cy, 1, color, alpha);
        egui_canvas_draw_line(cx + 1, cy, cx - 2, cy + 3, 1, color, alpha);
    }
}

static uint8_t calendar_view_selection_visible(egui_view_calendar_view_t *local)
{
    return (local->selection_year == local->display_year && local->selection_month == local->display_month) ? 1 : 0;
}

static void calendar_view_commit_selection_state(egui_view_calendar_view_t *local)
{
    local->committed_year = local->selection_year;
    local->committed_month = local->selection_month;
    local->committed_start_day = local->start_day;
    local->committed_end_day = local->end_day;
}

static void calendar_view_emit_changed(egui_view_t *self, egui_view_calendar_view_t *local)
{
    if (local->on_changed)
    {
        local->on_changed(self, local->selection_year, local->selection_month, local->start_day, local->end_day, local->focus_day, local->editing_range);
    }
}

static void calendar_view_emit_display_month_changed(egui_view_t *self, egui_view_calendar_view_t *local)
{
    if (local->on_display_month_changed)
    {
        local->on_display_month_changed(self, local->display_year, local->display_month);
    }
}

static void calendar_view_clamp_focus_day(egui_view_calendar_view_t *local)
{
    uint8_t max_day = calendar_view_days_in_month(local->display_year, local->display_month);

    if (local->focus_day < 1)
    {
        local->focus_day = 1;
    }
    else if (local->focus_day > max_day)
    {
        local->focus_day = max_day;
    }
}

static uint8_t calendar_view_assign_display_month(egui_view_calendar_view_t *local, uint16_t year, uint8_t month)
{
    calendar_view_normalize_display_month(&year, &month);
    if (local->display_year == year && local->display_month == month)
    {
        return 0;
    }

    local->display_year = year;
    local->display_month = month;
    calendar_view_clamp_focus_day(local);
    return 1;
}

static void calendar_view_get_metrics(egui_view_calendar_view_t *local, egui_view_t *self, egui_view_calendar_view_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_CALENDAR_VIEW_COMPACT_PAD_X : EGUI_VIEW_CALENDAR_VIEW_STANDARD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_CALENDAR_VIEW_COMPACT_PAD_Y : EGUI_VIEW_CALENDAR_VIEW_STANDARD_PAD_Y;
    egui_dim_t label_h = EGUI_VIEW_CALENDAR_VIEW_STANDARD_LABEL_HEIGHT;
    egui_dim_t label_gap = EGUI_VIEW_CALENDAR_VIEW_STANDARD_LABEL_GAP;
    egui_dim_t header_h = local->compact_mode ? EGUI_VIEW_CALENDAR_VIEW_COMPACT_HEADER_HEIGHT : EGUI_VIEW_CALENDAR_VIEW_STANDARD_HEADER_HEIGHT;
    egui_dim_t week_h = EGUI_VIEW_CALENDAR_VIEW_STANDARD_WEEK_HEIGHT;
    egui_dim_t helper_h = EGUI_VIEW_CALENDAR_VIEW_STANDARD_HELPER_HEIGHT;
    egui_dim_t helper_gap = EGUI_VIEW_CALENDAR_VIEW_STANDARD_HELPER_GAP;
    egui_dim_t summary_h = EGUI_VIEW_CALENDAR_VIEW_COMPACT_SUMMARY_HEIGHT;
    egui_dim_t cursor_y;
    egui_dim_t helper_total = 0;

    egui_view_get_work_region(self, &region);
    metrics->content_region.location.x = region.location.x + pad_x;
    metrics->content_region.location.y = region.location.y + pad_y;
    metrics->content_region.size.width = region.size.width - pad_x * 2;
    metrics->content_region.size.height = region.size.height - pad_y * 2;

    if (metrics->content_region.size.width < 0)
    {
        metrics->content_region.size.width = 0;
    }
    if (metrics->content_region.size.height < 0)
    {
        metrics->content_region.size.height = 0;
    }

    metrics->show_label = (!local->compact_mode && local->label != NULL && local->label[0] != '\0') ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && local->helper != NULL && local->helper[0] != '\0') ? 1 : 0;
    metrics->show_grid = local->compact_mode ? 0 : 1;

    cursor_y = metrics->content_region.location.y;
    if (metrics->show_label)
    {
        metrics->label_region.location.x = metrics->content_region.location.x;
        metrics->label_region.location.y = cursor_y;
        metrics->label_region.size.width = metrics->content_region.size.width;
        metrics->label_region.size.height = label_h;
        cursor_y += label_h + label_gap;
    }

    if (metrics->show_helper)
    {
        helper_total = helper_gap + helper_h;
    }

    metrics->header_region.location.x = metrics->content_region.location.x;
    metrics->header_region.location.y = cursor_y;
    metrics->header_region.size.width = metrics->content_region.size.width;
    metrics->header_region.size.height = header_h;
    cursor_y += header_h;

    metrics->prev_region.location.x = metrics->header_region.location.x;
    metrics->prev_region.location.y = metrics->header_region.location.y + (header_h > 16 ? (header_h - 16) / 2 : 0);
    metrics->prev_region.size.width = 16;
    metrics->prev_region.size.height = header_h > 16 ? 16 : header_h;

    metrics->next_region.location.x = metrics->header_region.location.x + metrics->header_region.size.width - 16;
    metrics->next_region.location.y = metrics->prev_region.location.y;
    metrics->next_region.size.width = 16;
    metrics->next_region.size.height = metrics->prev_region.size.height;

    metrics->title_region.location.x = metrics->header_region.location.x + 18;
    metrics->title_region.location.y = metrics->header_region.location.y;
    metrics->title_region.size.width = metrics->header_region.size.width - 36;
    metrics->title_region.size.height = header_h;

    if (metrics->show_grid)
    {
        metrics->week_region.location.x = metrics->content_region.location.x;
        metrics->week_region.location.y = cursor_y;
        metrics->week_region.size.width = metrics->content_region.size.width;
        metrics->week_region.size.height = week_h;
        cursor_y += week_h;

        metrics->grid_region.location.x = metrics->content_region.location.x;
        metrics->grid_region.location.y = cursor_y;
        metrics->grid_region.size.width = metrics->content_region.size.width;
        metrics->grid_region.size.height = metrics->content_region.location.y + metrics->content_region.size.height - helper_total - cursor_y;
        if (metrics->grid_region.size.height < 0)
        {
            metrics->grid_region.size.height = 0;
        }

        if (metrics->show_helper)
        {
            metrics->helper_region.location.x = metrics->content_region.location.x;
            metrics->helper_region.location.y = metrics->grid_region.location.y + metrics->grid_region.size.height + helper_gap;
            metrics->helper_region.size.width = metrics->content_region.size.width;
            metrics->helper_region.size.height = helper_h;
        }
    }
    else
    {
        metrics->summary_region.location.x = metrics->content_region.location.x;
        metrics->summary_region.location.y = cursor_y + 3;
        metrics->summary_region.size.width = metrics->content_region.size.width;
        metrics->summary_region.size.height = summary_h;
    }
}

static void calendar_view_draw_summary(egui_view_t *self, egui_view_calendar_view_t *local, const egui_view_calendar_view_metrics_t *metrics,
                                       egui_color_t accent_color, egui_color_t text_color, egui_color_t muted_text_color)
{
    char title[16];
    char summary[16];
    egui_region_t pill_region = metrics->summary_region;

    calendar_view_format_month_title(local->display_year, local->display_month, title);
    calendar_view_format_range_summary(local->selection_year, local->selection_month, local->start_day, local->end_day, summary);

    calendar_view_draw_text(local->meta_font, self, title, &metrics->title_region, EGUI_ALIGN_CENTER, muted_text_color);
    pill_region.location.x += 8;
    pill_region.size.width -= 16;
    egui_canvas_draw_round_rectangle_fill(pill_region.location.x, pill_region.location.y, pill_region.size.width, pill_region.size.height, 7,
                                          egui_rgb_mix(local->surface_color, accent_color, 14), egui_color_alpha_mix(self->alpha, 36));
    egui_canvas_draw_round_rectangle(pill_region.location.x, pill_region.location.y, pill_region.size.width, pill_region.size.height, 7, 1,
                                     egui_rgb_mix(local->border_color, accent_color, 28), egui_color_alpha_mix(self->alpha, 70));
    calendar_view_draw_text(local->font, self, summary, &pill_region, EGUI_ALIGN_CENTER, text_color);
}

static void calendar_view_draw_grid(egui_view_t *self, egui_view_calendar_view_t *local, const egui_view_calendar_view_metrics_t *metrics,
                                    egui_color_t accent_color, egui_color_t text_color, egui_color_t muted_text_color, egui_color_t border_color)
{
    const char **weekdays = local->first_day_of_week ? calendar_view_weekday_names_mon : calendar_view_weekday_names_sun;
    uint8_t first_dow = calendar_view_day_of_week(local->display_year, local->display_month, 1);
    uint8_t start_cell = (uint8_t)((first_dow - local->first_day_of_week + 7) % 7);
    uint8_t total_days = calendar_view_days_in_month(local->display_year, local->display_month);
    uint8_t selection_visible = calendar_view_selection_visible(local);
    egui_dim_t cell_w = metrics->grid_region.size.width / 7;
    egui_dim_t cell_h = metrics->grid_region.size.height / 6;
    uint8_t row;
    uint8_t col;
    char buffer[8];

    if (cell_w < 1 || cell_h < 1)
    {
        return;
    }

    for (col = 0; col < 7; ++col)
    {
        egui_region_t week_region;

        week_region.location.x = metrics->week_region.location.x + col * cell_w;
        week_region.location.y = metrics->week_region.location.y;
        week_region.size.width = cell_w;
        week_region.size.height = metrics->week_region.size.height;
        calendar_view_draw_text(local->meta_font, self, weekdays[col], &week_region, EGUI_ALIGN_CENTER, muted_text_color);
    }

    for (row = 0; row < 6; ++row)
    {
        for (col = 0; col < 7; ++col)
        {
            uint8_t cell_index = (uint8_t)(row * 7 + col);
            uint8_t day = 0;
            egui_region_t cell_region;
            egui_region_t cell_inner_region;
            egui_color_t day_text_color = text_color;
            uint8_t in_range = 0;
            uint8_t is_start = 0;
            uint8_t is_end = 0;

            if (cell_index >= start_cell && cell_index < start_cell + total_days)
            {
                day = (uint8_t)(cell_index - start_cell + 1);
            }
            if (day == 0)
            {
                continue;
            }

            cell_region.location.x = metrics->grid_region.location.x + col * cell_w;
            cell_region.location.y = metrics->grid_region.location.y + row * cell_h;
            cell_region.size.width = cell_w;
            cell_region.size.height = cell_h;

            cell_inner_region.location.x = cell_region.location.x + (cell_w > 20 ? 2 : 1);
            cell_inner_region.location.y = cell_region.location.y + 1;
            cell_inner_region.size.width = cell_region.size.width - ((cell_w > 20 ? 2 : 1) * 2);
            cell_inner_region.size.height = cell_region.size.height - 2;

            if (selection_visible && day >= local->start_day && day <= local->end_day)
            {
                in_range = 1;
                is_start = day == local->start_day ? 1 : 0;
                is_end = day == local->end_day ? 1 : 0;
            }

            if (in_range)
            {
                egui_dim_t bridge_x = cell_inner_region.location.x;
                egui_dim_t bridge_w = cell_inner_region.size.width;
                egui_alpha_t bridge_alpha = egui_color_alpha_mix(self->alpha, local->start_day == local->end_day ? 84 : 26);

                if (local->start_day != local->end_day)
                {
                    if (!is_start)
                    {
                        bridge_x -= 1;
                        bridge_w += 1;
                    }
                    if (!is_end)
                    {
                        bridge_w += 1;
                    }
                }

                egui_canvas_draw_rectangle_fill(bridge_x, cell_inner_region.location.y + 3, bridge_w, cell_inner_region.size.height - 6,
                                                egui_rgb_mix(local->surface_color, accent_color, 14), bridge_alpha);
                if (is_start || is_end || local->start_day == local->end_day)
                {
                    egui_canvas_draw_round_rectangle_fill(cell_inner_region.location.x, cell_inner_region.location.y, cell_inner_region.size.width,
                                                          cell_inner_region.size.height, 4, accent_color,
                                                          egui_color_alpha_mix(self->alpha, local->start_day == local->end_day ? 84 : 72));
                    egui_canvas_draw_round_rectangle(cell_inner_region.location.x, cell_inner_region.location.y, cell_inner_region.size.width,
                                                     cell_inner_region.size.height, 4, 1, egui_rgb_mix(accent_color, EGUI_COLOR_WHITE, 16),
                                                     egui_color_alpha_mix(self->alpha, 76));
                    day_text_color = EGUI_COLOR_WHITE;
                }
                else
                {
                    day_text_color = accent_color;
                }
            }
            else if (local->today_year == local->display_year && local->today_month == local->display_month && local->today_day == day)
            {
                egui_dim_t dot_radius = cell_inner_region.size.width > 16 ? 2 : 1;
                egui_dim_t dot_center_x = cell_inner_region.location.x + cell_inner_region.size.width / 2;
                egui_dim_t dot_center_y = cell_inner_region.location.y + cell_inner_region.size.height - 4;

                egui_canvas_draw_round_rectangle(cell_inner_region.location.x, cell_inner_region.location.y, cell_inner_region.size.width,
                                                 cell_inner_region.size.height, 4, 1, local->today_color, egui_color_alpha_mix(self->alpha, 78));
                egui_canvas_draw_circle_fill(dot_center_x, dot_center_y, dot_radius, local->today_color, egui_color_alpha_mix(self->alpha, 86));
                day_text_color = local->today_color;
            }
            else if (((col + local->first_day_of_week) % 7) == 0 || ((col + local->first_day_of_week) % 7) == 6)
            {
                day_text_color = muted_text_color;
            }

            if (local->pressed_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID && day == local->pressed_day)
            {
                egui_canvas_draw_round_rectangle_fill(cell_inner_region.location.x, cell_inner_region.location.y, cell_inner_region.size.width,
                                                      cell_inner_region.size.height, 4, egui_rgb_mix(local->surface_color, accent_color, 18),
                                                      egui_color_alpha_mix(self->alpha, 34));
                egui_canvas_draw_round_rectangle(cell_inner_region.location.x, cell_inner_region.location.y, cell_inner_region.size.width,
                                                 cell_inner_region.size.height, 4, 1, egui_rgb_mix(border_color, accent_color, 34),
                                                 egui_color_alpha_mix(self->alpha, 72));
            }

            if (local->editing_range && selection_visible && day == local->anchor_day)
            {
                egui_dim_t line_y = cell_inner_region.location.y + cell_inner_region.size.height - 2;

                egui_canvas_draw_line(cell_inner_region.location.x + 3, line_y, cell_inner_region.location.x + cell_inner_region.size.width - 3, line_y, 1,
                                      EGUI_COLOR_WHITE, egui_color_alpha_mix(self->alpha, 88));
            }

            if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID && day == local->focus_day)
            {
                egui_canvas_draw_round_rectangle(
                        cell_inner_region.location.x, cell_inner_region.location.y, cell_inner_region.size.width, cell_inner_region.size.height, 4, 1,
                        in_range ? EGUI_COLOR_WHITE : egui_rgb_mix(border_color, accent_color, 36), egui_color_alpha_mix(self->alpha, in_range ? 90 : 82));
            }

            buffer[0] = (char)('0' + (day / 10) % 10);
            buffer[1] = (char)('0' + day % 10);
            buffer[2] = '\0';
            if (day < 10)
            {
                buffer[0] = (char)('0' + day);
                buffer[1] = '\0';
            }

            calendar_view_draw_text((in_range && (is_start || is_end || local->start_day == local->end_day)) ? local->font : local->meta_font, self, buffer,
                                    &cell_region, EGUI_ALIGN_CENTER, day_text_color);
        }
    }
}

static void calendar_view_draw_header(egui_view_t *self, egui_view_calendar_view_t *local, const egui_view_calendar_view_metrics_t *metrics,
                                      egui_color_t accent_color, egui_color_t text_color, egui_color_t muted_text_color)
{
    char title[16];
    egui_color_t title_color = text_color;

    if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_PREV || local->pressed_part == EGUI_VIEW_CALENDAR_VIEW_PART_PREV)
    {
        egui_canvas_draw_round_rectangle_fill(metrics->prev_region.location.x, metrics->prev_region.location.y, metrics->prev_region.size.width,
                                              metrics->prev_region.size.height, 5, egui_rgb_mix(local->surface_color, accent_color, 18),
                                              egui_color_alpha_mix(self->alpha, 52));
    }
    if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_NEXT || local->pressed_part == EGUI_VIEW_CALENDAR_VIEW_PART_NEXT)
    {
        egui_canvas_draw_round_rectangle_fill(metrics->next_region.location.x, metrics->next_region.location.y, metrics->next_region.size.width,
                                              metrics->next_region.size.height, 5, egui_rgb_mix(local->surface_color, accent_color, 18),
                                              egui_color_alpha_mix(self->alpha, 52));
    }

    if (!calendar_view_selection_visible(local))
    {
        title_color = accent_color;
    }

    calendar_view_draw_chevron(self, &metrics->prev_region, muted_text_color, 0);
    calendar_view_draw_chevron(self, &metrics->next_region, muted_text_color, 1);
    calendar_view_format_month_title(local->display_year, local->display_month, title);
    calendar_view_draw_text(local->font, self, title, &metrics->title_region, EGUI_ALIGN_CENTER, title_color);
}

static void egui_view_calendar_view_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    egui_region_t region;
    egui_view_calendar_view_metrics_t metrics;
    egui_color_t accent_color = local->accent_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_text_color = local->muted_text_color;
    egui_color_t outer_fill = egui_rgb_mix(local->surface_color, local->accent_color, local->compact_mode ? 4 : 5);
    egui_color_t outer_border = egui_rgb_mix(local->border_color, local->accent_color, local->editing_range ? 22 : 10);

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    if (local->read_only_mode)
    {
        accent_color = egui_rgb_mix(accent_color, muted_text_color, 72);
        outer_fill = egui_rgb_mix(outer_fill, local->surface_color, 18);
        outer_border = egui_rgb_mix(outer_border, muted_text_color, 20);
    }

    if (!egui_view_get_enable(self))
    {
        accent_color = calendar_view_mix_disabled(accent_color);
        border_color = calendar_view_mix_disabled(border_color);
        text_color = calendar_view_mix_disabled(text_color);
        muted_text_color = calendar_view_mix_disabled(muted_text_color);
        outer_fill = calendar_view_mix_disabled(outer_fill);
        outer_border = calendar_view_mix_disabled(outer_border);
    }

    calendar_view_get_metrics(local, self, &metrics);

    egui_canvas_draw_round_rectangle_fill(
            region.location.x, region.location.y, region.size.width, region.size.height,
            local->compact_mode ? EGUI_VIEW_CALENDAR_VIEW_COMPACT_RADIUS : EGUI_VIEW_CALENDAR_VIEW_STANDARD_RADIUS, outer_fill,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_CALENDAR_VIEW_COMPACT_FILL_ALPHA : EGUI_VIEW_CALENDAR_VIEW_STANDARD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height,
                                     local->compact_mode ? EGUI_VIEW_CALENDAR_VIEW_COMPACT_RADIUS : EGUI_VIEW_CALENDAR_VIEW_STANDARD_RADIUS, 1, outer_border,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_CALENDAR_VIEW_COMPACT_BORDER_ALPHA
                                                                                           : EGUI_VIEW_CALENDAR_VIEW_STANDARD_BORDER_ALPHA));
    egui_canvas_draw_round_rectangle_fill(region.location.x + 2, region.location.y + 2, region.size.width - 4, local->compact_mode ? 3 : 4,
                                          local->compact_mode ? EGUI_VIEW_CALENDAR_VIEW_COMPACT_RADIUS : EGUI_VIEW_CALENDAR_VIEW_STANDARD_RADIUS, accent_color,
                                          egui_color_alpha_mix(self->alpha, local->read_only_mode ? 12 : 24));

    if (metrics.show_label)
    {
        calendar_view_draw_text(local->meta_font, self, local->label, &metrics.label_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    }

    calendar_view_draw_header(self, local, &metrics, accent_color, text_color, muted_text_color);

    if (metrics.show_grid)
    {
        calendar_view_draw_grid(self, local, &metrics, accent_color, text_color, muted_text_color, border_color);
    }
    else
    {
        calendar_view_draw_summary(self, local, &metrics, accent_color, text_color, muted_text_color);
    }

    if (metrics.show_helper)
    {
        calendar_view_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    }
}

static void calendar_view_set_current_part_inner(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    if (part != EGUI_VIEW_CALENDAR_VIEW_PART_GRID && part != EGUI_VIEW_CALENDAR_VIEW_PART_PREV && part != EGUI_VIEW_CALENDAR_VIEW_PART_NEXT)
    {
        part = EGUI_VIEW_CALENDAR_VIEW_PART_GRID;
    }
    if (local->current_part == part)
    {
        return;
    }
    local->current_part = part;
    egui_view_invalidate(self);
}

static void calendar_view_restore_committed_selection(egui_view_t *self, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    uint8_t changed = 0;

    if (local->selection_year != local->committed_year || local->selection_month != local->committed_month || local->start_day != local->committed_start_day ||
        local->end_day != local->committed_end_day || local->editing_range)
    {
        changed = 1;
    }

    local->selection_year = local->committed_year;
    local->selection_month = local->committed_month;
    local->start_day = local->committed_start_day;
    local->end_day = local->committed_end_day;
    local->editing_range = 0;
    local->anchor_day = 0;
    if (calendar_view_selection_visible(local))
    {
        local->focus_day = local->end_day;
    }
    calendar_view_clamp_focus_day(local);
    egui_view_invalidate(self);
    if (notify && changed)
    {
        calendar_view_emit_changed(self, local);
    }
}

static void calendar_view_apply_single_day(egui_view_t *self, uint8_t day, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    uint8_t changed = 0;

    if (day < 1)
    {
        day = 1;
    }
    if (day > calendar_view_days_in_month(local->display_year, local->display_month))
    {
        day = calendar_view_days_in_month(local->display_year, local->display_month);
    }

    changed = local->selection_year != local->display_year || local->selection_month != local->display_month || local->start_day != day ||
              local->end_day != day || local->focus_day != day || local->editing_range;

    local->selection_year = local->display_year;
    local->selection_month = local->display_month;
    local->start_day = day;
    local->end_day = day;
    local->focus_day = day;
    local->editing_range = 0;
    local->anchor_day = 0;
    calendar_view_commit_selection_state(local);
    egui_view_invalidate(self);
    if (notify && changed)
    {
        calendar_view_emit_changed(self, local);
    }
}

static void calendar_view_start_range_edit(egui_view_t *self, uint8_t day)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    uint8_t changed = 0;

    if (day < 1)
    {
        day = 1;
    }
    if (day > calendar_view_days_in_month(local->display_year, local->display_month))
    {
        day = calendar_view_days_in_month(local->display_year, local->display_month);
    }

    changed = local->selection_year != local->display_year || local->selection_month != local->display_month || local->start_day != day ||
              local->end_day != day || local->focus_day != day || !local->editing_range || local->anchor_day != day;

    calendar_view_commit_selection_state(local);
    local->selection_year = local->display_year;
    local->selection_month = local->display_month;
    local->start_day = day;
    local->end_day = day;
    local->focus_day = day;
    local->editing_range = 1;
    local->anchor_day = day;
    egui_view_invalidate(self);
    if (changed)
    {
        calendar_view_emit_changed(self, local);
    }
}

static void calendar_view_preview_range(egui_view_t *self, uint8_t focus_day)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    uint8_t start_day = local->anchor_day;
    uint8_t end_day = focus_day;
    uint8_t changed;

    if (focus_day < 1)
    {
        focus_day = 1;
    }
    if (focus_day > calendar_view_days_in_month(local->display_year, local->display_month))
    {
        focus_day = calendar_view_days_in_month(local->display_year, local->display_month);
    }

    if (start_day < 1)
    {
        start_day = focus_day;
    }
    end_day = focus_day;
    calendar_view_normalize_day_pair(local->display_year, local->display_month, &start_day, &end_day);
    changed = local->selection_year != local->display_year || local->selection_month != local->display_month || local->start_day != start_day ||
              local->end_day != end_day || local->focus_day != focus_day || !local->editing_range;

    local->selection_year = local->display_year;
    local->selection_month = local->display_month;
    local->start_day = start_day;
    local->end_day = end_day;
    local->focus_day = focus_day;
    local->editing_range = 1;
    egui_view_invalidate(self);
    if (changed)
    {
        calendar_view_emit_changed(self, local);
    }
}

static void calendar_view_finish_range_edit(egui_view_t *self, uint8_t focus_day)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    uint8_t start_day = local->anchor_day;
    uint8_t end_day = focus_day;
    uint8_t changed;

    if (focus_day < 1)
    {
        focus_day = 1;
    }
    if (focus_day > calendar_view_days_in_month(local->display_year, local->display_month))
    {
        focus_day = calendar_view_days_in_month(local->display_year, local->display_month);
    }

    if (start_day < 1)
    {
        start_day = focus_day;
    }
    end_day = focus_day;
    calendar_view_normalize_day_pair(local->display_year, local->display_month, &start_day, &end_day);

    changed = local->selection_year != local->display_year || local->selection_month != local->display_month || local->start_day != start_day ||
              local->end_day != end_day || local->focus_day != focus_day || !local->editing_range;

    local->selection_year = local->display_year;
    local->selection_month = local->display_month;
    local->start_day = start_day;
    local->end_day = end_day;
    local->focus_day = focus_day;
    local->editing_range = 0;
    local->anchor_day = 0;
    calendar_view_commit_selection_state(local);
    egui_view_invalidate(self);
    if (changed)
    {
        calendar_view_emit_changed(self, local);
    }
}

static void calendar_view_shift_display_month(egui_view_t *self, int8_t delta)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    int year = local->display_year;
    int month = local->display_month + delta;

    if (local->editing_range)
    {
        calendar_view_restore_committed_selection(self, 1);
    }

    while (month < 1)
    {
        month += 12;
        year--;
    }
    while (month > 12)
    {
        month -= 12;
        year++;
    }
    if (year < 1)
    {
        year = 1;
    }

    if (calendar_view_assign_display_month(local, (uint16_t)year, (uint8_t)month))
    {
        egui_view_invalidate(self);
        calendar_view_emit_display_month_changed(self, local);
    }
}

static void calendar_view_shift_focus_day(egui_view_t *self, int8_t delta)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    int focus_day = local->focus_day + delta;
    uint8_t max_day = calendar_view_days_in_month(local->display_year, local->display_month);

    if (focus_day < 1)
    {
        focus_day = 1;
    }
    else if (focus_day > max_day)
    {
        focus_day = max_day;
    }

    if (local->editing_range)
    {
        calendar_view_preview_range(self, (uint8_t)focus_day);
    }
    else
    {
        calendar_view_apply_single_day(self, (uint8_t)focus_day, 1);
    }
}

static uint8_t calendar_view_get_hit_day(egui_view_calendar_view_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_calendar_view_metrics_t metrics;
    egui_dim_t rel_x;
    egui_dim_t rel_y;
    egui_dim_t cell_w;
    egui_dim_t cell_h;
    uint8_t col;
    uint8_t row;
    uint8_t first_dow;
    uint8_t start_cell;
    uint8_t total_days;
    uint8_t pos;

    calendar_view_get_metrics(local, self, &metrics);
    if (!metrics.show_grid || metrics.grid_region.size.width <= 0 || metrics.grid_region.size.height <= 0)
    {
        return 0;
    }
    if (!calendar_view_region_contains_point(&metrics.grid_region, x, y))
    {
        return 0;
    }

    rel_x = x - metrics.grid_region.location.x;
    rel_y = y - metrics.grid_region.location.y;
    cell_w = metrics.grid_region.size.width / 7;
    cell_h = metrics.grid_region.size.height / 6;
    if (cell_w <= 0 || cell_h <= 0)
    {
        return 0;
    }

    col = (uint8_t)(rel_x / cell_w);
    row = (uint8_t)(rel_y / cell_h);
    if (col >= 7 || row >= 6)
    {
        return 0;
    }

    first_dow = calendar_view_day_of_week(local->display_year, local->display_month, 1);
    start_cell = (uint8_t)((first_dow - local->first_day_of_week + 7) % 7);
    total_days = calendar_view_days_in_month(local->display_year, local->display_month);
    pos = (uint8_t)(row * 7 + col);
    if (pos < start_cell || pos >= start_cell + total_days)
    {
        return 0;
    }
    return (uint8_t)(pos - start_cell + 1);
}

static uint8_t calendar_view_hit_part(egui_view_calendar_view_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_calendar_view_metrics_t metrics;

    calendar_view_get_metrics(local, self, &metrics);
    if (calendar_view_region_contains_point(&metrics.prev_region, x, y))
    {
        return EGUI_VIEW_CALENDAR_VIEW_PART_PREV;
    }
    if (calendar_view_region_contains_point(&metrics.next_region, x, y))
    {
        return EGUI_VIEW_CALENDAR_VIEW_PART_NEXT;
    }
    if (calendar_view_get_hit_day(local, self, x, y) > 0)
    {
        return EGUI_VIEW_CALENDAR_VIEW_PART_GRID;
    }
    return EGUI_VIEW_CALENDAR_VIEW_PART_NONE;
}

void egui_view_calendar_view_set_range(egui_view_t *self, uint16_t year, uint8_t month, uint8_t start_day, uint8_t end_day)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    calendar_view_normalize_display_month(&year, &month);
    calendar_view_normalize_day_pair(year, month, &start_day, &end_day);

    local->selection_year = year;
    local->selection_month = month;
    local->start_day = start_day;
    local->end_day = end_day;
    local->editing_range = 0;
    local->anchor_day = 0;
    local->focus_day = end_day;
    calendar_view_commit_selection_state(local);
    if (local->display_year == year && local->display_month == month)
    {
        local->focus_day = end_day;
    }
    calendar_view_clamp_focus_day(local);
    egui_view_invalidate(self);
}

uint16_t egui_view_calendar_view_get_selection_year(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    return local->selection_year;
}

uint8_t egui_view_calendar_view_get_selection_month(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    return local->selection_month;
}

uint8_t egui_view_calendar_view_get_start_day(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    return local->start_day;
}

uint8_t egui_view_calendar_view_get_end_day(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    return local->end_day;
}

uint8_t egui_view_calendar_view_get_focus_day(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    return local->focus_day;
}

uint8_t egui_view_calendar_view_get_editing_range(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    return local->editing_range;
}

void egui_view_calendar_view_set_today(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    calendar_view_normalize_display_month(&year, &month);
    if (day < 1)
    {
        day = 1;
    }
    else if (day > calendar_view_days_in_month(year, month))
    {
        day = calendar_view_days_in_month(year, month);
    }

    local->today_year = year;
    local->today_month = month;
    local->today_day = day;
    egui_view_invalidate(self);
}

void egui_view_calendar_view_set_first_day_of_week(egui_view_t *self, uint8_t first_day_of_week)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    first_day_of_week = first_day_of_week ? 1 : 0;
    if (local->first_day_of_week == first_day_of_week)
    {
        return;
    }
    local->first_day_of_week = first_day_of_week;
    egui_view_invalidate(self);
}

uint8_t egui_view_calendar_view_get_first_day_of_week(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    return local->first_day_of_week;
}

void egui_view_calendar_view_set_display_month(egui_view_t *self, uint16_t year, uint8_t month)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    if (!calendar_view_assign_display_month(local, year, month))
    {
        return;
    }
    egui_view_invalidate(self);
    calendar_view_emit_display_month_changed(self, local);
}

uint16_t egui_view_calendar_view_get_display_year(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    return local->display_year;
}

uint8_t egui_view_calendar_view_get_display_month(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    return local->display_month;
}

void egui_view_calendar_view_set_label(egui_view_t *self, const char *label)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    local->label = label;
    egui_view_invalidate(self);
}

void egui_view_calendar_view_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_calendar_view_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    local->font = font != NULL ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_calendar_view_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    local->meta_font = font != NULL ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_calendar_view_set_on_changed_listener(egui_view_t *self, egui_view_on_calendar_view_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    local->on_changed = listener;
}

void egui_view_calendar_view_set_on_display_month_changed_listener(egui_view_t *self, egui_view_on_calendar_view_display_month_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    local->on_display_month_changed = listener;
}

void egui_view_calendar_view_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    compact_mode = compact_mode ? 1 : 0;
    if (local->compact_mode == compact_mode)
    {
        return;
    }
    local->compact_mode = compact_mode;
    local->pressed_part = EGUI_VIEW_CALENDAR_VIEW_PART_NONE;
    local->pressed_day = 0;
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

void egui_view_calendar_view_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    read_only_mode = read_only_mode ? 1 : 0;
    if (local->read_only_mode == read_only_mode)
    {
        return;
    }
    local->read_only_mode = read_only_mode;
    local->pressed_part = EGUI_VIEW_CALENDAR_VIEW_PART_NONE;
    local->pressed_day = 0;
    egui_view_set_pressed(self, false);
    if (read_only_mode && local->editing_range)
    {
        calendar_view_restore_committed_selection(self, 0);
    }
    egui_view_invalidate(self);
}

void egui_view_calendar_view_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                         egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t today_color)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->today_color = today_color;
    egui_view_invalidate(self);
}

void egui_view_calendar_view_set_current_part(egui_view_t *self, uint8_t part)
{
    calendar_view_set_current_part_inner(self, part);
}

uint8_t egui_view_calendar_view_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    return local->current_part;
}

uint8_t egui_view_calendar_view_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    if (!egui_view_get_enable(self) || local->read_only_mode || local->compact_mode)
    {
        return 0;
    }

    switch (key_code)
    {
    case EGUI_KEY_CODE_TAB:
        if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID)
        {
            calendar_view_set_current_part_inner(self, EGUI_VIEW_CALENDAR_VIEW_PART_PREV);
        }
        else if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_PREV)
        {
            calendar_view_set_current_part_inner(self, EGUI_VIEW_CALENDAR_VIEW_PART_NEXT);
        }
        else
        {
            calendar_view_set_current_part_inner(self, EGUI_VIEW_CALENDAR_VIEW_PART_GRID);
        }
        return 1;
    case EGUI_KEY_CODE_LEFT:
        if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID)
        {
            calendar_view_shift_focus_day(self, -1);
        }
        else if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_NEXT)
        {
            calendar_view_set_current_part_inner(self, EGUI_VIEW_CALENDAR_VIEW_PART_PREV);
        }
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID)
        {
            calendar_view_shift_focus_day(self, 1);
        }
        else if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_PREV)
        {
            calendar_view_set_current_part_inner(self, EGUI_VIEW_CALENDAR_VIEW_PART_NEXT);
        }
        return 1;
    case EGUI_KEY_CODE_UP:
        if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID)
        {
            calendar_view_shift_focus_day(self, -7);
        }
        else
        {
            calendar_view_set_current_part_inner(self, EGUI_VIEW_CALENDAR_VIEW_PART_GRID);
        }
        return 1;
    case EGUI_KEY_CODE_DOWN:
        if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID)
        {
            calendar_view_shift_focus_day(self, 7);
        }
        else
        {
            calendar_view_set_current_part_inner(self, EGUI_VIEW_CALENDAR_VIEW_PART_GRID);
        }
        return 1;
    case EGUI_KEY_CODE_HOME:
        if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID)
        {
            if (local->editing_range)
            {
                calendar_view_preview_range(self, 1);
            }
            else
            {
                calendar_view_apply_single_day(self, 1, 1);
            }
            return 1;
        }
        return 0;
    case EGUI_KEY_CODE_END:
    {
        uint8_t last_day = calendar_view_days_in_month(local->display_year, local->display_month);

        if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID)
        {
            if (local->editing_range)
            {
                calendar_view_preview_range(self, last_day);
            }
            else
            {
                calendar_view_apply_single_day(self, last_day, 1);
            }
            return 1;
        }
        return 0;
    }
    case EGUI_KEY_CODE_MINUS:
        calendar_view_shift_display_month(self, -1);
        return 1;
    case EGUI_KEY_CODE_PLUS:
        calendar_view_shift_display_month(self, 1);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_PREV)
        {
            calendar_view_shift_display_month(self, -1);
            return 1;
        }
        if (local->current_part == EGUI_VIEW_CALENDAR_VIEW_PART_NEXT)
        {
            calendar_view_shift_display_month(self, 1);
            return 1;
        }
        if (local->editing_range)
        {
            calendar_view_finish_range_edit(self, local->focus_day);
        }
        else
        {
            calendar_view_start_range_edit(self, local->focus_day);
        }
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        if (local->editing_range)
        {
            calendar_view_restore_committed_selection(self, 1);
            return 1;
        }
        if (local->current_part != EGUI_VIEW_CALENDAR_VIEW_PART_GRID)
        {
            calendar_view_set_current_part_inner(self, EGUI_VIEW_CALENDAR_VIEW_PART_GRID);
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

uint8_t egui_view_calendar_view_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    egui_view_calendar_view_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }

    calendar_view_get_metrics(local, self, &metrics);
    switch (part)
    {
    case EGUI_VIEW_CALENDAR_VIEW_PART_GRID:
        if (!metrics.show_grid)
        {
            return 0;
        }
        *region = metrics.grid_region;
        return 1;
    case EGUI_VIEW_CALENDAR_VIEW_PART_PREV:
        *region = metrics.prev_region;
        return 1;
    case EGUI_VIEW_CALENDAR_VIEW_PART_NEXT:
        *region = metrics.next_region;
        return 1;
    default:
        return 0;
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void egui_view_calendar_view_on_focus_change(egui_view_t *self, int is_focused)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);

    if (is_focused)
    {
        return;
    }

    local->pressed_part = EGUI_VIEW_CALENDAR_VIEW_PART_NONE;
    local->pressed_day = 0;
    egui_view_set_pressed(self, false);
    if (local->editing_range)
    {
        calendar_view_restore_committed_selection(self, 1);
    }
    else if (local->current_part != EGUI_VIEW_CALENDAR_VIEW_PART_GRID)
    {
        local->current_part = EGUI_VIEW_CALENDAR_VIEW_PART_GRID;
        egui_view_invalidate(self);
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_calendar_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_calendar_view_t);
    uint8_t hit_part;
    uint8_t hit_day;

    if (!egui_view_get_enable(self) || local->read_only_mode || local->compact_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = calendar_view_hit_part(local, self, event->location.x, event->location.y);
        if (hit_part == EGUI_VIEW_CALENDAR_VIEW_PART_NONE)
        {
            return 0;
        }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (self->is_focusable)
        {
            egui_view_request_focus(self);
        }
#endif
        local->pressed_part = hit_part;
        local->pressed_day = hit_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID ? calendar_view_get_hit_day(local, self, event->location.x, event->location.y) : 0;
        egui_view_set_pressed(self, true);
        calendar_view_set_current_part_inner(self, hit_part);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->pressed_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID)
        {
            hit_day = calendar_view_get_hit_day(local, self, event->location.x, event->location.y);
            if (local->pressed_day != hit_day)
            {
                local->pressed_day = hit_day;
                egui_view_invalidate(self);
            }
            return 1;
        }
        return local->pressed_part != EGUI_VIEW_CALENDAR_VIEW_PART_NONE ? 1 : 0;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_CALENDAR_VIEW_PART_NONE;
        local->pressed_day = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = calendar_view_hit_part(local, self, event->location.x, event->location.y);
        hit_day = hit_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID ? calendar_view_get_hit_day(local, self, event->location.x, event->location.y) : 0;
        if (local->pressed_part == hit_part)
        {
            if (hit_part == EGUI_VIEW_CALENDAR_VIEW_PART_PREV)
            {
                calendar_view_shift_display_month(self, -1);
            }
            else if (hit_part == EGUI_VIEW_CALENDAR_VIEW_PART_NEXT)
            {
                calendar_view_shift_display_month(self, 1);
            }
            else if (hit_part == EGUI_VIEW_CALENDAR_VIEW_PART_GRID && hit_day > 0 && hit_day == local->pressed_day)
            {
                if (local->editing_range)
                {
                    calendar_view_finish_range_edit(self, hit_day);
                }
                else
                {
                    calendar_view_start_range_edit(self, hit_day);
                }
            }
        }
        local->pressed_part = EGUI_VIEW_CALENDAR_VIEW_PART_NONE;
        local->pressed_day = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_CALENDAR_VIEW_PART_NONE ? 1 : 0;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_calendar_view_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        switch (event->key_code)
        {
        case EGUI_KEY_CODE_TAB:
        case EGUI_KEY_CODE_LEFT:
        case EGUI_KEY_CODE_RIGHT:
        case EGUI_KEY_CODE_UP:
        case EGUI_KEY_CODE_DOWN:
        case EGUI_KEY_CODE_HOME:
        case EGUI_KEY_CODE_END:
        case EGUI_KEY_CODE_MINUS:
        case EGUI_KEY_CODE_PLUS:
        case EGUI_KEY_CODE_ENTER:
        case EGUI_KEY_CODE_SPACE:
        case EGUI_KEY_CODE_ESCAPE:
            return 1;
        default:
            return 0;
        }
    }

    if (egui_view_calendar_view_handle_navigation_key(self, event->key_code))
    {
        return 1;
    }
    return egui_view_on_key_event(self, event);
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_calendar_view_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_calendar_view_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_calendar_view_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_calendar_view_on_key_event,
#endif
};

void egui_view_calendar_view_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_calendar_view_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_calendar_view_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
    egui_view_set_on_focus_change_listener(self, egui_view_calendar_view_on_focus_change);
#endif

    local->on_changed = NULL;
    local->on_display_month_changed = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->label = NULL;
    local->helper = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->text_color = EGUI_COLOR_HEX(0x18222D);
    local->muted_text_color = EGUI_COLOR_HEX(0x69798A);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->today_color = EGUI_COLOR_HEX(0x0F766E);
    local->display_year = 2026;
    local->selection_year = 2026;
    local->committed_year = 2026;
    local->today_year = 2026;
    local->display_month = 3;
    local->selection_month = 3;
    local->committed_month = 3;
    local->start_day = 9;
    local->end_day = 11;
    local->committed_start_day = 9;
    local->committed_end_day = 11;
    local->focus_day = 11;
    local->anchor_day = 0;
    local->today_month = 3;
    local->today_day = 15;
    local->first_day_of_week = 1;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->editing_range = 0;
    local->current_part = EGUI_VIEW_CALENDAR_VIEW_PART_GRID;
    local->pressed_part = EGUI_VIEW_CALENDAR_VIEW_PART_NONE;
    local->pressed_day = 0;

    egui_view_set_view_name(self, "egui_view_calendar_view");
}
