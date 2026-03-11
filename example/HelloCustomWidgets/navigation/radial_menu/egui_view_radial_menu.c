#include <stdlib.h>

#include "egui_view_radial_menu.h"
#include "widget/egui_view_group.h"
#include "utils/egui_fixmath.h"

typedef struct egui_view_radial_menu_layout egui_view_radial_menu_layout_t;
struct egui_view_radial_menu_layout
{
    egui_region_t region;
    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t outer_radius;
    egui_dim_t inner_radius;
    egui_dim_t center_radius;
    egui_dim_t label_radius;
    egui_dim_t label_width;
    egui_dim_t label_height;
    int16_t start_angle;
    int16_t step_angle;
    uint8_t count;
};

static uint8_t egui_view_radial_menu_clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_RADIAL_MENU_MAX_ITEMS)
    {
        return EGUI_VIEW_RADIAL_MENU_MAX_ITEMS;
    }
    return count;
}

static int16_t egui_view_radial_menu_normalize_angle(int16_t angle)
{
    while (angle < 0)
    {
        angle += 360;
    }
    while (angle >= 360)
    {
        angle -= 360;
    }
    return angle;
}

static int16_t egui_view_radial_menu_atan2_deg(int32_t dy, int32_t dx)
{
    if (dx == 0 && dy == 0)
    {
        return 0;
    }
    if (dx == 0)
    {
        return (dy > 0) ? 90 : 270;
    }
    if (dy == 0)
    {
        return (dx > 0) ? 0 : 180;
    }

    int32_t abs_y = (dy < 0) ? -dy : dy;
    int32_t abs_x = (dx < 0) ? -dx : dx;
    int16_t angle;

    if (abs_x >= abs_y)
    {
        angle = (int16_t)((int32_t)45 * abs_y / abs_x);
    }
    else
    {
        angle = 90 - (int16_t)((int32_t)45 * abs_x / abs_y);
    }

    if (dx < 0)
    {
        angle = 180 - angle;
    }
    if (dy < 0)
    {
        angle = 360 - angle;
    }

    return angle % 360;
}

static int egui_view_radial_menu_build_layout(egui_view_t *self, egui_view_radial_menu_t *local, egui_view_radial_menu_layout_t *layout)
{
    egui_view_get_work_region(self, &layout->region);
    if (layout->region.size.width <= 0 || layout->region.size.height <= 0)
    {
        return 0;
    }

    layout->count = egui_view_radial_menu_clamp_count(local->item_count);
    layout->center_x = layout->region.location.x + layout->region.size.width / 2;
    layout->center_y = layout->region.location.y + layout->region.size.height / 2;
    layout->outer_radius = EGUI_MIN(layout->region.size.width, layout->region.size.height) / 2 - 4;
    if (layout->outer_radius < 20)
    {
        return 0;
    }

    layout->center_radius = EGUI_MAX(layout->outer_radius / 3, 18);
    layout->center_radius = EGUI_MIN(layout->center_radius, layout->outer_radius - 10);
    layout->inner_radius = layout->center_radius + EGUI_MAX(layout->outer_radius / 10, 8);
    if (layout->inner_radius >= layout->outer_radius - 4)
    {
        layout->inner_radius = layout->center_radius + 4;
    }
    if (layout->inner_radius >= layout->outer_radius)
    {
        layout->inner_radius = layout->outer_radius - 2;
    }

    layout->label_radius = layout->inner_radius + ((layout->outer_radius - layout->inner_radius) * 4) / 5;
    layout->label_width = EGUI_MAX(layout->region.size.width / 4, 32);
    layout->label_width = EGUI_MIN(layout->label_width, layout->region.size.width / 3);
    layout->label_height = EGUI_MAX(layout->region.size.height / 9, 16);
    layout->label_height = EGUI_MIN(layout->label_height, 22);

    /*
     * Keep label chips away from the view edge to avoid clipping (e.g. "Pause" on the right side).
     * We reserve extra pixels for the chip background (+4) and the per-item radius bump (+4).
     */
    {
        egui_dim_t edge_pad = 6;
        egui_dim_t safe_x = layout->region.size.width / 2 - (layout->label_width + 4) / 2 - edge_pad;
        egui_dim_t safe_y = layout->region.size.height / 2 - (layout->label_height + 2) / 2 - edge_pad;
        egui_dim_t safe_radius = EGUI_MIN(safe_x, safe_y);

        if (safe_radius > 12 && layout->label_radius > safe_radius - 4)
        {
            layout->label_radius = safe_radius - 4;
        }
        if (layout->label_radius < layout->inner_radius + 6)
        {
            layout->label_radius = layout->inner_radius + 6;
        }
    }

    if (layout->count > 0)
    {
        layout->step_angle = 360 / layout->count;
        layout->start_angle = 270 - layout->step_angle / 2;
    }
    else
    {
        layout->step_angle = 360;
        layout->start_angle = 270;
    }

    return 1;
}

static void egui_view_radial_menu_get_point(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t angle_deg, egui_dim_t *x, egui_dim_t *y)
{
    egui_float_t angle_rad = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(angle_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));
    egui_float_t cos_val = EGUI_FLOAT_COS(angle_rad);
    egui_float_t sin_val = EGUI_FLOAT_SIN(angle_rad);
    *x = center_x + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(radius), cos_val));
    *y = center_y + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(radius), sin_val));
}

static const char *egui_view_radial_menu_get_item_text(egui_view_radial_menu_t *local, uint8_t index)
{
    if (local->item_texts == NULL || index >= local->item_count || local->item_texts[index] == NULL)
    {
        return "";
    }
    return local->item_texts[index];
}

static int egui_view_radial_menu_is_center_hit(egui_view_t *self, egui_view_radial_menu_layout_t *layout, egui_dim_t screen_x, egui_dim_t screen_y)
{
    egui_dim_t center_x = self->region_screen.location.x + layout->center_x;
    egui_dim_t center_y = self->region_screen.location.y + layout->center_y;
    int32_t dx = screen_x - center_x;
    int32_t dy = screen_y - center_y;
    int32_t dist_sq = dx * dx + dy * dy;
    return dist_sq <= (int32_t)layout->center_radius * layout->center_radius;
}

static uint8_t egui_view_radial_menu_get_hit_index(egui_view_t *self, egui_view_radial_menu_t *local, egui_motion_event_t *event)
{
    egui_view_radial_menu_layout_t layout;
    if (!egui_view_radial_menu_build_layout(self, local, &layout) || layout.count == 0)
    {
        return EGUI_VIEW_RADIAL_MENU_INDEX_NONE;
    }

    egui_dim_t center_x = self->region_screen.location.x + layout.center_x;
    egui_dim_t center_y = self->region_screen.location.y + layout.center_y;
    int32_t dx = event->location.x - center_x;
    int32_t dy = event->location.y - center_y;
    int32_t dist_sq = dx * dx + dy * dy;
    int32_t inner_sq = (int32_t)layout.inner_radius * layout.inner_radius;
    int32_t outer_sq = (int32_t)layout.outer_radius * layout.outer_radius;

    if (dist_sq < inner_sq || dist_sq > outer_sq)
    {
        return EGUI_VIEW_RADIAL_MENU_INDEX_NONE;
    }

    int16_t angle = egui_view_radial_menu_atan2_deg(dy, dx);
    int16_t normalized = egui_view_radial_menu_normalize_angle(angle - 270 + layout.step_angle / 2);
    return (uint8_t)(normalized / layout.step_angle);
}

void egui_view_radial_menu_set_items(egui_view_t *self, const char **item_texts, uint8_t item_count)
{
    EGUI_LOCAL_INIT(egui_view_radial_menu_t);
    uint8_t clamped = egui_view_radial_menu_clamp_count(item_count);
    local->item_texts = item_texts;
    local->item_count = clamped;
    if (local->item_count == 0)
    {
        local->current_index = 0;
    }
    else if (local->current_index >= local->item_count)
    {
        local->current_index = 0;
    }
    local->hot_index = EGUI_VIEW_RADIAL_MENU_INDEX_NONE;
    egui_view_invalidate(self);
}

void egui_view_radial_menu_set_current_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_radial_menu_t);
    if (local->item_count == 0 || index >= local->item_count)
    {
        return;
    }
    if (local->current_index == index)
    {
        return;
    }
    local->current_index = index;
    if (local->on_selection_changed != NULL)
    {
        local->on_selection_changed(self, index);
    }
    egui_view_invalidate(self);
}

uint8_t egui_view_radial_menu_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_radial_menu_t);
    return local->current_index;
}

void egui_view_radial_menu_set_on_selection_changed_listener(egui_view_t *self, egui_view_radial_menu_on_selection_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_radial_menu_t);
    local->on_selection_changed = listener;
}

void egui_view_radial_menu_set_palette(egui_view_t *self, egui_color_t base_color, egui_color_t accent_color, egui_color_t center_color)
{
    EGUI_LOCAL_INIT(egui_view_radial_menu_t);
    local->base_color = base_color;
    local->accent_color = accent_color;
    local->center_color = center_color;
    egui_view_invalidate(self);
}

void egui_view_radial_menu_set_decoration_colors(egui_view_t *self, egui_color_t border_color, egui_color_t text_color, egui_color_t muted_text_color)
{
    EGUI_LOCAL_INIT(egui_view_radial_menu_t);
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    egui_view_invalidate(self);
}

void egui_view_radial_menu_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_radial_menu_t);
    if (font == NULL)
    {
        font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    }
    if (local->font == font)
    {
        return;
    }
    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_radial_menu_set_expanded(egui_view_t *self, uint8_t expanded)
{
    EGUI_LOCAL_INIT(egui_view_radial_menu_t);
    local->is_expanded = expanded ? 1 : 0;
    if (!local->is_expanded)
    {
        local->hot_index = EGUI_VIEW_RADIAL_MENU_INDEX_NONE;
    }
    egui_view_invalidate(self);
}

static void egui_view_radial_menu_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_radial_menu_t);
    egui_view_radial_menu_layout_t layout;
    if (!egui_view_radial_menu_build_layout(self, local, &layout))
    {
        return;
    }

    egui_color_t base_color = local->base_color;
    egui_color_t accent_color = local->accent_color;
    egui_color_t center_color = local->center_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_text_color = local->muted_text_color;
    egui_color_t plate_color;
    egui_color_t ring_back_color;
    egui_color_t chip_color;
    egui_alpha_t plate_alpha = egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30);
    egui_alpha_t ring_alpha = egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40);

    if (!egui_view_get_enable(self))
    {
        base_color = egui_rgb_mix(base_color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_60);
        accent_color = egui_rgb_mix(accent_color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_60);
        center_color = egui_rgb_mix(center_color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_60);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_50);
        text_color = egui_rgb_mix(text_color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_60);
        muted_text_color = egui_rgb_mix(muted_text_color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_60);
    }

    plate_color = egui_rgb_mix(EGUI_COLOR_BLACK, base_color, EGUI_ALPHA_50);
    ring_back_color = egui_rgb_mix(EGUI_COLOR_BLACK, base_color, EGUI_ALPHA_30);
    chip_color = egui_rgb_mix(EGUI_COLOR_BLACK, base_color, EGUI_ALPHA_60);

    egui_canvas_draw_circle_fill(layout.center_x, layout.center_y, layout.outer_radius, plate_color, plate_alpha);

    if (local->is_expanded && layout.count > 0)
    {
        uint8_t i;
        for (i = 0; i < layout.count; i++)
        {
            int16_t start_angle = layout.start_angle + layout.step_angle * i;
            int16_t end_angle = start_angle + layout.step_angle;
            egui_color_t fill_color = ring_back_color;
            egui_alpha_t fill_alpha = ring_alpha;

            if (i == local->current_index)
            {
                fill_color = egui_rgb_mix(base_color, accent_color, EGUI_ALPHA_60);
                fill_alpha = egui_color_alpha_mix(self->alpha, EGUI_ALPHA_80);
            }
            if (i == local->hot_index)
            {
                fill_color = accent_color;
                fill_alpha = self->alpha;
            }

            egui_canvas_draw_arc_fill(layout.center_x, layout.center_y, layout.outer_radius, start_angle, end_angle, fill_color, fill_alpha);
        }

        egui_canvas_draw_circle_fill(layout.center_x, layout.center_y, layout.inner_radius, EGUI_COLOR_HEX(0x0F172A), self->alpha);
        egui_canvas_draw_circle(
                layout.center_x,
                layout.center_y,
                layout.outer_radius,
                1,
                border_color,
                egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));
        egui_canvas_draw_circle(
                layout.center_x,
                layout.center_y,
                layout.inner_radius,
                1,
                border_color,
                egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));

        if (layout.count > 1)
        {
            uint8_t boundary_index;
            for (boundary_index = 0; boundary_index < layout.count; boundary_index++)
            {
                egui_dim_t x1;
                egui_dim_t y1;
                egui_dim_t x2;
                egui_dim_t y2;
                int16_t angle = layout.start_angle + layout.step_angle * boundary_index;
                egui_view_radial_menu_get_point(layout.center_x, layout.center_y, layout.inner_radius, angle, &x1, &y1);
                egui_view_radial_menu_get_point(layout.center_x, layout.center_y, layout.outer_radius, angle, &x2, &y2);
                egui_canvas_draw_line(x1, y1, x2, y2, 1, border_color, EGUI_ALPHA_20);
            }
        }

        if (layout.region.size.width <= 100 && local->current_index < layout.count && local->current_index != local->hot_index)
        {
            int16_t current_start = layout.start_angle + layout.step_angle * local->current_index + 8;
            int16_t current_end = layout.start_angle + layout.step_angle * (local->current_index + 1) - 8;
            egui_color_t current_trace_color = egui_rgb_mix(accent_color, EGUI_COLOR_WHITE, EGUI_ALPHA_30);
            egui_canvas_draw_arc(
                    layout.center_x,
                    layout.center_y,
                    layout.inner_radius + 5,
                    current_start,
                    current_end,
                    1,
                    current_trace_color,
                    egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
        }

        {
            uint8_t marker_index;
            for (marker_index = 0; marker_index < layout.count; marker_index++)
            {
                egui_dim_t marker_x;
                egui_dim_t marker_y;
                egui_color_t marker_color;
                egui_dim_t marker_radius;
                int16_t marker_angle;
                int16_t marker_start;
                int16_t marker_end;
                if (marker_index != local->current_index && marker_index != local->hot_index)
                {
                    continue;
                }
                marker_angle = layout.start_angle + layout.step_angle * marker_index + layout.step_angle / 2;
                marker_start = layout.start_angle + layout.step_angle * marker_index + 6;
                marker_end = layout.start_angle + layout.step_angle * (marker_index + 1) - 6;
                egui_view_radial_menu_get_point(layout.center_x, layout.center_y, layout.outer_radius - 6, marker_angle, &marker_x, &marker_y);
                marker_color = (marker_index == local->hot_index) ? accent_color : egui_rgb_mix(accent_color, EGUI_COLOR_WHITE, EGUI_ALPHA_20);
                marker_radius = (marker_index == local->hot_index) ? 4 : 2;
                egui_canvas_draw_arc(
                        layout.center_x,
                        layout.center_y,
                        layout.outer_radius - 2,
                        marker_start,
                        marker_end,
                        (marker_index == local->hot_index) ? 3 : 1,
                        marker_color,
                        (marker_index == local->hot_index) ? self->alpha : egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
                egui_canvas_draw_circle_fill(
                        marker_x,
                        marker_y,
                        marker_radius + ((marker_index == local->hot_index) ? 2 : 1),
                        marker_color,
                        egui_color_alpha_mix(self->alpha, (marker_index == local->hot_index) ? EGUI_ALPHA_30 : EGUI_ALPHA_20));
                egui_canvas_draw_circle_fill(marker_x, marker_y, marker_radius, marker_color, self->alpha);
            }
        }

        {
            uint8_t label_index;
            for (label_index = 0; label_index < layout.count; label_index++)
            {
                egui_dim_t label_x;
                egui_dim_t label_y;
                egui_region_t text_region;
                egui_region_t chip_region;
                egui_color_t chip_bg = chip_color;
                egui_color_t color = muted_text_color;
                egui_alpha_t current_chip_alpha = egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40);
                egui_alpha_t text_alpha = egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40);
                int show_single_label = (layout.region.size.width <= 100);
                int draw_chip = show_single_label;
                int16_t angle = layout.start_angle + layout.step_angle * label_index + layout.step_angle / 2;

                if (show_single_label)
                {
                    if (local->hot_index != EGUI_VIEW_RADIAL_MENU_INDEX_NONE)
                    {
                        if (label_index != local->hot_index)
                        {
                            continue;
                        }
                    }
                    else if (label_index != local->current_index)
                    {
                        continue;
                    }
                }

                if (label_index == local->current_index)
                {
                    chip_bg = egui_rgb_mix(chip_color, accent_color, EGUI_ALPHA_30);
                    color = text_color;
                    current_chip_alpha = egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60);
                    draw_chip = 1;
                    text_alpha = self->alpha;
                }
                if (label_index == local->hot_index)
                {
                    chip_bg = accent_color;
                    color = EGUI_COLOR_WHITE;
                    current_chip_alpha = egui_color_alpha_mix(self->alpha, EGUI_ALPHA_80);
                    draw_chip = 1;
                    text_alpha = self->alpha;
                }
                if (!show_single_label && local->hot_index == EGUI_VIEW_RADIAL_MENU_INDEX_NONE && label_index == local->current_index)
                {
                    continue;
                }
                if (!show_single_label && local->hot_index != EGUI_VIEW_RADIAL_MENU_INDEX_NONE && label_index == local->current_index)
                {
                    continue;
                }

                if (show_single_label)
                {
                    text_region.location.x = layout.center_x - (layout.label_width + 10) / 2;
                    text_region.location.y = layout.region.location.y + 8;
                    text_region.size.width = layout.label_width + 10;
                    text_region.size.height = layout.label_height;
                }
                else
                {
                    egui_dim_t label_radius = layout.label_radius;
                    if (label_index != local->hot_index)
                    {
                        label_radius += 4;
                    }
                    egui_view_radial_menu_get_point(layout.center_x, layout.center_y, label_radius, angle, &label_x, &label_y);
                    text_region.location.x = label_x - layout.label_width / 2;
                    text_region.location.y = label_y - layout.label_height / 2;
                    text_region.size.width = layout.label_width;
                    text_region.size.height = layout.label_height;
                }

                chip_region.location.x = text_region.location.x - 2;
                chip_region.location.y = text_region.location.y - 1;
                chip_region.size.width = text_region.size.width + 4;
                chip_region.size.height = text_region.size.height + 2;
                if (draw_chip)
                {
                    egui_canvas_draw_round_rectangle_fill(
                            chip_region.location.x,
                            chip_region.location.y,
                            chip_region.size.width,
                            chip_region.size.height,
                            chip_region.size.height / 2,
                            chip_bg,
                            current_chip_alpha);
                }
                egui_canvas_draw_text_in_rect(local->font, egui_view_radial_menu_get_item_text(local, label_index), &text_region, EGUI_ALIGN_CENTER, color, text_alpha);
            }
        }
    }
    else if (layout.count > 0)
    {
        uint8_t i;
        egui_canvas_draw_circle_fill(layout.center_x, layout.center_y, layout.inner_radius + 4, ring_back_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        egui_canvas_draw_circle(
                layout.center_x,
                layout.center_y,
                layout.label_radius,
                1,
                border_color,
                egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        for (i = 0; i < layout.count; i++)
        {
            egui_dim_t dot_x;
            egui_dim_t dot_y;
            egui_dim_t dot_radius = (i == local->current_index) ? 4 : 3;
            egui_color_t dot_color = (i == local->current_index) ? accent_color : muted_text_color;
            egui_alpha_t dot_alpha = (i == local->current_index) ? self->alpha : egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70);
            int16_t angle = layout.start_angle + layout.step_angle * i + layout.step_angle / 2;
            egui_view_radial_menu_get_point(layout.center_x, layout.center_y, layout.label_radius, angle, &dot_x, &dot_y);
            if (i == local->current_index)
            {
                egui_canvas_draw_circle_fill(dot_x, dot_y, dot_radius + 2, dot_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
            }
            egui_canvas_draw_circle_fill(dot_x, dot_y, dot_radius, dot_color, dot_alpha);
        }
        egui_canvas_draw_circle(layout.center_x, layout.center_y, layout.outer_radius, 1, border_color, EGUI_ALPHA_40);
    }

    {
        const char *center_text = "OPEN";
        const egui_font_t *center_font = local->font;
        egui_color_t draw_center_color = center_color;
        egui_color_t center_text_color = EGUI_COLOR_WHITE;
        egui_color_t center_border_color = border_color;
        egui_color_t center_outer_ring_color = accent_color;
        egui_alpha_t center_halo_alpha = egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20);
        egui_alpha_t center_outer_ring_alpha = 0;
        egui_dim_t center_halo_radius = layout.center_radius + 4;
        egui_dim_t center_text_width = 0;
        egui_dim_t center_text_height = 0;
        egui_region_t center_region;

        if (!egui_view_get_enable(self))
        {
            center_text = "LOCK";
            center_text_color = EGUI_COLOR_LIGHT_GREY;
        }
        else if (local->is_expanded)
        {
            if (local->hot_index != EGUI_VIEW_RADIAL_MENU_INDEX_NONE)
            {
                if (layout.region.size.width <= 100 && local->item_count > 0 && local->hot_index != local->current_index)
                {
                    center_text = egui_view_radial_menu_get_item_text(local, local->current_index);
                    draw_center_color = egui_rgb_mix(center_color, accent_color, EGUI_ALPHA_30);
                    center_border_color = accent_color;
                    center_halo_alpha = egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30);
                    center_halo_radius = layout.center_radius + 5;
                }
                else
                {
                    center_text = egui_view_radial_menu_get_item_text(local, local->hot_index);
                    draw_center_color = accent_color;
                    center_border_color = accent_color;
                    center_halo_alpha = egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40);
                    center_outer_ring_alpha = egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60);
                    center_halo_radius = layout.center_radius + 7;
                }
            }
            else if (local->expanded_on_down)
            {
                center_text = "TAP";
            }
            else if (local->item_count > 0)
            {
                center_text = egui_view_radial_menu_get_item_text(local, local->current_index);
                draw_center_color = egui_rgb_mix(center_color, accent_color, EGUI_ALPHA_30);
                center_halo_alpha = egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30);
            }
            else
            {
                center_text = "PICK";
            }
        }
        else if (layout.region.size.width <= 100 && local->item_count > 0)
        {
            center_text = egui_view_radial_menu_get_item_text(local, local->current_index);
            center_text_color = text_color;
        }

        egui_canvas_draw_circle_fill(layout.center_x, layout.center_y, center_halo_radius, draw_center_color, center_halo_alpha);
        if (center_outer_ring_alpha > 0)
        {
            egui_canvas_draw_circle(
                    layout.center_x,
                    layout.center_y,
                    layout.center_radius + 3,
                    1,
                    center_outer_ring_color,
                    center_outer_ring_alpha);
        }
        if (egui_view_get_pressed(self))
        {
            draw_center_color = egui_rgb_mix(draw_center_color, EGUI_COLOR_WHITE, EGUI_ALPHA_30);
        }

        egui_canvas_draw_circle_fill(layout.center_x, layout.center_y, layout.center_radius, draw_center_color, self->alpha);
        egui_canvas_draw_circle(layout.center_x, layout.center_y, layout.center_radius, 1, center_border_color, self->alpha);
        egui_canvas_draw_circle_fill(layout.center_x, layout.center_y, EGUI_MAX(layout.center_radius / 6, 2), EGUI_COLOR_WHITE, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_80));

        center_font->api->get_str_size(center_font, center_text, 0, 0, &center_text_width, &center_text_height);
        if (center_text_width > layout.center_radius * 2 - 10 && center_text[4] != '\0')
        {
            center_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;
            center_font->api->get_str_size(center_font, center_text, 0, 0, &center_text_width, &center_text_height);
        }
        center_region.location.x = layout.center_x - layout.center_radius + 5;
        center_region.location.y = layout.center_y - layout.center_radius + 4;
        center_region.size.width = layout.center_radius * 2 - 10;
        center_region.size.height = layout.center_radius * 2 - 8;
        egui_canvas_draw_text_in_rect(center_font, center_text, &center_region, EGUI_ALIGN_CENTER | EGUI_ALIGN_VCENTER, center_text_color, self->alpha);
    }
}


#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_radial_menu_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_radial_menu_t);
    egui_view_radial_menu_layout_t layout;

    if (!egui_view_radial_menu_build_layout(self, local, &layout))
    {
        return 0;
    }

    if (!egui_view_get_enable(self))
    {
        return 1;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        uint8_t hit_index = egui_view_radial_menu_get_hit_index(self, local, event);
        int center_hit = egui_view_radial_menu_is_center_hit(self, &layout, event->location.x, event->location.y);
        if (!center_hit && !local->is_expanded)
        {
            return 0;
        }

        local->expanded_on_down = local->is_expanded;
        local->is_tracking = 1;
        local->is_expanded = 1;
        local->hot_index = center_hit ? EGUI_VIEW_RADIAL_MENU_INDEX_NONE : hit_index;
        egui_view_set_pressed(self, true);
        if (self->parent != NULL)
        {
            egui_view_group_request_disallow_intercept_touch_event((egui_view_t *)self->parent, 1);
        }
        egui_view_invalidate(self);
        return 1;
    }
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->is_tracking)
        {
            uint8_t hit_index = egui_view_radial_menu_get_hit_index(self, local, event);
            if (local->hot_index != hit_index)
            {
                local->hot_index = hit_index;
                egui_view_invalidate(self);
            }
            return 1;
        }
        break;
    case EGUI_MOTION_EVENT_ACTION_UP:
        if (local->is_tracking)
        {
            uint8_t hit_index = egui_view_radial_menu_get_hit_index(self, local, event);
            int center_hit = egui_view_radial_menu_is_center_hit(self, &layout, event->location.x, event->location.y);
            if (hit_index != EGUI_VIEW_RADIAL_MENU_INDEX_NONE)
            {
                egui_view_radial_menu_set_current_index(self, hit_index);
                local->is_expanded = 0;
            }
            else if (center_hit)
            {
                local->is_expanded = local->expanded_on_down ? 0 : 1;
            }
            else
            {
                local->is_expanded = 0;
            }
            local->is_tracking = 0;
            local->hot_index = EGUI_VIEW_RADIAL_MENU_INDEX_NONE;
            egui_view_set_pressed(self, false);
            if (self->parent != NULL)
            {
                egui_view_group_request_disallow_intercept_touch_event((egui_view_t *)self->parent, 0);
            }
            egui_view_invalidate(self);
            return 1;
        }
        break;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->is_tracking = 0;
        local->is_expanded = local->expanded_on_down;
        local->hot_index = EGUI_VIEW_RADIAL_MENU_INDEX_NONE;
        egui_view_set_pressed(self, false);
        if (self->parent != NULL)
        {
            egui_view_group_request_disallow_intercept_touch_event((egui_view_t *)self->parent, 0);
        }
        egui_view_invalidate(self);
        return 1;
    default:
        break;
    }

    return 0;
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_radial_menu_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_radial_menu_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_radial_menu_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_radial_menu_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_radial_menu_t);
    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_radial_menu_t);
    egui_view_set_padding_all(self, 4);

    local->item_texts = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_selection_changed = NULL;
    local->base_color = EGUI_COLOR_HEX(0x1E293B);
    local->accent_color = EGUI_COLOR_HEX(0x38BDF8);
    local->center_color = EGUI_COLOR_HEX(0x0EA5E9);
    local->border_color = EGUI_COLOR_HEX(0xCBD5E1);
    local->text_color = EGUI_COLOR_WHITE;
    local->muted_text_color = EGUI_COLOR_LIGHT_GREY;
    local->item_count = 0;
    local->current_index = 0;
    local->hot_index = EGUI_VIEW_RADIAL_MENU_INDEX_NONE;
    local->is_expanded = 0;
    local->is_tracking = 0;
    local->expanded_on_down = 0;
}
