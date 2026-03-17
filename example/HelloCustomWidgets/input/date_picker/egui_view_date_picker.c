#include "egui_view_date_picker.h"

#define EGUI_VIEW_DATE_PICKER_STANDARD_RADIUS             10
#define EGUI_VIEW_DATE_PICKER_STANDARD_FILL_ALPHA         94
#define EGUI_VIEW_DATE_PICKER_STANDARD_BORDER_ALPHA       60
#define EGUI_VIEW_DATE_PICKER_STANDARD_PAD_X              10
#define EGUI_VIEW_DATE_PICKER_STANDARD_PAD_Y              8
#define EGUI_VIEW_DATE_PICKER_STANDARD_LABEL_HEIGHT       10
#define EGUI_VIEW_DATE_PICKER_STANDARD_LABEL_GAP          4
#define EGUI_VIEW_DATE_PICKER_STANDARD_FIELD_HEIGHT       26
#define EGUI_VIEW_DATE_PICKER_STANDARD_FIELD_RADIUS       7
#define EGUI_VIEW_DATE_PICKER_STANDARD_FIELD_FILL_ALPHA   46
#define EGUI_VIEW_DATE_PICKER_STANDARD_FIELD_BORDER_ALPHA 56
#define EGUI_VIEW_DATE_PICKER_STANDARD_PANEL_GAP          6
#define EGUI_VIEW_DATE_PICKER_STANDARD_PANEL_HEIGHT       100
#define EGUI_VIEW_DATE_PICKER_STANDARD_PANEL_RADIUS       8
#define EGUI_VIEW_DATE_PICKER_STANDARD_PANEL_FILL_ALPHA   40
#define EGUI_VIEW_DATE_PICKER_STANDARD_PANEL_BORDER_ALPHA 54
#define EGUI_VIEW_DATE_PICKER_STANDARD_HELPER_GAP         5
#define EGUI_VIEW_DATE_PICKER_STANDARD_HELPER_HEIGHT      10

#define EGUI_VIEW_DATE_PICKER_COMPACT_RADIUS             8
#define EGUI_VIEW_DATE_PICKER_COMPACT_FILL_ALPHA         92
#define EGUI_VIEW_DATE_PICKER_COMPACT_BORDER_ALPHA       56
#define EGUI_VIEW_DATE_PICKER_COMPACT_PAD_X              7
#define EGUI_VIEW_DATE_PICKER_COMPACT_PAD_Y              6
#define EGUI_VIEW_DATE_PICKER_COMPACT_FIELD_HEIGHT       22
#define EGUI_VIEW_DATE_PICKER_COMPACT_FIELD_RADIUS       6
#define EGUI_VIEW_DATE_PICKER_COMPACT_FIELD_FILL_ALPHA   42
#define EGUI_VIEW_DATE_PICKER_COMPACT_FIELD_BORDER_ALPHA 52

#define EGUI_VIEW_DATE_PICKER_PART_NONE  0
#define EGUI_VIEW_DATE_PICKER_PART_FIELD 1
#define EGUI_VIEW_DATE_PICKER_PART_PREV  2
#define EGUI_VIEW_DATE_PICKER_PART_NEXT  3
#define EGUI_VIEW_DATE_PICKER_PART_DAY   4

typedef struct egui_view_date_picker_metrics egui_view_date_picker_metrics_t;
struct egui_view_date_picker_metrics
{
    egui_region_t content_region;
    egui_region_t label_region;
    egui_region_t field_region;
    egui_region_t helper_region;
    egui_region_t panel_region;
    egui_region_t field_text_region;
    egui_region_t chevron_region;
    egui_region_t panel_prev_region;
    egui_region_t panel_next_region;
    egui_region_t panel_title_region;
    egui_region_t panel_week_region;
    egui_region_t panel_grid_region;
    uint8_t show_label;
    uint8_t show_helper;
    uint8_t show_panel;
};

static const char *date_picker_month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char *date_picker_weekday_names_sun[] = {"S", "M", "T", "W", "T", "F", "S"};
static const char *date_picker_weekday_names_mon[] = {"M", "T", "W", "T", "F", "S", "S"};

static uint8_t date_picker_is_leap_year(uint16_t year)
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 1 : 0;
}

static uint8_t date_picker_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month < 1 || month > 12)
    {
        return 30;
    }
    if (month == 2 && date_picker_is_leap_year(year))
    {
        return 29;
    }
    return days[month - 1];
}

static uint8_t date_picker_day_of_week(uint16_t year, uint8_t month, uint8_t day)
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

static void date_picker_normalize_date(uint16_t *year, uint8_t *month, uint8_t *day)
{
    uint8_t max_day;

    if (*month < 1)
    {
        *month = 1;
    }
    else if (*month > 12)
    {
        *month = 12;
    }

    max_day = date_picker_days_in_month(*year, *month);
    if (*day < 1)
    {
        *day = 1;
    }
    else if (*day > max_day)
    {
        *day = max_day;
    }
}

static void date_picker_normalize_display_month(uint16_t *year, uint8_t *month)
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

static egui_color_t date_picker_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static void date_picker_format_date_field(uint16_t year, uint8_t month, uint8_t day, uint8_t compact_mode, char *buffer)
{
    if (compact_mode)
    {
        buffer[0] = date_picker_month_names[month - 1][0];
        buffer[1] = date_picker_month_names[month - 1][1];
        buffer[2] = date_picker_month_names[month - 1][2];
        buffer[3] = ' ';
        buffer[4] = (char)('0' + (day / 10) % 10);
        buffer[5] = (char)('0' + day % 10);
        buffer[6] = '\0';
        return;
    }

    buffer[0] = (char)('0' + (year / 1000) % 10);
    buffer[1] = (char)('0' + (year / 100) % 10);
    buffer[2] = (char)('0' + (year / 10) % 10);
    buffer[3] = (char)('0' + year % 10);
    buffer[4] = '-';
    buffer[5] = (char)('0' + (month / 10) % 10);
    buffer[6] = (char)('0' + month % 10);
    buffer[7] = '-';
    buffer[8] = (char)('0' + (day / 10) % 10);
    buffer[9] = (char)('0' + day % 10);
    buffer[10] = '\0';
}

static void date_picker_format_month_title(uint16_t year, uint8_t month, char *buffer)
{
    buffer[0] = date_picker_month_names[month - 1][0];
    buffer[1] = date_picker_month_names[month - 1][1];
    buffer[2] = date_picker_month_names[month - 1][2];
    buffer[3] = ' ';
    buffer[4] = (char)('0' + (year / 1000) % 10);
    buffer[5] = (char)('0' + (year / 100) % 10);
    buffer[6] = (char)('0' + (year / 10) % 10);
    buffer[7] = (char)('0' + year % 10);
    buffer[8] = '\0';
}

static void date_picker_emit_date_changed(egui_view_t *self, egui_view_date_picker_t *local)
{
    if (local->on_date_changed)
    {
        local->on_date_changed(self, local->year, local->month, local->day);
    }
}

static void date_picker_emit_open_changed(egui_view_t *self, egui_view_date_picker_t *local)
{
    if (local->on_open_changed)
    {
        local->on_open_changed(self, local->open_mode);
    }
}

static void date_picker_emit_display_month_changed(egui_view_t *self, egui_view_date_picker_t *local)
{
    if (local->on_display_month_changed)
    {
        local->on_display_month_changed(self, local->panel_year, local->panel_month);
    }
}

static uint8_t date_picker_assign_display_month(egui_view_date_picker_t *local, uint16_t year, uint8_t month)
{
    date_picker_normalize_display_month(&year, &month);
    if (local->panel_year == year && local->panel_month == month)
    {
        return 0;
    }

    local->panel_year = year;
    local->panel_month = month;
    return 1;
}

static void date_picker_commit_date(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    uint8_t date_changed;
    uint8_t panel_changed;

    date_picker_normalize_date(&year, &month, &day);
    date_changed = (local->year != year || local->month != month || local->day != day) ? 1 : 0;
    panel_changed = (local->panel_year != year || local->panel_month != month) ? 1 : 0;
    if (!date_changed && !panel_changed)
    {
        return;
    }

    local->year = year;
    local->month = month;
    local->day = day;
    local->panel_year = year;
    local->panel_month = month;
    local->preserve_display_month_on_open = 0;
    egui_view_invalidate(self);
    if (notify && panel_changed)
    {
        date_picker_emit_display_month_changed(self, local);
    }
    if (notify && date_changed)
    {
        date_picker_emit_date_changed(self, local);
    }
}

static void date_picker_set_open_inner(egui_view_t *self, uint8_t opened, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    uint8_t panel_changed = 0;

    if (local->compact_mode || local->read_only_mode || !egui_view_get_enable(self))
    {
        opened = 0;
    }

    opened = opened ? 1 : 0;
    if (local->open_mode == opened)
    {
        return;
    }

    if (opened)
    {
        if (local->preserve_display_month_on_open)
        {
            local->preserve_display_month_on_open = 0;
        }
        else
        {
            panel_changed = date_picker_assign_display_month(local, local->year, local->month);
        }
    }
    else
    {
        panel_changed = date_picker_assign_display_month(local, local->year, local->month);
        local->pressed_part = EGUI_VIEW_DATE_PICKER_PART_NONE;
        local->pressed_day = 0;
        egui_view_set_pressed(self, false);
    }
    local->open_mode = opened;
    egui_view_invalidate(self);
    if (notify && panel_changed)
    {
        date_picker_emit_display_month_changed(self, local);
    }
    if (notify)
    {
        date_picker_emit_open_changed(self, local);
    }
}

static void date_picker_shift_panel_month(egui_view_t *self, int8_t delta)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    int year = local->panel_year;
    int month = local->panel_month + delta;

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

    if (date_picker_assign_display_month(local, (uint16_t)year, (uint8_t)month))
    {
        egui_view_invalidate(self);
        date_picker_emit_display_month_changed(self, local);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static void date_picker_get_navigation_anchor(egui_view_date_picker_t *local, uint16_t *year, uint8_t *month, uint8_t *day)
{
    *year = local->year;
    *month = local->month;
    *day = local->day;

    if (local->open_mode && (local->panel_year != local->year || local->panel_month != local->month))
    {
        uint8_t max_day;

        *year = local->panel_year;
        *month = local->panel_month;
        max_day = date_picker_days_in_month(*year, *month);
        if (*day > max_day)
        {
            *day = max_day;
        }
    }
}

static void date_picker_shift_day(egui_view_t *self, int8_t delta)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    uint16_t anchor_year;
    uint8_t anchor_month;
    uint8_t anchor_day;
    int year;
    int month;
    int day;
    int max_day;

    date_picker_get_navigation_anchor(local, &anchor_year, &anchor_month, &anchor_day);
    year = anchor_year;
    month = anchor_month;
    day = anchor_day + delta;

    while (day < 1)
    {
        month--;
        if (month < 1)
        {
            month = 12;
            year--;
        }
        if (year < 1)
        {
            year = 1;
            month = 1;
            day = 1;
            break;
        }
        day += date_picker_days_in_month((uint16_t)year, (uint8_t)month);
    }

    max_day = date_picker_days_in_month((uint16_t)year, (uint8_t)month);
    while (day > max_day)
    {
        day -= max_day;
        month++;
        if (month > 12)
        {
            month = 1;
            year++;
        }
        max_day = date_picker_days_in_month((uint16_t)year, (uint8_t)month);
    }

    date_picker_commit_date(self, (uint16_t)year, (uint8_t)month, (uint8_t)day, 1);
}
#endif

void egui_view_date_picker_set_date(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day)
{
    date_picker_commit_date(self, year, month, day, 0);
}

uint16_t egui_view_date_picker_get_year(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    return local->year;
}

uint8_t egui_view_date_picker_get_month(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    return local->month;
}

uint8_t egui_view_date_picker_get_day(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    return local->day;
}

void egui_view_date_picker_set_today(egui_view_t *self, uint16_t year, uint8_t month, uint8_t day)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);

    date_picker_normalize_date(&year, &month, &day);
    local->today_year = year;
    local->today_month = month;
    local->today_day = day;
    egui_view_invalidate(self);
}

void egui_view_date_picker_set_first_day_of_week(egui_view_t *self, uint8_t first_day_of_week)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);

    first_day_of_week = first_day_of_week ? 1 : 0;
    if (local->first_day_of_week == first_day_of_week)
    {
        return;
    }
    local->first_day_of_week = first_day_of_week;
    egui_view_invalidate(self);
}

uint8_t egui_view_date_picker_get_first_day_of_week(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    return local->first_day_of_week;
}

void egui_view_date_picker_set_display_month(egui_view_t *self, uint16_t year, uint8_t month)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);

    if (!date_picker_assign_display_month(local, year, month))
    {
        return;
    }

    if (!local->open_mode)
    {
        local->preserve_display_month_on_open = 1;
    }
    egui_view_invalidate(self);
    date_picker_emit_display_month_changed(self, local);
}

uint16_t egui_view_date_picker_get_display_year(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    return local->panel_year;
}

uint8_t egui_view_date_picker_get_display_month(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    return local->panel_month;
}

void egui_view_date_picker_set_opened(egui_view_t *self, uint8_t opened)
{
    date_picker_set_open_inner(self, opened, 0);
}

uint8_t egui_view_date_picker_get_opened(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    return local->open_mode;
}

void egui_view_date_picker_set_label(egui_view_t *self, const char *label)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);

    local->label = label;
    egui_view_invalidate(self);
}

void egui_view_date_picker_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);

    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_date_picker_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);

    local->font = font != NULL ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_date_picker_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);

    local->meta_font = font != NULL ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_date_picker_set_on_date_changed_listener(egui_view_t *self, egui_view_on_date_picker_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    local->on_date_changed = listener;
}

void egui_view_date_picker_set_on_open_changed_listener(egui_view_t *self, egui_view_on_date_picker_open_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    local->on_open_changed = listener;
}

void egui_view_date_picker_set_on_display_month_changed_listener(egui_view_t *self, egui_view_on_date_picker_display_month_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    local->on_display_month_changed = listener;
}

void egui_view_date_picker_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);

    compact_mode = compact_mode ? 1 : 0;
    if (local->compact_mode == compact_mode)
    {
        return;
    }
    local->compact_mode = compact_mode;
    if (compact_mode && local->open_mode)
    {
        date_picker_set_open_inner(self, 0, 1);
        return;
    }
    egui_view_invalidate(self);
}

void egui_view_date_picker_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);

    read_only_mode = read_only_mode ? 1 : 0;
    if (local->read_only_mode == read_only_mode)
    {
        return;
    }
    local->read_only_mode = read_only_mode;
    if (read_only_mode && local->open_mode)
    {
        date_picker_set_open_inner(self, 0, 1);
        return;
    }
    egui_view_invalidate(self);
}

void egui_view_date_picker_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t today_color)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->today_color = today_color;
    egui_view_invalidate(self);
}

static void date_picker_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align, egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void date_picker_draw_chevron(egui_view_t *self, const egui_region_t *region, egui_color_t color, uint8_t direction)
{
    egui_dim_t cx = region->location.x + region->size.width / 2;
    egui_dim_t cy = region->location.y + region->size.height / 2;
    egui_alpha_t alpha = egui_color_alpha_mix(self->alpha, 90);

    if (direction == 0)
    {
        egui_canvas_draw_line(cx + 2, cy - 3, cx - 1, cy, 1, color, alpha);
        egui_canvas_draw_line(cx - 1, cy, cx + 2, cy + 3, 1, color, alpha);
    }
    else if (direction == 1)
    {
        egui_canvas_draw_line(cx - 2, cy - 3, cx + 1, cy, 1, color, alpha);
        egui_canvas_draw_line(cx + 1, cy, cx - 2, cy + 3, 1, color, alpha);
    }
    else if (direction == 2)
    {
        egui_canvas_draw_line(cx - 3, cy - 1, cx, cy + 2, 1, color, alpha);
        egui_canvas_draw_line(cx, cy + 2, cx + 3, cy - 1, 1, color, alpha);
    }
    else
    {
        egui_canvas_draw_line(cx - 3, cy + 1, cx, cy - 2, 1, color, alpha);
        egui_canvas_draw_line(cx, cy - 2, cx + 3, cy + 1, 1, color, alpha);
    }
}

static void date_picker_get_metrics(egui_view_date_picker_t *local, egui_view_t *self, egui_view_date_picker_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_PAD_X : EGUI_VIEW_DATE_PICKER_STANDARD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_PAD_Y : EGUI_VIEW_DATE_PICKER_STANDARD_PAD_Y;
    egui_dim_t field_h = local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_FIELD_HEIGHT : EGUI_VIEW_DATE_PICKER_STANDARD_FIELD_HEIGHT;
    egui_dim_t label_h = EGUI_VIEW_DATE_PICKER_STANDARD_LABEL_HEIGHT;
    egui_dim_t label_gap = EGUI_VIEW_DATE_PICKER_STANDARD_LABEL_GAP;
    egui_dim_t panel_gap = EGUI_VIEW_DATE_PICKER_STANDARD_PANEL_GAP;
    egui_dim_t panel_h = EGUI_VIEW_DATE_PICKER_STANDARD_PANEL_HEIGHT;
    egui_dim_t helper_gap = EGUI_VIEW_DATE_PICKER_STANDARD_HELPER_GAP;
    egui_dim_t helper_h = EGUI_VIEW_DATE_PICKER_STANDARD_HELPER_HEIGHT;
    egui_dim_t block_h;
    egui_dim_t cursor_y;

    egui_view_get_work_region(self, &region);
    metrics->content_region.location.x = region.location.x + pad_x;
    metrics->content_region.location.y = region.location.y + pad_y;
    metrics->content_region.size.width = region.size.width - pad_x * 2;
    metrics->content_region.size.height = region.size.height - pad_y * 2;
    metrics->show_label = (!local->compact_mode && local->label != NULL && local->label[0] != '\0') ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && local->helper != NULL && local->helper[0] != '\0') ? 1 : 0;
    metrics->show_panel = (!local->compact_mode && !local->read_only_mode && local->open_mode) ? 1 : 0;

    block_h = field_h;
    if (metrics->show_label)
    {
        block_h += label_h + label_gap;
    }
    if (metrics->show_panel)
    {
        block_h += panel_gap + panel_h;
    }
    if (metrics->show_helper)
    {
        block_h += helper_gap + helper_h;
    }

    cursor_y = metrics->content_region.location.y;
    if (!metrics->show_panel && metrics->content_region.size.height > block_h)
    {
        cursor_y = metrics->content_region.location.y + (metrics->content_region.size.height - block_h) / 2;
    }

    metrics->label_region.location.x = metrics->content_region.location.x;
    metrics->label_region.location.y = cursor_y;
    metrics->label_region.size.width = metrics->content_region.size.width;
    metrics->label_region.size.height = label_h;

    if (metrics->show_label)
    {
        cursor_y += label_h + label_gap;
    }

    metrics->field_region.location.x = metrics->content_region.location.x;
    metrics->field_region.location.y = cursor_y;
    metrics->field_region.size.width = metrics->content_region.size.width;
    metrics->field_region.size.height = field_h;
    cursor_y += field_h;

    metrics->panel_region.location.x = metrics->content_region.location.x;
    metrics->panel_region.location.y = cursor_y + panel_gap;
    metrics->panel_region.size.width = metrics->content_region.size.width;
    metrics->panel_region.size.height = panel_h;
    if (metrics->show_panel)
    {
        cursor_y += panel_gap + panel_h;
    }

    metrics->helper_region.location.x = metrics->content_region.location.x;
    metrics->helper_region.location.y = cursor_y + helper_gap;
    metrics->helper_region.size.width = metrics->content_region.size.width;
    metrics->helper_region.size.height = helper_h;

    metrics->field_text_region.location.x = metrics->field_region.location.x + 8;
    metrics->field_text_region.location.y = metrics->field_region.location.y;
    metrics->field_text_region.size.width = metrics->field_region.size.width - (local->read_only_mode ? 16 : 34);
    metrics->field_text_region.size.height = metrics->field_region.size.height;

    metrics->chevron_region.location.x = metrics->field_region.location.x + metrics->field_region.size.width - (local->read_only_mode ? 0 : 22);
    metrics->chevron_region.location.y = metrics->field_region.location.y;
    metrics->chevron_region.size.width = local->read_only_mode ? 0 : 18;
    metrics->chevron_region.size.height = metrics->field_region.size.height;

    metrics->panel_prev_region.location.x = metrics->panel_region.location.x + 5;
    metrics->panel_prev_region.location.y = metrics->panel_region.location.y + 3;
    metrics->panel_prev_region.size.width = 20;
    metrics->panel_prev_region.size.height = 16;

    metrics->panel_next_region.location.x = metrics->panel_region.location.x + metrics->panel_region.size.width - 25;
    metrics->panel_next_region.location.y = metrics->panel_region.location.y + 3;
    metrics->panel_next_region.size.width = 20;
    metrics->panel_next_region.size.height = 16;

    metrics->panel_title_region.location.x = metrics->panel_prev_region.location.x + metrics->panel_prev_region.size.width + 5;
    metrics->panel_title_region.location.y = metrics->panel_region.location.y + 4;
    metrics->panel_title_region.size.width = metrics->panel_region.size.width - 60;
    metrics->panel_title_region.size.height = 14;

    metrics->panel_week_region.location.x = metrics->panel_region.location.x + 6;
    metrics->panel_week_region.location.y = metrics->panel_region.location.y + 22;
    metrics->panel_week_region.size.width = metrics->panel_region.size.width - 12;
    metrics->panel_week_region.size.height = 12;

    metrics->panel_grid_region.location.x = metrics->panel_region.location.x + 6;
    metrics->panel_grid_region.location.y = metrics->panel_region.location.y + 36;
    metrics->panel_grid_region.size.width = metrics->panel_region.size.width - 12;
    metrics->panel_grid_region.size.height = metrics->panel_region.size.height - 42;
}

static uint8_t date_picker_get_start_cell(egui_view_date_picker_t *local)
{
    uint8_t first_dow = date_picker_day_of_week(local->panel_year, local->panel_month, 1);

    return (uint8_t)((first_dow - local->first_day_of_week + 7) % 7);
}

static uint8_t date_picker_get_display_anchor_day(egui_view_date_picker_t *local)
{
    uint8_t max_day;

    if (local->panel_year == local->year && local->panel_month == local->month)
    {
        return 0;
    }

    max_day = date_picker_days_in_month(local->panel_year, local->panel_month);
    if (local->day > max_day)
    {
        return max_day;
    }
    return local->day;
}

static uint8_t date_picker_get_hit_day(egui_view_date_picker_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_date_picker_metrics_t metrics;
    egui_dim_t cell_w;
    egui_dim_t cell_h;
    uint8_t col;
    uint8_t row;
    uint8_t start_cell;
    uint8_t day;

    date_picker_get_metrics(local, self, &metrics);
    if (!metrics.show_panel || !egui_region_pt_in_rect(&metrics.panel_grid_region, x, y))
    {
        return 0;
    }

    cell_w = metrics.panel_grid_region.size.width / 7;
    cell_h = metrics.panel_grid_region.size.height / 6;
    if (cell_w <= 0 || cell_h <= 0)
    {
        return 0;
    }

    col = (uint8_t)((x - metrics.panel_grid_region.location.x) / cell_w);
    row = (uint8_t)((y - metrics.panel_grid_region.location.y) / cell_h);
    start_cell = date_picker_get_start_cell(local);
    day = (uint8_t)(row * 7 + col + 1);

    if (day <= start_cell)
    {
        return 0;
    }

    day = (uint8_t)(day - start_cell);
    if (day < 1 || day > date_picker_days_in_month(local->panel_year, local->panel_month))
    {
        return 0;
    }

    return day;
}

static uint8_t date_picker_hit_part(egui_view_date_picker_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_date_picker_metrics_t metrics;

    date_picker_get_metrics(local, self, &metrics);
    if (egui_region_pt_in_rect(&metrics.field_region, x, y))
    {
        return EGUI_VIEW_DATE_PICKER_PART_FIELD;
    }

    if (!metrics.show_panel)
    {
        return EGUI_VIEW_DATE_PICKER_PART_NONE;
    }

    if (egui_region_pt_in_rect(&metrics.panel_prev_region, x, y))
    {
        return EGUI_VIEW_DATE_PICKER_PART_PREV;
    }
    if (egui_region_pt_in_rect(&metrics.panel_next_region, x, y))
    {
        return EGUI_VIEW_DATE_PICKER_PART_NEXT;
    }
    if (date_picker_get_hit_day(local, self, x, y) > 0)
    {
        return EGUI_VIEW_DATE_PICKER_PART_DAY;
    }

    return EGUI_VIEW_DATE_PICKER_PART_NONE;
}
static void date_picker_draw_field(egui_view_t *self, egui_view_date_picker_t *local, egui_view_date_picker_metrics_t *metrics, egui_color_t accent_color,
                                   egui_color_t text_color, egui_color_t muted_text_color, egui_color_t border_color)
{
    egui_color_t field_fill = egui_rgb_mix(local->surface_color, accent_color, local->open_mode ? 10 : 5);
    egui_color_t field_border = egui_rgb_mix(border_color, accent_color, local->open_mode ? 24 : 12);
    egui_alpha_t fill_alpha = local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_FIELD_FILL_ALPHA : EGUI_VIEW_DATE_PICKER_STANDARD_FIELD_FILL_ALPHA;
    egui_alpha_t border_alpha = local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_FIELD_BORDER_ALPHA : EGUI_VIEW_DATE_PICKER_STANDARD_FIELD_BORDER_ALPHA;
    char buffer[16];

    if (local->pressed_part == EGUI_VIEW_DATE_PICKER_PART_FIELD)
    {
        field_fill = egui_rgb_mix(field_fill, accent_color, 18);
    }

    egui_canvas_draw_round_rectangle_fill(metrics->field_region.location.x, metrics->field_region.location.y, metrics->field_region.size.width,
                                          metrics->field_region.size.height,
                                          local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_FIELD_RADIUS : EGUI_VIEW_DATE_PICKER_STANDARD_FIELD_RADIUS,
                                          field_fill, egui_color_alpha_mix(self->alpha, fill_alpha));
    egui_canvas_draw_round_rectangle(metrics->field_region.location.x, metrics->field_region.location.y, metrics->field_region.size.width,
                                     metrics->field_region.size.height,
                                     local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_FIELD_RADIUS : EGUI_VIEW_DATE_PICKER_STANDARD_FIELD_RADIUS, 1,
                                     field_border, egui_color_alpha_mix(self->alpha, border_alpha));

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (self->is_focused && egui_view_get_enable(self) && !local->read_only_mode && metrics->field_region.size.width > 4 &&
        metrics->field_region.size.height > 4)
    {
        egui_dim_t radius = local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_FIELD_RADIUS : EGUI_VIEW_DATE_PICKER_STANDARD_FIELD_RADIUS;

        egui_canvas_draw_round_rectangle(metrics->field_region.location.x, metrics->field_region.location.y, metrics->field_region.size.width,
                                         metrics->field_region.size.height, radius, 2, accent_color,
                                         egui_color_alpha_mix(self->alpha, local->open_mode ? 96 : 84));
        egui_canvas_draw_round_rectangle(metrics->field_region.location.x + 2, metrics->field_region.location.y + 2, metrics->field_region.size.width - 4,
                                         metrics->field_region.size.height - 4, radius > 2 ? (radius - 2) : radius, 1, accent_color,
                                         egui_color_alpha_mix(self->alpha, local->open_mode ? 48 : 40));
    }
#endif

    date_picker_format_date_field(local->year, local->month, local->day, local->compact_mode, buffer);
    date_picker_draw_text(local->font, self, buffer, &metrics->field_text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color);

    if (!local->read_only_mode && !local->compact_mode)
    {
        egui_dim_t divider_x = metrics->chevron_region.location.x - 4;
        egui_color_t divider_color = egui_rgb_mix(border_color, accent_color, local->open_mode ? 18 : 8);

        egui_canvas_draw_line(divider_x, metrics->field_region.location.y + 6, divider_x,
                              metrics->field_region.location.y + metrics->field_region.size.height - 7, 1, divider_color,
                              egui_color_alpha_mix(self->alpha, local->open_mode ? 52 : 34));
        date_picker_draw_chevron(self, &metrics->chevron_region, muted_text_color, local->open_mode ? 3 : 2);
    }
}

static void date_picker_draw_panel(egui_view_t *self, egui_view_date_picker_t *local, egui_view_date_picker_metrics_t *metrics, egui_color_t accent_color,
                                   egui_color_t text_color, egui_color_t muted_text_color, egui_color_t border_color)
{
    egui_color_t panel_fill = egui_rgb_mix(local->surface_color, accent_color, 7);
    egui_color_t panel_border = egui_rgb_mix(border_color, accent_color, 18);
    egui_color_t nav_fill = egui_rgb_mix(local->surface_color, accent_color, 10);
    egui_color_t nav_border = egui_rgb_mix(border_color, accent_color, 18);
    egui_dim_t cell_w = metrics->panel_grid_region.size.width / 7;
    egui_dim_t cell_h = metrics->panel_grid_region.size.height / 6;
    egui_dim_t col;
    egui_dim_t row;
    uint8_t start_cell = date_picker_get_start_cell(local);
    uint8_t anchor_day = date_picker_get_display_anchor_day(local);
    uint8_t total_days = date_picker_days_in_month(local->panel_year, local->panel_month);
    const char **weekdays = local->first_day_of_week ? date_picker_weekday_names_mon : date_picker_weekday_names_sun;
    egui_color_t title_color = (local->panel_year != local->year || local->panel_month != local->month) ? accent_color : text_color;
    char buffer[16];

    egui_canvas_draw_round_rectangle_fill(metrics->panel_region.location.x, metrics->panel_region.location.y, metrics->panel_region.size.width,
                                          metrics->panel_region.size.height, EGUI_VIEW_DATE_PICKER_STANDARD_PANEL_RADIUS, panel_fill,
                                          egui_color_alpha_mix(self->alpha, EGUI_VIEW_DATE_PICKER_STANDARD_PANEL_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(metrics->panel_region.location.x, metrics->panel_region.location.y, metrics->panel_region.size.width,
                                     metrics->panel_region.size.height, EGUI_VIEW_DATE_PICKER_STANDARD_PANEL_RADIUS, 1, panel_border,
                                     egui_color_alpha_mix(self->alpha, EGUI_VIEW_DATE_PICKER_STANDARD_PANEL_BORDER_ALPHA));

    egui_canvas_draw_line(metrics->panel_region.location.x + 8, metrics->panel_week_region.location.y - 3,
                          metrics->panel_region.location.x + metrics->panel_region.size.width - 9, metrics->panel_week_region.location.y - 3, 1, panel_border,
                          egui_color_alpha_mix(self->alpha, 34));
    egui_canvas_draw_round_rectangle_fill(metrics->panel_week_region.location.x, metrics->panel_week_region.location.y, metrics->panel_week_region.size.width,
                                          metrics->panel_week_region.size.height, 5, egui_rgb_mix(local->surface_color, accent_color, 8),
                                          egui_color_alpha_mix(self->alpha, 30));

    egui_canvas_draw_round_rectangle_fill(metrics->panel_prev_region.location.x, metrics->panel_prev_region.location.y, metrics->panel_prev_region.size.width,
                                          metrics->panel_prev_region.size.height, 5, nav_fill, egui_color_alpha_mix(self->alpha, 24));
    egui_canvas_draw_round_rectangle(metrics->panel_prev_region.location.x, metrics->panel_prev_region.location.y, metrics->panel_prev_region.size.width,
                                     metrics->panel_prev_region.size.height, 5, 1, nav_border, egui_color_alpha_mix(self->alpha, 36));
    egui_canvas_draw_round_rectangle_fill(metrics->panel_next_region.location.x, metrics->panel_next_region.location.y, metrics->panel_next_region.size.width,
                                          metrics->panel_next_region.size.height, 5, nav_fill, egui_color_alpha_mix(self->alpha, 24));
    egui_canvas_draw_round_rectangle(metrics->panel_next_region.location.x, metrics->panel_next_region.location.y, metrics->panel_next_region.size.width,
                                     metrics->panel_next_region.size.height, 5, 1, nav_border, egui_color_alpha_mix(self->alpha, 36));

    if (local->pressed_part == EGUI_VIEW_DATE_PICKER_PART_PREV)
    {
        egui_canvas_draw_round_rectangle_fill(metrics->panel_prev_region.location.x, metrics->panel_prev_region.location.y,
                                              metrics->panel_prev_region.size.width, metrics->panel_prev_region.size.height, 5,
                                              egui_rgb_mix(local->surface_color, accent_color, 18), egui_color_alpha_mix(self->alpha, 60));
    }
    if (local->pressed_part == EGUI_VIEW_DATE_PICKER_PART_NEXT)
    {
        egui_canvas_draw_round_rectangle_fill(metrics->panel_next_region.location.x, metrics->panel_next_region.location.y,
                                              metrics->panel_next_region.size.width, metrics->panel_next_region.size.height, 5,
                                              egui_rgb_mix(local->surface_color, accent_color, 18), egui_color_alpha_mix(self->alpha, 60));
    }

    date_picker_draw_chevron(self, &metrics->panel_prev_region, muted_text_color, 0);
    date_picker_draw_chevron(self, &metrics->panel_next_region, muted_text_color, 1);

    date_picker_format_month_title(local->panel_year, local->panel_month, buffer);
    date_picker_draw_text(local->font, self, buffer, &metrics->panel_title_region, EGUI_ALIGN_CENTER, title_color);
    if (local->panel_year != local->year || local->panel_month != local->month)
    {
        egui_dim_t line_half = metrics->panel_title_region.size.width > 40 ? 10 : 8;
        egui_dim_t center_x = metrics->panel_title_region.location.x + metrics->panel_title_region.size.width / 2;
        egui_dim_t underline_y = metrics->panel_title_region.location.y + metrics->panel_title_region.size.height - 1;

        egui_canvas_draw_line(center_x - line_half, underline_y, center_x + line_half, underline_y, 2, accent_color, egui_color_alpha_mix(self->alpha, 76));
    }

    for (col = 0; col < 7; ++col)
    {
        egui_region_t week_region;

        week_region.location.x = metrics->panel_week_region.location.x + col * (metrics->panel_week_region.size.width / 7);
        week_region.location.y = metrics->panel_week_region.location.y;
        week_region.size.width = metrics->panel_week_region.size.width / 7;
        week_region.size.height = metrics->panel_week_region.size.height;
        date_picker_draw_text(local->meta_font, self, weekdays[col], &week_region, EGUI_ALIGN_CENTER, muted_text_color);
    }

    for (row = 0; row < 6; ++row)
    {
        for (col = 0; col < 7; ++col)
        {
            uint8_t cell_index = (uint8_t)(row * 7 + col);
            egui_region_t cell_region;
            egui_region_t cell_inner_region;
            uint8_t day = 0;
            egui_color_t day_text_color = text_color;

            cell_region.location.x = metrics->panel_grid_region.location.x + col * cell_w;
            cell_region.location.y = metrics->panel_grid_region.location.y + row * cell_h;
            cell_region.size.width = cell_w;
            cell_region.size.height = cell_h;
            cell_inner_region.location.x = cell_region.location.x + (cell_w > 20 ? 2 : 1);
            cell_inner_region.location.y = cell_region.location.y + 1;
            cell_inner_region.size.width = cell_region.size.width - ((cell_w > 20 ? 2 : 1) * 2);
            cell_inner_region.size.height = cell_region.size.height - 2;

            if (cell_index >= start_cell && cell_index < start_cell + total_days)
            {
                day = (uint8_t)(cell_index - start_cell + 1);
            }

            if (day == 0)
            {
                continue;
            }

            if (local->pressed_part == EGUI_VIEW_DATE_PICKER_PART_DAY && day == local->pressed_day)
            {
                egui_color_t press_fill = egui_rgb_mix(local->surface_color, accent_color, 18);

                egui_canvas_draw_round_rectangle_fill(cell_inner_region.location.x, cell_inner_region.location.y, cell_inner_region.size.width,
                                                      cell_inner_region.size.height, 4, press_fill, egui_color_alpha_mix(self->alpha, 40));
                egui_canvas_draw_round_rectangle(cell_inner_region.location.x, cell_inner_region.location.y, cell_inner_region.size.width,
                                                 cell_inner_region.size.height, 4, 1, egui_rgb_mix(border_color, accent_color, 32),
                                                 egui_color_alpha_mix(self->alpha, 76));
            }

            if (local->year == local->panel_year && local->month == local->panel_month && day == local->day)
            {
                egui_canvas_draw_round_rectangle_fill(cell_inner_region.location.x, cell_inner_region.location.y, cell_inner_region.size.width,
                                                      cell_inner_region.size.height, 4, accent_color, egui_color_alpha_mix(self->alpha, 84));
                egui_canvas_draw_round_rectangle(cell_inner_region.location.x, cell_inner_region.location.y, cell_inner_region.size.width,
                                                 cell_inner_region.size.height, 4, 1, egui_rgb_mix(accent_color, EGUI_COLOR_WHITE, 16),
                                                 egui_color_alpha_mix(self->alpha, 78));
                day_text_color = EGUI_COLOR_WHITE;
            }
            else if (anchor_day > 0 && day == anchor_day)
            {
                egui_canvas_draw_round_rectangle_fill(cell_inner_region.location.x, cell_inner_region.location.y, cell_inner_region.size.width,
                                                      cell_inner_region.size.height, 4, egui_rgb_mix(local->surface_color, accent_color, 14),
                                                      egui_color_alpha_mix(self->alpha, 24));
                egui_canvas_draw_round_rectangle(cell_inner_region.location.x, cell_inner_region.location.y, cell_inner_region.size.width,
                                                 cell_inner_region.size.height, 4, 1, egui_rgb_mix(border_color, accent_color, 32),
                                                 egui_color_alpha_mix(self->alpha, 72));
                day_text_color = accent_color;
            }
            else if (local->today_year == local->panel_year && local->today_month == local->panel_month && local->today_day == day)
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

            buffer[0] = (char)('0' + (day / 10) % 10);
            buffer[1] = (char)('0' + day % 10);
            buffer[2] = '\0';
            if (day < 10)
            {
                buffer[0] = (char)('0' + day);
                buffer[1] = '\0';
            }

            date_picker_draw_text((local->year == local->panel_year && local->month == local->panel_month && day == local->day) ? local->font
                                                                                                                                : local->meta_font,
                                  self, buffer, &cell_region, EGUI_ALIGN_CENTER, day_text_color);
        }
    }
}

static void egui_view_date_picker_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    egui_region_t region;
    egui_view_date_picker_metrics_t metrics;
    egui_color_t accent_color = local->accent_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_text_color = local->muted_text_color;
    egui_color_t outer_fill = egui_rgb_mix(local->surface_color, local->accent_color, local->compact_mode ? 3 : 5);
    egui_color_t outer_border = egui_rgb_mix(local->border_color, local->accent_color, local->open_mode ? 18 : 10);

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    if (local->read_only_mode)
    {
        accent_color = egui_rgb_mix(accent_color, muted_text_color, 72);
        outer_fill = egui_rgb_mix(outer_fill, local->surface_color, 16);
        outer_border = egui_rgb_mix(outer_border, muted_text_color, 18);
    }

    if (!egui_view_get_enable(self))
    {
        accent_color = date_picker_mix_disabled(accent_color);
        border_color = date_picker_mix_disabled(border_color);
        text_color = date_picker_mix_disabled(text_color);
        muted_text_color = date_picker_mix_disabled(muted_text_color);
        outer_fill = date_picker_mix_disabled(outer_fill);
        outer_border = date_picker_mix_disabled(outer_border);
    }

    date_picker_get_metrics(local, self, &metrics);

    egui_canvas_draw_round_rectangle_fill(
            region.location.x, region.location.y, region.size.width, region.size.height,
            local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_RADIUS : EGUI_VIEW_DATE_PICKER_STANDARD_RADIUS, outer_fill,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_FILL_ALPHA : EGUI_VIEW_DATE_PICKER_STANDARD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(
            region.location.x, region.location.y, region.size.width, region.size.height,
            local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_RADIUS : EGUI_VIEW_DATE_PICKER_STANDARD_RADIUS, 1, outer_border,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_BORDER_ALPHA : EGUI_VIEW_DATE_PICKER_STANDARD_BORDER_ALPHA));
    egui_canvas_draw_round_rectangle_fill(region.location.x + 2, region.location.y + 2, region.size.width - 4, local->compact_mode ? 3 : 4,
                                          local->compact_mode ? EGUI_VIEW_DATE_PICKER_COMPACT_RADIUS : EGUI_VIEW_DATE_PICKER_STANDARD_RADIUS, accent_color,
                                          egui_color_alpha_mix(self->alpha, local->read_only_mode ? 12 : 24));

    if (metrics.show_label)
    {
        date_picker_draw_text(local->meta_font, self, local->label, &metrics.label_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    }

    date_picker_draw_field(self, local, &metrics, accent_color, text_color, muted_text_color, border_color);

    if (metrics.show_panel)
    {
        date_picker_draw_panel(self, local, &metrics, accent_color, text_color, muted_text_color, border_color);
    }

    if (metrics.show_helper)
    {
        date_picker_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void egui_view_date_picker_on_focus_change(egui_view_t *self, int is_focused)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);

    if (is_focused)
    {
        return;
    }

    local->pressed_part = EGUI_VIEW_DATE_PICKER_PART_NONE;
    local->pressed_day = 0;
    egui_view_set_pressed(self, false);

    if (local->open_mode)
    {
        date_picker_set_open_inner(self, 0, 1);
        return;
    }

    egui_view_invalidate(self);
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_date_picker_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);
    uint8_t hit_part;
    uint8_t hit_day;

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = date_picker_hit_part(local, self, event->location.x, event->location.y);
        if (hit_part == EGUI_VIEW_DATE_PICKER_PART_NONE)
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
        local->pressed_day = hit_part == EGUI_VIEW_DATE_PICKER_PART_DAY ? date_picker_get_hit_day(local, self, event->location.x, event->location.y) : 0;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = date_picker_hit_part(local, self, event->location.x, event->location.y);
        hit_day = hit_part == EGUI_VIEW_DATE_PICKER_PART_DAY ? date_picker_get_hit_day(local, self, event->location.x, event->location.y) : 0;
        if (local->pressed_part != EGUI_VIEW_DATE_PICKER_PART_NONE && local->pressed_part == hit_part)
        {
            if (hit_part == EGUI_VIEW_DATE_PICKER_PART_FIELD)
            {
                if (!local->compact_mode)
                {
                    date_picker_set_open_inner(self, local->open_mode ? 0 : 1, 1);
                }
            }
            else if (hit_part == EGUI_VIEW_DATE_PICKER_PART_PREV)
            {
                date_picker_shift_panel_month(self, -1);
            }
            else if (hit_part == EGUI_VIEW_DATE_PICKER_PART_NEXT)
            {
                date_picker_shift_panel_month(self, 1);
            }
            else if (hit_part == EGUI_VIEW_DATE_PICKER_PART_DAY && hit_day > 0 && hit_day == local->pressed_day)
            {
                date_picker_commit_date(self, local->panel_year, local->panel_month, hit_day, 1);
            }
        }
        local->pressed_part = EGUI_VIEW_DATE_PICKER_PART_NONE;
        local->pressed_day = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_DATE_PICKER_PART_NONE ? 1 : 0;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->pressed_part == EGUI_VIEW_DATE_PICKER_PART_DAY)
        {
            hit_day = date_picker_get_hit_day(local, self, event->location.x, event->location.y);
            if (local->pressed_day != hit_day)
            {
                local->pressed_day = hit_day;
                egui_view_invalidate(self);
            }
            return 1;
        }
        return local->pressed_part != EGUI_VIEW_DATE_PICKER_PART_NONE ? 1 : 0;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_DATE_PICKER_PART_NONE;
        local->pressed_day = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_date_picker_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_date_picker_t);

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        switch (event->key_code)
        {
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

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_LEFT:
        date_picker_shift_day(self, -1);
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        date_picker_shift_day(self, 1);
        return 1;
    case EGUI_KEY_CODE_UP:
        date_picker_shift_day(self, -7);
        return 1;
    case EGUI_KEY_CODE_DOWN:
        date_picker_shift_day(self, 7);
        return 1;
    case EGUI_KEY_CODE_HOME:
    {
        uint16_t nav_year;
        uint8_t nav_month;
        uint8_t nav_day;

        date_picker_get_navigation_anchor(local, &nav_year, &nav_month, &nav_day);
        date_picker_commit_date(self, nav_year, nav_month, 1, 1);
        return 1;
    }
    case EGUI_KEY_CODE_END:
    {
        uint16_t nav_year;
        uint8_t nav_month;
        uint8_t nav_day;

        date_picker_get_navigation_anchor(local, &nav_year, &nav_month, &nav_day);
        date_picker_commit_date(self, nav_year, nav_month, date_picker_days_in_month(nav_year, nav_month), 1);
        return 1;
    }
    case EGUI_KEY_CODE_MINUS:
        if (local->open_mode)
        {
            date_picker_shift_panel_month(self, -1);
            return 1;
        }
        return 0;
    case EGUI_KEY_CODE_PLUS:
        if (local->open_mode)
        {
            date_picker_shift_panel_month(self, 1);
            return 1;
        }
        return 0;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (!local->compact_mode)
        {
            if (!local->open_mode)
            {
                date_picker_set_open_inner(self, 1, 1);
            }
            else if (local->panel_year != local->year || local->panel_month != local->month)
            {
                uint8_t anchor_day = date_picker_get_display_anchor_day(local);

                if (anchor_day > 0)
                {
                    date_picker_commit_date(self, local->panel_year, local->panel_month, anchor_day, 1);
                }
            }
            else
            {
                date_picker_set_open_inner(self, 0, 1);
            }
        }
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        if (local->open_mode)
        {
            date_picker_set_open_inner(self, 0, 1);
            return 1;
        }
        return 0;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_date_picker_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_date_picker_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_date_picker_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_date_picker_on_key_event,
#endif
};

void egui_view_date_picker_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_date_picker_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_date_picker_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
    egui_view_set_on_focus_change_listener(self, egui_view_date_picker_on_focus_change);
#endif

    local->on_date_changed = NULL;
    local->on_open_changed = NULL;
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
    local->year = 2026;
    local->panel_year = 2026;
    local->month = 3;
    local->panel_month = 3;
    local->day = 18;
    local->today_year = 2026;
    local->today_month = 3;
    local->today_day = 15;
    local->first_day_of_week = 1;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->open_mode = 0;
    local->preserve_display_month_on_open = 0;
    local->pressed_part = EGUI_VIEW_DATE_PICKER_PART_NONE;
    local->pressed_day = 0;

    egui_view_set_view_name(self, "egui_view_date_picker");
}
