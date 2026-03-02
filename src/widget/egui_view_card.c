#include <stdio.h>
#include <assert.h>

#include "egui_view_card.h"
#include "style/egui_theme.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#include "shadow/egui_shadow.h"
#endif

void egui_view_card_set_corner_radius(egui_view_t *self, egui_dim_t radius)
{
    EGUI_LOCAL_INIT(egui_view_card_t);
    local->corner_radius = radius;
    egui_view_invalidate(self);
}

void egui_view_card_set_border(egui_view_t *self, egui_dim_t width, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_card_t);
    local->border_width = width;
    local->border_color = color;
    egui_view_invalidate(self);
}

void egui_view_card_set_bg_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_view_card_t);
    local->bg_color = color;
    local->bg_alpha = alpha;
    local->bg_color_custom = 1;
    egui_view_invalidate(self);
}

void egui_view_card_add_child(egui_view_t *self, egui_view_t *child)
{
    egui_view_group_add_child(self, child);
}

void egui_view_card_layout_childs(egui_view_t *self, uint8_t is_horizontal, uint8_t align_type)
{
    egui_view_group_layout_childs(self, is_horizontal, 0, 0, align_type);
}

void egui_view_card_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_card_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    const egui_widget_style_desc_t *desc = egui_current_theme ? egui_current_theme->card : NULL;
    const egui_style_t *style = desc ? egui_style_get_current(desc, EGUI_PART_MAIN, self) : NULL;

    egui_color_t bg = (style && !local->bg_color_custom) ? style->bg_color : local->bg_color;
    egui_alpha_t alpha = (style && !local->bg_color_custom && (style->flags & EGUI_STYLE_PROP_ALPHA)) ? style->bg_alpha : local->bg_alpha;
    egui_dim_t radius = (style && (style->flags & EGUI_STYLE_PROP_RADIUS)) ? style->radius : local->corner_radius;
    egui_color_t border_color = (style && (style->flags & EGUI_STYLE_PROP_BORDER)) ? style->border_color : local->border_color;
    egui_dim_t border_width = (style && (style->flags & EGUI_STYLE_PROP_BORDER)) ? style->border_width : local->border_width;

    // Draw filled background
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, radius, bg, alpha);
    }
#else
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, radius, bg, alpha);
#endif

    // Draw border
    if (border_width > 0)
    {
        egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, radius, border_width, border_color,
                                         EGUI_ALPHA_100);
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

void egui_view_card_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_card_t);
    // call super init.
    egui_view_group_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_card_t);

    // init local data.
    local->corner_radius = EGUI_THEME_RADIUS_LG;
    local->border_width = EGUI_THEME_STROKE_WIDTH;
    local->border_color = EGUI_THEME_BORDER;
    local->bg_color = EGUI_THEME_SURFACE;
    local->bg_alpha = EGUI_ALPHA_100;
    local->bg_color_custom = 0;

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        const egui_widget_style_desc_t *desc = egui_current_theme ? egui_current_theme->card : NULL;
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

void egui_view_card_apply_params(egui_view_t *self, const egui_view_card_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_card_t);

    self->region = params->region;

    local->corner_radius = params->corner_radius;

    egui_view_invalidate(self);
}

void egui_view_card_init_with_params(egui_view_t *self, const egui_view_card_params_t *params)
{
    egui_view_card_init(self);
    egui_view_card_apply_params(self, params);
}
