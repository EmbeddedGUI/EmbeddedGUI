#include "egui_view_segmented_control.h"
#include "egui_view_icon_font.h"
#include "resource/egui_resource.h"

static const egui_font_t *egui_view_segmented_control_get_icon_font(egui_view_segmented_control_t *local, egui_dim_t area_size)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    return egui_view_icon_font_get_auto(area_size, 18, 22);
}

static uint8_t egui_view_segmented_control_clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_SEGMENTED_CONTROL_MAX_SEGMENTS)
    {
        return EGUI_VIEW_SEGMENTED_CONTROL_MAX_SEGMENTS;
    }
    return count;
}

typedef struct egui_view_segmented_control_layout egui_view_segmented_control_layout_t;
struct egui_view_segmented_control_layout
{
    egui_region_t region;
    uint8_t count;
    uint8_t active_index;
    egui_dim_t padding;
    egui_dim_t gap;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_width;
    egui_dim_t content_height;
    egui_dim_t outer_radius;
    egui_dim_t segment_radius;
    egui_dim_t segment_x[EGUI_VIEW_SEGMENTED_CONTROL_MAX_SEGMENTS];
    egui_dim_t segment_width[EGUI_VIEW_SEGMENTED_CONTROL_MAX_SEGMENTS];
};

static bool egui_view_segmented_control_build_layout(egui_view_t *self, egui_view_segmented_control_t *local, egui_view_segmented_control_layout_t *layout)
{
    egui_view_get_work_region(self, &layout->region);
    if (layout->region.size.width <= 0 || layout->region.size.height <= 0)
    {
        return false;
    }

    layout->count = egui_view_segmented_control_clamp_count(local->segment_count);
    if (layout->count == 0)
    {
        return false;
    }

    layout->padding = local->horizontal_padding;
    if (layout->padding * 2 >= layout->region.size.width || layout->padding * 2 >= layout->region.size.height)
    {
        layout->padding = 0;
    }

    layout->content_x = layout->region.location.x + layout->padding;
    layout->content_y = layout->region.location.y + layout->padding;
    layout->content_width = layout->region.size.width - layout->padding * 2;
    layout->content_height = layout->region.size.height - layout->padding * 2;
    if (layout->content_width <= 0 || layout->content_height <= 0)
    {
        return false;
    }

    layout->gap = local->segment_gap;
    if (layout->count <= 1)
    {
        layout->gap = 0;
    }
    while (layout->gap > 0 && (layout->content_width - layout->gap * (layout->count - 1)) < layout->count)
    {
        layout->gap--;
    }

    egui_dim_t available_width = layout->content_width - layout->gap * (layout->count - 1);
    if (available_width < layout->count)
    {
        return false;
    }

    egui_dim_t base_width = available_width / layout->count;
    egui_dim_t remainder = available_width % layout->count;
    egui_dim_t cursor_x = layout->content_x;
    uint8_t i;
    for (i = 0; i < layout->count; i++)
    {
        egui_dim_t width = base_width;
        if (remainder > 0)
        {
            width++;
            remainder--;
        }
        layout->segment_x[i] = cursor_x;
        layout->segment_width[i] = width;
        cursor_x += width + layout->gap;
    }

    layout->active_index = local->current_index;
    if (layout->active_index >= layout->count)
    {
        layout->active_index = 0;
    }

    layout->outer_radius = local->corner_radius;
    layout->outer_radius = EGUI_MIN(layout->outer_radius, layout->region.size.height / 2);
    layout->outer_radius = EGUI_MIN(layout->outer_radius, layout->region.size.width / 2);
    layout->segment_radius = layout->outer_radius;
    layout->segment_radius = EGUI_MIN(layout->segment_radius, layout->content_height / 2);
    layout->segment_radius = EGUI_MIN(layout->segment_radius, (base_width + 1) / 2);
    return true;
}

static void egui_view_segmented_control_draw_segment_content(egui_view_segmented_control_t *local, const egui_region_t *segment_region, const char *icon,
                                                             const char *text, egui_color_t color)
{
    if (icon != NULL && icon[0] != '\0')
    {
        const egui_font_t *icon_font = egui_view_segmented_control_get_icon_font(local, EGUI_MIN(segment_region->size.width, segment_region->size.height));
        if (text != NULL && text[0] != '\0')
        {
            if (icon_font == NULL)
            {
                egui_region_t text_region = *segment_region;
                egui_canvas_draw_text_in_rect(local->font, text, &text_region, EGUI_ALIGN_CENTER, color, local->alpha);
                return;
            }

            egui_dim_t text_w = 0;
            egui_dim_t text_h = 0;
            egui_dim_t icon_area_h;
            egui_dim_t content_h;
            egui_dim_t content_y;
            egui_dim_t text_gap = local->icon_text_gap;
            egui_region_t icon_region;
            egui_region_t text_region;
            egui_region_t content_region = *segment_region;

            if (content_region.size.width > 4)
            {
                content_region.location.x += 2;
                content_region.size.width -= 4;
            }
            if (content_region.size.height > 4)
            {
                content_region.location.y += 2;
                content_region.size.height -= 4;
            }

            if (local->font != NULL && local->font->api != NULL && local->font->api->get_str_size != NULL)
            {
                local->font->api->get_str_size(local->font, text, 0, 0, &text_w, &text_h);
            }
            if (text_h < 0)
            {
                text_h = 0;
            }
            if (text_gap < 0)
            {
                text_gap = 0;
            }
            if (text_w <= 0)
            {
                text_gap = 0;
            }

            icon_area_h = content_region.size.height - text_h - text_gap;
            if (icon_area_h < content_region.size.height / 2)
            {
                icon_area_h = content_region.size.height / 2;
            }
            if (icon_area_h > content_region.size.height)
            {
                icon_area_h = content_region.size.height;
            }

            content_h = icon_area_h + ((text_h > 0) ? (text_h + text_gap) : 0);
            if (content_h > content_region.size.height)
            {
                content_h = content_region.size.height;
            }
            content_y = content_region.location.y + (content_region.size.height - content_h) / 2;

            icon_region.location.x = content_region.location.x;
            icon_region.location.y = content_y;
            icon_region.size.width = content_region.size.width;
            icon_region.size.height = icon_area_h;

            text_region.location.x = content_region.location.x;
            text_region.location.y = content_y + icon_area_h;
            text_region.size.width = content_region.size.width;
            text_region.size.height = content_region.location.y + content_region.size.height - text_region.location.y;

            egui_canvas_draw_text_in_rect(icon_font, icon, &icon_region, EGUI_ALIGN_CENTER, color, local->alpha);
            egui_canvas_draw_text_in_rect(local->font, text, &text_region, EGUI_ALIGN_CENTER, color, local->alpha);
        }
        else if (icon_font != NULL)
        {
            egui_region_t icon_region = *segment_region;
            egui_canvas_draw_text_in_rect(icon_font, icon, &icon_region, EGUI_ALIGN_CENTER, color, local->alpha);
        }
    }
    else if (text != NULL && text[0] != '\0')
    {
        egui_region_t text_region = *segment_region;
        egui_canvas_draw_text_in_rect(local->font, text, &text_region, EGUI_ALIGN_CENTER, color, local->alpha);
    }
}

static uint8_t egui_view_segmented_control_get_hit_index(egui_view_t *self, egui_dim_t local_x, egui_dim_t local_y)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    egui_view_segmented_control_layout_t layout;
    if (!egui_view_segmented_control_build_layout(self, local, &layout))
    {
        return EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE;
    }

    if (local_x < layout.content_x || local_y < layout.content_y || local_x >= layout.content_x + layout.content_width ||
        local_y >= layout.content_y + layout.content_height)
    {
        return EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE;
    }

    uint8_t i;
    for (i = 0; i < layout.count; i++)
    {
        egui_dim_t segment_start = layout.segment_x[i];
        egui_dim_t segment_end = segment_start + layout.segment_width[i];
        if (local_x >= segment_start && local_x < segment_end)
        {
            return i;
        }
    }
    return EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_segmented_control_on_key_event(egui_view_t *self, egui_key_event_t *event);
#endif

void egui_view_segmented_control_set_segments(egui_view_t *self, const char **segment_texts, uint8_t segment_count)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    uint8_t clamped_count = egui_view_segmented_control_clamp_count(segment_count);
    if (local->segment_texts == segment_texts && local->segment_count == clamped_count)
    {
        return;
    }
    local->segment_texts = segment_texts;
    local->segment_count = clamped_count;
    if (local->segment_count == 0)
    {
        local->current_index = 0;
    }
    else if (local->current_index >= local->segment_count)
    {
        local->current_index = local->segment_count - 1;
    }
    local->pressed_index = EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_set_segment_icons(egui_view_t *self, const char **segment_icons)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->segment_icons == segment_icons)
    {
        return;
    }

    local->segment_icons = segment_icons;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_set_current_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->segment_count == 0 || index >= local->segment_count)
    {
        return;
    }
    if (local->current_index == index)
    {
        return;
    }
    local->current_index = index;
    if (local->on_segment_changed != NULL)
    {
        local->on_segment_changed(self, index);
    }
    egui_view_invalidate(self);
}

uint8_t egui_view_segmented_control_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    return local->current_index;
}

void egui_view_segmented_control_set_on_segment_changed_listener(egui_view_t *self, egui_view_on_segment_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->on_segment_changed == listener)
    {
        return;
    }
    local->on_segment_changed = listener;
}

void egui_view_segmented_control_set_bg_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->bg_color.full == color.full)
    {
        return;
    }
    local->bg_color = color;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_set_selected_bg_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->selected_bg_color.full == color.full)
    {
        return;
    }
    local->selected_bg_color = color;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->text_color.full == color.full)
    {
        return;
    }
    local->text_color = color;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_set_selected_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->selected_text_color.full == color.full)
    {
        return;
    }
    local->selected_text_color = color;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_set_border_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->border_color.full == color.full)
    {
        return;
    }
    local->border_color = color;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_set_corner_radius(egui_view_t *self, uint8_t radius)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->corner_radius == radius)
    {
        return;
    }
    local->corner_radius = radius;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_set_segment_gap(egui_view_t *self, uint8_t gap)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->segment_gap == gap)
    {
        return;
    }
    local->segment_gap = gap;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_set_horizontal_padding(egui_view_t *self, uint8_t padding)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->horizontal_padding == padding)
    {
        return;
    }
    local->horizontal_padding = padding;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->font == font)
    {
        return;
    }
    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->icon_text_gap == gap)
    {
        return;
    }

    local->icon_text_gap = gap;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (local->segment_count == 0 || (local->segment_texts == NULL && local->segment_icons == NULL) || local->font == NULL)
    {
        return;
    }

    egui_view_segmented_control_layout_t layout;
    if (!egui_view_segmented_control_build_layout(self, local, &layout))
    {
        return;
    }

    bool is_enabled = egui_view_get_enable(self);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    bool is_focused = self->is_focused ? true : false;
#else
    bool is_focused = false;
#endif

    egui_color_t base_bg = local->bg_color;
    egui_color_t selected_bg = local->selected_bg_color;
    egui_color_t text_color = local->text_color;
    egui_color_t selected_text_color = local->selected_text_color;
    egui_color_t border_color = local->border_color;
    egui_color_t focus_color = EGUI_THEME_FOCUS;
    if (!is_enabled)
    {
        base_bg = egui_rgb_mix(base_bg, EGUI_THEME_DISABLED, EGUI_ALPHA_40);
        selected_bg = egui_rgb_mix(selected_bg, EGUI_THEME_DISABLED, EGUI_ALPHA_60);
        border_color = egui_rgb_mix(border_color, EGUI_THEME_DISABLED, EGUI_ALPHA_50);
        text_color = egui_rgb_mix(text_color, base_bg, EGUI_ALPHA_40);
        selected_text_color = egui_rgb_mix(selected_text_color, selected_bg, EGUI_ALPHA_50);
        focus_color = egui_rgb_mix(focus_color, EGUI_THEME_DISABLED, EGUI_ALPHA_50);
    }

    egui_canvas_draw_round_rectangle_fill(layout.region.location.x, layout.region.location.y, layout.region.size.width, layout.region.size.height,
                                          layout.outer_radius, base_bg, local->alpha);

    egui_canvas_draw_round_rectangle(layout.region.location.x, layout.region.location.y, layout.region.size.width, layout.region.size.height,
                                     layout.outer_radius, 1, border_color, local->alpha);

    egui_dim_t selected_x = layout.segment_x[layout.active_index];
    egui_dim_t selected_width = layout.segment_width[layout.active_index];
    egui_canvas_draw_round_rectangle_fill(selected_x, layout.content_y, selected_width, layout.content_height, layout.segment_radius, selected_bg,
                                          local->alpha);

    if (is_focused && is_enabled && layout.region.size.width > 4 && layout.region.size.height > 4)
    {
        egui_dim_t focus_x = layout.region.location.x + 2;
        egui_dim_t focus_y = layout.region.location.y + 2;
        egui_dim_t focus_w = layout.region.size.width - 4;
        egui_dim_t focus_h = layout.region.size.height - 4;
        egui_dim_t focus_radius = layout.outer_radius > 2 ? (layout.outer_radius - 2) : layout.outer_radius;

        egui_canvas_draw_round_rectangle(layout.region.location.x, layout.region.location.y, layout.region.size.width, layout.region.size.height,
                                         layout.outer_radius, 2, focus_color, egui_color_alpha_mix(local->alpha, 100));
        egui_canvas_draw_round_rectangle(focus_x, focus_y, focus_w, focus_h, focus_radius, 1, focus_color, egui_color_alpha_mix(local->alpha, 56));
    }

    if (is_enabled && local->pressed_index < layout.count)
    {
        egui_dim_t pressed_x = layout.segment_x[local->pressed_index];
        egui_dim_t pressed_width = layout.segment_width[local->pressed_index];
        egui_color_t pressed_color = EGUI_THEME_PRESS_OVERLAY;
        if (local->pressed_index == layout.active_index)
        {
            pressed_color = egui_rgb_mix(EGUI_THEME_PRESS_OVERLAY, selected_bg, EGUI_ALPHA_40);
        }
        egui_canvas_draw_round_rectangle_fill(pressed_x, layout.content_y, pressed_width, layout.content_height, layout.segment_radius, pressed_color,
                                              EGUI_THEME_PRESS_OVERLAY_ALPHA);
    }

    if (layout.count > 1 && layout.gap == 0 && layout.content_height > 6)
    {
        egui_color_t separator_color = egui_rgb_mix(border_color, base_bg, EGUI_ALPHA_50);
        uint8_t i;
        for (i = 1; i < layout.count; i++)
        {
            egui_dim_t separator_x = layout.segment_x[i] - 1;
            egui_canvas_draw_rectangle_fill(separator_x, layout.content_y + 2, 1, layout.content_height - 4, separator_color, local->alpha);
        }
    }

    uint8_t i;
    for (i = 0; i < layout.count; i++)
    {
        const char *text = (local->segment_texts != NULL) ? local->segment_texts[i] : NULL;
        const char *icon = (local->segment_icons != NULL) ? local->segment_icons[i] : NULL;
        if (text == NULL)
        {
            text = "";
        }
        egui_region_t text_region = {
                .location =
                        {
                                .x = layout.segment_x[i],
                                .y = layout.content_y,
                        },
                .size =
                        {
                                .width = layout.segment_width[i],
                                .height = layout.content_height,
                        },
        };
        egui_color_t color = (i == layout.active_index) ? selected_text_color : text_color;
        egui_view_segmented_control_draw_segment_content(local, &text_region, icon, text, color);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_segmented_control_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    if (!egui_view_get_enable(self) || local->segment_count == 0 || (local->segment_texts == NULL && local->segment_icons == NULL))
    {
        return 0;
    }

    egui_dim_t local_x = event->location.x - self->region_screen.location.x;
    egui_dim_t local_y = event->location.y - self->region_screen.location.y;
    uint8_t hit_index = egui_view_segmented_control_get_hit_index(self, local_x, local_y);

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (self->is_focusable)
        {
            egui_view_request_focus(self);
        }
#endif
        if (local->pressed_index != hit_index)
        {
            local->pressed_index = hit_index;
            egui_view_invalidate(self);
        }
        egui_view_set_pressed(self, hit_index != EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE);
        break;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        egui_view_set_pressed(self, local->pressed_index != EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE && local->pressed_index == hit_index);
        break;
    case EGUI_MOTION_EVENT_ACTION_UP:
        if (self->is_pressed && local->pressed_index != EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE && local->pressed_index == hit_index)
        {
            egui_view_segmented_control_set_current_index(self, hit_index);
        }
        local->pressed_index = EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        break;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_index = EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        break;
    default:
        break;
    }

    return 1;
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_segmented_control_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    uint8_t count = egui_view_segmented_control_clamp_count(local->segment_count);
    uint8_t next_index;

    if (!egui_view_get_enable(self) || count == 0)
    {
        return 0;
    }

    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        if ((event->key_code == EGUI_KEY_CODE_ENTER || event->key_code == EGUI_KEY_CODE_SPACE) && event->type == EGUI_KEY_EVENT_ACTION_DOWN)
        {
            return 1;
        }
        return 0;
    }

    next_index = local->current_index;
    if (next_index >= count)
    {
        next_index = 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_LEFT:
    case EGUI_KEY_CODE_UP:
        if (next_index > 0)
        {
            next_index--;
        }
        egui_view_segmented_control_set_current_index(self, next_index);
        return 1;
    case EGUI_KEY_CODE_RIGHT:
    case EGUI_KEY_CODE_DOWN:
        if (next_index + 1 < count)
        {
            next_index++;
        }
        egui_view_segmented_control_set_current_index(self, next_index);
        return 1;
    case EGUI_KEY_CODE_HOME:
        egui_view_segmented_control_set_current_index(self, 0);
        return 1;
    case EGUI_KEY_CODE_END:
        egui_view_segmented_control_set_current_index(self, count - 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        next_index++;
        if (next_index >= count)
        {
            next_index = 0;
        }
        egui_view_segmented_control_set_current_index(self, next_index);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        return 1;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_segmented_control_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_segmented_control_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_segmented_control_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_segmented_control_on_key_event,
#endif
};

void egui_view_segmented_control_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_segmented_control_t);
    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_segmented_control_t);

    local->segment_texts = NULL;
    local->segment_icons = NULL;
    local->segment_count = 0;
    local->current_index = 0;
    local->pressed_index = EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE;
    local->horizontal_padding = 2;
    local->segment_gap = 2;
    local->corner_radius = EGUI_THEME_RADIUS_MD;
    local->alpha = EGUI_ALPHA_100;
    local->bg_color = EGUI_THEME_SURFACE_VARIANT;
    local->selected_bg_color = EGUI_THEME_PRIMARY;
    local->text_color = EGUI_THEME_TEXT_SECONDARY;
    local->selected_text_color = EGUI_COLOR_WHITE;
    local->border_color = EGUI_THEME_BORDER;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->icon_font = NULL;
    local->icon_text_gap = 1;
    local->on_segment_changed = NULL;

    egui_view_set_view_name(self, "egui_view_segmented_control");
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif
}

void egui_view_segmented_control_apply_params(egui_view_t *self, const egui_view_segmented_control_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_segmented_control_t);
    self->region = params->region;
    local->segment_texts = params->segment_texts;
    local->segment_icons = params->segment_icons;
    local->segment_count = egui_view_segmented_control_clamp_count(params->segment_count);
    if (local->segment_count == 0)
    {
        local->current_index = 0;
    }
    else if (local->current_index >= local->segment_count)
    {
        local->current_index = local->segment_count - 1;
    }
    local->pressed_index = EGUI_VIEW_SEGMENTED_CONTROL_PRESSED_NONE;
    egui_view_invalidate(self);
}

void egui_view_segmented_control_init_with_params(egui_view_t *self, const egui_view_segmented_control_params_t *params)
{
    egui_view_segmented_control_init(self);
    egui_view_segmented_control_apply_params(self, params);
}
