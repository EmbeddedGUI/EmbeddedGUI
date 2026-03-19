#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_textblock.h"
#include "core/egui_canvas_gradient.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#include "core/egui_api.h"
#endif

#define EGUI_TEXTBLOCK_CURSOR_WIDTH  1
#define EGUI_TEXTBLOCK_BORDER_STROKE 1

static void egui_view_textblock_update_content_size(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);

    const char *measure_text;
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    measure_text = (local->is_editable) ? local->edit_buf : local->text;
#else
    measure_text = local->text;
#endif

    if (local->font == NULL || measure_text == NULL || measure_text[0] == '\0')
    {
        local->content_width = 0;
        local->content_height = 0;
        return;
    }
    egui_dim_t w = 0, h = 0;
    local->font->api->get_str_size(local->font, measure_text, 1, local->line_space, &w, &h);
    local->content_width = w;
    local->content_height = h;
}

static void egui_view_textblock_clamp_scroll(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    egui_region_t work_region;
    egui_view_get_work_region(self, &work_region);

    egui_dim_t max_scroll_x = local->content_width - work_region.size.width;
    egui_dim_t max_scroll_y = local->content_height - work_region.size.height;

    if (max_scroll_x < 0)
    {
        max_scroll_x = 0;
    }
    if (max_scroll_y < 0)
    {
        max_scroll_y = 0;
    }

    if (local->scroll_offset_x < 0)
    {
        local->scroll_offset_x = 0;
    }
    if (local->scroll_offset_x > max_scroll_x)
    {
        local->scroll_offset_x = max_scroll_x;
    }
    if (local->scroll_offset_y < 0)
    {
        local->scroll_offset_y = 0;
    }
    if (local->scroll_offset_y > max_scroll_y)
    {
        local->scroll_offset_y = max_scroll_y;
    }
}

// ========================= Edit mode helpers =========================
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

/**
 * Get the pixel width of text from index 0 to pos (multi-line aware).
 * Returns the x offset of the cursor on its current line.
 */
static egui_dim_t egui_view_textblock_get_cursor_x(egui_view_textblock_t *local, uint16_t pos)
{
    if (local->font == NULL || pos == 0)
    {
        return 0;
    }

    // Find the start of the line containing pos
    const char *text = local->edit_buf;
    uint16_t line_start = 0;
    for (uint16_t i = 0; i < pos; i++)
    {
        if (text[i] == '\n')
        {
            line_start = i + 1;
        }
    }

    // Measure width from line_start to pos
    char tmp[EGUI_CONFIG_TEXTBLOCK_EDIT_MAX_LENGTH + 1];
    uint16_t len = pos - line_start;
    memcpy(tmp, &text[line_start], len);
    tmp[len] = '\0';

    egui_dim_t width = 0, height = 0;
    local->font->api->get_str_size(local->font, tmp, 0, 0, &width, &height);
    return width;
}

/**
 * Get the y offset and line height for the line containing cursor at pos.
 */
static void egui_view_textblock_get_cursor_y(egui_view_textblock_t *local, uint16_t pos, egui_dim_t *out_y, egui_dim_t *out_h)
{
    if (local->font == NULL)
    {
        *out_y = 0;
        *out_h = 0;
        return;
    }

    // Get single-line height
    egui_dim_t dummy_w = 0, line_h = 0;
    local->font->api->get_str_size(local->font, "A", 0, 0, &dummy_w, &line_h);
    *out_h = line_h;

    // Count lines before pos
    const char *text = local->edit_buf;
    int line_idx = 0;
    for (uint16_t i = 0; i < pos; i++)
    {
        if (text[i] == '\n')
        {
            line_idx++;
        }
    }

    *out_y = line_idx * (line_h + local->line_space);
}

/**
 * Ensure cursor is visible by adjusting scroll offsets.
 */
static void egui_view_textblock_ensure_cursor_visible(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);

    egui_region_t work_region;
    egui_view_get_work_region(self, &work_region);

    egui_dim_t cursor_x = egui_view_textblock_get_cursor_x(local, local->cursor_pos);
    egui_dim_t cursor_y = 0, cursor_h = 0;
    egui_view_textblock_get_cursor_y(local, local->cursor_pos, &cursor_y, &cursor_h);

    // Horizontal
    if (cursor_x - local->scroll_offset_x > work_region.size.width - EGUI_TEXTBLOCK_CURSOR_WIDTH)
    {
        local->scroll_offset_x = cursor_x - work_region.size.width + EGUI_TEXTBLOCK_CURSOR_WIDTH;
    }
    if (cursor_x < local->scroll_offset_x)
    {
        local->scroll_offset_x = cursor_x;
    }

    // Vertical
    if (cursor_y + cursor_h - local->scroll_offset_y > work_region.size.height)
    {
        local->scroll_offset_y = cursor_y + cursor_h - work_region.size.height;
    }
    if (cursor_y < local->scroll_offset_y)
    {
        local->scroll_offset_y = cursor_y;
    }

    egui_view_textblock_clamp_scroll(self);
}

static void egui_view_textblock_cursor_timer_callback(egui_timer_t *timer)
{
    egui_view_textblock_t *local = (egui_view_textblock_t *)timer->user_data;
    egui_view_t *self = (egui_view_t *)local;

    local->cursor_visible = !local->cursor_visible;
    egui_view_invalidate(self);

    egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTBLOCK_CURSOR_BLINK_MS, 0);
}

static void egui_view_textblock_on_focus_change(egui_view_t *self, int is_focused)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);

    if (!local->is_editable)
    {
        return;
    }

    if (is_focused)
    {
        local->cursor_visible = 1;
        egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTBLOCK_CURSOR_BLINK_MS, 0);
    }
    else
    {
        local->cursor_visible = 0;
        egui_timer_stop_timer(&local->cursor_timer);
    }
    egui_view_invalidate(self);
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

// ========================= Draw =========================

void egui_view_textblock_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);

    if (local->font == NULL)
    {
        return;
    }

    // --- Draw rounded border ---
    if (local->is_border_enabled)
    {
        egui_dim_t radius = local->border_radius;
        if (radius > self->region.size.height / 2)
        {
            radius = self->region.size.height / 2;
        }
        egui_canvas_draw_round_rectangle(0, 0, self->region.size.width, self->region.size.height, radius, EGUI_TEXTBLOCK_BORDER_STROKE,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
                                         (local->is_editable && self->is_focused) ? EGUI_THEME_FOCUS : local->border_color,
#else
                                         local->border_color,
#endif
                                         EGUI_ALPHA_100);
    }

    // --- Draw text content ---
    egui_region_t work_region;
    egui_view_get_work_region(self, &work_region);

    // Determine the text pointer to use
    const char *draw_text;
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    draw_text = (local->is_editable) ? local->edit_buf : local->text;
#else
    draw_text = local->text;
#endif

    if (draw_text == NULL || draw_text[0] == '\0')
    {
        goto draw_cursor;
    }

    {
        // Build a virtual content rect adjusted for scroll offset.
        // The canvas work region (already set to view screen region) will clip overflow.
        egui_region_t draw_rect;
        draw_rect.location.x = self->padding.left - local->scroll_offset_x;
        draw_rect.location.y = self->padding.top - local->scroll_offset_y;
        draw_rect.size.width = EGUI_MAX(local->content_width, work_region.size.width);
        draw_rect.size.height = EGUI_MAX(local->content_height, work_region.size.height);

        // Use LEFT|TOP alignment when scrolling on that axis, otherwise keep user alignment
        uint8_t draw_align = local->align_type;
        if (local->content_width > work_region.size.width)
        {
            draw_align = (draw_align & ~EGUI_ALIGN_HMASK) | EGUI_ALIGN_LEFT;
        }
        if (local->content_height > work_region.size.height)
        {
            draw_align = (draw_align & ~EGUI_ALIGN_VMASK) | EGUI_ALIGN_TOP;
        }

        egui_canvas_draw_text_in_rect_with_line_space(local->font, draw_text, &draw_rect, draw_align, local->line_space, local->color, local->alpha);
    }

draw_cursor:
    // --- Draw cursor (edit mode) ---
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (local->is_editable && self->is_focused && local->cursor_visible)
    {
        egui_dim_t cx = egui_view_textblock_get_cursor_x(local, local->cursor_pos);
        egui_dim_t cy = 0, ch = 0;
        egui_view_textblock_get_cursor_y(local, local->cursor_pos, &cy, &ch);

        egui_dim_t draw_cx = self->padding.left + cx - local->scroll_offset_x;
        egui_dim_t draw_cy = self->padding.top + cy - local->scroll_offset_y;

        egui_canvas_draw_rectangle_fill(draw_cx, draw_cy, EGUI_TEXTBLOCK_CURSOR_WIDTH, ch, local->cursor_color, EGUI_ALPHA_100);
    }
#else
    (void)0; // avoid empty label warning
#endif

    // --- Draw scrollbars ---
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    if (local->is_scrollbar_enabled)
    {
        egui_dim_t view_w = work_region.size.width;
        egui_dim_t view_h = work_region.size.height;
        egui_dim_t margin = EGUI_THEME_SCROLLBAR_MARGIN;

        // Vertical scrollbar
        if (local->content_height > view_h && view_h > 0 && local->content_height > 0)
        {
            egui_dim_t track_length = self->region.size.height - 2 * margin;
            if (track_length > 0)
            {
                egui_dim_t thumb_length = (egui_dim_t)(((int32_t)track_length * view_h) / local->content_height);
                if (thumb_length < EGUI_THEME_SCROLLBAR_MIN_LENGTH)
                {
                    thumb_length = EGUI_THEME_SCROLLBAR_MIN_LENGTH;
                }
                if (thumb_length > track_length)
                {
                    thumb_length = track_length;
                }

                egui_dim_t thumb_travel = track_length - thumb_length;
                egui_dim_t max_scroll = local->content_height - view_h;
                egui_dim_t thumb_y = 0;
                if (max_scroll > 0 && thumb_travel > 0)
                {
                    thumb_y = (egui_dim_t)(((int32_t)local->scroll_offset_y * thumb_travel) / max_scroll);
                    if (thumb_y > thumb_travel)
                    {
                        thumb_y = thumb_travel;
                    }
                }

                egui_dim_t bar_x = self->region.size.width - EGUI_THEME_SCROLLBAR_THICKNESS - margin;
                egui_dim_t bar_y = margin + thumb_y;

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
                {
                    egui_color_t color_light = egui_rgb_mix(EGUI_THEME_SCROLLBAR_COLOR, EGUI_COLOR_WHITE, 80);
                    egui_gradient_stop_t stops[2] = {
                            {.position = 0, .color = color_light},
                            {.position = 255, .color = EGUI_THEME_SCROLLBAR_COLOR},
                    };
                    egui_gradient_t grad = {
                            .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                            .stop_count = 2,
                            .alpha = EGUI_THEME_SCROLLBAR_ALPHA,
                            .stops = stops,
                    };
                    egui_canvas_draw_round_rectangle_fill_gradient(bar_x, bar_y, EGUI_THEME_SCROLLBAR_THICKNESS, thumb_length, EGUI_THEME_SCROLLBAR_RADIUS,
                                                                   &grad);
                }
#else
                egui_canvas_draw_round_rectangle_fill(bar_x, bar_y, EGUI_THEME_SCROLLBAR_THICKNESS, thumb_length, EGUI_THEME_SCROLLBAR_RADIUS,
                                                      EGUI_THEME_SCROLLBAR_COLOR, EGUI_THEME_SCROLLBAR_ALPHA);
#endif
            }
        }

        // Horizontal scrollbar
        if (local->content_width > view_w && view_w > 0 && local->content_width > 0)
        {
            egui_dim_t track_length = self->region.size.width - 2 * margin;
            if (track_length > 0)
            {
                egui_dim_t thumb_length = (egui_dim_t)(((int32_t)track_length * view_w) / local->content_width);
                if (thumb_length < EGUI_THEME_SCROLLBAR_MIN_LENGTH)
                {
                    thumb_length = EGUI_THEME_SCROLLBAR_MIN_LENGTH;
                }
                if (thumb_length > track_length)
                {
                    thumb_length = track_length;
                }

                egui_dim_t thumb_travel = track_length - thumb_length;
                egui_dim_t max_scroll = local->content_width - view_w;
                egui_dim_t thumb_x = 0;
                if (max_scroll > 0 && thumb_travel > 0)
                {
                    thumb_x = (egui_dim_t)(((int32_t)local->scroll_offset_x * thumb_travel) / max_scroll);
                    if (thumb_x > thumb_travel)
                    {
                        thumb_x = thumb_travel;
                    }
                }

                egui_dim_t bar_x = margin + thumb_x;
                egui_dim_t bar_y = self->region.size.height - EGUI_THEME_SCROLLBAR_THICKNESS - margin;

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
                {
                    egui_color_t color_light = egui_rgb_mix(EGUI_THEME_SCROLLBAR_COLOR, EGUI_COLOR_WHITE, 80);
                    egui_gradient_stop_t stops[2] = {
                            {.position = 0, .color = color_light},
                            {.position = 255, .color = EGUI_THEME_SCROLLBAR_COLOR},
                    };
                    egui_gradient_t grad = {
                            .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                            .stop_count = 2,
                            .alpha = EGUI_THEME_SCROLLBAR_ALPHA,
                            .stops = stops,
                    };
                    egui_canvas_draw_round_rectangle_fill_gradient(bar_x, bar_y, thumb_length, EGUI_THEME_SCROLLBAR_THICKNESS, EGUI_THEME_SCROLLBAR_RADIUS,
                                                                   &grad);
                }
#else
                egui_canvas_draw_round_rectangle_fill(bar_x, bar_y, thumb_length, EGUI_THEME_SCROLLBAR_THICKNESS, EGUI_THEME_SCROLLBAR_RADIUS,
                                                      EGUI_THEME_SCROLLBAR_COLOR, EGUI_THEME_SCROLLBAR_ALPHA);
#endif
            }
        }
    }
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
}

// ========================= Touch handling =========================

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
/**
 * Handle scrollbar drag: map touch position to scroll offset.
 * Returns 1 if the touch was on a scrollbar and handled.
 */
static int egui_view_textblock_handle_scrollbar_drag(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);

    if (!local->is_scrollbar_enabled)
    {
        return 0;
    }

    egui_region_t work_region;
    egui_view_get_work_region(self, &work_region);
    egui_dim_t local_x = event->location.x - self->region_screen.location.x;
    egui_dim_t local_y = event->location.y - self->region_screen.location.y;
    egui_dim_t margin = EGUI_THEME_SCROLLBAR_MARGIN;

    if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        // Check vertical scrollbar area (right edge)
        if (local->content_height > work_region.size.height)
        {
            egui_dim_t sb_area_start = self->region.size.width - EGUI_THEME_SCROLLBAR_TOUCH_WIDTH;
            if (local_x >= sb_area_start)
            {
                local->is_scrollbar_v_dragging = 1;
                local->is_scrollbar_h_dragging = 0;
                // Fall through to MOVE handling below
            }
        }

        // Check horizontal scrollbar area (bottom edge)
        if (!local->is_scrollbar_v_dragging && local->content_width > work_region.size.width)
        {
            egui_dim_t sb_area_start = self->region.size.height - EGUI_THEME_SCROLLBAR_TOUCH_WIDTH;
            if (local_y >= sb_area_start)
            {
                local->is_scrollbar_h_dragging = 1;
                local->is_scrollbar_v_dragging = 0;
                // Fall through to MOVE handling below
            }
        }
    }

    // Handle vertical scrollbar dragging
    if (local->is_scrollbar_v_dragging)
    {
        if (event->type == EGUI_MOTION_EVENT_ACTION_UP || event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
        {
            local->is_scrollbar_v_dragging = 0;
            return 1;
        }

        egui_dim_t view_h = work_region.size.height;
        egui_dim_t content_h = local->content_height;
        egui_dim_t track_length = self->region.size.height - 2 * margin;
        if (track_length > 0 && content_h > view_h)
        {
            egui_dim_t thumb_length = (egui_dim_t)(((int32_t)track_length * view_h) / content_h);
            if (thumb_length < EGUI_THEME_SCROLLBAR_MIN_LENGTH)
                thumb_length = EGUI_THEME_SCROLLBAR_MIN_LENGTH;
            if (thumb_length > track_length)
                thumb_length = track_length;

            egui_dim_t thumb_travel = track_length - thumb_length;
            if (thumb_travel > 0)
            {
                egui_dim_t thumb_pos = local_y - margin - thumb_length / 2;
                if (thumb_pos < 0)
                    thumb_pos = 0;
                if (thumb_pos > thumb_travel)
                    thumb_pos = thumb_travel;

                egui_dim_t max_scroll = content_h - view_h;
                local->scroll_offset_y = (egui_dim_t)(((int32_t)thumb_pos * max_scroll) / thumb_travel);
                egui_view_textblock_clamp_scroll(self);
                egui_view_invalidate(self);
            }
        }
        return 1;
    }

    // Handle horizontal scrollbar dragging
    if (local->is_scrollbar_h_dragging)
    {
        if (event->type == EGUI_MOTION_EVENT_ACTION_UP || event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
        {
            local->is_scrollbar_h_dragging = 0;
            return 1;
        }

        egui_dim_t view_w = work_region.size.width;
        egui_dim_t content_w = local->content_width;
        egui_dim_t track_length = self->region.size.width - 2 * margin;
        if (track_length > 0 && content_w > view_w)
        {
            egui_dim_t thumb_length = (egui_dim_t)(((int32_t)track_length * view_w) / content_w);
            if (thumb_length < EGUI_THEME_SCROLLBAR_MIN_LENGTH)
                thumb_length = EGUI_THEME_SCROLLBAR_MIN_LENGTH;
            if (thumb_length > track_length)
                thumb_length = track_length;

            egui_dim_t thumb_travel = track_length - thumb_length;
            if (thumb_travel > 0)
            {
                egui_dim_t thumb_pos = local_x - margin - thumb_length / 2;
                if (thumb_pos < 0)
                    thumb_pos = 0;
                if (thumb_pos > thumb_travel)
                    thumb_pos = thumb_travel;

                egui_dim_t max_scroll = content_w - view_w;
                local->scroll_offset_x = (egui_dim_t)(((int32_t)thumb_pos * max_scroll) / thumb_travel);
                egui_view_textblock_clamp_scroll(self);
                egui_view_invalidate(self);
            }
        }
        return 1;
    }

    return 0;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR

static int egui_view_textblock_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);

    if (!self->is_enable)
    {
        return 0;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    // Scrollbar drag takes priority
    if (egui_view_textblock_handle_scrollbar_drag(self, event))
    {
        return 1;
    }
#endif

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        local->touch_last_x = event->location.x;
        local->touch_last_y = event->location.y;
        local->is_touch_dragging = 0;

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (local->is_editable)
        {
            egui_view_request_focus(self);
        }
#endif
        return 1;

    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        egui_dim_t dx = event->location.x - local->touch_last_x;
        egui_dim_t dy = event->location.y - local->touch_last_y;

        if (!local->is_touch_dragging)
        {
            if (EGUI_ABS(dx) > 3 || EGUI_ABS(dy) > 3)
            {
                local->is_touch_dragging = 1;
            }
        }

        if (local->is_touch_dragging)
        {
            local->scroll_offset_x -= dx;
            local->scroll_offset_y -= dy;
            egui_view_textblock_clamp_scroll(self);
            egui_view_invalidate(self);

            local->touch_last_x = event->location.x;
            local->touch_last_y = event->location.y;
        }
        return 1;
    }

    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->is_touch_dragging = 0;
        return 1;

    default:
        break;
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

// ========================= Key handling (edit mode) =========================

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static int egui_view_textblock_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);

    if (!self->is_enable || !local->is_editable)
    {
        return 0;
    }

    if (event->type == EGUI_KEY_EVENT_ACTION_UP)
    {
        switch (event->key_code)
        {
        case EGUI_KEY_CODE_BACKSPACE:
            egui_view_textblock_delete_char(self);
            return 1;
        case EGUI_KEY_CODE_LEFT:
            if (local->cursor_pos > 0)
            {
                egui_view_textblock_set_cursor_pos(self, local->cursor_pos - 1);
            }
            return 1;
        case EGUI_KEY_CODE_RIGHT:
            if (local->cursor_pos < local->text_len)
            {
                egui_view_textblock_set_cursor_pos(self, local->cursor_pos + 1);
            }
            return 1;
        case EGUI_KEY_CODE_UP:
        {
            // Move cursor up one line
            egui_dim_t cx = egui_view_textblock_get_cursor_x(local, local->cursor_pos);
            // Find start of current line
            uint16_t cur = local->cursor_pos;
            while (cur > 0 && local->edit_buf[cur - 1] != '\n')
            {
                cur--;
            }
            if (cur == 0)
            {
                return 1; // already on first line
            }
            // Find start of previous line
            uint16_t prev_end = cur - 1; // the '\n' before current line
            uint16_t prev_start = prev_end;
            while (prev_start > 0 && local->edit_buf[prev_start - 1] != '\n')
            {
                prev_start--;
            }
            // Find best position on previous line matching cx
            uint16_t best = prev_start;
            for (uint16_t i = prev_start; i <= prev_end; i++)
            {
                char tmp[EGUI_CONFIG_TEXTBLOCK_EDIT_MAX_LENGTH + 1];
                uint16_t len = i - prev_start;
                memcpy(tmp, &local->edit_buf[prev_start], len);
                tmp[len] = '\0';
                egui_dim_t w = 0, h = 0;
                local->font->api->get_str_size(local->font, tmp, 0, 0, &w, &h);
                if (w <= cx)
                {
                    best = i;
                }
                else
                {
                    break;
                }
            }
            egui_view_textblock_set_cursor_pos(self, best);
            return 1;
        }
        case EGUI_KEY_CODE_DOWN:
        {
            // Move cursor down one line
            egui_dim_t cx = egui_view_textblock_get_cursor_x(local, local->cursor_pos);
            // Find end of current line (the '\n' or end of text)
            uint16_t next_start = local->cursor_pos;
            while (next_start < local->text_len && local->edit_buf[next_start] != '\n')
            {
                next_start++;
            }
            if (next_start >= local->text_len)
            {
                return 1; // already on last line
            }
            next_start++; // skip '\n'
            // Find end of next line
            uint16_t next_end = next_start;
            while (next_end < local->text_len && local->edit_buf[next_end] != '\n')
            {
                next_end++;
            }
            // Find best position on next line matching cx
            uint16_t best = next_start;
            for (uint16_t i = next_start; i <= next_end; i++)
            {
                char tmp[EGUI_CONFIG_TEXTBLOCK_EDIT_MAX_LENGTH + 1];
                uint16_t len = i - next_start;
                memcpy(tmp, &local->edit_buf[next_start], len);
                tmp[len] = '\0';
                egui_dim_t w = 0, h = 0;
                local->font->api->get_str_size(local->font, tmp, 0, 0, &w, &h);
                if (w <= cx)
                {
                    best = i;
                }
                else
                {
                    break;
                }
            }
            egui_view_textblock_set_cursor_pos(self, best);
            return 1;
        }
        case EGUI_KEY_CODE_HOME:
        {
            // Move to start of current line
            uint16_t pos = local->cursor_pos;
            while (pos > 0 && local->edit_buf[pos - 1] != '\n')
            {
                pos--;
            }
            egui_view_textblock_set_cursor_pos(self, pos);
            return 1;
        }
        case EGUI_KEY_CODE_END:
        {
            // Move to end of current line
            uint16_t pos = local->cursor_pos;
            while (pos < local->text_len && local->edit_buf[pos] != '\n')
            {
                pos++;
            }
            egui_view_textblock_set_cursor_pos(self, pos);
            return 1;
        }
        case EGUI_KEY_CODE_ENTER:
            egui_view_textblock_insert_char(self, '\n');
            return 1;
        default:
        {
            char c = egui_key_event_to_char(event);
            if (c != 0)
            {
                egui_view_textblock_insert_char(self, c);
                return 1;
            }
            break;
        }
        }
    }
    else if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
    {
        // Consume DOWN events for keys we handle
        switch (event->key_code)
        {
        case EGUI_KEY_CODE_BACKSPACE:
        case EGUI_KEY_CODE_LEFT:
        case EGUI_KEY_CODE_RIGHT:
        case EGUI_KEY_CODE_UP:
        case EGUI_KEY_CODE_DOWN:
        case EGUI_KEY_CODE_HOME:
        case EGUI_KEY_CODE_END:
        case EGUI_KEY_CODE_ENTER:
            return 1;
        default:
        {
            char c = egui_key_event_to_char(event);
            if (c != 0)
            {
                return 1;
            }
            break;
        }
        }
    }

    return 0;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

// ========================= Setters =========================

void egui_view_textblock_set_line_space(egui_view_t *self, egui_dim_t line_space)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    if (local->line_space == line_space)
    {
        return;
    }
    local->line_space = line_space;
    egui_view_textblock_update_content_size(self);
    egui_view_textblock_clamp_scroll(self);
    egui_view_invalidate(self);
}

void egui_view_textblock_set_max_lines(egui_view_t *self, egui_dim_t max_lines)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    local->max_lines = max_lines;
    egui_view_invalidate(self);
}

void egui_view_textblock_set_auto_height(egui_view_t *self, uint8_t is_auto_height)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    local->is_auto_height = is_auto_height;
    if (is_auto_height && local->font != NULL && local->text != NULL)
    {
        egui_view_textblock_update_content_size(self);
        egui_view_set_size(self, self->region.size.width, local->content_height + self->padding.top + self->padding.bottom);
    }
    egui_view_invalidate(self);
}

void egui_view_textblock_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    if (local->font == font)
    {
        return;
    }
    local->font = font;
    egui_view_textblock_update_content_size(self);
    egui_view_textblock_clamp_scroll(self);
    egui_view_invalidate(self);
}

void egui_view_textblock_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    local->color = color;
    local->alpha = alpha;
    egui_view_invalidate(self);
}

void egui_view_textblock_set_align_type(egui_view_t *self, uint8_t align_type)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    local->align_type = align_type;
    egui_view_invalidate(self);
}

void egui_view_textblock_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    local->text = text;

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (local->is_editable)
    {
        if (text != NULL)
        {
            uint16_t len = strlen(text);
            if (len > local->max_length)
            {
                len = local->max_length;
            }
            memcpy(local->edit_buf, text, len);
            local->edit_buf[len] = '\0';
            local->text_len = len;
        }
        else
        {
            local->edit_buf[0] = '\0';
            local->text_len = 0;
        }
        local->cursor_pos = local->text_len;
    }
#endif

    egui_view_textblock_update_content_size(self);
    local->scroll_offset_x = 0;
    local->scroll_offset_y = 0;

    if (local->is_auto_height && local->font != NULL)
    {
        egui_view_set_size(self, self->region.size.width, local->content_height + self->padding.top + self->padding.bottom);
    }

    egui_view_invalidate(self);
}

int egui_view_textblock_get_text_size(egui_view_t *self, const char *text, egui_dim_t max_width, egui_dim_t *width, egui_dim_t *height)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);

    if (local->font == NULL || text == NULL)
    {
        *width = 0;
        *height = 0;
        return 0;
    }
    local->font->api->get_str_size(local->font, text, 1, local->line_space, width, height);
    return 1;
}

// ========================= Border API =========================

void egui_view_textblock_set_border_enabled(egui_view_t *self, uint8_t enabled)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    if (local->is_border_enabled != enabled)
    {
        local->is_border_enabled = enabled;
        egui_view_invalidate(self);
    }
}

void egui_view_textblock_set_border_radius(egui_view_t *self, egui_dim_t radius)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    local->border_radius = radius;
    egui_view_invalidate(self);
}

void egui_view_textblock_set_border_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    local->border_color = color;
    egui_view_invalidate(self);
}

// ========================= Scrollbar API =========================

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
void egui_view_textblock_set_scrollbar_enabled(egui_view_t *self, uint8_t enabled)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    if (local->is_scrollbar_enabled != enabled)
    {
        local->is_scrollbar_enabled = enabled;
        egui_view_invalidate(self);
    }
}
#endif

// ========================= Edit mode API =========================

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
void egui_view_textblock_set_editable(egui_view_t *self, uint8_t is_editable)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    local->is_editable = is_editable;

    if (is_editable)
    {
        self->is_clickable = 1;
        self->is_focusable = 1;

        // Copy current text to edit buffer
        if (local->text != NULL)
        {
            uint16_t len = strlen(local->text);
            if (len > local->max_length)
            {
                len = local->max_length;
            }
            memcpy(local->edit_buf, local->text, len);
            local->edit_buf[len] = '\0';
            local->text_len = len;
        }
        else
        {
            local->edit_buf[0] = '\0';
            local->text_len = 0;
        }
        local->cursor_pos = local->text_len;
    }

    egui_view_textblock_update_content_size(self);
    egui_view_invalidate(self);
}

uint8_t egui_view_textblock_get_editable(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    return local->is_editable;
}

void egui_view_textblock_insert_char(egui_view_t *self, char c)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);

    if (!local->is_editable || local->text_len >= local->max_length || c == 0)
    {
        return;
    }

    // Shift characters right from cursor_pos
    for (int i = local->text_len; i > local->cursor_pos; i--)
    {
        local->edit_buf[i] = local->edit_buf[i - 1];
    }

    local->edit_buf[local->cursor_pos] = c;
    local->cursor_pos++;
    local->text_len++;
    local->edit_buf[local->text_len] = '\0';

    // Reset cursor blink
    local->cursor_visible = 1;
    egui_timer_stop_timer(&local->cursor_timer);
    egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTBLOCK_CURSOR_BLINK_MS, 0);

    egui_view_textblock_update_content_size(self);
    egui_view_textblock_ensure_cursor_visible(self);
    egui_view_invalidate(self);
}

void egui_view_textblock_delete_char(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);

    if (!local->is_editable || local->cursor_pos == 0)
    {
        return;
    }

    for (int i = local->cursor_pos - 1; i < local->text_len - 1; i++)
    {
        local->edit_buf[i] = local->edit_buf[i + 1];
    }

    local->cursor_pos--;
    local->text_len--;
    local->edit_buf[local->text_len] = '\0';

    // Reset cursor blink
    local->cursor_visible = 1;
    egui_timer_stop_timer(&local->cursor_timer);
    egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTBLOCK_CURSOR_BLINK_MS, 0);

    egui_view_textblock_update_content_size(self);
    egui_view_textblock_ensure_cursor_visible(self);
    egui_view_invalidate(self);
}

void egui_view_textblock_set_cursor_pos(egui_view_t *self, uint16_t pos)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);

    if (pos > local->text_len)
    {
        pos = local->text_len;
    }
    if (pos == local->cursor_pos)
    {
        return;
    }

    local->cursor_pos = pos;

    // Reset cursor blink
    local->cursor_visible = 1;
    egui_timer_stop_timer(&local->cursor_timer);
    egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTBLOCK_CURSOR_BLINK_MS, 0);

    egui_view_textblock_ensure_cursor_visible(self);
    egui_view_invalidate(self);
}

void egui_view_textblock_set_cursor_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    local->cursor_color = color;
    egui_view_invalidate(self);
}

const char *egui_view_textblock_get_edit_text(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);
    return local->edit_buf;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

// ========================= API Table =========================

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_textblock_t) = {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_textblock_on_touch_event,
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
        .on_draw = egui_view_textblock_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        .on_key_event = egui_view_textblock_on_key_event,
#else
        .on_key_event = egui_view_on_key_event,
#endif
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        .on_focus_changed = egui_view_textblock_on_focus_change,
#endif
};

// ========================= Init =========================

void egui_view_textblock_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_textblock_t);

    // Call super init
    egui_view_init(self);

    // Update API table
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_textblock_t);

    // Init text fields
    local->line_space = 2;
    local->max_lines = 0;
    local->align_type = EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP;
    local->is_auto_height = 0;
    local->alpha = EGUI_ALPHA_100;
    local->color = EGUI_THEME_TEXT_PRIMARY;
    local->font = NULL;
    local->text = NULL;

    // Scroll fields
    local->scroll_offset_x = 0;
    local->scroll_offset_y = 0;
    local->content_width = 0;
    local->content_height = 0;

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    local->touch_last_x = 0;
    local->touch_last_y = 0;
    local->is_touch_dragging = 0;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    local->is_scrollbar_enabled = 0;
    local->is_scrollbar_v_dragging = 0;
    local->is_scrollbar_h_dragging = 0;
    local->reserved_sb = 0;
#endif

    // Border fields
    local->is_border_enabled = 0;
    local->reserved_border = 0;
    local->border_radius = EGUI_THEME_RADIUS_SM;
    local->border_color = EGUI_THEME_BORDER;

    // Edit mode fields
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    local->is_editable = 0;
    local->cursor_visible = 0;
    local->reserved_edit = 0;
    local->cursor_pos = 0;
    local->text_len = 0;
    local->max_length = EGUI_CONFIG_TEXTBLOCK_EDIT_MAX_LENGTH;
    local->edit_buf[0] = '\0';
    local->cursor_color = EGUI_THEME_PRIMARY;

    egui_timer_init_timer(&local->cursor_timer, (void *)local, egui_view_textblock_cursor_timer_callback);
#endif

    // Make clickable so touch events work for scroll dragging
    self->is_clickable = 1;

    egui_view_set_view_name(self, "egui_view_textblock");
}

void egui_view_textblock_apply_params(egui_view_t *self, const egui_view_textblock_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_textblock_t);

    self->region = params->region;
    local->align_type = params->align_type;
    local->font = params->font;
    local->color = params->color;
    local->alpha = params->alpha;
    local->line_space = params->line_space;
    local->max_lines = params->max_lines;

    // Set text (this also updates content size)
    egui_view_textblock_set_text(self, params->text);
}

void egui_view_textblock_init_with_params(egui_view_t *self, const egui_view_textblock_params_t *params)
{
    egui_view_textblock_init(self);
    egui_view_textblock_apply_params(self, params);
}
