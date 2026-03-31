#include <stdio.h>
#include <string.h>

#include "egui_view_textinput.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"
#include "core/egui_api.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

#define EGUI_TEXTINPUT_CURSOR_WIDTH 1

/**
 * Get the pixel width of text from index 0 to pos.
 */
static egui_dim_t egui_view_textinput_get_text_width_to_pos(egui_view_textinput_t *local, uint8_t pos)
{
    if (local->font == NULL || pos == 0)
    {
        return 0;
    }

    // Create a temporary substring
    char tmp[EGUI_CONFIG_TEXTINPUT_MAX_LENGTH + 1];
    uint8_t len = pos;
    if (len > local->text_len)
    {
        len = local->text_len;
    }
    egui_api_memcpy(tmp, local->text, len);
    tmp[len] = '\0';

    egui_dim_t width = 0;
    egui_dim_t height = 0;
    local->font->api->get_str_size(local->font, tmp, 0, 0, &width, &height);

    return width;
}

/**
 * Update scroll offset to ensure cursor is visible in the work region.
 */
static void egui_view_textinput_update_scroll(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);

    egui_dim_t cursor_x = egui_view_textinput_get_text_width_to_pos(local, local->cursor_pos);
    egui_dim_t work_width = self->region.size.width - (self->padding.left + self->padding.right);

    // Ensure cursor is visible
    if (cursor_x - local->scroll_offset_x > work_width - EGUI_TEXTINPUT_CURSOR_WIDTH)
    {
        local->scroll_offset_x = cursor_x - work_width + EGUI_TEXTINPUT_CURSOR_WIDTH;
    }
    if (cursor_x < local->scroll_offset_x)
    {
        local->scroll_offset_x = cursor_x;
    }
    if (local->scroll_offset_x < 0)
    {
        local->scroll_offset_x = 0;
    }
}

static int egui_view_textinput_get_cursor_region(egui_view_t *self, egui_view_textinput_t *local, egui_region_t *cursor_region)
{
    egui_region_t work_region;
    egui_dim_t cursor_x;
    egui_dim_t dummy_width = 0;
    egui_dim_t cursor_height = 0;

    if (local->font == NULL || cursor_region == NULL)
    {
        return 0;
    }

    egui_view_get_work_region(self, &work_region);
    cursor_x = work_region.location.x + egui_view_textinput_get_text_width_to_pos(local, local->cursor_pos) - local->scroll_offset_x;
    local->font->api->get_str_size(local->font, "A", 0, 0, &dummy_width, &cursor_height);

    cursor_region->location.x = cursor_x;
    cursor_region->location.y = work_region.location.y + (work_region.size.height - cursor_height) / 2;
    cursor_region->size.width = EGUI_TEXTINPUT_CURSOR_WIDTH;
    cursor_region->size.height = cursor_height;

    return !egui_region_is_empty(cursor_region);
}

static void egui_view_textinput_local_region_to_screen(egui_view_t *self, const egui_region_t *local_region, egui_region_t *screen_region)
{
    screen_region->location.x = self->region_screen.location.x + local_region->location.x;
    screen_region->location.y = self->region_screen.location.y + local_region->location.y;
    screen_region->size.width = local_region->size.width;
    screen_region->size.height = local_region->size.height;
}

static void egui_view_textinput_invalidate_cursor_region(egui_view_t *self, egui_view_textinput_t *local)
{
    egui_region_t cursor_region;

    if (!egui_view_textinput_get_cursor_region(self, local, &cursor_region))
    {
        egui_view_invalidate(self);
        return;
    }

    egui_view_invalidate_region(self, &cursor_region);
}

static void egui_view_textinput_cursor_timer_callback(egui_timer_t *timer)
{
    egui_view_textinput_t *local = (egui_view_textinput_t *)timer->user_data;
    egui_view_t *self = (egui_view_t *)local;

    local->cursor_visible = !local->cursor_visible;
    egui_view_textinput_invalidate_cursor_region(self, local);

    // Restart timer for next blink
    egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void egui_view_textinput_on_focus_change(egui_view_t *self, int is_focused)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);

    if (is_focused)
    {
        // Start cursor blinking
        local->cursor_visible = 1;
        egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);
    }
    else
    {
        // Stop cursor blinking
        local->cursor_visible = 0;
        egui_timer_stop_timer(&local->cursor_timer);
    }
    egui_view_invalidate(self);
}
#endif

void egui_view_textinput_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);

    if (text == NULL)
    {
        local->text[0] = '\0';
        local->text_len = 0;
    }
    else
    {
        uint8_t len = strlen(text);
        if (len > local->max_length)
        {
            len = local->max_length;
        }
        egui_api_memcpy(local->text, text, len);
        local->text[len] = '\0';
        local->text_len = len;
    }
    local->cursor_pos = local->text_len;
    local->scroll_offset_x = 0;
    egui_view_textinput_update_scroll(self);
    egui_view_invalidate(self);

    if (local->on_text_changed)
    {
        local->on_text_changed(self, local->text);
    }
}

const char *egui_view_textinput_get_text(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    return local->text;
}

void egui_view_textinput_clear(egui_view_t *self)
{
    egui_view_textinput_set_text(self, NULL);
}

void egui_view_textinput_insert_char(egui_view_t *self, char c)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);

    if (local->text_len >= local->max_length || c == 0)
    {
        return;
    }

    // Shift characters right from cursor_pos
    for (int i = local->text_len; i > local->cursor_pos; i--)
    {
        local->text[i] = local->text[i - 1];
    }

    local->text[local->cursor_pos] = c;
    local->cursor_pos++;
    local->text_len++;
    local->text[local->text_len] = '\0';

    // Reset cursor blink
    local->cursor_visible = 1;
    egui_timer_stop_timer(&local->cursor_timer);
    egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);

    egui_view_textinput_update_scroll(self);
    egui_view_invalidate(self);

    if (local->on_text_changed)
    {
        local->on_text_changed(self, local->text);
    }
}

void egui_view_textinput_delete_char(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);

    if (local->cursor_pos == 0)
    {
        return;
    }

    // Shift characters left
    for (int i = local->cursor_pos - 1; i < local->text_len - 1; i++)
    {
        local->text[i] = local->text[i + 1];
    }

    local->cursor_pos--;
    local->text_len--;
    local->text[local->text_len] = '\0';

    // Reset cursor blink
    local->cursor_visible = 1;
    egui_timer_stop_timer(&local->cursor_timer);
    egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);

    egui_view_textinput_update_scroll(self);
    egui_view_invalidate(self);

    if (local->on_text_changed)
    {
        local->on_text_changed(self, local->text);
    }
}

void egui_view_textinput_delete_forward(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);

    if (local->cursor_pos >= local->text_len)
    {
        return;
    }

    // Shift characters left
    for (int i = local->cursor_pos; i < local->text_len - 1; i++)
    {
        local->text[i] = local->text[i + 1];
    }

    local->text_len--;
    local->text[local->text_len] = '\0';

    egui_view_textinput_update_scroll(self);
    egui_view_invalidate(self);

    if (local->on_text_changed)
    {
        local->on_text_changed(self, local->text);
    }
}

void egui_view_textinput_set_cursor_pos(egui_view_t *self, uint8_t pos)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);

    if (pos > local->text_len)
    {
        pos = local->text_len;
    }
    if (pos == local->cursor_pos)
    {
        return;
    }

    egui_view_textinput_invalidate_cursor_region(self, local);
    local->cursor_pos = pos;

    // Reset cursor blink
    local->cursor_visible = 1;
    egui_timer_stop_timer(&local->cursor_timer);
    egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);

    egui_view_textinput_update_scroll(self);
    egui_view_textinput_invalidate_cursor_region(self, local);
}

void egui_view_textinput_move_cursor_left(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    if (local->cursor_pos > 0)
    {
        egui_view_textinput_set_cursor_pos(self, local->cursor_pos - 1);
    }
}

void egui_view_textinput_move_cursor_right(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    if (local->cursor_pos < local->text_len)
    {
        egui_view_textinput_set_cursor_pos(self, local->cursor_pos + 1);
    }
}

void egui_view_textinput_move_cursor_home(egui_view_t *self)
{
    egui_view_textinput_set_cursor_pos(self, 0);
}

void egui_view_textinput_move_cursor_end(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    egui_view_textinput_set_cursor_pos(self, local->text_len);
}

void egui_view_textinput_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    if (local->font == font)
    {
        return;
    }
    local->font = font;
    egui_view_textinput_update_scroll(self);
    egui_view_invalidate(self);
}

void egui_view_textinput_set_text_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    local->text_color = color;
    local->text_alpha = alpha;
    egui_view_invalidate(self);
}

void egui_view_textinput_set_placeholder(egui_view_t *self, const char *placeholder)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    local->placeholder = placeholder;
    egui_view_invalidate(self);
}

void egui_view_textinput_set_placeholder_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    local->placeholder_color = color;
    local->placeholder_alpha = alpha;
    egui_view_invalidate(self);
}

void egui_view_textinput_set_cursor_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    local->cursor_color = color;
    egui_view_invalidate(self);
}

void egui_view_textinput_set_max_length(egui_view_t *self, uint8_t max_length)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    if (max_length > EGUI_CONFIG_TEXTINPUT_MAX_LENGTH)
    {
        max_length = EGUI_CONFIG_TEXTINPUT_MAX_LENGTH;
    }
    local->max_length = max_length;

    // Truncate if needed
    if (local->text_len > max_length)
    {
        local->text_len = max_length;
        local->text[local->text_len] = '\0';
        if (local->cursor_pos > local->text_len)
        {
            local->cursor_pos = local->text_len;
        }
        egui_view_textinput_update_scroll(self);
        egui_view_invalidate(self);
    }
}

void egui_view_textinput_set_on_text_changed(egui_view_t *self, egui_view_textinput_on_text_changed_t listener)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    local->on_text_changed = listener;
}

void egui_view_textinput_set_on_submit(egui_view_t *self, egui_view_textinput_on_submit_t listener)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);
    local->on_submit = listener;
}

void egui_view_textinput_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);

    if (local->font == NULL)
    {
        return;
    }

    egui_region_t region = self->region_screen;
    egui_dim_t radius = EGUI_THEME_RADIUS_MD;
    if (radius > region.size.height / 2)
    {
        radius = region.size.height / 2;
    }

    // Draw input container first.
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(EGUI_THEME_SURFACE, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = EGUI_THEME_SURFACE},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = stops,
        };
        egui_canvas_draw_round_rectangle_fill_gradient(region.location.x, region.location.y, region.size.width, region.size.height, radius, &grad);
    }
#else
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, radius, EGUI_THEME_SURFACE,
                                          EGUI_ALPHA_100);
#endif
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, radius, EGUI_THEME_STROKE_WIDTH,
                                     self->is_focused ? EGUI_THEME_FOCUS : EGUI_THEME_BORDER, EGUI_ALPHA_100);

    egui_region_t work_region;
    egui_view_get_work_region(self, &work_region);

    egui_region_t text_screen_region;
    int text_active;

    egui_view_textinput_local_region_to_screen(self, &work_region, &text_screen_region);
    text_active = egui_canvas_is_region_active(&text_screen_region);

    // Draw text or placeholder
    if (text_active && local->text_len == 0 && !self->is_focused && local->placeholder != NULL)
    {
        // Draw placeholder
        egui_canvas_draw_text_in_rect(local->font, local->placeholder, &work_region, local->align_type, local->placeholder_color, local->placeholder_alpha);
    }
    else if (text_active && local->text_len > 0)
    {
        // Draw text with scroll offset
        egui_dim_t text_x = work_region.location.x - local->scroll_offset_x;
        egui_dim_t text_y = work_region.location.y;

        // Get text height for vertical centering
        egui_dim_t text_width = 0;
        egui_dim_t text_height = 0;
        local->font->api->get_str_size(local->font, local->text, 0, 0, &text_width, &text_height);

        // Vertical center
        text_y += (work_region.size.height - text_height) / 2;

        egui_canvas_draw_text(local->font, local->text, text_x, text_y, local->text_color, local->text_alpha);
    }

    // Draw cursor
    if (self->is_focused && local->cursor_visible)
    {
        egui_region_t cursor_region;
        egui_region_t cursor_screen_region;

        if (egui_view_textinput_get_cursor_region(self, local, &cursor_region))
        {
            egui_view_textinput_local_region_to_screen(self, &cursor_region, &cursor_screen_region);
            if (egui_canvas_is_region_active(&cursor_screen_region))
            {
                egui_canvas_draw_rectangle_fill(cursor_region.location.x, cursor_region.location.y, cursor_region.size.width, cursor_region.size.height,
                                                local->cursor_color, EGUI_ALPHA_100);
            }
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_textinput_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);

    if (!self->is_enable)
    {
        return 0;
    }

    // Only process key up events for actions (and key down for printable chars)
    if (event->type == EGUI_KEY_EVENT_ACTION_UP)
    {
        switch (event->key_code)
        {
        case EGUI_KEY_CODE_BACKSPACE:
            egui_view_textinput_delete_char(self);
            return 1;
        case EGUI_KEY_CODE_DELETE:
            egui_view_textinput_delete_forward(self);
            return 1;
        case EGUI_KEY_CODE_LEFT:
            egui_view_textinput_move_cursor_left(self);
            return 1;
        case EGUI_KEY_CODE_RIGHT:
            egui_view_textinput_move_cursor_right(self);
            return 1;
        case EGUI_KEY_CODE_HOME:
            egui_view_textinput_move_cursor_home(self);
            return 1;
        case EGUI_KEY_CODE_END:
            egui_view_textinput_move_cursor_end(self);
            return 1;
        case EGUI_KEY_CODE_ENTER:
            if (local->on_submit)
            {
                local->on_submit(self, local->text);
            }
            return 1;
        default:
        {
            // Try to convert to printable character
            char c = egui_key_event_to_char(event);
            if (c != 0)
            {
                egui_view_textinput_insert_char(self, c);
                return 1;
            }
            break;
        }
        }
    }
    else if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
    {
        // Consume DOWN events for keys we handle on UP to prevent propagation
        switch (event->key_code)
        {
        case EGUI_KEY_CODE_BACKSPACE:
        case EGUI_KEY_CODE_DELETE:
        case EGUI_KEY_CODE_LEFT:
        case EGUI_KEY_CODE_RIGHT:
        case EGUI_KEY_CODE_HOME:
        case EGUI_KEY_CODE_END:
        case EGUI_KEY_CODE_ENTER:
            return 1;
        default:
        {
            char c = egui_key_event_to_char(event);
            if (c != 0)
            {
                return 1; // consume printable key down
            }
            break;
        }
        }
    }

    return 0;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_textinput_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_textinput_t);

    if (!self->is_enable)
    {
        return 0;
    }

    if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        // Calculate cursor position from touch x
        egui_dim_t touch_x = event->location.x - self->region_screen.location.x - self->padding.left + local->scroll_offset_x;

        if (local->font != NULL && local->text_len > 0)
        {
            uint8_t best_pos = 0;
            egui_dim_t best_dist = 0x7FFF;

            for (uint8_t i = 0; i <= local->text_len; i++)
            {
                egui_dim_t char_x = egui_view_textinput_get_text_width_to_pos(local, i);
                egui_dim_t dist = touch_x > char_x ? (touch_x - char_x) : (char_x - touch_x);
                if (dist < best_dist)
                {
                    best_dist = dist;
                    best_pos = i;
                }
            }
            egui_view_textinput_set_cursor_pos(self, best_pos);
        }

        // Request focus on touch
        egui_view_request_focus(self);
        return 1;
    }
    else if (event->type == EGUI_MOTION_EVENT_ACTION_UP)
    {
        return 1;
    }

    return 1; // consume all touch events
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_textinput_t) = {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_textinput_on_touch_event,
#else
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_textinput_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_textinput_on_key_event,
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        .on_focus_changed = egui_view_textinput_on_focus_change,
#endif
};

void egui_view_textinput_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_textinput_t);

    // call super init
    egui_view_init(self);
    // update api
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_textinput_t);

    // init local data
    local->text[0] = '\0';
    local->text_len = 0;
    local->cursor_pos = 0;
    local->max_length = EGUI_CONFIG_TEXTINPUT_MAX_LENGTH;

    local->cursor_visible = 0;
    local->reserved = 0;

    // init timer
    egui_timer_init_timer(&local->cursor_timer, (void *)local, egui_view_textinput_cursor_timer_callback);

    local->font = NULL;
    local->text_color = EGUI_THEME_TEXT_PRIMARY;
    local->text_alpha = EGUI_ALPHA_100;
    local->placeholder_color = EGUI_THEME_TEXT_SECONDARY;
    local->placeholder_alpha = EGUI_ALPHA_100;
    local->cursor_color = EGUI_THEME_PRIMARY;
    local->align_type = EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER;

    local->placeholder = NULL;
    local->scroll_offset_x = 0;

    local->on_text_changed = NULL;
    local->on_submit = NULL;

    // Make textinput focusable and clickable
    self->is_clickable = true;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    self->is_focusable = true;
#endif

    egui_view_set_view_name(self, "egui_view_textinput");
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
