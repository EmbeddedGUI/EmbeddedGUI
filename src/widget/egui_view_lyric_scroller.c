#include <string.h>

#include "egui_view_lyric_scroller.h"
#include "core/egui_core.h"
#include "font/egui_font.h"

/*
 * The lyric scroller is implemented as a clipping group plus one moving label child.
 * It only starts a timer when the text is wider than the visible work
 * region.
 */

static void egui_view_lyric_scroller_stop_internal(egui_view_lyric_scroller_t *local)
{
    egui_view_t *self = EGUI_VIEW_OF(&local->base);

    if (egui_view_get_core(self) == NULL)
    {
        return;
    }
    if (egui_view_check_timer_start(self, &local->scroll_timer))
    {
        egui_view_stop_timer(self, &local->scroll_timer);
    }
}

static uint16_t egui_view_lyric_scroller_get_pause_ticks(const egui_view_lyric_scroller_t *local)
{
    if (local->pause_duration_ms == 0 || local->interval_ms == 0)
    {
        return 0;
    }

    // Convert the pause duration from milliseconds into timer ticks, rounded up.
    return (uint16_t)EGUI_MAX(1, (local->pause_duration_ms + local->interval_ms - 1) / local->interval_ms);
}

static void egui_view_lyric_scroller_apply_offset(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    // Scrolling is implemented by moving the child label left/right inside the clipped group.
    if (EGUI_VIEW_OF(&local->label)->region.location.x != -local->scroll_offset_x || EGUI_VIEW_OF(&local->label)->region.location.y != 0)
    {
        egui_view_set_position(EGUI_VIEW_OF(&local->label), -local->scroll_offset_x, 0);
    }
}

static void egui_view_lyric_scroller_update_timer_state(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    if (egui_view_get_core(self) == NULL)
    {
        return;
    }

    uint8_t should_run = local->is_attached && local->max_scroll_offset > 0 && local->scroll_step > 0 && local->interval_ms > 0;

    // The timer is only useful when the widget is live and the text actually overflows.
    if (!should_run)
    {
        egui_view_lyric_scroller_stop_internal(local);
        return;
    }

    if (!egui_view_check_timer_start(self, &local->scroll_timer))
    {
        egui_view_start_timer(self, &local->scroll_timer, local->interval_ms, local->interval_ms);
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
        egui_font_get_str_size_with_core(font, egui_view_get_core(self), text, 0, 0, &local->text_width, &local->text_height);
    }

    // Overflow distance determines both whether scrolling is needed and the end-stop position.
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

    // The label stays at least as large as the viewport so alignment and clipping remain stable.
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

    // Direction flips at each end so the text bounces instead of wrapping.
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
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_region_t clip_region;
    const egui_region_t *prev_clip = egui_canvas_get_extra_clip(canvas);
    const egui_region_t *active_clip = &self->region_screen;

    // Intersect with any existing clip so the moving label never paints outside the scroller viewport.
    if (prev_clip != NULL)
    {
        egui_region_intersect(&self->region_screen, prev_clip, &clip_region);
        active_clip = &clip_region;
    }

    egui_canvas_set_extra_clip(canvas, active_clip);
    egui_view_group_draw(self);

    if (prev_clip != NULL)
    {
        egui_canvas_set_extra_clip(canvas, prev_clip);
    }
    else
    {
        egui_canvas_clear_extra_clip(canvas);
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
    // Recompute overflow before delegating layout to the group so the child size/offset stay in sync.
    egui_view_lyric_scroller_update_label_layout(self);
    egui_view_group_calculate_layout(self);
}

void egui_view_lyric_scroller_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    // New text always restarts from the leftmost position and recalculates overflow.
    egui_view_label_set_text(EGUI_VIEW_OF(&local->label), text);
    local->scroll_offset_x = 0;
    local->scroll_direction = 1;
    local->pause_ticks_remaining = 0;
    egui_view_lyric_scroller_update_label_layout(self);
    egui_view_invalidate(self);
}

const char *egui_view_lyric_scroller_get_text(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    return egui_view_label_get_text(EGUI_VIEW_OF(&local->label));
}

void egui_view_lyric_scroller_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    // Changing the font changes measured text width, so restart and recompute layout.
    egui_view_label_set_font(EGUI_VIEW_OF(&local->label), font);
    local->scroll_offset_x = 0;
    local->scroll_direction = 1;
    local->pause_ticks_remaining = 0;
    egui_view_lyric_scroller_update_label_layout(self);
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_lyric_scroller_get_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    return egui_view_label_get_font(EGUI_VIEW_OF(&local->label));
}

void egui_view_lyric_scroller_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    egui_view_label_set_font_color(EGUI_VIEW_OF(&local->label), color, alpha);
    egui_view_invalidate(self);
}

egui_color_t egui_view_lyric_scroller_get_font_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    return egui_view_label_get_font_color(EGUI_VIEW_OF(&local->label));
}

egui_alpha_t egui_view_lyric_scroller_get_font_alpha(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    return egui_view_label_get_alpha(EGUI_VIEW_OF(&local->label));
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

egui_dim_t egui_view_lyric_scroller_get_scroll_step(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    return local->scroll_step;
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

uint16_t egui_view_lyric_scroller_get_interval_ms(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    return local->interval_ms;
}

void egui_view_lyric_scroller_set_pause_duration_ms(egui_view_t *self, uint16_t pause_duration_ms)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    local->pause_duration_ms = pause_duration_ms;
}

uint16_t egui_view_lyric_scroller_get_pause_duration_ms(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    return local->pause_duration_ms;
}

egui_dim_t egui_view_lyric_scroller_get_text_width(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    return local->text_width;
}

egui_dim_t egui_view_lyric_scroller_get_text_height(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    return local->text_height;
}

egui_dim_t egui_view_lyric_scroller_get_scroll_offset_x(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    return local->scroll_offset_x;
}

egui_dim_t egui_view_lyric_scroller_get_max_scroll_offset(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    return local->max_scroll_offset;
}

void egui_view_lyric_scroller_restart(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);

    // Restart keeps content/style intact and only rewinds the scrolling state machine.
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

uint8_t egui_view_lyric_scroller_is_scrolling(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_lyric_scroller_t);
    return (uint8_t)(egui_view_check_timer_start(self, &local->scroll_timer) ? 1 : 0);
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

void egui_view_lyric_scroller_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_lyric_scroller_t);

    // The outer widget is a group so it can own and clip its internal label child.
    egui_view_group_init(self, core);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_lyric_scroller_t);

    // The child label does all text rendering; the scroller only manages its size and x-offset.
    egui_view_label_init(EGUI_VIEW_OF(&local->label), core);
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

    // Params first seed scrolling behavior, then configure the embedded label and text content.
    self->region = params->region;
    local->scroll_step = params->scroll_step > 0 ? params->scroll_step : 1;
    local->interval_ms = params->interval_ms > 0 ? params->interval_ms : 1;
    local->pause_duration_ms = params->pause_duration_ms;

    egui_view_label_set_font(EGUI_VIEW_OF(&local->label), params->font);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&local->label), params->color, params->alpha);
    egui_view_lyric_scroller_set_text(self, params->text);
}

void egui_view_lyric_scroller_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_lyric_scroller_params_t *params)
{
    egui_view_lyric_scroller_init(self, core);
    egui_view_lyric_scroller_apply_params(self, params);
}
