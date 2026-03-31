#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_spangroup.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"

static void egui_view_spangroup_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_spangroup_t);

    if (local->span_count == 0)
    {
        return;
    }

    egui_region_t work_region;
    egui_view_get_work_region(self, &work_region);

    egui_dim_t cursor_x = 0;
    egui_dim_t cursor_y = 0;
    egui_dim_t region_w = work_region.size.width;

    for (uint8_t i = 0; i < local->span_count; i++)
    {
        egui_view_span_t *span = &local->spans[i];
        if (span->text == NULL || span->text[0] == '\0')
        {
            continue;
        }

        const egui_font_t *font = span->font;
        if (font == NULL)
        {
            continue;
        }

        egui_dim_t text_w = 0;
        egui_dim_t text_h = 0;
        font->api->get_str_size(font, span->text, 0, 0, &text_w, &text_h);

        // If text does not fit on current line and cursor_x > 0, wrap to next line
        if (cursor_x > 0 && (cursor_x + text_w) > region_w)
        {
            cursor_x = 0;
            cursor_y += text_h + local->line_spacing;
        }

        // Draw the span text at cursor position
        egui_region_t draw_rect;
        draw_rect.location.x = work_region.location.x + cursor_x;
        draw_rect.location.y = work_region.location.y + cursor_y;
        draw_rect.size.width = text_w;
        draw_rect.size.height = text_h;

        egui_canvas_draw_text_in_rect(font, span->text, &draw_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, span->color, EGUI_ALPHA_100);

        cursor_x += text_w;
    }
}

int egui_view_spangroup_add_span(egui_view_t *self, const char *text, const egui_font_t *font, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_spangroup_t);

    if (local->span_count >= EGUI_VIEW_SPANGROUP_MAX_SPANS)
    {
        return -1;
    }

    egui_view_span_t *span = &local->spans[local->span_count];
    span->text = text;
    span->font = font;
    span->color = color;
    local->span_count++;

    egui_view_invalidate(self);
    return local->span_count - 1;
}

void egui_view_spangroup_clear(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_spangroup_t);

    local->span_count = 0;
    egui_view_invalidate(self);
}

void egui_view_spangroup_set_align(egui_view_t *self, uint8_t align)
{
    EGUI_LOCAL_INIT(egui_view_spangroup_t);

    if (local->align == align)
    {
        return;
    }
    local->align = align;
    egui_view_invalidate(self);
}

void egui_view_spangroup_set_line_spacing(egui_view_t *self, uint8_t spacing)
{
    EGUI_LOCAL_INIT(egui_view_spangroup_t);

    if (local->line_spacing == spacing)
    {
        return;
    }
    local->line_spacing = spacing;
    egui_view_invalidate(self);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_spangroup_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_spangroup_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_spangroup_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_spangroup_t);

    // Call super init
    egui_view_init(self);

    // Update API table
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_spangroup_t);

    // Init local data
    local->span_count = 0;
    local->line_spacing = 2;
    local->align = EGUI_ALIGN_LEFT;

    egui_api_memset(local->spans, 0, sizeof(local->spans));

    egui_view_set_view_name(self, "egui_view_spangroup");
}

void egui_view_spangroup_apply_params(egui_view_t *self, const egui_view_spangroup_params_t *params)
{
    self->region = params->region;
    egui_view_invalidate(self);
}

void egui_view_spangroup_init_with_params(egui_view_t *self, const egui_view_spangroup_params_t *params)
{
    egui_view_spangroup_init(self);
    egui_view_spangroup_apply_params(self, params);
}
