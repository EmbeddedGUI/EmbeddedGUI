#include "egui_view_pips_pager.h"

#define PP_STD_RADIUS     10
#define PP_STD_PAD_X      10
#define PP_STD_PAD_Y      8
#define PP_STD_TITLE_H    10
#define PP_STD_TITLE_GAP  4
#define PP_STD_ROW_H      24
#define PP_STD_HELPER_H   10
#define PP_STD_HELPER_GAP 5
#define PP_STD_BUTTON_W   22
#define PP_STD_BUTTON_GAP 8
#define PP_STD_SLOT_W     12
#define PP_STD_SLOT_GAP   4
#define PP_STD_ACTIVE_W   16
#define PP_STD_DOT_D      6

#define PP_COMPACT_RADIUS   8
#define PP_COMPACT_PAD_X    7
#define PP_COMPACT_PAD_Y    6
#define PP_COMPACT_ROW_H    18
#define PP_COMPACT_BUTTON_W 16
#define PP_COMPACT_GAP      6
#define PP_COMPACT_SLOT_W   10
#define PP_COMPACT_SLOT_GAP 3
#define PP_COMPACT_ACTIVE_W 12
#define PP_COMPACT_DOT_D    5

typedef struct egui_view_pips_pager_metrics egui_view_pips_pager_metrics_t;
struct egui_view_pips_pager_metrics
{
    egui_region_t title_region;
    egui_region_t helper_region;
    egui_region_t row_region;
    egui_region_t previous_region;
    egui_region_t rail_region;
    egui_region_t next_region;
    uint8_t show_title;
    uint8_t show_helper;
    uint8_t window_start;
    uint8_t window_count;
};

static uint8_t pips_pager_has_text(const char *text)
{
    return (text != NULL && text[0] != '\0') ? 1 : 0;
}

static void pips_pager_normalize_state(egui_view_pips_pager_t *local)
{
    if (local->total_count == 0)
    {
        local->current_index = 0;
        local->current_part = EGUI_VIEW_PIPS_PAGER_PART_NONE;
        local->pressed_part = EGUI_VIEW_PIPS_PAGER_PART_NONE;
        local->pressed_index = 0;
        return;
    }

    if (local->visible_count < 1)
    {
        local->visible_count = local->total_count > 0 ? local->total_count : 1;
    }
    if (local->visible_count > local->total_count)
    {
        local->visible_count = local->total_count;
    }
    if (local->current_index >= local->total_count)
    {
        local->current_index = (uint8_t)(local->total_count - 1);
    }
    if (local->current_part != EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS && local->current_part != EGUI_VIEW_PIPS_PAGER_PART_PIP &&
        local->current_part != EGUI_VIEW_PIPS_PAGER_PART_NEXT)
    {
        local->current_part = EGUI_VIEW_PIPS_PAGER_PART_PIP;
    }
    if (local->pressed_part != EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS && local->pressed_part != EGUI_VIEW_PIPS_PAGER_PART_PIP &&
        local->pressed_part != EGUI_VIEW_PIPS_PAGER_PART_NEXT)
    {
        local->pressed_part = EGUI_VIEW_PIPS_PAGER_PART_NONE;
        local->pressed_index = 0;
    }
    if (local->compact_mode || local->read_only_mode)
    {
        local->pressed_part = EGUI_VIEW_PIPS_PAGER_PART_NONE;
        local->pressed_index = 0;
    }
}

static uint8_t pips_pager_get_window_start(egui_view_pips_pager_t *local)
{
    int start;
    int max_start;

    if (local->total_count <= local->visible_count)
    {
        return 0;
    }

    start = (int)local->current_index - (int)(local->visible_count / 2);
    max_start = (int)local->total_count - (int)local->visible_count;
    if (start < 0)
    {
        start = 0;
    }
    if (start > max_start)
    {
        start = max_start;
    }
    return (uint8_t)start;
}

static void pips_pager_notify(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);

    if (local->on_changed != NULL)
    {
        local->on_changed(self, local->current_index, local->total_count, local->current_part);
    }
}

static uint8_t pips_pager_part_enabled(egui_view_pips_pager_t *local, egui_view_t *self, uint8_t part)
{
    if (!egui_view_get_enable(self) || local->read_only_mode || local->total_count == 0)
    {
        return 0;
    }

    if (part == EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS)
    {
        return local->current_index > 0 ? 1 : 0;
    }
    if (part == EGUI_VIEW_PIPS_PAGER_PART_NEXT)
    {
        return local->current_index + 1 < local->total_count ? 1 : 0;
    }
    if (part == EGUI_VIEW_PIPS_PAGER_PART_PIP)
    {
        return 1;
    }

    return 0;
}

static void pips_pager_set_current_part_inner(egui_view_t *self, uint8_t part, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);

    pips_pager_normalize_state(local);
    if (!pips_pager_part_enabled(local, self, part) && part != EGUI_VIEW_PIPS_PAGER_PART_PIP)
    {
        part = EGUI_VIEW_PIPS_PAGER_PART_PIP;
    }
    if (local->current_part != part)
    {
        local->current_part = part;
        egui_view_invalidate(self);
    }
    if (notify)
    {
        pips_pager_notify(self);
    }
}

static void pips_pager_set_current_index_inner(egui_view_t *self, uint8_t current_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);

    if (local->total_count == 0)
    {
        return;
    }
    if (current_index >= local->total_count)
    {
        current_index = (uint8_t)(local->total_count - 1);
    }
    if (local->current_index != current_index)
    {
        local->current_index = current_index;
        egui_view_invalidate(self);
    }
    if (notify)
    {
        pips_pager_notify(self);
    }
}

static void pips_pager_change_page(egui_view_t *self, int delta, uint8_t focus_part)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    int next;

    if (local->total_count == 0)
    {
        return;
    }

    next = (int)local->current_index + delta;
    if (next < 0)
    {
        next = 0;
    }
    if (next >= local->total_count)
    {
        next = local->total_count - 1;
    }
    local->current_part = focus_part;
    pips_pager_set_current_index_inner(self, (uint8_t)next, 1);
}

static void pips_pager_get_metrics(egui_view_pips_pager_t *local, egui_view_t *self, egui_view_pips_pager_metrics_t *metrics)
{
    egui_region_t work_region;
    egui_dim_t pad_x = local->compact_mode ? PP_COMPACT_PAD_X : PP_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? PP_COMPACT_PAD_Y : PP_STD_PAD_Y;
    egui_dim_t button_w = local->compact_mode ? PP_COMPACT_BUTTON_W : PP_STD_BUTTON_W;
    egui_dim_t gap = local->compact_mode ? PP_COMPACT_GAP : PP_STD_BUTTON_GAP;
    egui_dim_t slot_w = local->compact_mode ? PP_COMPACT_SLOT_W : PP_STD_SLOT_W;
    egui_dim_t slot_gap = local->compact_mode ? PP_COMPACT_SLOT_GAP : PP_STD_SLOT_GAP;
    egui_dim_t row_y;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t content_h;
    egui_dim_t rail_w;

    pips_pager_normalize_state(local);
    egui_view_get_work_region(self, &work_region);
    content_x = work_region.location.x + pad_x;
    content_y = work_region.location.y + pad_y;
    content_w = work_region.size.width - pad_x * 2;
    content_h = work_region.size.height - pad_y * 2;

    metrics->show_title = (!local->compact_mode && pips_pager_has_text(local->title)) ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && pips_pager_has_text(local->helper)) ? 1 : 0;
    metrics->window_start = pips_pager_get_window_start(local);
    metrics->window_count = local->total_count == 0 ? 0 : local->visible_count;

    metrics->title_region.location.x = content_x;
    metrics->title_region.location.y = content_y;
    metrics->title_region.size.width = content_w;
    metrics->title_region.size.height = PP_STD_TITLE_H;

    if (metrics->show_title)
    {
        row_y = content_y + PP_STD_TITLE_H + PP_STD_TITLE_GAP;
    }
    else
    {
        row_y = content_y + (content_h - (local->compact_mode ? PP_COMPACT_ROW_H : PP_STD_ROW_H)) / 2;
    }

    if (metrics->show_helper)
    {
        row_y = content_y + (content_h - (PP_STD_ROW_H + PP_STD_HELPER_GAP + PP_STD_HELPER_H)) / 2;
    }

    metrics->row_region.location.x = content_x;
    metrics->row_region.location.y = row_y;
    metrics->row_region.size.width = content_w;
    metrics->row_region.size.height = local->compact_mode ? PP_COMPACT_ROW_H : PP_STD_ROW_H;

    rail_w = button_w * 2 + gap * 2;
    if (metrics->window_count > 0)
    {
        rail_w += metrics->window_count * slot_w + (metrics->window_count - 1) * slot_gap;
    }
    if (rail_w > content_w)
    {
        rail_w = content_w;
    }

    metrics->previous_region.location.x = content_x + (content_w - rail_w) / 2;
    metrics->previous_region.location.y = row_y;
    metrics->previous_region.size.width = button_w;
    metrics->previous_region.size.height = metrics->row_region.size.height;

    metrics->rail_region.location.x = metrics->previous_region.location.x + button_w + gap;
    metrics->rail_region.location.y = row_y;
    metrics->rail_region.size.width = rail_w - button_w * 2 - gap * 2;
    metrics->rail_region.size.height = metrics->row_region.size.height;

    metrics->next_region.location.x = metrics->rail_region.location.x + metrics->rail_region.size.width + gap;
    metrics->next_region.location.y = row_y;
    metrics->next_region.size.width = button_w;
    metrics->next_region.size.height = metrics->row_region.size.height;

    metrics->helper_region.location.x = content_x;
    metrics->helper_region.location.y = row_y + metrics->row_region.size.height + PP_STD_HELPER_GAP;
    metrics->helper_region.size.width = content_w;
    metrics->helper_region.size.height = PP_STD_HELPER_H;
}

static uint8_t pips_pager_get_pip_region_inner(egui_view_pips_pager_t *local, egui_view_t *self, uint8_t index, egui_region_t *region)
{
    egui_view_pips_pager_metrics_t metrics;
    egui_dim_t slot_w = local->compact_mode ? PP_COMPACT_SLOT_W : PP_STD_SLOT_W;
    egui_dim_t slot_gap = local->compact_mode ? PP_COMPACT_SLOT_GAP : PP_STD_SLOT_GAP;
    uint8_t offset;

    if (local->total_count == 0 || region == NULL)
    {
        return 0;
    }

    pips_pager_get_metrics(local, self, &metrics);
    if (index < metrics.window_start || index >= metrics.window_start + metrics.window_count)
    {
        return 0;
    }

    offset = (uint8_t)(index - metrics.window_start);
    region->location.x = metrics.rail_region.location.x + offset * (slot_w + slot_gap);
    region->location.y = metrics.rail_region.location.y;
    region->size.width = slot_w;
    region->size.height = metrics.rail_region.size.height;
    return 1;
}

static uint8_t pips_pager_hit_part(egui_view_pips_pager_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y, uint8_t *index)
{
    egui_view_pips_pager_metrics_t metrics;
    egui_region_t pip_region;
    uint8_t i;

    if (index != NULL)
    {
        *index = 0;
    }

    pips_pager_get_metrics(local, self, &metrics);
    if (egui_region_pt_in_rect(&metrics.previous_region, x, y))
    {
        return EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS;
    }
    if (egui_region_pt_in_rect(&metrics.next_region, x, y))
    {
        return EGUI_VIEW_PIPS_PAGER_PART_NEXT;
    }
    for (i = metrics.window_start; i < metrics.window_start + metrics.window_count; i++)
    {
        if (pips_pager_get_pip_region_inner(local, self, i, &pip_region) && egui_region_pt_in_rect(&pip_region, x, y))
        {
            if (index != NULL)
            {
                *index = i;
            }
            return EGUI_VIEW_PIPS_PAGER_PART_PIP;
        }
    }

    return EGUI_VIEW_PIPS_PAGER_PART_NONE;
}

void egui_view_pips_pager_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_pips_pager_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_pips_pager_set_title(egui_view_t *self, const char *title)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    local->title = title;
    egui_view_invalidate(self);
}

void egui_view_pips_pager_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_pips_pager_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                      egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t inactive_color, egui_color_t preview_color)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->inactive_color = inactive_color;
    local->preview_color = preview_color;
    egui_view_invalidate(self);
}

void egui_view_pips_pager_set_page_metrics(egui_view_t *self, uint8_t total_count, uint8_t current_index, uint8_t visible_count)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    local->total_count = total_count;
    local->current_index = current_index;
    local->visible_count = visible_count;
    pips_pager_normalize_state(local);
    egui_view_invalidate(self);
}

uint8_t egui_view_pips_pager_get_total_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    return local->total_count;
}

uint8_t egui_view_pips_pager_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    return local->current_index;
}

uint8_t egui_view_pips_pager_get_visible_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    return local->visible_count;
}

void egui_view_pips_pager_set_current_index(egui_view_t *self, uint8_t current_index)
{
    pips_pager_set_current_index_inner(self, current_index, 0);
}

void egui_view_pips_pager_set_current_part(egui_view_t *self, uint8_t part)
{
    pips_pager_set_current_part_inner(self, part, 0);
}

uint8_t egui_view_pips_pager_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    return local->current_part;
}

void egui_view_pips_pager_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    local->compact_mode = compact_mode ? 1 : 0;
    pips_pager_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_pips_pager_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    pips_pager_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_pips_pager_set_on_changed_listener(egui_view_t *self, egui_view_on_pips_pager_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    local->on_changed = listener;
}

uint8_t egui_view_pips_pager_get_part_region(egui_view_t *self, uint8_t part, uint8_t index, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    egui_view_pips_pager_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }

    pips_pager_get_metrics(local, self, &metrics);
    if (part == EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS)
    {
        *region = metrics.previous_region;
        return 1;
    }
    if (part == EGUI_VIEW_PIPS_PAGER_PART_NEXT)
    {
        *region = metrics.next_region;
        return 1;
    }
    if (part == EGUI_VIEW_PIPS_PAGER_PART_PIP)
    {
        return pips_pager_get_pip_region_inner(local, self, index, region);
    }

    return 0;
}

uint8_t egui_view_pips_pager_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    uint8_t next_part;

    pips_pager_normalize_state(local);
    if (local->read_only_mode || local->compact_mode || local->total_count == 0 || !egui_view_get_enable(self))
    {
        return 0;
    }

    switch (key_code)
    {
    case EGUI_KEY_CODE_LEFT:
        if (local->current_part == EGUI_VIEW_PIPS_PAGER_PART_NEXT)
        {
            pips_pager_set_current_part_inner(self, EGUI_VIEW_PIPS_PAGER_PART_PIP, 1);
        }
        else
        {
            pips_pager_change_page(self, -1, EGUI_VIEW_PIPS_PAGER_PART_PIP);
        }
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        if (local->current_part == EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS)
        {
            pips_pager_set_current_part_inner(self, EGUI_VIEW_PIPS_PAGER_PART_PIP, 1);
        }
        else
        {
            pips_pager_change_page(self, 1, EGUI_VIEW_PIPS_PAGER_PART_PIP);
        }
        return 1;
    case EGUI_KEY_CODE_HOME:
        local->current_part = EGUI_VIEW_PIPS_PAGER_PART_PIP;
        pips_pager_set_current_index_inner(self, 0, 1);
        return 1;
    case EGUI_KEY_CODE_END:
        local->current_part = EGUI_VIEW_PIPS_PAGER_PART_PIP;
        pips_pager_set_current_index_inner(self, (uint8_t)(local->total_count - 1), 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        if (local->current_part == EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS)
        {
            next_part = EGUI_VIEW_PIPS_PAGER_PART_PIP;
        }
        else if (local->current_part == EGUI_VIEW_PIPS_PAGER_PART_PIP)
        {
            next_part = EGUI_VIEW_PIPS_PAGER_PART_NEXT;
        }
        else
        {
            next_part = EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS;
        }
        pips_pager_set_current_part_inner(self, next_part, 1);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (local->current_part == EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS)
        {
            pips_pager_change_page(self, -1, EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS);
        }
        else if (local->current_part == EGUI_VIEW_PIPS_PAGER_PART_NEXT)
        {
            pips_pager_change_page(self, 1, EGUI_VIEW_PIPS_PAGER_PART_NEXT);
        }
        else
        {
            pips_pager_notify(self);
        }
        return 1;
    case EGUI_KEY_CODE_PLUS:
        pips_pager_change_page(self, 1, EGUI_VIEW_PIPS_PAGER_PART_PIP);
        return 1;
    case EGUI_KEY_CODE_MINUS:
        pips_pager_change_page(self, -1, EGUI_VIEW_PIPS_PAGER_PART_PIP);
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        pips_pager_set_current_part_inner(self, EGUI_VIEW_PIPS_PAGER_PART_PIP, 1);
        return 1;
    default:
        return 0;
    }
}

static void pips_pager_draw_chevron(egui_view_t *self, const egui_region_t *region, egui_color_t color, uint8_t is_next)
{
    egui_dim_t cx = region->location.x + region->size.width / 2;
    egui_dim_t cy = region->location.y + region->size.height / 2;
    egui_alpha_t alpha = egui_color_alpha_mix(self->alpha, 92);

    if (is_next)
    {
        egui_canvas_draw_line(cx - 2, cy - 3, cx + 1, cy, 1, color, alpha);
        egui_canvas_draw_line(cx + 1, cy, cx - 2, cy + 3, 1, color, alpha);
    }
    else
    {
        egui_canvas_draw_line(cx + 2, cy - 3, cx - 1, cy, 1, color, alpha);
        egui_canvas_draw_line(cx - 1, cy, cx + 2, cy + 3, 1, color, alpha);
    }
}

static void pips_pager_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (!pips_pager_has_text(text))
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, color, self->alpha);
}

static void pips_pager_draw_focus(egui_view_t *self, const egui_region_t *region, egui_dim_t radius, egui_color_t color)
{
    egui_canvas_draw_round_rectangle(region->location.x - 1, region->location.y - 1, region->size.width + 2, region->size.height + 2, radius, 1, color,
                                     egui_color_alpha_mix(self->alpha, 68));
}

static void egui_view_pips_pager_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    egui_view_pips_pager_metrics_t metrics;
    egui_region_t pip_region;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_text_color = local->muted_text_color;
    egui_color_t accent_color = local->accent_color;
    egui_dim_t outer_radius = local->compact_mode ? PP_COMPACT_RADIUS : PP_STD_RADIUS;
    egui_dim_t button_radius = local->compact_mode ? 5 : 7;
    egui_dim_t active_w = local->compact_mode ? PP_COMPACT_ACTIVE_W : PP_STD_ACTIVE_W;
    egui_dim_t dot_d = local->compact_mode ? PP_COMPACT_DOT_D : PP_STD_DOT_D;
    uint8_t i;

    pips_pager_normalize_state(local);
    if (local->read_only_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xEEF2F6), 50);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0x9AA7B4), 44);
        text_color = egui_rgb_mix(text_color, EGUI_COLOR_HEX(0x7A8794), 38);
        muted_text_color = egui_rgb_mix(muted_text_color, EGUI_COLOR_HEX(0x8B98A5), 38);
        accent_color = egui_rgb_mix(accent_color, EGUI_COLOR_HEX(0x9FB1C5), 36);
    }

    pips_pager_get_metrics(local, self, &metrics);

    egui_canvas_draw_round_rectangle_fill(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                          self->region_screen.size.height, outer_radius, surface_color, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                     self->region_screen.size.height, outer_radius, 1, border_color, egui_color_alpha_mix(self->alpha, 58));

    pips_pager_draw_text(local->meta_font, self, local->title, &metrics.title_region, muted_text_color);
    pips_pager_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, muted_text_color);

    egui_canvas_draw_round_rectangle_fill(metrics.previous_region.location.x, metrics.previous_region.location.y, metrics.previous_region.size.width,
                                          metrics.previous_region.size.height, button_radius,
                                          egui_rgb_mix(surface_color, accent_color, local->pressed_part == EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS ? 28 : 10),
                                          egui_color_alpha_mix(self->alpha, 94));
    egui_canvas_draw_round_rectangle(metrics.previous_region.location.x, metrics.previous_region.location.y, metrics.previous_region.size.width,
                                     metrics.previous_region.size.height, button_radius, 1, egui_rgb_mix(border_color, accent_color, 20),
                                     egui_color_alpha_mix(self->alpha, 44));
    pips_pager_draw_chevron(self, &metrics.previous_region,
                            pips_pager_part_enabled(local, self, EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS) ? text_color : muted_text_color, 0);

    egui_canvas_draw_round_rectangle_fill(
            metrics.next_region.location.x, metrics.next_region.location.y, metrics.next_region.size.width, metrics.next_region.size.height, button_radius,
            egui_rgb_mix(surface_color, accent_color, local->pressed_part == EGUI_VIEW_PIPS_PAGER_PART_NEXT ? 28 : 10), egui_color_alpha_mix(self->alpha, 94));
    egui_canvas_draw_round_rectangle(metrics.next_region.location.x, metrics.next_region.location.y, metrics.next_region.size.width,
                                     metrics.next_region.size.height, button_radius, 1, egui_rgb_mix(border_color, accent_color, 20),
                                     egui_color_alpha_mix(self->alpha, 44));
    pips_pager_draw_chevron(self, &metrics.next_region, pips_pager_part_enabled(local, self, EGUI_VIEW_PIPS_PAGER_PART_NEXT) ? text_color : muted_text_color,
                            1);

    for (i = metrics.window_start; i < metrics.window_start + metrics.window_count; i++)
    {
        egui_dim_t dot_x;
        egui_dim_t dot_y;
        if (!pips_pager_get_pip_region_inner(local, self, i, &pip_region))
        {
            continue;
        }
        if (i == local->current_index)
        {
            egui_dim_t pill_x = pip_region.location.x + (pip_region.size.width - active_w) / 2;
            egui_dim_t pill_y = pip_region.location.y + (pip_region.size.height - dot_d) / 2;
            egui_canvas_draw_round_rectangle_fill(pill_x, pill_y, active_w, dot_d, dot_d / 2, accent_color, egui_color_alpha_mix(self->alpha, 100));
        }
        else
        {
            dot_x = pip_region.location.x + pip_region.size.width / 2;
            dot_y = pip_region.location.y + pip_region.size.height / 2;
            egui_canvas_draw_circle_fill(dot_x, dot_y, dot_d / 2, local->compact_mode ? local->preview_color : local->inactive_color,
                                         egui_color_alpha_mix(self->alpha, 96));
        }
        if (local->current_part == EGUI_VIEW_PIPS_PAGER_PART_PIP && i == local->current_index && !local->read_only_mode)
        {
            pips_pager_draw_focus(self, &pip_region, dot_d / 2 + 2, accent_color);
        }
    }

    if (local->current_part == EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS && !local->read_only_mode)
    {
        pips_pager_draw_focus(self, &metrics.previous_region, button_radius, accent_color);
    }
    else if (local->current_part == EGUI_VIEW_PIPS_PAGER_PART_NEXT && !local->read_only_mode)
    {
        pips_pager_draw_focus(self, &metrics.next_region, button_radius, accent_color);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_pips_pager_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_pips_pager_t);
    uint8_t hit_part;
    uint8_t hit_index = 0;

    pips_pager_normalize_state(local);
    if (local->read_only_mode || local->compact_mode || local->total_count == 0 || !egui_view_get_enable(self))
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = pips_pager_hit_part(local, self, event->location.x, event->location.y, &hit_index);
        if (hit_part == EGUI_VIEW_PIPS_PAGER_PART_NONE)
        {
            return 0;
        }
        local->pressed_part = hit_part;
        local->pressed_index = hit_index;
        local->current_part = hit_part == EGUI_VIEW_PIPS_PAGER_PART_PIP ? EGUI_VIEW_PIPS_PAGER_PART_PIP : hit_part;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = pips_pager_hit_part(local, self, event->location.x, event->location.y, &hit_index);
        if (local->pressed_part == hit_part)
        {
            if (hit_part == EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS)
            {
                pips_pager_change_page(self, -1, EGUI_VIEW_PIPS_PAGER_PART_PREVIOUS);
            }
            else if (hit_part == EGUI_VIEW_PIPS_PAGER_PART_NEXT)
            {
                pips_pager_change_page(self, 1, EGUI_VIEW_PIPS_PAGER_PART_NEXT);
            }
            else if (hit_part == EGUI_VIEW_PIPS_PAGER_PART_PIP && hit_index == local->pressed_index)
            {
                local->current_index = hit_index;
                local->current_part = EGUI_VIEW_PIPS_PAGER_PART_PIP;
                egui_view_invalidate(self);
                pips_pager_notify(self);
            }
        }
        local->pressed_part = EGUI_VIEW_PIPS_PAGER_PART_NONE;
        local->pressed_index = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_PIPS_PAGER_PART_NONE;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_PIPS_PAGER_PART_NONE;
        local->pressed_index = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_pips_pager_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    if (egui_view_pips_pager_handle_navigation_key(self, event->key_code))
    {
        return 1;
    }

    return egui_view_on_key_event(self, event);
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_pips_pager_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_pips_pager_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_pips_pager_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_pips_pager_on_key_event,
#endif
};

void egui_view_pips_pager_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_pips_pager_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_pips_pager_t);
    egui_view_set_padding_all(self, 2);

    local->on_changed = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->title = NULL;
    local->helper = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD6DEE7);
    local->text_color = EGUI_COLOR_HEX(0x1D2630);
    local->muted_text_color = EGUI_COLOR_HEX(0x6F7B89);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->inactive_color = EGUI_COLOR_HEX(0xAAB6C3);
    local->preview_color = EGUI_COLOR_HEX(0x6DD3C4);
    local->total_count = 5;
    local->current_index = 0;
    local->visible_count = 5;
    local->current_part = EGUI_VIEW_PIPS_PAGER_PART_PIP;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->pressed_part = EGUI_VIEW_PIPS_PAGER_PART_NONE;
    local->pressed_index = 0;

    egui_view_set_view_name(self, "egui_view_pips_pager");
}
