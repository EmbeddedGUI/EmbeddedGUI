#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_button_matrix.h"
#include "core/egui_core.h"
#include "egui_view_icon_font.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
#include "canvas/egui_canvas_gradient.h"
#endif

/**
 * @file egui_view_button_matrix.c
 * @brief Equal-sized button grid with optional icons and persistent selection.
 *
 * Reading notes:
 * - cell geometry
 * is recomputed from the current work region on every draw;
 * - press and selection transitions invalidate only the affected cells when possible;
 * - icon
 * and text painting is clipped per cell so neighboring buttons stay isolated.
 */

/* Expand dirty regions slightly so active borders redraw without clipped edges. */
#define EGUI_VIEW_BUTTON_MATRIX_DIRTY_PAD 2

/** Resolve the icon font, falling back to an automatically sized built-in font. */
static const egui_font_t *egui_view_button_matrix_resolve_icon_font(egui_view_button_matrix_t *local, egui_dim_t area_size)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    return egui_view_icon_font_get_auto(area_size, 18, 22);
}

/** Draw text under a temporary cell clip and then restore the previous clip state. */
static void egui_view_button_matrix_draw_text_clipped(egui_canvas_t *canvas, const egui_font_t *font, const char *text, const egui_region_t *text_rect,
                                                      const egui_region_t *clip_rect, uint8_t align, egui_color_t color, egui_alpha_t alpha)
{
    egui_region_t draw_rect = *text_rect;
    egui_region_t text_clip = *clip_rect;
    const egui_region_t *prev_clip = egui_canvas_get_extra_clip(canvas);

    if (font == NULL || text == NULL || text[0] == '\0')
    {
        return;
    }

    if (prev_clip != NULL)
    {
        egui_region_intersect(&text_clip, prev_clip, &text_clip);
    }

    if (egui_region_is_empty(&text_clip))
    {
        return;
    }

    egui_canvas_set_extra_clip(canvas, &text_clip);
    egui_canvas_draw_text_in_rect(canvas, font, text, &draw_rect, align, color, alpha);
    if (prev_clip != NULL)
    {
        egui_canvas_set_extra_clip(canvas, prev_clip);
    }
    else
    {
        egui_canvas_clear_extra_clip(canvas);
    }
}

/** Map one cell index to its padded work-region rectangle for local invalidation. */
static uint8_t egui_view_button_matrix_get_cell_region(egui_view_t *self, egui_view_button_matrix_t *local, uint8_t index, egui_region_t *cell_region)
{
    egui_region_t work_region;
    egui_dim_t btn_w;
    egui_dim_t btn_h;
    uint8_t rows;
    uint8_t col;
    uint8_t row;

    if (cell_region == NULL || index == EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE || index >= local->btn_count || local->btn_count == 0 || local->cols == 0)
    {
        return 0;
    }

    egui_view_get_work_region(self, &work_region);
    if (egui_region_is_empty(&work_region))
    {
        return 0;
    }

    rows = (uint8_t)((local->btn_count + local->cols - 1U) / local->cols);
    if (rows == 0)
    {
        return 0;
    }

    btn_w = (work_region.size.width - local->gap * (local->cols - 1)) / local->cols;
    btn_h = (work_region.size.height - local->gap * (rows - 1)) / rows;
    if (btn_w <= 0 || btn_h <= 0)
    {
        return 0;
    }

    col = (uint8_t)(index % local->cols);
    row = (uint8_t)(index / local->cols);

    egui_region_init(cell_region, work_region.location.x + col * (btn_w + local->gap), work_region.location.y + row * (btn_h + local->gap), btn_w, btn_h);

    cell_region->location.x -= EGUI_VIEW_BUTTON_MATRIX_DIRTY_PAD;
    cell_region->location.y -= EGUI_VIEW_BUTTON_MATRIX_DIRTY_PAD;
    cell_region->size.width += EGUI_VIEW_BUTTON_MATRIX_DIRTY_PAD * 2;
    cell_region->size.height += EGUI_VIEW_BUTTON_MATRIX_DIRTY_PAD * 2;
    egui_region_intersect(cell_region, &work_region, cell_region);

    return egui_region_is_empty(cell_region) ? 0 : 1;
}

/** Merge the dirty regions for a short list of changed cell indices and invalidate once. */
static void egui_view_button_matrix_invalidate_indices(egui_view_t *self, egui_view_button_matrix_t *local, const uint8_t *indices, uint8_t count)
{
    egui_region_t dirty_region;
    egui_region_t cell_region;
    uint8_t i;
    uint8_t j;
    uint8_t valid_count = 0;

    if (count == 0)
    {
        return;
    }

    if (self->region_screen.size.width <= 0 || self->region_screen.size.height <= 0)
    {
        egui_view_invalidate(self);
        return;
    }

    if (egui_view_has_pending_dirty(self))
    {
        egui_view_invalidate_full(self);
        return;
    }

    egui_region_init_empty(&dirty_region);
    for (i = 0; i < count; i++)
    {
        uint8_t index = indices[i];
        uint8_t is_duplicate = 0;

        if (index == EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE || index >= local->btn_count)
        {
            continue;
        }

        for (j = 0; j < i; j++)
        {
            if (indices[j] == index)
            {
                is_duplicate = 1;
                break;
            }
        }
        if (is_duplicate)
        {
            continue;
        }

        valid_count++;
        if (egui_view_button_matrix_get_cell_region(self, local, index, &cell_region))
        {
            if (egui_region_is_empty(&dirty_region))
            {
                egui_region_copy(&dirty_region, &cell_region);
            }
            else
            {
                egui_region_union(&dirty_region, &cell_region, &dirty_region);
            }
        }
    }

    if (egui_region_is_empty(&dirty_region))
    {
        if (valid_count > 0)
        {
            egui_view_invalidate(self);
        }
        return;
    }

    egui_view_invalidate_region(self, &dirty_region);
}

/** Borrow new label data, clamp the cell count, and reset invalid stored selection. */
void egui_view_button_matrix_set_labels(egui_view_t *self, const char **labels, uint8_t count, uint8_t cols)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->labels = labels;
    if (count > EGUI_VIEW_BUTTON_MATRIX_MAX_BUTTONS)
    {
        count = EGUI_VIEW_BUTTON_MATRIX_MAX_BUTTONS;
    }
    local->btn_count = count;
    local->cols = cols;
    if (local->selected_index >= local->btn_count)
    {
        local->selected_index = EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE;
    }
    egui_view_invalidate(self);
}

const char **egui_view_button_matrix_get_labels(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->labels;
}

uint8_t egui_view_button_matrix_get_button_count(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->btn_count;
}

uint8_t egui_view_button_matrix_get_cols(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->cols;
}

/** Store the callback fired when a touch-up completes inside the same cell. */
void egui_view_button_matrix_set_on_click(egui_view_t *self, egui_view_button_matrix_click_cb_t callback)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->on_click = callback;
}

egui_view_button_matrix_click_cb_t egui_view_button_matrix_get_on_click(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->on_click;
}

/** Toggle persistent single selection and redraw only cells whose highlight changes. */
void egui_view_button_matrix_set_selection_enabled(egui_view_t *self, uint8_t enabled)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    uint8_t dirty_indices[1];
    uint8_t dirty_count = 0;

    enabled = enabled ? 1 : 0;
    if (local->selection_enabled == enabled)
    {
        return;
    }

    if (local->selection_enabled && local->selected_index != EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE)
    {
        dirty_indices[dirty_count++] = local->selected_index;
    }

    local->selection_enabled = enabled;
    if (!enabled)
    {
        local->selected_index = EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE;
    }
    else if (local->selected_index != EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE)
    {
        dirty_indices[dirty_count++] = local->selected_index;
    }

    egui_view_button_matrix_invalidate_indices(self, local, dirty_indices, dirty_count);
}

uint8_t egui_view_button_matrix_get_selection_enabled(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->selection_enabled;
}

/** Update the stored selection, clamping invalid input and redrawing old and new cells. */
void egui_view_button_matrix_set_selected_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    uint8_t old_index;
    uint8_t dirty_indices[2];

    if (index == EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE || local->btn_count == 0)
    {
        index = EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE;
    }
    else if (index >= local->btn_count)
    {
        index = (uint8_t)(local->btn_count - 1U);
    }
    if (local->selected_index == index)
    {
        return;
    }

    old_index = local->selected_index;
    local->selected_index = index;
    if (!local->selection_enabled)
    {
        return;
    }

    dirty_indices[0] = old_index;
    dirty_indices[1] = local->selected_index;
    egui_view_button_matrix_invalidate_indices(self, local, dirty_indices, EGUI_ARRAY_SIZE(dirty_indices));
}

/** Return the stored selected cell index, or `SELECTED_NONE`. */
uint8_t egui_view_button_matrix_get_selected_index(egui_view_t *self)
{
    if (self == NULL)
    {
        return EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->selected_index;
}

/** Override the idle background color used by every cell. */
void egui_view_button_matrix_set_btn_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->btn_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_button_matrix_get_btn_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->btn_color;
}

/** Override the highlight color used for pressed and selected cells. */
void egui_view_button_matrix_set_btn_pressed_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->btn_pressed_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_button_matrix_get_btn_pressed_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->btn_pressed_color;
}

/** Override the shared text and icon tint color. */
void egui_view_button_matrix_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->text_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_button_matrix_get_text_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->text_color;
}

/** Change the spacing between neighboring cells. */
void egui_view_button_matrix_set_gap(egui_view_t *self, uint8_t gap)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->gap = gap;
    egui_view_invalidate(self);
}

uint8_t egui_view_button_matrix_get_gap(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->gap;
}

/** Change the rounded corner radius shared by all cells. */
void egui_view_button_matrix_set_corner_radius(egui_view_t *self, uint8_t radius)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->corner_radius = radius;
    egui_view_invalidate(self);
}

uint8_t egui_view_button_matrix_get_corner_radius(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->corner_radius;
}

/** Override the border color used by idle cells. */
void egui_view_button_matrix_set_border_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->border_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_button_matrix_get_border_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->border_color;
}

/** Override the font used for button labels. */
void egui_view_button_matrix_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    local->font = font;
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_button_matrix_get_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->font;
}

/** Borrow an optional icon array parallel to the current label array. */
void egui_view_button_matrix_set_icons(egui_view_t *self, const char **icons)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->icons == icons)
    {
        return;
    }

    local->icons = icons;
    egui_view_invalidate(self);
}

const char **egui_view_button_matrix_get_icons(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->icons;
}

/** Override the icon font used by cells that show glyph strings. */
void egui_view_button_matrix_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_button_matrix_get_icon_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->icon_font;
}

/** Change the vertical spacing between one icon and one label inside the same cell. */
void egui_view_button_matrix_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    if (local->icon_text_gap == gap)
    {
        return;
    }

    local->icon_text_gap = gap;
    egui_view_invalidate(self);
}

egui_dim_t egui_view_button_matrix_get_icon_text_gap(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    return local->icon_text_gap;
}

/** Draw every visible cell by recomputing equal-width geometry from the work region. */
void egui_view_button_matrix_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);

    if ((local->labels == NULL && local->icons == NULL) || local->btn_count == 0 || local->cols == 0)
    {
        return;
    }

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    /* Recompute equal cell geometry from the current inner work region. */
    egui_dim_t x = region.location.x;
    egui_dim_t y = region.location.y;
    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;

    uint8_t cols = local->cols;
    uint8_t rows = (local->btn_count + cols - 1) / cols;
    uint8_t gap = local->gap;

    egui_dim_t btn_w = (w - gap * (cols - 1)) / cols;
    egui_dim_t btn_h = (h - gap * (rows - 1)) / rows;

    const egui_font_t *font = local->font ? local->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    uint8_t i;
    for (i = 0; i < local->btn_count; i++)
    {
        uint8_t col = i % cols;
        uint8_t row = i / cols;
        const char *label = (local->labels != NULL) ? local->labels[i] : NULL;
        const char *icon = (local->icons != NULL) ? local->icons[i] : NULL;

        egui_dim_t btn_x = x + col * (btn_w + gap);
        egui_dim_t btn_y = y + row * (btn_h + gap);

        /* Highlight the actively pressed cell, or the stored selection when enabled. */
        egui_color_t bg_color = local->btn_color;
        if (i == local->pressed_index)
        {
            bg_color = local->btn_pressed_color;
        }
        else if (local->selection_enabled && i == local->selected_index)
        {
            bg_color = local->btn_pressed_color;
        }

        egui_dim_t r = local->corner_radius;
        if (r > btn_h / 2)
        {
            r = btn_h / 2;
        }
        if (r > btn_w / 2)
        {
            r = btn_w / 2;
        }

        // Draw rounded rectangle background
#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
        {
            egui_color_t color_light = egui_rgb_mix(bg_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t stops[2] = {
                    {.position = 0, .color = color_light},
                    {.position = 255, .color = bg_color},
            };
            egui_gradient_t grad = {
                    .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = stops,
            };
            egui_canvas_draw_round_rectangle_fill_gradient(canvas, btn_x, btn_y, btn_w, btn_h, r, &grad);
        }
#else
        egui_canvas_draw_round_rectangle_fill(canvas, btn_x, btn_y, btn_w, btn_h, r, bg_color, EGUI_ALPHA_100);
#endif

        // Draw border, emphasize selected/pressed state for better affordance.
        egui_dim_t border_w = 1;
        egui_color_t border_color = local->border_color;
        if (i == local->pressed_index || (local->selection_enabled && i == local->selected_index))
        {
            border_w = 2;
            border_color = EGUI_COLOR_WHITE;
        }
        egui_canvas_draw_round_rectangle(canvas, btn_x, btn_y, btn_w, btn_h, r, border_w, border_color, EGUI_ALPHA_100);

        // Draw centered content with per-cell clip to avoid overflow into neighboring cells.
        egui_region_t content_rect = {{btn_x + 4, btn_y + 4}, {btn_w > 8 ? (btn_w - 8) : btn_w, btn_h > 8 ? (btn_h - 8) : btn_h}};
        if (icon != NULL && icon[0] != '\0')
        {
            if (label != NULL && label[0] != '\0')
            {
                /* Stack icon above text and keep the combined block centered vertically. */
                const egui_font_t *icon_font = egui_view_button_matrix_resolve_icon_font(local, EGUI_MIN(content_rect.size.width, content_rect.size.height));
                if (icon_font == NULL)
                {
                    egui_view_button_matrix_draw_text_clipped(canvas, font, label, &content_rect, &content_rect, EGUI_ALIGN_CENTER, local->text_color,
                                                              EGUI_ALPHA_100);
                    continue;
                }

                egui_dim_t text_w = 0;
                egui_dim_t text_h = 0;
                egui_dim_t icon_area_h;
                egui_dim_t content_h;
                egui_dim_t content_y;
                egui_dim_t text_gap = local->icon_text_gap;
                egui_region_t icon_rect;
                egui_region_t text_rect;

                if (font->api != NULL && font->api->get_str_size != NULL)
                {
                    egui_font_get_str_size_with_canvas(font, canvas, label, 0, 0, &text_w, &text_h);
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

                icon_area_h = content_rect.size.height - text_h - text_gap;
                if (icon_area_h < content_rect.size.height / 2)
                {
                    icon_area_h = content_rect.size.height / 2;
                }
                if (icon_area_h > content_rect.size.height)
                {
                    icon_area_h = content_rect.size.height;
                }

                content_h = icon_area_h + ((text_h > 0) ? (text_h + text_gap) : 0);
                if (content_h > content_rect.size.height)
                {
                    content_h = content_rect.size.height;
                }
                content_y = content_rect.location.y + (content_rect.size.height - content_h) / 2;

                icon_rect.location.x = content_rect.location.x;
                icon_rect.location.y = content_y;
                icon_rect.size.width = content_rect.size.width;
                icon_rect.size.height = icon_area_h;

                text_rect.location.x = content_rect.location.x;
                text_rect.location.y = content_y + icon_area_h;
                text_rect.size.width = content_rect.size.width;
                text_rect.size.height = content_rect.location.y + content_rect.size.height - text_rect.location.y;

                egui_view_button_matrix_draw_text_clipped(canvas, icon_font, icon, &icon_rect, &content_rect, EGUI_ALIGN_CENTER, local->text_color,
                                                          EGUI_ALPHA_100);
                egui_view_button_matrix_draw_text_clipped(canvas, font, label, &text_rect, &content_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
            }
            else
            {
                const egui_font_t *icon_font = egui_view_button_matrix_resolve_icon_font(local, EGUI_MIN(content_rect.size.width, content_rect.size.height));
                if (icon_font != NULL)
                {
                    egui_view_button_matrix_draw_text_clipped(canvas, icon_font, icon, &content_rect, &content_rect, EGUI_ALIGN_CENTER, local->text_color,
                                                              EGUI_ALPHA_100);
                }
            }
        }
        else if (label != NULL && label[0] != '\0')
        {
            egui_view_button_matrix_draw_text_clipped(canvas, font, label, &content_rect, &content_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/** Translate pointer events into pressed and selected cell state plus click callbacks. */
static int egui_view_button_matrix_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);
    uint8_t dirty_indices[3];
    uint8_t dirty_count;
    uint8_t old_pressed;
    uint8_t old_selected;

    if (self->is_enable == false)
    {
        return 0;
    }

    if ((local->labels == NULL && local->icons == NULL) || local->btn_count == 0 || local->cols == 0)
    {
        return 0;
    }

    egui_dim_t w = self->region.size.width;
    egui_dim_t h = self->region.size.height;
    uint8_t cols = local->cols;
    uint8_t rows = (local->btn_count + cols - 1) / cols;
    uint8_t gap = local->gap;

    egui_dim_t btn_w = (w - gap * (cols - 1)) / cols;
    egui_dim_t btn_h = (h - gap * (rows - 1)) / rows;

    egui_dim_t touch_x = event->location.x - self->region_screen.location.x;
    egui_dim_t touch_y = event->location.y - self->region_screen.location.y;

    // Calculate which button was touched.
    uint8_t hit_index = EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE;
    if (btn_w > 0 && btn_h > 0)
    {
        // Account for gaps so touches between cells do not activate either neighbor.
        uint8_t col;
        uint8_t row;
        egui_dim_t cell_w = btn_w + gap;
        egui_dim_t cell_h = btn_h + gap;

        if (cell_w > 0 && cell_h > 0)
        {
            col = (uint8_t)(touch_x / cell_w);
            row = (uint8_t)(touch_y / cell_h);

            // Check whether the touch lands inside the actual cell body.
            egui_dim_t local_x = touch_x - col * cell_w;
            egui_dim_t local_y = touch_y - row * cell_h;

            if (col < cols && row < rows && local_x < btn_w && local_y < btn_h)
            {
                uint8_t idx = row * cols + col;
                if (idx < local->btn_count)
                {
                    hit_index = idx;
                }
            }
        }
    }

    if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        old_pressed = local->pressed_index;
        local->pressed_index = hit_index;
        dirty_indices[0] = old_pressed;
        dirty_indices[1] = local->pressed_index;
        egui_view_button_matrix_invalidate_indices(self, local, dirty_indices, 2);
    }
    else if (event->type == EGUI_MOTION_EVENT_ACTION_UP)
    {
        old_pressed = local->pressed_index;
        old_selected = local->selected_index;
        if (hit_index == local->pressed_index && hit_index != EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE)
        {
            if (local->selection_enabled)
            {
                local->selected_index = hit_index;
            }
            if (local->on_click)
            {
                local->on_click(self, hit_index);
            }
        }
        local->pressed_index = EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE;
        dirty_count = 0;
        dirty_indices[dirty_count++] = old_pressed;
        if (local->selection_enabled)
        {
            dirty_indices[dirty_count++] = old_selected;
            dirty_indices[dirty_count++] = local->selected_index;
        }
        egui_view_button_matrix_invalidate_indices(self, local, dirty_indices, dirty_count);
    }
    else if (event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
    {
        old_pressed = local->pressed_index;
        local->pressed_index = EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE;
        dirty_indices[0] = old_pressed;
        egui_view_button_matrix_invalidate_indices(self, local, dirty_indices, 1);
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static void egui_view_button_matrix_commit_index(egui_view_t *self, egui_view_button_matrix_t *local, uint8_t index)
{
    uint8_t old_selected = local->selected_index;
    uint8_t dirty_indices[2];
    uint8_t dirty_count = 0;

    if (index == EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE || index >= local->btn_count)
    {
        return;
    }

    if (local->selection_enabled)
    {
        local->selected_index = index;
    }

    if (local->on_click)
    {
        local->on_click(self, index);
    }

    if (local->selection_enabled)
    {
        dirty_indices[dirty_count++] = old_selected;
        dirty_indices[dirty_count++] = local->selected_index;
        egui_view_button_matrix_invalidate_indices(self, local, dirty_indices, dirty_count);
    }
}

static uint8_t egui_view_button_matrix_get_keyboard_index(egui_view_button_matrix_t *local)
{
    if (local->selected_index != EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE && local->selected_index < local->btn_count)
    {
        return local->selected_index;
    }
    if (local->pressed_index != EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE && local->pressed_index < local->btn_count)
    {
        return local->pressed_index;
    }
    return 0;
}

static int egui_view_button_matrix_move_keyboard_index(egui_view_t *self, egui_view_button_matrix_t *local, uint8_t key_code)
{
    uint8_t old_index;
    uint8_t new_index;
    uint8_t row;
    uint8_t dirty_indices[2];

    if (local->btn_count == 0 || local->cols == 0)
    {
        return 0;
    }

    old_index = egui_view_button_matrix_get_keyboard_index(local);
    new_index = old_index;
    row = (uint8_t)(old_index / local->cols);

    switch (key_code)
    {
    case EGUI_KEY_CODE_LEFT:
        if (old_index == 0)
        {
            return 0;
        }
        new_index = (uint8_t)(old_index - 1u);
        break;
    case EGUI_KEY_CODE_RIGHT:
        if (old_index + 1u >= local->btn_count)
        {
            return 0;
        }
        new_index = (uint8_t)(old_index + 1u);
        break;
    case EGUI_KEY_CODE_UP:
        if (row > 0)
        {
            new_index = (uint8_t)(old_index - local->cols);
        }
        else
        {
            return 0;
        }
        break;
    case EGUI_KEY_CODE_DOWN:
        if (old_index + local->cols < local->btn_count)
        {
            new_index = (uint8_t)(old_index + local->cols);
        }
        else
        {
            return 0;
        }
        break;
    default:
        return 0;
    }

    if (new_index >= local->btn_count)
    {
        return 0;
    }

    local->pressed_index = new_index;
    if (local->selection_enabled)
    {
        local->selected_index = new_index;
    }

    dirty_indices[0] = old_index;
    dirty_indices[1] = new_index;
    egui_view_button_matrix_invalidate_indices(self, local, dirty_indices, EGUI_ARRAY_SIZE(dirty_indices));
    return 1;
}

static int egui_view_button_matrix_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);

    if (self->is_enable == false || event == NULL || (local->labels == NULL && local->icons == NULL) || local->btn_count == 0 || local->cols == 0)
    {
        return 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_LEFT:
    case EGUI_KEY_CODE_RIGHT:
    case EGUI_KEY_CODE_UP:
    case EGUI_KEY_CODE_DOWN:
        if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
        {
            return 1;
        }
        if (event->type == EGUI_KEY_EVENT_ACTION_UP || event->type == EGUI_KEY_EVENT_ACTION_REPEAT)
        {
            return egui_view_button_matrix_move_keyboard_index(self, local, event->key_code);
        }
        return 1;
    case EGUI_KEY_CODE_ENTER:
        if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
        {
            return 1;
        }
        if (event->type == EGUI_KEY_EVENT_ACTION_UP)
        {
            egui_view_button_matrix_commit_index(self, local, egui_view_button_matrix_get_keyboard_index(local));
            return 1;
        }
        return 1;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

/* Use default view hooks for everything except button-matrix drawing and touch hit testing. */
const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_button_matrix_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_button_matrix_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_button_matrix_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_button_matrix_on_key_event,
#endif
};

/** Initialize default geometry, colors, and sentinel state for one button matrix. */
void egui_view_button_matrix_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_button_matrix_t);
    // call super init.
    egui_view_init(self, core);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_button_matrix_t);

    // init local data.
    local->labels = NULL;
    local->icons = NULL;
    local->btn_count = 0;
    local->cols = 4;
    local->gap = 2;
    local->pressed_index = EGUI_VIEW_BUTTON_MATRIX_PRESSED_NONE;
    local->selected_index = EGUI_VIEW_BUTTON_MATRIX_SELECTED_NONE;
    local->selection_enabled = 0;
    local->btn_color = EGUI_COLOR_DARK_GREY;
    local->btn_pressed_color = EGUI_COLOR_LIGHT_GREY;
    local->text_color = EGUI_COLOR_WHITE;
    local->border_color = EGUI_COLOR_WHITE;
    local->corner_radius = 4;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->icon_font = NULL;
    local->icon_text_gap = 2;
    local->on_click = NULL;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    self->is_focusable = true;
#endif

    egui_view_set_view_name(self, "egui_view_button_matrix");
}

/** Apply a parameter block that configures region, columns, and inter-cell gap. */
void egui_view_button_matrix_apply_params(egui_view_t *self, const egui_view_button_matrix_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_button_matrix_t);

    self->region = params->region;
    local->cols = params->cols;
    local->gap = params->gap;

    egui_view_invalidate(self);
}

/** Convenience helper that chains default initialization and parameter application. */
void egui_view_button_matrix_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_button_matrix_params_t *params)
{
    egui_view_button_matrix_init(self, core);
    egui_view_button_matrix_apply_params(self, params);
}
