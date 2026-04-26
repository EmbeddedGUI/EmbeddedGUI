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

/** Worst-case byte count for one decoded scanline of image pixels. */
#define EGUI_IMAGE_DECODE_SINGLE_ROW_PIXEL_MAX_BYTES (EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH * EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE)

#if EGUI_IMAGE_CODEC_FAST_DRAW_ROW_CACHE_ENABLED
/** Worst-case byte count for the cached pixel rows kept for one fast-draw row band. */
#define EGUI_IMAGE_DECODE_ROW_CACHE_PIXEL_MAX_BYTES (EGUI_CONFIG_PFB_HEIGHT * EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH * EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE)
/** Worst-case byte count for the cached alpha rows kept for one fast-draw row band. */
#define EGUI_IMAGE_DECODE_ROW_CACHE_ALPHA_MAX_BYTES (EGUI_CONFIG_PFB_HEIGHT * EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH)
#endif

#if EGUI_IMAGE_CODEC_FAST_DRAW_ROW_CACHE_ENABLED
/** Select a 16-bit capacity type only when every relevant cache fits in 64 KiB. */
#define EGUI_IMAGE_DECODE_CAPACITY_CAN_USE_16BIT                                                                                                               \
    ((EGUI_IMAGE_DECODE_SINGLE_ROW_PIXEL_MAX_BYTES <= 0xFFFFu) && (EGUI_IMAGE_DECODE_ROW_CACHE_PIXEL_MAX_BYTES <= 0xFFFFu) &&                                  \
     (EGUI_IMAGE_DECODE_ROW_CACHE_ALPHA_MAX_BYTES <= 0xFFFFu))
#else
/** Select a 16-bit capacity type only when the row buffer fits in 64 KiB. */
#define EGUI_IMAGE_DECODE_CAPACITY_CAN_USE_16BIT (EGUI_IMAGE_DECODE_SINGLE_ROW_PIXEL_MAX_BYTES <= 0xFFFFu)
#endif

#if EGUI_IMAGE_DECODE_CAPACITY_CAN_USE_16BIT
typedef uint16_t egui_image_decode_capacity_t;
#else
typedef uint32_t egui_image_decode_capacity_t;
#endif

typedef enum
{
    EGUI_IMAGE_DECODE_CACHE_MODE_NONE = 0,   // no reusable decoded image data is currently cached
    EGUI_IMAGE_DECODE_CACHE_MODE_ROW_BAND,   // cache contains a vertical band of decoded rows for tiled drawing
    EGUI_IMAGE_DECODE_CACHE_MODE_FULL_IMAGE, // cache contains the entire decoded image payload
} egui_image_decode_cache_mode_t;

/** Tracks what image data is currently stored in the shared decode cache buffers. */
typedef struct
{
    const void *image_info;   // image descriptor that owns the cached decoded data
    uint16_t row_band_start;  // first cached row when `mode` is `ROW_BAND`
    uint16_t cache_col_start; // first cached column covered by the row-band cache
    uint16_t cache_col_count; // number of cached columns in the row-band cache
    uint8_t mode;             // enum egui_image_decode_cache_mode
} egui_image_decode_cache_state_t;

#if EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES > 0
/** Heap-backed decoded-image cache kept across draw calls for one frequently reused image. */
typedef struct
{
    const void *image_info;   // image descriptor currently occupying the persistent cache
    uint8_t *buffer;          // heap buffer that stores decoded pixel data followed by optional alpha data
    uint32_t capacity_bytes;  // total heap capacity owned by `buffer`
    uint32_t pixel_bytes;     // number of bytes used by decoded pixel rows
    uint16_t width;           // decoded image width cached in `buffer`
    uint16_t height;          // decoded image height cached in `buffer`
    uint8_t bytes_per_pixel;  // decoded pixel stride for one pixel
    uint16_t alpha_row_bytes; // bytes per decoded alpha row when alpha data is cached
} egui_image_decode_persistent_cache_t;
#endif

/** Shared decode scratch buffers and optional row/full-image caches reused across image codecs. */
typedef struct
{
#if !EGUI_IMAGE_CODEC_FAST_DRAW_ROW_CACHE_ENABLED
    uint8_t *row_pixel_buf;                              // decoded pixel row scratch when fast-draw row caching is disabled
    egui_image_decode_capacity_t row_pixel_buf_capacity; // capacity of `row_pixel_buf` in bytes
#endif
    uint8_t *row_alpha_buf;                              // decoded alpha row scratch shared by codecs
    egui_image_decode_capacity_t row_alpha_buf_capacity; // capacity of `row_alpha_buf` in bytes
#if EGUI_IMAGE_CODEC_FAST_DRAW_ROW_CACHE_ENABLED
    uint8_t *row_cache_pixel;                              // cached decoded pixel rows for the current band/full-image cache
    uint8_t *row_cache_alpha;                              // cached decoded alpha rows paired with `row_cache_pixel`
    egui_image_decode_cache_state_t cache_state;           // metadata describing what the row cache currently holds
    egui_image_decode_capacity_t row_cache_pixel_capacity; // capacity of `row_cache_pixel` in bytes
    egui_image_decode_capacity_t row_cache_alpha_capacity; // capacity of `row_cache_alpha` in bytes
#endif
#if EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES > 0
    egui_image_decode_persistent_cache_t persistent_cache; // optional heap-backed persistent decoded-image cache
#endif
} egui_image_decode_cache_storage_t;

#if EGUI_CONFIG_FUNCTION_IMAGE_CODEC_QOI
typedef uint16_t egui_image_qoi_row_index_t;
#define EGUI_IMAGE_QOI_ROW_INDEX_MAX UINT16_MAX

/** Number of decoder checkpoints kept so QOI row seeks can restart from a nearby state. */
#define EGUI_IMAGE_QOI_CHECKPOINT_COUNT 2

/** Incremental QOI decoder state that can continue row decoding without restarting the stream. */
typedef struct
{
    const egui_image_qoi_info_t *info;      // QOI image being decoded
    uint32_t data_pos;                      // byte position inside the encoded payload
    egui_image_qoi_row_index_t current_row; // row that will be decoded next
    uint8_t run;                            // remaining run-length pixels from the current QOI op
    uint16_t prev_rgb565;                   // previous pixel cached in RGB565 mode
    uint8_t prev_r;                         // previous pixel red channel
    uint8_t prev_g;                         // previous pixel green channel
    uint8_t prev_b;                         // previous pixel blue channel
    uint8_t prev_a;                         // previous pixel alpha channel
    uint16_t index_rgb565[64];              // QOI index table used when decoding to RGB565
    uint32_t index_rgba[64];                // QOI index table used when decoding to RGBA/ARGB formats
} egui_image_qoi_decode_state_t;

/** Snapshot of a QOI decoder state stored for faster row seeking. */
typedef struct
{
    egui_image_qoi_decode_state_t state; // decoder state captured at `row`
    uint16_t row;                        // row index represented by this checkpoint
    const egui_image_qoi_info_t *info;   // image that owns this checkpoint
} egui_image_qoi_checkpoint_t;

/** Shared QOI cache consisting of the live decode state plus a few reusable checkpoints. */
typedef struct
{
    egui_image_qoi_decode_state_t state;                                      // live decoder state used by the next row decode
    egui_image_qoi_checkpoint_t checkpoints[EGUI_IMAGE_QOI_CHECKPOINT_COUNT]; // stored restart points for row seeks
    uint8_t checkpoints_ready;                                                // number of initialized entries in `checkpoints`
    uint8_t checkpoint_next;                                                  // round-robin index to overwrite when storing the next checkpoint
} egui_image_qoi_cache_t;
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_CODEC_RLE
/** Incremental RLE decoder position for continuing row decoding from the last stop point. */
typedef struct
{
    const egui_image_rle_info_t *info; // RLE image being decoded
    uint32_t data_pos;                 // current byte position inside encoded pixel data
    uint32_t alpha_pos;                // current byte position inside encoded alpha data
    uint16_t current_row;              // row that will be decoded next
} egui_image_rle_decode_state_t;

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
#if (EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES >= EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE)
#define EGUI_IMAGE_RLE_EXTERNAL_CACHE_SHARE_WINDOW 1
#else
#define EGUI_IMAGE_RLE_EXTERNAL_CACHE_SHARE_WINDOW 0
#endif

typedef struct
{
    const uint8_t *src;     // external resource base pointer currently cached
    uint32_t src_len;       // total byte length available from `src`
    uint32_t window_offset; // byte offset of the cached window inside `src`
    uint16_t window_size;   // valid byte count currently cached
#if EGUI_IMAGE_RLE_EXTERNAL_CACHE_SHARE_WINDOW
    uint32_t shared_generation; // generation id of the shared external cache window
#else
    uint8_t window[EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE]; // private cached slice of external resource data
#endif
} egui_image_rle_external_window_cache_t;
#else
typedef struct
{
    uint8_t unused; // placeholder so the type exists when external resources are disabled
} egui_image_rle_external_window_cache_t;
#endif

/** Shared RLE cache combining the live decoder state, a restart checkpoint, and external I/O cache data. */
typedef struct
{
    egui_image_rle_decode_state_t state;                          // live decoder state used for the next row decode
    egui_image_rle_decode_state_t checkpoint;                     // restart point saved for backward/nearby row seeks
    egui_image_rle_external_window_cache_t external_window_cache; // external-resource read cache used by the decoder
} egui_image_rle_cache_t;

#define EGUI_IMAGE_RLE_CACHE_TYPES_DEFINED 1
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_DECODE_CACHE_H_ */
