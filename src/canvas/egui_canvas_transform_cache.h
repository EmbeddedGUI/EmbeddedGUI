#ifndef _EGUI_CANVAS_TRANSFORM_CACHE_H_
#define _EGUI_CANVAS_TRANSFORM_CACHE_H_

#include "font/egui_font.h"
#include "image/egui_image_std.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
typedef struct
{
    const egui_image_std_info_t *info;
    const void *src_addr;
    int32_t chunk_row_start;
    int32_t chunk_row_count;
    uint32_t row_bytes;
    uint32_t rows_per_chunk;
    uint32_t shared_generation;
} image_transform_external_data_row_slot_t;

typedef struct
{
    const egui_image_std_info_t *info;
    const void *src_addr;
    int32_t chunk_row_start;
    int32_t chunk_row_count;
    uint32_t row_bytes;
    uint32_t rows_per_chunk;
    uint32_t shared_generation;
} image_transform_external_alpha_row_slot_t;
#endif

#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT
typedef uint16_t text_transform_layout_pixel_idx_t;
#define TEXT_TRANSFORM_LAYOUT_PIXEL_IDX_MAX UINT16_MAX
#else
typedef uint32_t text_transform_layout_pixel_idx_t;
#define TEXT_TRANSFORM_LAYOUT_PIXEL_IDX_MAX UINT32_MAX
#endif

#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT
typedef uint16_t text_transform_layout_index_t;
#define TEXT_TRANSFORM_LAYOUT_INDEX_MAX UINT16_MAX
#else
typedef int text_transform_layout_index_t;
#define TEXT_TRANSFORM_LAYOUT_INDEX_MAX 0x7FFFFFFF
#endif

typedef struct
{
    text_transform_layout_pixel_idx_t pixel_idx;
    int16_t x;
    int16_t y;
    uint8_t box_w;
    uint8_t box_h;
} text_transform_layout_glyph_t;

typedef struct
{
    const egui_font_t *font;
    const void *string;
    egui_dim_t line_space;
    int count;
    int line_count;
} text_transform_layout_cache_t;

typedef struct
{
    text_transform_layout_index_t start;
    text_transform_layout_index_t end;
    int16_t x_min;
    int16_t x_max;
    int16_t y_min;
    int16_t y_max;
} text_transform_layout_line_t;

typedef struct
{
    uint8_t valid;
    int16_t text_w;
    int16_t text_h;
    int16_t cx;
    int16_t cy;
    egui_dim_t x;
    egui_dim_t y;
    int16_t angle_deg;
    int16_t scale_q8;
    egui_alpha_t alpha;
    int32_t inv_m00;
    int32_t inv_m01;
    int32_t inv_m10;
    int32_t inv_m11;
    int32_t min_bx;
    int32_t min_by;
    int32_t max_bx;
    int32_t max_by;
    int32_t offx;
    int32_t offy;
} text_transform_prepare_cache_t;

typedef struct
{
    uint8_t *buf;
    int capacity;
} text_transform_visible_tile_cache_t;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CANVAS_TRANSFORM_CACHE_H_ */
