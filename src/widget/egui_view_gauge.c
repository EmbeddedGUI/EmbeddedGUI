#include "egui_view_gauge.h"
#include "core/egui_common.h"
#include "resource/egui_resource.h"
#include "utils/egui_fixmath.h"
#include "egui_view_circle_dirty.h"
#include "egui_view_ring_text_basic.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

typedef uint8_t (*egui_view_gauge_get_text_region_fn)(egui_view_gauge_t *local, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t inner_r,
                                                      uint8_t value, egui_region_t *text_region);
typedef void (*egui_view_gauge_draw_text_fn)(egui_view_gauge_t *local, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t inner_r);

struct egui_view_gauge_text_ops
{
    egui_view_gauge_get_text_region_fn get_text_region;
    egui_view_gauge_draw_text_fn draw_text;
};

static const egui_font_t *egui_view_gauge_get_text_font(const egui_view_gauge_t *local)
{
    if (local->font != NULL)
    {
        return local->font;
    }

    return (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
}

static uint8_t egui_view_gauge_get_text_box_rich(egui_view_gauge_t *local, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t inner_r,
                                                 const char *text, egui_region_t *text_box)
{
    egui_dim_t text_w;
    egui_dim_t text_h;
    egui_dim_t box_width;
    egui_dim_t box_height;
    const egui_font_t *text_font = egui_view_gauge_get_text_font(local);

    if (text_box == NULL || text == NULL || text[0] == '\0')
    {
        return 0;
    }

    box_width = inner_r * 2 - 4;
    if (box_width <= 0)
    {
        return 0;
    }

    box_height = box_width;
    text_w = 0;
    text_h = 0;
    if (text_font != NULL && text_font->api != NULL && text_font->api->get_str_size != NULL)
    {
        text_font->api->get_str_size(text_font, text, 0, 0, &text_w, &text_h);
        if (text_h > 0)
        {
            box_height = EGUI_MIN(box_height, text_h + 4);
        }
    }

    text_box->location.x = center_x - box_width / 2;
    text_box->location.y = center_y + EGUI_MAX(radius / 10, 3) + 1;
    text_box->size.width = box_width;
    text_box->size.height = box_height;

    return egui_region_is_empty(text_box) ? 0 : 1;
}

static uint8_t egui_view_gauge_get_text_region_basic(egui_view_gauge_t *local, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t inner_r,
                                                     uint8_t value, egui_region_t *text_region)
{
    (void)local;
    return egui_view_ring_text_get_region_basic(center_x, center_y, inner_r, EGUI_MAX(radius / 10, 3) + 1, 0, value, 0, text_region);
}

static uint8_t egui_view_gauge_get_text_region_rich(egui_view_gauge_t *local, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t inner_r,
                                                    uint8_t value, egui_region_t *text_region)
{
    egui_region_t text_box;
    egui_dim_t text_w;
    egui_dim_t text_h;
    egui_dim_t offset_x;
    egui_dim_t offset_y;
    char text_buf[8];
    const egui_font_t *text_font = egui_view_gauge_get_text_font(local);

    if (text_region == NULL)
    {
        return 0;
    }

    egui_view_ring_text_format_value(value, 0, text_buf, sizeof(text_buf));
    if (!egui_view_gauge_get_text_box_rich(local, center_x, center_y, radius, inner_r, text_buf, &text_box))
    {
        return 0;
    }

    if (text_font == NULL || text_font->api == NULL || text_font->api->get_str_size == NULL)
    {
        egui_region_copy(text_region, &text_box);
        return 1;
    }

    text_font->api->get_str_size(text_font, text_buf, 0, 0, &text_w, &text_h);
    if (text_w <= 0 || text_h <= 0)
    {
        egui_region_copy(text_region, &text_box);
        return 1;
    }

    text_w = EGUI_MIN(text_w, text_box.size.width);
    text_h = EGUI_MIN(text_h, text_box.size.height);
    egui_common_align_get_x_y(text_box.size.width, text_box.size.height, text_w, text_h, EGUI_ALIGN_CENTER, &offset_x, &offset_y);

    text_region->location.x = text_box.location.x + offset_x;
    text_region->location.y = text_box.location.y + offset_y;
    text_region->size.width = text_w;
    text_region->size.height = text_h;

    if (text_region->location.x > text_box.location.x)
    {
        text_region->location.x--;
        text_region->size.width++;
    }
    if (text_region->location.y > text_box.location.y)
    {
        text_region->location.y--;
        text_region->size.height++;
    }
    if (text_region->location.x + text_region->size.width < text_box.location.x + text_box.size.width)
    {
        text_region->size.width++;
    }
    if (text_region->location.y + text_region->size.height < text_box.location.y + text_box.size.height)
    {
        text_region->size.height++;
    }

    egui_region_intersect(text_region, &text_box, text_region);
    return egui_region_is_empty(text_region) ? 0 : 1;
}

static void egui_view_gauge_draw_text_basic(egui_view_gauge_t *local, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t inner_r)
{
    egui_view_ring_text_draw_basic(center_x, center_y, inner_r, EGUI_MAX(radius / 10, 3) + 1, 0, local->value, 0, local->text_color, EGUI_ALPHA_100);
}

static void egui_view_gauge_draw_text_rich(egui_view_gauge_t *local, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t inner_r)
{
    char val_buf[8];
    egui_region_t text_rect;
    const egui_font_t *text_font = egui_view_gauge_get_text_font(local);

    egui_view_ring_text_format_value(local->value, 0, val_buf, sizeof(val_buf));
    if (text_font != NULL && egui_view_gauge_get_text_box_rich(local, center_x, center_y, radius, inner_r, val_buf, &text_rect))
    {
        egui_canvas_draw_text_in_rect(text_font, val_buf, &text_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
    }
}

static const egui_view_gauge_text_ops_t egui_view_gauge_basic_text_ops = {
        .get_text_region = egui_view_gauge_get_text_region_basic,
        .draw_text = egui_view_gauge_draw_text_basic,
};

static const egui_view_gauge_text_ops_t egui_view_gauge_rich_text_ops = {
        .get_text_region = egui_view_gauge_get_text_region_rich,
        .draw_text = egui_view_gauge_draw_text_rich,
};

static void egui_view_gauge_invalidate_value_change(egui_view_t *self, egui_view_gauge_t *local, uint8_t old_value)
{
    egui_region_t region;
    egui_region_t indicator_dirty_region;
    egui_region_t arc_region;
    egui_region_t text_region;
    egui_region_t text_dirty_region;
    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t radius;
    egui_dim_t inner_r;
    egui_dim_t mid_r;
    egui_dim_t needle_len;
    egui_dim_t tip_x;
    egui_dim_t tip_y;
    int16_t old_end_angle;
    int16_t new_end_angle;
    int16_t dirty_start_angle;
    uint16_t dirty_sweep;

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

    egui_view_get_work_region(self, &region);
    center_x = region.location.x + region.size.width / 2;
    center_y = region.location.y + region.size.height / 2;
    radius = EGUI_MIN(region.size.width, region.size.height) / 2 - local->stroke_width / 2 - 1;
    if (radius <= 0)
    {
        egui_view_invalidate(self);
        return;
    }

    inner_r = radius - local->stroke_width;
    if (inner_r < 0)
    {
        inner_r = 0;
    }
    mid_r = radius - local->stroke_width / 2;
    if (mid_r < 0)
    {
        mid_r = 0;
    }
    needle_len = radius - local->stroke_width / 2 - 4;
    if (needle_len < 0)
    {
        needle_len = 0;
    }

    egui_region_init_empty(&indicator_dirty_region);
    egui_region_init_empty(&text_dirty_region);

    old_end_angle = local->start_angle + (int16_t)((int32_t)local->sweep_angle * old_value / 100);
    new_end_angle = local->start_angle + (int16_t)((int32_t)local->sweep_angle * local->value / 100);
    if (old_end_angle != new_end_angle)
    {
        dirty_start_angle = EGUI_MIN(old_end_angle, new_end_angle);
        dirty_sweep = (uint16_t)EGUI_ABS(new_end_angle - old_end_angle);
        if (egui_view_circle_dirty_compute_arc_region(center_x, center_y, mid_r, local->stroke_width / 2 + EGUI_VIEW_CIRCLE_DIRTY_AA_PAD, dirty_start_angle,
                                                      dirty_sweep, &arc_region))
        {
            egui_view_circle_dirty_union_region(&indicator_dirty_region, &arc_region);
        }
    }

    egui_view_circle_dirty_get_circle_point(center_x, center_y, needle_len, old_end_angle, &tip_x, &tip_y);
    egui_view_circle_dirty_add_line_region(&indicator_dirty_region, center_x, center_y, tip_x, tip_y, local->needle_width, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD);

    egui_view_circle_dirty_get_circle_point(center_x, center_y, needle_len, new_end_angle, &tip_x, &tip_y);
    egui_view_circle_dirty_add_line_region(&indicator_dirty_region, center_x, center_y, tip_x, tip_y, local->needle_width, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD);

    if (local->text_ops != NULL && local->text_ops->get_text_region(local, center_x, center_y, radius, inner_r, old_value, &text_region))
    {
        egui_view_circle_dirty_union_region(&text_dirty_region, &text_region);
    }
    if (old_value != local->value && local->text_ops != NULL &&
        local->text_ops->get_text_region(local, center_x, center_y, radius, inner_r, local->value, &text_region))
    {
        egui_view_circle_dirty_union_region(&text_dirty_region, &text_region);
    }

    if (egui_region_is_empty(&indicator_dirty_region) && egui_region_is_empty(&text_dirty_region))
    {
        egui_view_invalidate(self);
        return;
    }

    if (!egui_region_is_empty(&indicator_dirty_region))
    {
        egui_view_invalidate_region(self, &indicator_dirty_region);
    }
    if (!egui_region_is_empty(&text_dirty_region))
    {
        egui_view_invalidate_region(self, &text_dirty_region);
    }
}

void egui_view_gauge_set_value(egui_view_t *self, uint8_t value)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);
    uint8_t old_value;
    if (value > 100)
    {
        value = 100;
    }
    if (value != local->value)
    {
        old_value = local->value;
        local->value = value;
        egui_view_gauge_invalidate_value_change(self, local, old_value);
    }
}

void egui_view_gauge_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);
    local->font = font;
    local->text_ops = (font == NULL) ? &egui_view_gauge_basic_text_ops : &egui_view_gauge_rich_text_ops;
    egui_view_invalidate(self);
}

void egui_view_gauge_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);
    local->text_color = color;
    egui_view_invalidate(self);
}

void egui_view_gauge_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;
    egui_dim_t center_x = region.location.x + w / 2;
    egui_dim_t center_y = region.location.y + h / 2;
    egui_dim_t radius = EGUI_MIN(w, h) / 2 - local->stroke_width / 2 - 1;

    if (radius <= 0)
    {
        return;
    }

    egui_dim_t inner_r = radius - local->stroke_width;
    if (inner_r < 0)
        inner_r = 0;

    // Background arc (full sweep)
    int16_t bg_start = local->start_angle;
    int16_t bg_end = local->start_angle + local->sweep_angle;
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_gradient_stop_t bg_stops[2] = {
                {.position = 0, .color = local->bk_color},
                {.position = 255, .color = local->bk_color},
        };
        egui_gradient_t bg_grad = {
                .type = EGUI_GRADIENT_TYPE_RADIAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = bg_stops,
        };
        egui_canvas_draw_arc_ring_fill_gradient(center_x, center_y, radius, inner_r, bg_start, bg_end, &bg_grad);
    }
#else
    egui_canvas_draw_arc(center_x, center_y, radius, bg_start, bg_end, local->stroke_width, local->bk_color, EGUI_ALPHA_100);
#endif

    // Progress arc
    int16_t progress_end = local->start_angle + (int16_t)((int32_t)local->sweep_angle * local->value / 100);
    if (local->value > 0)
    {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t color_light = egui_rgb_mix(local->progress_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t stops[2] = {
                    {.position = 0, .color = color_light},
                    {.position = 255, .color = local->progress_color},
            };
            egui_gradient_t grad = {
                    .type = EGUI_GRADIENT_TYPE_RADIAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = stops,
            };
            egui_canvas_draw_arc_ring_fill_gradient(center_x, center_y, radius, inner_r, bg_start, progress_end, &grad);
        }
#else
        egui_canvas_draw_arc(center_x, center_y, radius, bg_start, progress_end, local->stroke_width, local->progress_color, EGUI_ALPHA_100);
#endif
    }

    // Needle - analog clock style hand
    egui_dim_t needle_len = radius - local->stroke_width / 2 - 4;
    egui_dim_t center_dot_r = EGUI_MAX(radius / 10, 3);

    // Tick marks: 10 divisions, minor every 10%, major at 0%/50%/100%
    {
        egui_dim_t tick_base = inner_r - 3; // just inside the arc ring
        for (int i = 0; i <= 10; i++)
        {
            int16_t tick_deg = local->start_angle + (int16_t)((int32_t)local->sweep_angle * i / 10);
            int is_major = (i % 5 == 0);
            egui_dim_t tlen = is_major ? 9 : 5;
            egui_alpha_t ta = is_major ? 200 : 110;
            egui_dim_t x1;
            egui_dim_t y1;
            egui_dim_t x2;
            egui_dim_t y2;

            egui_view_circle_dirty_get_circle_point(center_x, center_y, tick_base, tick_deg, &x1, &y1);
            egui_view_circle_dirty_get_circle_point(center_x, center_y, tick_base - tlen, tick_deg, &x2, &y2);
            egui_canvas_draw_line(x1, y1, x2, y2, is_major ? 2 : 1, local->needle_color, ta);
        }
    }
    if (needle_len > 0)
    {
        // angle_deg = start_angle + value * sweep_angle / 100
        int16_t needle_deg = local->start_angle + (int16_t)((int32_t)local->sweep_angle * local->value / 100);
        egui_dim_t tip_x;
        egui_dim_t tip_y;
        egui_dim_t hand_w = local->needle_width;

        egui_view_circle_dirty_get_circle_point(center_x, center_y, needle_len, needle_deg, &tip_x, &tip_y);
        egui_canvas_draw_line_round_cap_hq(center_x, center_y, tip_x, tip_y, hand_w, local->needle_color, EGUI_ALPHA_100);
    }

    // Center dot decoration
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(local->needle_color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = local->needle_color},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_RADIAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = stops,
        };
        egui_canvas_draw_circle_fill_gradient(center_x, center_y, center_dot_r, &grad);
    }
#else
    egui_canvas_draw_circle_fill(center_x, center_y, center_dot_r, local->needle_color, EGUI_ALPHA_100);
#endif

    // Center value text below the pivot dot. NULL font falls back to the default widget font.
    if (local->text_ops != NULL)
    {
        local->text_ops->draw_text(local, center_x, center_y, radius, inner_r);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_gauge_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_gauge_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_gauge_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_gauge_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_gauge_t);

    // init local data.
    local->value = 0;
    local->stroke_width = EGUI_THEME_TRACK_THICKNESS;
    local->start_angle = 150; // ~7 o'clock (0=3 o'clock, clockwise)
    local->sweep_angle = 240; // sweep 240 degrees, gap at bottom
    local->bk_color = EGUI_THEME_TRACK_BG;
    local->progress_color = EGUI_THEME_PRIMARY;
    local->needle_color = EGUI_THEME_DANGER;
    local->needle_width = 3;
    local->text_color = EGUI_THEME_TEXT_PRIMARY;
    local->font = NULL;
    local->text_ops = &egui_view_gauge_basic_text_ops;

    egui_view_set_view_name(self, "egui_view_gauge");
}

void egui_view_gauge_apply_params(egui_view_t *self, const egui_view_gauge_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);

    self->region = params->region;

    local->value = params->value;

    egui_view_invalidate(self);
}

void egui_view_gauge_init_with_params(egui_view_t *self, const egui_view_gauge_params_t *params)
{
    egui_view_gauge_init(self);
    egui_view_gauge_apply_params(self, params);
}
