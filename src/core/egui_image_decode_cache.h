#ifndef _EGUI_IMAGE_DECODE_CACHE_H_
#define _EGUI_IMAGE_DECODE_CACHE_H_

#include "egui_common.h"
#include "image/egui_image_std.h"
#include "image/egui_image_qoi.h"
#include "image/egui_image_rle.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_IMAGE_DECODE_SINGLE_ROW_PIXEL_MAX_BYTES (EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH * EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE)

#if EGUI_IMAGE_CODEC_FAST_DRAW_ROW_CACHE_ENABLED
#define EGUI_IMAGE_DECODE_ROW_CACHE_PIXEL_MAX_BYTES (EGUI_CONFIG_PFB_HEIGHT * EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH * EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE)
#define EGUI_IMAGE_DECODE_ROW_CACHE_ALPHA_MAX_BYTES (EGUI_CONFIG_PFB_HEIGHT * EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH)
#endif

#if EGUI_IMAGE_CODEC_FAST_DRAW_ROW_CACHE_ENABLED
#define EGUI_IMAGE_DECODE_CAPACITY_CAN_USE_16BIT                                                                                                               \
    ((EGUI_IMAGE_DECODE_SINGLE_ROW_PIXEL_MAX_BYTES <= 0xFFFFu) && (EGUI_IMAGE_DECODE_ROW_CACHE_PIXEL_MAX_BYTES <= 0xFFFFu) &&                                  \
     (EGUI_IMAGE_DECODE_ROW_CACHE_ALPHA_MAX_BYTES <= 0xFFFFu))
#else
#define EGUI_IMAGE_DECODE_CAPACITY_CAN_USE_16BIT (EGUI_IMAGE_DECODE_SINGLE_ROW_PIXEL_MAX_BYTES <= 0xFFFFu)
#endif

#if EGUI_IMAGE_DECODE_CAPACITY_CAN_USE_16BIT
typedef uint16_t egui_image_decode_capacity_t;
#else
typedef uint32_t egui_image_decode_capacity_t;
#endif

typedef enum
{
    EGUI_IMAGE_DECODE_CACHE_MODE_NONE = 0,
    EGUI_IMAGE_DECODE_CACHE_MODE_ROW_BAND,
    EGUI_IMAGE_DECODE_CACHE_MODE_FULL_IMAGE,
} egui_image_decode_cache_mode_t;

typedef struct
{
    const void *image_info;
    uint16_t row_band_start;
    uint16_t cache_col_start;
    uint16_t cache_col_count;
    uint8_t mode;
} egui_image_decode_cache_state_t;

#if EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES > 0
typedef struct
{
    const void *image_info;
    uint8_t *buffer;
    uint32_t capacity_bytes;
    uint32_t pixel_bytes;
    uint16_t width;
    uint16_t height;
    uint8_t bytes_per_pixel;
    uint16_t alpha_row_bytes;
} egui_image_decode_persistent_cache_t;
#endif

typedef struct
{
#if !EGUI_IMAGE_CODEC_FAST_DRAW_ROW_CACHE_ENABLED
    uint8_t *row_pixel_buf;
    egui_image_decode_capacity_t row_pixel_buf_capacity;
#endif
    uint8_t *row_alpha_buf;
    egui_image_decode_capacity_t row_alpha_buf_capacity;
#if EGUI_IMAGE_CODEC_FAST_DRAW_ROW_CACHE_ENABLED
    uint8_t *row_cache_pixel;
    uint8_t *row_cache_alpha;
    egui_image_decode_cache_state_t cache_state;
    egui_image_decode_capacity_t row_cache_pixel_capacity;
    egui_image_decode_capacity_t row_cache_alpha_capacity;
#endif
#if EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES > 0
    egui_image_decode_persistent_cache_t persistent_cache;
#endif
} egui_image_decode_cache_storage_t;

#if EGUI_CONFIG_FUNCTION_IMAGE_CODEC_QOI
typedef uint16_t egui_image_qoi_row_index_t;
#define EGUI_IMAGE_QOI_ROW_INDEX_MAX UINT16_MAX

#define EGUI_IMAGE_QOI_CHECKPOINT_COUNT 2

typedef struct
{
    const egui_image_qoi_info_t *info;
    uint32_t data_pos;
    egui_image_qoi_row_index_t current_row;
    uint8_t run;
    uint16_t prev_rgb565;
    uint8_t prev_r;
    uint8_t prev_g;
    uint8_t prev_b;
    uint8_t prev_a;
    uint16_t index_rgb565[64];
    uint32_t index_rgba[64];
} egui_image_qoi_decode_state_t;

typedef struct
{
    egui_image_qoi_decode_state_t state;
    uint16_t row;
    const egui_image_qoi_info_t *info;
} egui_image_qoi_checkpoint_t;

typedef struct
{
    egui_image_qoi_decode_state_t state;
    egui_image_qoi_checkpoint_t checkpoints[EGUI_IMAGE_QOI_CHECKPOINT_COUNT];
    uint8_t checkpoints_ready;
    uint8_t checkpoint_next;
} egui_image_qoi_cache_t;
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_CODEC_RLE
typedef struct
{
    const egui_image_rle_info_t *info;
    uint32_t data_pos;
    uint32_t alpha_pos;
    uint16_t current_row;
} egui_image_rle_decode_state_t;

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
#ifndef EGUI_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE
#define EGUI_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE 128
#endif

#if (EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES >= EGUI_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE)
#define EGUI_IMAGE_RLE_EXTERNAL_CACHE_SHARE_WINDOW 1
#else
#define EGUI_IMAGE_RLE_EXTERNAL_CACHE_SHARE_WINDOW 0
#endif

typedef struct
{
    const uint8_t *src;
    uint32_t src_len;
    uint32_t window_offset;
    uint16_t window_size;
#if EGUI_IMAGE_RLE_EXTERNAL_CACHE_SHARE_WINDOW
    uint32_t shared_generation;
#else
    uint8_t window[EGUI_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE];
#endif
} egui_image_rle_external_window_cache_t;
#else
typedef struct
{
    uint8_t unused;
} egui_image_rle_external_window_cache_t;
#endif

typedef struct
{
    egui_image_rle_decode_state_t state;
    egui_image_rle_decode_state_t checkpoint;
    egui_image_rle_external_window_cache_t external_window_cache;
} egui_image_rle_cache_t;

#define EGUI_IMAGE_RLE_CACHE_TYPES_DEFINED 1
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_DECODE_CACHE_H_ */
