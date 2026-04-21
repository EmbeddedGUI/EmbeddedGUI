#ifndef _EGUI_IMAGE_DECODE_UTILS_H_
#define _EGUI_IMAGE_DECODE_UTILS_H_

#include "core/egui_api.h"
#include "core/egui_core.h"
#include "egui_image.h"
#include "egui_image_std.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

void egui_image_decode_release_frame_cache(egui_core_t *core);

#if EGUI_CONFIG_FUNCTION_IMAGE_CODEC_QOI || EGUI_CONFIG_FUNCTION_IMAGE_CODEC_RLE

/* Shared row decode buffers.
 * Pixel buffer: EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH * EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE bytes.
 * With row-band cache enabled, the single-row pixel scratch borrows the same
 * heap-backed pixel buffer instead of keeping a second persistent handle.
 * Alpha buffer: EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH bytes.
 */
uint8_t *egui_image_decode_get_row_pixel_buf(egui_core_t *core, uint8_t bytes_per_pixel);
uint8_t *egui_image_decode_get_opaque_alpha_row(egui_core_t *core, egui_dim_t count);
uint8_t *egui_image_decode_get_row_alpha_scratch(egui_core_t *core, uint16_t alpha_row_bytes);

#if EGUI_IMAGE_CODEC_FAST_DRAW_ROW_CACHE_ENABLED
/*
 * Row-band decode cache: PFB_HEIGHT rows of decoded pixel + alpha data.
 * Depending on the caller, the cache may hold the whole row band or only the
 * horizontal tail that later tiles still need.
 */
int egui_image_decode_cache_prepare_bytes(egui_core_t *core, uint32_t pixel_bytes, uint32_t alpha_bytes);
int egui_image_decode_cache_prepare_rows(egui_core_t *core, uint16_t img_width, uint16_t row_count, uint8_t bytes_per_pixel, uint16_t alpha_row_bytes);

static inline uint16_t egui_image_decode_limit_tail_cache_cols(uint16_t cache_col_count)
{
    return cache_col_count;
}

/* Return pointer to the pixel cache buffer for a given row offset within the band */
static inline uint8_t *egui_image_decode_cache_pixel_row(egui_core_t *core, uint16_t row_in_band, uint16_t img_width, uint8_t bytes_per_pixel)
{
    EGUI_ASSERT(bytes_per_pixel > 0 && bytes_per_pixel <= EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE);
    EGUI_ASSERT(core->image.image_decode_cache.row_cache_pixel != NULL);
    return &core->image.image_decode_cache.row_cache_pixel[(uint32_t)row_in_band * img_width * bytes_per_pixel];
}

/* Return pointer to the alpha cache buffer for a given row offset within the band */
static inline uint8_t *egui_image_decode_cache_alpha_row(egui_core_t *core, uint16_t row_in_band, uint16_t img_width)
{
    EGUI_ASSERT(core->image.image_decode_cache.row_cache_alpha != NULL);
    return &core->image.image_decode_cache.row_cache_alpha[(uint32_t)row_in_band * img_width];
}

static inline uint8_t *egui_image_decode_cache_alpha_row_bytes(egui_core_t *core, uint16_t row_in_band, uint16_t alpha_row_bytes)
{
    EGUI_ASSERT(core->image.image_decode_cache.row_cache_alpha != NULL);
    return &core->image.image_decode_cache.row_cache_alpha[(uint32_t)row_in_band * alpha_row_bytes];
}

int egui_image_decode_cache_can_hold_full_image(egui_core_t *core, uint16_t img_width, uint16_t img_height, uint8_t bytes_per_pixel, uint16_t alpha_row_bytes);
void egui_image_decode_cache_set_row_band(egui_core_t *core, const void *image_info, uint16_t row_band_start, uint16_t row_count, uint16_t cache_col_start,
                                          uint16_t cache_col_count);
void egui_image_decode_cache_set_full_image(egui_core_t *core, const void *image_info, uint16_t row_count, uint16_t img_width);

static inline uint16_t egui_image_decode_cache_col_start(egui_core_t *core)
{
    return core->image.image_decode_cache.cache_state.cache_col_start;
}

static inline uint16_t egui_image_decode_cache_row_width(egui_core_t *core)
{
    return core->image.image_decode_cache.cache_state.cache_col_count;
}

static inline int egui_image_decode_cache_is_row_band_hit(egui_core_t *core, const void *image_info, uint16_t row_band_start, uint16_t img_col_start,
                                                          uint16_t count)
{
    uint32_t cache_col_end = (uint32_t)core->image.image_decode_cache.cache_state.cache_col_start + core->image.image_decode_cache.cache_state.cache_col_count;
    uint32_t request_col_end = (uint32_t)img_col_start + count;

    return core->image.image_decode_cache.cache_state.mode == EGUI_IMAGE_DECODE_CACHE_MODE_ROW_BAND &&
           core->image.image_decode_cache.cache_state.image_info == image_info && core->image.image_decode_cache.cache_state.row_band_start == row_band_start &&
           img_col_start >= core->image.image_decode_cache.cache_state.cache_col_start && request_col_end <= cache_col_end;
}

static inline int egui_image_decode_cache_is_full_image_hit(egui_core_t *core, const void *image_info)
{
    return core->image.image_decode_cache.cache_state.mode == EGUI_IMAGE_DECODE_CACHE_MODE_FULL_IMAGE &&
           core->image.image_decode_cache.cache_state.image_info == image_info;
}
#endif /* EGUI_IMAGE_CODEC_FAST_DRAW_ROW_CACHE_ENABLED */

#if EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES > 0
int egui_image_decode_persistent_cache_prepare(egui_core_t *core, uint16_t img_width, uint16_t img_height, uint8_t bytes_per_pixel, uint16_t alpha_row_bytes);
void egui_image_decode_persistent_cache_set_image(egui_core_t *core, const void *image_info);

static inline int egui_image_decode_persistent_cache_is_hit(egui_core_t *core, const void *image_info)
{
    return core->image.image_decode_cache.persistent_cache.image_info == image_info;
}

static inline uint8_t *egui_image_decode_persistent_cache_pixel_row(egui_core_t *core, uint16_t row)
{
    return &core->image.image_decode_cache.persistent_cache.buffer[(uint32_t)row * core->image.image_decode_cache.persistent_cache.width *
                                                                   core->image.image_decode_cache.persistent_cache.bytes_per_pixel];
}

static inline uint8_t *egui_image_decode_persistent_cache_alpha_row_bytes(egui_core_t *core, uint16_t row)
{
    return &core->image.image_decode_cache.persistent_cache.buffer[core->image.image_decode_cache.persistent_cache.pixel_bytes +
                                                                   (uint32_t)row * core->image.image_decode_cache.persistent_cache.alpha_row_bytes];
}
#endif

int egui_image_decode_get_horizontal_clip(egui_canvas_t *canvas, egui_dim_t img_x, uint16_t img_width, egui_dim_t *screen_x_start, egui_dim_t *img_col_start,
                                          egui_dim_t *count);
int egui_image_decode_get_rgb565_dst(egui_canvas_t *canvas, egui_dim_t screen_x, egui_dim_t screen_y, egui_color_int_t **dst_row, egui_dim_t *dst_stride);
int egui_image_decode_get_fast_rgb565_dst(egui_canvas_t *canvas, egui_dim_t screen_x, egui_dim_t screen_y, egui_color_int_t **dst_row, egui_dim_t *dst_stride);
void egui_image_decode_blend_rgb565_alpha8_row_fast_path(egui_color_int_t *dst, const uint16_t *src_pixels, const uint8_t *src_alpha, egui_dim_t count);
void egui_image_decode_blend_row_clipped(egui_canvas_t *canvas, egui_dim_t screen_x, egui_dim_t screen_y, egui_dim_t img_col_start, egui_dim_t count,
                                         uint8_t data_type, uint8_t alpha_type, int has_alpha, const uint8_t *pixel_buf, const uint8_t *alpha_buf);
void egui_image_decode_blend_row(egui_canvas_t *canvas, egui_dim_t img_x, egui_dim_t img_y, uint16_t row, uint16_t img_width, uint8_t data_type,
                                 uint8_t alpha_type, int has_alpha, const uint8_t *pixel_buf, const uint8_t *alpha_buf);

#endif /* EGUI_CONFIG_FUNCTION_IMAGE_CODEC_QOI || EGUI_CONFIG_FUNCTION_IMAGE_CODEC_RLE */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_DECODE_UTILS_H_ */
