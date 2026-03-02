#include <stdio.h>
#include <assert.h>

#include "egui_view_button.h"
#include "font/egui_font.h"
#include "egui_view.h" // Fixed include path
#include "style/egui_theme.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#include "shadow/egui_shadow.h"
#endif

void egui_view_button_on_draw(egui_view_t *self)
{
    // Only draw default background if no custom background is set.
    // When a custom background is set, egui_view_draw_background() has already drawn it
    // before on_draw is called.
    if (self->background == NULL)
    {
        egui_region_t region;
        egui_view_get_work_region(self, &region);

        egui_dim_t radius = EGUI_THEME_RADIUS_MD;
        if (radius > region.size.height / 2)
            radius = region.size.height / 2;

        /* Try theme style first */
        const egui_widget_style_desc_t *desc = egui_current_theme ? egui_current_theme->button : NULL;
        const egui_style_t *style = desc ? egui_style_get_current(desc, EGUI_PART_MAIN, self) : NULL;

        if (style)
        {
            egui_color_t bg = style->bg_color;
            egui_alpha_t alpha = (style->flags & EGUI_STYLE_PROP_ALPHA) ? style->bg_alpha : EGUI_ALPHA_100;
            if (style->flags & EGUI_STYLE_PROP_RADIUS)
            {
                radius = style->radius;
                if (radius > region.size.height / 2)
                    radius = region.size.height / 2;
            }

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
            /* Subtle gradient from style bg_color */
            egui_color_t color_top = egui_rgb_mix(bg, EGUI_COLOR_WHITE, 20);
            egui_color_t color_bottom = egui_rgb_mix(bg, EGUI_COLOR_BLACK, 20);
            egui_gradient_stop_t btn_stops[2] = {
                    {.position = 0, .color = color_top},
                    {.position = 255, .color = color_bottom},
            };
            egui_gradient_t btn_grad = {
                    .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                    .stop_count = 2,
                    .alpha = alpha,
                    .stops = btn_stops,
            };
            egui_canvas_draw_round_rectangle_fill_gradient(region.location.x, region.location.y, region.size.width, region.size.height, radius, &btn_grad);

            if ((style->flags & EGUI_STYLE_PROP_SHADOW) && style->shadow)
            {
                egui_view_set_shadow(self, style->shadow);
            }
#else
            egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, radius, bg, alpha);
#endif

            /* Update text color from style */
            if (style->flags & EGUI_STYLE_PROP_TEXT_COLOR)
            {
                EGUI_LOCAL_INIT(egui_view_label_t);
                local->color = style->text_color;
                local->alpha = style->text_alpha;
            }
        }
        else
        {
            /* Original fallback code */
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
            egui_color_t color_top, color_bottom;
            if (egui_view_get_enable(self))
            {
                if (self->is_pressed)
                {
                    color_top = EGUI_THEME_PRIMARY;
                    color_bottom = egui_rgb_mix(EGUI_THEME_PRIMARY_DARK, EGUI_COLOR_BLACK, 30);
                }
                else
                {
                    color_top = EGUI_THEME_PRIMARY_LIGHT;
                    color_bottom = EGUI_THEME_PRIMARY_DARK;
                }
            }
            else
            {
                color_top = EGUI_THEME_DISABLED;
                color_bottom = EGUI_THEME_DISABLED;
            }

            egui_gradient_stop_t btn_stops[2] = {
                    {.position = 0, .color = color_top},
                    {.position = 255, .color = color_bottom},
            };
            egui_gradient_t btn_grad = {
                    .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = btn_stops,
            };
            egui_canvas_draw_round_rectangle_fill_gradient(region.location.x, region.location.y, region.size.width, region.size.height, radius, &btn_grad);
#else
            egui_color_t color_bg;
            if (egui_view_get_enable(self))
            {
                if (self->is_pressed)
                {
                    color_bg = EGUI_THEME_PRIMARY_DARK;
                }
                else
                {
                    color_bg = EGUI_THEME_PRIMARY;
                }
            }
            else
            {
                color_bg = EGUI_THEME_DISABLED;
            }

            egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, radius, color_bg,
                                                  EGUI_ALPHA_100);
#endif
        }
    }

    // Draw text (Label logic)
    // Respect user-defined font color from egui_view_label_set_font_color().
    egui_view_label_on_draw(self);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_button_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_button_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_button_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_label_t);
    // call super init.
    egui_view_label_init(self);

    // Override API to support custom drawing
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_button_t);

    // init local data.
    local->align_type = EGUI_ALIGN_CENTER;
    egui_view_set_clickable(self, 1);

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        const egui_widget_style_desc_t *desc = egui_current_theme ? egui_current_theme->button : NULL;
        const egui_style_t *style = desc ? egui_style_get(desc, EGUI_PART_MAIN, EGUI_STATE_NORMAL) : NULL;
        if (style && (style->flags & EGUI_STYLE_PROP_SHADOW) && style->shadow)
        {
            egui_view_set_shadow(self, style->shadow);
        }
        else
        {
            /* Original fallback shadow */
            static const egui_shadow_t btn_shadow = {
                    .width = EGUI_THEME_SHADOW_WIDTH_MD,
                    .ofs_x = 0,
                    .ofs_y = EGUI_THEME_SHADOW_OFS_Y_MD,
                    .spread = 0,
                    .opa = EGUI_THEME_SHADOW_OPA,
                    .color = EGUI_COLOR_BLACK,
                    .corner_radius = EGUI_THEME_RADIUS_MD,
            };
            egui_view_set_shadow(self, &btn_shadow);
        }
    }
#endif

    egui_view_set_view_name(self, "egui_view_button");
}

void egui_view_button_init_with_params(egui_view_t *self, const egui_view_label_params_t *params)
{
    egui_view_button_init(self);
    egui_view_label_apply_params(self, params);
}
