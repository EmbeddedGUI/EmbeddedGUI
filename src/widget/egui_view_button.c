#include <stdio.h>
#include <assert.h>

#include "egui_view_button.h"
#include "egui_view_icon_font.h"
#include "font/egui_font.h"
#include "egui_view.h" // Fixed include path
#include "resource/egui_resource.h"
#include "style/egui_theme.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#include "shadow/egui_shadow.h"
#endif

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_button_t);

static const egui_font_t *egui_view_button_get_text_font(const egui_view_button_t *local)
{
    if (local->base.font != NULL)
    {
        return local->base.font;
    }

    return (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
}

static void egui_view_button_draw_content(egui_view_button_t *local, const egui_region_t *region, egui_color_t text_color, egui_alpha_t text_alpha)
{
    egui_view_label_t *label = &local->base;
    const egui_font_t *text_font = egui_view_button_get_text_font(local);
    const char *icon = local->icon;
    const char *text = label->text;
    egui_region_t draw_region = *region;

    if (!EGUI_VIEW_ICON_TEXT_VALID(icon) && !EGUI_VIEW_TEXT_VALID(text))
    {
        return;
    }

    if (!EGUI_VIEW_ICON_TEXT_VALID(icon))
    {
        if (text_font != NULL)
        {
            egui_canvas_draw_text_in_rect_with_line_space(text_font, text, &draw_region, label->align_type, label->line_space, text_color, text_alpha);
        }
        return;
    }

    if (!EGUI_VIEW_TEXT_VALID(text))
    {
        const egui_font_t *icon_font = EGUI_VIEW_ICON_FONT_RESOLVE(local->icon_font, EGUI_MIN(region->size.width, region->size.height), 20, 26);
        if (icon_font != NULL)
        {
            egui_canvas_draw_text_in_rect(icon_font, icon, &draw_region, label->align_type, text_color, text_alpha);
        }
        return;
    }

    {
        const egui_font_t *icon_font = EGUI_VIEW_ICON_FONT_RESOLVE(local->icon_font, region->size.height, 20, 26);
        egui_dim_t icon_width = 0;
        egui_dim_t icon_height = 0;
        egui_dim_t text_width = 0;
        egui_dim_t text_height = 0;
        egui_dim_t content_width;
        egui_dim_t start_x;
        egui_dim_t gap = local->icon_text_gap;
        egui_region_t icon_region;
        egui_region_t text_region;

        if (icon_font == NULL)
        {
            if (text_font != NULL)
            {
                egui_canvas_draw_text_in_rect_with_line_space(text_font, text, &draw_region, label->align_type, label->line_space, text_color, text_alpha);
            }
            return;
        }

        if (icon_font->api != NULL && icon_font->api->get_str_size != NULL)
        {
            icon_font->api->get_str_size(icon_font, icon, 0, 0, &icon_width, &icon_height);
        }
        if (text_font != NULL && text_font->api != NULL && text_font->api->get_str_size != NULL)
        {
            text_font->api->get_str_size(text_font, text, 0, 0, &text_width, &text_height);
        }
        (void)icon_height;
        (void)text_height;

        if (icon_width <= 0)
        {
            icon_width = EGUI_MIN(region->size.height, 20);
        }
        if (text_width <= 0)
        {
            gap = 0;
        }
        if (gap < 0)
        {
            gap = 0;
        }

        content_width = icon_width + gap + text_width;
        if (content_width < 0)
        {
            content_width = 0;
        }

        switch (label->align_type & EGUI_ALIGN_HMASK)
        {
        case EGUI_ALIGN_LEFT:
            start_x = region->location.x;
            break;
        case EGUI_ALIGN_RIGHT:
            start_x = region->location.x + region->size.width - content_width;
            break;
        default:
            start_x = region->location.x + (region->size.width - content_width) / 2;
            break;
        }

        if (start_x < region->location.x)
        {
            start_x = region->location.x;
        }

        icon_region.location.x = start_x;
        icon_region.location.y = region->location.y;
        icon_region.size.width = EGUI_MIN(icon_width, region->size.width);
        icon_region.size.height = region->size.height;

        text_region.location.x = icon_region.location.x + icon_region.size.width + gap;
        text_region.location.y = region->location.y;
        text_region.size.width = region->location.x + region->size.width - text_region.location.x;
        text_region.size.height = region->size.height;

        egui_canvas_draw_text_in_rect(icon_font, icon, &icon_region, EGUI_ALIGN_CENTER, text_color, text_alpha);
        if (text_region.size.width > 0 && text_width > 0 && text_font != NULL)
        {
            egui_canvas_draw_text_in_rect(text_font, text, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color, text_alpha);
        }
    }
}

static void egui_view_button_draw_frame(egui_view_t *self, egui_view_button_t *local, egui_color_t *text_color, egui_alpha_t *text_alpha)
{
    egui_view_label_t *label = &local->base;

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
                label->color = style->text_color;
                label->alpha = style->text_alpha;
                *text_color = style->text_color;
                *text_alpha = style->text_alpha;
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
}

void egui_view_button_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_button_t);
    egui_view_label_t *label = &local->base;
    egui_color_t text_color = label->color;
    egui_alpha_t text_alpha = label->alpha;
    egui_region_t text_region;

    egui_view_button_draw_frame(self, local, &text_color, &text_alpha);
    egui_view_get_work_region(self, &text_region);
    egui_view_button_draw_content(local, &text_region, text_color, text_alpha);
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
    EGUI_INIT_LOCAL(egui_view_button_t);
    egui_view_label_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_button_t);

    local->icon = NULL;
    local->icon_font = NULL;
    local->icon_text_gap = 6;
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

void egui_view_button_apply_params(egui_view_t *self, const egui_view_label_params_t *params)
{
    egui_view_label_apply_params(self, params);
}

void egui_view_button_init_with_params(egui_view_t *self, const egui_view_label_params_t *params)
{
    egui_view_button_init(self);
    egui_view_button_apply_params(self, params);
}

void egui_view_button_set_icon(egui_view_t *self, const char *icon)
{
    EGUI_LOCAL_INIT(egui_view_button_t);

    if (local->icon == icon)
    {
        return;
    }

    local->icon = icon;
    egui_view_invalidate(self);
}

void egui_view_button_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_button_t);

    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

void egui_view_button_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_button_t);

    if (local->icon_text_gap == gap)
    {
        return;
    }

    local->icon_text_gap = gap;
    egui_view_invalidate(self);
}
