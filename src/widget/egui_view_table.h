#ifndef _EGUI_VIEW_TABLE_H_
#define _EGUI_VIEW_TABLE_H_

#include "egui_view.h"
#include "font/egui_font.h"

/**
 * @brief Fixed-capacity table widget that draws text cells in a uniform grid.
 *
 * The widget stores borrowed string pointers for each cell, divides the current
 * width evenly across visible columns, and optionally highlights the first few
 * rows as header rows. It is intended for lightweight status tables rather than
 * rich per-cell widgets or variable column sizing.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of rows stored by one table instance. */
#define EGUI_VIEW_TABLE_MAX_ROWS 16
/** Maximum number of columns stored by one table instance. */
#define EGUI_VIEW_TABLE_MAX_COLS 8

typedef struct egui_view_table egui_view_table_t;
struct egui_view_table
{
    /* Base view responsible for region, invalidation, and draw dispatch. */
    egui_view_t base;

    /* Borrowed text pointers for each logical cell in the fixed-capacity grid. */
    const char *cells[EGUI_VIEW_TABLE_MAX_ROWS][EGUI_VIEW_TABLE_MAX_COLS];
    /* Number of visible rows currently drawn. */
    uint8_t row_count;
    /* Number of visible columns currently drawn. */
    uint8_t col_count;
    /* Number of leading rows styled as table headers. */
    uint8_t header_rows;
    /* Uniform pixel height used by every row. */
    egui_dim_t row_height;
    /* Fill color used behind header rows. */
    egui_color_t header_bg_color;
    /* Text color used inside header cells. */
    egui_color_t header_text_color;
    /* Text color used inside normal body cells. */
    egui_color_t cell_text_color;
    /* Stroke color used for optional grid lines. */
    egui_color_t grid_color;
    /* Whether row and column separators are drawn. */
    uint8_t show_grid;
    /* Font used for both header and body text. */
    const egui_font_t *font;
};

// ============== Table Params ==============
typedef struct egui_view_table_params egui_view_table_params_t;
struct egui_view_table_params
{
    /* Outer region occupied by the whole table. */
    egui_region_t region;
    /* Initial visible row count. */
    uint8_t row_count;
    /* Initial visible column count. */
    uint8_t col_count;
};

/** Build a table parameter block with region plus initial row and column counts. */
#define EGUI_VIEW_TABLE_PARAMS_INIT(_name, _x, _y, _w, _h, _row_count, _col_count)                                                                             \
    static const egui_view_table_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .row_count = (_row_count), .col_count = (_col_count)}

/** Apply region, row count, and column count from one parameter block. */
void egui_view_table_apply_params(egui_view_t *self, const egui_view_table_params_t *params);
/** Initialize a table view and immediately apply its parameter block. */
void egui_view_table_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_table_params_t *params);

/** Set one cell's text pointer. The widget borrows the string and ignores indices outside the fixed table capacity. */
void egui_view_table_set_cell(egui_view_t *self, uint8_t row, uint8_t col, const char *text);
/** Return one cell's borrowed text pointer, or NULL when the index is outside capacity. */
const char *egui_view_table_get_cell(egui_view_t *self, uint8_t row, uint8_t col);
/** Set the visible row and column counts. Values larger than the built-in maxima are clamped. */
void egui_view_table_set_size(egui_view_t *self, uint8_t rows, uint8_t cols);
/** Return the visible row count. */
uint8_t egui_view_table_get_row_count(egui_view_t *self);
/** Return the visible column count. */
uint8_t egui_view_table_get_col_count(egui_view_t *self);
/** Mark the first `count` rows as header rows so they use header colors. */
void egui_view_table_set_header_rows(egui_view_t *self, uint8_t count);
/** Return the number of leading rows styled as headers. */
uint8_t egui_view_table_get_header_rows(egui_view_t *self);
/** Set the height of each row in pixels. */
void egui_view_table_set_row_height(egui_view_t *self, egui_dim_t height);
/** Return the configured row height in pixels. */
egui_dim_t egui_view_table_get_row_height(egui_view_t *self);
/** Show or hide the grid lines around cells. */
void egui_view_table_set_show_grid(egui_view_t *self, uint8_t show);
/** Return whether grid lines are drawn. */
uint8_t egui_view_table_get_show_grid(egui_view_t *self);
/** Set the fill color used for header rows. */
void egui_view_table_set_header_bg_color(egui_view_t *self, egui_color_t color);
/** Return the fill color used for header rows. */
egui_color_t egui_view_table_get_header_bg_color(egui_view_t *self);
/** Set the text color used inside header cells. */
void egui_view_table_set_header_text_color(egui_view_t *self, egui_color_t color);
/** Return the text color used inside header cells. */
egui_color_t egui_view_table_get_header_text_color(egui_view_t *self);
/** Set the text color used inside body cells. */
void egui_view_table_set_cell_text_color(egui_view_t *self, egui_color_t color);
/** Return the text color used inside body cells. */
egui_color_t egui_view_table_get_cell_text_color(egui_view_t *self);
/** Set the color used for grid lines. */
void egui_view_table_set_grid_color(egui_view_t *self, egui_color_t color);
/** Return the color used for grid lines. */
egui_color_t egui_view_table_get_grid_color(egui_view_t *self);
/** Override the font used for both header and body text. Passing NULL restores the default font. */
void egui_view_table_set_font(egui_view_t *self, const egui_font_t *font);
/** Return the font used for both header and body text. */
const egui_font_t *egui_view_table_get_font(egui_view_t *self);
/** Default draw hook used by the table API table. */
void egui_view_table_on_draw(egui_view_t *self);
/** Initialize the fixed-capacity table widget with default font, colors, and grid rendering enabled. */
void egui_view_table_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TABLE_H_ */
