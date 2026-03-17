#include "egui_view_time_picker.h"

#define EGUI_VIEW_TIME_PICKER_STANDARD_RADIUS             10
#define EGUI_VIEW_TIME_PICKER_STANDARD_FILL_ALPHA         94
#define EGUI_VIEW_TIME_PICKER_STANDARD_BORDER_ALPHA       60
#define EGUI_VIEW_TIME_PICKER_STANDARD_PAD_X              10
#define EGUI_VIEW_TIME_PICKER_STANDARD_PAD_Y              8
#define EGUI_VIEW_TIME_PICKER_STANDARD_LABEL_HEIGHT       10
#define EGUI_VIEW_TIME_PICKER_STANDARD_LABEL_GAP          4
#define EGUI_VIEW_TIME_PICKER_STANDARD_FIELD_HEIGHT       26
#define EGUI_VIEW_TIME_PICKER_STANDARD_FIELD_RADIUS       7
#define EGUI_VIEW_TIME_PICKER_STANDARD_FIELD_FILL_ALPHA   46
#define EGUI_VIEW_TIME_PICKER_STANDARD_FIELD_BORDER_ALPHA 56
#define EGUI_VIEW_TIME_PICKER_STANDARD_PANEL_GAP          6
#define EGUI_VIEW_TIME_PICKER_STANDARD_PANEL_HEIGHT       52
#define EGUI_VIEW_TIME_PICKER_STANDARD_PANEL_RADIUS       8
#define EGUI_VIEW_TIME_PICKER_STANDARD_PANEL_FILL_ALPHA   40
#define EGUI_VIEW_TIME_PICKER_STANDARD_PANEL_BORDER_ALPHA 54
#define EGUI_VIEW_TIME_PICKER_STANDARD_HELPER_GAP         5
#define EGUI_VIEW_TIME_PICKER_STANDARD_HELPER_HEIGHT      10

#define EGUI_VIEW_TIME_PICKER_COMPACT_RADIUS             8
#define EGUI_VIEW_TIME_PICKER_COMPACT_FILL_ALPHA         92
#define EGUI_VIEW_TIME_PICKER_COMPACT_BORDER_ALPHA       56
#define EGUI_VIEW_TIME_PICKER_COMPACT_PAD_X              7
#define EGUI_VIEW_TIME_PICKER_COMPACT_PAD_Y              6
#define EGUI_VIEW_TIME_PICKER_COMPACT_FIELD_HEIGHT       22
#define EGUI_VIEW_TIME_PICKER_COMPACT_FIELD_RADIUS       6
#define EGUI_VIEW_TIME_PICKER_COMPACT_FIELD_FILL_ALPHA   42
#define EGUI_VIEW_TIME_PICKER_COMPACT_FIELD_BORDER_ALPHA 52

#define EGUI_VIEW_TIME_PICKER_PART_NONE   0
#define EGUI_VIEW_TIME_PICKER_PART_FIELD  1
#define EGUI_VIEW_TIME_PICKER_PART_HOUR   2
#define EGUI_VIEW_TIME_PICKER_PART_MINUTE 3
#define EGUI_VIEW_TIME_PICKER_PART_PERIOD 4

typedef struct egui_view_time_picker_metrics egui_view_time_picker_metrics_t;
struct egui_view_time_picker_metrics
{
    egui_region_t content_region;
    egui_region_t label_region;
    egui_region_t field_region;
    egui_region_t helper_region;
    egui_region_t panel_region;
    egui_region_t field_segment_regions[3];
    egui_region_t chevron_region;
    egui_region_t panel_columns[3];
    uint8_t show_label;
    uint8_t show_helper;
    uint8_t show_panel;
    uint8_t show_period;
    uint8_t segment_count;
};

static uint8_t time_picker_normalize_step(uint8_t minute_step)
{
    if (minute_step == 0)
    {
        return 1;
    }
    if (minute_step > 30)
    {
        return 30;
    }
    return minute_step;
}

static uint8_t time_picker_round_minute(uint8_t minute, uint8_t minute_step)
{
    if (minute > 59)
    {
        minute = 59;
    }
    minute_step = time_picker_normalize_step(minute_step);
    return (uint8_t)((minute / minute_step) * minute_step);
}

static uint8_t time_picker_visible_segment_count(egui_view_time_picker_t *local)
{
    return local->use_24h ? 2 : 3;
}

static uint8_t time_picker_normalize_focus(egui_view_time_picker_t *local, uint8_t segment)
{
    uint8_t segment_count = time_picker_visible_segment_count(local);

    if (segment >= segment_count)
    {
        return segment_count - 1;
    }
    return segment;
}

static egui_color_t time_picker_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static void time_picker_format_2d(uint8_t value, char *buffer)
{
    buffer[0] = (char)('0' + (value / 10) % 10);
    buffer[1] = (char)('0' + value % 10);
    buffer[2] = '\0';
}

static void time_picker_get_segment_text(egui_view_time_picker_t *local, uint8_t segment, int8_t delta, char *buffer)
{
    int value;

    if (segment == EGUI_VIEW_TIME_PICKER_SEGMENT_HOUR)
    {
        value = local->hour24 + delta;
        while (value < 0)
        {
            value += 24;
        }
        while (value >= 24)
        {
            value -= 24;
        }
        if (local->use_24h)
        {
            time_picker_format_2d((uint8_t)value, buffer);
        }
        else
        {
            value %= 12;
            if (value == 0)
            {
                value = 12;
            }
            time_picker_format_2d((uint8_t)value, buffer);
        }
        return;
    }

    if (segment == EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE)
    {
        value = local->minute + delta * local->minute_step;
        while (value < 0)
        {
            value += 60;
        }
        while (value >= 60)
        {
            value -= 60;
        }
        time_picker_format_2d((uint8_t)value, buffer);
        return;
    }

    if ((((local->hour24 >= 12) ? 1 : 0) + delta) & 0x01)
    {
        buffer[0] = 'P';
        buffer[1] = 'M';
    }
    else
    {
        buffer[0] = 'A';
        buffer[1] = 'M';
    }
    buffer[2] = '\0';
}

static void time_picker_emit_time_changed(egui_view_t *self, egui_view_time_picker_t *local)
{
    if (local->on_time_changed)
    {
        local->on_time_changed(self, local->hour24, local->minute);
    }
}

static void time_picker_emit_open_changed(egui_view_t *self, egui_view_time_picker_t *local)
{
    if (local->on_open_changed)
    {
        local->on_open_changed(self, local->open_mode);
    }
}

static void time_picker_commit_time(egui_view_t *self, uint8_t hour24, uint8_t minute, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    uint8_t rounded_minute = time_picker_round_minute(minute, local->minute_step);

    if (hour24 > 23)
    {
        hour24 = 23;
    }

    if (local->hour24 == hour24 && local->minute == rounded_minute)
    {
        return;
    }

    local->hour24 = hour24;
    local->minute = rounded_minute;
    egui_view_invalidate(self);
    if (notify)
    {
        time_picker_emit_time_changed(self, local);
    }
}

static void time_picker_set_open_inner(egui_view_t *self, uint8_t opened, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);

    if (local->compact_mode || local->read_only_mode || !egui_view_get_enable(self))
    {
        opened = 0;
    }

    opened = opened ? 1 : 0;
    if (local->open_mode == opened)
    {
        return;
    }

    local->open_mode = opened;
    egui_view_invalidate(self);
    if (notify)
    {
        time_picker_emit_open_changed(self, local);
    }
}

static void time_picker_set_focus_inner(egui_view_t *self, uint8_t segment)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);

    segment = time_picker_normalize_focus(local, segment);
    if (local->focused_segment == segment)
    {
        return;
    }

    local->focused_segment = segment;
    egui_view_invalidate(self);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static void time_picker_move_focus(egui_view_t *self, int8_t delta)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    uint8_t segment_count = time_picker_visible_segment_count(local);
    int next = (int)local->focused_segment + delta;

    if (next < 0)
    {
        next = 0;
    }
    if (next >= segment_count)
    {
        next = segment_count - 1;
    }
    time_picker_set_focus_inner(self, (uint8_t)next);
}
#endif

static void time_picker_step_segment(egui_view_t *self, uint8_t segment, int8_t delta)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    int value;

    segment = time_picker_normalize_focus(local, segment);
    if (segment == EGUI_VIEW_TIME_PICKER_SEGMENT_HOUR)
    {
        value = local->hour24 + delta;
        while (value < 0)
        {
            value += 24;
        }
        while (value >= 24)
        {
            value -= 24;
        }
        time_picker_commit_time(self, (uint8_t)value, local->minute, 1);
        return;
    }

    if (segment == EGUI_VIEW_TIME_PICKER_SEGMENT_MINUTE)
    {
        value = local->minute + delta * local->minute_step;
        while (value < 0)
        {
            value += 60;
        }
        while (value >= 60)
        {
            value -= 60;
        }
        time_picker_commit_time(self, local->hour24, (uint8_t)value, 1);
        return;
    }

    if (!local->use_24h)
    {
        value = (local->hour24 + 12) % 24;
        time_picker_commit_time(self, (uint8_t)value, local->minute, 1);
    }
}

void egui_view_time_picker_set_time(egui_view_t *self, uint8_t hour24, uint8_t minute)
{
    time_picker_commit_time(self, hour24, minute, 0);
}

uint8_t egui_view_time_picker_get_hour24(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    return local->hour24;
}

uint8_t egui_view_time_picker_get_minute(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    return local->minute;
}

void egui_view_time_picker_set_minute_step(egui_view_t *self, uint8_t minute_step)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    uint8_t normalized = time_picker_normalize_step(minute_step);

    if (local->minute_step == normalized)
    {
        return;
    }

    local->minute_step = normalized;
    time_picker_commit_time(self, local->hour24, local->minute, 0);
}

uint8_t egui_view_time_picker_get_minute_step(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    return local->minute_step;
}

void egui_view_time_picker_set_use_24h(egui_view_t *self, uint8_t use_24h)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);

    use_24h = use_24h ? 1 : 0;
    if (local->use_24h == use_24h)
    {
        return;
    }

    local->use_24h = use_24h;
    local->focused_segment = time_picker_normalize_focus(local, local->focused_segment);
    egui_view_invalidate(self);
}

uint8_t egui_view_time_picker_get_use_24h(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    return local->use_24h;
}

void egui_view_time_picker_set_opened(egui_view_t *self, uint8_t opened)
{
    time_picker_set_open_inner(self, opened, 0);
}

uint8_t egui_view_time_picker_get_opened(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    return local->open_mode;
}

void egui_view_time_picker_set_focused_segment(egui_view_t *self, uint8_t focused_segment)
{
    time_picker_set_focus_inner(self, focused_segment);
}

uint8_t egui_view_time_picker_get_focused_segment(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    return local->focused_segment;
}

void egui_view_time_picker_set_label(egui_view_t *self, const char *label)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);

    local->label = label;
    egui_view_invalidate(self);
}

void egui_view_time_picker_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);

    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_time_picker_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);

    local->font = font != NULL ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_time_picker_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);

    local->meta_font = font != NULL ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_time_picker_set_on_time_changed_listener(egui_view_t *self, egui_view_on_time_picker_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    local->on_time_changed = listener;
}

void egui_view_time_picker_set_on_open_changed_listener(egui_view_t *self, egui_view_on_time_picker_open_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    local->on_open_changed = listener;
}

void egui_view_time_picker_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);

    compact_mode = compact_mode ? 1 : 0;
    if (local->compact_mode == compact_mode)
    {
        return;
    }

    local->compact_mode = compact_mode;
    if (compact_mode)
    {
        local->open_mode = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_time_picker_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);

    read_only_mode = read_only_mode ? 1 : 0;
    if (local->read_only_mode == read_only_mode)
    {
        return;
    }

    local->read_only_mode = read_only_mode;
    if (read_only_mode)
    {
        local->open_mode = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_time_picker_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t accent_color)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    egui_view_invalidate(self);
}

static void time_picker_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align, egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void time_picker_draw_chevron(egui_view_t *self, const egui_region_t *region, egui_color_t color, uint8_t opened)
{
    egui_dim_t cx = region->location.x + region->size.width / 2;
    egui_dim_t cy = region->location.y + region->size.height / 2;
    egui_alpha_t alpha = egui_color_alpha_mix(self->alpha, 90);

    if (opened)
    {
        egui_canvas_draw_line(cx - 3, cy + 1, cx, cy - 2, 1, color, alpha);
        egui_canvas_draw_line(cx, cy - 2, cx + 3, cy + 1, 1, color, alpha);
    }
    else
    {
        egui_canvas_draw_line(cx - 3, cy - 1, cx, cy + 2, 1, color, alpha);
        egui_canvas_draw_line(cx, cy + 2, cx + 3, cy - 1, 1, color, alpha);
    }
}

static void time_picker_get_metrics(egui_view_time_picker_t *local, egui_view_t *self, egui_view_time_picker_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_TIME_PICKER_COMPACT_PAD_X : EGUI_VIEW_TIME_PICKER_STANDARD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_TIME_PICKER_COMPACT_PAD_Y : EGUI_VIEW_TIME_PICKER_STANDARD_PAD_Y;
    egui_dim_t field_h = local->compact_mode ? EGUI_VIEW_TIME_PICKER_COMPACT_FIELD_HEIGHT : EGUI_VIEW_TIME_PICKER_STANDARD_FIELD_HEIGHT;
    egui_dim_t label_h = EGUI_VIEW_TIME_PICKER_STANDARD_LABEL_HEIGHT;
    egui_dim_t label_gap = EGUI_VIEW_TIME_PICKER_STANDARD_LABEL_GAP;
    egui_dim_t panel_gap = EGUI_VIEW_TIME_PICKER_STANDARD_PANEL_GAP;
    egui_dim_t panel_h = EGUI_VIEW_TIME_PICKER_STANDARD_PANEL_HEIGHT;
    egui_dim_t helper_gap = EGUI_VIEW_TIME_PICKER_STANDARD_HELPER_GAP;
    egui_dim_t helper_h = EGUI_VIEW_TIME_PICKER_STANDARD_HELPER_HEIGHT;
    egui_dim_t block_h;
    egui_dim_t cursor_y;
    egui_dim_t chevron_w = (!local->compact_mode && !local->read_only_mode) ? 14 : 0;
    egui_dim_t available_w;
    egui_dim_t hour_w;
    egui_dim_t minute_w;
    egui_dim_t period_w;
    egui_dim_t colon_w = 6;
    egui_dim_t gap = 6;
    egui_dim_t used_w;
    egui_dim_t start_x;
    egui_dim_t segment_height;
    egui_dim_t panel_inner_x;
    egui_dim_t column_gap = 6;
    egui_dim_t column_w;
    egui_dim_t column_h;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    metrics->content_region.location.x = region.location.x + pad_x;
    metrics->content_region.location.y = region.location.y + pad_y;
    metrics->content_region.size.width = region.size.width - pad_x * 2;
    metrics->content_region.size.height = region.size.height - pad_y * 2;
    metrics->show_label = (!local->compact_mode && local->label != NULL && local->label[0] != '\0') ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && local->helper != NULL && local->helper[0] != '\0') ? 1 : 0;
    metrics->show_panel = (!local->compact_mode && !local->read_only_mode && local->open_mode) ? 1 : 0;
    metrics->show_period = local->use_24h ? 0 : 1;
    metrics->segment_count = time_picker_visible_segment_count(local);

    cursor_y = metrics->content_region.location.y;
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

    available_w = metrics->field_region.size.width - 16 - chevron_w;
    if (available_w < 40)
    {
        available_w = 40;
    }
    if (metrics->segment_count == 3)
    {
        period_w = available_w > 86 ? 24 : 20;
        hour_w = (available_w - colon_w - gap - period_w) / 2;
        minute_w = hour_w;
        if (hour_w < 16)
        {
            hour_w = 16;
            minute_w = 16;
        }
        used_w = hour_w + colon_w + minute_w + gap + period_w;
    }
    else
    {
        period_w = 0;
        gap = 0;
        hour_w = (available_w - colon_w) / 2;
        minute_w = hour_w;
        if (hour_w < 18)
        {
            hour_w = 18;
            minute_w = 18;
        }
        used_w = hour_w + colon_w + minute_w;
    }

    start_x = metrics->field_region.location.x + (metrics->field_region.size.width - used_w - chevron_w) / 2;
    segment_height = metrics->field_region.size.height - 8;
    metrics->field_segment_regions[0].location.x = start_x;
    metrics->field_segment_regions[0].location.y = metrics->field_region.location.y + 4;
    metrics->field_segment_regions[0].size.width = hour_w;
    metrics->field_segment_regions[0].size.height = segment_height;
    metrics->field_segment_regions[1].location.x = start_x + hour_w + colon_w;
    metrics->field_segment_regions[1].location.y = metrics->field_segment_regions[0].location.y;
    metrics->field_segment_regions[1].size.width = minute_w;
    metrics->field_segment_regions[1].size.height = segment_height;
    metrics->field_segment_regions[2].location.x = start_x + hour_w + colon_w + minute_w + gap;
    metrics->field_segment_regions[2].location.y = metrics->field_segment_regions[0].location.y;
    metrics->field_segment_regions[2].size.width = period_w;
    metrics->field_segment_regions[2].size.height = segment_height;
    metrics->chevron_region.location.x = metrics->field_region.location.x + metrics->field_region.size.width - chevron_w;
    metrics->chevron_region.location.y = metrics->field_region.location.y;
    metrics->chevron_region.size.width = chevron_w;
    metrics->chevron_region.size.height = metrics->field_region.size.height;

    panel_inner_x = metrics->panel_region.location.x + 6;
    column_w = (metrics->panel_region.size.width - 12 - column_gap * (metrics->segment_count - 1)) / metrics->segment_count;
    column_h = metrics->panel_region.size.height - 12;
    for (i = 0; i < metrics->segment_count; ++i)
    {
        metrics->panel_columns[i].location.x = panel_inner_x + i * (column_w + column_gap);
        metrics->panel_columns[i].location.y = metrics->panel_region.location.y + 6;
        metrics->panel_columns[i].size.width = column_w;
        metrics->panel_columns[i].size.height = column_h;
    }
}

static uint8_t time_picker_hit_part(egui_view_time_picker_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_time_picker_metrics_t metrics;
    uint8_t i;

    time_picker_get_metrics(local, self, &metrics);
    if (egui_region_pt_in_rect(&metrics.field_region, x, y))
    {
        return EGUI_VIEW_TIME_PICKER_PART_FIELD;
    }

    if (!metrics.show_panel)
    {
        return EGUI_VIEW_TIME_PICKER_PART_NONE;
    }

    for (i = 0; i < metrics.segment_count; ++i)
    {
        if (egui_region_pt_in_rect(&metrics.panel_columns[i], x, y))
        {
            return (uint8_t)(EGUI_VIEW_TIME_PICKER_PART_HOUR + i);
        }
    }

    return EGUI_VIEW_TIME_PICKER_PART_NONE;
}

static uint8_t time_picker_panel_row(const egui_region_t *region, egui_dim_t y)
{
    egui_dim_t relative_y = y - region->location.y;
    egui_dim_t row_h = region->size.height / 3;

    if (relative_y < row_h)
    {
        return 0;
    }
    if (relative_y < row_h * 2)
    {
        return 1;
    }
    return 2;
}

static void time_picker_draw_field(egui_view_t *self, egui_view_time_picker_t *local, egui_view_time_picker_metrics_t *metrics, egui_color_t accent_color,
                                   egui_color_t text_color, egui_color_t muted_text_color, egui_color_t border_color)
{
    egui_color_t field_fill = egui_rgb_mix(local->surface_color, accent_color, local->open_mode ? 10 : 5);
    egui_color_t field_border = egui_rgb_mix(border_color, accent_color, local->open_mode ? 24 : 12);
    egui_color_t segment_fill;
    egui_color_t segment_border;
    egui_region_t colon_region;
    egui_alpha_t fill_alpha = local->compact_mode ? EGUI_VIEW_TIME_PICKER_COMPACT_FIELD_FILL_ALPHA : EGUI_VIEW_TIME_PICKER_STANDARD_FIELD_FILL_ALPHA;
    egui_alpha_t border_alpha = local->compact_mode ? EGUI_VIEW_TIME_PICKER_COMPACT_FIELD_BORDER_ALPHA : EGUI_VIEW_TIME_PICKER_STANDARD_FIELD_BORDER_ALPHA;
    uint8_t segment_count = metrics->segment_count;
    uint8_t i;
    char buffer[8];

    if (local->pressed_part == EGUI_VIEW_TIME_PICKER_PART_FIELD)
    {
        field_fill = egui_rgb_mix(field_fill, accent_color, 18);
    }

    egui_canvas_draw_round_rectangle_fill(metrics->field_region.location.x, metrics->field_region.location.y, metrics->field_region.size.width,
                                          metrics->field_region.size.height,
                                          local->compact_mode ? EGUI_VIEW_TIME_PICKER_COMPACT_FIELD_RADIUS : EGUI_VIEW_TIME_PICKER_STANDARD_FIELD_RADIUS,
                                          field_fill, egui_color_alpha_mix(self->alpha, fill_alpha));
    egui_canvas_draw_round_rectangle(metrics->field_region.location.x, metrics->field_region.location.y, metrics->field_region.size.width,
                                     metrics->field_region.size.height,
                                     local->compact_mode ? EGUI_VIEW_TIME_PICKER_COMPACT_FIELD_RADIUS : EGUI_VIEW_TIME_PICKER_STANDARD_FIELD_RADIUS, 1,
                                     field_border, egui_color_alpha_mix(self->alpha, border_alpha));

    for (i = 0; i < segment_count; ++i)
    {
        if (local->focused_segment == i && egui_view_get_enable(self) && !local->read_only_mode)
        {
            segment_fill = egui_rgb_mix(local->surface_color, accent_color, local->open_mode ? 18 : 12);
            segment_border = egui_rgb_mix(border_color, accent_color, 28);
            egui_canvas_draw_round_rectangle_fill(metrics->field_segment_regions[i].location.x, metrics->field_segment_regions[i].location.y,
                                                  metrics->field_segment_regions[i].size.width, metrics->field_segment_regions[i].size.height, 5, segment_fill,
                                                  egui_color_alpha_mix(self->alpha, local->open_mode ? 72 : 64));
            egui_canvas_draw_round_rectangle(metrics->field_segment_regions[i].location.x, metrics->field_segment_regions[i].location.y,
                                             metrics->field_segment_regions[i].size.width, metrics->field_segment_regions[i].size.height, 5, 1, segment_border,
                                             egui_color_alpha_mix(self->alpha, 72));
        }

        time_picker_get_segment_text(local, i, 0, buffer);
        time_picker_draw_text(local->font, self, buffer, &metrics->field_segment_regions[i], EGUI_ALIGN_CENTER,
                              local->focused_segment == i ? accent_color : text_color);
    }

    colon_region.location.x = metrics->field_segment_regions[0].location.x + metrics->field_segment_regions[0].size.width;
    colon_region.location.y = metrics->field_region.location.y;
    colon_region.size.width = metrics->field_segment_regions[1].location.x - colon_region.location.x;
    colon_region.size.height = metrics->field_region.size.height;
    time_picker_draw_text(local->font, self, ":", &colon_region, EGUI_ALIGN_CENTER, muted_text_color);

    if (!local->compact_mode && !local->read_only_mode)
    {
        time_picker_draw_chevron(self, &metrics->chevron_region, muted_text_color, local->open_mode);
    }
}

static void time_picker_draw_panel(egui_view_t *self, egui_view_time_picker_t *local, egui_view_time_picker_metrics_t *metrics, egui_color_t accent_color,
                                   egui_color_t text_color, egui_color_t muted_text_color, egui_color_t border_color)
{
    egui_color_t panel_fill = egui_rgb_mix(local->surface_color, accent_color, 7);
    egui_color_t panel_border = egui_rgb_mix(border_color, accent_color, 18);
    egui_color_t column_fill;
    egui_color_t row_fill;
    egui_region_t row_region;
    egui_dim_t row_h;
    uint8_t i;
    uint8_t row;
    char buffer[8];

    egui_canvas_draw_round_rectangle_fill(metrics->panel_region.location.x, metrics->panel_region.location.y, metrics->panel_region.size.width,
                                          metrics->panel_region.size.height, EGUI_VIEW_TIME_PICKER_STANDARD_PANEL_RADIUS, panel_fill,
                                          egui_color_alpha_mix(self->alpha, EGUI_VIEW_TIME_PICKER_STANDARD_PANEL_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(metrics->panel_region.location.x, metrics->panel_region.location.y, metrics->panel_region.size.width,
                                     metrics->panel_region.size.height, EGUI_VIEW_TIME_PICKER_STANDARD_PANEL_RADIUS, 1, panel_border,
                                     egui_color_alpha_mix(self->alpha, EGUI_VIEW_TIME_PICKER_STANDARD_PANEL_BORDER_ALPHA));

    for (i = 0; i < metrics->segment_count; ++i)
    {
        row_h = metrics->panel_columns[i].size.height / 3;
        column_fill = egui_rgb_mix(local->surface_color, accent_color, local->focused_segment == i ? 14 : 5);
        egui_canvas_draw_round_rectangle_fill(metrics->panel_columns[i].location.x, metrics->panel_columns[i].location.y, metrics->panel_columns[i].size.width,
                                              metrics->panel_columns[i].size.height, 6, column_fill,
                                              egui_color_alpha_mix(self->alpha, local->focused_segment == i ? 40 : 28));
        egui_canvas_draw_round_rectangle(metrics->panel_columns[i].location.x, metrics->panel_columns[i].location.y, metrics->panel_columns[i].size.width,
                                         metrics->panel_columns[i].size.height, 6, 1,
                                         egui_rgb_mix(border_color, accent_color, local->focused_segment == i ? 24 : 12),
                                         egui_color_alpha_mix(self->alpha, 56));

        row_region.location.x = metrics->panel_columns[i].location.x + 2;
        row_region.location.y = metrics->panel_columns[i].location.y + row_h;
        row_region.size.width = metrics->panel_columns[i].size.width - 4;
        row_region.size.height = row_h;
        row_fill = egui_rgb_mix(local->surface_color, accent_color, local->focused_segment == i ? 28 : 16);
        egui_canvas_draw_round_rectangle_fill(row_region.location.x, row_region.location.y, row_region.size.width, row_region.size.height, 5, row_fill,
                                              egui_color_alpha_mix(self->alpha, local->focused_segment == i ? 78 : 64));

        for (row = 0; row < 3; ++row)
        {
            row_region.location.x = metrics->panel_columns[i].location.x;
            row_region.location.y = metrics->panel_columns[i].location.y + row_h * row;
            row_region.size.width = metrics->panel_columns[i].size.width;
            row_region.size.height = row_h;
            time_picker_get_segment_text(local, i, (int8_t)(row - 1), buffer);
            time_picker_draw_text(row == 1 ? local->font : local->meta_font, self, buffer, &row_region, EGUI_ALIGN_CENTER,
                                  row == 1 ? (local->focused_segment == i ? accent_color : text_color) : muted_text_color);
        }
    }
}

static void egui_view_time_picker_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    egui_region_t region;
    egui_view_time_picker_metrics_t metrics;
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

    local->focused_segment = time_picker_normalize_focus(local, local->focused_segment);
    if (local->compact_mode || local->read_only_mode)
    {
        local->open_mode = 0;
    }

    if (local->read_only_mode)
    {
        accent_color = egui_rgb_mix(accent_color, muted_text_color, 72);
        outer_fill = egui_rgb_mix(outer_fill, local->surface_color, 16);
        outer_border = egui_rgb_mix(outer_border, muted_text_color, 18);
    }

    if (!egui_view_get_enable(self))
    {
        accent_color = time_picker_mix_disabled(accent_color);
        border_color = time_picker_mix_disabled(border_color);
        text_color = time_picker_mix_disabled(text_color);
        muted_text_color = time_picker_mix_disabled(muted_text_color);
        outer_fill = time_picker_mix_disabled(outer_fill);
        outer_border = time_picker_mix_disabled(outer_border);
    }

    time_picker_get_metrics(local, self, &metrics);
    egui_canvas_draw_round_rectangle_fill(
            region.location.x, region.location.y, region.size.width, region.size.height,
            local->compact_mode ? EGUI_VIEW_TIME_PICKER_COMPACT_RADIUS : EGUI_VIEW_TIME_PICKER_STANDARD_RADIUS, outer_fill,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_TIME_PICKER_COMPACT_FILL_ALPHA : EGUI_VIEW_TIME_PICKER_STANDARD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(
            region.location.x, region.location.y, region.size.width, region.size.height,
            local->compact_mode ? EGUI_VIEW_TIME_PICKER_COMPACT_RADIUS : EGUI_VIEW_TIME_PICKER_STANDARD_RADIUS, 1, outer_border,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_TIME_PICKER_COMPACT_BORDER_ALPHA : EGUI_VIEW_TIME_PICKER_STANDARD_BORDER_ALPHA));
    egui_canvas_draw_round_rectangle_fill(region.location.x + 2, region.location.y + 2, region.size.width - 4, local->compact_mode ? 3 : 4,
                                          local->compact_mode ? EGUI_VIEW_TIME_PICKER_COMPACT_RADIUS : EGUI_VIEW_TIME_PICKER_STANDARD_RADIUS, accent_color,
                                          egui_color_alpha_mix(self->alpha, local->read_only_mode ? 12 : 24));

    if (metrics.show_label)
    {
        time_picker_draw_text(local->meta_font, self, local->label, &metrics.label_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    }

    time_picker_draw_field(self, local, &metrics, accent_color, text_color, muted_text_color, border_color);

    if (metrics.show_panel)
    {
        time_picker_draw_panel(self, local, &metrics, accent_color, text_color, muted_text_color, border_color);
    }

    if (metrics.show_helper)
    {
        time_picker_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_time_picker_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);
    uint8_t hit_part;

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = time_picker_hit_part(local, self, event->location.x, event->location.y);
        if (hit_part == EGUI_VIEW_TIME_PICKER_PART_NONE)
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
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = time_picker_hit_part(local, self, event->location.x, event->location.y);
        if (local->pressed_part != EGUI_VIEW_TIME_PICKER_PART_NONE && local->pressed_part == hit_part)
        {
            if (hit_part == EGUI_VIEW_TIME_PICKER_PART_FIELD)
            {
                if (!local->compact_mode)
                {
                    time_picker_set_open_inner(self, local->open_mode ? 0 : 1, 1);
                }
            }
            else if (hit_part >= EGUI_VIEW_TIME_PICKER_PART_HOUR && hit_part <= EGUI_VIEW_TIME_PICKER_PART_PERIOD)
            {
                egui_view_time_picker_metrics_t metrics;
                uint8_t segment = (uint8_t)(hit_part - EGUI_VIEW_TIME_PICKER_PART_HOUR);
                uint8_t row;

                time_picker_get_metrics(local, self, &metrics);
                time_picker_set_focus_inner(self, segment);
                row = time_picker_panel_row(&metrics.panel_columns[segment], event->location.y);
                if (row == 0)
                {
                    time_picker_step_segment(self, segment, -1);
                }
                else if (row == 2)
                {
                    time_picker_step_segment(self, segment, 1);
                }
            }
        }
        local->pressed_part = EGUI_VIEW_TIME_PICKER_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_TIME_PICKER_PART_NONE ? 1 : 0;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_TIME_PICKER_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_time_picker_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_time_picker_t);

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
        case EGUI_KEY_CODE_HOME:
        case EGUI_KEY_CODE_END:
        case EGUI_KEY_CODE_UP:
        case EGUI_KEY_CODE_DOWN:
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
        time_picker_move_focus(self, -1);
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        time_picker_move_focus(self, 1);
        return 1;
    case EGUI_KEY_CODE_HOME:
        time_picker_set_focus_inner(self, EGUI_VIEW_TIME_PICKER_SEGMENT_HOUR);
        return 1;
    case EGUI_KEY_CODE_END:
        time_picker_set_focus_inner(self, (uint8_t)(time_picker_visible_segment_count(local) - 1));
        return 1;
    case EGUI_KEY_CODE_UP:
        time_picker_step_segment(self, local->focused_segment, -1);
        return 1;
    case EGUI_KEY_CODE_DOWN:
        time_picker_step_segment(self, local->focused_segment, 1);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (!local->compact_mode)
        {
            time_picker_set_open_inner(self, local->open_mode ? 0 : 1, 1);
        }
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        if (local->open_mode)
        {
            time_picker_set_open_inner(self, 0, 1);
            return 1;
        }
        return 0;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_time_picker_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_time_picker_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_time_picker_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_time_picker_on_key_event,
#endif
};

void egui_view_time_picker_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_time_picker_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_time_picker_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif

    local->on_time_changed = NULL;
    local->on_open_changed = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->label = NULL;
    local->helper = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->text_color = EGUI_COLOR_HEX(0x18222D);
    local->muted_text_color = EGUI_COLOR_HEX(0x69798A);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->hour24 = 8;
    local->minute = 0;
    local->minute_step = 15;
    local->focused_segment = EGUI_VIEW_TIME_PICKER_SEGMENT_HOUR;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->use_24h = 0;
    local->open_mode = 0;
    local->pressed_part = EGUI_VIEW_TIME_PICKER_PART_NONE;

    egui_view_set_view_name(self, "egui_view_time_picker");
}
