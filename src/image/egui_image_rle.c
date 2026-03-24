#include <string.h>
#include <stdio.h>

#include "egui_image_rle.h"
#include "egui_image_decode_utils.h"
#include "core/egui_api.h"

#if EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE

/*
 * RLE control byte format (compatible with LVGL RLE):
 *   bit7 = 1: literal mode, count = ctrl & 0x7F, copy count * blk_size raw bytes
 *   bit7 = 0: repeat mode, count = ctrl, repeat next blk_size bytes count times
 */

/* Persistent decode state for PFB tile rendering */
typedef struct
{
    const egui_image_rle_info_t *info;
    uint32_t data_pos;    /* current read position in data stream */
    uint32_t alpha_pos;   /* current read position in alpha stream */
    uint16_t current_row; /* next row to be decoded */
} egui_image_rle_decode_state_t;

static egui_image_rle_decode_state_t rle_state;

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
typedef struct
{
    const egui_image_rle_info_t *info;
    const void *data_addr;
    const void *alpha_addr;
    uint32_t data_size;
    uint32_t alpha_size;
    uint8_t *data_buf;
    uint8_t *alpha_buf;
} egui_image_rle_external_cache_t;

static egui_image_rle_external_cache_t rle_external_cache;

static void egui_image_rle_release_external_cache(void)
{
    if (rle_external_cache.data_buf != NULL)
    {
        egui_free(rle_external_cache.data_buf);
    }
    if (rle_external_cache.alpha_buf != NULL)
    {
        egui_free(rle_external_cache.alpha_buf);
    }
    memset(&rle_external_cache, 0, sizeof(rle_external_cache));
}
#endif

static int egui_image_rle_prepare_decode_info(const egui_image_rle_info_t *info, egui_image_rle_info_t *decode_info)
{
    *decode_info = *info;

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        int has_alpha = (info->alpha_buf != NULL);
        int data_hit = rle_external_cache.info == info &&
                       rle_external_cache.data_addr == info->data_buf &&
                       rle_external_cache.data_size == info->data_size &&
                       rle_external_cache.data_buf != NULL;
        int alpha_hit = !has_alpha ||
                        (rle_external_cache.info == info &&
                         rle_external_cache.alpha_addr == info->alpha_buf &&
                         rle_external_cache.alpha_size == info->alpha_size &&
                         rle_external_cache.alpha_buf != NULL);

        if (!data_hit || !alpha_hit)
        {
            uint8_t *new_data_buf = (uint8_t *)egui_malloc((int)info->data_size);
            uint8_t *new_alpha_buf = NULL;

            if (new_data_buf == NULL)
            {
                return 0;
            }

            egui_api_load_external_resource(new_data_buf, (egui_uintptr_t)(info->data_buf), 0, info->data_size);

            if (has_alpha)
            {
                new_alpha_buf = (uint8_t *)egui_malloc((int)info->alpha_size);
                if (new_alpha_buf == NULL)
                {
                    egui_free(new_data_buf);
                    return 0;
                }

                egui_api_load_external_resource(new_alpha_buf, (egui_uintptr_t)(info->alpha_buf), 0, info->alpha_size);
            }

            egui_image_rle_release_external_cache();

            rle_external_cache.info = info;
            rle_external_cache.data_addr = info->data_buf;
            rle_external_cache.alpha_addr = info->alpha_buf;
            rle_external_cache.data_size = info->data_size;
            rle_external_cache.alpha_size = info->alpha_size;
            rle_external_cache.data_buf = new_data_buf;
            rle_external_cache.alpha_buf = new_alpha_buf;
        }

        decode_info->data_buf = rle_external_cache.data_buf;
        decode_info->alpha_buf = rle_external_cache.alpha_buf;
        decode_info->res_type = EGUI_RESOURCE_TYPE_INTERNAL;
    }
#else
    if (info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        EGUI_ASSERT(0);
        return 0;
    }
#endif

    return 1;
}

/**
 * Decompress exactly `pixels` pixels from RLE stream into output buffer.
 * Returns the new read position in the source stream.
 */
static uint32_t egui_image_rle_decompress_row(const uint8_t *src, uint32_t src_len,
                                              uint32_t src_offset,
                                              uint8_t *dst, uint16_t pixels,
                                              uint8_t blk_size)
{
    uint32_t rd = src_offset;
    uint32_t pixels_written = 0;

    while (pixels_written < pixels && rd < src_len)
    {
        uint8_t ctrl = src[rd++];

        if (ctrl & 0x80)
        {
            /* Literal: copy (ctrl & 0x7F) blocks */
            uint32_t count = ctrl & 0x7F;
            uint32_t bytes = count * blk_size;

            if (rd + bytes > src_len)
            {
                break;
            }

            /* Only copy what we need */
            uint32_t pixels_to_copy = count;
            if (pixels_written + pixels_to_copy > pixels)
            {
                pixels_to_copy = pixels - pixels_written;
            }

            if (dst != NULL)
            {
                memcpy(dst + pixels_written * blk_size, &src[rd], pixels_to_copy * blk_size);
            }
            rd += bytes;
            pixels_written += count;
        }
        else
        {
            /* Repeat: repeat next blk_size bytes ctrl times */
            uint32_t count = ctrl;

            if (rd + blk_size > src_len)
            {
                break;
            }

            uint32_t pixels_to_fill = count;
            if (pixels_written + pixels_to_fill > pixels)
            {
                pixels_to_fill = pixels - pixels_written;
            }

            if (dst != NULL)
            {
                if (blk_size == 1)
                {
                    memset(dst + pixels_written, src[rd], pixels_to_fill);
                }
                else if (blk_size == 2)
                {
                    uint16_t val;
                    memcpy(&val, &src[rd], 2);
                    uint16_t *out = (uint16_t *)(dst + pixels_written * 2);
                    for (uint32_t i = 0; i < pixels_to_fill; i++)
                    {
                        out[i] = val;
                    }
                }
                else
                {
                    for (uint32_t i = 0; i < pixels_to_fill; i++)
                    {
                        memcpy(dst + (pixels_written + i) * blk_size, &src[rd], blk_size);
                    }
                }
            }
            rd += blk_size;
            pixels_written += count;
        }
    }

    return rd;
}

/**
 * Get the block size for pixel data based on data_type.
 */
static uint8_t egui_image_rle_get_data_blk_size(uint8_t data_type)
{
    switch (data_type)
    {
    case EGUI_IMAGE_DATA_TYPE_RGB32:
        return 4;
    case EGUI_IMAGE_DATA_TYPE_RGB565:
        return 2;
    case EGUI_IMAGE_DATA_TYPE_GRAY8:
        return 1;
    default:
        return 1;
    }
}

/**
 * Get the block size for alpha data based on alpha_type.
 * For packed alpha (1/2/4-bit), the RLE stream operates on packed bytes.
 */
static uint8_t egui_image_rle_get_alpha_blk_size(uint8_t alpha_type)
{
    /* Alpha is always stored as 1 byte per unit in RLE stream */
    return 1;
}

/**
 * Get number of alpha bytes per row based on alpha_type and width.
 */
static uint16_t egui_image_rle_get_alpha_row_bytes(uint8_t alpha_type, uint16_t width)
{
    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_8:
        return width;
    case EGUI_IMAGE_ALPHA_TYPE_4:
        return (width + 1) >> 1;
    case EGUI_IMAGE_ALPHA_TYPE_2:
        return (width + 3) >> 2;
    case EGUI_IMAGE_ALPHA_TYPE_1:
        return (width + 7) >> 3;
    default:
        return width;
    }
}

static void egui_image_rle_reset_state(const egui_image_rle_info_t *info)
{
    rle_state.info = info;
    rle_state.data_pos = 0;
    rle_state.alpha_pos = 0;
    rle_state.current_row = 0;
}

/*
 * Row-band checkpoint: saves decoder state at the start of a PFB row band
 * so horizontal tile neighbors can restore instead of re-scanning from the
 * beginning.  RLE state is small (~10 bytes), so checkpoint is cheap.
 */
static egui_image_rle_decode_state_t rle_checkpoint;
static uint16_t rle_checkpoint_row;
static const egui_image_rle_info_t *rle_checkpoint_info;

static void egui_image_rle_save_checkpoint(const egui_image_rle_info_t *info, uint16_t row)
{
    rle_checkpoint = rle_state;
    rle_checkpoint_row = row;
    rle_checkpoint_info = info;
}

static int egui_image_rle_restore_checkpoint(const egui_image_rle_info_t *info, uint16_t target_row)
{
    if (rle_checkpoint_info == info && rle_checkpoint_row == target_row)
    {
        rle_state = rle_checkpoint;
        return 1;
    }
    return 0;
}

static int egui_image_rle_get_point(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    /* Compressed images don't support efficient random access get_point.
     * Return default values. */
    color->full = 0;
    *alpha = 0;
    return 0;
}

static int egui_image_rle_get_point_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y,
                                           egui_dim_t width, egui_dim_t height,
                                           egui_color_t *color, egui_alpha_t *alpha)
{
    /* Resize not supported for compressed images */
    color->full = 0;
    *alpha = 0;
    return 0;
}

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
static void egui_image_rle_blend_cached_rows(const egui_image_rle_info_t *info, egui_dim_t y,
                                             egui_dim_t screen_x_start, egui_dim_t img_col_start, egui_dim_t count,
                                             egui_dim_t img_y_start, egui_dim_t img_y_end,
                                             egui_dim_t cache_row_start, uint8_t data_blk_size, uint16_t alpha_row_bytes, int has_alpha)
{
    egui_dim_t screen_y = y + img_y_start;

    for (egui_dim_t row = img_y_start; row < img_y_end; row++)
    {
        uint16_t row_in_cache = (uint16_t)(row - cache_row_start);
        const uint8_t *pixel_buf = egui_image_decode_cache_pixel_row(row_in_cache, info->width, data_blk_size);
        const uint8_t *alpha_buf = has_alpha ? egui_image_decode_cache_alpha_row_bytes(row_in_cache, alpha_row_bytes) : NULL;

        egui_image_decode_blend_row_clipped(screen_x_start, screen_y, img_col_start, count,
                                            info->data_type, info->alpha_type, has_alpha, pixel_buf, alpha_buf);
        screen_y++;
    }
}
#endif

#if EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES > 0
static void egui_image_rle_blend_persistent_cached_rows(const egui_image_rle_info_t *info, egui_dim_t y,
                                                        egui_dim_t screen_x_start, egui_dim_t img_col_start, egui_dim_t count,
                                                        egui_dim_t img_y_start, egui_dim_t img_y_end, uint16_t alpha_row_bytes, int has_alpha)
{
    egui_dim_t screen_y = y + img_y_start;

    for (egui_dim_t row = img_y_start; row < img_y_end; row++)
    {
        const uint8_t *pixel_buf = egui_image_decode_persistent_cache_pixel_row((uint16_t)row);
        const uint8_t *alpha_buf = has_alpha ? egui_image_decode_persistent_cache_alpha_row_bytes((uint16_t)row) : NULL;

        egui_image_decode_blend_row_clipped(screen_x_start, screen_y, img_col_start, count,
                                            info->data_type, info->alpha_type, has_alpha, pixel_buf, alpha_buf);
        screen_y++;
    }
    (void)alpha_row_bytes;
}
#endif

static void egui_image_rle_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y)
{
    const egui_image_rle_info_t *info = (const egui_image_rle_info_t *)self->res;
    egui_image_rle_info_t decode_info;
    const egui_image_rle_info_t *draw_info;

    if (info == NULL || info->data_buf == NULL)
    {
        return;
    }

    if (!egui_image_rle_prepare_decode_info(info, &decode_info))
    {
        return;
    }
    draw_info = &decode_info;

    /* Check if info changed or state needs reset (new frame) */
    if (rle_state.info != info || rle_state.current_row > info->height)
    {
        egui_image_rle_reset_state(info);
    }

    egui_region_t *work_region = egui_canvas_get_base_view_work_region();

    /* Calculate which rows of the image overlap with the current PFB tile */
    egui_dim_t img_y_start = work_region->location.y - y;
    egui_dim_t img_y_end = img_y_start + work_region->size.height;

    if (img_y_start < 0)
    {
        img_y_start = 0;
    }
    if (img_y_end > draw_info->height)
    {
        img_y_end = draw_info->height;
    }
    if (img_y_start >= img_y_end)
    {
        return;
    }

    egui_dim_t screen_x_start;
    egui_dim_t img_col_start;
    egui_dim_t count;
    uint8_t data_blk_size = egui_image_rle_get_data_blk_size(draw_info->data_type);
    int has_alpha = (draw_info->alpha_buf != NULL);
    uint8_t alpha_blk_size = egui_image_rle_get_alpha_blk_size(draw_info->alpha_type);
    uint16_t alpha_row_bytes = has_alpha ? egui_image_rle_get_alpha_row_bytes(draw_info->alpha_type, draw_info->width) : 0;

    if (!egui_image_decode_get_horizontal_clip(x, draw_info->width, &screen_x_start, &img_col_start, &count))
    {
        return;
    }

#if EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES > 0
    if (egui_image_decode_persistent_cache_is_hit((const void *)info))
    {
        egui_image_rle_blend_persistent_cached_rows(draw_info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end, alpha_row_bytes, has_alpha);
        return;
    }

    if (egui_image_decode_persistent_cache_prepare(draw_info->width, draw_info->height, data_blk_size, alpha_row_bytes))
    {
        egui_image_rle_reset_state(info);

        for (egui_dim_t row = 0; row < draw_info->height; row++)
        {
            uint8_t *pixel_buf = egui_image_decode_persistent_cache_pixel_row((uint16_t)row);
            uint8_t *alpha_buf = has_alpha ? egui_image_decode_persistent_cache_alpha_row_bytes((uint16_t)row) : NULL;

            rle_state.data_pos = egui_image_rle_decompress_row(
                    draw_info->data_buf, draw_info->data_size, rle_state.data_pos,
                    pixel_buf, draw_info->width, data_blk_size);

            if (has_alpha)
            {
                rle_state.alpha_pos = egui_image_rle_decompress_row(
                        draw_info->alpha_buf, draw_info->alpha_size, rle_state.alpha_pos,
                        alpha_buf, alpha_row_bytes, alpha_blk_size);
            }

            rle_state.current_row++;
        }

        egui_image_decode_persistent_cache_set_image((const void *)info);
        egui_image_rle_blend_persistent_cached_rows(draw_info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end, alpha_row_bytes, has_alpha);
        return;
    }
#endif

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
    if (egui_image_decode_cache_is_full_image_hit((const void *)info))
    {
        egui_image_rle_blend_cached_rows(info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end, 0, data_blk_size, alpha_row_bytes, has_alpha);
        return;
    }

    if (egui_image_decode_cache_can_hold_full_image(draw_info->width, draw_info->height, data_blk_size, alpha_row_bytes))
    {
        egui_image_rle_reset_state(info);

        for (egui_dim_t row = 0; row < draw_info->height; row++)
        {
            uint8_t *pixel_buf = egui_image_decode_cache_pixel_row((uint16_t)row, draw_info->width, data_blk_size);
            uint8_t *alpha_buf = has_alpha ? egui_image_decode_cache_alpha_row_bytes((uint16_t)row, alpha_row_bytes) : NULL;

            rle_state.data_pos = egui_image_rle_decompress_row(
                    draw_info->data_buf, draw_info->data_size, rle_state.data_pos,
                    pixel_buf, draw_info->width, data_blk_size);

            if (has_alpha)
            {
                rle_state.alpha_pos = egui_image_rle_decompress_row(
                        draw_info->alpha_buf, draw_info->alpha_size, rle_state.alpha_pos,
                        alpha_buf, alpha_row_bytes, alpha_blk_size);
            }

            rle_state.current_row++;
        }

        egui_image_decode_cache_set_full_image((const void *)info, draw_info->height);
        egui_image_rle_blend_cached_rows(info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end, 0, data_blk_size, alpha_row_bytes, has_alpha);
        return;
    }

    /* Row-band cache: check if this row band is already cached */
    if (egui_image_decode_cache_is_row_band_hit((const void *)info, (uint16_t)img_y_start))
    {
        /* Cache hit — blend directly from cached rows without decoding */
        egui_image_rle_blend_cached_rows(info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end, img_y_start, data_blk_size,
                                         alpha_row_bytes, has_alpha);
        return;
    }
#endif /* EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE */

    /* If PFB requests rows before current state, try checkpoint restore */
    if ((uint16_t)img_y_start < rle_state.current_row)
    {
        if (!egui_image_rle_restore_checkpoint(info, (uint16_t)img_y_start))
        {
            egui_image_rle_reset_state(info);
        }
    }

    /* Skip rows to reach img_y_start */
    while (rle_state.current_row < (uint16_t)img_y_start)
    {
        rle_state.data_pos = egui_image_rle_decompress_row(
            draw_info->data_buf, draw_info->data_size, rle_state.data_pos,
            NULL, draw_info->width, data_blk_size);

        if (has_alpha)
        {
            rle_state.alpha_pos = egui_image_rle_decompress_row(
                draw_info->alpha_buf, draw_info->alpha_size, rle_state.alpha_pos,
                NULL, alpha_row_bytes, alpha_blk_size);
        }
        rle_state.current_row++;
    }

    /* Save checkpoint at the start of this row band so horizontal tile
     * neighbors can restore directly instead of re-scanning from row 0. */
    if (rle_checkpoint_info != info || rle_checkpoint_row != (uint16_t)img_y_start)
    {
        egui_image_rle_save_checkpoint(info, (uint16_t)img_y_start);
    }

    /* Decode and blend visible rows */
    egui_dim_t screen_y = y + img_y_start;
    for (egui_dim_t row = img_y_start; row < img_y_end; row++)
    {
#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
        uint16_t row_in_band = (uint16_t)row - (uint16_t)img_y_start;
        uint8_t *pixel_buf = egui_image_decode_cache_pixel_row(row_in_band, draw_info->width, data_blk_size);
        uint8_t *alpha_buf = has_alpha ?
            egui_image_decode_cache_alpha_row_bytes(row_in_band, alpha_row_bytes) : NULL;
#else
        uint8_t *pixel_buf = egui_image_decode_row_pixel_buf;
        uint8_t *alpha_buf = has_alpha ? egui_image_decode_row_alpha_buf : NULL;
#endif
        /* Decode pixel data */
        rle_state.data_pos = egui_image_rle_decompress_row(
            draw_info->data_buf, draw_info->data_size, rle_state.data_pos,
            pixel_buf, draw_info->width, data_blk_size);

        /* Decode alpha data if present */
        if (has_alpha)
        {
            rle_state.alpha_pos = egui_image_rle_decompress_row(
                draw_info->alpha_buf, draw_info->alpha_size, rle_state.alpha_pos,
                alpha_buf, alpha_row_bytes, alpha_blk_size);
        }

        rle_state.current_row++;

        /* Blend into PFB */
        egui_image_decode_blend_row_clipped(screen_x_start, screen_y, img_col_start, count,
                                            draw_info->data_type, draw_info->alpha_type, has_alpha, pixel_buf, alpha_buf);
        screen_y++;
    }

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
    /* Mark this row band as cached for subsequent horizontal tiles */
    egui_image_decode_cache_set_row_band((const void *)info, (uint16_t)img_y_start, (uint16_t)(img_y_end - img_y_start));
#endif
}

static void egui_image_rle_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y,
                                             egui_dim_t width, egui_dim_t height)
{
    /* Resize not supported for compressed images */
    (void)self;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

const egui_image_api_t egui_image_rle_t_api_table = {
    .get_point = egui_image_rle_get_point,
    .get_point_resize = egui_image_rle_get_point_resize,
    .draw_image = egui_image_rle_draw_image,
    .draw_image_resize = egui_image_rle_draw_image_resize,
};

void egui_image_rle_init(egui_image_t *self, const void *res)
{
    /* Call base init */
    egui_image_init(self, res);

    /* Update vtable to RLE implementation */
    self->api = &egui_image_rle_t_api_table;
}

#endif /* EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE */
