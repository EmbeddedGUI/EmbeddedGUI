#ifndef _EGUI_IMAGE_DECODE_UTILS_H_
#define _EGUI_IMAGE_DECODE_UTILS_H_

#include "egui_image.h"
#include "egui_image_std.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE || EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE

/* Shared row decode buffers (static, zero dynamic allocation).
 * Pixel buffer: EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH * 4 bytes (worst case RGB32)
 * Alpha buffer: EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH bytes
 */
extern uint8_t egui_image_decode_row_pixel_buf[];
extern uint8_t egui_image_decode_row_alpha_buf[];

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
/*
 * Row-band decode cache: PFB_HEIGHT rows of decoded pixel + alpha data.
 * On the first tile of a row band, all rows are decoded into the cache.
 * Subsequent horizontal tiles blend from the cache without re-decoding.
 */
extern uint8_t egui_image_decode_row_cache_pixel[];
extern uint8_t egui_image_decode_row_cache_alpha[];

typedef enum
{
    EGUI_IMAGE_DECODE_CACHE_MODE_NONE = 0,
    EGUI_IMAGE_DECODE_CACHE_MODE_ROW_BAND,
    EGUI_IMAGE_DECODE_CACHE_MODE_FULL_IMAGE,
} egui_image_decode_cache_mode_t;

/* Return pointer to the pixel cache buffer for a given row offset within the band */
static inline uint8_t *egui_image_decode_cache_pixel_row(uint16_t row_in_band, uint16_t img_width, uint8_t bytes_per_pixel)
{
    return &egui_image_decode_row_cache_pixel[(uint32_t)row_in_band * img_width * bytes_per_pixel];
}

/* Return pointer to the alpha cache buffer for a given row offset within the band */
static inline uint8_t *egui_image_decode_cache_alpha_row(uint16_t row_in_band, uint16_t img_width)
{
    return &egui_image_decode_row_cache_alpha[(uint32_t)row_in_band * img_width];
}

static inline uint8_t *egui_image_decode_cache_alpha_row_bytes(uint16_t row_in_band, uint16_t alpha_row_bytes)
{
    return &egui_image_decode_row_cache_alpha[(uint32_t)row_in_band * alpha_row_bytes];
}

/* Cache tracking — shared between QOI and RLE */
typedef struct
{
    const void *image_info; /* pointer to the info struct that populated the cache */
    uint16_t row_band_start; /* first image row in the cached band */
    uint16_t row_count;      /* number of rows cached */
    uint8_t mode;            /* row-band cache or whole-image cache */
} egui_image_decode_cache_state_t;

extern egui_image_decode_cache_state_t egui_image_decode_cache_state;

int egui_image_decode_cache_can_hold_full_image(uint16_t img_width, uint16_t img_height, uint8_t bytes_per_pixel, uint16_t alpha_row_bytes);
void egui_image_decode_cache_set_row_band(const void *image_info, uint16_t row_band_start, uint16_t row_count);
void egui_image_decode_cache_set_full_image(const void *image_info, uint16_t row_count);

static inline int egui_image_decode_cache_is_row_band_hit(const void *image_info, uint16_t row_band_start)
{
    return egui_image_decode_cache_state.mode == EGUI_IMAGE_DECODE_CACHE_MODE_ROW_BAND &&
           egui_image_decode_cache_state.image_info == image_info &&
           egui_image_decode_cache_state.row_band_start == row_band_start;
}

static inline int egui_image_decode_cache_is_full_image_hit(const void *image_info)
{
    return egui_image_decode_cache_state.mode == EGUI_IMAGE_DECODE_CACHE_MODE_FULL_IMAGE &&
           egui_image_decode_cache_state.image_info == image_info;
}
#endif /* EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE */

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

extern egui_image_decode_persistent_cache_t egui_image_decode_persistent_cache;

int egui_image_decode_persistent_cache_prepare(uint16_t img_width, uint16_t img_height, uint8_t bytes_per_pixel, uint16_t alpha_row_bytes);
void egui_image_decode_persistent_cache_set_image(const void *image_info);

static inline int egui_image_decode_persistent_cache_is_hit(const void *image_info)
{
    return egui_image_decode_persistent_cache.image_info == image_info;
}

static inline uint8_t *egui_image_decode_persistent_cache_pixel_row(uint16_t row)
{
    return &egui_image_decode_persistent_cache.buffer[(uint32_t)row * egui_image_decode_persistent_cache.width *
                                                      egui_image_decode_persistent_cache.bytes_per_pixel];
}

static inline uint8_t *egui_image_decode_persistent_cache_alpha_row_bytes(uint16_t row)
{
    return &egui_image_decode_persistent_cache.buffer[egui_image_decode_persistent_cache.pixel_bytes +
                                                      (uint32_t)row * egui_image_decode_persistent_cache.alpha_row_bytes];
}
#endif

/**
 * Blend one decoded row into current PFB region.
 *
 * @param img_x      image top-left X position on screen (base view coordinates)
 * @param img_y      image top-left Y position on screen (base view coordinates)
 * @param row        the image row number being blended (0-based)
 * @param img_width  image width in pixels
 * @param data_type  EGUI_IMAGE_DATA_TYPE_RGB565 / RGB32 / GRAY8
 * @param alpha_type EGUI_IMAGE_ALPHA_TYPE_1/2/4/8
 * @param has_alpha  1 if alpha channel present, 0 otherwise
 * @param pixel_buf  decoded pixel data for this full row
 * @param alpha_buf  decoded alpha data for this full row (NULL if no alpha)
 */
int egui_image_decode_get_horizontal_clip(egui_dim_t img_x, uint16_t img_width,
                                          egui_dim_t *screen_x_start, egui_dim_t *img_col_start, egui_dim_t *count);
int egui_image_decode_get_rgb565_dst(egui_dim_t screen_x, egui_dim_t screen_y, egui_color_int_t **dst_row, egui_dim_t *dst_stride);
int egui_image_decode_get_fast_rgb565_dst(egui_dim_t screen_x, egui_dim_t screen_y, egui_color_int_t **dst_row, egui_dim_t *dst_stride);
void egui_image_decode_blend_rgb565_alpha8_row_fast_path(egui_color_int_t *dst, const uint16_t *src_pixels, const uint8_t *src_alpha,
                                                         egui_dim_t count);
void egui_image_decode_blend_row_clipped(egui_dim_t screen_x, egui_dim_t screen_y,
                                         egui_dim_t img_col_start, egui_dim_t count,
                                         uint8_t data_type, uint8_t alpha_type,
                                         int has_alpha,
                                         const uint8_t *pixel_buf, const uint8_t *alpha_buf);
void egui_image_decode_blend_row(egui_dim_t img_x, egui_dim_t img_y, uint16_t row,
                                 uint16_t img_width, uint8_t data_type, uint8_t alpha_type,
                                 int has_alpha,
                                 const uint8_t *pixel_buf, const uint8_t *alpha_buf);

#endif /* EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE || EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_DECODE_UTILS_H_ */
