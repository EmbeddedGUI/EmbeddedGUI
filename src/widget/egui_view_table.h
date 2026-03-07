#ifndef _EGUI_VIEW_TABLE_H_
#define _EGUI_VIEW_TABLE_H_

#include "egui_view.h"
#include "font/egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_TABLE_MAX_ROWS 16
#define EGUI_VIEW_TABLE_MAX_COLS 8

typedef struct egui_view_table egui_view_table_t;
struct egui_view_table
{
    egui_view_t base;

    const char *cells[EGUI_VIEW_TABLE_MAX_ROWS][EGUI_VIEW_TABLE_MAX_COLS];
    uint8_t row_count;
    uint8_t col_count;
    uint8_t header_rows;
    egui_dim_t row_height;
    egui_color_t header_bg_color;
    egui_color_t header_text_color;
    egui_color_t cell_text_color;
    egui_color_t grid_color;
    uint8_t show_grid;
    const egui_font_t *font;
};

// ============== Table Params ==============
typedef struct egui_view_table_params egui_view_table_params_t;
struct egui_view_table_params
{
    egui_region_t region;
    uint8_t row_count;
    uint8_t col_count;
};

#define EGUI_VIEW_TABLE_PARAMS_INIT(_name, _x, _y, _w, _h, _row_count, _col_count)                                                                             \
    static const egui_view_table_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .row_count = (_row_count), .col_count = (_col_count)}

void egui_view_table_apply_params(egui_view_t *self, const egui_view_table_params_t *params);
void egui_view_table_init_with_params(egui_view_t *self, const egui_view_table_params_t *params);

void egui_view_table_set_cell(egui_view_t *self, uint8_t row, uint8_t col, const char *text);
void egui_view_table_set_size(egui_view_t *self, uint8_t rows, uint8_t cols);
void egui_view_table_set_header_rows(egui_view_t *self, uint8_t count);
void egui_view_table_set_row_height(egui_view_t *self, egui_dim_t height);
void egui_view_table_set_show_grid(egui_view_t *self, uint8_t show);
void egui_view_table_set_header_bg_color(egui_view_t *self, egui_color_t color);
void egui_view_table_set_grid_color(egui_view_t *self, egui_color_t color);
void egui_view_table_on_draw(egui_view_t *self);
void egui_view_table_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_TABLE_H_ */
