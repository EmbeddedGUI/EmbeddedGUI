#include <string.h>

#include "egui_view_flip_view.h"
#include "utils/egui_sprintf.h"

#define FV_STD_RADIUS        12
#define FV_STD_PAD_X         10
#define FV_STD_PAD_Y         9
#define FV_STD_TITLE_H       10
#define FV_STD_TITLE_GAP     5
#define FV_STD_HELPER_H      10
#define FV_STD_HELPER_GAP    5
#define FV_STD_BUTTON_W      18
#define FV_STD_BUTTON_H      34
#define FV_STD_BUTTON_INSET  6
#define FV_STD_CARD_SHADOW_X 2
#define FV_STD_CARD_SHADOW_Y 4

#define FV_COMPACT_RADIUS       9
#define FV_COMPACT_PAD_X        7
#define FV_COMPACT_PAD_Y        6
#define FV_COMPACT_BUTTON_W     14
#define FV_COMPACT_BUTTON_H     26
#define FV_COMPACT_BUTTON_INSET 5

typedef struct egui_view_flip_view_metrics egui_view_flip_view_metrics_t;
struct egui_view_flip_view_metrics
{
    egui_region_t title_region;
    egui_region_t surface_region;
    egui_region_t helper_region;
    egui_region_t previous_region;
    egui_region_t next_region;
    uint8_t show_title;
    uint8_t show_helper;
};

static uint8_t flip_view_has_text(const char *text)
{
    return (text != NULL && text[0] != '\0') ? 1 : 0;
}

static void flip_view_draw_round_fill_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle_fill(x, y, w, h, radius, color, alpha);
}

static void flip_view_draw_round_stroke_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_dim_t stroke_width,
                                             egui_color_t color, egui_alpha_t alpha)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(x, y, w, h, radius, stroke_width, color, alpha);
}

static void flip_view_normalize_state(egui_view_flip_view_t *local)
{
    if (local->item_count == 0 || local->items == NULL)
    {
        local->current_index = 0;
        local->current_part = EGUI_VIEW_FLIP_VIEW_PART_NONE;
        local->pressed_part = EGUI_VIEW_FLIP_VIEW_PART_NONE;
        return;
    }

    if (local->current_index >= local->item_count)
    {
        local->current_index = (uint8_t)(local->item_count - 1);
    }
    if (local->current_part != EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS && local->current_part != EGUI_VIEW_FLIP_VIEW_PART_SURFACE &&
        local->current_part != EGUI_VIEW_FLIP_VIEW_PART_NEXT)
    {
        local->current_part = EGUI_VIEW_FLIP_VIEW_PART_SURFACE;
    }
    if (local->pressed_part != EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS && local->pressed_part != EGUI_VIEW_FLIP_VIEW_PART_SURFACE &&
        local->pressed_part != EGUI_VIEW_FLIP_VIEW_PART_NEXT)
    {
        local->pressed_part = EGUI_VIEW_FLIP_VIEW_PART_NONE;
    }
    if (local->compact_mode || local->read_only_mode)
    {
        local->pressed_part = EGUI_VIEW_FLIP_VIEW_PART_NONE;
    }
    if (local->current_part == EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS && local->current_index == 0)
    {
        local->current_part = EGUI_VIEW_FLIP_VIEW_PART_SURFACE;
    }
    if (local->current_part == EGUI_VIEW_FLIP_VIEW_PART_NEXT && local->current_index + 1 >= local->item_count)
    {
        local->current_part = EGUI_VIEW_FLIP_VIEW_PART_SURFACE;
    }
}

static const egui_view_flip_view_item_t *flip_view_get_item_inner(egui_view_flip_view_t *local)
{
    flip_view_normalize_state(local);
    if (local->item_count == 0 || local->items == NULL)
    {
        return NULL;
    }
    return &local->items[local->current_index];
}

static void flip_view_format_counter(char *buffer, size_t size, uint8_t current_index, uint8_t item_count)
{
    int pos = 0;

    if (buffer == NULL || size == 0)
    {
        return;
    }
    buffer[0] = '\0';
    pos += egui_sprintf_int(buffer, (int)size, current_index + 1);
    if (pos < (int)size)
    {
        pos += egui_sprintf_str(&buffer[pos], (int)size - pos, " / ");
    }
    if (pos < (int)size)
    {
        egui_sprintf_int(&buffer[pos], (int)size - pos, item_count);
    }
}

static void flip_view_notify(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);

    if (local->on_changed != NULL)
    {
        local->on_changed(self, local->current_index, local->item_count, local->current_part);
    }
}

static uint8_t flip_view_part_enabled(egui_view_flip_view_t *local, egui_view_t *self, uint8_t part)
{
    if (!egui_view_get_enable(self) || local->read_only_mode || local->item_count == 0 || local->items == NULL)
    {
        return 0;
    }

    if (part == EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS)
    {
        return local->current_index > 0 ? 1 : 0;
    }
    if (part == EGUI_VIEW_FLIP_VIEW_PART_NEXT)
    {
        return local->current_index + 1 < local->item_count ? 1 : 0;
    }
    if (part == EGUI_VIEW_FLIP_VIEW_PART_SURFACE)
    {
        return 1;
    }
    return 0;
}

static void flip_view_set_current_part_inner(egui_view_t *self, uint8_t part, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);

    flip_view_normalize_state(local);
    if (part != EGUI_VIEW_FLIP_VIEW_PART_SURFACE && !flip_view_part_enabled(local, self, part))
    {
        part = EGUI_VIEW_FLIP_VIEW_PART_SURFACE;
    }
    if (local->current_part != part)
    {
        local->current_part = part;
        egui_view_invalidate(self);
    }
    if (notify)
    {
        flip_view_notify(self);
    }
}

static void flip_view_set_current_index_inner(egui_view_t *self, uint8_t current_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);

    if (local->item_count == 0 || local->items == NULL)
    {
        return;
    }
    if (current_index >= local->item_count)
    {
        current_index = (uint8_t)(local->item_count - 1);
    }
    if (local->current_index != current_index)
    {
        local->current_index = current_index;
        egui_view_invalidate(self);
    }
    if (notify)
    {
        flip_view_notify(self);
    }
}

static void flip_view_change_index(egui_view_t *self, int delta, uint8_t focus_part)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    int next;

    if (local->item_count == 0 || local->items == NULL)
    {
        return;
    }

    next = (int)local->current_index + delta;
    if (next < 0)
    {
        next = 0;
    }
    if (next >= local->item_count)
    {
        next = local->item_count - 1;
    }
    local->current_part = focus_part;
    flip_view_set_current_index_inner(self, (uint8_t)next, 1);
}

static void flip_view_get_metrics(egui_view_flip_view_t *local, egui_view_t *self, egui_view_flip_view_metrics_t *metrics)
{
    egui_region_t work_region;
    egui_dim_t pad_x = local->compact_mode ? FV_COMPACT_PAD_X : FV_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? FV_COMPACT_PAD_Y : FV_STD_PAD_Y;
    egui_dim_t button_w = local->compact_mode ? FV_COMPACT_BUTTON_W : FV_STD_BUTTON_W;
    egui_dim_t button_h = local->compact_mode ? FV_COMPACT_BUTTON_H : FV_STD_BUTTON_H;
    egui_dim_t button_inset = local->compact_mode ? FV_COMPACT_BUTTON_INSET : FV_STD_BUTTON_INSET;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t content_h;

    flip_view_normalize_state(local);
    egui_view_get_work_region(self, &work_region);

    content_x = work_region.location.x + pad_x;
    content_y = work_region.location.y + pad_y;
    content_w = work_region.size.width - pad_x * 2;
    content_h = work_region.size.height - pad_y * 2;

    metrics->show_title = (!local->compact_mode && flip_view_has_text(local->title)) ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && flip_view_has_text(local->helper)) ? 1 : 0;

    metrics->title_region.location.x = content_x;
    metrics->title_region.location.y = content_y;
    metrics->title_region.size.width = content_w;
    metrics->title_region.size.height = FV_STD_TITLE_H;

    if (metrics->show_title)
    {
        content_y += FV_STD_TITLE_H + FV_STD_TITLE_GAP;
        content_h -= FV_STD_TITLE_H + FV_STD_TITLE_GAP;
    }
    if (metrics->show_helper)
    {
        content_h -= FV_STD_HELPER_H + FV_STD_HELPER_GAP;
    }

    metrics->surface_region.location.x = content_x;
    metrics->surface_region.location.y = content_y;
    metrics->surface_region.size.width = content_w;
    metrics->surface_region.size.height = content_h;

    metrics->helper_region.location.x = content_x;
    metrics->helper_region.location.y = content_y + content_h + FV_STD_HELPER_GAP;
    metrics->helper_region.size.width = content_w;
    metrics->helper_region.size.height = FV_STD_HELPER_H;

    metrics->previous_region.location.x = content_x + button_inset;
    metrics->previous_region.location.y = content_y + (content_h - button_h) / 2;
    metrics->previous_region.size.width = button_w;
    metrics->previous_region.size.height = button_h;

    metrics->next_region.location.x = content_x + content_w - button_w - button_inset;
    metrics->next_region.location.y = metrics->previous_region.location.y;
    metrics->next_region.size.width = button_w;
    metrics->next_region.size.height = button_h;
}

void egui_view_flip_view_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_flip_view_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_flip_view_set_title(egui_view_t *self, const char *title)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    local->title = title;
    egui_view_invalidate(self);
}

void egui_view_flip_view_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_flip_view_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                     egui_color_t muted_text_color, egui_color_t inactive_color)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->inactive_color = inactive_color;
    egui_view_invalidate(self);
}

void egui_view_flip_view_set_items(egui_view_t *self, const egui_view_flip_view_item_t *items, uint8_t item_count, uint8_t current_index)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    local->items = items;
    local->item_count = item_count;
    local->current_index = current_index;
    flip_view_normalize_state(local);
    egui_view_invalidate(self);
}

const egui_view_flip_view_item_t *egui_view_flip_view_get_current_item(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    return flip_view_get_item_inner(local);
}

uint8_t egui_view_flip_view_get_item_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    return local->item_count;
}

uint8_t egui_view_flip_view_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    return local->current_index;
}

void egui_view_flip_view_set_current_index(egui_view_t *self, uint8_t current_index)
{
    flip_view_set_current_index_inner(self, current_index, 0);
}

void egui_view_flip_view_set_current_part(egui_view_t *self, uint8_t part)
{
    flip_view_set_current_part_inner(self, part, 0);
}

uint8_t egui_view_flip_view_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    return local->current_part;
}

void egui_view_flip_view_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    local->compact_mode = compact_mode ? 1 : 0;
    flip_view_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_flip_view_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    flip_view_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_flip_view_set_on_changed_listener(egui_view_t *self, egui_view_on_flip_view_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    local->on_changed = listener;
}

uint8_t egui_view_flip_view_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    egui_view_flip_view_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }

    flip_view_get_metrics(local, self, &metrics);
    if (part == EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS)
    {
        *region = metrics.previous_region;
        return 1;
    }
    if (part == EGUI_VIEW_FLIP_VIEW_PART_SURFACE)
    {
        *region = metrics.surface_region;
        return 1;
    }
    if (part == EGUI_VIEW_FLIP_VIEW_PART_NEXT)
    {
        *region = metrics.next_region;
        return 1;
    }

    return 0;
}

uint8_t egui_view_flip_view_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    uint8_t next_part;

    flip_view_normalize_state(local);
    if (local->compact_mode || local->read_only_mode || local->item_count == 0 || local->items == NULL || !egui_view_get_enable(self))
    {
        return 0;
    }

    switch (key_code)
    {
    case EGUI_KEY_CODE_LEFT:
        if (local->current_part == EGUI_VIEW_FLIP_VIEW_PART_NEXT)
        {
            flip_view_set_current_part_inner(self, EGUI_VIEW_FLIP_VIEW_PART_SURFACE, 1);
        }
        else
        {
            flip_view_change_index(
                    self, -1, local->current_part == EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS ? EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS : EGUI_VIEW_FLIP_VIEW_PART_SURFACE);
        }
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        if (local->current_part == EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS)
        {
            flip_view_set_current_part_inner(self, EGUI_VIEW_FLIP_VIEW_PART_SURFACE, 1);
        }
        else
        {
            flip_view_change_index(self, 1,
                                   local->current_part == EGUI_VIEW_FLIP_VIEW_PART_NEXT ? EGUI_VIEW_FLIP_VIEW_PART_NEXT : EGUI_VIEW_FLIP_VIEW_PART_SURFACE);
        }
        return 1;
    case EGUI_KEY_CODE_HOME:
        local->current_part = EGUI_VIEW_FLIP_VIEW_PART_SURFACE;
        flip_view_set_current_index_inner(self, 0, 1);
        return 1;
    case EGUI_KEY_CODE_END:
        local->current_part = EGUI_VIEW_FLIP_VIEW_PART_SURFACE;
        flip_view_set_current_index_inner(self, (uint8_t)(local->item_count - 1), 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        if (local->current_part == EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS)
        {
            next_part = EGUI_VIEW_FLIP_VIEW_PART_SURFACE;
        }
        else if (local->current_part == EGUI_VIEW_FLIP_VIEW_PART_SURFACE)
        {
            next_part = EGUI_VIEW_FLIP_VIEW_PART_NEXT;
        }
        else
        {
            next_part = EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS;
        }
        flip_view_set_current_part_inner(self, next_part, 1);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (local->current_part == EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS)
        {
            flip_view_change_index(self, -1, EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS);
        }
        else if (local->current_part == EGUI_VIEW_FLIP_VIEW_PART_NEXT)
        {
            flip_view_change_index(self, 1, EGUI_VIEW_FLIP_VIEW_PART_NEXT);
        }
        else
        {
            flip_view_notify(self);
        }
        return 1;
    case EGUI_KEY_CODE_PLUS:
        flip_view_change_index(self, 1, EGUI_VIEW_FLIP_VIEW_PART_SURFACE);
        return 1;
    case EGUI_KEY_CODE_MINUS:
        flip_view_change_index(self, -1, EGUI_VIEW_FLIP_VIEW_PART_SURFACE);
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        flip_view_set_current_part_inner(self, EGUI_VIEW_FLIP_VIEW_PART_SURFACE, 1);
        return 1;
    default:
        return 0;
    }
}
static void flip_view_draw_chevron(egui_view_t *self, const egui_region_t *region, egui_color_t color, uint8_t is_next)
{
    egui_dim_t cx = region->location.x + region->size.width / 2;
    egui_dim_t cy = region->location.y + region->size.height / 2;
    egui_dim_t x1;
    egui_dim_t x2;
    egui_dim_t x3;

    if (is_next)
    {
        x1 = cx - 2;
        x2 = cx + 1;
        x3 = cx - 2;
    }
    else
    {
        x1 = cx + 2;
        x2 = cx - 1;
        x3 = cx + 2;
    }

    egui_canvas_draw_line(x1, cy - 3, x2, cy, 1, color, egui_color_alpha_mix(self->alpha, 92));
    egui_canvas_draw_line(x2, cy, x3, cy + 3, 1, color, egui_color_alpha_mix(self->alpha, 92));
}

static void flip_view_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align, egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (!flip_view_has_text(text))
    {
        return;
    }
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void flip_view_draw_focus(egui_view_t *self, const egui_region_t *region, egui_dim_t radius, egui_color_t color)
{
    flip_view_draw_round_stroke_safe(region->location.x - 1, region->location.y - 1, region->size.width + 2, region->size.height + 2, radius, 1, color,
                                     egui_color_alpha_mix(self->alpha, 74));
}

static void flip_view_draw_counter(egui_view_t *self, const egui_view_flip_view_t *local, const egui_view_flip_view_item_t *item,
                                   const egui_region_t *surface_region, const egui_font_t *meta_font)
{
    egui_region_t region;
    char counter_text[16];
    egui_dim_t pill_w;

    flip_view_format_counter(counter_text, sizeof(counter_text), local->current_index, local->item_count);
    pill_w = (egui_dim_t)(34 + strlen(counter_text) * 4);
    if (pill_w < 44)
    {
        pill_w = 44;
    }
    if (pill_w > surface_region->size.width - 24)
    {
        pill_w = surface_region->size.width - 24;
    }

    region.location.x = surface_region->location.x + surface_region->size.width - pill_w - 11;
    region.location.y = surface_region->location.y + 10;
    region.size.width = pill_w;
    region.size.height = local->compact_mode ? 11 : 12;

    flip_view_draw_round_fill_safe(region.location.x, region.location.y, region.size.width, region.size.height, region.size.height / 2,
                                   egui_rgb_mix(item->accent_color, local->surface_color, 44), egui_color_alpha_mix(self->alpha, 84));
    flip_view_draw_round_stroke_safe(region.location.x, region.location.y, region.size.width, region.size.height, region.size.height / 2, 1,
                                     egui_rgb_mix(item->accent_color, local->border_color, 18), egui_color_alpha_mix(self->alpha, 58));
    flip_view_draw_text(meta_font, self, counter_text, &region, EGUI_ALIGN_CENTER, local->text_color);
}

static void flip_view_draw_surface(egui_view_t *self, egui_view_flip_view_t *local, const egui_view_flip_view_item_t *item,
                                   const egui_view_flip_view_metrics_t *metrics)
{
    egui_region_t text_region;
    egui_color_t shell_color;
    egui_color_t title_color = local->text_color;
    egui_color_t body_color = egui_rgb_mix(local->text_color, local->muted_text_color, 30);
    egui_color_t accent_color = item->accent_color;
    egui_dim_t card_radius = local->compact_mode ? 10 : 12;
    egui_dim_t eyebrow_w;
    egui_dim_t description_y;
    egui_dim_t content_x;
    egui_dim_t content_w;
    const egui_font_t *title_font = local->font;
    const egui_font_t *meta_font = local->meta_font;

    shell_color = egui_rgb_mix(item->surface_color, local->surface_color, local->read_only_mode ? 42 : 18);

    if (local->read_only_mode)
    {
        accent_color = egui_rgb_mix(item->accent_color, local->inactive_color, 46);
        title_color = egui_rgb_mix(local->text_color, local->inactive_color, 28);
        body_color = egui_rgb_mix(local->muted_text_color, local->inactive_color, 20);
    }

    flip_view_draw_round_fill_safe(metrics->surface_region.location.x + FV_STD_CARD_SHADOW_X, metrics->surface_region.location.y + FV_STD_CARD_SHADOW_Y,
                                   metrics->surface_region.size.width, metrics->surface_region.size.height, card_radius,
                                   egui_rgb_mix(local->border_color, EGUI_COLOR_HEX(0x081018), 58), egui_color_alpha_mix(self->alpha, 28));
    flip_view_draw_round_fill_safe(metrics->surface_region.location.x, metrics->surface_region.location.y, metrics->surface_region.size.width,
                                   metrics->surface_region.size.height, card_radius, shell_color, egui_color_alpha_mix(self->alpha, 100));
    flip_view_draw_round_stroke_safe(metrics->surface_region.location.x, metrics->surface_region.location.y, metrics->surface_region.size.width,
                                     metrics->surface_region.size.height, card_radius, 1, egui_rgb_mix(accent_color, local->border_color, 28),
                                     egui_color_alpha_mix(self->alpha, 66));

    eyebrow_w = (egui_dim_t)(24 + strlen(item->eyebrow ? item->eyebrow : "") * 4);
    if (eyebrow_w < 42)
    {
        eyebrow_w = 42;
    }
    if (eyebrow_w > metrics->surface_region.size.width - 56)
    {
        eyebrow_w = metrics->surface_region.size.width - 56;
    }

    text_region.location.x = metrics->surface_region.location.x + 11;
    text_region.location.y = metrics->surface_region.location.y + 10;
    text_region.size.width = eyebrow_w;
    text_region.size.height = local->compact_mode ? 11 : 12;
    flip_view_draw_round_fill_safe(text_region.location.x, text_region.location.y, text_region.size.width, text_region.size.height, text_region.size.height / 2,
                                   accent_color, egui_color_alpha_mix(self->alpha, local->read_only_mode ? 56 : 82));
    flip_view_draw_text(meta_font, self, item->eyebrow, &text_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0xFFFFFF));

    flip_view_draw_counter(self, local, item, &metrics->surface_region, meta_font);

    content_x = metrics->previous_region.location.x + metrics->previous_region.size.width + 8;
    content_w = metrics->next_region.location.x - 8 - content_x;
    if (content_w < 44)
    {
        content_x = metrics->surface_region.location.x + 12;
        content_w = metrics->surface_region.size.width - 24;
    }

    text_region.location.x = content_x;
    text_region.location.y = metrics->surface_region.location.y + (local->compact_mode ? 30 : 34);
    text_region.size.width = content_w;
    text_region.size.height = local->compact_mode ? 14 : 16;
    flip_view_draw_text(title_font, self, item->title, &text_region, EGUI_ALIGN_LEFT, title_color);

    if (!local->compact_mode)
    {
        description_y = text_region.location.y + 21;
        text_region.location.y = description_y;
        text_region.size.height = 24;
        flip_view_draw_text(meta_font, self, item->description, &text_region, EGUI_ALIGN_LEFT, body_color);

        text_region.location.x = content_x;
        text_region.location.y = metrics->surface_region.location.y + metrics->surface_region.size.height - 22;
        text_region.size.width = content_w;
        text_region.size.height = 12;
        flip_view_draw_text(meta_font, self, item->footer, &text_region, EGUI_ALIGN_LEFT, egui_rgb_mix(body_color, accent_color, 18));

        egui_canvas_draw_line(metrics->surface_region.location.x + 12, metrics->surface_region.location.y + metrics->surface_region.size.height - 30,
                              metrics->surface_region.location.x + metrics->surface_region.size.width - 12,
                              metrics->surface_region.location.y + metrics->surface_region.size.height - 30, 1,
                              egui_rgb_mix(accent_color, local->surface_color, 36), egui_color_alpha_mix(self->alpha, 64));
    }
    else
    {
        text_region.location.x = content_x;
        text_region.location.y = metrics->surface_region.location.y + metrics->surface_region.size.height - 18;
        text_region.size.width = content_w;
        text_region.size.height = 10;
        flip_view_draw_text(meta_font, self, item->footer, &text_region, EGUI_ALIGN_LEFT, egui_rgb_mix(body_color, accent_color, 18));
    }
}
static void flip_view_draw_button(egui_view_t *self, egui_view_flip_view_t *local, const egui_region_t *region, const egui_view_flip_view_item_t *item,
                                  uint8_t part, uint8_t is_next)
{
    egui_color_t fill_color;
    egui_color_t border_color;
    egui_color_t icon_color;
    uint8_t enabled = flip_view_part_enabled(local, self, part);

    if (enabled)
    {
        fill_color = egui_rgb_mix(item->accent_color, local->surface_color, local->pressed_part == part ? 34 : 18);
        border_color = egui_rgb_mix(item->accent_color, local->border_color, 20);
        icon_color = local->text_color;
    }
    else
    {
        fill_color = egui_rgb_mix(local->inactive_color, local->surface_color, 28);
        border_color = egui_rgb_mix(local->inactive_color, local->border_color, 16);
        icon_color = egui_rgb_mix(local->inactive_color, local->muted_text_color, 30);
    }

    flip_view_draw_round_fill_safe(region->location.x, region->location.y, region->size.width, region->size.height, region->size.width / 2, fill_color,
                                   egui_color_alpha_mix(self->alpha, 92));
    flip_view_draw_round_stroke_safe(region->location.x, region->location.y, region->size.width, region->size.height, region->size.width / 2, 1, border_color,
                                     egui_color_alpha_mix(self->alpha, 58));
    flip_view_draw_chevron(self, region, icon_color, is_next);
}

static void egui_view_flip_view_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    egui_view_flip_view_metrics_t metrics;
    const egui_view_flip_view_item_t *item;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_text_color = local->muted_text_color;
    egui_dim_t outer_radius = local->compact_mode ? FV_COMPACT_RADIUS : FV_STD_RADIUS;

    item = flip_view_get_item_inner(local);
    flip_view_get_metrics(local, self, &metrics);

    if (local->read_only_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xF0F3F7), 52);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0xA7B1BD), 42);
        text_color = egui_rgb_mix(text_color, EGUI_COLOR_HEX(0x8892A0), 34);
        muted_text_color = egui_rgb_mix(muted_text_color, EGUI_COLOR_HEX(0x93A0AE), 30);
    }

    flip_view_draw_round_fill_safe(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                   self->region_screen.size.height, outer_radius, surface_color, egui_color_alpha_mix(self->alpha, 100));
    flip_view_draw_round_stroke_safe(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                     self->region_screen.size.height, outer_radius, 1, border_color, egui_color_alpha_mix(self->alpha, 58));

    if (item == NULL)
    {
        flip_view_draw_text(local->meta_font, self, "No content", &metrics.surface_region, EGUI_ALIGN_CENTER, muted_text_color);
        return;
    }

    flip_view_draw_text(local->meta_font, self, local->title, &metrics.title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    flip_view_draw_surface(self, local, item, &metrics);
    flip_view_draw_button(self, local, &metrics.previous_region, item, EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS, 0);
    flip_view_draw_button(self, local, &metrics.next_region, item, EGUI_VIEW_FLIP_VIEW_PART_NEXT, 1);
    flip_view_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);

    if (!local->read_only_mode)
    {
        if (local->current_part == EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS)
        {
            flip_view_draw_focus(self, &metrics.previous_region, metrics.previous_region.size.width / 2, item->accent_color);
        }
        else if (local->current_part == EGUI_VIEW_FLIP_VIEW_PART_NEXT)
        {
            flip_view_draw_focus(self, &metrics.next_region, metrics.next_region.size.width / 2, item->accent_color);
        }
        else if (local->current_part == EGUI_VIEW_FLIP_VIEW_PART_SURFACE)
        {
            flip_view_draw_focus(self, &metrics.surface_region, local->compact_mode ? 10 : 12, item->accent_color);
        }
    }
}
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static uint8_t flip_view_hit_part(egui_view_flip_view_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_flip_view_metrics_t metrics;

    flip_view_get_metrics(local, self, &metrics);
    if (egui_region_pt_in_rect(&metrics.previous_region, x, y))
    {
        return EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS;
    }
    if (egui_region_pt_in_rect(&metrics.next_region, x, y))
    {
        return EGUI_VIEW_FLIP_VIEW_PART_NEXT;
    }
    if (egui_region_pt_in_rect(&metrics.surface_region, x, y))
    {
        return EGUI_VIEW_FLIP_VIEW_PART_SURFACE;
    }
    return EGUI_VIEW_FLIP_VIEW_PART_NONE;
}

static int egui_view_flip_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_flip_view_t);
    uint8_t hit_part;

    flip_view_normalize_state(local);
    if (local->compact_mode || local->read_only_mode || local->item_count == 0 || local->items == NULL || !egui_view_get_enable(self))
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = flip_view_hit_part(local, self, event->location.x, event->location.y);
        if (hit_part == EGUI_VIEW_FLIP_VIEW_PART_NONE)
        {
            return 0;
        }
        local->pressed_part = hit_part;
        local->current_part = hit_part;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = flip_view_hit_part(local, self, event->location.x, event->location.y);
        if (hit_part == local->pressed_part)
        {
            if (hit_part == EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS)
            {
                flip_view_change_index(self, -1, EGUI_VIEW_FLIP_VIEW_PART_PREVIOUS);
            }
            else if (hit_part == EGUI_VIEW_FLIP_VIEW_PART_NEXT)
            {
                flip_view_change_index(self, 1, EGUI_VIEW_FLIP_VIEW_PART_NEXT);
            }
            else
            {
                local->current_part = EGUI_VIEW_FLIP_VIEW_PART_SURFACE;
                egui_view_invalidate(self);
                flip_view_notify(self);
            }
        }
        local->pressed_part = EGUI_VIEW_FLIP_VIEW_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_FLIP_VIEW_PART_NONE;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_FLIP_VIEW_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_flip_view_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    if (egui_view_flip_view_handle_navigation_key(self, event->key_code))
    {
        return 1;
    }

    return egui_view_on_key_event(self, event);
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_flip_view_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_flip_view_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_flip_view_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_flip_view_on_key_event,
#endif
};

void egui_view_flip_view_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_flip_view_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_flip_view_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 2);

    local->on_changed = NULL;
    local->items = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->title = NULL;
    local->helper = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD6DEE7);
    local->text_color = EGUI_COLOR_HEX(0x172331);
    local->muted_text_color = EGUI_COLOR_HEX(0x6E7C8B);
    local->inactive_color = EGUI_COLOR_HEX(0xA6B0BD);
    local->item_count = 0;
    local->current_index = 0;
    local->current_part = EGUI_VIEW_FLIP_VIEW_PART_SURFACE;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->pressed_part = EGUI_VIEW_FLIP_VIEW_PART_NONE;

    egui_view_set_view_name(self, "egui_view_flip_view");
}
