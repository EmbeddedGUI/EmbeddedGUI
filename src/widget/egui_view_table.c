#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_table.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

void egui_view_table_set_cell(egui_view_t *self, uint8_t row, uint8_t col, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    if (row >= EGUI_VIEW_TABLE_MAX_ROWS || col >= EGUI_VIEW_TABLE_MAX_COLS)
    {
        return;
    }
    local->cells[row][col] = text;
    egui_view_invalidate(self);
}

void egui_view_table_set_size(egui_view_t *self, uint8_t rows, uint8_t cols)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    if (rows > EGUI_VIEW_TABLE_MAX_ROWS)
    {
        rows = EGUI_VIEW_TABLE_MAX_ROWS;
    }
    if (cols > EGUI_VIEW_TABLE_MAX_COLS)
    {
        cols = EGUI_VIEW_TABLE_MAX_COLS;
    }
    local->row_count = rows;
    local->col_count = cols;
    egui_view_invalidate(self);
}

void egui_view_table_set_header_rows(egui_view_t *self, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->header_rows = count;
    egui_view_invalidate(self);
}

void egui_view_table_set_row_height(egui_view_t *self, egui_dim_t height)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->row_height = height;
    egui_view_invalidate(self);
}

void egui_view_table_set_show_grid(egui_view_t *self, uint8_t show)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->show_grid = show;
    egui_view_invalidate(self);
}

void egui_view_table_set_header_bg_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->header_bg_color = color;
    egui_view_invalidate(self);
}

void egui_view_table_set_grid_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->grid_color = color;
    egui_view_invalidate(self);
}

void egui_view_table_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_table_t);

    if (local->row_count == 0 || local->col_count == 0)
    {
        return;
    }

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t x = region.location.x;
    egui_dim_t y = region.location.y;
    egui_dim_t w = region.size.width;

    const egui_font_t *font = local->font ? local->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t col_w = w / local->col_count;
    egui_dim_t rh = local->row_height;
    uint8_t text_pad = 4;

    uint8_t r;
    uint8_t c;

    // Draw rows
    for (r = 0; r < local->row_count; r++)
    {
        egui_dim_t cell_y = y + r * rh;

        // Header background
        if (r < local->header_rows)
        {
            egui_canvas_draw_rectangle_fill(x, cell_y, w, rh, local->header_bg_color, EGUI_ALPHA_100);
        }

        // Draw cells
        for (c = 0; c < local->col_count; c++)
        {
            egui_dim_t cell_x = x + c * col_w;
            const char *text = local->cells[r][c];
            if (text != NULL)
            {
                egui_region_t cell_rect = {{cell_x + text_pad, cell_y}, {col_w - text_pad, rh}};
                egui_color_t color = (r < local->header_rows) ? local->header_text_color : local->cell_text_color;
                egui_canvas_draw_text_in_rect(font, text, &cell_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, color, EGUI_ALPHA_100);
            }
        }
    }

    // Draw grid lines
    if (local->show_grid)
    {
        // Horizontal lines
        for (r = 0; r <= local->row_count; r++)
        {
            egui_dim_t ly = y + r * rh;
            if (r == local->row_count)
            {
                ly = y + local->row_count * rh - 1;
            }
            egui_canvas_draw_line(x, ly, x + w - 1, ly, 1, local->grid_color, EGUI_ALPHA_100);
        }
        // Vertical lines
        for (c = 0; c <= local->col_count; c++)
        {
            egui_dim_t lx = (c == local->col_count) ? (x + w - 1) : (x + c * col_w);
            egui_canvas_draw_line(lx, y, lx, y + local->row_count * rh - 1, 1, local->grid_color, EGUI_ALPHA_100);
        }
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_table_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_table_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_table_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_table_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_table_t);

    // init local data.
    egui_api_memset(local->cells, 0, sizeof(local->cells));
    local->row_count = 0;
    local->col_count = 0;
    local->header_rows = 1;
    local->row_height = 20;
    local->header_bg_color = EGUI_THEME_PRIMARY_DARK;
    local->header_text_color = EGUI_THEME_TEXT;
    local->cell_text_color = EGUI_THEME_TEXT;
    local->grid_color = EGUI_THEME_TRACK_BG;
    local->show_grid = 1;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    egui_view_set_view_name(self, "egui_view_table");
}

void egui_view_table_apply_params(egui_view_t *self, const egui_view_table_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_table_t);

    self->region = params->region;

    if (params->row_count <= EGUI_VIEW_TABLE_MAX_ROWS)
    {
        local->row_count = params->row_count;
    }
    if (params->col_count <= EGUI_VIEW_TABLE_MAX_COLS)
    {
        local->col_count = params->col_count;
    }

    egui_view_invalidate(self);
}

void egui_view_table_init_with_params(egui_view_t *self, const egui_view_table_params_t *params)
{
    egui_view_table_init(self);
    egui_view_table_apply_params(self, params);
}
