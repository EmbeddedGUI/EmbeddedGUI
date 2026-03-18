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

#define EGUI_VIEW_MENU_PRESSED_NONE     (-1)
#define EGUI_VIEW_MENU_PRESSED_BACK     (-2)
#define EGUI_VIEW_MENU_TEXT_PADDING     6
#define EGUI_VIEW_MENU_ICON_GAP_DEFAULT 6

static const egui_font_t *egui_view_menu_get_icon_font(egui_view_menu_t *local, egui_dim_t area_size)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    if (area_size <= 18)
    {
        return EGUI_FONT_ICON_MS_16;
    }
    if (area_size <= 22)
    {
        return EGUI_FONT_ICON_MS_20;
    }
    return EGUI_FONT_ICON_MS_24;
}

static const char *egui_view_menu_get_back_icon(const egui_view_menu_t *local)
{
    if (local->back_icon != NULL)
    {
        return local->back_icon;
    }

    return EGUI_ICON_MS_ARROW_BACK;
}

static const char *egui_view_menu_get_submenu_icon(const egui_view_menu_t *local)
{
    if (local->submenu_icon != NULL)
    {
        return local->submenu_icon;
    }

    return EGUI_ICON_MS_ARROW_FORWARD;
}

void egui_view_menu_set_pages(egui_view_t *self, const egui_view_menu_page_t *pages, uint8_t page_count)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    local->pages = pages;
    local->page_count = page_count;
    local->current_page = 0;
    local->stack_depth = 0;
    local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
    egui_view_set_pressed(self, false);
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
    egui_view_set_pressed(self, false);
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
        egui_view_set_pressed(self, false);
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

void egui_view_menu_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

void egui_view_menu_set_navigation_icons(egui_view_t *self, const char *back_icon, const char *submenu_icon)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);

    if (back_icon == NULL)
    {
        back_icon = EGUI_ICON_MS_ARROW_BACK;
    }

    if (submenu_icon == NULL)
    {
        submenu_icon = EGUI_ICON_MS_ARROW_FORWARD;
    }

    if (local->back_icon == back_icon && local->submenu_icon == submenu_icon)
    {
        return;
    }

    local->back_icon = back_icon;
    local->submenu_icon = submenu_icon;
    egui_view_invalidate(self);
}

void egui_view_menu_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);

    if (gap < 0)
    {
        gap = 0;
    }

    if (local->icon_text_gap == gap)
    {
        return;
    }

    local->icon_text_gap = gap;
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
    const egui_font_t *header_icon_font = egui_view_menu_get_icon_font(local, hdr_h);
    const egui_font_t *item_icon_font = egui_view_menu_get_icon_font(local, item_h);

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
        egui_canvas_draw_text_in_rect(header_icon_font, egui_view_menu_get_back_icon(local), &back_rect, EGUI_ALIGN_CENTER, local->header_text_color,
                                      EGUI_ALPHA_100);
    }

    // Draw title centered in header
    {
        egui_region_t title_rect = {{x, y}, {w, hdr_h}};
        if (local->stack_depth > 0 && w > hdr_h * 2)
        {
            title_rect.location.x += hdr_h;
            title_rect.size.width -= hdr_h * 2;
        }
        egui_canvas_draw_text_in_rect(font, page->title, &title_rect, EGUI_ALIGN_CENTER, local->header_text_color, EGUI_ALPHA_100);
    }

    // Draw items
    uint8_t i;
    for (i = 0; i < page->item_count; i++)
    {
        egui_dim_t item_y = y + hdr_h + (egui_dim_t)i * item_h;

        // Highlight pressed item
        if (self->is_pressed && local->pressed_index == (int8_t)i)
        {
            egui_canvas_draw_rectangle_fill(x, item_y, w, item_h, local->highlight_color, EGUI_ALPHA_100);
        }
        else
        {
            egui_canvas_draw_rectangle_fill(x, item_y, w, item_h, local->item_color, EGUI_ALPHA_100);
        }

        // Draw item text left-aligned with padding
        {
            egui_dim_t text_x = x + EGUI_VIEW_MENU_TEXT_PADDING;
            egui_dim_t trailing_width = 0;
            egui_dim_t leading_width = 0;
            egui_dim_t text_width;

            if (page->items[i].icon != NULL)
            {
                egui_region_t icon_rect = {{x, item_y}, {item_h, item_h}};
                egui_canvas_draw_text_in_rect(item_icon_font, page->items[i].icon, &icon_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
                leading_width = item_h + local->icon_text_gap;
            }

            if (page->items[i].sub_page_index != EGUI_VIEW_MENU_ITEM_LEAF)
            {
                trailing_width = item_h;
            }
            text_x += leading_width;
            text_width = w - (text_x - x) - EGUI_VIEW_MENU_TEXT_PADDING - trailing_width;
            if (text_width < 0)
            {
                text_width = 0;
            }
            egui_region_t text_rect = {{text_x, item_y}, {text_width, item_h}};
            egui_canvas_draw_text_in_rect(font, page->items[i].text, &text_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, local->text_color, EGUI_ALPHA_100);
        }

        // Draw trailing arrow icon for sub-menu items
        if (page->items[i].sub_page_index != EGUI_VIEW_MENU_ITEM_LEAF)
        {
            egui_region_t arrow_rect = {{x + w - item_h, item_y}, {item_h, item_h}};
            egui_canvas_draw_text_in_rect(item_icon_font, egui_view_menu_get_submenu_icon(local), &arrow_rect, EGUI_ALIGN_CENTER, local->text_color,
                                          EGUI_ALPHA_100);
        }

        // Draw separator between items only (not after the last item)
        if (i < page->item_count - 1)
        {
            egui_canvas_draw_rectangle_fill(x, item_y + item_h, w, 1, local->highlight_color, EGUI_ALPHA_100);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int8_t egui_view_menu_get_hit_index(egui_view_t *self, egui_view_menu_t *local, egui_dim_t touch_x, egui_dim_t touch_y)
{
    const egui_view_menu_page_t *page = &local->pages[local->current_page];

    if (touch_x < 0 || touch_y < 0 || touch_x >= self->region_screen.size.width || touch_y >= self->region_screen.size.height)
    {
        return EGUI_VIEW_MENU_PRESSED_NONE;
    }

    if (touch_y < local->header_height)
    {
        if (local->stack_depth > 0 && touch_x < local->header_height)
        {
            return EGUI_VIEW_MENU_PRESSED_BACK;
        }
        return EGUI_VIEW_MENU_PRESSED_NONE;
    }

    {
        int8_t idx = (int8_t)((touch_y - local->header_height) / local->item_height);
        if (idx >= 0 && idx < (int8_t)page->item_count)
        {
            return idx;
        }
    }

    return EGUI_VIEW_MENU_PRESSED_NONE;
}

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
    int8_t hit_index = egui_view_menu_get_hit_index(self, local, touch_x, touch_y);

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        if (local->pressed_index != hit_index)
        {
            local->pressed_index = hit_index;
            egui_view_invalidate(self);
        }
        egui_view_set_pressed(self, hit_index != EGUI_VIEW_MENU_PRESSED_NONE);
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        uint8_t is_pressed = (local->pressed_index != EGUI_VIEW_MENU_PRESSED_NONE && local->pressed_index == hit_index);

        if (self->is_pressed != is_pressed)
        {
            egui_view_set_pressed(self, is_pressed);
            egui_view_invalidate(self);
        }
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_UP:
    {
        int8_t pressed_index = local->pressed_index;
        uint8_t was_pressed = self->is_pressed;

        local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
        egui_view_set_pressed(self, false);

        if (was_pressed && pressed_index == EGUI_VIEW_MENU_PRESSED_BACK && hit_index == EGUI_VIEW_MENU_PRESSED_BACK)
        {
            egui_view_menu_go_back(self);
        }
        else if (was_pressed && pressed_index >= 0 && pressed_index < (int8_t)page->item_count && pressed_index == hit_index)
        {
            const egui_view_menu_item_t *item = &page->items[pressed_index];
            if (item->sub_page_index != EGUI_VIEW_MENU_ITEM_LEAF)
            {
                egui_view_menu_navigate_to(self, item->sub_page_index);
            }
            else if (local->on_item_click)
            {
                local->on_item_click(self, local->current_page, pressed_index);
            }
        }
        egui_view_invalidate(self);
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
    {
        local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
        egui_view_set_pressed(self, false);
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
    local->icon_text_gap = EGUI_VIEW_MENU_ICON_GAP_DEFAULT;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->icon_font = NULL;
    local->back_icon = EGUI_ICON_MS_ARROW_BACK;
    local->submenu_icon = EGUI_ICON_MS_ARROW_FORWARD;
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
