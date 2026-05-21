#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_table.h"
#include "core/egui_core.h"
#include "core/egui_api.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

/**
 * @file egui_view_table.c
 * @brief Fixed-capacity table view that renders borrowed strings in a simple text grid.
 *
 * Reading notes:
 * - cell text is
 * borrowed rather than copied, so callers own the string lifetime;
 * - each visible column uses the same width because the widget divides the view width
 * evenly;
 * - header rows are purely a visual style change and still use the same text layout path as body rows.
 */

/** Replace one borrowed cell string and redraw the table if the indices are within capacity. */
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

const char *egui_view_table_get_cell(egui_view_t *self, uint8_t row, uint8_t col)
{
    if (self == NULL || row >= EGUI_VIEW_TABLE_MAX_ROWS || col >= EGUI_VIEW_TABLE_MAX_COLS)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_table_t);
    return local->cells[row][col];
}

static void egui_view_table_clear_cells(egui_view_table_t *local)
{
    uint8_t row;
    uint8_t col;

    for (row = 0; row < EGUI_VIEW_TABLE_MAX_ROWS; row++)
    {
        for (col = 0; col < EGUI_VIEW_TABLE_MAX_COLS; col++)
        {
            local->cells[row][col] = NULL;
        }
    }
}

/** Clamp the visible row and column counts to the built-in storage capacity. */
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

uint8_t egui_view_table_get_row_count(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_table_t);
    return local->row_count;
}

uint8_t egui_view_table_get_col_count(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_table_t);
    return local->col_count;
}

/** Mark the first `count` visible rows as header rows. */
void egui_view_table_set_header_rows(egui_view_t *self, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->header_rows = count;
    egui_view_invalidate(self);
}

uint8_t egui_view_table_get_header_rows(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_table_t);
    return local->header_rows;
}

/** Change the uniform row height used by every table row. */
void egui_view_table_set_row_height(egui_view_t *self, egui_dim_t height)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->row_height = height;
    egui_view_invalidate(self);
}

egui_dim_t egui_view_table_get_row_height(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_table_t);
    return local->row_height;
}

/** Show or hide the grid lines drawn between cells. */
void egui_view_table_set_show_grid(egui_view_t *self, uint8_t show)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->show_grid = show;
    egui_view_invalidate(self);
}

uint8_t egui_view_table_get_show_grid(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_table_t);
    return local->show_grid;
}

/** Override the background color used for header rows. */
void egui_view_table_set_header_bg_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->header_bg_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_table_get_header_bg_color(egui_view_t *self)
{
    egui_color_t zero;

    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_table_t);
    return local->header_bg_color;
}

void egui_view_table_set_header_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->header_text_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_table_get_header_text_color(egui_view_t *self)
{
    egui_color_t zero;

    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_table_t);
    return local->header_text_color;
}

void egui_view_table_set_cell_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->cell_text_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_table_get_cell_text_color(egui_view_t *self)
{
    egui_color_t zero;

    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_table_t);
    return local->cell_text_color;
}

/** Override the stroke color used for the optional grid. */
void egui_view_table_set_grid_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->grid_color = color;
    egui_view_invalidate(self);
}

egui_color_t egui_view_table_get_grid_color(egui_view_t *self)
{
    egui_color_t zero;

    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_table_t);
    return local->grid_color;
}

void egui_view_table_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    local->font = font != NULL ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

const egui_font_t *egui_view_table_get_font(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_table_t);
    return local->font;
}

/** Draw header fills, cell text, and the optional grid for the current visible table slice. */
void egui_view_table_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_table_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);

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
            egui_canvas_draw_rectangle_fill(canvas, x, cell_y, w, rh, local->header_bg_color, EGUI_ALPHA_100);
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
                egui_canvas_draw_text_in_rect(canvas, font, text, &cell_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, color, EGUI_ALPHA_100);
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
            egui_canvas_draw_line(canvas, x, ly, x + w - 1, ly, 1, local->grid_color, EGUI_ALPHA_100);
        }
        // Vertical lines
        for (c = 0; c <= local->col_count; c++)
        {
            egui_dim_t lx = (c == local->col_count) ? (x + w - 1) : (x + c * col_w);
            egui_canvas_draw_line(canvas, lx, y, lx, y + local->row_count * rh - 1, 1, local->grid_color, EGUI_ALPHA_100);
        }
    }
}

/* Use default view behavior and override only the table's custom draw hook. */
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

/** Initialize empty cell storage plus default colors, row sizing, and grid visibility. */
void egui_view_table_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_table_t);
    // call super init.
    egui_view_init(self, core);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_table_t);

    // init local data.
    egui_view_table_clear_cells(local);
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

/** Apply geometry and initial visible dimensions from one parameter block. */
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

/** Convenience helper that initializes the table before applying params. */
void egui_view_table_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_table_params_t *params)
{
    egui_view_table_init(self, core);
    egui_view_table_apply_params(self, params);
}
