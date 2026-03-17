#include <string.h>

#include "egui_view_swipe_control.h"

#define SWC_STD_RADIUS      12
#define SWC_STD_PAD_X       10
#define SWC_STD_PAD_Y       9
#define SWC_STD_TITLE_H     10
#define SWC_STD_TITLE_GAP   5
#define SWC_STD_HELPER_H    10
#define SWC_STD_HELPER_GAP  5
#define SWC_STD_ACTION_W    40
#define SWC_STD_ACTION_GAP  6
#define SWC_STD_SHADOW_X    2
#define SWC_STD_SHADOW_Y    4
#define SWC_STD_SWIPE_DELTA 14

#define SWC_COMPACT_RADIUS     9
#define SWC_COMPACT_PAD_X      7
#define SWC_COMPACT_PAD_Y      6
#define SWC_COMPACT_ACTION_W   32
#define SWC_COMPACT_ACTION_GAP 4

typedef struct egui_view_swipe_control_metrics egui_view_swipe_control_metrics_t;
struct egui_view_swipe_control_metrics
{
    egui_region_t title_region;
    egui_region_t surface_region;
    egui_region_t start_action_region;
    egui_region_t end_action_region;
    egui_region_t helper_region;
    uint8_t show_title;
    uint8_t show_helper;
};

static uint8_t swipe_control_has_text(const char *text)
{
    return (text != NULL && text[0] != '\0') ? 1 : 0;
}

static void swipe_control_draw_round_fill_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_color_t color,
                                               egui_alpha_t alpha)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle_fill(x, y, w, h, radius, color, alpha);
}

static void swipe_control_draw_round_stroke_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_dim_t stroke_width,
                                                 egui_color_t color, egui_alpha_t alpha)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(x, y, w, h, radius, stroke_width, color, alpha);
}

static uint8_t swipe_control_part_enabled(egui_view_swipe_control_t *local, egui_view_t *self, uint8_t part)
{
    if (!egui_view_get_enable(self) || local->item == NULL || local->compact_mode || local->read_only_mode)
    {
        return 0;
    }

    if (part == EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE)
    {
        return 1;
    }
    if (part == EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION)
    {
        return local->start_action != NULL ? 1 : 0;
    }
    if (part == EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION)
    {
        return local->end_action != NULL ? 1 : 0;
    }
    return 0;
}

static void swipe_control_normalize_state(egui_view_swipe_control_t *local)
{
    if (local->item == NULL)
    {
        local->reveal_state = EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE;
        local->current_part = EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE;
        local->pressed_part = EGUI_VIEW_SWIPE_CONTROL_PART_NONE;
        local->dragging = 0;
        return;
    }

    if (local->compact_mode || local->read_only_mode)
    {
        local->reveal_state = EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE;
        local->current_part = EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE;
        local->pressed_part = EGUI_VIEW_SWIPE_CONTROL_PART_NONE;
        local->dragging = 0;
        return;
    }

    if (local->reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_START && local->start_action == NULL)
    {
        local->reveal_state = EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE;
    }
    if (local->reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_END && local->end_action == NULL)
    {
        local->reveal_state = EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE;
    }

    if (local->reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_START)
    {
        local->current_part = EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION;
    }
    else if (local->reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_END)
    {
        local->current_part = EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION;
    }
    else
    {
        local->current_part = EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE;
    }

    if (local->pressed_part != EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE && local->pressed_part != EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION &&
        local->pressed_part != EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION)
    {
        local->pressed_part = EGUI_VIEW_SWIPE_CONTROL_PART_NONE;
    }
}

static void swipe_control_notify(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);

    if (local->on_changed != NULL)
    {
        local->on_changed(self, local->reveal_state, local->current_part);
    }
}

static void swipe_control_set_state_inner(egui_view_t *self, uint8_t reveal_state, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    uint8_t changed = 0;

    if (local->item == NULL)
    {
        reveal_state = EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE;
    }
    if (reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_START && local->start_action == NULL)
    {
        reveal_state = EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE;
    }
    if (reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_END && local->end_action == NULL)
    {
        reveal_state = EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE;
    }
    if (local->compact_mode || local->read_only_mode || !egui_view_get_enable(self))
    {
        reveal_state = EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE;
    }

    swipe_control_normalize_state(local);
    if (local->reveal_state != reveal_state)
    {
        local->reveal_state = reveal_state;
        changed = 1;
    }

    if (reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_START)
    {
        if (local->current_part != EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION)
        {
            local->current_part = EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION;
            changed = 1;
        }
    }
    else if (reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_END)
    {
        if (local->current_part != EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION)
        {
            local->current_part = EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION;
            changed = 1;
        }
    }
    else if (local->current_part != EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE)
    {
        local->current_part = EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE;
        changed = 1;
    }

    if (changed)
    {
        egui_view_invalidate(self);
    }
    if (notify)
    {
        swipe_control_notify(self);
    }
}

static void swipe_control_reveal_preferred(egui_view_t *self, uint8_t preferred_reveal, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);

    if (preferred_reveal == EGUI_VIEW_SWIPE_CONTROL_REVEAL_START)
    {
        if (swipe_control_part_enabled(local, self, EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION))
        {
            swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_START, notify);
            return;
        }
        if (swipe_control_part_enabled(local, self, EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION))
        {
            swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_END, notify);
            return;
        }
    }
    else
    {
        if (swipe_control_part_enabled(local, self, EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION))
        {
            swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_END, notify);
            return;
        }
        if (swipe_control_part_enabled(local, self, EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION))
        {
            swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_START, notify);
            return;
        }
    }

    swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE, notify);
}

static void swipe_control_cycle_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);

    swipe_control_normalize_state(local);
    if (local->current_part == EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE)
    {
        if (swipe_control_part_enabled(local, self, EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION))
        {
            swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_START, 1);
        }
        else if (swipe_control_part_enabled(local, self, EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION))
        {
            swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_END, 1);
        }
        else
        {
            swipe_control_notify(self);
        }
        return;
    }

    if (local->current_part == EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION && swipe_control_part_enabled(local, self, EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION))
    {
        swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_END, 1);
    }
    else
    {
        swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE, 1);
    }
}

static void swipe_control_get_metrics(egui_view_swipe_control_t *local, egui_view_t *self, egui_view_swipe_control_metrics_t *metrics)
{
    egui_region_t work_region;
    egui_dim_t pad_x = local->compact_mode ? SWC_COMPACT_PAD_X : SWC_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? SWC_COMPACT_PAD_Y : SWC_STD_PAD_Y;
    egui_dim_t action_w = local->compact_mode ? SWC_COMPACT_ACTION_W : SWC_STD_ACTION_W;
    egui_dim_t action_gap = local->compact_mode ? SWC_COMPACT_ACTION_GAP : SWC_STD_ACTION_GAP;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t content_h;

    swipe_control_normalize_state(local);
    egui_view_get_work_region(self, &work_region);

    content_x = work_region.location.x + pad_x;
    content_y = work_region.location.y + pad_y;
    content_w = work_region.size.width - pad_x * 2;
    content_h = work_region.size.height - pad_y * 2;

    metrics->show_title = (!local->compact_mode && swipe_control_has_text(local->title)) ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && swipe_control_has_text(local->helper)) ? 1 : 0;

    metrics->title_region.location.x = content_x;
    metrics->title_region.location.y = content_y;
    metrics->title_region.size.width = content_w;
    metrics->title_region.size.height = SWC_STD_TITLE_H;

    if (metrics->show_title)
    {
        content_y += SWC_STD_TITLE_H + SWC_STD_TITLE_GAP;
        content_h -= SWC_STD_TITLE_H + SWC_STD_TITLE_GAP;
    }
    if (metrics->show_helper)
    {
        content_h -= SWC_STD_HELPER_H + SWC_STD_HELPER_GAP;
    }

    metrics->helper_region.location.x = content_x;
    metrics->helper_region.location.y = content_y + content_h + SWC_STD_HELPER_GAP;
    metrics->helper_region.size.width = content_w;
    metrics->helper_region.size.height = SWC_STD_HELPER_H;

    metrics->start_action_region.location.x = content_x;
    metrics->start_action_region.location.y = content_y;
    metrics->start_action_region.size.width = 0;
    metrics->start_action_region.size.height = 0;
    metrics->end_action_region = metrics->start_action_region;

    metrics->surface_region.location.x = content_x;
    metrics->surface_region.location.y = content_y;
    metrics->surface_region.size.width = content_w;
    metrics->surface_region.size.height = content_h;

    if (action_w > content_w - 18)
    {
        action_w = content_w - 18;
    }
    if (action_w < 0)
    {
        action_w = 0;
    }
    if (metrics->surface_region.size.height < 20)
    {
        metrics->surface_region.size.height = 20;
    }

    if (local->reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_START && local->start_action != NULL && action_w > 0 && content_w > action_w + action_gap + 40)
    {
        metrics->start_action_region.size.width = action_w;
        metrics->start_action_region.size.height = content_h;
        metrics->surface_region.location.x = content_x + action_w + action_gap;
        metrics->surface_region.size.width = content_w - action_w - action_gap;
    }
    else if (local->reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_END && local->end_action != NULL && action_w > 0 && content_w > action_w + action_gap + 40)
    {
        metrics->surface_region.size.width = content_w - action_w - action_gap;
        metrics->end_action_region.location.x = metrics->surface_region.location.x + metrics->surface_region.size.width + action_gap;
        metrics->end_action_region.location.y = content_y;
        metrics->end_action_region.size.width = action_w;
        metrics->end_action_region.size.height = content_h;
    }
}

static void swipe_control_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                    egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (!swipe_control_has_text(text))
    {
        return;
    }
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void swipe_control_draw_focus(egui_view_t *self, const egui_region_t *region, egui_dim_t radius, egui_color_t color)
{
    swipe_control_draw_round_stroke_safe(region->location.x - 1, region->location.y - 1, region->size.width + 2, region->size.height + 2, radius, 1, color,
                                         egui_color_alpha_mix(self->alpha, 76));
}

static void swipe_control_draw_action(egui_view_t *self, egui_view_swipe_control_t *local, const egui_region_t *region,
                                      const egui_view_swipe_control_action_t *action, uint8_t is_pressed)
{
    egui_region_t text_region;
    egui_color_t fill_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_dim_t radius = local->compact_mode ? 9 : 10;

    if (action == NULL || region->size.width <= 0 || region->size.height <= 0)
    {
        return;
    }

    fill_color = action->fill_color;
    border_color = egui_rgb_mix(action->fill_color, local->border_color, 18);
    text_color = action->text_color;

    if (local->read_only_mode)
    {
        fill_color = egui_rgb_mix(fill_color, local->inactive_color, 34);
        border_color = egui_rgb_mix(border_color, local->inactive_color, 24);
        text_color = egui_rgb_mix(text_color, local->inactive_color, 38);
    }
    else if (is_pressed)
    {
        fill_color = egui_rgb_mix(fill_color, EGUI_COLOR_HEX(0x0A1724), 18);
    }

    swipe_control_draw_round_fill_safe(region->location.x, region->location.y, region->size.width, region->size.height, radius, fill_color,
                                       egui_color_alpha_mix(self->alpha, 94));
    swipe_control_draw_round_stroke_safe(region->location.x, region->location.y, region->size.width, region->size.height, radius, 1, border_color,
                                         egui_color_alpha_mix(self->alpha, 58));

    text_region.location.x = region->location.x + 4;
    text_region.location.y = region->location.y + (local->compact_mode ? 10 : 12);
    text_region.size.width = region->size.width - 8;
    text_region.size.height = 12;
    swipe_control_draw_text(local->font, self, action->label, &text_region, EGUI_ALIGN_CENTER, text_color);

    if (!local->compact_mode)
    {
        text_region.location.y = region->location.y + region->size.height - 18;
        text_region.size.height = 10;
        swipe_control_draw_text(local->meta_font, self, action->hint, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(text_color, EGUI_COLOR_HEX(0xFFFFFF), 24));
    }
}

static void swipe_control_draw_state_pill(egui_view_t *self, egui_view_swipe_control_t *local, const egui_view_swipe_control_item_t *item,
                                          const egui_region_t *surface_region)
{
    const char *state_text = "Ready";
    egui_region_t region;
    egui_dim_t pill_w;
    egui_color_t fill_color;

    if (local->read_only_mode)
    {
        state_text = "Locked";
    }
    else if (local->reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_START)
    {
        state_text = "Start";
    }
    else if (local->reveal_state == EGUI_VIEW_SWIPE_CONTROL_REVEAL_END)
    {
        state_text = "End";
    }

    pill_w = (egui_dim_t)(28 + strlen(state_text) * 4);
    if (pill_w < 42)
    {
        pill_w = 42;
    }
    if (pill_w > surface_region->size.width - 28)
    {
        pill_w = surface_region->size.width - 28;
    }

    region.location.x = surface_region->location.x + surface_region->size.width - pill_w - 10;
    region.location.y = surface_region->location.y + 10;
    region.size.width = pill_w;
    region.size.height = local->compact_mode ? 11 : 12;

    fill_color = egui_rgb_mix(item->accent_color, local->surface_color, local->read_only_mode ? 42 : 18);
    swipe_control_draw_round_fill_safe(region.location.x, region.location.y, region.size.width, region.size.height, region.size.height / 2, fill_color,
                                       egui_color_alpha_mix(self->alpha, 88));
    swipe_control_draw_round_stroke_safe(region.location.x, region.location.y, region.size.width, region.size.height, region.size.height / 2, 1,
                                         egui_rgb_mix(item->accent_color, local->border_color, 18), egui_color_alpha_mix(self->alpha, 54));
    swipe_control_draw_text(local->meta_font, self, state_text, &region, EGUI_ALIGN_CENTER, local->text_color);
}

static void swipe_control_draw_surface(egui_view_t *self, egui_view_swipe_control_t *local, const egui_view_swipe_control_item_t *item,
                                       const egui_view_swipe_control_metrics_t *metrics)
{
    egui_region_t text_region;
    egui_region_t badge_region;
    egui_dim_t radius = local->compact_mode ? 10 : 12;
    egui_color_t shell_color = egui_rgb_mix(item->surface_color, local->surface_color, local->read_only_mode ? 42 : 14);
    egui_color_t accent_color = local->read_only_mode ? egui_rgb_mix(item->accent_color, local->inactive_color, 44) : item->accent_color;
    egui_color_t title_color = local->read_only_mode ? egui_rgb_mix(local->text_color, local->inactive_color, 30) : local->text_color;
    egui_color_t body_color = local->read_only_mode ? egui_rgb_mix(local->muted_text_color, local->inactive_color, 26)
                                                    : egui_rgb_mix(local->text_color, local->muted_text_color, 28);
    egui_dim_t badge_w;

    swipe_control_draw_round_fill_safe(metrics->surface_region.location.x + SWC_STD_SHADOW_X, metrics->surface_region.location.y + SWC_STD_SHADOW_Y,
                                       metrics->surface_region.size.width, metrics->surface_region.size.height, radius,
                                       egui_rgb_mix(local->border_color, EGUI_COLOR_HEX(0x081018), 56), egui_color_alpha_mix(self->alpha, 24));
    swipe_control_draw_round_fill_safe(metrics->surface_region.location.x, metrics->surface_region.location.y, metrics->surface_region.size.width,
                                       metrics->surface_region.size.height, radius, shell_color, egui_color_alpha_mix(self->alpha, 100));
    swipe_control_draw_round_stroke_safe(metrics->surface_region.location.x, metrics->surface_region.location.y, metrics->surface_region.size.width,
                                         metrics->surface_region.size.height, radius, 1, egui_rgb_mix(accent_color, local->border_color, 24),
                                         egui_color_alpha_mix(self->alpha, 62));

    swipe_control_draw_round_fill_safe(metrics->surface_region.location.x + 10, metrics->surface_region.location.y + 11, 10,
                                       metrics->surface_region.size.height - 22, 5, accent_color, egui_color_alpha_mix(self->alpha, 86));

    badge_w = (egui_dim_t)(22 + strlen(item->eyebrow ? item->eyebrow : "") * 4);
    if (badge_w < 40)
    {
        badge_w = 40;
    }
    if (badge_w > metrics->surface_region.size.width - 70)
    {
        badge_w = metrics->surface_region.size.width - 70;
    }
    badge_region.location.x = metrics->surface_region.location.x + 26;
    badge_region.location.y = metrics->surface_region.location.y + 10;
    badge_region.size.width = badge_w;
    badge_region.size.height = local->compact_mode ? 11 : 12;
    swipe_control_draw_round_fill_safe(badge_region.location.x, badge_region.location.y, badge_region.size.width, badge_region.size.height,
                                       badge_region.size.height / 2, accent_color, egui_color_alpha_mix(self->alpha, local->read_only_mode ? 58 : 84));
    swipe_control_draw_text(local->meta_font, self, item->eyebrow, &badge_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0xFFFFFF));

    swipe_control_draw_state_pill(self, local, item, &metrics->surface_region);

    text_region.location.x = metrics->surface_region.location.x + 26;
    text_region.location.y = metrics->surface_region.location.y + (local->compact_mode ? 29 : 33);
    text_region.size.width = metrics->surface_region.size.width - 36;
    text_region.size.height = local->compact_mode ? 13 : 15;
    swipe_control_draw_text(local->font, self, item->title, &text_region, EGUI_ALIGN_LEFT, title_color);

    if (!local->compact_mode)
    {
        text_region.location.y += 19;
        text_region.size.height = 22;
        swipe_control_draw_text(local->meta_font, self, item->description, &text_region, EGUI_ALIGN_LEFT, body_color);

        egui_canvas_draw_line(metrics->surface_region.location.x + 12, metrics->surface_region.location.y + metrics->surface_region.size.height - 28,
                              metrics->surface_region.location.x + metrics->surface_region.size.width - 12,
                              metrics->surface_region.location.y + metrics->surface_region.size.height - 28, 1,
                              egui_rgb_mix(accent_color, local->surface_color, 34), egui_color_alpha_mix(self->alpha, 62));

        text_region.location.y = metrics->surface_region.location.y + metrics->surface_region.size.height - 20;
        text_region.size.height = 10;
        swipe_control_draw_text(local->meta_font, self, item->footer, &text_region, EGUI_ALIGN_LEFT, egui_rgb_mix(body_color, accent_color, 20));
    }
    else
    {
        text_region.location.y = metrics->surface_region.location.y + metrics->surface_region.size.height - 16;
        text_region.size.height = 10;
        swipe_control_draw_text(local->meta_font, self, item->footer, &text_region, EGUI_ALIGN_LEFT, egui_rgb_mix(body_color, accent_color, 22));
    }
}

void egui_view_swipe_control_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_swipe_control_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_swipe_control_set_title(egui_view_t *self, const char *title)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    local->title = title;
    egui_view_invalidate(self);
}

void egui_view_swipe_control_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_swipe_control_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                         egui_color_t muted_text_color, egui_color_t inactive_color)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->inactive_color = inactive_color;
    egui_view_invalidate(self);
}

void egui_view_swipe_control_set_item(egui_view_t *self, const egui_view_swipe_control_item_t *item)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    local->item = item;
    swipe_control_normalize_state(local);
    egui_view_invalidate(self);
}

const egui_view_swipe_control_item_t *egui_view_swipe_control_get_item(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    return local->item;
}

void egui_view_swipe_control_set_actions(egui_view_t *self, const egui_view_swipe_control_action_t *start_action,
                                         const egui_view_swipe_control_action_t *end_action)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    local->start_action = start_action;
    local->end_action = end_action;
    swipe_control_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_swipe_control_set_current_part(egui_view_t *self, uint8_t part)
{
    if (part == EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION)
    {
        swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_START, 0);
    }
    else if (part == EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION)
    {
        swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_END, 0);
    }
    else
    {
        swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE, 0);
    }
}

uint8_t egui_view_swipe_control_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    swipe_control_normalize_state(local);
    return local->current_part;
}

void egui_view_swipe_control_set_reveal_state(egui_view_t *self, uint8_t reveal_state)
{
    swipe_control_set_state_inner(self, reveal_state, 0);
}

uint8_t egui_view_swipe_control_get_reveal_state(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    swipe_control_normalize_state(local);
    return local->reveal_state;
}

void egui_view_swipe_control_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    local->compact_mode = compact_mode ? 1 : 0;
    swipe_control_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_swipe_control_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    swipe_control_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_swipe_control_set_on_changed_listener(egui_view_t *self, egui_view_on_swipe_control_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    local->on_changed = listener;
}

uint8_t egui_view_swipe_control_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    egui_view_swipe_control_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }

    swipe_control_get_metrics(local, self, &metrics);
    if (part == EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE)
    {
        *region = metrics.surface_region;
        return 1;
    }
    if (part == EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION && metrics.start_action_region.size.width > 0)
    {
        *region = metrics.start_action_region;
        return 1;
    }
    if (part == EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION && metrics.end_action_region.size.width > 0)
    {
        *region = metrics.end_action_region;
        return 1;
    }
    return 0;
}

uint8_t egui_view_swipe_control_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);

    swipe_control_normalize_state(local);
    if (local->compact_mode || local->read_only_mode || local->item == NULL || !egui_view_get_enable(self))
    {
        return 0;
    }

    switch (key_code)
    {
    case EGUI_KEY_CODE_RIGHT:
    case EGUI_KEY_CODE_PLUS:
    case EGUI_KEY_CODE_HOME:
        swipe_control_reveal_preferred(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_START, 1);
        return 1;
    case EGUI_KEY_CODE_LEFT:
    case EGUI_KEY_CODE_MINUS:
    case EGUI_KEY_CODE_END:
        swipe_control_reveal_preferred(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_END, 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        swipe_control_cycle_part(self);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        swipe_control_notify(self);
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE, 1);
        return 1;
    default:
        return 0;
    }
}

static void egui_view_swipe_control_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    egui_view_swipe_control_metrics_t metrics;
    egui_dim_t radius = local->compact_mode ? SWC_COMPACT_RADIUS : SWC_STD_RADIUS;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t muted_text_color = local->muted_text_color;

    swipe_control_get_metrics(local, self, &metrics);

    if (local->read_only_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xF0F3F7), 50);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0xA7B1BD), 42);
        muted_text_color = egui_rgb_mix(muted_text_color, EGUI_COLOR_HEX(0x96A3AF), 34);
    }

    swipe_control_draw_round_fill_safe(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                       self->region_screen.size.height, radius, surface_color, egui_color_alpha_mix(self->alpha, 100));
    swipe_control_draw_round_stroke_safe(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                         self->region_screen.size.height, radius, 1, border_color, egui_color_alpha_mix(self->alpha, 58));

    swipe_control_draw_text(local->meta_font, self, local->title, &metrics.title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);

    if (local->item == NULL)
    {
        swipe_control_draw_text(local->meta_font, self, "No row", &metrics.surface_region, EGUI_ALIGN_CENTER, muted_text_color);
        return;
    }

    if (metrics.start_action_region.size.width > 0)
    {
        swipe_control_draw_action(self, local, &metrics.start_action_region, local->start_action,
                                  local->pressed_part == EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION);
    }
    if (metrics.end_action_region.size.width > 0)
    {
        swipe_control_draw_action(self, local, &metrics.end_action_region, local->end_action, local->pressed_part == EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION);
    }

    swipe_control_draw_surface(self, local, local->item, &metrics);
    swipe_control_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);

    if (!local->read_only_mode)
    {
        if (local->current_part == EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE)
        {
            swipe_control_draw_focus(self, &metrics.surface_region, local->compact_mode ? 10 : 12, local->item->accent_color);
        }
        else if (local->current_part == EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION && metrics.start_action_region.size.width > 0)
        {
            swipe_control_draw_focus(self, &metrics.start_action_region, local->compact_mode ? 9 : 10, local->item->accent_color);
        }
        else if (local->current_part == EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION && metrics.end_action_region.size.width > 0)
        {
            swipe_control_draw_focus(self, &metrics.end_action_region, local->compact_mode ? 9 : 10, local->item->accent_color);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static uint8_t swipe_control_hit_part(egui_view_swipe_control_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_swipe_control_metrics_t metrics;

    swipe_control_get_metrics(local, self, &metrics);
    if (metrics.start_action_region.size.width > 0 && egui_region_pt_in_rect(&metrics.start_action_region, x, y))
    {
        return EGUI_VIEW_SWIPE_CONTROL_PART_START_ACTION;
    }
    if (metrics.end_action_region.size.width > 0 && egui_region_pt_in_rect(&metrics.end_action_region, x, y))
    {
        return EGUI_VIEW_SWIPE_CONTROL_PART_END_ACTION;
    }
    if (egui_region_pt_in_rect(&metrics.surface_region, x, y))
    {
        return EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE;
    }
    return EGUI_VIEW_SWIPE_CONTROL_PART_NONE;
}

static int egui_view_swipe_control_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_swipe_control_t);
    int delta_x;
    int delta_y;
    uint8_t hit_part;

    swipe_control_normalize_state(local);
    if (local->compact_mode || local->read_only_mode || local->item == NULL || !egui_view_get_enable(self))
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = swipe_control_hit_part(local, self, event->location.x, event->location.y);
        if (hit_part == EGUI_VIEW_SWIPE_CONTROL_PART_NONE)
        {
            return 0;
        }
        local->pressed_part = hit_part;
        local->dragging = 0;
        local->gesture_start_x = event->location.x;
        local->gesture_start_y = event->location.y;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->pressed_part == EGUI_VIEW_SWIPE_CONTROL_PART_NONE)
        {
            return 0;
        }
        delta_x = (int)event->location.x - (int)local->gesture_start_x;
        delta_y = (int)event->location.y - (int)local->gesture_start_y;
        if (!local->dragging)
        {
            if (delta_x < 0)
            {
                delta_x = -delta_x;
            }
            if (delta_y < 0)
            {
                delta_y = -delta_y;
            }
            if (delta_x >= SWC_STD_SWIPE_DELTA && delta_x > delta_y)
            {
                local->dragging = 1;
            }
            else
            {
                return 0;
            }
            delta_x = (int)event->location.x - (int)local->gesture_start_x;
        }

        if (delta_x >= SWC_STD_SWIPE_DELTA)
        {
            swipe_control_reveal_preferred(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_START, 0);
        }
        else if (delta_x <= -SWC_STD_SWIPE_DELTA)
        {
            swipe_control_reveal_preferred(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_END, 0);
        }
        else
        {
            swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE, 0);
        }
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        if (local->pressed_part == EGUI_VIEW_SWIPE_CONTROL_PART_NONE)
        {
            return 0;
        }
        hit_part = swipe_control_hit_part(local, self, event->location.x, event->location.y);
        if (local->dragging)
        {
            local->dragging = 0;
            local->pressed_part = EGUI_VIEW_SWIPE_CONTROL_PART_NONE;
            egui_view_set_pressed(self, false);
            egui_view_invalidate(self);
            swipe_control_notify(self);
            return 1;
        }

        if (local->pressed_part == EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE)
        {
            if (local->reveal_state != EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE && hit_part == EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE)
            {
                swipe_control_set_state_inner(self, EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE, 1);
            }
            else
            {
                swipe_control_notify(self);
            }
        }
        else if (local->pressed_part == hit_part)
        {
            swipe_control_notify(self);
        }

        local->pressed_part = EGUI_VIEW_SWIPE_CONTROL_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_SWIPE_CONTROL_PART_NONE;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_SWIPE_CONTROL_PART_NONE;
        local->dragging = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_swipe_control_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    if (egui_view_swipe_control_handle_navigation_key(self, event->key_code))
    {
        return 1;
    }

    return egui_view_on_key_event(self, event);
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_swipe_control_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_swipe_control_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_swipe_control_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_swipe_control_on_key_event,
#endif
};

void egui_view_swipe_control_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_swipe_control_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_swipe_control_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 2);

    local->on_changed = NULL;
    local->item = NULL;
    local->start_action = NULL;
    local->end_action = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->title = NULL;
    local->helper = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD6DEE7);
    local->text_color = EGUI_COLOR_HEX(0x172331);
    local->muted_text_color = EGUI_COLOR_HEX(0x6E7C8B);
    local->inactive_color = EGUI_COLOR_HEX(0xA6B0BD);
    local->current_part = EGUI_VIEW_SWIPE_CONTROL_PART_SURFACE;
    local->reveal_state = EGUI_VIEW_SWIPE_CONTROL_REVEAL_NONE;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->pressed_part = EGUI_VIEW_SWIPE_CONTROL_PART_NONE;
    local->dragging = 0;
    local->gesture_start_x = 0;
    local->gesture_start_y = 0;

    egui_view_set_view_name(self, "egui_view_swipe_control");
}
