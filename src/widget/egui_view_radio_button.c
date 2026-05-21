#include <stdio.h>
#include <assert.h>

#include "egui_view_radio_button.h"
#include "core/egui_core.h"
#include "egui_view_icon_font.h"
#include "egui_view_circle_dirty.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
#include "canvas/egui_canvas_gradient.h"
#endif

/**
 * @file egui_view_radio_button.c
 * @brief Radio button widget with optional grouping and single-selection behavior.
 *
 * Radio buttons look similar to checkboxes, but their state transition is
 * asymmetric: clicking selects the current button and, when grouped, clears any
 * previously selected peer in the same group.
 */

/** Return the effective label font, falling back to the default UI font. */
static const egui_font_t *egui_view_radio_button_get_text_font(const egui_view_radio_button_t *local)
{
    if (local->font != NULL)
    {
        return local->font;
    }

    return (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
}

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_radio_button_t);

/** Calculate the smallest dirty region that still fully covers the circular indicator. */
static uint8_t egui_view_radio_button_get_indicator_dirty_region(egui_view_t *self, egui_view_radio_button_t *local, egui_region_t *dirty_region)
{
    egui_region_t region;
    egui_dim_t size;
    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t outer_radius;

    if (dirty_region == NULL)
    {
        return 0;
    }

    egui_region_init_empty(dirty_region);
    if (self->region_screen.size.width <= 0 || self->region_screen.size.height <= 0)
    {
        return 0;
    }

    egui_view_get_work_region(self, &region);
    size = EGUI_MIN(region.size.width, region.size.height);
    outer_radius = size / 2 - 1;
    if (outer_radius <= 0)
    {
        return 0;
    }

    center_x = region.location.x + size / 2;
    center_y = region.location.y + size / 2;
    if (EGUI_VIEW_TEXT_VALID(local->text))
    {
        center_x = region.location.x + size / 2 + 1;
    }

    // Only the circular indicator needs refresh when selection changes.
    egui_view_circle_dirty_add_circle_region(dirty_region, center_x, center_y, outer_radius + 1, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD);
    return egui_region_is_empty(dirty_region) ? 0 : 1;
}

/** Replace the optional label text and redraw the whole widget row. */
void egui_view_radio_button_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    local->text = text;
    egui_view_invalidate(self);
}

const char *egui_view_radio_button_get_text(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    return local->text;
}

/** Override the label font used when drawing optional radio-button text. */
void egui_view_radio_button_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    local->font = font;
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_radio_button_get_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    return local->font;
}

/** Set the text color used for the optional label while enabled. */
void egui_view_radio_button_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    local->text_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_radio_button_get_text_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    return local->text_color;
}

/** Switch between inner-dot rendering and icon-font rendering for checked state. */
void egui_view_radio_button_set_mark_style(egui_view_t *self, egui_view_radio_button_mark_style_t style)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    if (local->mark_style == (uint8_t)style)
    {
        return;
    }

    local->mark_style = (uint8_t)style;
    egui_view_invalidate(self);
}

egui_view_radio_button_mark_style_t egui_view_radio_button_get_mark_style(egui_view_t *self)
{
    if (self == NULL)
    {
        return EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_DOT;
    }
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    return (egui_view_radio_button_mark_style_t)local->mark_style;
}

/** Replace the icon glyph used when the checked mark style is `ICON`. */
void egui_view_radio_button_set_mark_icon(egui_view_t *self, const char *icon)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    if (local->mark_icon == icon)
    {
        return;
    }

    local->mark_icon = icon;
    egui_view_invalidate(self);
}

const char *egui_view_radio_button_get_mark_icon(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    return local->mark_icon;
}

/** Override the icon font used for icon-style checked marks. */
void egui_view_radio_button_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_radio_button_get_icon_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    return local->icon_font;
}

/** Set the gap between the indicator ring and the optional label text. */
void egui_view_radio_button_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);

    if (gap < 0)
    {
        gap = 0;
    }

    if (local->text_gap == gap)
    {
        return;
    }

    local->text_gap = gap;
    egui_view_invalidate(self);
}

egui_dim_t egui_view_radio_button_get_icon_text_gap(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    return local->text_gap;
}

// ============== Radio Group ==============
/** Reset one radio-group list plus its optional selection callback. */
void egui_view_radio_group_init(egui_view_radio_group_t *group)
{
    // The group is only a small linked-list wrapper plus one optional callback.
    egui_slist_init(&group->buttons);
    group->on_changed = NULL;
}

/** Append one radio button to the group so it participates in mutual exclusion. */
void egui_view_radio_group_add(egui_view_radio_group_t *group, egui_view_t *button)
{
    // Append order becomes the logical index reported by the group callback.
    egui_view_radio_button_t *rb = (egui_view_radio_button_t *)button;
    rb->group = group;
    egui_slist_append(&group->buttons, &rb->group_node);
}

/** Register the callback fired when the group selects a new button index. */
void egui_view_radio_group_set_on_changed_listener(egui_view_radio_group_t *group, egui_view_on_radio_changed_listener_t listener)
{
    group->on_changed = listener;
}

egui_view_on_radio_changed_listener_t egui_view_radio_group_get_on_changed_listener(egui_view_radio_group_t *group)
{
    if (group == NULL)
    {
        return NULL;
    }
    return group->on_changed;
}

// ============== Radio Button ==============
/** Set the checked state, clear grouped peers, and invalidate only indicator regions when possible. */
void egui_view_radio_button_set_checked(egui_view_t *self, uint8_t is_checked)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    egui_region_t dirty_region;

    if (is_checked == local->is_checked)
    {
        return;
    }

    local->is_checked = is_checked;

    // Selecting one grouped radio button immediately clears every other selected peer.
    if (is_checked && local->group != NULL)
    {
        egui_snode_t *sn;
        int index = 0;
        int selected_index = 0;

        EGUI_SLIST_FOR_EACH_NODE(&local->group->buttons, sn)
        {
            egui_view_radio_button_t *rb = egui_slist_container_of(sn, egui_view_radio_button_t, group_node);
            if (rb != local && rb->is_checked)
            {
                rb->is_checked = 0;
                if (egui_view_radio_button_get_indicator_dirty_region(EGUI_VIEW_OF(rb), rb, &dirty_region))
                {
                    egui_view_invalidate_region(EGUI_VIEW_OF(rb), &dirty_region);
                }
                else
                {
                    egui_view_invalidate(EGUI_VIEW_OF(rb));
                }
            }
            if (rb == local)
            {
                selected_index = index;
            }
            index++;
        }

        // The group listener reports the selected button by append order.
        if (local->group->on_changed)
        {
            local->group->on_changed(self, selected_index);
        }
    }

    if (egui_view_radio_button_get_indicator_dirty_region(self, local, &dirty_region))
    {
        egui_view_invalidate_region(self, &dirty_region);
    }
    else
    {
        egui_view_invalidate(self);
    }
}

uint8_t egui_view_radio_button_get_checked(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    return local->is_checked;
}

egui_view_radio_group_t *egui_view_radio_button_get_group(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    return local->group;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
/** Select this button when a completed click lands on it. */
static int egui_view_radio_button_perform_click(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);

    // Radio buttons only transition toward "checked"; clicking the active one keeps it selected.
    if (!local->is_checked)
    {
        egui_view_radio_button_set_checked(self, 1);
    }
    return 1;
}
#endif

/** Draw the circular indicator using either a filled dot or an icon glyph. */
static void egui_view_radio_button_draw_indicator(egui_view_t *self, uint8_t use_icon_mark)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    EGUI_LOCAL_INIT(egui_view_radio_button_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    // Radio button is a circle. Use min(width, height) as diameter.
    egui_dim_t size = EGUI_MIN(region.size.width, region.size.height);
    egui_dim_t center_x = region.location.x + size / 2;
    egui_dim_t center_y = region.location.y + size / 2;
    egui_dim_t outer_radius = size / 2 - 1;
    egui_dim_t stroke = 2;

    if (EGUI_VIEW_TEXT_VALID(local->text))
    {
        center_x = region.location.x + size / 2 + 1;
    }

    // The outer ring is always visible so the control remains identifiable even when unchecked.
    egui_color_t outer_color = local->is_checked ? local->dot_color : local->circle_color;
    egui_color_t ring_color = egui_view_get_enable(self) ? outer_color : EGUI_THEME_DISABLED;

    egui_canvas_draw_circle_basic(canvas, center_x, center_y, outer_radius, stroke, ring_color, local->alpha);

    if (local->is_checked)
    {
        egui_color_t fill_color = egui_view_get_enable(self) ? local->dot_color : EGUI_THEME_DISABLED;
        if (use_icon_mark && EGUI_VIEW_ICON_TEXT_VALID(local->mark_icon))
        {
            egui_dim_t icon_size = size - 8;
            if (icon_size < 8)
            {
                icon_size = size;
            }

            egui_region_t icon_region = {
                    {center_x - icon_size / 2, center_y - icon_size / 2},
                    {icon_size, icon_size},
            };
            const egui_font_t *icon_font = EGUI_VIEW_ICON_FONT_RESOLVE(local->icon_font, icon_size, 18, 22);
            if (icon_font != NULL)
            {
                egui_canvas_draw_text_in_rect(canvas, icon_font, local->mark_icon, &icon_region, EGUI_ALIGN_CENTER, fill_color, local->alpha);
            }
        }
        else
        {
            // The default checked mark is a smaller filled dot centered inside the ring.
            egui_dim_t inner_radius = outer_radius * 5 / 10;
#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
            {
                egui_color_t dot_light = egui_rgb_mix(fill_color, EGUI_COLOR_WHITE, 80);
                egui_gradient_stop_t dot_stops[2] = {
                        {.position = 0, .color = dot_light},
                        {.position = 255, .color = fill_color},
                };
                egui_gradient_t dot_grad = {
                        .type = EGUI_GRADIENT_TYPE_RADIAL,
                        .stop_count = 2,
                        .alpha = local->alpha,
                        .stops = dot_stops,
                        .center_x = 0,
                        .center_y = 0,
                        .radius = inner_radius,
                };
                egui_canvas_draw_circle_fill_gradient(canvas, center_x, center_y, inner_radius, &dot_grad);
            }
#else
            egui_canvas_draw_circle_fill_basic(canvas, center_x, center_y, inner_radius, fill_color, local->alpha);
#endif
        }
    }
}

/** Draw the indicator first, then place the optional label to its right. */
void egui_view_radio_button_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_region_t region;
    egui_dim_t size;
    egui_dim_t center_x;
    egui_dim_t outer_radius;

    egui_view_radio_button_draw_indicator(self, local->mark_style == EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_ICON);

    if (EGUI_VIEW_TEXT_VALID(local->text))
    {
        // Label layout is derived from the indicator diameter so text starts after the ring.
        const egui_font_t *font = egui_view_radio_button_get_text_font(local);
        egui_color_t text_color = egui_view_get_enable(self) ? local->text_color : EGUI_THEME_DISABLED;

        egui_view_get_work_region(self, &region);
        size = EGUI_MIN(region.size.width, region.size.height);
        center_x = region.location.x + size / 2 + 1;
        outer_radius = size / 2 - 1;
        egui_region_t text_region;
        text_region.location.x = center_x + outer_radius + local->text_gap;
        text_region.location.y = region.location.y;
        text_region.size.width = region.location.x + region.size.width - text_region.location.x;
        text_region.size.height = region.size.height;
        if (text_region.size.width > 0 && font != NULL)
        {
            egui_canvas_draw_text_in_rect(canvas, font, local->text, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color, local->alpha);
        }
    }
}

/** API table for the radio-button widget. */
const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_radio_button_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_radio_button_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .perform_click = egui_view_radio_button_perform_click,
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

/** Initialize the radio button with clickable selection behavior and stock defaults. */
void egui_view_radio_button_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_radio_button_t);
    // Initialize the base view before installing radio-button-specific behavior.
    egui_view_init(self, core);
    // Replace generic draw/click callbacks with the radio button API table.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_radio_button_t);

    // Defaults produce a standalone radio button; grouping is opt-in via egui_view_radio_group_add.
    local->group_node.next = NULL;
    local->group = NULL;
    local->is_checked = false;
    local->alpha = EGUI_ALPHA_100;
    local->circle_color = EGUI_THEME_BORDER;
    local->dot_color = EGUI_THEME_PRIMARY;
    local->text = NULL;
    local->font = NULL;
    local->text_color = EGUI_THEME_TEXT;
    local->text_gap = 6;
    local->mark_style = EGUI_VIEW_RADIO_BUTTON_MARK_STYLE_DOT;
    local->mark_icon = EGUI_ICON_MS_DONE;
    local->icon_font = NULL;
    self->is_clickable = true;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    self->is_focusable = true;
#endif

    egui_view_set_view_name(self, "egui_view_radio_button");
}

/** Apply geometry, initial checked state, and optional label from one parameter block. */
void egui_view_radio_button_apply_params(egui_view_t *self, const egui_view_radio_button_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_radio_button_t);

    self->region = params->region;

    local->is_checked = params->is_checked;
    local->text = params->text;

    egui_view_invalidate(self);
}

/** Convenience helper that initializes the radio button before applying params. */
void egui_view_radio_button_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_radio_button_params_t *params)
{
    egui_view_radio_button_init(self, core);
    egui_view_radio_button_apply_params(self, params);
}
