#include <stdlib.h>

#include "egui_view_nav_panel.h"

#define EGUI_VIEW_NAV_PANEL_STANDARD_RADIUS             8
#define EGUI_VIEW_NAV_PANEL_STANDARD_FILL_ALPHA         94
#define EGUI_VIEW_NAV_PANEL_STANDARD_BORDER_ALPHA       66
#define EGUI_VIEW_NAV_PANEL_STANDARD_PAD_X              10
#define EGUI_VIEW_NAV_PANEL_STANDARD_PAD_Y              8
#define EGUI_VIEW_NAV_PANEL_STANDARD_HEADER_HEIGHT      10
#define EGUI_VIEW_NAV_PANEL_STANDARD_HEADER_GAP         5
#define EGUI_VIEW_NAV_PANEL_STANDARD_ROW_GAP            4
#define EGUI_VIEW_NAV_PANEL_STANDARD_FOOTER_GAP         5
#define EGUI_VIEW_NAV_PANEL_STANDARD_FOOTER_HEIGHT      15
#define EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_WIDTH        18
#define EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_HEIGHT       14
#define EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_RADIUS       4
#define EGUI_VIEW_NAV_PANEL_STANDARD_ROW_RADIUS         6
#define EGUI_VIEW_NAV_PANEL_STANDARD_INDICATOR_WIDTH    3
#define EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_FILL_ALPHA   32
#define EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_BORDER_ALPHA 40

#define EGUI_VIEW_NAV_PANEL_COMPACT_RADIUS             7
#define EGUI_VIEW_NAV_PANEL_COMPACT_FILL_ALPHA         90
#define EGUI_VIEW_NAV_PANEL_COMPACT_BORDER_ALPHA       62
#define EGUI_VIEW_NAV_PANEL_COMPACT_PAD_X              7
#define EGUI_VIEW_NAV_PANEL_COMPACT_PAD_Y              6
#define EGUI_VIEW_NAV_PANEL_COMPACT_ROW_GAP            4
#define EGUI_VIEW_NAV_PANEL_COMPACT_FOOTER_GAP         4
#define EGUI_VIEW_NAV_PANEL_COMPACT_FOOTER_HEIGHT      13
#define EGUI_VIEW_NAV_PANEL_COMPACT_ROW_RADIUS         5
#define EGUI_VIEW_NAV_PANEL_COMPACT_INDICATOR_WIDTH    2
#define EGUI_VIEW_NAV_PANEL_COMPACT_BADGE_FILL_ALPHA   36
#define EGUI_VIEW_NAV_PANEL_COMPACT_BADGE_BORDER_ALPHA 44

#define EGUI_VIEW_NAV_PANEL_INDEX_NONE 0xFF

typedef struct egui_view_nav_panel_metrics egui_view_nav_panel_metrics_t;
struct egui_view_nav_panel_metrics
{
    egui_region_t content;
    egui_region_t header_region;
    egui_region_t item_regions[EGUI_VIEW_NAV_PANEL_MAX_ITEMS];
    egui_region_t footer_region;
    uint8_t visible_item_count;
    uint8_t show_header;
    uint8_t show_footer;
};

static egui_color_t egui_view_nav_panel_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static uint8_t egui_view_nav_panel_get_visible_item_count(egui_view_nav_panel_t *local)
{
    return local->item_count > EGUI_VIEW_NAV_PANEL_MAX_ITEMS ? EGUI_VIEW_NAV_PANEL_MAX_ITEMS : local->item_count;
}

static const egui_view_nav_panel_item_t *egui_view_nav_panel_get_item(egui_view_nav_panel_t *local, uint8_t index)
{
    if (local->items == NULL || index >= egui_view_nav_panel_get_visible_item_count(local))
    {
        return NULL;
    }
    return &local->items[index];
}

static const char *egui_view_nav_panel_get_badge_text(const egui_view_nav_panel_item_t *item)
{
    if (item == NULL)
    {
        return "";
    }
    if (item->badge != NULL && item->badge[0] != '\0')
    {
        return item->badge;
    }
    if (item->title != NULL && item->title[0] != '\0')
    {
        return item->title;
    }
    return "";
}

static void egui_view_nav_panel_get_metrics(egui_view_nav_panel_t *local, egui_view_t *self, egui_view_nav_panel_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_NAV_PANEL_COMPACT_PAD_X : EGUI_VIEW_NAV_PANEL_STANDARD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_NAV_PANEL_COMPACT_PAD_Y : EGUI_VIEW_NAV_PANEL_STANDARD_PAD_Y;
    egui_dim_t footer_h = local->compact_mode ? EGUI_VIEW_NAV_PANEL_COMPACT_FOOTER_HEIGHT : EGUI_VIEW_NAV_PANEL_STANDARD_FOOTER_HEIGHT;
    egui_dim_t footer_gap = local->compact_mode ? EGUI_VIEW_NAV_PANEL_COMPACT_FOOTER_GAP : EGUI_VIEW_NAV_PANEL_STANDARD_FOOTER_GAP;
    egui_dim_t header_h = local->compact_mode ? 0 : EGUI_VIEW_NAV_PANEL_STANDARD_HEADER_HEIGHT;
    egui_dim_t header_gap = local->compact_mode ? 0 : EGUI_VIEW_NAV_PANEL_STANDARD_HEADER_GAP;
    egui_dim_t row_gap = local->compact_mode ? EGUI_VIEW_NAV_PANEL_COMPACT_ROW_GAP : EGUI_VIEW_NAV_PANEL_STANDARD_ROW_GAP;
    egui_dim_t rows_area_height;
    egui_dim_t row_height;
    egui_dim_t item_y;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    metrics->content.location.x = region.location.x + pad_x;
    metrics->content.location.y = region.location.y + pad_y;
    metrics->content.size.width = region.size.width - pad_x * 2;
    metrics->content.size.height = region.size.height - pad_y * 2;
    metrics->visible_item_count = egui_view_nav_panel_get_visible_item_count(local);
    metrics->show_header = local->compact_mode ? 0 : ((local->header_text != NULL && local->header_text[0] != '\0') ? 1 : 0);
    metrics->show_footer = (local->footer_badge != NULL && local->footer_badge[0] != '\0') ||
                           (!local->compact_mode && local->footer_text != NULL && local->footer_text[0] != '\0');

    metrics->header_region.location.x = metrics->content.location.x;
    metrics->header_region.location.y = metrics->content.location.y;
    metrics->header_region.size.width = metrics->content.size.width;
    metrics->header_region.size.height = header_h;

    metrics->footer_region.location.x = metrics->content.location.x;
    metrics->footer_region.location.y = metrics->content.location.y + metrics->content.size.height - footer_h;
    metrics->footer_region.size.width = metrics->content.size.width;
    metrics->footer_region.size.height = metrics->show_footer ? footer_h : 0;

    if (metrics->visible_item_count == 0)
    {
        return;
    }

    item_y = metrics->content.location.y + (metrics->show_header ? (header_h + header_gap) : 0);
    rows_area_height = metrics->content.size.height - (metrics->show_header ? (header_h + header_gap) : 0) -
                       (metrics->show_footer ? (footer_h + footer_gap) : 0) - row_gap * (metrics->visible_item_count - 1);

    if (rows_area_height < (egui_dim_t)metrics->visible_item_count * 10)
    {
        rows_area_height = (egui_dim_t)metrics->visible_item_count * 10;
    }
    row_height = rows_area_height / metrics->visible_item_count;

    for (i = 0; i < metrics->visible_item_count; ++i)
    {
        metrics->item_regions[i].location.x = metrics->content.location.x;
        metrics->item_regions[i].location.y = item_y;
        metrics->item_regions[i].size.width = metrics->content.size.width;
        metrics->item_regions[i].size.height = row_height;
        item_y += row_height + row_gap;
    }
}

static void egui_view_nav_panel_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                          egui_color_t color)
{
    egui_region_t draw_region = *region;
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void egui_view_nav_panel_set_current_index_inner(egui_view_t *self, uint8_t index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    uint8_t visible_item_count = egui_view_nav_panel_get_visible_item_count(local);

    if (visible_item_count == 0)
    {
        local->current_index = 0;
        return;
    }
    if (index >= visible_item_count)
    {
        index = visible_item_count - 1;
    }
    if (local->current_index == index)
    {
        return;
    }

    local->current_index = index;
    if (notify && local->on_selection_changed != NULL)
    {
        local->on_selection_changed(self, index);
    }
    egui_view_invalidate(self);
}

void egui_view_nav_panel_set_items(egui_view_t *self, const egui_view_nav_panel_item_t *items, uint8_t item_count)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    local->items = items;
    local->item_count = item_count;
    if (local->current_index >= egui_view_nav_panel_get_visible_item_count(local))
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_nav_panel_set_current_index(egui_view_t *self, uint8_t index)
{
    egui_view_nav_panel_set_current_index_inner(self, index, 1);
}

uint8_t egui_view_nav_panel_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    return local->current_index;
}

void egui_view_nav_panel_set_header_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    local->header_text = text;
    egui_view_invalidate(self);
}

void egui_view_nav_panel_set_footer_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    local->footer_text = text;
    egui_view_invalidate(self);
}

void egui_view_nav_panel_set_footer_badge(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    local->footer_badge = text;
    egui_view_invalidate(self);
}

void egui_view_nav_panel_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_nav_panel_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_nav_panel_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_nav_panel_selection_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    local->on_selection_changed = listener;
}

void egui_view_nav_panel_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_nav_panel_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    local->locked_mode = locked_mode ? 1 : 0;
    local->pressed_index = EGUI_VIEW_NAV_PANEL_INDEX_NONE;
    egui_view_invalidate(self);
}

void egui_view_nav_panel_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                     egui_color_t muted_text_color, egui_color_t accent_color)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    egui_view_invalidate(self);
}

static void egui_view_nav_panel_draw_standard_item(egui_view_t *self, egui_view_nav_panel_t *local, const egui_region_t *item_region,
                                                   const egui_view_nav_panel_item_t *item, uint8_t index, egui_color_t border_color, egui_color_t text_color,
                                                   egui_color_t muted_text_color, egui_color_t accent_color)
{
    egui_region_t badge_region;
    egui_region_t text_region = *item_region;
    egui_color_t badge_fill;
    egui_color_t badge_border;
    egui_color_t badge_text;
    egui_color_t row_fill;
    uint8_t is_selected = index == local->current_index;
    uint8_t is_pressed = index == local->pressed_index;

    row_fill = egui_rgb_mix(local->surface_color, accent_color, is_selected ? 16 : (is_pressed ? 8 : 0));
    if (is_selected || is_pressed)
    {
        egui_canvas_draw_round_rectangle_fill(item_region->location.x, item_region->location.y, item_region->size.width, item_region->size.height,
                                              EGUI_VIEW_NAV_PANEL_STANDARD_ROW_RADIUS, row_fill, egui_color_alpha_mix(self->alpha, is_selected ? 92 : 54));
    }

    if (is_selected)
    {
        egui_canvas_draw_round_rectangle_fill(item_region->location.x, item_region->location.y + 2, EGUI_VIEW_NAV_PANEL_STANDARD_INDICATOR_WIDTH,
                                              item_region->size.height - 4, 2, accent_color, egui_color_alpha_mix(self->alpha, 100));
    }

    badge_region.location.x = item_region->location.x + 8;
    badge_region.location.y = item_region->location.y + (item_region->size.height - EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_HEIGHT) / 2;
    badge_region.size.width = EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_WIDTH;
    badge_region.size.height = EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_HEIGHT;

    badge_fill = egui_rgb_mix(local->surface_color, accent_color, is_selected ? 22 : 10);
    badge_border = egui_rgb_mix(border_color, accent_color, is_selected ? 18 : 8);
    badge_text = is_selected ? accent_color : text_color;
    egui_canvas_draw_round_rectangle_fill(badge_region.location.x, badge_region.location.y, badge_region.size.width, badge_region.size.height,
                                          EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_RADIUS, badge_fill,
                                          egui_color_alpha_mix(self->alpha, EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(badge_region.location.x, badge_region.location.y, badge_region.size.width, badge_region.size.height,
                                     EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_RADIUS, 1, badge_border,
                                     egui_color_alpha_mix(self->alpha, EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_BORDER_ALPHA));
    egui_view_nav_panel_draw_text(local->meta_font, self, egui_view_nav_panel_get_badge_text(item), &badge_region, EGUI_ALIGN_CENTER, badge_text);

    text_region.location.x = badge_region.location.x + badge_region.size.width + 8;
    text_region.size.width = item_region->location.x + item_region->size.width - text_region.location.x - 6;
    egui_view_nav_panel_draw_text(local->font, self, item->title, &text_region, EGUI_ALIGN_LEFT, is_selected ? text_color : muted_text_color);
}

static void egui_view_nav_panel_draw_compact_item(egui_view_t *self, egui_view_nav_panel_t *local, const egui_region_t *item_region,
                                                  const egui_view_nav_panel_item_t *item, uint8_t index, egui_color_t border_color, egui_color_t text_color,
                                                  egui_color_t accent_color)
{
    uint8_t is_selected = index == local->current_index;
    uint8_t is_pressed = index == local->pressed_index;
    egui_color_t row_fill = egui_rgb_mix(local->surface_color, accent_color, is_selected ? 16 : (is_pressed ? 8 : 0));
    egui_color_t row_border = egui_rgb_mix(border_color, accent_color, is_selected ? 20 : 8);

    egui_canvas_draw_round_rectangle_fill(item_region->location.x, item_region->location.y, item_region->size.width, item_region->size.height,
                                          EGUI_VIEW_NAV_PANEL_COMPACT_ROW_RADIUS, row_fill,
                                          egui_color_alpha_mix(self->alpha, EGUI_VIEW_NAV_PANEL_COMPACT_BADGE_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(item_region->location.x, item_region->location.y, item_region->size.width, item_region->size.height,
                                     EGUI_VIEW_NAV_PANEL_COMPACT_ROW_RADIUS, 1, row_border,
                                     egui_color_alpha_mix(self->alpha, EGUI_VIEW_NAV_PANEL_COMPACT_BADGE_BORDER_ALPHA));
    if (is_selected)
    {
        egui_canvas_draw_round_rectangle_fill(item_region->location.x, item_region->location.y + 2, EGUI_VIEW_NAV_PANEL_COMPACT_INDICATOR_WIDTH,
                                              item_region->size.height - 4, 2, accent_color, egui_color_alpha_mix(self->alpha, 100));
    }
    egui_view_nav_panel_draw_text(local->meta_font, self, egui_view_nav_panel_get_badge_text(item), item_region, EGUI_ALIGN_CENTER,
                                  is_selected ? accent_color : text_color);
}

static void egui_view_nav_panel_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    egui_region_t region;
    egui_view_nav_panel_metrics_t metrics;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_text_color = local->muted_text_color;
    egui_color_t accent_color = local->accent_color;
    egui_dim_t radius = local->compact_mode ? EGUI_VIEW_NAV_PANEL_COMPACT_RADIUS : EGUI_VIEW_NAV_PANEL_STANDARD_RADIUS;
    uint8_t is_enabled = egui_view_get_enable(self) ? 1 : 0;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    egui_view_nav_panel_get_metrics(local, self, &metrics);

    if (local->locked_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFBFCFD), 24);
        border_color = egui_rgb_mix(border_color, muted_text_color, 18);
        text_color = egui_rgb_mix(text_color, muted_text_color, 56);
        accent_color = egui_rgb_mix(accent_color, muted_text_color, 76);
    }

    if (!is_enabled)
    {
        surface_color = egui_view_nav_panel_mix_disabled(surface_color);
        border_color = egui_view_nav_panel_mix_disabled(border_color);
        text_color = egui_view_nav_panel_mix_disabled(text_color);
        muted_text_color = egui_view_nav_panel_mix_disabled(muted_text_color);
        accent_color = egui_view_nav_panel_mix_disabled(accent_color);
    }

    egui_canvas_draw_round_rectangle_fill(
            region.location.x, region.location.y, region.size.width, region.size.height, radius, surface_color,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_NAV_PANEL_COMPACT_FILL_ALPHA : EGUI_VIEW_NAV_PANEL_STANDARD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(
            region.location.x, region.location.y, region.size.width, region.size.height, radius, 1, border_color,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_NAV_PANEL_COMPACT_BORDER_ALPHA : EGUI_VIEW_NAV_PANEL_STANDARD_BORDER_ALPHA));

    if (metrics.show_header)
    {
        egui_view_nav_panel_draw_text(local->meta_font, self, local->header_text, &metrics.header_region, EGUI_ALIGN_LEFT, muted_text_color);
    }

    for (i = 0; i < metrics.visible_item_count; ++i)
    {
        const egui_view_nav_panel_item_t *item = egui_view_nav_panel_get_item(local, i);

        if (item == NULL)
        {
            continue;
        }

        if (local->compact_mode)
        {
            egui_view_nav_panel_draw_compact_item(self, local, &metrics.item_regions[i], item, i, border_color, text_color, accent_color);
        }
        else
        {
            egui_view_nav_panel_draw_standard_item(self, local, &metrics.item_regions[i], item, i, border_color, text_color, muted_text_color, accent_color);
        }
    }

    if (metrics.show_footer)
    {
        if (!local->compact_mode)
        {
            egui_canvas_draw_line(metrics.footer_region.location.x, metrics.footer_region.location.y - 2,
                                  metrics.footer_region.location.x + metrics.footer_region.size.width, metrics.footer_region.location.y - 2, 1, border_color,
                                  egui_color_alpha_mix(self->alpha, 36));

            if (local->footer_badge != NULL && local->footer_badge[0] != '\0')
            {
                egui_region_t badge_region = metrics.footer_region;

                badge_region.size.width = EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_WIDTH;
                badge_region.size.height = EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_HEIGHT;
                badge_region.location.y += (metrics.footer_region.size.height - badge_region.size.height) / 2;
                egui_canvas_draw_round_rectangle_fill(badge_region.location.x, badge_region.location.y, badge_region.size.width, badge_region.size.height,
                                                      EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_RADIUS, egui_rgb_mix(surface_color, accent_color, 8),
                                                      egui_color_alpha_mix(self->alpha, EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_FILL_ALPHA));
                egui_canvas_draw_round_rectangle(badge_region.location.x, badge_region.location.y, badge_region.size.width, badge_region.size.height,
                                                 EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_RADIUS, 1, border_color,
                                                 egui_color_alpha_mix(self->alpha, EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_BORDER_ALPHA));
                egui_view_nav_panel_draw_text(local->meta_font, self, local->footer_badge, &badge_region, EGUI_ALIGN_CENTER, muted_text_color);
            }

            if (local->footer_text != NULL)
            {
                egui_region_t text_region = metrics.footer_region;

                text_region.location.x += EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_WIDTH + 8;
                text_region.size.width -= EGUI_VIEW_NAV_PANEL_STANDARD_BADGE_WIDTH + 8;
                egui_view_nav_panel_draw_text(local->meta_font, self, local->footer_text, &text_region, EGUI_ALIGN_LEFT, muted_text_color);
            }
        }
        else if (local->footer_badge != NULL && local->footer_badge[0] != '\0')
        {
            egui_canvas_draw_round_rectangle_fill(metrics.footer_region.location.x, metrics.footer_region.location.y, metrics.footer_region.size.width,
                                                  metrics.footer_region.size.height, EGUI_VIEW_NAV_PANEL_COMPACT_ROW_RADIUS,
                                                  egui_rgb_mix(surface_color, accent_color, 8),
                                                  egui_color_alpha_mix(self->alpha, EGUI_VIEW_NAV_PANEL_COMPACT_BADGE_FILL_ALPHA));
            egui_canvas_draw_round_rectangle(metrics.footer_region.location.x, metrics.footer_region.location.y, metrics.footer_region.size.width,
                                             metrics.footer_region.size.height, EGUI_VIEW_NAV_PANEL_COMPACT_ROW_RADIUS, 1, border_color,
                                             egui_color_alpha_mix(self->alpha, EGUI_VIEW_NAV_PANEL_COMPACT_BADGE_BORDER_ALPHA));
            egui_view_nav_panel_draw_text(local->meta_font, self, local->footer_badge, &metrics.footer_region, EGUI_ALIGN_CENTER, muted_text_color);
        }
    }
}

static uint8_t egui_view_nav_panel_hit_item(egui_view_nav_panel_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_nav_panel_metrics_t metrics;
    uint8_t i;

    egui_view_nav_panel_get_metrics(local, self, &metrics);
    for (i = 0; i < metrics.visible_item_count; ++i)
    {
        if (egui_region_pt_in_rect(&metrics.item_regions[i], x, y))
        {
            return i;
        }
    }
    return EGUI_VIEW_NAV_PANEL_INDEX_NONE;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_nav_panel_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_nav_panel_t);
    uint8_t hit_index;

    if (!egui_view_get_enable(self) || local->locked_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_index = egui_view_nav_panel_hit_item(local, self, event->location.x, event->location.y);
        if (hit_index == EGUI_VIEW_NAV_PANEL_INDEX_NONE)
        {
            return 0;
        }
        local->pressed_index = hit_index;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_index = egui_view_nav_panel_hit_item(local, self, event->location.x, event->location.y);
        if (local->pressed_index != EGUI_VIEW_NAV_PANEL_INDEX_NONE && local->pressed_index == hit_index)
        {
            egui_view_nav_panel_set_current_index_inner(self, hit_index, 1);
        }
        local->pressed_index = EGUI_VIEW_NAV_PANEL_INDEX_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_index != EGUI_VIEW_NAV_PANEL_INDEX_NONE;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_index = EGUI_VIEW_NAV_PANEL_INDEX_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_nav_panel_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_nav_panel_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_nav_panel_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_nav_panel_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_nav_panel_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_nav_panel_t);
    egui_view_set_padding_all(self, 2);

    local->on_selection_changed = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->items = NULL;
    local->header_text = NULL;
    local->footer_text = NULL;
    local->footer_badge = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD5DDE6);
    local->text_color = EGUI_COLOR_HEX(0x202B37);
    local->muted_text_color = EGUI_COLOR_HEX(0x667688);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->item_count = 0;
    local->current_index = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
    local->pressed_index = EGUI_VIEW_NAV_PANEL_INDEX_NONE;

    egui_view_set_view_name(self, "egui_view_nav_panel");
}
