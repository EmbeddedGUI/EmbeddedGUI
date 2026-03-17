#include <stdlib.h>

#include "egui_view_tab_strip.h"

#define EGUI_VIEW_TAB_STRIP_STANDARD_BASE_WIDTH        18
#define EGUI_VIEW_TAB_STRIP_STANDARD_CHAR_WIDTH        5
#define EGUI_VIEW_TAB_STRIP_STANDARD_ACTIVE_BONUS      9
#define EGUI_VIEW_TAB_STRIP_STANDARD_MIN_WIDTH         28
#define EGUI_VIEW_TAB_STRIP_STANDARD_MAX_WIDTH         60
#define EGUI_VIEW_TAB_STRIP_STANDARD_GAP               9
#define EGUI_VIEW_TAB_STRIP_STANDARD_CONTENT_PAD_X     9
#define EGUI_VIEW_TAB_STRIP_STANDARD_CONTENT_PAD_Y     6
#define EGUI_VIEW_TAB_STRIP_STANDARD_LABEL_HEIGHT      18
#define EGUI_VIEW_TAB_STRIP_STANDARD_LABEL_OFFSET      6
#define EGUI_VIEW_TAB_STRIP_STANDARD_DIVIDER_OFFSET    9
#define EGUI_VIEW_TAB_STRIP_STANDARD_FILL_ALPHA        94
#define EGUI_VIEW_TAB_STRIP_STANDARD_BORDER_ALPHA      66
#define EGUI_VIEW_TAB_STRIP_STANDARD_ACTIVE_FILL_ALPHA 44
#define EGUI_VIEW_TAB_STRIP_STANDARD_ACTIVE_FILL_MIX   8
#define EGUI_VIEW_TAB_STRIP_STANDARD_TEXT_MIX          14

#define EGUI_VIEW_TAB_STRIP_COMPACT_BASE_WIDTH        12
#define EGUI_VIEW_TAB_STRIP_COMPACT_CHAR_WIDTH        4
#define EGUI_VIEW_TAB_STRIP_COMPACT_ACTIVE_BONUS      7
#define EGUI_VIEW_TAB_STRIP_COMPACT_MIN_WIDTH         22
#define EGUI_VIEW_TAB_STRIP_COMPACT_MAX_WIDTH         36
#define EGUI_VIEW_TAB_STRIP_COMPACT_GAP               4
#define EGUI_VIEW_TAB_STRIP_COMPACT_CONTENT_PAD_X     7
#define EGUI_VIEW_TAB_STRIP_COMPACT_CONTENT_PAD_Y     5
#define EGUI_VIEW_TAB_STRIP_COMPACT_LABEL_HEIGHT      13
#define EGUI_VIEW_TAB_STRIP_COMPACT_LABEL_OFFSET      3
#define EGUI_VIEW_TAB_STRIP_COMPACT_DIVIDER_OFFSET    8
#define EGUI_VIEW_TAB_STRIP_COMPACT_FILL_ALPHA        90
#define EGUI_VIEW_TAB_STRIP_COMPACT_BORDER_ALPHA      62
#define EGUI_VIEW_TAB_STRIP_COMPACT_ACTIVE_FILL_ALPHA 36
#define EGUI_VIEW_TAB_STRIP_COMPACT_ACTIVE_FILL_MIX   6
#define EGUI_VIEW_TAB_STRIP_COMPACT_TEXT_MIX          18

typedef struct egui_view_tab_strip_layout_item egui_view_tab_strip_layout_item_t;
struct egui_view_tab_strip_layout_item
{
    egui_dim_t x;
    egui_dim_t width;
    char label[16];
};

static uint8_t egui_view_tab_strip_clamp_tab_count(uint8_t count)
{
    if (count > EGUI_VIEW_TAB_STRIP_MAX_TABS)
    {
        return EGUI_VIEW_TAB_STRIP_MAX_TABS;
    }
    return count;
}

static egui_color_t egui_view_tab_strip_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 64);
}

static uint8_t egui_view_tab_strip_text_len(const char *text)
{
    uint8_t length = 0;

    if (text == NULL)
    {
        return 0;
    }
    while (text[length] != '\0')
    {
        length++;
    }
    return length;
}

static void egui_view_tab_strip_copy_elided(char *buffer, uint8_t buffer_size, const char *text, uint8_t max_chars)
{
    uint8_t length = 0;
    uint8_t copy_length;
    uint8_t i;

    if (buffer_size == 0)
    {
        return;
    }

    buffer[0] = '\0';
    if (text == NULL)
    {
        return;
    }

    while (text[length] != '\0')
    {
        length++;
    }
    if (max_chars == 0)
    {
        return;
    }

    if (length <= max_chars)
    {
        copy_length = length;
        if (copy_length >= buffer_size)
        {
            copy_length = buffer_size - 1;
        }
        for (i = 0; i < copy_length; i++)
        {
            buffer[i] = text[i];
        }
        buffer[copy_length] = '\0';
        return;
    }

    if (max_chars <= 3)
    {
        copy_length = max_chars;
        if (copy_length >= buffer_size)
        {
            copy_length = buffer_size - 1;
        }
        for (i = 0; i < copy_length; i++)
        {
            buffer[i] = '.';
        }
        buffer[copy_length] = '\0';
        return;
    }

    copy_length = max_chars - 3;
    if (copy_length > buffer_size - 4)
    {
        copy_length = buffer_size - 4;
    }
    for (i = 0; i < copy_length; i++)
    {
        buffer[i] = text[i];
    }
    buffer[copy_length + 0] = '.';
    buffer[copy_length + 1] = '.';
    buffer[copy_length + 2] = '.';
    buffer[copy_length + 3] = '\0';
}

static egui_dim_t egui_view_tab_strip_measure_tab_width(uint8_t compact_mode, uint8_t is_active, const char *text)
{
    egui_dim_t width;
    uint8_t length = egui_view_tab_strip_text_len(text);

    width = compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_BASE_WIDTH : EGUI_VIEW_TAB_STRIP_STANDARD_BASE_WIDTH;
    width += (egui_dim_t)length * (compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_CHAR_WIDTH : EGUI_VIEW_TAB_STRIP_STANDARD_CHAR_WIDTH);
    if (is_active)
    {
        width += compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_ACTIVE_BONUS : EGUI_VIEW_TAB_STRIP_STANDARD_ACTIVE_BONUS;
    }
    if (compact_mode)
    {
        if (width < EGUI_VIEW_TAB_STRIP_COMPACT_MIN_WIDTH)
        {
            width = EGUI_VIEW_TAB_STRIP_COMPACT_MIN_WIDTH;
        }
        if (width > EGUI_VIEW_TAB_STRIP_COMPACT_MAX_WIDTH)
        {
            width = EGUI_VIEW_TAB_STRIP_COMPACT_MAX_WIDTH;
        }
    }
    else
    {
        if (width < EGUI_VIEW_TAB_STRIP_STANDARD_MIN_WIDTH)
        {
            width = EGUI_VIEW_TAB_STRIP_STANDARD_MIN_WIDTH;
        }
        if (width > EGUI_VIEW_TAB_STRIP_STANDARD_MAX_WIDTH)
        {
            width = EGUI_VIEW_TAB_STRIP_STANDARD_MAX_WIDTH;
        }
    }
    return width;
}

static egui_dim_t egui_view_tab_strip_item_gap(uint8_t compact_mode)
{
    return compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_GAP : EGUI_VIEW_TAB_STRIP_STANDARD_GAP;
}

static uint8_t egui_view_tab_strip_prepare_layout(egui_view_tab_strip_t *local, egui_dim_t start_x, egui_dim_t available_width,
                                                  egui_view_tab_strip_layout_item_t *items)
{
    egui_dim_t total_width = 0;
    egui_dim_t gap = egui_view_tab_strip_item_gap(local->compact_mode);
    uint8_t count = egui_view_tab_strip_clamp_tab_count(local->tab_count);
    uint8_t i;

    if (count == 0 || local->tab_texts == NULL)
    {
        return 0;
    }

    for (i = 0; i < count; i++)
    {
        uint8_t is_active = i == local->current_index;
        uint8_t max_chars = local->compact_mode ? (is_active ? 6 : 5) : (is_active ? 10 : 8);

        egui_view_tab_strip_copy_elided(items[i].label, sizeof(items[i].label), local->tab_texts[i], max_chars);
        items[i].width = egui_view_tab_strip_measure_tab_width(local->compact_mode, is_active, items[i].label);
        total_width += items[i].width;
    }
    if (count > 1)
    {
        total_width += gap * (count - 1);
    }

    if (total_width > available_width)
    {
        egui_dim_t fallback_width = (available_width - gap * (count - 1)) / count;
        if (fallback_width < (local->compact_mode ? 20 : 24))
        {
            fallback_width = local->compact_mode ? 20 : 24;
        }
        for (i = 0; i < count; i++)
        {
            uint8_t max_chars = 1;

            if (fallback_width > (local->compact_mode ? 12 : 16))
            {
                max_chars = (uint8_t)((fallback_width - (local->compact_mode ? 12 : 16)) /
                                      (local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_CHAR_WIDTH : EGUI_VIEW_TAB_STRIP_STANDARD_CHAR_WIDTH));
            }
            egui_view_tab_strip_copy_elided(items[i].label, sizeof(items[i].label), local->tab_texts[i], max_chars);
            items[i].width = fallback_width;
        }
        total_width = fallback_width * count + gap * (count - 1);
    }

    if (total_width < available_width)
    {
        start_x += (available_width - total_width) / 2;
    }

    for (i = 0; i < count; i++)
    {
        items[i].x = start_x;
        start_x += items[i].width + gap;
    }

    return count;
}

void egui_view_tab_strip_set_tabs(egui_view_t *self, const char **tab_texts, uint8_t tab_count)
{
    EGUI_LOCAL_INIT(egui_view_tab_strip_t);

    local->tab_texts = tab_texts;
    local->tab_count = egui_view_tab_strip_clamp_tab_count(tab_count);
    if (local->current_index >= local->tab_count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_tab_strip_set_current_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_tab_strip_t);

    if (local->tab_count == 0 || index >= local->tab_count)
    {
        return;
    }
    if (local->current_index == index)
    {
        return;
    }

    local->current_index = index;
    if (local->on_tab_changed)
    {
        local->on_tab_changed(self, index);
    }
    egui_view_invalidate(self);
}

uint8_t egui_view_tab_strip_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_strip_t);
    return local->current_index;
}

void egui_view_tab_strip_set_on_tab_changed_listener(egui_view_t *self, egui_view_on_tab_strip_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_tab_strip_t);
    local->on_tab_changed = listener;
}

void egui_view_tab_strip_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_tab_strip_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_tab_strip_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_tab_strip_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_tab_strip_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_tab_strip_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_tab_strip_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                     egui_color_t muted_text_color, egui_color_t accent_color)
{
    EGUI_LOCAL_INIT(egui_view_tab_strip_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    egui_view_invalidate(self);
}

static void egui_view_tab_strip_draw_text(egui_view_tab_strip_t *local, egui_view_t *self, const char *text, egui_dim_t x, egui_dim_t y, egui_dim_t width,
                                          egui_dim_t height, egui_color_t color)
{
    egui_region_t text_region;

    text_region.location.x = x;
    text_region.location.y = y;
    text_region.size.width = width;
    text_region.size.height = height;
    egui_canvas_draw_text_in_rect(local->font, text, &text_region, EGUI_ALIGN_CENTER, color, self->alpha);
}

static uint8_t egui_view_tab_strip_resolve_hit(egui_view_tab_strip_t *local, egui_view_t *self, egui_dim_t screen_x)
{
    egui_region_t region;
    egui_view_tab_strip_layout_item_t items[EGUI_VIEW_TAB_STRIP_MAX_TABS];
    egui_dim_t content_x;
    egui_dim_t content_w;
    uint8_t count;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    content_x = region.location.x + (local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_CONTENT_PAD_X : EGUI_VIEW_TAB_STRIP_STANDARD_CONTENT_PAD_X);
    content_w = region.size.width - (local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_CONTENT_PAD_X * 2 : EGUI_VIEW_TAB_STRIP_STANDARD_CONTENT_PAD_X * 2);
    if (content_w <= 0)
    {
        return 0xFF;
    }

    count = egui_view_tab_strip_prepare_layout(local, content_x, content_w, items);
    for (i = 0; i < count; i++)
    {
        if (screen_x >= items[i].x && screen_x < items[i].x + items[i].width)
        {
            return i;
        }
    }
    return 0xFF;
}

static void egui_view_tab_strip_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_strip_t);
    egui_region_t region;
    egui_view_tab_strip_layout_item_t items[EGUI_VIEW_TAB_STRIP_MAX_TABS];
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t divider_y;
    egui_dim_t radius;
    egui_dim_t label_y;
    egui_dim_t label_h;
    uint8_t is_enabled;
    uint8_t count;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->tab_texts == NULL || local->tab_count == 0)
    {
        return;
    }

    surface_color = local->surface_color;
    border_color = local->border_color;
    text_color = local->text_color;
    muted_text_color = local->muted_text_color;
    accent_color = local->accent_color;
    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    radius = local->compact_mode ? 6 : 8;

    if (local->locked_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFBFCFD), 24);
        border_color = egui_rgb_mix(border_color, muted_text_color, 18);
        text_color = egui_rgb_mix(text_color, muted_text_color, 70);
        accent_color = egui_rgb_mix(accent_color, muted_text_color, 86);
    }
    else if (!local->compact_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFFFFFF), 14);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0xFFFFFF), 24);
    }

    if (!is_enabled)
    {
        surface_color = egui_view_tab_strip_mix_disabled(surface_color);
        border_color = egui_view_tab_strip_mix_disabled(border_color);
        text_color = egui_view_tab_strip_mix_disabled(text_color);
        muted_text_color = egui_view_tab_strip_mix_disabled(muted_text_color);
        accent_color = egui_view_tab_strip_mix_disabled(accent_color);
    }

    egui_canvas_draw_round_rectangle_fill(
            region.location.x, region.location.y, region.size.width, region.size.height, radius, surface_color,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_FILL_ALPHA : EGUI_VIEW_TAB_STRIP_STANDARD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(
            region.location.x, region.location.y, region.size.width, region.size.height, radius, 1, border_color,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_BORDER_ALPHA : EGUI_VIEW_TAB_STRIP_STANDARD_BORDER_ALPHA));

    content_x = region.location.x + (local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_CONTENT_PAD_X : EGUI_VIEW_TAB_STRIP_STANDARD_CONTENT_PAD_X);
    content_y = region.location.y + (local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_CONTENT_PAD_Y : EGUI_VIEW_TAB_STRIP_STANDARD_CONTENT_PAD_Y);
    content_w = region.size.width - (local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_CONTENT_PAD_X * 2 : EGUI_VIEW_TAB_STRIP_STANDARD_CONTENT_PAD_X * 2);
    if (content_w <= 0)
    {
        return;
    }

    divider_y = region.location.y + region.size.height -
                (local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_DIVIDER_OFFSET : EGUI_VIEW_TAB_STRIP_STANDARD_DIVIDER_OFFSET);
    label_h = local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_LABEL_HEIGHT : EGUI_VIEW_TAB_STRIP_STANDARD_LABEL_HEIGHT;
    label_y = divider_y - label_h - (local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_LABEL_OFFSET : EGUI_VIEW_TAB_STRIP_STANDARD_LABEL_OFFSET);
    count = egui_view_tab_strip_prepare_layout(local, content_x, content_w, items);

    egui_canvas_draw_line(content_x, divider_y, content_x + content_w, divider_y, 1, border_color,
                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 32 : 28));

    for (i = 0; i < count; i++)
    {
        egui_dim_t indicator_w;
        egui_dim_t indicator_x;
        uint8_t is_active = i == local->current_index;
        egui_color_t item_text_color = is_active ? text_color : muted_text_color;

        if (is_active)
        {
            egui_color_t active_fill =
                    egui_rgb_mix(surface_color, accent_color,
                                 local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_ACTIVE_FILL_MIX : EGUI_VIEW_TAB_STRIP_STANDARD_ACTIVE_FILL_MIX);
            egui_alpha_t active_alpha = local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_ACTIVE_FILL_ALPHA : EGUI_VIEW_TAB_STRIP_STANDARD_ACTIVE_FILL_ALPHA;

            egui_canvas_draw_round_rectangle_fill(items[i].x, label_y - 2, items[i].width, label_h + 2, local->compact_mode ? 4 : 5, active_fill,
                                                  egui_color_alpha_mix(self->alpha, active_alpha));
            if (!local->locked_mode && is_enabled)
            {
                item_text_color = egui_rgb_mix(text_color, accent_color,
                                               local->compact_mode ? EGUI_VIEW_TAB_STRIP_COMPACT_TEXT_MIX : EGUI_VIEW_TAB_STRIP_STANDARD_TEXT_MIX);
            }
        }
        else if (!local->compact_mode && count > 3)
        {
            item_text_color = egui_rgb_mix(muted_text_color, text_color, 6);
        }

        egui_view_tab_strip_draw_text(local, self, items[i].label, items[i].x, label_y, items[i].width, label_h, item_text_color);

        if (is_active)
        {
            indicator_w = items[i].width - (local->compact_mode ? 12 : 14);
            if (indicator_w < (local->compact_mode ? 12 : 16))
            {
                indicator_w = local->compact_mode ? 12 : 16;
            }
            indicator_x = items[i].x + (items[i].width - indicator_w) / 2;
            egui_canvas_draw_line(indicator_x, divider_y, indicator_x + indicator_w, divider_y, 1, accent_color,
                                  egui_color_alpha_mix(self->alpha, local->locked_mode ? 46 : 86));
            egui_canvas_draw_line(indicator_x, divider_y + 1, indicator_x + indicator_w, divider_y + 1, 1, accent_color,
                                  egui_color_alpha_mix(self->alpha, local->locked_mode ? 20 : 52));
        }
    }

    if (local->locked_mode)
    {
        egui_canvas_draw_line(content_x + 2, content_y + 1, content_x + content_w - 2, content_y + 1, 1, border_color, egui_color_alpha_mix(self->alpha, 16));
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_tab_strip_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_tab_strip_t);
    uint8_t hit_index;

    if (!egui_view_get_enable(self) || local->locked_mode || local->tab_count == 0)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_index = egui_view_tab_strip_resolve_hit(local, self, event->location.x);
        local->pressed_index = hit_index;
        egui_view_set_pressed(self, hit_index != 0xFF);
        return hit_index != 0xFF;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_index = egui_view_tab_strip_resolve_hit(local, self, event->location.x);
        if (local->pressed_index != 0xFF && local->pressed_index == hit_index)
        {
            egui_view_tab_strip_set_current_index(self, hit_index);
        }
        local->pressed_index = 0xFF;
        egui_view_set_pressed(self, false);
        return hit_index != 0xFF;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_index = 0xFF;
        egui_view_set_pressed(self, false);
        return 1;
    default:
        return 0;
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_tab_strip_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_tab_strip_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_tab_strip_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_tab_strip_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_tab_strip_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_tab_strip_t);
    egui_view_set_padding_all(self, 2);

    local->on_tab_changed = NULL;
    local->tab_texts = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DEE7);
    local->text_color = EGUI_COLOR_HEX(0x1B2430);
    local->muted_text_color = EGUI_COLOR_HEX(0x657382);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->tab_count = 0;
    local->current_index = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
    local->pressed_index = 0xFF;

    egui_view_set_view_name(self, "egui_view_tab_strip");
}
