#include "egui_view_teaching_tip.h"

#define EGUI_VIEW_TEACHING_TIP_STANDARD_RADIUS        10
#define EGUI_VIEW_TEACHING_TIP_STANDARD_FILL_ALPHA    94
#define EGUI_VIEW_TEACHING_TIP_STANDARD_BORDER_ALPHA  60
#define EGUI_VIEW_TEACHING_TIP_STANDARD_PAD_X         10
#define EGUI_VIEW_TEACHING_TIP_STANDARD_PAD_Y         7
#define EGUI_VIEW_TEACHING_TIP_STANDARD_TARGET_HEIGHT 23
#define EGUI_VIEW_TEACHING_TIP_STANDARD_TARGET_GAP    7
#define EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_HEIGHT 88
#define EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_RADIUS 8
#define EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_PAD_X  10
#define EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_PAD_Y  9
#define EGUI_VIEW_TEACHING_TIP_STANDARD_ACTION_HEIGHT 19
#define EGUI_VIEW_TEACHING_TIP_STANDARD_ACTION_GAP    6
#define EGUI_VIEW_TEACHING_TIP_STANDARD_CLOSE_SIZE    14
#define EGUI_VIEW_TEACHING_TIP_STANDARD_ARROW_WIDTH   14
#define EGUI_VIEW_TEACHING_TIP_STANDARD_ARROW_HEIGHT  8

#define EGUI_VIEW_TEACHING_TIP_COMPACT_RADIUS        8
#define EGUI_VIEW_TEACHING_TIP_COMPACT_FILL_ALPHA    92
#define EGUI_VIEW_TEACHING_TIP_COMPACT_BORDER_ALPHA  56
#define EGUI_VIEW_TEACHING_TIP_COMPACT_PAD_X         8
#define EGUI_VIEW_TEACHING_TIP_COMPACT_PAD_Y         6
#define EGUI_VIEW_TEACHING_TIP_COMPACT_TARGET_HEIGHT 18
#define EGUI_VIEW_TEACHING_TIP_COMPACT_TARGET_GAP    4
#define EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_HEIGHT 34
#define EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_RADIUS 6
#define EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_PAD_X  6
#define EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_PAD_Y  5
#define EGUI_VIEW_TEACHING_TIP_COMPACT_ACTION_HEIGHT 14
#define EGUI_VIEW_TEACHING_TIP_COMPACT_ACTION_GAP    5
#define EGUI_VIEW_TEACHING_TIP_COMPACT_CLOSE_SIZE    10
#define EGUI_VIEW_TEACHING_TIP_COMPACT_ARROW_WIDTH   8
#define EGUI_VIEW_TEACHING_TIP_COMPACT_ARROW_HEIGHT  4

typedef struct egui_view_teaching_tip_metrics egui_view_teaching_tip_metrics_t;
struct egui_view_teaching_tip_metrics
{
    egui_region_t content_region;
    egui_region_t target_region;
    egui_region_t bubble_region;
    egui_region_t eyebrow_region;
    egui_region_t title_region;
    egui_region_t body_region;
    egui_region_t footer_region;
    egui_region_t closed_title_region;
    egui_region_t closed_body_region;
    egui_region_t primary_region;
    egui_region_t secondary_region;
    egui_region_t close_region;
    egui_dim_t arrow_center_x;
    uint8_t show_bubble;
    uint8_t show_eyebrow;
    uint8_t show_body;
    uint8_t show_footer;
    uint8_t show_closed_hint;
    uint8_t show_primary;
    uint8_t show_secondary;
    uint8_t show_close;
};

static uint8_t egui_view_teaching_tip_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_TEACHING_TIP_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_TEACHING_TIP_MAX_SNAPSHOTS;
    }

    return count;
}

static const egui_view_teaching_tip_snapshot_t *egui_view_teaching_tip_get_snapshot(egui_view_teaching_tip_t *local)
{
    if (local->snapshots == NULL || local->snapshot_count == 0 || local->current_snapshot >= local->snapshot_count)
    {
        return NULL;
    }

    return &local->snapshots[local->current_snapshot];
}

static uint8_t egui_view_teaching_tip_text_len(const char *text)
{
    uint8_t length = 0;

    if (text == NULL)
    {
        return 0;
    }

    while (text[length] != '\0')
    {
        length++;
    }

    return length;
}

static uint8_t egui_view_teaching_tip_has_text(const char *text)
{
    return (text != NULL && text[0] != '\0') ? 1 : 0;
}

static egui_color_t egui_view_teaching_tip_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static egui_color_t egui_view_teaching_tip_tone_color(egui_view_teaching_tip_t *local, uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_TEACHING_TIP_TONE_SUCCESS:
        return local->success_color;
    case EGUI_VIEW_TEACHING_TIP_TONE_WARNING:
        return local->warning_color;
    case EGUI_VIEW_TEACHING_TIP_TONE_NEUTRAL:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static void egui_view_teaching_tip_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                             egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static uint8_t egui_view_teaching_tip_part_is_enabled(egui_view_teaching_tip_t *local, egui_view_t *self, const egui_view_teaching_tip_snapshot_t *snapshot,
                                                      uint8_t part)
{
    if (snapshot == NULL || !egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (part)
    {
    case EGUI_VIEW_TEACHING_TIP_PART_TARGET:
        return 1;
    case EGUI_VIEW_TEACHING_TIP_PART_PRIMARY:
        return (snapshot->open_mode && snapshot->primary_action != NULL && snapshot->primary_action[0] != '\0' && snapshot->primary_enabled) ? 1 : 0;
    case EGUI_VIEW_TEACHING_TIP_PART_SECONDARY:
        return (snapshot->open_mode && snapshot->secondary_action != NULL && snapshot->secondary_action[0] != '\0' && snapshot->secondary_enabled) ? 1 : 0;
    case EGUI_VIEW_TEACHING_TIP_PART_CLOSE:
        return (snapshot->open_mode && snapshot->show_close) ? 1 : 0;
    default:
        return 0;
    }
}

static uint8_t egui_view_teaching_tip_resolve_default_part(egui_view_teaching_tip_t *local, egui_view_t *self,
                                                           const egui_view_teaching_tip_snapshot_t *snapshot)
{
    uint8_t focus_part;

    if (snapshot == NULL)
    {
        return EGUI_VIEW_TEACHING_TIP_PART_NONE;
    }

    focus_part = snapshot->focus_part;
    if (egui_view_teaching_tip_part_is_enabled(local, self, snapshot, focus_part))
    {
        return focus_part;
    }
    if (egui_view_teaching_tip_part_is_enabled(local, self, snapshot, EGUI_VIEW_TEACHING_TIP_PART_PRIMARY))
    {
        return EGUI_VIEW_TEACHING_TIP_PART_PRIMARY;
    }
    if (egui_view_teaching_tip_part_is_enabled(local, self, snapshot, EGUI_VIEW_TEACHING_TIP_PART_SECONDARY))
    {
        return EGUI_VIEW_TEACHING_TIP_PART_SECONDARY;
    }
    if (egui_view_teaching_tip_part_is_enabled(local, self, snapshot, EGUI_VIEW_TEACHING_TIP_PART_CLOSE))
    {
        return EGUI_VIEW_TEACHING_TIP_PART_CLOSE;
    }

    return EGUI_VIEW_TEACHING_TIP_PART_TARGET;
}

static void egui_view_teaching_tip_set_current_part_inner(egui_view_t *self, uint8_t part, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);
    const egui_view_teaching_tip_snapshot_t *snapshot = egui_view_teaching_tip_get_snapshot(local);

    if (!egui_view_teaching_tip_part_is_enabled(local, self, snapshot, part))
    {
        return;
    }

    if (local->current_part == part)
    {
        if (notify && local->on_part_changed)
        {
            local->on_part_changed(self, part);
        }
        egui_view_invalidate(self);
        return;
    }

    local->current_part = part;
    if (notify && local->on_part_changed)
    {
        local->on_part_changed(self, part);
    }
    egui_view_invalidate(self);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static void egui_view_teaching_tip_notify_part(egui_view_teaching_tip_t *local, egui_view_t *self, uint8_t part)
{
    if (local->on_part_changed)
    {
        local->on_part_changed(self, part);
    }
    egui_view_invalidate(self);
}

static uint8_t egui_view_teaching_tip_collect_parts(egui_view_teaching_tip_t *local, egui_view_t *self, const egui_view_teaching_tip_snapshot_t *snapshot,
                                                    uint8_t *parts, uint8_t max_parts)
{
    uint8_t count = 0;

    if (count < max_parts && egui_view_teaching_tip_part_is_enabled(local, self, snapshot, EGUI_VIEW_TEACHING_TIP_PART_TARGET))
    {
        parts[count++] = EGUI_VIEW_TEACHING_TIP_PART_TARGET;
    }
    if (count < max_parts && egui_view_teaching_tip_part_is_enabled(local, self, snapshot, EGUI_VIEW_TEACHING_TIP_PART_SECONDARY))
    {
        parts[count++] = EGUI_VIEW_TEACHING_TIP_PART_SECONDARY;
    }
    if (count < max_parts && egui_view_teaching_tip_part_is_enabled(local, self, snapshot, EGUI_VIEW_TEACHING_TIP_PART_PRIMARY))
    {
        parts[count++] = EGUI_VIEW_TEACHING_TIP_PART_PRIMARY;
    }
    if (count < max_parts && egui_view_teaching_tip_part_is_enabled(local, self, snapshot, EGUI_VIEW_TEACHING_TIP_PART_CLOSE))
    {
        parts[count++] = EGUI_VIEW_TEACHING_TIP_PART_CLOSE;
    }

    return count;
}

static uint8_t egui_view_teaching_tip_find_part_index(const uint8_t *parts, uint8_t count, uint8_t part)
{
    uint8_t index;

    for (index = 0; index < count; index++)
    {
        if (parts[index] == part)
        {
            return index;
        }
    }

    return 0;
}
#endif

void egui_view_teaching_tip_set_snapshots(egui_view_t *self, const egui_view_teaching_tip_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);
    const egui_view_teaching_tip_snapshot_t *snapshot;

    local->snapshots = snapshots;
    local->snapshot_count = egui_view_teaching_tip_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }

    snapshot = egui_view_teaching_tip_get_snapshot(local);
    local->current_part = egui_view_teaching_tip_resolve_default_part(local, self, snapshot);
    local->pressed_part = EGUI_VIEW_TEACHING_TIP_PART_NONE;
    egui_view_invalidate(self);
}

void egui_view_teaching_tip_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);
    const egui_view_teaching_tip_snapshot_t *snapshot;

    if (local->snapshot_count == 0 || snapshot_index >= local->snapshot_count)
    {
        return;
    }
    if (local->current_snapshot == snapshot_index)
    {
        return;
    }

    local->current_snapshot = snapshot_index;
    snapshot = egui_view_teaching_tip_get_snapshot(local);
    local->current_part = egui_view_teaching_tip_resolve_default_part(local, self, snapshot);
    local->pressed_part = EGUI_VIEW_TEACHING_TIP_PART_NONE;
    egui_view_invalidate(self);
}
uint8_t egui_view_teaching_tip_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);
    return local->current_snapshot;
}

void egui_view_teaching_tip_set_current_part(egui_view_t *self, uint8_t part)
{
    egui_view_teaching_tip_set_current_part_inner(self, part, 0);
}

uint8_t egui_view_teaching_tip_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);
    return local->current_part;
}

void egui_view_teaching_tip_set_on_part_changed_listener(egui_view_t *self, egui_view_on_teaching_tip_part_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);
    local->on_part_changed = listener;
}

void egui_view_teaching_tip_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);

    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_teaching_tip_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);

    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_teaching_tip_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);

    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_teaching_tip_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);

    local->read_only_mode = read_only_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_teaching_tip_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                        egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color, egui_color_t warning_color,
                                        egui_color_t neutral_color, egui_color_t shadow_color)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->success_color = success_color;
    local->warning_color = warning_color;
    local->neutral_color = neutral_color;
    local->shadow_color = shadow_color;
    egui_view_invalidate(self);
}

static void egui_view_teaching_tip_get_metrics(egui_view_teaching_tip_t *local, egui_view_t *self, const egui_view_teaching_tip_snapshot_t *snapshot,
                                               egui_view_teaching_tip_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_PAD_X : EGUI_VIEW_TEACHING_TIP_STANDARD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_PAD_Y : EGUI_VIEW_TEACHING_TIP_STANDARD_PAD_Y;
    egui_dim_t target_h = local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_TARGET_HEIGHT : EGUI_VIEW_TEACHING_TIP_STANDARD_TARGET_HEIGHT;
    egui_dim_t gap = local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_TARGET_GAP : EGUI_VIEW_TEACHING_TIP_STANDARD_TARGET_GAP;
    egui_dim_t bubble_h = local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_HEIGHT : EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_HEIGHT;
    egui_dim_t inner_pad_x = local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_PAD_X : EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_PAD_X;
    egui_dim_t inner_pad_y = local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_PAD_Y : EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_PAD_Y;
    egui_dim_t close_size = local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_CLOSE_SIZE : EGUI_VIEW_TEACHING_TIP_STANDARD_CLOSE_SIZE;
    egui_dim_t action_h = local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_ACTION_HEIGHT : EGUI_VIEW_TEACHING_TIP_STANDARD_ACTION_HEIGHT;
    egui_dim_t action_gap = local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_ACTION_GAP : EGUI_VIEW_TEACHING_TIP_STANDARD_ACTION_GAP;
    egui_dim_t target_w;
    egui_dim_t target_x;
    egui_dim_t target_y;
    egui_dim_t bubble_y;
    egui_dim_t bubble_x;
    egui_dim_t bubble_inner_w;
    egui_dim_t actions_y;
    egui_dim_t button_w;
    egui_dim_t title_y;
    egui_dim_t title_h;
    egui_dim_t body_y;
    egui_dim_t body_bottom;
    egui_dim_t footer_y;
    egui_dim_t compact_button_w;
    egui_dim_t bubble_inset = local->compact_mode ? 0 : 8;
    egui_dim_t bubble_shift = 0;
    egui_dim_t bubble_width;
    egui_dim_t bubble_min_x;
    egui_dim_t bubble_max_x;
    egui_dim_t bubble_side_margin = local->compact_mode ? 0 : 6;
    egui_dim_t closed_panel_w = 0;
    egui_dim_t closed_panel_x = 0;

    egui_view_get_work_region(self, &region);
    metrics->content_region.location.x = region.location.x + pad_x;
    metrics->content_region.location.y = region.location.y + pad_y;
    metrics->content_region.size.width = region.size.width - pad_x * 2;
    metrics->content_region.size.height = region.size.height - pad_y * 2;

    metrics->show_bubble = (snapshot != NULL && snapshot->open_mode) ? 1 : 0;
    metrics->show_primary = (snapshot != NULL && egui_view_teaching_tip_has_text(snapshot->primary_action) && !local->read_only_mode) ? 1 : 0;
    metrics->show_secondary =
            (snapshot != NULL && egui_view_teaching_tip_has_text(snapshot->secondary_action) && !local->compact_mode && !local->read_only_mode) ? 1 : 0;
    metrics->show_footer = (snapshot != NULL && egui_view_teaching_tip_has_text(snapshot->footer) && !local->compact_mode) ? 1 : 0;
    metrics->show_close = (metrics->show_bubble && snapshot != NULL && snapshot->show_close && !local->compact_mode && !local->read_only_mode) ? 1 : 0;
    metrics->show_eyebrow = (snapshot != NULL && egui_view_teaching_tip_has_text(snapshot->eyebrow) && !local->compact_mode) ? 1 : 0;
    metrics->show_body = (snapshot != NULL && egui_view_teaching_tip_has_text(snapshot->body)) ? 1 : 0;
    metrics->show_closed_hint = (!metrics->show_bubble && !local->compact_mode &&
                                 (snapshot != NULL && (egui_view_teaching_tip_has_text(snapshot->title) || egui_view_teaching_tip_has_text(snapshot->body) ||
                                                       egui_view_teaching_tip_has_text(snapshot->footer))))
                                        ? 1
                                        : 0;
    if (local->compact_mode && metrics->show_primary)
    {
        metrics->show_body = 0;
    }

    target_w =
            (local->compact_mode ? 46 : 50) + egui_view_teaching_tip_text_len(snapshot == NULL ? NULL : snapshot->target_label) * (local->compact_mode ? 4 : 5);
    if (target_w > metrics->content_region.size.width - 12)
    {
        target_w = metrics->content_region.size.width - 12;
    }
    if (target_w < (local->compact_mode ? 54 : 66))
    {
        target_w = local->compact_mode ? 54 : 66;
    }

    target_x = metrics->content_region.location.x + (metrics->content_region.size.width - target_w) / 2;
    if (snapshot != NULL)
    {
        target_x += snapshot->target_offset_x;
    }
    if (target_x < metrics->content_region.location.x + 2)
    {
        target_x = metrics->content_region.location.x + 2;
    }
    if (target_x + target_w > metrics->content_region.location.x + metrics->content_region.size.width - 2)
    {
        target_x = metrics->content_region.location.x + metrics->content_region.size.width - target_w - 2;
    }
    if (metrics->show_bubble && snapshot != NULL && snapshot->placement == EGUI_VIEW_TEACHING_TIP_PLACEMENT_TOP)
    {
        bubble_y = metrics->content_region.location.y;
        target_y = bubble_y + bubble_h + gap;
    }
    else if (metrics->show_bubble)
    {
        target_y = metrics->content_region.location.y;
        bubble_y = target_y + target_h + gap;
    }
    else
    {
        if (metrics->show_closed_hint)
        {
            target_y = metrics->content_region.location.y + 26;
        }
        else
        {
            target_y = metrics->content_region.location.y + (metrics->content_region.size.height - target_h) / 2;
        }
        bubble_y = metrics->content_region.location.y;
    }

    metrics->target_region.location.x = target_x;
    metrics->target_region.location.y = target_y;
    metrics->target_region.size.width = target_w;
    metrics->target_region.size.height = target_h;

    if (!local->compact_mode && snapshot != NULL)
    {
        bubble_shift = snapshot->target_offset_x / 3;
    }

    bubble_width = metrics->content_region.size.width - bubble_inset * 2 - bubble_side_margin * 2;
    bubble_min_x = metrics->content_region.location.x + bubble_inset + bubble_side_margin;
    bubble_max_x = metrics->content_region.location.x + metrics->content_region.size.width - bubble_width - bubble_inset - bubble_side_margin;
    bubble_x = bubble_min_x + bubble_shift;
    if (bubble_x < bubble_min_x)
    {
        bubble_x = bubble_min_x;
    }
    if (bubble_x > bubble_max_x)
    {
        bubble_x = bubble_max_x;
    }
    metrics->bubble_region.location.x = bubble_x;
    metrics->bubble_region.location.y = bubble_y;
    metrics->bubble_region.size.width = bubble_width;
    metrics->bubble_region.size.height = metrics->show_bubble ? bubble_h : 0;

    metrics->arrow_center_x = metrics->target_region.location.x + metrics->target_region.size.width / 2;
    if (metrics->arrow_center_x < metrics->bubble_region.location.x + 14)
    {
        metrics->arrow_center_x = metrics->bubble_region.location.x + 14;
    }
    if (metrics->arrow_center_x > metrics->bubble_region.location.x + metrics->bubble_region.size.width - 14)
    {
        metrics->arrow_center_x = metrics->bubble_region.location.x + metrics->bubble_region.size.width - 14;
    }

    metrics->close_region.location.x = metrics->bubble_region.location.x + metrics->bubble_region.size.width - inner_pad_x - close_size;
    metrics->close_region.location.y = metrics->bubble_region.location.y + inner_pad_y - 1;
    metrics->close_region.size.width = close_size;
    metrics->close_region.size.height = close_size;

    bubble_inner_w = metrics->bubble_region.size.width - inner_pad_x * 2;
    metrics->eyebrow_region.location.x = metrics->bubble_region.location.x + inner_pad_x;
    metrics->eyebrow_region.location.y = metrics->bubble_region.location.y + inner_pad_y;
    metrics->eyebrow_region.size.width = bubble_inner_w - (metrics->show_close ? (close_size + 4) : 0);
    metrics->eyebrow_region.size.height = metrics->show_eyebrow ? 8 : 0;

    metrics->footer_region.location.x = metrics->bubble_region.location.x + inner_pad_x;
    metrics->footer_region.location.y = 0;
    metrics->footer_region.size.width = 0;
    metrics->footer_region.size.height = 0;
    if (metrics->show_closed_hint)
    {
        closed_panel_w = metrics->content_region.size.width - 52;
        if (closed_panel_w < metrics->target_region.size.width + 22)
        {
            closed_panel_w = metrics->target_region.size.width + 22;
        }
        if (closed_panel_w > metrics->content_region.size.width - 20)
        {
            closed_panel_w = metrics->content_region.size.width - 20;
        }

        closed_panel_x = metrics->target_region.location.x + metrics->target_region.size.width / 2 - closed_panel_w / 2;
        if (closed_panel_x < metrics->content_region.location.x + 10)
        {
            closed_panel_x = metrics->content_region.location.x + 10;
        }
        if (closed_panel_x + closed_panel_w > metrics->content_region.location.x + metrics->content_region.size.width - 10)
        {
            closed_panel_x = metrics->content_region.location.x + metrics->content_region.size.width - closed_panel_w - 10;
        }

        metrics->closed_title_region.location.x = closed_panel_x + 10;
        metrics->closed_title_region.location.y = metrics->target_region.location.y + metrics->target_region.size.height + 10;
        metrics->closed_title_region.size.width = closed_panel_w - 20;
        metrics->closed_title_region.size.height = 11;
        metrics->closed_body_region.location.x = metrics->closed_title_region.location.x;
        metrics->closed_body_region.location.y = metrics->closed_title_region.location.y + 12;
        metrics->closed_body_region.size.width = metrics->closed_title_region.size.width;
        metrics->closed_body_region.size.height = 9;
    }
    else
    {
        metrics->closed_title_region.location.x = metrics->content_region.location.x + 8;
        metrics->closed_title_region.location.y = metrics->target_region.location.y + metrics->target_region.size.height + 10;
        metrics->closed_title_region.size.width = metrics->content_region.size.width - 16;
        metrics->closed_title_region.size.height = 0;
        metrics->closed_body_region.location.x = metrics->content_region.location.x + 8;
        metrics->closed_body_region.location.y = metrics->closed_title_region.location.y + 12;
        metrics->closed_body_region.size.width = metrics->content_region.size.width - 16;
        metrics->closed_body_region.size.height = 0;
    }

    metrics->secondary_region.location.x = metrics->bubble_region.location.x + inner_pad_x;
    metrics->secondary_region.location.y = 0;
    metrics->secondary_region.size.width = 0;
    metrics->secondary_region.size.height = 0;

    metrics->primary_region.location.x = metrics->bubble_region.location.x + inner_pad_x;
    metrics->primary_region.location.y = 0;
    metrics->primary_region.size.width = 0;
    metrics->primary_region.size.height = 0;

    if (local->compact_mode)
    {
        title_y = metrics->bubble_region.location.y + inner_pad_y + 2;
        if (metrics->show_primary)
        {
            compact_button_w = 18 + egui_view_teaching_tip_text_len(snapshot == NULL ? NULL : snapshot->primary_action) * 4;
            if (compact_button_w < 30)
            {
                compact_button_w = 30;
            }
            if (compact_button_w > 42)
            {
                compact_button_w = 42;
            }
            if (compact_button_w > bubble_inner_w - 24)
            {
                compact_button_w = bubble_inner_w - 24;
            }

            metrics->primary_region.location.x = metrics->bubble_region.location.x + inner_pad_x + bubble_inner_w - compact_button_w;
            metrics->primary_region.location.y = metrics->bubble_region.location.y + (metrics->bubble_region.size.height - action_h) / 2;
            metrics->primary_region.size.width = compact_button_w;
            metrics->primary_region.size.height = action_h;

            metrics->title_region.location.x = metrics->bubble_region.location.x + inner_pad_x;
            metrics->title_region.location.y = title_y;
            metrics->title_region.size.width = metrics->primary_region.location.x - metrics->title_region.location.x - action_gap;
            metrics->title_region.size.height = 10;

            metrics->body_region.location.x = metrics->title_region.location.x;
            metrics->body_region.location.y = metrics->title_region.location.y;
            metrics->body_region.size.width = 0;
            metrics->body_region.size.height = 0;
        }
        else
        {
            metrics->title_region.location.x = metrics->bubble_region.location.x + inner_pad_x;
            metrics->title_region.location.y = title_y;
            metrics->title_region.size.width = bubble_inner_w;
            metrics->title_region.size.height = 9;

            metrics->body_region.location.x = metrics->bubble_region.location.x + inner_pad_x;
            metrics->body_region.location.y = metrics->title_region.location.y + metrics->title_region.size.height + 2;
            metrics->body_region.size.width = bubble_inner_w;
            metrics->body_region.size.height = metrics->show_body ? 8 : 0;
        }

        return;
    }

    title_y = metrics->bubble_region.location.y + inner_pad_y + (metrics->show_eyebrow ? 11 : 3);
    title_h = 11;
    metrics->title_region.location.x = metrics->bubble_region.location.x + inner_pad_x;
    metrics->title_region.location.y = title_y;
    metrics->title_region.size.width = bubble_inner_w - (metrics->show_close ? (close_size + 4) : 0);
    metrics->title_region.size.height = title_h;

    actions_y = metrics->bubble_region.location.y + metrics->bubble_region.size.height - inner_pad_y - action_h;
    if (metrics->show_secondary)
    {
        egui_dim_t min_primary_w = 56;
        egui_dim_t min_secondary_w = 42;
        egui_dim_t max_secondary_w = bubble_inner_w - action_gap - min_primary_w;

        button_w = 24 + egui_view_teaching_tip_text_len(snapshot == NULL ? NULL : snapshot->secondary_action) * 5;
        if (button_w < min_secondary_w)
        {
            button_w = min_secondary_w;
        }
        if (max_secondary_w < min_secondary_w)
        {
            button_w = (bubble_inner_w - action_gap) / 2;
        }
        else if (button_w > max_secondary_w)
        {
            button_w = max_secondary_w;
        }
        metrics->secondary_region.location.x = metrics->bubble_region.location.x + inner_pad_x;
        metrics->secondary_region.location.y = actions_y;
        metrics->secondary_region.size.width = button_w;
        metrics->secondary_region.size.height = action_h;

        metrics->primary_region.location.x = metrics->secondary_region.location.x + button_w + action_gap;
        metrics->primary_region.location.y = actions_y;
        metrics->primary_region.size.width = bubble_inner_w - button_w - action_gap;
        metrics->primary_region.size.height = action_h;
    }
    else if (metrics->show_primary)
    {
        metrics->primary_region.location.x = metrics->bubble_region.location.x + inner_pad_x;
        metrics->primary_region.location.y = actions_y;
        metrics->primary_region.size.width = bubble_inner_w;
        metrics->primary_region.size.height = action_h;
    }

    if (metrics->show_footer)
    {
        footer_y = actions_y - 12;
        metrics->footer_region.location.x = metrics->bubble_region.location.x + inner_pad_x;
        metrics->footer_region.location.y = footer_y;
        metrics->footer_region.size.width = bubble_inner_w;
        metrics->footer_region.size.height = 8;
    }

    metrics->body_region.location.x = metrics->bubble_region.location.x + inner_pad_x;
    metrics->body_region.location.y = metrics->title_region.location.y + metrics->title_region.size.height + 5;
    metrics->body_region.size.width = bubble_inner_w;
    body_y = metrics->body_region.location.y;
    body_bottom = (metrics->show_footer ? metrics->footer_region.location.y - 4 : actions_y - 6);
    if (body_bottom < body_y)
    {
        body_bottom = body_y;
    }
    metrics->body_region.size.height = metrics->show_body ? (body_bottom - body_y) : 0;
}
static uint8_t egui_view_teaching_tip_hit_part(egui_view_teaching_tip_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_teaching_tip_metrics_t metrics;
    const egui_view_teaching_tip_snapshot_t *snapshot = egui_view_teaching_tip_get_snapshot(local);
    egui_dim_t local_x = x - self->region_screen.location.x;
    egui_dim_t local_y = y - self->region_screen.location.y;

    egui_view_teaching_tip_get_metrics(local, self, snapshot, &metrics);
    if (egui_region_pt_in_rect(&metrics.target_region, local_x, local_y))
    {
        return EGUI_VIEW_TEACHING_TIP_PART_TARGET;
    }
    if (metrics.show_bubble && metrics.show_close && egui_region_pt_in_rect(&metrics.close_region, local_x, local_y))
    {
        return EGUI_VIEW_TEACHING_TIP_PART_CLOSE;
    }
    if (metrics.show_bubble && metrics.show_primary && egui_region_pt_in_rect(&metrics.primary_region, local_x, local_y))
    {
        return EGUI_VIEW_TEACHING_TIP_PART_PRIMARY;
    }
    if (metrics.show_bubble && metrics.show_secondary && egui_region_pt_in_rect(&metrics.secondary_region, local_x, local_y))
    {
        return EGUI_VIEW_TEACHING_TIP_PART_SECONDARY;
    }

    return EGUI_VIEW_TEACHING_TIP_PART_NONE;
}

uint8_t egui_view_teaching_tip_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);
    egui_view_teaching_tip_metrics_t metrics;
    const egui_view_teaching_tip_snapshot_t *snapshot = egui_view_teaching_tip_get_snapshot(local);

    if (region == NULL || snapshot == NULL)
    {
        return 0;
    }

    egui_view_teaching_tip_get_metrics(local, self, snapshot, &metrics);
    switch (part)
    {
    case EGUI_VIEW_TEACHING_TIP_PART_TARGET:
        *region = metrics.target_region;
        break;
    case EGUI_VIEW_TEACHING_TIP_PART_PRIMARY:
        if (metrics.show_bubble && metrics.show_primary)
        {
            *region = metrics.primary_region;
            break;
        }
        return 0;
    case EGUI_VIEW_TEACHING_TIP_PART_SECONDARY:
        if (metrics.show_bubble && metrics.show_secondary)
        {
            *region = metrics.secondary_region;
            break;
        }
        return 0;
    case EGUI_VIEW_TEACHING_TIP_PART_CLOSE:
        if (metrics.show_bubble && metrics.show_close)
        {
            *region = metrics.close_region;
            break;
        }
        return 0;
    default:
        return 0;
    }

    region->location.x += self->region_screen.location.x;
    region->location.y += self->region_screen.location.y;
    return 1;
}

static void egui_view_teaching_tip_draw_close(egui_view_t *self, const egui_region_t *region, egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t pad = region->size.width > 11 ? 4 : 3;
    egui_dim_t x1 = region->location.x + pad;
    egui_dim_t y1 = region->location.y + pad;
    egui_dim_t x2 = region->location.x + region->size.width - pad - 1;
    egui_dim_t y2 = region->location.y + region->size.height - pad - 1;
    egui_alpha_t mixed_alpha = egui_color_alpha_mix(self->alpha, alpha);

    egui_canvas_draw_line(x1, y1, x2, y2, 1, color, mixed_alpha);
    egui_canvas_draw_line(x2, y1, x1, y2, 1, color, mixed_alpha);
}

static void egui_view_teaching_tip_draw_focus_ring(egui_view_t *self, const egui_region_t *region, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_draw_round_rectangle(region->location.x - 1, region->location.y - 1, region->size.width + 2, region->size.height + 2, radius + 1, 1, color,
                                     egui_color_alpha_mix(self->alpha, alpha));
}

static void egui_view_teaching_tip_draw_arrow(egui_view_t *self, egui_view_teaching_tip_t *local, const egui_view_teaching_tip_snapshot_t *snapshot,
                                              const egui_view_teaching_tip_metrics_t *metrics, egui_color_t fill_color, egui_color_t border_color)
{
    egui_dim_t arrow_w = local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_ARROW_WIDTH : EGUI_VIEW_TEACHING_TIP_STANDARD_ARROW_WIDTH;
    egui_dim_t arrow_h = local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_ARROW_HEIGHT : EGUI_VIEW_TEACHING_TIP_STANDARD_ARROW_HEIGHT;
    egui_dim_t center_x = metrics->arrow_center_x;

    if (snapshot == NULL || !metrics->show_bubble)
    {
        return;
    }

    if (snapshot->placement == EGUI_VIEW_TEACHING_TIP_PLACEMENT_TOP)
    {
        egui_dim_t top_y = metrics->bubble_region.location.y + metrics->bubble_region.size.height;

        egui_canvas_draw_triangle_fill(center_x - arrow_w / 2, top_y - 1, center_x + arrow_w / 2, top_y - 1, center_x, top_y + arrow_h, fill_color,
                                       egui_color_alpha_mix(self->alpha, 98));
        egui_canvas_draw_triangle(center_x - arrow_w / 2, top_y - 1, center_x + arrow_w / 2, top_y - 1, center_x, top_y + arrow_h, border_color,
                                  egui_color_alpha_mix(self->alpha, 58));
    }
    else
    {
        egui_dim_t top_y = metrics->bubble_region.location.y;

        egui_canvas_draw_triangle_fill(center_x - arrow_w / 2, top_y + 1, center_x + arrow_w / 2, top_y + 1, center_x, top_y - arrow_h, fill_color,
                                       egui_color_alpha_mix(self->alpha, 98));
        egui_canvas_draw_triangle(center_x - arrow_w / 2, top_y + 1, center_x + arrow_w / 2, top_y + 1, center_x, top_y - arrow_h, border_color,
                                  egui_color_alpha_mix(self->alpha, 58));
    }
}

static void egui_view_teaching_tip_draw_target(egui_view_t *self, egui_view_teaching_tip_t *local, const egui_view_teaching_tip_snapshot_t *snapshot,
                                               const egui_view_teaching_tip_metrics_t *metrics, egui_color_t tone_color, egui_color_t border_color,
                                               egui_color_t text_color)
{
    egui_color_t fill_color = egui_rgb_mix(local->surface_color, tone_color, snapshot != NULL && snapshot->open_mode ? 10 : 6);
    egui_color_t outline_color = egui_rgb_mix(border_color, tone_color, 26);
    egui_color_t focus_color = egui_rgb_mix(tone_color, EGUI_COLOR_WHITE, 8);
    egui_color_t halo_color = egui_rgb_mix(local->surface_color, tone_color, 14);
    egui_region_t text_region;
    egui_region_t halo_region;
    egui_dim_t dot_x;
    egui_dim_t dot_y;

    if (local->current_part == EGUI_VIEW_TEACHING_TIP_PART_TARGET)
    {
        fill_color = egui_rgb_mix(fill_color, tone_color, 16);
        outline_color = egui_rgb_mix(outline_color, tone_color, 34);
    }
    if (local->pressed_part == EGUI_VIEW_TEACHING_TIP_PART_TARGET)
    {
        fill_color = egui_rgb_mix(fill_color, tone_color, 18);
    }

    if (!local->compact_mode && snapshot != NULL && snapshot->open_mode)
    {
        halo_region.location.x = metrics->target_region.location.x - 7;
        halo_region.location.y = metrics->target_region.location.y - 4;
        halo_region.size.width = metrics->target_region.size.width + 14;
        halo_region.size.height = metrics->target_region.size.height + 8;
        egui_canvas_draw_round_rectangle_fill(halo_region.location.x, halo_region.location.y, halo_region.size.width, halo_region.size.height,
                                              EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_RADIUS + 3, halo_color,
                                              egui_color_alpha_mix(self->alpha, local->current_part == EGUI_VIEW_TEACHING_TIP_PART_TARGET ? 30 : 18));
    }

    if (local->current_part == EGUI_VIEW_TEACHING_TIP_PART_TARGET)
    {
        egui_view_teaching_tip_draw_focus_ring(
                self, &metrics->target_region,
                (local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_RADIUS : EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_RADIUS), focus_color,
                local->compact_mode ? 54 : 62);
    }
    egui_canvas_draw_round_rectangle_fill(metrics->target_region.location.x, metrics->target_region.location.y, metrics->target_region.size.width,
                                          metrics->target_region.size.height,
                                          local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_RADIUS : EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_RADIUS,
                                          fill_color, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(metrics->target_region.location.x, metrics->target_region.location.y, metrics->target_region.size.width,
                                     metrics->target_region.size.height,
                                     local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_RADIUS : EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_RADIUS, 1,
                                     outline_color, egui_color_alpha_mix(self->alpha, 56));

    dot_x = metrics->target_region.location.x + 10;
    dot_y = metrics->target_region.location.y + metrics->target_region.size.height / 2;
    egui_canvas_draw_circle_fill(dot_x, dot_y, local->compact_mode ? 2 : 3, tone_color, egui_color_alpha_mix(self->alpha, 92));

    text_region.location.x = dot_x + (local->compact_mode ? 6 : 8);
    text_region.location.y = metrics->target_region.location.y;
    text_region.size.width = metrics->target_region.size.width - (text_region.location.x - metrics->target_region.location.x) - 8;
    text_region.size.height = metrics->target_region.size.height;
    egui_view_teaching_tip_draw_text(local->font, self, snapshot == NULL ? NULL : snapshot->target_label, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                     text_color);
}

static void egui_view_teaching_tip_draw_action(egui_view_t *self, egui_view_teaching_tip_t *local, const egui_region_t *region, const char *label,
                                               egui_color_t tone_color, egui_color_t border_color, egui_color_t text_color, uint8_t emphasized, uint8_t focused,
                                               uint8_t pressed, uint8_t enabled)
{
    egui_color_t fill_color = emphasized ? tone_color : egui_rgb_mix(local->surface_color, tone_color, focused ? 14 : 8);
    egui_color_t outline_color = emphasized ? egui_rgb_mix(tone_color, EGUI_COLOR_WHITE, 10) : egui_rgb_mix(border_color, tone_color, focused ? 26 : 18);
    egui_color_t label_color = emphasized ? EGUI_COLOR_WHITE : (focused ? tone_color : text_color);
    egui_color_t focus_color = emphasized ? egui_rgb_mix(EGUI_COLOR_WHITE, tone_color, 16) : egui_rgb_mix(tone_color, EGUI_COLOR_WHITE, 10);
    egui_color_t indicator_color = emphasized ? EGUI_COLOR_WHITE : tone_color;

    if (emphasized && local->compact_mode)
    {
        fill_color = egui_rgb_mix(tone_color, local->surface_color, 22);
        outline_color = egui_rgb_mix(outline_color, EGUI_COLOR_WHITE, 12);
    }

    if (!enabled)
    {
        fill_color = egui_rgb_mix(fill_color, local->surface_color, 36);
        outline_color = egui_rgb_mix(outline_color, local->muted_text_color, 34);
        label_color = egui_rgb_mix(label_color, local->muted_text_color, 42);
    }
    if (pressed)
    {
        fill_color = egui_rgb_mix(fill_color, tone_color, 16);
    }
    if (focused && enabled)
    {
        fill_color = emphasized ? egui_rgb_mix(fill_color, EGUI_COLOR_WHITE, 8) : egui_rgb_mix(fill_color, tone_color, 14);
        outline_color = emphasized ? egui_rgb_mix(outline_color, EGUI_COLOR_WHITE, 12) : egui_rgb_mix(outline_color, tone_color, 20);
    }

    if (focused && enabled)
    {
        egui_view_teaching_tip_draw_focus_ring(self, region, local->compact_mode ? 5 : 6, focus_color, emphasized ? 62 : 54);
    }
    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, local->compact_mode ? 5 : 6,
                                          fill_color, egui_color_alpha_mix(self->alpha, emphasized ? 94 : 86));
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, local->compact_mode ? 5 : 6, 1,
                                     outline_color, egui_color_alpha_mix(self->alpha, 54));
    if (focused && enabled && region->size.width > 18)
    {
        egui_canvas_draw_round_rectangle_fill(region->location.x + 6, region->location.y + region->size.height - 4, region->size.width - 12, 2, 1,
                                              indicator_color, egui_color_alpha_mix(self->alpha, emphasized ? 82 : 74));
    }
    egui_view_teaching_tip_draw_text(local->meta_font, self, label, region, EGUI_ALIGN_CENTER, label_color);
}

static void egui_view_teaching_tip_draw_bubble(egui_view_t *self, egui_view_teaching_tip_t *local, const egui_view_teaching_tip_snapshot_t *snapshot,
                                               const egui_view_teaching_tip_metrics_t *metrics, egui_color_t tone_color, egui_color_t border_color,
                                               egui_color_t text_color, egui_color_t muted_text_color)
{
    egui_color_t bubble_fill = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 8 : 10);
    egui_color_t bubble_border = egui_rgb_mix(border_color, tone_color, 20);
    egui_color_t body_color =
            egui_rgb_mix(egui_rgb_mix(text_color, muted_text_color, local->compact_mode ? 68 : 58), tone_color, local->compact_mode ? 10 : 12);
    egui_color_t footer_color = egui_rgb_mix(muted_text_color, local->surface_color, local->compact_mode ? 8 : 16);
    egui_color_t divider_color = egui_rgb_mix(bubble_border, tone_color, 12);
    uint8_t draw_enabled = (egui_view_get_enable(self) && !local->read_only_mode) ? 1 : 0;

    if (!metrics->show_bubble)
    {
        return;
    }

    egui_canvas_draw_round_rectangle_fill(metrics->bubble_region.location.x + 1, metrics->bubble_region.location.y + 2, metrics->bubble_region.size.width,
                                          metrics->bubble_region.size.height,
                                          local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_RADIUS : EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_RADIUS,
                                          local->shadow_color, egui_color_alpha_mix(self->alpha, local->compact_mode ? 18 : 30));
    egui_view_teaching_tip_draw_arrow(self, local, snapshot, metrics, bubble_fill, bubble_border);
    egui_canvas_draw_round_rectangle_fill(metrics->bubble_region.location.x, metrics->bubble_region.location.y, metrics->bubble_region.size.width,
                                          metrics->bubble_region.size.height,
                                          local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_RADIUS : EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_RADIUS,
                                          bubble_fill, egui_color_alpha_mix(self->alpha, 98));
    egui_canvas_draw_round_rectangle(metrics->bubble_region.location.x, metrics->bubble_region.location.y, metrics->bubble_region.size.width,
                                     metrics->bubble_region.size.height,
                                     local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_BUBBLE_RADIUS : EGUI_VIEW_TEACHING_TIP_STANDARD_BUBBLE_RADIUS, 1,
                                     bubble_border, egui_color_alpha_mix(self->alpha, local->compact_mode ? 58 : 62));
    egui_canvas_draw_round_rectangle_fill(metrics->bubble_region.location.x + (local->compact_mode ? 6 : 10), metrics->bubble_region.location.y + 6,
                                          local->compact_mode ? 14 : 24, local->compact_mode ? 2 : 3, 2, tone_color,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 82 : 88));

    if (metrics->show_eyebrow)
    {
        egui_view_teaching_tip_draw_text(local->meta_font, self, snapshot->eyebrow, &metrics->eyebrow_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, tone_color);
    }
    egui_view_teaching_tip_draw_text(local->font, self, snapshot->title, &metrics->title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color);
    if (metrics->show_body)
    {
        egui_view_teaching_tip_draw_text(local->meta_font, self, snapshot->body, &metrics->body_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, body_color);
    }

    if (metrics->show_footer)
    {
        egui_view_teaching_tip_draw_text(local->meta_font, self, snapshot->footer, &metrics->footer_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, footer_color);
    }
    if (!local->compact_mode && metrics->show_primary)
    {
        egui_dim_t divider_y = metrics->primary_region.location.y - 4;
        egui_dim_t divider_x1 = metrics->bubble_region.location.x + 10;
        egui_dim_t divider_x2 = metrics->bubble_region.location.x + metrics->bubble_region.size.width - 11;

        egui_canvas_draw_line(divider_x1, divider_y, divider_x2, divider_y, 1, divider_color, egui_color_alpha_mix(self->alpha, 30));
    }
    if (metrics->show_close)
    {
        egui_color_t close_fill = egui_rgb_mix(local->surface_color, tone_color, local->current_part == EGUI_VIEW_TEACHING_TIP_PART_CLOSE ? 18 : 13);
        egui_color_t close_focus = egui_rgb_mix(tone_color, EGUI_COLOR_WHITE, 10);
        egui_color_t close_icon = egui_rgb_mix(muted_text_color, text_color, 24);

        if (local->pressed_part == EGUI_VIEW_TEACHING_TIP_PART_CLOSE)
        {
            close_fill = egui_rgb_mix(close_fill, tone_color, 16);
        }
        if (local->current_part == EGUI_VIEW_TEACHING_TIP_PART_CLOSE)
        {
            egui_view_teaching_tip_draw_focus_ring(self, &metrics->close_region, 4, close_focus, 42);
        }
        egui_canvas_draw_round_rectangle_fill(metrics->close_region.location.x, metrics->close_region.location.y, metrics->close_region.size.width,
                                              metrics->close_region.size.height, 4, close_fill, egui_color_alpha_mix(self->alpha, 88));
        egui_canvas_draw_round_rectangle(metrics->close_region.location.x, metrics->close_region.location.y, metrics->close_region.size.width,
                                         metrics->close_region.size.height, 4, 1, bubble_border, egui_color_alpha_mix(self->alpha, 56));
        egui_view_teaching_tip_draw_close(self, &metrics->close_region, close_icon, 94);
    }

    if (metrics->show_primary)
    {
        egui_view_teaching_tip_draw_action(self, local, &metrics->primary_region, snapshot->primary_action, tone_color, border_color, text_color, 1,
                                           local->current_part == EGUI_VIEW_TEACHING_TIP_PART_PRIMARY ? 1 : 0,
                                           local->pressed_part == EGUI_VIEW_TEACHING_TIP_PART_PRIMARY ? 1 : 0,
                                           (snapshot->primary_enabled && draw_enabled) ? 1 : 0);
    }
    if (metrics->show_secondary)
    {
        egui_view_teaching_tip_draw_action(self, local, &metrics->secondary_region, snapshot->secondary_action, tone_color, border_color, text_color, 0,
                                           local->current_part == EGUI_VIEW_TEACHING_TIP_PART_SECONDARY ? 1 : 0,
                                           local->pressed_part == EGUI_VIEW_TEACHING_TIP_PART_SECONDARY ? 1 : 0,
                                           (snapshot->secondary_enabled && draw_enabled) ? 1 : 0);
    }
}

static void egui_view_teaching_tip_draw_closed_hint(egui_view_t *self, egui_view_teaching_tip_t *local, const egui_view_teaching_tip_snapshot_t *snapshot,
                                                    const egui_view_teaching_tip_metrics_t *metrics, egui_color_t tone_color, egui_color_t text_color,
                                                    egui_color_t muted_text_color)
{
    const char *hint_body;
    egui_region_t hint_panel_region;
    egui_color_t hint_fill;
    egui_color_t hint_border;

    if (!metrics->show_closed_hint || snapshot == NULL)
    {
        return;
    }

    hint_body = egui_view_teaching_tip_has_text(snapshot->body) ? snapshot->body : snapshot->footer;
    hint_panel_region.location.x = metrics->closed_title_region.location.x - 10;
    hint_panel_region.location.y = metrics->closed_title_region.location.y - 7;
    hint_panel_region.size.width = metrics->closed_title_region.size.width + 20;
    hint_panel_region.size.height = metrics->closed_body_region.location.y + metrics->closed_body_region.size.height - hint_panel_region.location.y + 6;
    hint_fill = egui_rgb_mix(local->surface_color, tone_color, 6);
    hint_border = egui_rgb_mix(muted_text_color, tone_color, 16);

    egui_canvas_draw_round_rectangle_fill(hint_panel_region.location.x, hint_panel_region.location.y, hint_panel_region.size.width,
                                          hint_panel_region.size.height, 6, hint_fill, egui_color_alpha_mix(self->alpha, 72));
    egui_canvas_draw_round_rectangle(hint_panel_region.location.x, hint_panel_region.location.y, hint_panel_region.size.width, hint_panel_region.size.height, 6,
                                     1, hint_border, egui_color_alpha_mix(self->alpha, 30));
    egui_canvas_draw_round_rectangle_fill(hint_panel_region.location.x + hint_panel_region.size.width / 2 - 9, hint_panel_region.location.y + 4, 18, 2, 1,
                                          tone_color, egui_color_alpha_mix(self->alpha, 62));

    egui_view_teaching_tip_draw_text(local->font, self, snapshot->title, &metrics->closed_title_region, EGUI_ALIGN_CENTER, text_color);
    egui_view_teaching_tip_draw_text(local->meta_font, self, hint_body, &metrics->closed_body_region, EGUI_ALIGN_CENTER,
                                     egui_rgb_mix(muted_text_color, tone_color, 14));
}
static void egui_view_teaching_tip_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);
    egui_region_t region;
    egui_view_teaching_tip_metrics_t metrics;
    const egui_view_teaching_tip_snapshot_t *snapshot;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_text_color = local->muted_text_color;
    egui_color_t shadow_color = local->shadow_color;
    egui_color_t tone_color;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    snapshot = egui_view_teaching_tip_get_snapshot(local);
    if (snapshot == NULL)
    {
        return;
    }

    tone_color = egui_view_teaching_tip_tone_color(local, snapshot->tone);
    if (local->read_only_mode)
    {
        tone_color = egui_rgb_mix(tone_color, muted_text_color, 62);
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFBFCFD), 24);
        border_color = egui_rgb_mix(border_color, muted_text_color, 18);
        shadow_color = egui_rgb_mix(shadow_color, surface_color, 34);
    }
    if (!egui_view_get_enable(self))
    {
        tone_color = egui_view_teaching_tip_mix_disabled(tone_color);
        surface_color = egui_view_teaching_tip_mix_disabled(surface_color);
        border_color = egui_view_teaching_tip_mix_disabled(border_color);
        text_color = egui_view_teaching_tip_mix_disabled(text_color);
        muted_text_color = egui_view_teaching_tip_mix_disabled(muted_text_color);
        shadow_color = egui_view_teaching_tip_mix_disabled(shadow_color);
    }
    egui_view_teaching_tip_get_metrics(local, self, snapshot, &metrics);

    egui_canvas_draw_round_rectangle_fill(
            region.location.x, region.location.y, region.size.width, region.size.height,
            local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_RADIUS : EGUI_VIEW_TEACHING_TIP_STANDARD_RADIUS, surface_color,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_FILL_ALPHA : EGUI_VIEW_TEACHING_TIP_STANDARD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height,
                                     local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_RADIUS : EGUI_VIEW_TEACHING_TIP_STANDARD_RADIUS, 1, border_color,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_TEACHING_TIP_COMPACT_BORDER_ALPHA
                                                                                           : EGUI_VIEW_TEACHING_TIP_STANDARD_BORDER_ALPHA));

    egui_view_teaching_tip_draw_target(self, local, snapshot, &metrics, tone_color, border_color, text_color);
    egui_view_teaching_tip_draw_bubble(self, local, snapshot, &metrics, tone_color, border_color, text_color, muted_text_color);
    egui_view_teaching_tip_draw_closed_hint(self, local, snapshot, &metrics, tone_color, text_color, muted_text_color);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_teaching_tip_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);
    uint8_t hit_part;

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = egui_view_teaching_tip_hit_part(local, self, event->location.x, event->location.y);
        if (!egui_view_teaching_tip_part_is_enabled(local, self, egui_view_teaching_tip_get_snapshot(local), hit_part))
        {
            return 0;
        }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (self->is_focusable)
        {
            egui_view_request_focus(self);
        }
#endif
        local->pressed_part = hit_part;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = egui_view_teaching_tip_hit_part(local, self, event->location.x, event->location.y);
        if (local->pressed_part != EGUI_VIEW_TEACHING_TIP_PART_NONE && local->pressed_part == hit_part &&
            egui_view_teaching_tip_part_is_enabled(local, self, egui_view_teaching_tip_get_snapshot(local), hit_part))
        {
            egui_view_teaching_tip_set_current_part_inner(self, hit_part, 1);
        }
        local->pressed_part = EGUI_VIEW_TEACHING_TIP_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_TEACHING_TIP_PART_NONE ? 1 : 0;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_TEACHING_TIP_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

static uint8_t egui_view_teaching_tip_handle_navigation_key_inner(egui_view_t *self, uint8_t key_code)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    EGUI_LOCAL_INIT(egui_view_teaching_tip_t);
    const egui_view_teaching_tip_snapshot_t *snapshot = egui_view_teaching_tip_get_snapshot(local);
    uint8_t parts[4];
    uint8_t count;
    uint8_t index;

    if (snapshot == NULL || !egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    count = egui_view_teaching_tip_collect_parts(local, self, snapshot, parts, 4);
    if (count == 0)
    {
        return 0;
    }

    index = egui_view_teaching_tip_find_part_index(parts, count, local->current_part);
    switch (key_code)
    {
    case EGUI_KEY_CODE_LEFT:
    case EGUI_KEY_CODE_UP:
        if (index > 0)
        {
            egui_view_teaching_tip_set_current_part_inner(self, parts[index - 1], 1);
        }
        else
        {
            egui_view_teaching_tip_set_current_part_inner(self, parts[0], 1);
        }
        return 1;
    case EGUI_KEY_CODE_RIGHT:
    case EGUI_KEY_CODE_DOWN:
        if (index + 1 < count)
        {
            egui_view_teaching_tip_set_current_part_inner(self, parts[index + 1], 1);
        }
        else
        {
            egui_view_teaching_tip_set_current_part_inner(self, parts[count - 1], 1);
        }
        return 1;
    case EGUI_KEY_CODE_HOME:
        egui_view_teaching_tip_set_current_part_inner(self, parts[0], 1);
        return 1;
    case EGUI_KEY_CODE_END:
        egui_view_teaching_tip_set_current_part_inner(self, parts[count - 1], 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        index++;
        if (index >= count)
        {
            index = 0;
        }
        egui_view_teaching_tip_set_current_part_inner(self, parts[index], 1);
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        egui_view_teaching_tip_set_current_part_inner(self, EGUI_VIEW_TEACHING_TIP_PART_TARGET, 1);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        egui_view_teaching_tip_notify_part(local, self, local->current_part);
        return 1;
    default:
        return 0;
    }
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(key_code);
    return 0;
#endif
}

uint8_t egui_view_teaching_tip_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    return egui_view_teaching_tip_handle_navigation_key_inner(self, key_code);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_teaching_tip_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    if (egui_view_teaching_tip_handle_navigation_key_inner(self, event->key_code))
    {
        return 1;
    }

    return egui_view_on_key_event(self, event);
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_teaching_tip_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_teaching_tip_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_teaching_tip_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_teaching_tip_on_key_event,
#endif
};

void egui_view_teaching_tip_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_teaching_tip_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_teaching_tip_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_part_changed = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->text_color = EGUI_COLOR_HEX(0x18222D);
    local->muted_text_color = EGUI_COLOR_HEX(0x69798A);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB77719);
    local->neutral_color = EGUI_COLOR_HEX(0x728091);
    local->shadow_color = EGUI_COLOR_HEX(0xD8DEE6);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->current_part = EGUI_VIEW_TEACHING_TIP_PART_NONE;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->pressed_part = EGUI_VIEW_TEACHING_TIP_PART_NONE;

    egui_view_set_view_name(self, "egui_view_teaching_tip");
}
