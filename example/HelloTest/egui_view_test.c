#include <assert.h>
#include <stdio.h>

#include "egui_view_test.h"
#include "app_egui_resource_generate.h"
#include "egui.h"
#include "widget/egui_view_circle_dirty.h"

#define EGUI_VIEW_TEST_TIMER_INTERVAL_MS 33

#define EGUI_VIEW_TEST_PANEL_MARGIN      16
#define EGUI_VIEW_TEST_ARC_RADIUS        60
#define EGUI_VIEW_TEST_ARC_STROKE        10
#define EGUI_VIEW_TEST_ARC_STEP          6
#define EGUI_VIEW_TEST_POPUP_PHASE_MAX   128
#define EGUI_VIEW_TEST_IMAGE_BASE_SIZE   40
#define EGUI_VIEW_TEST_IMAGE_BASE_HALF   (EGUI_VIEW_TEST_IMAGE_BASE_SIZE / 2)
#define EGUI_VIEW_TEST_TEXT_TOP_OFFSET   11
#define EGUI_VIEW_TEST_TEXT_GAP          3
#define EGUI_VIEW_TEST_TEXT_SIDE_GAP     8
#define EGUI_VIEW_TEST_DIRTY_PAD         2
#define EGUI_VIEW_TEST_BLOCK_EDGE_MARGIN 12

typedef struct egui_view_test_transform_item
{
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t width;
    egui_dim_t height;
    int16_t angle_deg;
    int16_t scale_q8;
} egui_view_test_transform_item_t;

static egui_color_t egui_view_test_color_make(uint8_t r, uint8_t g, uint8_t b)
{
    return EGUI_COLOR_MAKE(r, g, b);
}

static egui_dim_t egui_view_test_lerp(egui_dim_t from, egui_dim_t to, uint16_t step, uint16_t step_max)
{
    if (step_max == 0)
    {
        return to;
    }

    return (egui_dim_t)(from + (((int32_t)(to - from) * step) / step_max));
}

static void egui_view_test_ensure_metrics(egui_view_test_t *local)
{
    const egui_font_t *font = EGUI_FONT_OF(&egui_res_font_notosanssc_16_4);
    static const char text_cn[] = "\xE4\xBD\xA0\xE5\xA5\xBD";
    static const char text_en[] = "Hello";

    if (local->metrics_ready)
    {
        return;
    }

    egui_image_std_get_width_height((const egui_image_t *)&egui_res_image_test_rgb565_8, &local->photo_w, &local->photo_h);

    if (font != NULL && font->api != NULL && font->api->get_str_size != NULL)
    {
        if (font->api->get_str_size(font, text_cn, 0, 0, &local->text_cn_w, &local->text_cn_h) != 0)
        {
            local->text_cn_w = 0;
            local->text_cn_h = 0;
        }
        if (font->api->get_str_size(font, text_en, 0, 0, &local->text_en_w, &local->text_en_h) != 0)
        {
            local->text_en_w = 0;
            local->text_en_h = 0;
        }
    }

    local->metrics_ready = 1;
}

static void egui_view_test_get_panel_region(const egui_view_t *self, egui_region_t *panel)
{
    egui_region_t work_region;

    egui_region_init_empty(panel);
    egui_view_get_work_region((egui_view_t *)self, &work_region);

    panel->location.x = work_region.location.x + EGUI_VIEW_TEST_PANEL_MARGIN;
    panel->location.y = work_region.location.y + EGUI_VIEW_TEST_PANEL_MARGIN;
    panel->size.width = EGUI_MAX(0, work_region.size.width - EGUI_VIEW_TEST_PANEL_MARGIN * 2);
    panel->size.height = EGUI_MAX(0, work_region.size.height - EGUI_VIEW_TEST_PANEL_MARGIN * 2);
}

static void egui_view_test_get_panel_center(const egui_region_t *panel, egui_dim_t *center_x, egui_dim_t *center_y)
{
    if (center_x != NULL)
    {
        *center_x = panel->location.x + panel->size.width / 2;
    }
    if (center_y != NULL)
    {
        *center_y = panel->location.y + panel->size.height / 2;
    }
}

static void egui_view_test_reset_blocks(egui_view_test_t *local)
{
    local->blocks[0].x = 26;
    local->blocks[0].y = 24;
    local->blocks[0].size = 30;
    local->blocks[0].vx = 3;
    local->blocks[0].vy = 2;
    local->blocks[0].fill_color = egui_view_test_color_make(84, 194, 213);
    local->blocks[0].alpha = EGUI_ALPHA_50;

    local->blocks[1].x = 174;
    local->blocks[1].y = 144;
    local->blocks[1].size = 24;
    local->blocks[1].vx = -2;
    local->blocks[1].vy = 3;
    local->blocks[1].fill_color = egui_view_test_color_make(224, 176, 92);
    local->blocks[1].alpha = EGUI_ALPHA_50;
}

static void egui_view_test_get_block_bounds(const egui_view_t *self, egui_dim_t *left, egui_dim_t *top, egui_dim_t *right, egui_dim_t *bottom)
{
    egui_region_t work_region;

    egui_view_get_work_region((egui_view_t *)self, &work_region);

    *left = work_region.location.x + EGUI_VIEW_TEST_BLOCK_EDGE_MARGIN;
    *top = work_region.location.y + EGUI_VIEW_TEST_BLOCK_EDGE_MARGIN;
    *right = work_region.location.x + work_region.size.width - EGUI_VIEW_TEST_BLOCK_EDGE_MARGIN;
    *bottom = work_region.location.y + work_region.size.height - EGUI_VIEW_TEST_BLOCK_EDGE_MARGIN;
}

static uint8_t egui_view_test_block_is_circle(uint8_t index)
{
    return index == 0;
}

static void egui_view_test_add_block_dirty(egui_region_t *dirty_region, uint8_t index, const egui_view_test_block_t *block)
{
    if (dirty_region == NULL || block == NULL)
    {
        return;
    }

    if (egui_view_test_block_is_circle(index))
    {
        egui_view_circle_dirty_add_circle_region(dirty_region, block->x + block->size / 2, block->y + block->size / 2, block->size / 2, 1);
    }
    else
    {
        egui_view_circle_dirty_add_rect_region(dirty_region, block->x, block->y, block->size, block->size, 1);
    }
}

static int egui_view_test_blocks_overlap(const egui_view_test_block_t *a, const egui_view_test_block_t *b)
{
    return !(a->x + a->size <= b->x || b->x + b->size <= a->x || a->y + a->size <= b->y || b->y + b->size <= a->y);
}

static void egui_view_test_resolve_blocks(egui_view_test_block_t *a, egui_view_test_block_t *b)
{
    egui_dim_t overlap_x;
    egui_dim_t overlap_y;
    int8_t tmp;

    if (!egui_view_test_blocks_overlap(a, b))
    {
        return;
    }

    overlap_x = EGUI_MIN(a->x + a->size, b->x + b->size) - EGUI_MAX(a->x, b->x);
    overlap_y = EGUI_MIN(a->y + a->size, b->y + b->size) - EGUI_MAX(a->y, b->y);

    if (overlap_x <= overlap_y)
    {
        egui_dim_t shift_a = overlap_x / 2;
        egui_dim_t shift_b = overlap_x - shift_a;

        if (a->x <= b->x)
        {
            a->x -= shift_a;
            b->x += shift_b;
        }
        else
        {
            a->x += shift_b;
            b->x -= shift_a;
        }

        tmp = a->vx;
        a->vx = b->vx;
        b->vx = tmp;
    }
    else
    {
        egui_dim_t shift_a = overlap_y / 2;
        egui_dim_t shift_b = overlap_y - shift_a;

        if (a->y <= b->y)
        {
            a->y -= shift_a;
            b->y += shift_b;
        }
        else
        {
            a->y += shift_b;
            b->y -= shift_a;
        }

        tmp = a->vy;
        a->vy = b->vy;
        b->vy = tmp;
    }
}

static void egui_view_test_step_blocks(egui_view_test_t *local)
{
    egui_view_t *self = EGUI_VIEW_OF(local);
    egui_dim_t left;
    egui_dim_t top;
    egui_dim_t right;
    egui_dim_t bottom;
    uint8_t i;

    egui_view_test_get_block_bounds(self, &left, &top, &right, &bottom);

    for (i = 0; i < 2; i++)
    {
        egui_view_test_block_t *block = &local->blocks[i];

        block->x += block->vx;
        block->y += block->vy;

        if (block->x < left)
        {
            block->x = left;
            block->vx = -block->vx;
        }
        else if (block->x + block->size > right)
        {
            block->x = right - block->size;
            block->vx = -block->vx;
        }

        if (block->y < top)
        {
            block->y = top;
            block->vy = -block->vy;
        }
        else if (block->y + block->size > bottom)
        {
            block->y = bottom - block->size;
            block->vy = -block->vy;
        }
    }

    egui_view_test_resolve_blocks(&local->blocks[0], &local->blocks[1]);
}

static void egui_view_test_eval_popup(const egui_region_t *panel, uint16_t phase, egui_dim_t *center_y, int16_t *scale_q8)
{
    egui_dim_t hidden_y = panel->location.y + panel->size.height + 18;
    egui_dim_t enter_y = panel->location.y + panel->size.height - 18;
    egui_dim_t peak_y = panel->location.y + panel->size.height - 32;
    egui_dim_t settle_y = panel->location.y + panel->size.height - 24;

    if (phase < 32)
    {
        *center_y = egui_view_test_lerp(hidden_y, enter_y, phase, 31);
        *scale_q8 = (int16_t)egui_view_test_lerp(96, 218, phase, 31);
    }
    else if (phase < 56)
    {
        phase -= 32;
        *center_y = egui_view_test_lerp(enter_y, peak_y, phase, 23);
        *scale_q8 = (int16_t)egui_view_test_lerp(218, 248, phase, 23);
    }
    else if (phase < 80)
    {
        phase -= 56;
        *center_y = egui_view_test_lerp(peak_y, settle_y, phase, 23);
        *scale_q8 = (int16_t)egui_view_test_lerp(248, 228, phase, 23);
    }
    else if (phase < 104)
    {
        phase -= 80;
        *center_y = egui_view_test_lerp(settle_y, enter_y, phase, 23);
        *scale_q8 = (int16_t)egui_view_test_lerp(228, 236, phase, 23);
    }
    else
    {
        phase -= 104;
        *center_y = egui_view_test_lerp(enter_y, hidden_y, phase, 23);
        *scale_q8 = (int16_t)egui_view_test_lerp(236, 96, phase, 23);
    }
}

static void egui_view_test_get_text_items(const egui_region_t *panel, const egui_view_test_t *local, uint16_t phase, egui_view_test_transform_item_t *cn_item,
                                          egui_view_test_transform_item_t *en_item)
{
    egui_dim_t panel_center_x;
    egui_dim_t panel_center_y;
    EGUI_UNUSED(phase);

    panel_center_x = panel->location.x + panel->size.width / 2;
    panel_center_y = panel->location.y + panel->size.height / 2;

    if (cn_item != NULL)
    {
        cn_item->x = panel_center_x - EGUI_VIEW_TEST_ARC_RADIUS - local->text_cn_w - EGUI_VIEW_TEST_TEXT_SIDE_GAP;
        cn_item->y = panel_center_y - local->text_cn_h / 2 - 4;
        cn_item->width = local->text_cn_w;
        cn_item->height = local->text_cn_h;
        cn_item->angle_deg = 0;
        cn_item->scale_q8 = 256;
    }

    if (en_item != NULL)
    {
        en_item->x = panel_center_x + EGUI_VIEW_TEST_ARC_RADIUS + EGUI_VIEW_TEST_TEXT_SIDE_GAP;
        en_item->y = panel_center_y - local->text_en_h / 2 - 4;
        en_item->width = local->text_en_w;
        en_item->height = local->text_en_h;
        en_item->angle_deg = 0;
        en_item->scale_q8 = 256;
    }
}

static void egui_view_test_get_popup_item(const egui_region_t *panel, uint16_t phase, int16_t angle_deg, egui_view_test_transform_item_t *popup_item)
{
    egui_dim_t center_x;
    egui_dim_t center_y;
    int16_t scale_q8;

    if (popup_item == NULL)
    {
        return;
    }

    egui_view_test_get_panel_center(panel, &center_x, NULL);
    egui_view_test_eval_popup(panel, phase, &center_y, &scale_q8);

    popup_item->x = center_x - EGUI_VIEW_TEST_IMAGE_BASE_HALF;
    popup_item->y = center_y - EGUI_VIEW_TEST_IMAGE_BASE_HALF;
    popup_item->width = EGUI_VIEW_TEST_IMAGE_BASE_SIZE;
    popup_item->height = EGUI_VIEW_TEST_IMAGE_BASE_SIZE;
    popup_item->angle_deg = angle_deg;
    popup_item->scale_q8 = scale_q8;
}

static void egui_view_test_add_transform_region(egui_region_t *dirty_region, const egui_view_test_transform_item_t *item, egui_dim_t pad)
{
    egui_float_t angle_rad;
    egui_float_t cos_val;
    egui_float_t sin_val;
    egui_dim_t scaled_half_w;
    egui_dim_t scaled_half_h;
    egui_dim_t bbox_half_w;
    egui_dim_t bbox_half_h;
    egui_dim_t center_x;
    egui_dim_t center_y;

    if (dirty_region == NULL || item == NULL || item->width <= 0 || item->height <= 0 || item->scale_q8 <= 0)
    {
        return;
    }

    scaled_half_w = (egui_dim_t)((((int32_t)item->width * item->scale_q8) + 511) / 512);
    scaled_half_h = (egui_dim_t)((((int32_t)item->height * item->scale_q8) + 511) / 512);
    if (scaled_half_w <= 0)
    {
        scaled_half_w = 1;
    }
    if (scaled_half_h <= 0)
    {
        scaled_half_h = 1;
    }

    angle_rad = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(item->angle_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));
    cos_val = EGUI_FLOAT_COS(angle_rad);
    sin_val = EGUI_FLOAT_SIN(angle_rad);
    if (cos_val < 0)
    {
        cos_val = -cos_val;
    }
    if (sin_val < 0)
    {
        sin_val = -sin_val;
    }

    bbox_half_w = EGUI_FLOAT_INT_PART((EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(scaled_half_w), cos_val) +
                                       EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(scaled_half_h), sin_val))) +
                  1;
    bbox_half_h = EGUI_FLOAT_INT_PART((EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(scaled_half_w), sin_val) +
                                       EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(scaled_half_h), cos_val))) +
                  1;

    center_x = item->x + item->width / 2;
    center_y = item->y + item->height / 2;
    egui_view_circle_dirty_add_rect_region(dirty_region, center_x - bbox_half_w, center_y - bbox_half_h, bbox_half_w * 2 + 1, bbox_half_h * 2 + 1, pad);
}

static void egui_view_test_invalidate_arc_change(egui_view_t *self, const egui_region_t *panel, uint16_t old_sweep, uint16_t new_sweep)
{
    egui_region_t dirty_region;
    egui_region_t arc_region;
    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t start_x;
    egui_dim_t start_y;
    egui_dim_t end_x;
    egui_dim_t end_y;
    egui_dim_t mid_radius = EGUI_VIEW_TEST_ARC_RADIUS - EGUI_VIEW_TEST_ARC_STROKE / 2;
    egui_dim_t expand_radius = EGUI_VIEW_TEST_ARC_STROKE / 2 + EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 1;
    uint16_t dirty_min_sweep;
    uint16_t dirty_max_sweep;
    uint16_t dirty_sweep;
    int16_t dirty_start_angle;
    int16_t old_end_angle;
    int16_t new_end_angle;
    egui_dim_t cap_radius = expand_radius;

    if (old_sweep == new_sweep)
    {
        return;
    }

    egui_region_init_empty(&dirty_region);
    egui_view_test_get_panel_center(panel, &center_x, &center_y);
    egui_view_circle_dirty_get_circle_point(center_x, center_y, mid_radius, -90, &start_x, &start_y);
    dirty_min_sweep = old_sweep < new_sweep ? old_sweep : new_sweep;
    dirty_max_sweep = old_sweep > new_sweep ? old_sweep : new_sweep;
    dirty_sweep = dirty_max_sweep - dirty_min_sweep;
    dirty_start_angle = -90 + (int16_t)dirty_min_sweep;

    if (dirty_sweep > 0 &&
        egui_view_circle_dirty_compute_arc_region(center_x, center_y, mid_radius, expand_radius, dirty_start_angle, dirty_sweep, &arc_region))
    {
        egui_view_circle_dirty_union_region(&dirty_region, &arc_region);
    }

    if ((old_sweep == 0 && new_sweep > 0) || (new_sweep == 0 && old_sweep > 0))
    {
        egui_view_circle_dirty_add_circle_region(&dirty_region, start_x, start_y, cap_radius, 1);
    }

    if (old_sweep > 0)
    {
        old_end_angle = -90 + (int16_t)old_sweep;
        egui_view_circle_dirty_get_circle_point(center_x, center_y, mid_radius, old_end_angle, &end_x, &end_y);
        egui_view_circle_dirty_add_circle_region(&dirty_region, end_x, end_y, cap_radius, 1);
    }

    if (new_sweep > 0)
    {
        new_end_angle = -90 + (int16_t)new_sweep;
        egui_view_circle_dirty_get_circle_point(center_x, center_y, mid_radius, new_end_angle, &end_x, &end_y);
        egui_view_circle_dirty_add_circle_region(&dirty_region, end_x, end_y, cap_radius, 1);
    }

    if (egui_region_is_empty(&dirty_region))
    {
        return;
    }

    egui_view_invalidate_region(self, &dirty_region);
}

static void egui_view_test_invalidate_popup_change(egui_view_t *self, const egui_region_t *panel, uint16_t old_phase, int16_t old_angle, uint16_t new_phase,
                                                   int16_t new_angle)
{
    egui_region_t dirty_region;
    egui_view_test_transform_item_t popup_item;

    egui_region_init_empty(&dirty_region);
    egui_view_test_get_popup_item(panel, old_phase, old_angle, &popup_item);
    egui_view_test_add_transform_region(&dirty_region, &popup_item, EGUI_VIEW_TEST_DIRTY_PAD);
    egui_view_test_get_popup_item(panel, new_phase, new_angle, &popup_item);
    egui_view_test_add_transform_region(&dirty_region, &popup_item, EGUI_VIEW_TEST_DIRTY_PAD);

    if (!egui_region_is_empty(&dirty_region))
    {
        egui_view_invalidate_region(self, &dirty_region);
    }
}

static void egui_view_test_invalidate_blocks_change(egui_view_t *self, const egui_view_test_block_t *old_blocks, const egui_view_test_block_t *new_blocks)
{
    uint8_t i;

    for (i = 0; i < 2; i++)
    {
        egui_region_t dirty_region;

        egui_region_init_empty(&dirty_region);
        egui_view_test_add_block_dirty(&dirty_region, i, &old_blocks[i]);
        egui_view_test_add_block_dirty(&dirty_region, i, &new_blocks[i]);

        if (!egui_region_is_empty(&dirty_region))
        {
            egui_view_invalidate_region(self, &dirty_region);
        }
    }
}

static void egui_view_test_draw_text_anim(const egui_region_t *panel, const egui_view_test_t *local, uint16_t phase)
{
    static const char text_cn[] = "\xE4\xBD\xA0\xE5\xA5\xBD";
    static const char text_en[] = "Hello";
    const egui_font_t *font = EGUI_FONT_OF(&egui_res_font_notosanssc_16_4);
    egui_view_test_transform_item_t cn_item;
    egui_view_test_transform_item_t en_item;

    if (font == NULL || local == NULL || !local->metrics_ready)
    {
        return;
    }

    egui_view_test_get_text_items(panel, local, phase, &cn_item, &en_item);

    egui_canvas_draw_text(font, text_cn, cn_item.x, cn_item.y, egui_view_test_color_make(176, 230, 248), EGUI_ALPHA_90);
    egui_canvas_draw_text(font, text_en, en_item.x, en_item.y, egui_view_test_color_make(231, 212, 170), EGUI_ALPHA_80);
}

static void egui_view_test_timer_callback(egui_timer_t *timer)
{
    egui_view_test_t *local = (egui_view_test_t *)timer->user_data;
    egui_view_t *self = EGUI_VIEW_OF(local);
    egui_region_t panel;
    egui_view_test_block_t old_blocks[2];
    uint16_t old_arc_sweep = local->arc_sweep;
    uint16_t old_popup_phase = local->popup_phase;
    int16_t old_popup_angle = local->popup_angle;
    uint8_t i;

    egui_view_test_ensure_metrics(local);
    for (i = 0; i < EGUI_ARRAY_SIZE(old_blocks); i++)
    {
        old_blocks[i] = local->blocks[i];
    }

    if (local->arc_direction > 0)
    {
        if (local->arc_sweep + EGUI_VIEW_TEST_ARC_STEP >= 360)
        {
            local->arc_sweep = 360;
            local->arc_direction = -1;
        }
        else
        {
            local->arc_sweep += EGUI_VIEW_TEST_ARC_STEP;
        }
    }
    else
    {
        if (local->arc_sweep <= EGUI_VIEW_TEST_ARC_STEP)
        {
            local->arc_sweep = 0;
            local->arc_direction = 1;
        }
        else
        {
            local->arc_sweep -= EGUI_VIEW_TEST_ARC_STEP;
        }
    }

    local->popup_phase = (local->popup_phase + 1) % EGUI_VIEW_TEST_POPUP_PHASE_MAX;
    local->popup_angle = (int16_t)((local->popup_angle + 6) % 360);

    egui_view_test_step_blocks(local);
    egui_view_test_get_panel_region(self, &panel);
    if (self->region_screen.size.width <= 0 || self->region_screen.size.height <= 0 || panel.size.width <= 0 || panel.size.height <= 0)
    {
        egui_view_invalidate(self);
        return;
    }

    egui_view_test_invalidate_arc_change(self, &panel, old_arc_sweep, local->arc_sweep);
    egui_view_test_invalidate_popup_change(self, &panel, old_popup_phase, old_popup_angle, local->popup_phase, local->popup_angle);
    egui_view_test_invalidate_blocks_change(self, old_blocks, local->blocks);
}

static void egui_view_test_on_attach_to_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_test_t);

    egui_view_on_attach_to_window(self);
    egui_timer_start_timer(&local->anim_timer, EGUI_VIEW_TEST_TIMER_INTERVAL_MS, EGUI_VIEW_TEST_TIMER_INTERVAL_MS);
    egui_view_invalidate(self);
}

static void egui_view_test_on_detach_from_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_test_t);

    egui_timer_stop_timer(&local->anim_timer);
    egui_view_on_detach_from_window(self);
}

static void egui_view_test_draw_blocks(const egui_view_test_t *local)
{
    uint8_t i;

    for (i = 0; i < 2; i++)
    {
        const egui_view_test_block_t *block = &local->blocks[i];

        if (egui_view_test_block_is_circle(i))
        {
            egui_canvas_draw_circle_fill(block->x + block->size / 2, block->y + block->size / 2, block->size / 2, block->fill_color, block->alpha);
        }
        else
        {
            egui_canvas_draw_rectangle_fill(block->x, block->y, block->size, block->size, block->fill_color, block->alpha);
        }
    }
}

static void egui_view_test_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_test_t);
    egui_region_t panel;
    egui_dim_t panel_center_x;
    egui_dim_t panel_center_y;
    egui_dim_t photo_x;
    egui_dim_t photo_y;
    egui_dim_t photo_w;
    egui_dim_t photo_h;
    egui_dim_t popup_center_y;
    egui_dim_t popup_anchor_x;
    egui_dim_t popup_anchor_y;
    int16_t popup_scale_q8;

    egui_view_test_ensure_metrics(local);
    egui_view_test_get_panel_region(self, &panel);
    photo_w = local->photo_w;
    photo_h = local->photo_h;

    egui_view_test_get_panel_center(&panel, &panel_center_x, &panel_center_y);
    photo_x = panel_center_x - photo_w / 2;
    photo_y = panel_center_y - photo_h / 2;

    egui_mask_set_position((egui_mask_t *)&local->photo_mask, photo_x, photo_y);
    egui_mask_set_size((egui_mask_t *)&local->photo_mask, photo_w, photo_h);
    egui_canvas_set_mask((egui_mask_t *)&local->photo_mask);
    egui_canvas_draw_image((egui_image_t *)&egui_res_image_test_rgb565_8, photo_x, photo_y);
    egui_canvas_clear_mask();

    egui_canvas_draw_circle_hq(panel_center_x, panel_center_y, EGUI_VIEW_TEST_ARC_RADIUS, EGUI_VIEW_TEST_ARC_STROKE, egui_view_test_color_make(37, 58, 76),
                               EGUI_ALPHA_70);

    if (local->arc_sweep > 0)
    {
        egui_canvas_draw_arc_round_cap_sweep_hq(panel_center_x, panel_center_y, EGUI_VIEW_TEST_ARC_RADIUS, -90, (int16_t)local->arc_sweep,
                                                EGUI_VIEW_TEST_ARC_STROKE, egui_view_test_color_make(255, 200, 82), EGUI_ALPHA_100);
    }

    egui_view_test_draw_text_anim(&panel, local, local->popup_phase);

    egui_view_test_eval_popup(&panel, local->popup_phase, &popup_center_y, &popup_scale_q8);
    popup_anchor_x = panel_center_x - EGUI_VIEW_TEST_IMAGE_BASE_HALF;
    popup_anchor_y = popup_center_y - EGUI_VIEW_TEST_IMAGE_BASE_HALF;
    egui_canvas_draw_image_rotate_scale((egui_image_t *)&egui_res_image_star_rgb565_8, popup_anchor_x, popup_anchor_y, local->popup_angle, popup_scale_q8);

    egui_view_test_draw_blocks(local);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_test_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_test_on_attach_to_window,
        .on_draw = egui_view_test_on_draw,
        .on_detach_from_window = egui_view_test_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_test_init(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_test_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_test_t);

    local->arc_sweep = 0;
    local->arc_direction = 1;
    local->popup_phase = 0;
    local->popup_angle = 0;
    egui_view_test_reset_blocks(local);

    egui_timer_init_timer(&local->anim_timer, local, egui_view_test_timer_callback);
    egui_timer_start_timer(&local->anim_timer, EGUI_VIEW_TEST_TIMER_INTERVAL_MS, EGUI_VIEW_TEST_TIMER_INTERVAL_MS);
    egui_mask_circle_init((egui_mask_t *)&local->photo_mask);
    egui_view_test_ensure_metrics(local);

    egui_view_set_view_name(self, "egui_view_test");
}
