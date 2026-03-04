#include <stdio.h>
#include <assert.h>

#include "egui_view_menu.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#include "shadow/egui_shadow.h"
#endif

#define EGUI_VIEW_MENU_PRESSED_NONE (-1)
#define EGUI_VIEW_MENU_PRESSED_BACK (-2)
#define EGUI_VIEW_MENU_TEXT_PADDING 6

void egui_view_menu_set_pages(egui_view_t *self, const egui_view_menu_page_t *pages, uint8_t page_count)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    local->pages = pages;
    local->page_count = page_count;
    local->current_page = 0;
    local->stack_depth = 0;
    local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
    egui_view_invalidate(self);
}

void egui_view_menu_navigate_to(egui_view_t *self, uint8_t page_index)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    if (page_index >= local->page_count)
    {
        return;
    }
    if (local->stack_depth < EGUI_VIEW_MENU_MAX_STACK)
    {
        local->page_stack[local->stack_depth] = local->current_page;
        local->stack_depth++;
    }
    local->current_page = page_index;
    local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
    egui_view_invalidate(self);
}

void egui_view_menu_go_back(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    if (local->stack_depth > 0)
    {
        local->stack_depth--;
        local->current_page = local->page_stack[local->stack_depth];
        local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
        egui_view_invalidate(self);
    }
}

void egui_view_menu_set_on_item_click(egui_view_t *self, egui_view_menu_item_click_cb_t callback)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    local->on_item_click = callback;
}

void egui_view_menu_set_header_height(egui_view_t *self, egui_dim_t height)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    local->header_height = height;
    egui_view_invalidate(self);
}

void egui_view_menu_set_item_height(egui_view_t *self, egui_dim_t height)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    local->item_height = height;
    egui_view_invalidate(self);
}

void egui_view_menu_set_header_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    local->header_text_color = color;
    egui_view_invalidate(self);
}

void egui_view_menu_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    if (local->pages == NULL || local->current_page >= local->page_count)
    {
        return;
    }

    const egui_view_menu_page_t *page = &local->pages[local->current_page];
    const egui_font_t *font = local->font ? local->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t x = region.location.x;
    egui_dim_t y = region.location.y;
    egui_dim_t w = region.size.width;
    egui_dim_t hdr_h = local->header_height;
    egui_dim_t item_h = local->item_height;

    // Fill full widget background first to prevent shadow inner-rect bleed-through
    // in rows below the last item when items don't span the full widget height.
    egui_canvas_draw_rectangle_fill(x, y, w, region.size.height, local->item_color, EGUI_ALPHA_100);

    // Draw header background
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(local->header_color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = local->header_color},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = stops,
        };
        egui_canvas_draw_rectangle_fill_gradient(x, y, w, hdr_h, &grad);
    }
#else
    egui_canvas_draw_rectangle_fill(x, y, w, hdr_h, local->header_color, EGUI_ALPHA_100);
#endif

    // Draw back arrow if we have stack depth
    if (local->stack_depth > 0)
    {
        egui_region_t back_rect = {{x, y}, {hdr_h, hdr_h}};
        egui_canvas_draw_text_in_rect(font, "<", &back_rect, EGUI_ALIGN_CENTER, local->header_text_color, EGUI_ALPHA_100);
    }

    // Draw title centered in header
    {
        egui_region_t title_rect = {{x, y}, {w, hdr_h}};
        egui_canvas_draw_text_in_rect(font, page->title, &title_rect, EGUI_ALIGN_CENTER, local->header_text_color, EGUI_ALPHA_100);
    }

    // Draw items
    uint8_t i;
    for (i = 0; i < page->item_count; i++)
    {
        egui_dim_t item_y = y + hdr_h + (egui_dim_t)i * item_h;

        // Highlight pressed item
        if (local->pressed_index == (int8_t)i)
        {
            egui_canvas_draw_rectangle_fill(x, item_y, w, item_h, local->highlight_color, EGUI_ALPHA_100);
        }
        else
        {
            egui_canvas_draw_rectangle_fill(x, item_y, w, item_h, local->item_color, EGUI_ALPHA_100);
        }

        // Draw item text left-aligned with padding
        {
            egui_region_t text_rect = {{x + EGUI_VIEW_MENU_TEXT_PADDING, item_y}, {w - 2 * EGUI_VIEW_MENU_TEXT_PADDING, item_h}};
            egui_canvas_draw_text_in_rect(font, page->items[i].text, &text_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, local->text_color, EGUI_ALPHA_100);
        }

        // Draw ">" arrow for sub-menu items
        if (page->items[i].sub_page_index != EGUI_VIEW_MENU_ITEM_LEAF)
        {
            egui_region_t arrow_rect = {{x + w - hdr_h, item_y}, {hdr_h, item_h}};
            egui_canvas_draw_text_in_rect(font, ">", &arrow_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
        }

        // Draw separator between items only (not after the last item)
        if (i < page->item_count - 1)
        {
            egui_canvas_draw_rectangle_fill(x, item_y + item_h, w, 1, local->highlight_color, EGUI_ALPHA_100);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
int egui_view_menu_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);

    if (self->is_enable == false || local->pages == NULL)
    {
        return 0;
    }

    const egui_view_menu_page_t *page = &local->pages[local->current_page];
    egui_dim_t touch_x = event->location.x - self->region_screen.location.x;
    egui_dim_t touch_y = event->location.y - self->region_screen.location.y;
    egui_dim_t hdr_h = local->header_height;
    egui_dim_t item_h = local->item_height;

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        // Check if touch is in header area (back button)
        if (touch_y < hdr_h && local->stack_depth > 0 && touch_x < hdr_h)
        {
            local->pressed_index = EGUI_VIEW_MENU_PRESSED_BACK;
        }
        else if (touch_y >= hdr_h)
        {
            // Check which item was pressed
            int8_t idx = (int8_t)((touch_y - hdr_h) / item_h);
            if (idx >= 0 && idx < (int8_t)page->item_count)
            {
                local->pressed_index = idx;
            }
            else
            {
                local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
            }
        }
        else
        {
            local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
        }
        egui_view_invalidate(self);
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_UP:
    {
        if (local->pressed_index == EGUI_VIEW_MENU_PRESSED_BACK)
        {
            egui_view_menu_go_back(self);
        }
        else if (local->pressed_index >= 0 && local->pressed_index < (int8_t)page->item_count)
        {
            const egui_view_menu_item_t *item = &page->items[local->pressed_index];
            if (item->sub_page_index != EGUI_VIEW_MENU_ITEM_LEAF)
            {
                egui_view_menu_navigate_to(self, item->sub_page_index);
            }
            else if (local->on_item_click)
            {
                local->on_item_click(self, local->current_page, local->pressed_index);
            }
        }
        local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
        egui_view_invalidate(self);
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
    {
        local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
        egui_view_invalidate(self);
        break;
    }
    default:
        break;
    }

    return 1; // consume event
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_menu_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_menu_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = NULL,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_menu_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_menu_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_menu_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_menu_t);

    // init local data.
    local->pages = NULL;
    local->page_count = 0;
    local->current_page = 0;
    local->stack_depth = 0;
    local->header_height = 30;
    local->item_height = 30;
    local->header_color = EGUI_THEME_PRIMARY_DARK;
    local->item_color = EGUI_THEME_SURFACE;
    local->text_color = EGUI_THEME_TEXT_PRIMARY;
    local->header_text_color = EGUI_COLOR_WHITE;
    local->highlight_color = EGUI_THEME_TRACK_BG;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
    local->on_item_click = NULL;

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
    {
        static const egui_shadow_t menu_shadow = {
                .width = EGUI_THEME_SHADOW_WIDTH_MD,
                .ofs_x = 0,
                .ofs_y = EGUI_THEME_SHADOW_OFS_Y_MD,
                .spread = 0,
                .opa = EGUI_THEME_SHADOW_OPA,
                .color = EGUI_COLOR_BLACK,
                .corner_radius = 0,
        };
        egui_view_set_shadow(self, &menu_shadow);
    }
#endif

    egui_view_set_view_name(self, "egui_view_menu");
}

void egui_view_menu_apply_params(egui_view_t *self, const egui_view_menu_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);

    self->region = params->region;
    local->header_height = params->header_height;
    local->item_height = params->item_height;

    egui_view_invalidate(self);
}

void egui_view_menu_init_with_params(egui_view_t *self, const egui_view_menu_params_t *params)
{
    egui_view_menu_init(self);
    egui_view_menu_apply_params(self, params);
}
