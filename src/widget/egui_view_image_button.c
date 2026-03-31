#include <stdio.h>
#include <assert.h>

#include "egui_view_image_button.h"
#include "egui_view_icon_font.h"
#include "core/egui_canvas_gradient.h"
#include "resource/egui_resource.h"
#include "style/egui_theme.h"

static const egui_font_t *egui_view_image_button_get_icon_font(egui_view_image_button_t *local, egui_dim_t area_size)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    return egui_view_icon_font_get_auto(area_size, 20, 26);
}

static void egui_view_image_button_draw_content(egui_view_image_button_t *local, const egui_region_t *region, egui_color_t color, egui_alpha_t alpha)
{
    const char *icon = local->icon;
    const char *text = local->text;
    const egui_font_t *font = (local->font != NULL) ? local->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_region_t content_region = *region;

    if (content_region.size.width > 8)
    {
        content_region.location.x += 4;
        content_region.size.width -= 8;
    }
    if (content_region.size.height > 8)
    {
        content_region.location.y += 4;
        content_region.size.height -= 8;
    }

    if ((icon == NULL || icon[0] == '\0') && (text == NULL || text[0] == '\0'))
    {
        return;
    }

    if (icon != NULL && icon[0] != '\0')
    {
        if (text != NULL && text[0] != '\0')
        {
            if (egui_view_image_button_get_icon_font(local, EGUI_MIN(content_region.size.width, content_region.size.height)) == NULL)
            {
                egui_canvas_draw_text_in_rect(font, text, &content_region, EGUI_ALIGN_CENTER, color, alpha);
                return;
            }

            egui_dim_t text_w = 0;
            egui_dim_t text_h = 0;
            egui_dim_t icon_area_h;
            egui_dim_t content_h;
            egui_dim_t content_y;
            egui_dim_t text_gap = local->icon_text_gap;
            egui_region_t icon_region;
            egui_region_t text_region;

            if (font != NULL && font->api != NULL && font->api->get_str_size != NULL)
            {
                font->api->get_str_size(font, text, 0, 0, &text_w, &text_h);
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

            icon_area_h = content_region.size.height - text_h - text_gap;
            if (icon_area_h < content_region.size.height / 2)
            {
                icon_area_h = content_region.size.height / 2;
            }
            if (icon_area_h > content_region.size.height)
            {
                icon_area_h = content_region.size.height;
            }

            content_h = icon_area_h + ((text_h > 0) ? (text_h + text_gap) : 0);
            if (content_h > content_region.size.height)
            {
                content_h = content_region.size.height;
            }
            content_y = content_region.location.y + (content_region.size.height - content_h) / 2;

            icon_region.location.x = content_region.location.x;
            icon_region.location.y = content_y;
            icon_region.size.width = content_region.size.width;
            icon_region.size.height = icon_area_h;

            text_region.location.x = content_region.location.x;
            text_region.location.y = content_y + icon_area_h;
            text_region.size.width = content_region.size.width;
            text_region.size.height = content_region.location.y + content_region.size.height - text_region.location.y;

            egui_canvas_draw_text_in_rect(egui_view_image_button_get_icon_font(local, EGUI_MIN(icon_region.size.width, icon_region.size.height)), icon,
                                          &icon_region, EGUI_ALIGN_CENTER, color, alpha);
            egui_canvas_draw_text_in_rect(font, text, &text_region, EGUI_ALIGN_CENTER, color, alpha);
        }
        else
        {
            const egui_font_t *icon_font = egui_view_image_button_get_icon_font(local, EGUI_MIN(content_region.size.width, content_region.size.height));
            if (icon_font != NULL)
            {
                egui_canvas_draw_text_in_rect(icon_font, icon, &content_region, EGUI_ALIGN_CENTER, color, alpha);
            }
        }
    }
    else if (text != NULL && text[0] != '\0')
    {
        egui_canvas_draw_text_in_rect(font, text, &content_region, EGUI_ALIGN_CENTER, color, alpha);
    }
}

void egui_view_image_button_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_image_button_t);
    egui_region_t region;
    egui_view_get_work_region(self, &region);

    if (local->base.image == NULL && ((local->icon != NULL && local->icon[0] != '\0') || (local->text != NULL && local->text[0] != '\0')))
    {
        egui_color_t bg_color = egui_view_get_enable(self) ? EGUI_THEME_SURFACE_VARIANT : EGUI_THEME_DISABLED;
        egui_color_t border_color = egui_view_get_enable(self) ? EGUI_THEME_BORDER : EGUI_THEME_DISABLED;
        egui_dim_t radius = EGUI_MIN(region.size.width, region.size.height) / 4;

        if (radius < EGUI_THEME_RADIUS_SM)
        {
            radius = EGUI_THEME_RADIUS_SM;
        }
        if (radius > EGUI_THEME_RADIUS_LG)
        {
            radius = EGUI_THEME_RADIUS_LG;
        }

        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, radius, bg_color, EGUI_ALPHA_100);
        egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, radius, 1, border_color, EGUI_ALPHA_100);
    }

    // Draw the image using parent's draw
    egui_view_image_on_draw(self);

    if ((local->icon != NULL && local->icon[0] != '\0') || (local->text != NULL && local->text[0] != '\0'))
    {
        egui_color_t content_color = local->content_color;
        egui_alpha_t content_alpha = local->content_alpha;

        if (!egui_view_get_enable(self))
        {
            content_color = EGUI_THEME_DISABLED;
        }

        egui_view_image_button_draw_content(local, &region, content_color, content_alpha);
    }

    // If pressed, draw a semi-transparent overlay for visual feedback
    if (self->is_pressed)
    {
        egui_dim_t radius = EGUI_MIN(region.size.width, region.size.height) / 4;

        if (radius < EGUI_THEME_RADIUS_SM)
        {
            radius = EGUI_THEME_RADIUS_SM;
        }
        if (radius > EGUI_THEME_RADIUS_LG)
        {
            radius = EGUI_THEME_RADIUS_LG;
        }
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t color_light = egui_rgb_mix(EGUI_THEME_PRESS_OVERLAY, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t stops[2] = {
                    {.position = 0, .color = color_light},
                    {.position = 255, .color = EGUI_THEME_PRESS_OVERLAY},
            };
            egui_gradient_t grad = {
                    .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                    .stop_count = 2,
                    .alpha = EGUI_THEME_PRESS_OVERLAY_ALPHA,
                    .stops = stops,
            };
            egui_canvas_draw_round_rectangle_fill_gradient(region.location.x, region.location.y, region.size.width, region.size.height, radius, &grad);
        }
#else
        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, radius, EGUI_THEME_PRESS_OVERLAY,
                                              EGUI_THEME_PRESS_OVERLAY_ALPHA);
#endif
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_image_button_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_image_button_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_image_button_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_image_button_t);
    // call super init.
    egui_view_image_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_image_button_t);

    // init local data.
    local->icon = NULL;
    local->text = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->icon_font = NULL;
    local->content_color = EGUI_THEME_TEXT_PRIMARY;
    local->content_alpha = EGUI_ALPHA_100;
    local->icon_text_gap = 2;
    egui_view_set_view_name(self, "egui_view_image_button");
}

void egui_view_image_button_init_with_params(egui_view_t *self, const egui_view_image_params_t *params)
{
    egui_view_image_button_init(self);
    egui_view_image_apply_params(self, params);
}

void egui_view_image_button_set_icon(egui_view_t *self, const char *icon)
{
    EGUI_LOCAL_INIT(egui_view_image_button_t);

    if (local->icon == icon)
    {
        return;
    }

    local->icon = icon;
    egui_view_invalidate(self);
}

void egui_view_image_button_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_image_button_t);

    if (local->text == text)
    {
        return;
    }

    local->text = text;
    egui_view_invalidate(self);
}

void egui_view_image_button_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_image_button_t);

    if (local->font == font)
    {
        return;
    }

    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_image_button_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_image_button_t);

    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

void egui_view_image_button_set_content_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_view_image_button_t);

    if (local->content_color.full == color.full && local->content_alpha == alpha)
    {
        return;
    }

    local->content_color = color;
    local->content_alpha = alpha;
    egui_view_invalidate(self);
}

void egui_view_image_button_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_image_button_t);

    if (local->icon_text_gap == gap)
    {
        return;
    }

    local->icon_text_gap = gap;
    egui_view_invalidate(self);
}
