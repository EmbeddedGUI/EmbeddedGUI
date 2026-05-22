#include <stdio.h>
#include <assert.h>

#include "egui_view_card.h"
#include "core/egui_core.h"
#include "style/egui_theme.h"

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
#include "canvas/egui_canvas_gradient.h"
#include "shadow/egui_shadow.h"
#endif

/**
 * @file egui_view_card.c
 * @brief Card container that combines group layout with themed surface chrome.
 *
 * Reading tip:
 * - child management and layout stay delegated to `egui_view_group`,
 * - this file mainly resolves style values and paints the rounded surface,
 * - shadow setup is synced from theme style so card elevation can follow state.
 */

/**
 * @brief Override the rounded corner radius used by the card surface.
 */
void egui_view_card_set_corner_radius(egui_view_t *self, egui_dim_t radius)
{
    EGUI_LOCAL_INIT(egui_view_card_t);
    local->corner_radius = radius;
    egui_view_invalidate(self);
}

egui_dim_t egui_view_card_get_corner_radius(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_card_t);
    return local->corner_radius;
}

/**
 * @brief Override the card border thickness and border color together.
 */
void egui_view_card_set_border(egui_view_t *self, egui_dim_t width, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_card_t);
    local->border_width = width;
    local->border_color = color;
    egui_view_invalidate(self);
}

egui_dim_t egui_view_card_get_border_width(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_card_t);
    return local->border_width;
}

egui_color_t egui_view_card_get_border_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_card_t);
    return local->border_color;
}

/**
 * @brief Replace the themed background fill with a caller-owned custom color.
 *
 * After this call the draw path keeps using the local background fields
 * until
 * the widget is reinitialized or another customization policy is applied.
 */
void egui_view_card_set_bg_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_view_card_t);
    local->bg_color = color;
    local->bg_alpha = alpha;
    local->bg_color_custom = 1;
    egui_view_invalidate(self);
}

egui_color_t egui_view_card_get_bg_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_card_t);
    return local->bg_color;
}

egui_alpha_t egui_view_card_get_bg_alpha(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_card_t);
    return local->bg_alpha;
}

uint8_t egui_view_card_get_bg_color_custom(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_card_t);
    return local->bg_color_custom;
}

/**
 * @brief Add one child to the internal group content tree.
 */
void egui_view_card_add_child(egui_view_t *self, egui_view_t *child)
{
    egui_view_group_add_child(self, child);
}

/**
 * @brief Forward simple horizontal or vertical layout to the group helper.
 */
void egui_view_card_layout_childs(egui_view_t *self, uint8_t is_horizontal, uint8_t align_type)
{
    egui_view_group_layout_childs(self, is_horizontal, 0, 0, align_type);
}

/**
 * @brief Draw the card background and border, then sync themed shadow state.
 *
 * The card reads the current theme style every draw so state-dependent style
 * changes can update radius, border, alpha, and shadow without extra plumbing.
 */
void egui_view_card_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_card_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    const egui_theme_t *theme = egui_theme_get(egui_view_get_core(self));

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    const egui_widget_style_desc_t *desc = theme ? theme->card : NULL;
    const egui_style_t *style = desc ? egui_style_get_current(desc, EGUI_PART_MAIN, self) : NULL;

    egui_color_t bg = (style && !local->bg_color_custom) ? style->bg_color : local->bg_color;
    egui_alpha_t alpha = (style && !local->bg_color_custom && (style->flags & EGUI_STYLE_PROP_ALPHA)) ? style->bg_alpha : local->bg_alpha;
    egui_dim_t radius = (style && (style->flags & EGUI_STYLE_PROP_RADIUS)) ? style->radius : local->corner_radius;
    egui_color_t border_color = (style && (style->flags & EGUI_STYLE_PROP_BORDER)) ? style->border_color : local->border_color;
    egui_dim_t border_width = (style && (style->flags & EGUI_STYLE_PROP_BORDER)) ? style->border_width : local->border_width;

    // Draw filled background
#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
    {
        egui_canvas_draw_round_rectangle_fill(canvas, region.location.x, region.location.y, region.size.width, region.size.height, radius, bg, alpha);
    }
#else
    egui_canvas_draw_round_rectangle_fill(canvas, region.location.x, region.location.y, region.size.width, region.size.height, radius, bg, alpha);
#endif

    // Draw border
    if (border_width > 0)
    {
        egui_canvas_draw_round_rectangle(canvas, region.location.x, region.location.y, region.size.width, region.size.height, radius, border_width,
                                         border_color, EGUI_ALPHA_100);
    }

    // Update shadow from style
    if (style && (style->flags & EGUI_STYLE_PROP_SHADOW) && style->shadow)
    {
        egui_view_set_shadow(self, style->shadow);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_card_t) = {
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = egui_view_group_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_group_calculate_layout,
        .request_layout = egui_view_group_request_layout,
        .draw = egui_view_group_draw,
        .on_attach_to_window = egui_view_group_on_attach_to_window,
        .on_draw = egui_view_card_on_draw,
        .on_detach_from_window = egui_view_group_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_group_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

/**
 * @brief Initialize the card as a themed group container with surface defaults.
 */
void egui_view_card_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_card_t);
    // call super init.
    egui_view_group_init(self, core);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_card_t);

    // init local data.
    local->corner_radius = EGUI_THEME_RADIUS_LG;
    local->border_width = EGUI_THEME_STROKE_WIDTH;
    local->border_color = EGUI_THEME_BORDER;
    local->bg_color = EGUI_THEME_SURFACE;
    local->bg_alpha = EGUI_ALPHA_100;
    local->bg_color_custom = 0;

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
    {
        const egui_theme_t *theme = egui_theme_get(egui_view_get_core(self));
        const egui_widget_style_desc_t *desc = theme ? theme->card : NULL;
        const egui_style_t *style = desc ? egui_style_get(desc, EGUI_PART_MAIN, EGUI_STATE_NORMAL) : NULL;
        if (style && (style->flags & EGUI_STYLE_PROP_SHADOW) && style->shadow)
        {
            egui_view_set_shadow(self, style->shadow);
        }
        else
        {
            /* Original fallback shadow */
            static const egui_shadow_t card_shadow = {
                    .width = EGUI_THEME_SHADOW_WIDTH_LG,
                    .ofs_x = 0,
                    .ofs_y = EGUI_THEME_SHADOW_OFS_Y_LG,
                    .spread = 0,
                    .opa = EGUI_THEME_SHADOW_OPA,
                    .color = EGUI_COLOR_BLACK,
                    .corner_radius = EGUI_THEME_RADIUS_LG,
            };
            egui_view_set_shadow(self, &card_shadow);
        }
    }
#endif

    egui_view_set_view_name(self, "egui_view_card");
}

/**
 * @brief Apply card geometry fields from one parameter block.
 */
void egui_view_card_apply_params(egui_view_t *self, const egui_view_card_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_card_t);

    self->region = params->region;

    local->corner_radius = params->corner_radius;

    egui_view_invalidate(self);
}

/**
 * @brief Convenience initializer that chains card init and params.
 */
void egui_view_card_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_card_params_t *params)
{
    egui_view_card_init(self, core);
    egui_view_card_apply_params(self, params);
}
