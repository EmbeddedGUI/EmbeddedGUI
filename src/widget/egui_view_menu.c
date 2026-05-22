#include <stdio.h>
#include <assert.h>

#include "egui_view_menu.h"
#include "core/egui_core.h"
#include "egui_view_icon_font.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
#include "canvas/egui_canvas_gradient.h"
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#include "shadow/egui_shadow.h"
#endif

#define EGUI_VIEW_MENU_PRESSED_NONE     (-1)
#define EGUI_VIEW_MENU_PRESSED_BACK     (-2)
#define EGUI_VIEW_MENU_TEXT_PADDING     6
#define EGUI_VIEW_MENU_ICON_GAP_DEFAULT 6

/**
 * @file egui_view_menu.c
 * @brief Stack-based menu widget with optional row icons and submenu arrows.
 *
 * Learning notes:
 * - page descriptors are
 * borrowed, so this widget never allocates menu data;
 * - navigation is modeled as a compact page-index stack;
 * - drawing and hit-testing both follow the
 * same header-plus-rows geometry.
 */

/** Resolve the icon font, falling back to a size derived from the target row. */
static const egui_font_t *egui_view_menu_resolve_icon_font(const egui_view_menu_t *local, egui_dim_t area_size)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    return egui_view_icon_font_get_auto(area_size, 18, 22);
}

/** Return the effective back icon glyph, using the default arrow when unset. */
static const char *egui_view_menu_resolve_back_icon(const egui_view_menu_t *local)
{
    if (local->back_icon != NULL)
    {
        return local->back_icon;
    }

    return EGUI_ICON_MS_ARROW_BACK;
}

/** Return the effective submenu icon glyph, using the default arrow when unset. */
static const char *egui_view_menu_resolve_submenu_icon(const egui_view_menu_t *local)
{
    if (local->submenu_icon != NULL)
    {
        return local->submenu_icon;
    }

    return EGUI_ICON_MS_ARROW_FORWARD;
}

/** Report whether the current page should render a clickable back icon. */
static uint8_t egui_view_menu_has_back_icon(const egui_view_menu_t *local)
{
    const char *icon = egui_view_menu_resolve_back_icon(local);
    return (local->stack_depth > 0 && icon != NULL && icon[0] != '\0' && egui_view_menu_resolve_icon_font(local, local->header_height) != NULL) ? 1U : 0U;
}

/** Report whether submenu rows can draw their trailing arrow icon. */
static uint8_t egui_view_menu_has_submenu_icon(const egui_view_menu_t *local)
{
    const char *icon = egui_view_menu_resolve_submenu_icon(local);
    return (icon != NULL && icon[0] != '\0' && egui_view_menu_resolve_icon_font(local, local->item_height) != NULL) ? 1U : 0U;
}

static uint8_t egui_view_menu_normalize_selected_index(egui_view_menu_t *local)
{
    const egui_view_menu_page_t *page;

    if (local->pages == NULL || local->current_page >= local->page_count)
    {
        local->selected_index = EGUI_VIEW_MENU_SELECTED_NONE;
        return local->selected_index;
    }

    page = &local->pages[local->current_page];
    if (page->item_count == 0)
    {
        local->selected_index = EGUI_VIEW_MENU_SELECTED_NONE;
    }
    else if (local->selected_index == EGUI_VIEW_MENU_SELECTED_NONE || local->selected_index >= page->item_count)
    {
        local->selected_index = 0;
    }

    return local->selected_index;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static void egui_view_menu_set_selected_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    const egui_view_menu_page_t *page;

    if (local->pages == NULL || local->current_page >= local->page_count)
    {
        return;
    }

    page = &local->pages[local->current_page];
    if (index >= page->item_count)
    {
        return;
    }

    if (local->selected_index != index)
    {
        local->selected_index = index;
        egui_view_invalidate(self);
    }
}
#endif

/** Attach a new borrowed page table and reset navigation state to the root page. */
void egui_view_menu_set_pages(egui_view_t *self, const egui_view_menu_page_t *pages, uint8_t page_count)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    local->pages = pages;
    local->page_count = page_count;
    local->current_page = 0;
    local->stack_depth = 0;
    local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
    local->selected_index = EGUI_VIEW_MENU_SELECTED_NONE;
    egui_view_menu_normalize_selected_index(local);
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

const egui_view_menu_page_t *egui_view_menu_get_pages(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return local->pages;
}

uint8_t egui_view_menu_get_page_count(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return local->page_count;
}

/** Push the current page onto the back stack and switch to a submenu page. */
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
    local->selected_index = EGUI_VIEW_MENU_SELECTED_NONE;
    egui_view_menu_normalize_selected_index(local);
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

uint8_t egui_view_menu_get_current_page(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return local->current_page;
}

uint8_t egui_view_menu_get_stack_depth(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return local->stack_depth;
}

uint8_t egui_view_menu_get_selected_index(egui_view_t *self)
{
    if (self == NULL)
    {
        return EGUI_VIEW_MENU_SELECTED_NONE;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return local->selected_index;
}

/** Pop one previous page from the back stack, if history is available. */
void egui_view_menu_go_back(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    if (local->stack_depth > 0)
    {
        local->stack_depth--;
        local->current_page = local->page_stack[local->stack_depth];
        local->pressed_index = EGUI_VIEW_MENU_PRESSED_NONE;
        local->selected_index = EGUI_VIEW_MENU_SELECTED_NONE;
        egui_view_menu_normalize_selected_index(local);
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
    }
}

/** Store the callback invoked when a leaf row is activated. */
void egui_view_menu_set_on_item_click(egui_view_t *self, egui_view_menu_item_click_cb_t callback)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    local->on_item_click = callback;
}

egui_view_menu_item_click_cb_t egui_view_menu_get_on_item_click(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return local->on_item_click;
}

/** Override the header row height used for both draw and hit-testing. */
void egui_view_menu_set_header_height(egui_view_t *self, egui_dim_t height)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    local->header_height = height;
    egui_view_invalidate(self);
}

egui_dim_t egui_view_menu_get_header_height(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return local->header_height;
}

/** Override the item row height used for layout and hit-testing. */
void egui_view_menu_set_item_height(egui_view_t *self, egui_dim_t height)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    local->item_height = height;
    egui_view_invalidate(self);
}

egui_dim_t egui_view_menu_get_item_height(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return local->item_height;
}

egui_color_t egui_view_menu_get_text_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return local->text_color;
}

/** Override the text color used in the header row. */
void egui_view_menu_set_header_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    local->header_text_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_menu_get_header_text_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return local->header_text_color;
}

/** Override the icon font used for row icons and navigation arrows. */
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

const egui_font_t *egui_view_menu_get_icon_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return local->icon_font;
}

/** Override the glyph strings used for back and submenu navigation icons. */
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

const char *egui_view_menu_get_back_icon(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return egui_view_menu_resolve_back_icon(local);
}

const char *egui_view_menu_get_submenu_icon(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return egui_view_menu_resolve_submenu_icon(local);
}

/** Set the gap between a row's optional icon glyph and its text label. */
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

egui_dim_t egui_view_menu_get_icon_text_gap(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_menu_t);
    return local->icon_text_gap;
}

/** Draw the active page: widget background, header, rows, and separators. */
void egui_view_menu_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);

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
    const egui_font_t *header_icon_font = egui_view_menu_resolve_icon_font(local, hdr_h);
    const egui_font_t *item_icon_font = egui_view_menu_resolve_icon_font(local, item_h);
    uint8_t has_back_icon = egui_view_menu_has_back_icon(local);
    uint8_t has_submenu_icon = egui_view_menu_has_submenu_icon(local);

    // Fill the full widget background so shadow or empty trailing space stays clean.
    egui_canvas_draw_rectangle_fill(canvas, x, y, w, region.size.height, local->item_color, EGUI_ALPHA_100);

    // Paint the fixed header strip before drawing its title and optional back icon.
#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
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
        egui_canvas_draw_rectangle_fill_gradient(canvas, x, y, w, hdr_h, &grad);
    }
#else
    egui_canvas_draw_rectangle_fill(canvas, x, y, w, hdr_h, local->header_color, EGUI_ALPHA_100);
#endif

    // The back icon only appears after the user has navigated below the root page.
    if (has_back_icon)
    {
        egui_region_t back_rect = {{x, y}, {hdr_h, hdr_h}};
        egui_canvas_draw_text_in_rect(canvas, header_icon_font, egui_view_menu_resolve_back_icon(local), &back_rect, EGUI_ALIGN_CENTER,
                                      local->header_text_color, EGUI_ALPHA_100);
    }

    // Keep the title centered, but reserve edge space when the back button is shown.
    {
        egui_region_t title_rect = {{x, y}, {w, hdr_h}};
        if (has_back_icon && w > hdr_h * 2)
        {
            title_rect.location.x += hdr_h;
            title_rect.size.width -= hdr_h * 2;
        }
        egui_canvas_draw_text_in_rect(canvas, font, page->title, &title_rect, EGUI_ALIGN_CENTER, local->header_text_color, EGUI_ALPHA_100);
    }

    // Each page item occupies one fixed-height row directly below the header.
    uint8_t i;
    for (i = 0; i < page->item_count; i++)
    {
        egui_dim_t item_y = y + hdr_h + (egui_dim_t)i * item_h;

        // Reuse the same row geometry for both press feedback and normal fill.
        if (self->is_pressed && local->pressed_index == (int8_t)i)
        {
            egui_canvas_draw_rectangle_fill(canvas, x, item_y, w, item_h, local->highlight_color, EGUI_ALPHA_100);
        }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        else if (self->is_focused && local->selected_index == i)
        {
            egui_canvas_draw_rectangle_fill(canvas, x, item_y, w, item_h, EGUI_THEME_FOCUS, EGUI_ALPHA_20);
            egui_canvas_draw_rectangle(canvas, x, item_y, w, item_h, 1, EGUI_THEME_FOCUS, EGUI_ALPHA_100);
        }
#endif
        else
        {
            egui_canvas_draw_rectangle_fill(canvas, x, item_y, w, item_h, local->item_color, EGUI_ALPHA_100);
        }

        // Leading icons and trailing submenu arrows both shrink the available text span.
        {
            egui_dim_t text_x = x + EGUI_VIEW_MENU_TEXT_PADDING;
            egui_dim_t trailing_width = 0;
            egui_dim_t leading_width = 0;
            egui_dim_t text_width;

            if (item_icon_font != NULL && page->items[i].icon != NULL && page->items[i].icon[0] != '\0')
            {
                egui_region_t icon_rect = {{x, item_y}, {item_h, item_h}};
                egui_canvas_draw_text_in_rect(canvas, item_icon_font, page->items[i].icon, &icon_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
                leading_width = item_h + local->icon_text_gap;
            }

            if (page->items[i].sub_page_index != EGUI_VIEW_MENU_ITEM_LEAF && has_submenu_icon)
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
            egui_canvas_draw_text_in_rect(canvas, font, page->items[i].text, &text_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, local->text_color,
                                          EGUI_ALPHA_100);
        }

        // Submenu rows add a trailing arrow to communicate drill-down navigation.
        if (page->items[i].sub_page_index != EGUI_VIEW_MENU_ITEM_LEAF && has_submenu_icon)
        {
            egui_region_t arrow_rect = {{x + w - item_h, item_y}, {item_h, item_h}};
            egui_canvas_draw_text_in_rect(canvas, item_icon_font, egui_view_menu_resolve_submenu_icon(local), &arrow_rect, EGUI_ALIGN_CENTER, local->text_color,
                                          EGUI_ALPHA_100);
        }

        // Draw thin separators between rows, but not after the last one.
        if (i < page->item_count - 1)
        {
            egui_canvas_draw_rectangle_fill(canvas, x, item_y + item_h, w, 1, local->highlight_color, EGUI_ALPHA_100);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/** Map widget-local pointer coordinates to the back button or one item row. */
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

/** Track press state and trigger either back navigation, submenu navigation, or a leaf callback. */
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
            egui_view_menu_set_selected_index(self, (uint8_t)pressed_index);
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

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_menu_move_selected_index(egui_view_t *self, uint8_t key_code)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    const egui_view_menu_page_t *page;
    uint8_t old_index;
    uint8_t new_index;

    if (local->pages == NULL || local->current_page >= local->page_count)
    {
        return 0;
    }

    page = &local->pages[local->current_page];
    if (page->item_count == 0)
    {
        local->selected_index = EGUI_VIEW_MENU_SELECTED_NONE;
        return 1;
    }

    old_index = egui_view_menu_normalize_selected_index(local);
    new_index = old_index;

    switch (key_code)
    {
    case EGUI_KEY_CODE_UP:
        if (new_index > 0)
        {
            new_index--;
        }
        break;
    case EGUI_KEY_CODE_DOWN:
        if (new_index + 1 < page->item_count)
        {
            new_index++;
        }
        break;
    case EGUI_KEY_CODE_HOME:
        new_index = 0;
        break;
    case EGUI_KEY_CODE_END:
        new_index = (uint8_t)(page->item_count - 1);
        break;
    default:
        return 0;
    }

    egui_view_menu_set_selected_index(self, new_index);
    return 1;
}

static int egui_view_menu_activate_selected_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);
    const egui_view_menu_page_t *page;
    const egui_view_menu_item_t *item;
    uint8_t index;

    if (local->pages == NULL || local->current_page >= local->page_count)
    {
        return 0;
    }

    page = &local->pages[local->current_page];
    index = egui_view_menu_normalize_selected_index(local);
    if (index == EGUI_VIEW_MENU_SELECTED_NONE || index >= page->item_count)
    {
        return 1;
    }

    item = &page->items[index];
    if (item->sub_page_index != EGUI_VIEW_MENU_ITEM_LEAF)
    {
        egui_view_menu_navigate_to(self, item->sub_page_index);
    }
    else if (local->on_item_click != NULL)
    {
        local->on_item_click(self, local->current_page, index);
    }

    return 1;
}

static int egui_view_menu_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (self->is_enable == false || event == NULL)
    {
        return 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_UP:
    case EGUI_KEY_CODE_DOWN:
    case EGUI_KEY_CODE_HOME:
    case EGUI_KEY_CODE_END:
        if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
        {
            return 1;
        }
        if (event->type == EGUI_KEY_EVENT_ACTION_UP || event->type == EGUI_KEY_EVENT_ACTION_REPEAT)
        {
            return egui_view_menu_move_selected_index(self, event->key_code);
        }
        return 1;
    case EGUI_KEY_CODE_RIGHT:
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
        {
            return 1;
        }
        if (event->type == EGUI_KEY_EVENT_ACTION_UP)
        {
            return egui_view_menu_activate_selected_index(self);
        }
        return 1;
    case EGUI_KEY_CODE_LEFT:
        if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
        {
            return 1;
        }
        if (event->type == EGUI_KEY_EVENT_ACTION_UP)
        {
            egui_view_menu_go_back(self);
            return 1;
        }
        return 1;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

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
        .on_key_event = egui_view_menu_on_key_event,
#endif
};

/** Initialize the menu with root-page defaults, stock colors, and default arrows. */
void egui_view_menu_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_menu_t);
    // call super init.
    egui_view_init(self, core);
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
    local->selected_index = EGUI_VIEW_MENU_SELECTED_NONE;
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

/** Apply geometry plus header and row sizing from a parameter block. */
void egui_view_menu_apply_params(egui_view_t *self, const egui_view_menu_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_menu_t);

    self->region = params->region;
    local->header_height = params->header_height;
    local->item_height = params->item_height;

    egui_view_invalidate(self);
}

/** Convenience helper that initializes the menu before applying params. */
void egui_view_menu_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_menu_params_t *params)
{
    egui_view_menu_init(self, core);
    egui_view_menu_apply_params(self, params);
}
