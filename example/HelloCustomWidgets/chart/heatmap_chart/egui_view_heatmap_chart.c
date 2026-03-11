#define HEATMAP_PANEL_SHADOW_ALPHA      20
#define HEATMAP_PANEL_FILL_ALPHA        32
#define HEATMAP_PANEL_BORDER_ALPHA      52
#define HEATMAP_INNER_BORDER_ALPHA      20
#define HEATMAP_HEADER_MODE             2
#define HEATMAP_HEADER_PILL_MIN_WIDTH   82
#define HEATMAP_HEADER_LINE_ALPHA       22
#define HEATMAP_FOOTER_MODE             2
#define HEATMAP_FOOTER_WIDTH            80
#define HEATMAP_FOOTER_FILL_ALPHA       46
#define HEATMAP_FOOTER_BORDER_ALPHA     22
#define HEATMAP_MINI_BADGE_MODE         2
#define HEATMAP_MINI_BADGE_WIDTH        30
#define HEATMAP_MINI_BADGE_FILL_ALPHA   70
#define HEATMAP_MINI_BADGE_BORDER_ALPHA 20
#define HEATMAP_MINI_TOP_STRIP_ALPHA    18
#define HEATMAP_MINI_BOTTOM_STRIP_ALPHA 18
#define HEATMAP_DISABLED_OVERLAY_ALPHA  14
#define HEATMAP_DISABLED_CROSS_ALPHA    12
#define HEATMAP_DISABLED_MIX_ALPHA      70

#include <stdlib.h>
#include <string.h>

#include "egui_view_heatmap_chart.h"

static uint8_t egui_view_heatmap_chart_clamp_rows(uint8_t rows)
{
    if (rows == 0)
    {
        return 1;
    }
    if (rows > EGUI_VIEW_HEATMAP_CHART_MAX_ROWS)
    {
        return EGUI_VIEW_HEATMAP_CHART_MAX_ROWS;
    }
    return rows;
}

static uint8_t egui_view_heatmap_chart_clamp_cols(uint8_t cols)
{
    if (cols == 0)
    {
        return 1;
    }
    if (cols > EGUI_VIEW_HEATMAP_CHART_MAX_COLS)
    {
        return EGUI_VIEW_HEATMAP_CHART_MAX_COLS;
    }
    return cols;
}

static uint8_t egui_view_heatmap_chart_clamp_value_set_count(uint8_t count)
{
    if (count > EGUI_VIEW_HEATMAP_CHART_MAX_VALUE_SETS)
    {
        return EGUI_VIEW_HEATMAP_CHART_MAX_VALUE_SETS;
    }
    return count;
}

static egui_color_t egui_view_heatmap_chart_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, HEATMAP_DISABLED_MIX_ALPHA);
}

static const char *egui_view_heatmap_chart_get_header_text(egui_view_heatmap_chart_t *local, uint8_t is_enabled)
{
    if (!is_enabled)
    {
        return "Locked";
    }
    return (local->current_value_set == 0) ? "Matrix A" : ((local->current_value_set == 1) ? "Matrix B" : "Matrix C");
}

static const char *egui_view_heatmap_chart_get_footer_text(egui_view_heatmap_chart_t *local)
{
    return (local->current_value_set == 0) ? "Zone A" : ((local->current_value_set == 1) ? "Zone B" : "Zone C");
}

static egui_alpha_t egui_view_heatmap_chart_value_to_alpha(uint8_t value)
{
    uint16_t alpha = 26U + ((uint16_t)value * (255U - 26U)) / 100U;

    if (value >= 80)
    {
        alpha += 18U;
    }
    if (alpha > 255U)
    {
        alpha = 255U;
    }
    return (egui_alpha_t)alpha;
}

static void egui_view_heatmap_chart_format_value(uint8_t value, char out_text[4])
{
    if (value > 99)
    {
        out_text[0] = '1';
        out_text[1] = '0';
        out_text[2] = '0';
        out_text[3] = 0;
        return;
    }
    if (value > 9)
    {
        out_text[0] = (char)('0' + (value / 10));
        out_text[1] = (char)('0' + (value % 10));
        out_text[2] = 0;
        return;
    }
    out_text[0] = (char)('0' + value);
    out_text[1] = 0;
}

static void egui_view_heatmap_chart_draw_cell(egui_view_heatmap_chart_t *local, egui_region_t cell_region, uint8_t value, uint8_t is_hotspot,
                                              egui_alpha_t alpha, uint8_t is_enabled)
{
    egui_color_t cell_color;
    egui_color_t border_color;
    egui_color_t value_text_color;
    egui_region_t text_region;
    char value_text[4] = {0};

    if (cell_region.size.width <= 2 || cell_region.size.height <= 2)
    {
        return;
    }

    cell_color = egui_rgb_mix(local->cold_color, local->hot_color, egui_view_heatmap_chart_value_to_alpha(value));
    cell_color = egui_rgb_mix(local->surface_color, cell_color, EGUI_ALPHA_80);
    if (!is_enabled)
    {
        cell_color = egui_view_heatmap_chart_mix_disabled(cell_color);
    }
    value_text_color = local->text_color;
    if (is_enabled && value >= 62)
    {
        value_text_color = EGUI_COLOR_HEX(0x0F172A);
    }
    border_color = local->border_color;
    if (is_hotspot && is_enabled)
    {
        border_color = egui_rgb_mix(local->border_color, local->hot_color, EGUI_ALPHA_50);
    }

    egui_canvas_draw_round_rectangle_fill(cell_region.location.x + 1, cell_region.location.y + 1, cell_region.size.width - 2, cell_region.size.height - 2, 3,
                                          cell_color, alpha);
    egui_canvas_draw_rectangle(cell_region.location.x, cell_region.location.y, cell_region.size.width, cell_region.size.height, is_hotspot ? 2 : 1,
                               border_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_40));
    if (is_hotspot && is_enabled && cell_region.size.width > 10 && cell_region.size.height > 10)
    {
        egui_canvas_draw_round_rectangle(cell_region.location.x + 2, cell_region.location.y + 2, cell_region.size.width - 4, cell_region.size.height - 4, 2, 1,
                                         local->hot_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_60));
        egui_canvas_draw_circle_fill(cell_region.location.x + cell_region.size.width - 4, cell_region.location.y + 4, 2, local->hot_color,
                                     egui_color_alpha_mix(alpha, EGUI_ALPHA_80));
    }

    if (!local->show_values || cell_region.size.width < 20 || cell_region.size.height < 14)
    {
        return;
    }

    egui_view_heatmap_chart_format_value(value, value_text);
    text_region.location.x = cell_region.location.x + 1;
    text_region.location.y = cell_region.location.y + 1;
    text_region.size.width = cell_region.size.width - 2;
    text_region.size.height = cell_region.size.height - 2;
    egui_canvas_draw_text_in_rect(local->font, value_text, &text_region, EGUI_ALIGN_CENTER, value_text_color, alpha);
}

static void egui_view_heatmap_chart_draw_grid_overlay(egui_view_heatmap_chart_t *local, egui_region_t content_region, egui_alpha_t alpha, uint8_t is_enabled)
{
    egui_color_t line_color;
    uint8_t r;
    uint8_t c;

    if (local->rows < 2 && local->cols < 2)
    {
        return;
    }
    line_color = is_enabled ? egui_rgb_mix(local->border_color, local->text_color, EGUI_ALPHA_30)
                            : egui_rgb_mix(local->border_color, local->muted_text_color, EGUI_ALPHA_20);

    for (r = 1; r < local->rows; r++)
    {
        egui_dim_t y = content_region.location.y + (egui_dim_t)(((int32_t)content_region.size.height * r) / local->rows);
        egui_canvas_draw_line(content_region.location.x + 1, y, content_region.location.x + content_region.size.width - 2, y, 1, line_color,
                              egui_color_alpha_mix(alpha, EGUI_ALPHA_30));
    }
    for (c = 1; c < local->cols; c++)
    {
        egui_dim_t x = content_region.location.x + (egui_dim_t)(((int32_t)content_region.size.width * c) / local->cols);
        egui_canvas_draw_line(x, content_region.location.y + 1, x, content_region.location.y + content_region.size.height - 2, 1, line_color,
                              egui_color_alpha_mix(alpha, EGUI_ALPHA_30));
    }
}

static void egui_view_heatmap_chart_draw_hotspot_guides(egui_view_heatmap_chart_t *local, egui_region_t content_region, uint8_t max_index, egui_alpha_t alpha,
                                                        uint8_t is_enabled)
{
    egui_color_t guide_color;
    uint8_t max_row;
    uint8_t max_col;
    egui_dim_t y0;
    egui_dim_t y1;
    egui_dim_t x0;
    egui_dim_t x1;
    egui_dim_t cy;
    egui_dim_t cx;

    if (!is_enabled || local->cols == 0)
    {
        return;
    }
    max_row = (uint8_t)(max_index / local->cols);
    max_col = (uint8_t)(max_index % local->cols);

    y0 = content_region.location.y + (egui_dim_t)(((int32_t)content_region.size.height * max_row) / local->rows);
    y1 = content_region.location.y + (egui_dim_t)(((int32_t)content_region.size.height * (max_row + 1)) / local->rows);
    x0 = content_region.location.x + (egui_dim_t)(((int32_t)content_region.size.width * max_col) / local->cols);
    x1 = content_region.location.x + (egui_dim_t)(((int32_t)content_region.size.width * (max_col + 1)) / local->cols);
    cy = (egui_dim_t)((y0 + y1) / 2);
    cx = (egui_dim_t)((x0 + x1) / 2);

    guide_color = egui_rgb_mix(local->hot_color, local->border_color, EGUI_ALPHA_50);
    egui_canvas_draw_line(content_region.location.x + 1, cy, content_region.location.x + content_region.size.width - 2, cy, 1, guide_color,
                          egui_color_alpha_mix(alpha, EGUI_ALPHA_30));
    egui_canvas_draw_line(cx, content_region.location.y + 1, cx, content_region.location.y + content_region.size.height - 2, 1, guide_color,
                          egui_color_alpha_mix(alpha, EGUI_ALPHA_30));
}

static uint8_t egui_view_heatmap_chart_find_max_index(const uint8_t *values, uint8_t count)
{
    uint8_t max_index = 0;
    uint8_t max_value = 0;
    uint8_t i;

    if (values == NULL || count == 0)
    {
        return 0;
    }
    max_value = values[0];
    for (i = 1; i < count; i++)
    {
        if (values[i] > max_value)
        {
            max_value = values[i];
            max_index = i;
        }
    }
    return max_index;
}

static void egui_view_heatmap_chart_draw_axis_labels(egui_view_heatmap_chart_t *local, egui_region_t content_region, egui_dim_t axis_left_width,
                                                     egui_dim_t axis_top_height, egui_alpha_t alpha)
{
    egui_region_t text_region;
    egui_color_t chip_color = egui_rgb_mix(local->surface_color, EGUI_COLOR_BLACK, EGUI_ALPHA_50);
    uint8_t r;
    uint8_t c;

    if (!local->show_axis_labels)
    {
        return;
    }
    if (local->row_labels != NULL && axis_left_width > 0)
    {
        for (r = 0; r < local->rows; r++)
        {
            egui_dim_t y0 = content_region.location.y + (egui_dim_t)(((int32_t)content_region.size.height * r) / local->rows);
            egui_dim_t y1 = content_region.location.y + (egui_dim_t)(((int32_t)content_region.size.height * (r + 1)) / local->rows);
            text_region.location.x = content_region.location.x - axis_left_width;
            text_region.location.y = y0;
            text_region.size.width = axis_left_width - 2;
            text_region.size.height = y1 - y0;
            if (text_region.size.width > 5 && text_region.size.height > 4)
            {
                egui_canvas_draw_round_rectangle_fill(text_region.location.x + 1, text_region.location.y + 1, text_region.size.width - 1,
                                                      text_region.size.height - 2, 2, chip_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_40));
            }
            egui_canvas_draw_text_in_rect(local->font, local->row_labels[r], &text_region, EGUI_ALIGN_RIGHT, local->muted_text_color, alpha);
        }
    }
    if (local->col_labels != NULL && axis_top_height > 0)
    {
        for (c = 0; c < local->cols; c++)
        {
            egui_dim_t x0 = content_region.location.x + (egui_dim_t)(((int32_t)content_region.size.width * c) / local->cols);
            egui_dim_t x1 = content_region.location.x + (egui_dim_t)(((int32_t)content_region.size.width * (c + 1)) / local->cols);
            text_region.location.x = x0;
            text_region.location.y = content_region.location.y - axis_top_height;
            text_region.size.width = x1 - x0;
            text_region.size.height = axis_top_height - 1;
            if (text_region.size.width > 7 && text_region.size.height > 4)
            {
                egui_canvas_draw_round_rectangle_fill(text_region.location.x + 1, text_region.location.y, text_region.size.width - 2, text_region.size.height,
                                                      2, chip_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_40));
            }
            egui_canvas_draw_text_in_rect(local->font, local->col_labels[c], &text_region, EGUI_ALIGN_CENTER, local->muted_text_color, alpha);
        }
    }
}

void egui_view_heatmap_chart_set_shape(egui_view_t *self, uint8_t rows, uint8_t cols)
{
    EGUI_LOCAL_INIT(egui_view_heatmap_chart_t);
    local->rows = egui_view_heatmap_chart_clamp_rows(rows);
    local->cols = egui_view_heatmap_chart_clamp_cols(cols);
    egui_view_invalidate(self);
}

void egui_view_heatmap_chart_set_axis_labels(egui_view_t *self, const char **row_labels, const char **col_labels)
{
    EGUI_LOCAL_INIT(egui_view_heatmap_chart_t);
    local->row_labels = row_labels;
    local->col_labels = col_labels;
    egui_view_invalidate(self);
}

void egui_view_heatmap_chart_set_value_set(egui_view_t *self, uint8_t set_index, const uint8_t *values)
{
    EGUI_LOCAL_INIT(egui_view_heatmap_chart_t);
    if (set_index >= EGUI_VIEW_HEATMAP_CHART_MAX_VALUE_SETS)
    {
        return;
    }
    local->value_sets[set_index] = values;
    if (local->value_set_count <= set_index)
    {
        local->value_set_count = set_index + 1;
    }
    local->value_set_count = egui_view_heatmap_chart_clamp_value_set_count(local->value_set_count);
    egui_view_invalidate(self);
}

void egui_view_heatmap_chart_set_current_value_set(egui_view_t *self, uint8_t set_index)
{
    EGUI_LOCAL_INIT(egui_view_heatmap_chart_t);
    if (local->value_set_count == 0 || set_index >= local->value_set_count)
    {
        return;
    }
    if (local->current_value_set == set_index)
    {
        return;
    }
    local->current_value_set = set_index;
    egui_view_invalidate(self);
}

uint8_t egui_view_heatmap_chart_get_current_value_set(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_heatmap_chart_t);
    return local->current_value_set;
}

void egui_view_heatmap_chart_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_heatmap_chart_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_heatmap_chart_set_show_values(egui_view_t *self, uint8_t show_values)
{
    EGUI_LOCAL_INIT(egui_view_heatmap_chart_t);
    local->show_values = show_values ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_heatmap_chart_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_heatmap_chart_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_heatmap_chart_set_show_axis_labels(egui_view_t *self, uint8_t show_axis_labels)
{
    EGUI_LOCAL_INIT(egui_view_heatmap_chart_t);
    local->show_axis_labels = show_axis_labels ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_heatmap_chart_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t cold_color,
                                         egui_color_t hot_color, egui_color_t text_color, egui_color_t muted_text_color)
{
    EGUI_LOCAL_INIT(egui_view_heatmap_chart_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->cold_color = cold_color;
    local->hot_color = hot_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    egui_view_invalidate(self);
}

static void egui_view_heatmap_chart_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_heatmap_chart_t);
    egui_region_t region;
    egui_region_t content_region;
    egui_region_t cell_region;
    egui_region_t header_region;
    egui_region_t text_region;
    const char *header_text;
    const char *footer_text;
    const uint8_t *values;
    egui_color_t panel_color;
    egui_color_t shadow_color;
    egui_dim_t axis_left_width = 0;
    egui_dim_t axis_top_height = 0;
    egui_dim_t pill_w;
    egui_dim_t footer_x;
    uint8_t is_enabled;
    uint8_t r;
    uint8_t c;
    uint8_t index;
    uint8_t max_index = 0;
    uint8_t total_cells;
    char compact_badge[3] = {'L', 'K', '\0'};

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }
    if (local->rows == 0 || local->cols == 0)
    {
        return;
    }
    values = local->value_sets[local->current_value_set];
    if (values == NULL)
    {
        return;
    }
    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    total_cells = (uint8_t)(local->rows * local->cols);
    max_index = egui_view_heatmap_chart_find_max_index(values, total_cells);

    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_40);
    shadow_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_10);
    if (!is_enabled)
    {
        panel_color = egui_view_heatmap_chart_mix_disabled(panel_color);
        shadow_color = egui_view_heatmap_chart_mix_disabled(shadow_color);
    }

    if (HEATMAP_PANEL_SHADOW_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle_fill(region.location.x + 1, region.location.y + 2, region.size.width, region.size.height, 8, shadow_color,
                                              egui_color_alpha_mix(self->alpha, HEATMAP_PANEL_SHADOW_ALPHA));
    }
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, 8, panel_color,
                                          egui_color_alpha_mix(self->alpha, HEATMAP_PANEL_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, 8, 1, local->border_color,
                                     egui_color_alpha_mix(self->alpha, HEATMAP_PANEL_BORDER_ALPHA));
    if (HEATMAP_INNER_BORDER_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle(region.location.x + 2, region.location.y + 2, region.size.width - 4, region.size.height - 4, 6, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, HEATMAP_INNER_BORDER_ALPHA));
    }

    content_region.location.x = region.location.x + 4;
    content_region.location.y = region.location.y + 4;
    content_region.size.width = region.size.width - 8;
    content_region.size.height = region.size.height - 8;

    if (!local->show_header)
    {
        if (is_enabled)
        {
            compact_badge[0] = (local->current_value_set == 0) ? 'A' : ((local->current_value_set == 1) ? 'B' : 'C');
            compact_badge[1] = '\0';
        }
        if (HEATMAP_MINI_TOP_STRIP_ALPHA > 0)
        {
            egui_canvas_draw_round_rectangle_fill(region.location.x + 12, region.location.y + 6, region.size.width - 20, 2, 1,
                                                  is_enabled ? local->text_color : local->muted_text_color,
                                                  egui_color_alpha_mix(self->alpha, HEATMAP_MINI_TOP_STRIP_ALPHA));
        }
        if (HEATMAP_MINI_BADGE_MODE > 0)
        {
            header_region.location.x = content_region.location.x + (content_region.size.width - HEATMAP_MINI_BADGE_WIDTH) / 2;
            header_region.location.y = content_region.location.y + 6;
            header_region.size.width = HEATMAP_MINI_BADGE_WIDTH;
            header_region.size.height = 10;
            if (HEATMAP_MINI_BADGE_MODE > 1)
            {
                egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height,
                                                      4, egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                                                      egui_color_alpha_mix(self->alpha, HEATMAP_MINI_BADGE_FILL_ALPHA));
                egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4, 1,
                                                 egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                                 egui_color_alpha_mix(self->alpha, HEATMAP_MINI_BADGE_BORDER_ALPHA));
            }
            egui_canvas_draw_text_in_rect(local->font, compact_badge, &header_region, EGUI_ALIGN_CENTER,
                                          is_enabled ? local->text_color : local->muted_text_color, self->alpha);
            content_region.location.y += 12;
            content_region.size.height -= 12;
        }
    }

    if (local->show_header)
    {
        header_text = egui_view_heatmap_chart_get_header_text(local, is_enabled);
        pill_w = 34 + (egui_dim_t)strlen(header_text) * 6;
        if (pill_w < HEATMAP_HEADER_PILL_MIN_WIDTH)
        {
            pill_w = HEATMAP_HEADER_PILL_MIN_WIDTH;
        }
        if (pill_w > content_region.size.width)
        {
            pill_w = content_region.size.width;
        }
        header_region.location.x =
                (HEATMAP_HEADER_MODE == 0) ? (content_region.location.x + 4) : (content_region.location.x + (content_region.size.width - pill_w) / 2);
        header_region.location.y = content_region.location.y;
        header_region.size.width = (HEATMAP_HEADER_MODE == 0) ? (content_region.size.width - 8) : pill_w;
        header_region.size.height = 14;
        if (HEATMAP_HEADER_MODE == 2)
        {
            egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6,
                                                  egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                                                  egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
            egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6, 1,
                                             egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                             egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        }
        egui_canvas_draw_text_in_rect(local->font, header_text, &header_region, (HEATMAP_HEADER_MODE == 0) ? EGUI_ALIGN_LEFT : EGUI_ALIGN_CENTER,
                                      is_enabled ? local->text_color : local->muted_text_color, self->alpha);
        if (HEATMAP_HEADER_LINE_ALPHA > 0)
        {
            egui_canvas_draw_line(content_region.location.x + 5, content_region.location.y + 17, content_region.location.x + content_region.size.width - 6,
                                  content_region.location.y + 17, 1, local->border_color, egui_color_alpha_mix(self->alpha, HEATMAP_HEADER_LINE_ALPHA));
        }
        content_region.location.y += 20;
        content_region.size.height -= 20;
        if (HEATMAP_FOOTER_MODE > 0 && content_region.size.height > 14)
        {
            content_region.size.height -= 14;
        }
    }

    if (content_region.size.width <= 8 || content_region.size.height <= 8)
    {
        return;
    }

    if (local->show_axis_labels && local->row_labels != NULL && content_region.size.width > 26)
    {
        axis_left_width = 16;
        content_region.location.x += axis_left_width;
        content_region.size.width -= axis_left_width;
    }
    if (local->show_axis_labels && local->col_labels != NULL && content_region.size.height > 22)
    {
        axis_top_height = 10;
        content_region.location.y += axis_top_height;
        content_region.size.height -= axis_top_height;
    }
    if (content_region.size.width <= 8 || content_region.size.height <= 8)
    {
        return;
    }

    egui_view_heatmap_chart_draw_axis_labels(local, content_region, axis_left_width, axis_top_height, self->alpha);

    for (r = 0; r < local->rows; r++)
    {
        egui_dim_t y0 = content_region.location.y + (egui_dim_t)(((int32_t)content_region.size.height * r) / local->rows);
        egui_dim_t y1 = content_region.location.y + (egui_dim_t)(((int32_t)content_region.size.height * (r + 1)) / local->rows);
        for (c = 0; c < local->cols; c++)
        {
            egui_dim_t x0 = content_region.location.x + (egui_dim_t)(((int32_t)content_region.size.width * c) / local->cols);
            egui_dim_t x1 = content_region.location.x + (egui_dim_t)(((int32_t)content_region.size.width * (c + 1)) / local->cols);
            index = (uint8_t)(r * local->cols + c);
            cell_region.location.x = x0;
            cell_region.location.y = y0;
            cell_region.size.width = x1 - x0;
            cell_region.size.height = y1 - y0;
            egui_view_heatmap_chart_draw_cell(local, cell_region, values[index], (uint8_t)(index == max_index), self->alpha, is_enabled);
        }
    }
    if (local->show_axis_labels)
    {
        egui_view_heatmap_chart_draw_grid_overlay(local, content_region, self->alpha, is_enabled);
        egui_view_heatmap_chart_draw_hotspot_guides(local, content_region, max_index, self->alpha, is_enabled);
    }
    if (!local->show_header && HEATMAP_MINI_BOTTOM_STRIP_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle_fill(content_region.location.x + 8, region.location.y + region.size.height - 8, content_region.size.width - 14, 2, 1,
                                              is_enabled ? local->text_color : local->muted_text_color,
                                              egui_color_alpha_mix(self->alpha, HEATMAP_MINI_BOTTOM_STRIP_ALPHA));
    }
    if (!is_enabled)
    {
        if (HEATMAP_DISABLED_OVERLAY_ALPHA > 0)
        {
            egui_canvas_draw_round_rectangle_fill(content_region.location.x + 1, content_region.location.y + 1, content_region.size.width - 2,
                                                  content_region.size.height - 2, 4, EGUI_COLOR_BLACK,
                                                  egui_color_alpha_mix(self->alpha, HEATMAP_DISABLED_OVERLAY_ALPHA));
        }
        if (HEATMAP_DISABLED_CROSS_ALPHA > 0)
        {
            egui_canvas_draw_line(content_region.location.x + 2, content_region.location.y + 2, content_region.location.x + content_region.size.width - 3,
                                  content_region.location.y + content_region.size.height - 3, 1, local->muted_text_color,
                                  egui_color_alpha_mix(self->alpha, HEATMAP_DISABLED_CROSS_ALPHA));
            egui_canvas_draw_line(content_region.location.x + 2, content_region.location.y + content_region.size.height - 3,
                                  content_region.location.x + content_region.size.width - 3, content_region.location.y + 2, 1, local->muted_text_color,
                                  egui_color_alpha_mix(self->alpha, HEATMAP_DISABLED_CROSS_ALPHA));
        }
    }

    if (local->show_header && HEATMAP_FOOTER_MODE > 0)
    {
        footer_text = egui_view_heatmap_chart_get_footer_text(local);
        footer_x = content_region.location.x + (content_region.size.width - HEATMAP_FOOTER_WIDTH) / 2;
        text_region.location.x = content_region.location.x;
        text_region.location.y = content_region.location.y + content_region.size.height + 3;
        text_region.size.width = content_region.size.width;
        text_region.size.height = 11;
        if (HEATMAP_FOOTER_MODE > 1)
        {
            egui_canvas_draw_round_rectangle_fill(footer_x, content_region.location.y + content_region.size.height + 2, HEATMAP_FOOTER_WIDTH, 11, 5,
                                                  panel_color, egui_color_alpha_mix(self->alpha, HEATMAP_FOOTER_FILL_ALPHA));
            egui_canvas_draw_round_rectangle(footer_x, content_region.location.y + content_region.size.height + 2, HEATMAP_FOOTER_WIDTH, 11, 5, 1,
                                             egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                             egui_color_alpha_mix(self->alpha, HEATMAP_FOOTER_BORDER_ALPHA));
        }
        egui_canvas_draw_text_in_rect(local->font, footer_text, &text_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_heatmap_chart_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_heatmap_chart_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_heatmap_chart_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_heatmap_chart_t);
    uint8_t i;

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_heatmap_chart_t);
    egui_view_set_padding_all(self, 2);

    for (i = 0; i < EGUI_VIEW_HEATMAP_CHART_MAX_VALUE_SETS; i++)
    {
        local->value_sets[i] = NULL;
    }
    local->row_labels = NULL;
    local->col_labels = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x0B1220);
    local->border_color = EGUI_COLOR_HEX(0x475569);
    local->cold_color = EGUI_COLOR_HEX(0x2563EB);
    local->hot_color = EGUI_COLOR_HEX(0xF97316);
    local->text_color = EGUI_COLOR_HEX(0xE2E8F0);
    local->muted_text_color = EGUI_COLOR_HEX(0x94A3B8);
    local->rows = 4;
    local->cols = 4;
    local->value_set_count = 0;
    local->current_value_set = 0;
    local->show_values = 1;
    local->show_header = 1;
    local->show_axis_labels = 1;
}
