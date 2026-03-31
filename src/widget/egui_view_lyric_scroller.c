#include <string.h>

#include "egui_view_lyric_scroller.h"

static void egui_view_lyric_scroller_stop_internal(egui_view_lyric_scroller_t *local)
{
    if (egui_timer_check_timer_start(&local->scroll_timer))
    {
        egui_timer_stop_timer(&local->scroll_timer);
    }
}

static uint16_t egui_view_lyric_scroller_get_pause_ticks(const egui_view_lyric_scroller_t *local)
{
    if (local->pause_duration_ms == 0 || local->interval_ms == 0)
    {
        return 0;
    }

    return (uint16_t)EGUI_MAX(1, (local->pause_duration_ms + local->interval_ms - 1) / local->interval_ms);
}

static void egui_view_lyric_scroller_apply_offset(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    if (EGUI_VIEW_OF(&local->label)->region.location.x != -local->scroll_offset_x || EGUI_VIEW_OF(&local->label)->region.location.y != 0)
    {
        egui_view_set_position(EGUI_VIEW_OF(&local->label), -local->scroll_offset_x, 0);
    }
}

static void egui_view_lyric_scroller_update_timer_state(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    uint8_t should_run = local->is_attached && local->max_scroll_offset > 0 && local->scroll_step > 0 && local->interval_ms > 0;

    if (!should_run)
    {
        egui_view_lyric_scroller_stop_internal(local);
        return;
    }

    if (!egui_timer_check_timer_start(&local->scroll_timer))
    {
        egui_timer_start_timer(&local->scroll_timer, local->interval_ms, local->interval_ms);
    }
}

static void egui_view_lyric_scroller_update_label_layout(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    egui_region_t work_region;
    const char *text = local->label.text;
    const egui_font_t *font = local->label.font;
    egui_dim_t label_width;
    egui_dim_t label_height;

    egui_view_get_work_region(self, &work_region);
    if (work_region.size.width < 0)
    {
        work_region.size.width = 0;
    }
    if (work_region.size.height < 0)
    {
        work_region.size.height = 0;
    }

    local->text_width = 0;
    local->text_height = 0;
    if (font != NULL && text != NULL && text[0] != '\0')
    {
        font->api->get_str_size(font, text, 0, 0, &local->text_width, &local->text_height);
    }

    local->max_scroll_offset = local->text_width - work_region.size.width;
    if (local->max_scroll_offset < 0)
    {
        local->max_scroll_offset = 0;
    }
    if (local->scroll_offset_x > local->max_scroll_offset)
    {
        local->scroll_offset_x = local->max_scroll_offset;
    }
    if (local->max_scroll_offset == 0)
    {
        local->scroll_offset_x = 0;
        local->scroll_direction = 1;
        local->pause_ticks_remaining = 0;
    }

    label_width = EGUI_MAX(local->text_width, work_region.size.width);
    label_height = EGUI_MAX(local->text_height, work_region.size.height);

    if (EGUI_VIEW_OF(&local->label)->region.size.width != label_width || EGUI_VIEW_OF(&local->label)->region.size.height != label_height)
    {
        egui_view_set_size(EGUI_VIEW_OF(&local->label), label_width, label_height);
    }
    egui_view_lyric_scroller_apply_offset(self);
    egui_view_lyric_scroller_update_timer_state(self);
}

static void egui_view_lyric_scroller_timer_callback(egui_timer_t *timer)
{
    egui_view_lyric_scroller_t *local = (egui_view_lyric_scroller_t *)timer->user_data;
    egui_view_t *self = EGUI_VIEW_OF(local);
    egui_dim_t next_offset;

    if (local->max_scroll_offset <= 0 || local->scroll_step <= 0)
    {
        return;
    }

    if (local->pause_ticks_remaining > 0)
    {
        local->pause_ticks_remaining--;
        return;
    }

    next_offset = local->scroll_offset_x + (local->scroll_direction > 0 ? local->scroll_step : -local->scroll_step);
    if (next_offset >= local->max_scroll_offset)
    {
        next_offset = local->max_scroll_offset;
        local->scroll_direction = -1;
        local->pause_ticks_remaining = egui_view_lyric_scroller_get_pause_ticks(local);
    }
    else if (next_offset <= 0)
    {
        next_offset = 0;
        local->scroll_direction = 1;
        local->pause_ticks_remaining = egui_view_lyric_scroller_get_pause_ticks(local);
    }

    if (next_offset != local->scroll_offset_x)
    {
        local->scroll_offset_x = next_offset;
        egui_view_lyric_scroller_apply_offset(self);
        egui_view_invalidate(self);
    }
}

static void egui_view_lyric_scroller_draw(egui_view_t *self)
{
    egui_region_t clip_region;
    const egui_region_t *prev_clip = egui_canvas_get_extra_clip();
    const egui_region_t *active_clip = &self->region_screen;

    if (prev_clip != NULL)
    {
        egui_region_intersect(&self->region_screen, prev_clip, &clip_region);
        active_clip = &clip_region;
    }

    egui_canvas_set_extra_clip(active_clip);
    egui_view_group_draw(self);

    if (prev_clip != NULL)
    {
        egui_canvas_set_extra_clip(prev_clip);
    }
    else
    {
        egui_canvas_clear_extra_clip();
    }
}

static void egui_view_lyric_scroller_on_attach_to_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    local->is_attached = 1;
    egui_view_group_on_attach_to_window(self);
    egui_view_lyric_scroller_update_label_layout(self);
}

static void egui_view_lyric_scroller_on_detach_from_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    local->is_attached = 0;
    egui_view_lyric_scroller_stop_internal(local);
    egui_view_group_on_detach_from_window(self);
}

static void egui_view_lyric_scroller_calculate_layout(egui_view_t *self)
{
    egui_view_lyric_scroller_update_label_layout(self);
    egui_view_group_calculate_layout(self);
}

void egui_view_lyric_scroller_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    egui_view_label_set_text(EGUI_VIEW_OF(&local->label), text);
    local->scroll_offset_x = 0;
    local->scroll_direction = 1;
    local->pause_ticks_remaining = 0;
    egui_view_lyric_scroller_update_label_layout(self);
    egui_view_invalidate(self);
}

void egui_view_lyric_scroller_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    egui_view_label_set_font(EGUI_VIEW_OF(&local->label), font);
    local->scroll_offset_x = 0;
    local->scroll_direction = 1;
    local->pause_ticks_remaining = 0;
    egui_view_lyric_scroller_update_label_layout(self);
    egui_view_invalidate(self);
}

void egui_view_lyric_scroller_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    egui_view_label_set_font_color(EGUI_VIEW_OF(&local->label), color, alpha);
    egui_view_invalidate(self);
}

void egui_view_lyric_scroller_set_scroll_step(egui_view_t *self, egui_dim_t scroll_step)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    if (scroll_step < 1)
    {
        scroll_step = 1;
    }
    if (local->scroll_step == scroll_step)
    {
        return;
    }
    local->scroll_step = scroll_step;
    egui_view_lyric_scroller_update_timer_state(self);
}

void egui_view_lyric_scroller_set_interval_ms(egui_view_t *self, uint16_t interval_ms)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    if (interval_ms == 0)
    {
        interval_ms = 1;
    }
    if (local->interval_ms == interval_ms)
    {
        return;
    }
    local->interval_ms = interval_ms;
    egui_view_lyric_scroller_stop_internal(local);
    egui_view_lyric_scroller_update_timer_state(self);
}

void egui_view_lyric_scroller_set_pause_duration_ms(egui_view_t *self, uint16_t pause_duration_ms)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    local->pause_duration_ms = pause_duration_ms;
}

void egui_view_lyric_scroller_restart(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    local->scroll_offset_x = 0;
    local->scroll_direction = 1;
    local->pause_ticks_remaining = 0;
    egui_view_lyric_scroller_update_label_layout(self);
    egui_view_invalidate(self);
}

void egui_view_lyric_scroller_start(egui_view_t *self)
{
    egui_view_lyric_scroller_update_timer_state(self);
}

void egui_view_lyric_scroller_stop(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    egui_view_lyric_scroller_stop_internal(local);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_lyric_scroller_t) = {
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = egui_view_group_on_touch_event,
        .on_intercept_touch_event = egui_view_group_on_intercept_touch_event,
        .compute_scroll = egui_view_group_compute_scroll,
        .calculate_layout = egui_view_lyric_scroller_calculate_layout,
        .request_layout = egui_view_group_request_layout,
        .draw = egui_view_lyric_scroller_draw,
        .on_attach_to_window = egui_view_lyric_scroller_on_attach_to_window,
        .on_draw = egui_view_on_draw,
        .on_detach_from_window = egui_view_lyric_scroller_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_group_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_lyric_scroller_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_lyric_scroller_t);

    egui_view_group_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_lyric_scroller_t);

    egui_view_label_init(EGUI_VIEW_OF(&local->label));
    egui_view_set_parent(EGUI_VIEW_OF(&local->label), &local->base);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&local->label), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_group_add_child(self, EGUI_VIEW_OF(&local->label));

    local->text_width = 0;
    local->text_height = 0;
    local->scroll_offset_x = 0;
    local->max_scroll_offset = 0;
    local->scroll_step = 1;
    local->interval_ms = 50;
    local->pause_duration_ms = 400;
    local->pause_ticks_remaining = 0;
    local->scroll_direction = 1;
    local->is_attached = 0;

    egui_timer_init_timer(&local->scroll_timer, local, egui_view_lyric_scroller_timer_callback);

    egui_view_set_view_name(self, "egui_view_lyric_scroller");
}

void egui_view_lyric_scroller_apply_params(egui_view_t *self, const egui_view_lyric_scroller_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    self->region = params->region;
    local->scroll_step = params->scroll_step > 0 ? params->scroll_step : 1;
    local->interval_ms = params->interval_ms > 0 ? params->interval_ms : 1;
    local->pause_duration_ms = params->pause_duration_ms;

    egui_view_label_set_font(EGUI_VIEW_OF(&local->label), params->font);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&local->label), params->color, params->alpha);
    egui_view_lyric_scroller_set_text(self, params->text);
}

void egui_view_lyric_scroller_init_with_params(egui_view_t *self, const egui_view_lyric_scroller_params_t *params)
{
    egui_view_lyric_scroller_init(self);
    egui_view_lyric_scroller_apply_params(self, params);
}
