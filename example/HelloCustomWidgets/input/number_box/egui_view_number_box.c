#include <stdlib.h>

#include "egui_view_number_box.h"
#include "utils/egui_sprintf.h"

#define EGUI_VIEW_NUMBER_BOX_STANDARD_RADIUS              8
#define EGUI_VIEW_NUMBER_BOX_STANDARD_FILL_ALPHA          94
#define EGUI_VIEW_NUMBER_BOX_STANDARD_BORDER_ALPHA        66
#define EGUI_VIEW_NUMBER_BOX_STANDARD_CONTENT_PAD_X       10
#define EGUI_VIEW_NUMBER_BOX_STANDARD_CONTENT_PAD_Y       8
#define EGUI_VIEW_NUMBER_BOX_STANDARD_LABEL_HEIGHT        10
#define EGUI_VIEW_NUMBER_BOX_STANDARD_LABEL_GAP           5
#define EGUI_VIEW_NUMBER_BOX_STANDARD_ROW_HEIGHT          25
#define EGUI_VIEW_NUMBER_BOX_STANDARD_HELPER_GAP          5
#define EGUI_VIEW_NUMBER_BOX_STANDARD_HELPER_HEIGHT       10
#define EGUI_VIEW_NUMBER_BOX_STANDARD_BUTTON_WIDTH        24
#define EGUI_VIEW_NUMBER_BOX_STANDARD_BUTTON_GAP          5
#define EGUI_VIEW_NUMBER_BOX_STANDARD_FIELD_RADIUS        6
#define EGUI_VIEW_NUMBER_BOX_STANDARD_FIELD_FILL_ALPHA    44
#define EGUI_VIEW_NUMBER_BOX_STANDARD_FIELD_BORDER_ALPHA  56
#define EGUI_VIEW_NUMBER_BOX_STANDARD_BUTTON_FILL_ALPHA   36
#define EGUI_VIEW_NUMBER_BOX_STANDARD_BUTTON_BORDER_ALPHA 50
#define EGUI_VIEW_NUMBER_BOX_STANDARD_ICON_SIZE           4

#define EGUI_VIEW_NUMBER_BOX_COMPACT_RADIUS              6
#define EGUI_VIEW_NUMBER_BOX_COMPACT_FILL_ALPHA          90
#define EGUI_VIEW_NUMBER_BOX_COMPACT_BORDER_ALPHA        62
#define EGUI_VIEW_NUMBER_BOX_COMPACT_CONTENT_PAD_X       7
#define EGUI_VIEW_NUMBER_BOX_COMPACT_CONTENT_PAD_Y       6
#define EGUI_VIEW_NUMBER_BOX_COMPACT_ROW_HEIGHT          20
#define EGUI_VIEW_NUMBER_BOX_COMPACT_BUTTON_WIDTH        18
#define EGUI_VIEW_NUMBER_BOX_COMPACT_BUTTON_GAP          4
#define EGUI_VIEW_NUMBER_BOX_COMPACT_FIELD_RADIUS        5
#define EGUI_VIEW_NUMBER_BOX_COMPACT_FIELD_FILL_ALPHA    40
#define EGUI_VIEW_NUMBER_BOX_COMPACT_FIELD_BORDER_ALPHA  50
#define EGUI_VIEW_NUMBER_BOX_COMPACT_BUTTON_FILL_ALPHA   32
#define EGUI_VIEW_NUMBER_BOX_COMPACT_BUTTON_BORDER_ALPHA 42
#define EGUI_VIEW_NUMBER_BOX_COMPACT_ICON_SIZE           3

#define EGUI_VIEW_NUMBER_BOX_PART_NONE 0
#define EGUI_VIEW_NUMBER_BOX_PART_DEC  1
#define EGUI_VIEW_NUMBER_BOX_PART_INC  2

typedef struct egui_view_number_box_metrics egui_view_number_box_metrics_t;
struct egui_view_number_box_metrics
{
    egui_region_t content;
    egui_region_t label_region;
    egui_region_t helper_region;
    egui_region_t dec_region;
    egui_region_t field_region;
    egui_region_t inc_region;
    uint8_t show_meta;
    uint8_t show_buttons;
};

static egui_color_t egui_view_number_box_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 64);
}

static int16_t egui_view_number_box_clamp_value(egui_view_number_box_t *local, int16_t value)
{
    if (value < local->min_value)
    {
        value = local->min_value;
    }
    if (value > local->max_value)
    {
        value = local->max_value;
    }
    return value;
}

static void egui_view_number_box_build_value_text(egui_view_number_box_t *local)
{
    int pos = egui_sprintf_int(local->value_buffer, sizeof(local->value_buffer), local->value);

    if (local->suffix != NULL && local->suffix[0] != '\0')
    {
        pos += egui_sprintf_char(&local->value_buffer[pos], (int)sizeof(local->value_buffer) - pos, ' ');
        egui_sprintf_str(&local->value_buffer[pos], (int)sizeof(local->value_buffer) - pos, local->suffix);
    }
}

static void egui_view_number_box_commit_value(egui_view_t *self, int16_t value)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);

    value = egui_view_number_box_clamp_value(local, value);
    if (value == local->value)
    {
        return;
    }

    local->value = value;
    if (local->on_value_changed)
    {
        local->on_value_changed(self, value);
    }
    egui_view_invalidate(self);
}

static void egui_view_number_box_get_metrics(egui_view_number_box_t *local, egui_view_t *self, egui_view_number_box_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_CONTENT_PAD_X : EGUI_VIEW_NUMBER_BOX_STANDARD_CONTENT_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_CONTENT_PAD_Y : EGUI_VIEW_NUMBER_BOX_STANDARD_CONTENT_PAD_Y;
    egui_dim_t row_h = local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_ROW_HEIGHT : EGUI_VIEW_NUMBER_BOX_STANDARD_ROW_HEIGHT;
    egui_dim_t button_w = local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_BUTTON_WIDTH : EGUI_VIEW_NUMBER_BOX_STANDARD_BUTTON_WIDTH;
    egui_dim_t button_gap = local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_BUTTON_GAP : EGUI_VIEW_NUMBER_BOX_STANDARD_BUTTON_GAP;
    egui_dim_t row_y;
    egui_dim_t row_x;
    egui_dim_t row_w;

    egui_view_get_work_region(self, &region);
    metrics->content.location.x = region.location.x + pad_x;
    metrics->content.location.y = region.location.y + pad_y;
    metrics->content.size.width = region.size.width - pad_x * 2;
    metrics->content.size.height = region.size.height - pad_y * 2;
    metrics->show_meta = local->compact_mode ? 0 : 1;
    metrics->show_buttons = local->locked_mode ? 0 : 1;

    metrics->label_region.location.x = metrics->content.location.x;
    metrics->label_region.location.y = metrics->content.location.y;
    metrics->label_region.size.width = metrics->content.size.width;
    metrics->label_region.size.height = EGUI_VIEW_NUMBER_BOX_STANDARD_LABEL_HEIGHT;

    metrics->helper_region.location.x = metrics->content.location.x;
    metrics->helper_region.location.y = metrics->content.location.y + EGUI_VIEW_NUMBER_BOX_STANDARD_LABEL_HEIGHT + EGUI_VIEW_NUMBER_BOX_STANDARD_LABEL_GAP +
                                        EGUI_VIEW_NUMBER_BOX_STANDARD_ROW_HEIGHT + EGUI_VIEW_NUMBER_BOX_STANDARD_HELPER_GAP;
    metrics->helper_region.size.width = metrics->content.size.width;
    metrics->helper_region.size.height = EGUI_VIEW_NUMBER_BOX_STANDARD_HELPER_HEIGHT;

    if (metrics->show_meta)
    {
        row_y = metrics->content.location.y + EGUI_VIEW_NUMBER_BOX_STANDARD_LABEL_HEIGHT + EGUI_VIEW_NUMBER_BOX_STANDARD_LABEL_GAP;
    }
    else
    {
        row_y = metrics->content.location.y + (metrics->content.size.height - row_h) / 2;
    }

    row_x = metrics->content.location.x;
    row_w = metrics->content.size.width;

    metrics->dec_region.location.x = row_x;
    metrics->dec_region.location.y = row_y;
    metrics->dec_region.size.width = metrics->show_buttons ? button_w : 0;
    metrics->dec_region.size.height = row_h;

    metrics->inc_region.location.x = row_x + row_w - (metrics->show_buttons ? button_w : 0);
    metrics->inc_region.location.y = row_y;
    metrics->inc_region.size.width = metrics->show_buttons ? button_w : 0;
    metrics->inc_region.size.height = row_h;

    metrics->field_region.location.x = row_x + (metrics->show_buttons ? (button_w + button_gap) : 0);
    metrics->field_region.location.y = row_y;
    metrics->field_region.size.width = row_w - (metrics->show_buttons ? (button_w * 2 + button_gap * 2) : 0);
    metrics->field_region.size.height = row_h;
}

void egui_view_number_box_set_value(egui_view_t *self, int16_t value)
{
    egui_view_number_box_commit_value(self, value);
}

int16_t egui_view_number_box_get_value(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);
    return local->value;
}

void egui_view_number_box_set_range(egui_view_t *self, int16_t min_value, int16_t max_value)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);

    if (min_value > max_value)
    {
        int16_t temp = min_value;
        min_value = max_value;
        max_value = temp;
    }

    local->min_value = min_value;
    local->max_value = max_value;
    local->value = egui_view_number_box_clamp_value(local, local->value);
    egui_view_invalidate(self);
}

void egui_view_number_box_set_step(egui_view_t *self, int16_t step)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);

    if (step <= 0)
    {
        step = 1;
    }
    local->step = step;
}

void egui_view_number_box_set_label(egui_view_t *self, const char *label)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);
    local->label = label;
    egui_view_invalidate(self);
}

void egui_view_number_box_set_suffix(egui_view_t *self, const char *suffix)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);
    local->suffix = suffix;
    egui_view_invalidate(self);
}

void egui_view_number_box_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);
    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_number_box_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_number_box_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_number_box_set_on_value_changed_listener(egui_view_t *self, egui_view_on_number_box_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);
    local->on_value_changed = listener;
}

void egui_view_number_box_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_number_box_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);
    local->locked_mode = locked_mode ? 1 : 0;
    local->pressed_part = EGUI_VIEW_NUMBER_BOX_PART_NONE;
    egui_view_invalidate(self);
}

void egui_view_number_box_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                      egui_color_t muted_text_color, egui_color_t accent_color)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    egui_view_invalidate(self);
}

static void egui_view_number_box_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                           egui_color_t color)
{
    egui_region_t draw_region = *region;

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void egui_view_number_box_draw_icon(egui_view_t *self, const egui_region_t *region, uint8_t part, egui_color_t color, egui_alpha_t alpha,
                                           egui_dim_t icon_size)
{
    egui_dim_t cx = region->location.x + region->size.width / 2;
    egui_dim_t cy = region->location.y + region->size.height / 2;

    egui_canvas_draw_line(cx - icon_size, cy, cx + icon_size, cy, 1, color, egui_color_alpha_mix(self->alpha, alpha));
    if (part == EGUI_VIEW_NUMBER_BOX_PART_INC)
    {
        egui_canvas_draw_line(cx, cy - icon_size, cx, cy + icon_size, 1, color, egui_color_alpha_mix(self->alpha, alpha));
    }
}

static uint8_t egui_view_number_box_hit_part(egui_view_number_box_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_number_box_metrics_t metrics;

    egui_view_number_box_get_metrics(local, self, &metrics);
    if (metrics.show_buttons)
    {
        if (egui_region_pt_in_rect(&metrics.dec_region, x, y))
        {
            return EGUI_VIEW_NUMBER_BOX_PART_DEC;
        }
        if (egui_region_pt_in_rect(&metrics.inc_region, x, y))
        {
            return EGUI_VIEW_NUMBER_BOX_PART_INC;
        }
    }
    return EGUI_VIEW_NUMBER_BOX_PART_NONE;
}

static void egui_view_number_box_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);
    egui_region_t region;
    egui_view_number_box_metrics_t metrics;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_color_t field_fill;
    egui_color_t field_border;
    egui_color_t button_fill;
    egui_color_t button_border;
    egui_color_t button_icon;
    uint8_t is_enabled;
    egui_dim_t radius = local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_RADIUS : EGUI_VIEW_NUMBER_BOX_STANDARD_RADIUS;
    egui_dim_t field_radius = local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_FIELD_RADIUS : EGUI_VIEW_NUMBER_BOX_STANDARD_FIELD_RADIUS;
    egui_dim_t icon_size = local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_ICON_SIZE : EGUI_VIEW_NUMBER_BOX_STANDARD_ICON_SIZE;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    egui_view_number_box_build_value_text(local);
    egui_view_number_box_get_metrics(local, self, &metrics);

    surface_color = local->surface_color;
    border_color = local->border_color;
    text_color = local->text_color;
    muted_text_color = local->muted_text_color;
    accent_color = local->accent_color;
    field_fill = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFFFFFF), local->compact_mode ? 16 : 22);
    field_border = egui_rgb_mix(border_color, accent_color, local->compact_mode ? 10 : 12);
    button_fill = egui_rgb_mix(surface_color, accent_color, local->compact_mode ? 8 : 10);
    button_border = egui_rgb_mix(border_color, accent_color, local->compact_mode ? 12 : 16);
    button_icon = accent_color;
    is_enabled = egui_view_get_enable(self) ? 1 : 0;

    if (local->locked_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFBFCFD), 24);
        border_color = egui_rgb_mix(border_color, muted_text_color, 18);
        text_color = egui_rgb_mix(text_color, muted_text_color, 72);
        accent_color = egui_rgb_mix(accent_color, muted_text_color, 84);
        field_fill = egui_rgb_mix(field_fill, surface_color, 34);
        field_border = egui_rgb_mix(field_border, muted_text_color, 18);
        button_fill = egui_rgb_mix(button_fill, surface_color, 56);
        button_border = egui_rgb_mix(button_border, muted_text_color, 42);
        button_icon = egui_rgb_mix(button_icon, muted_text_color, 86);
    }

    if (!is_enabled)
    {
        surface_color = egui_view_number_box_mix_disabled(surface_color);
        border_color = egui_view_number_box_mix_disabled(border_color);
        text_color = egui_view_number_box_mix_disabled(text_color);
        muted_text_color = egui_view_number_box_mix_disabled(muted_text_color);
        accent_color = egui_view_number_box_mix_disabled(accent_color);
        field_fill = egui_view_number_box_mix_disabled(field_fill);
        field_border = egui_view_number_box_mix_disabled(field_border);
        button_fill = egui_view_number_box_mix_disabled(button_fill);
        button_border = egui_view_number_box_mix_disabled(button_border);
        button_icon = egui_view_number_box_mix_disabled(button_icon);
    }

    egui_canvas_draw_round_rectangle_fill(
            region.location.x, region.location.y, region.size.width, region.size.height, radius, surface_color,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_FILL_ALPHA : EGUI_VIEW_NUMBER_BOX_STANDARD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(
            region.location.x, region.location.y, region.size.width, region.size.height, radius, 1, border_color,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_BORDER_ALPHA : EGUI_VIEW_NUMBER_BOX_STANDARD_BORDER_ALPHA));

    if (metrics.show_meta && local->label != NULL)
    {
        egui_view_number_box_draw_text(local->meta_font, self, local->label, &metrics.label_region, EGUI_ALIGN_LEFT, muted_text_color);
    }

    egui_canvas_draw_round_rectangle_fill(metrics.field_region.location.x, metrics.field_region.location.y, metrics.field_region.size.width,
                                          metrics.field_region.size.height, field_radius, field_fill,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_FIELD_FILL_ALPHA
                                                                                                : EGUI_VIEW_NUMBER_BOX_STANDARD_FIELD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(metrics.field_region.location.x, metrics.field_region.location.y, metrics.field_region.size.width,
                                     metrics.field_region.size.height, field_radius, 1, field_border,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_FIELD_BORDER_ALPHA
                                                                                           : EGUI_VIEW_NUMBER_BOX_STANDARD_FIELD_BORDER_ALPHA));
    egui_view_number_box_draw_text(local->font, self, local->value_buffer, &metrics.field_region, EGUI_ALIGN_CENTER, text_color);

    if (metrics.show_buttons)
    {
        egui_alpha_t button_fill_alpha = local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_BUTTON_FILL_ALPHA : EGUI_VIEW_NUMBER_BOX_STANDARD_BUTTON_FILL_ALPHA;
        egui_alpha_t button_border_alpha =
                local->compact_mode ? EGUI_VIEW_NUMBER_BOX_COMPACT_BUTTON_BORDER_ALPHA : EGUI_VIEW_NUMBER_BOX_STANDARD_BUTTON_BORDER_ALPHA;
        egui_color_t dec_fill = button_fill;
        egui_color_t inc_fill = button_fill;

        if (local->pressed_part == EGUI_VIEW_NUMBER_BOX_PART_DEC)
        {
            dec_fill = egui_rgb_mix(button_fill, accent_color, 12);
        }
        if (local->pressed_part == EGUI_VIEW_NUMBER_BOX_PART_INC)
        {
            inc_fill = egui_rgb_mix(button_fill, accent_color, 12);
        }

        egui_canvas_draw_round_rectangle_fill(metrics.dec_region.location.x, metrics.dec_region.location.y, metrics.dec_region.size.width,
                                              metrics.dec_region.size.height, field_radius, dec_fill, egui_color_alpha_mix(self->alpha, button_fill_alpha));
        egui_canvas_draw_round_rectangle(metrics.dec_region.location.x, metrics.dec_region.location.y, metrics.dec_region.size.width,
                                         metrics.dec_region.size.height, field_radius, 1, button_border,
                                         egui_color_alpha_mix(self->alpha, button_border_alpha));
        egui_view_number_box_draw_icon(self, &metrics.dec_region, EGUI_VIEW_NUMBER_BOX_PART_DEC, button_icon, 86, icon_size);

        egui_canvas_draw_round_rectangle_fill(metrics.inc_region.location.x, metrics.inc_region.location.y, metrics.inc_region.size.width,
                                              metrics.inc_region.size.height, field_radius, inc_fill, egui_color_alpha_mix(self->alpha, button_fill_alpha));
        egui_canvas_draw_round_rectangle(metrics.inc_region.location.x, metrics.inc_region.location.y, metrics.inc_region.size.width,
                                         metrics.inc_region.size.height, field_radius, 1, button_border,
                                         egui_color_alpha_mix(self->alpha, button_border_alpha));
        egui_view_number_box_draw_icon(self, &metrics.inc_region, EGUI_VIEW_NUMBER_BOX_PART_INC, button_icon, 86, icon_size);
    }

    if (metrics.show_meta && local->helper != NULL)
    {
        egui_view_number_box_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, EGUI_ALIGN_LEFT,
                                       local->locked_mode ? egui_rgb_mix(muted_text_color, text_color, 12) : muted_text_color);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_number_box_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_number_box_t);
    uint8_t hit_part;

    if (!egui_view_get_enable(self) || local->locked_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = egui_view_number_box_hit_part(local, self, event->location.x, event->location.y);
        if (hit_part == EGUI_VIEW_NUMBER_BOX_PART_NONE)
        {
            return 0;
        }
        local->pressed_part = hit_part;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = egui_view_number_box_hit_part(local, self, event->location.x, event->location.y);
        if (local->pressed_part != EGUI_VIEW_NUMBER_BOX_PART_NONE && local->pressed_part == hit_part)
        {
            int16_t delta = hit_part == EGUI_VIEW_NUMBER_BOX_PART_INC ? local->step : (int16_t)(-local->step);
            egui_view_number_box_commit_value(self, (int16_t)(local->value + delta));
        }
        local->pressed_part = EGUI_VIEW_NUMBER_BOX_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_NUMBER_BOX_PART_NONE;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_NUMBER_BOX_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_number_box_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_number_box_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_number_box_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_number_box_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_number_box_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_number_box_t);
    egui_view_set_padding_all(self, 2);

    local->on_value_changed = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->label = NULL;
    local->suffix = NULL;
    local->helper = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DEE7);
    local->text_color = EGUI_COLOR_HEX(0x1B2430);
    local->muted_text_color = EGUI_COLOR_HEX(0x697789);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->value = 0;
    local->min_value = 0;
    local->max_value = 100;
    local->step = 1;
    local->compact_mode = 0;
    local->locked_mode = 0;
    local->pressed_part = EGUI_VIEW_NUMBER_BOX_PART_NONE;
    local->value_buffer[0] = '\0';

    egui_view_set_view_name(self, "egui_view_number_box");
}
