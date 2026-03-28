#include <string.h>

#include "egui_image_decode_utils.h"
#include "core/egui_api.h"

#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE || EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE

#define EGUI_IMAGE_DECODE_SINGLE_ROW_PIXEL_MAX_BYTES (EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH * EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE)

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
#define EGUI_IMAGE_DECODE_CAPACITY_CAN_USE_16BIT                                                                                                            \
    ((EGUI_IMAGE_DECODE_SINGLE_ROW_PIXEL_MAX_BYTES <= 0xFFFFu) && (EGUI_IMAGE_DECODE_ROW_CACHE_PIXEL_MAX_BYTES <= 0xFFFFu) &&                             \
     (EGUI_IMAGE_DECODE_ROW_CACHE_ALPHA_MAX_BYTES <= 0xFFFFu))
#else
#define EGUI_IMAGE_DECODE_CAPACITY_CAN_USE_16BIT (EGUI_IMAGE_DECODE_SINGLE_ROW_PIXEL_MAX_BYTES <= 0xFFFFu)
#endif

#if EGUI_IMAGE_DECODE_CAPACITY_CAN_USE_16BIT
typedef uint16_t egui_image_decode_capacity_t;
#else
typedef uint32_t egui_image_decode_capacity_t;
#endif

/* Shared row decode buffers */
#if !EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
uint8_t *egui_image_decode_row_pixel_buf = NULL;
static egui_image_decode_capacity_t g_egui_image_decode_row_pixel_buf_capacity = 0;
#endif
#if EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE && !EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
#error "EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE requires EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE"
#endif
#if !EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE || !EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE
uint8_t *egui_image_decode_row_alpha_buf = NULL;
static egui_image_decode_capacity_t g_egui_image_decode_row_alpha_buf_capacity = 0;
#endif

#if !EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
static uint8_t *egui_image_decode_prepare_single_row_pixel_buf(uint8_t bytes_per_pixel)
{
    uint32_t required_bytes;
    uint8_t *new_buf;

    EGUI_ASSERT(bytes_per_pixel > 0 && bytes_per_pixel <= EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE);

    required_bytes = (uint32_t)EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH * bytes_per_pixel;
    if (required_bytes <= g_egui_image_decode_row_pixel_buf_capacity)
    {
        return egui_image_decode_row_pixel_buf;
    }

    new_buf = (uint8_t *)egui_malloc((int)required_bytes);
    if (new_buf == NULL)
    {
        return NULL;
    }

    if (egui_image_decode_row_pixel_buf != NULL)
    {
        egui_free(egui_image_decode_row_pixel_buf);
    }

    egui_image_decode_row_pixel_buf = new_buf;
    g_egui_image_decode_row_pixel_buf_capacity = (egui_image_decode_capacity_t)required_bytes;
    return egui_image_decode_row_pixel_buf;
}
#endif

#if !EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE || !EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE
static uint8_t *egui_image_decode_prepare_single_row_alpha_buf(uint16_t alpha_row_bytes)
{
    uint32_t required_bytes;
    uint8_t *new_buf;

    EGUI_ASSERT(alpha_row_bytes <= EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH);

    required_bytes = alpha_row_bytes;
    if (required_bytes <= g_egui_image_decode_row_alpha_buf_capacity)
    {
        return egui_image_decode_row_alpha_buf;
    }

    new_buf = (uint8_t *)egui_malloc((int)required_bytes);
    if (new_buf == NULL)
    {
        return NULL;
    }

    if (egui_image_decode_row_alpha_buf != NULL)
    {
        egui_free(egui_image_decode_row_alpha_buf);
    }

    egui_image_decode_row_alpha_buf = new_buf;
    g_egui_image_decode_row_alpha_buf_capacity = (egui_image_decode_capacity_t)required_bytes;
    return egui_image_decode_row_alpha_buf;
}
#endif

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
/* Row-band decode cache: frame-local heap scratch sized from the active image row band. */
uint8_t *egui_image_decode_row_cache_pixel = NULL;
uint8_t *egui_image_decode_row_cache_alpha = NULL;
egui_image_decode_cache_state_t egui_image_decode_cache_state = {NULL, 0xFFFF, EGUI_IMAGE_DECODE_CACHE_MODE_NONE};
static egui_image_decode_capacity_t g_egui_image_decode_row_cache_pixel_capacity = 0;
static egui_image_decode_capacity_t g_egui_image_decode_row_cache_alpha_capacity = 0;

static void egui_image_decode_reset_row_cache_state(void)
{
    egui_image_decode_cache_state.image_info = NULL;
    egui_image_decode_cache_state.row_band_start = 0xFFFF;
    egui_image_decode_cache_state.cache_col_start = 0;
    egui_image_decode_cache_state.cache_col_count = 0;
    egui_image_decode_cache_state.mode = EGUI_IMAGE_DECODE_CACHE_MODE_NONE;
}

static uint8_t *egui_image_decode_prepare_single_row_pixel_buf(uint8_t bytes_per_pixel)
{
    uint32_t required_bytes;

    EGUI_ASSERT(bytes_per_pixel > 0 && bytes_per_pixel <= EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE);

    required_bytes = (uint32_t)EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH * bytes_per_pixel;
    if (!egui_image_decode_cache_prepare_bytes(required_bytes, 0))
    {
        return NULL;
    }

    /*
     * Single-row scratch and row-band cache do not share live pixel contents.
     * Reuse the same heap buffer and drop any previous cached-band tag when
     * the scratch path borrows it.
     */
    egui_image_decode_reset_row_cache_state();
    return egui_image_decode_row_cache_pixel;
}

int egui_image_decode_cache_prepare_bytes(uint32_t pixel_bytes, uint32_t alpha_bytes)
{
    uint8_t *new_pixel = egui_image_decode_row_cache_pixel;
    uint8_t *new_alpha = egui_image_decode_row_cache_alpha;

    if (pixel_bytes > EGUI_IMAGE_DECODE_ROW_CACHE_PIXEL_MAX_BYTES || alpha_bytes > EGUI_IMAGE_DECODE_ROW_CACHE_ALPHA_MAX_BYTES)
    {
        return 0;
    }

    if (pixel_bytes > g_egui_image_decode_row_cache_pixel_capacity)
    {
        new_pixel = (uint8_t *)egui_malloc((int)pixel_bytes);
        if (new_pixel == NULL)
        {
            return 0;
        }
    }

    if (alpha_bytes > g_egui_image_decode_row_cache_alpha_capacity)
    {
        new_alpha = (uint8_t *)egui_malloc((int)alpha_bytes);
        if (new_alpha == NULL)
        {
            if (new_pixel != egui_image_decode_row_cache_pixel)
            {
                egui_free(new_pixel);
            }
            return 0;
        }
    }

    if (new_pixel != egui_image_decode_row_cache_pixel)
    {
        if (egui_image_decode_row_cache_pixel != NULL)
        {
            egui_free(egui_image_decode_row_cache_pixel);
        }
        egui_image_decode_row_cache_pixel = new_pixel;
        g_egui_image_decode_row_cache_pixel_capacity = (egui_image_decode_capacity_t)pixel_bytes;
    }

    if (new_alpha != egui_image_decode_row_cache_alpha)
    {
        if (egui_image_decode_row_cache_alpha != NULL)
        {
            egui_free(egui_image_decode_row_cache_alpha);
        }
        egui_image_decode_row_cache_alpha = new_alpha;
        g_egui_image_decode_row_cache_alpha_capacity = (egui_image_decode_capacity_t)alpha_bytes;
    }

    return 1;
}

int egui_image_decode_cache_prepare_rows(uint16_t img_width, uint16_t row_count, uint8_t bytes_per_pixel, uint16_t alpha_row_bytes)
{
    uint32_t pixel_bytes;
    uint32_t alpha_bytes;

    if (img_width == 0 || row_count == 0 || bytes_per_pixel == 0 || bytes_per_pixel > EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE)
    {
        return 0;
    }

    pixel_bytes = (uint32_t)img_width * row_count * bytes_per_pixel;
    alpha_bytes = (uint32_t)row_count * alpha_row_bytes;

#if EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE
    if (alpha_bytes < EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH)
    {
        alpha_bytes = EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH;
    }
#endif

    return egui_image_decode_cache_prepare_bytes(pixel_bytes, alpha_bytes);
}

void egui_image_decode_release_frame_cache(void)
{
#if !EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE || !EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE
    if (egui_image_decode_row_alpha_buf != NULL)
    {
        egui_free(egui_image_decode_row_alpha_buf);
        egui_image_decode_row_alpha_buf = NULL;
    }
    g_egui_image_decode_row_alpha_buf_capacity = 0;
#endif

    if (egui_image_decode_row_cache_pixel != NULL)
    {
        egui_free(egui_image_decode_row_cache_pixel);
        egui_image_decode_row_cache_pixel = NULL;
    }

    if (egui_image_decode_row_cache_alpha != NULL)
    {
        egui_free(egui_image_decode_row_cache_alpha);
        egui_image_decode_row_cache_alpha = NULL;
    }

    g_egui_image_decode_row_cache_pixel_capacity = 0;
    g_egui_image_decode_row_cache_alpha_capacity = 0;
    egui_image_decode_reset_row_cache_state();
}

int egui_image_decode_cache_can_hold_full_image(uint16_t img_width, uint16_t img_height, uint8_t bytes_per_pixel, uint16_t alpha_row_bytes)
{
    uint32_t pixel_bytes = (uint32_t)img_width * img_height * bytes_per_pixel;

    if (pixel_bytes > EGUI_IMAGE_DECODE_ROW_CACHE_PIXEL_MAX_BYTES)
    {
        return 0;
    }

    if (alpha_row_bytes != 0)
    {
        uint32_t alpha_bytes = (uint32_t)img_height * alpha_row_bytes;
        if (alpha_bytes > EGUI_IMAGE_DECODE_ROW_CACHE_ALPHA_MAX_BYTES)
        {
            return 0;
        }
    }

    return 1;
}

void egui_image_decode_cache_set_row_band(const void *image_info, uint16_t row_band_start, uint16_t row_count,
                                          uint16_t cache_col_start, uint16_t cache_col_count)
{
    EGUI_UNUSED(row_count);
    egui_image_decode_cache_state.image_info = image_info;
    egui_image_decode_cache_state.row_band_start = row_band_start;
    egui_image_decode_cache_state.cache_col_start = cache_col_start;
    egui_image_decode_cache_state.cache_col_count = cache_col_count;
    egui_image_decode_cache_state.mode = EGUI_IMAGE_DECODE_CACHE_MODE_ROW_BAND;
}

void egui_image_decode_cache_set_full_image(const void *image_info, uint16_t row_count, uint16_t img_width)
{
    EGUI_UNUSED(row_count);
    egui_image_decode_cache_state.image_info = image_info;
    egui_image_decode_cache_state.row_band_start = 0;
    egui_image_decode_cache_state.cache_col_start = 0;
    egui_image_decode_cache_state.cache_col_count = img_width;
    egui_image_decode_cache_state.mode = EGUI_IMAGE_DECODE_CACHE_MODE_FULL_IMAGE;
}
#else
void egui_image_decode_release_frame_cache(void)
{
    if (egui_image_decode_row_pixel_buf != NULL)
    {
        egui_free(egui_image_decode_row_pixel_buf);
        egui_image_decode_row_pixel_buf = NULL;
    }
    g_egui_image_decode_row_pixel_buf_capacity = 0;

#if !EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE
    if (egui_image_decode_row_alpha_buf != NULL)
    {
        egui_free(egui_image_decode_row_alpha_buf);
        egui_image_decode_row_alpha_buf = NULL;
    }
    g_egui_image_decode_row_alpha_buf_capacity = 0;
#endif
}
#endif

uint8_t *egui_image_decode_get_row_pixel_buf(uint8_t bytes_per_pixel)
{
    return egui_image_decode_prepare_single_row_pixel_buf(bytes_per_pixel);
}

uint8_t *egui_image_decode_get_opaque_alpha_row(egui_dim_t count)
{
    uint8_t *opaque_alpha_row;

    EGUI_ASSERT(count >= 0 && count <= EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH);

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE && EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE
    if (egui_image_decode_row_cache_alpha == NULL && !egui_image_decode_cache_prepare_bytes(0, EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH))
    {
        return NULL;
    }
    opaque_alpha_row = egui_image_decode_row_cache_alpha;
#else
    opaque_alpha_row = egui_image_decode_prepare_single_row_alpha_buf(EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH);
#endif

    if (opaque_alpha_row == NULL)
    {
        return NULL;
    }

    memset(opaque_alpha_row, EGUI_ALPHA_100, (size_t)count);
    return opaque_alpha_row;
}

uint8_t *egui_image_decode_get_row_alpha_scratch(uint16_t alpha_row_bytes)
{
    EGUI_ASSERT(alpha_row_bytes <= EGUI_CONFIG_IMAGE_DECODE_ROW_BUF_WIDTH);

#if !EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE || !EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE
    return egui_image_decode_prepare_single_row_alpha_buf(alpha_row_bytes);
#else
    if (egui_image_decode_row_cache_alpha == NULL && !egui_image_decode_cache_prepare_bytes(0, alpha_row_bytes))
    {
        return NULL;
    }
    return egui_image_decode_row_cache_alpha;
#endif
}

#if EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES > 0
egui_image_decode_persistent_cache_t egui_image_decode_persistent_cache = {0};

int egui_image_decode_persistent_cache_prepare(uint16_t img_width, uint16_t img_height, uint8_t bytes_per_pixel, uint16_t alpha_row_bytes)
{
    uint32_t pixel_bytes = (uint32_t)img_width * img_height * bytes_per_pixel;
    uint32_t alpha_bytes = (uint32_t)img_height * alpha_row_bytes;
    uint32_t total_bytes = pixel_bytes + alpha_bytes;

    if (total_bytes == 0 || total_bytes > EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES)
    {
        return 0;
    }

    if (egui_image_decode_persistent_cache.capacity_bytes < total_bytes)
    {
        uint8_t *new_buffer = (uint8_t *)egui_malloc((int)total_bytes);

        if (new_buffer == NULL)
        {
            return 0;
        }

        if (egui_image_decode_persistent_cache.buffer != NULL)
        {
            egui_free(egui_image_decode_persistent_cache.buffer);
        }

        egui_image_decode_persistent_cache.buffer = new_buffer;
        egui_image_decode_persistent_cache.capacity_bytes = total_bytes;
    }

    egui_image_decode_persistent_cache.image_info = NULL;
    egui_image_decode_persistent_cache.pixel_bytes = pixel_bytes;
    egui_image_decode_persistent_cache.width = img_width;
    egui_image_decode_persistent_cache.height = img_height;
    egui_image_decode_persistent_cache.bytes_per_pixel = bytes_per_pixel;
    egui_image_decode_persistent_cache.alpha_row_bytes = alpha_row_bytes;
    return 1;
}

void egui_image_decode_persistent_cache_set_image(const void *image_info)
{
    egui_image_decode_persistent_cache.image_info = image_info;
}
#endif

#if EGUI_CONFIG_COLOR_DEPTH == 16
__EGUI_STATIC_INLINE__ int egui_image_decode_can_use_rgb565_fast_path(egui_canvas_t *canvas)
{
    return canvas->alpha == EGUI_ALPHA_100
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
           && canvas->mask == NULL
#endif
            ;
}

__EGUI_STATIC_INLINE__ egui_color_int_t *egui_image_decode_get_row_dst(egui_canvas_t *canvas, egui_dim_t screen_x, egui_dim_t screen_y)
{
    egui_dim_t pos_x = screen_x - canvas->pfb_location_in_base_view.x;
    egui_dim_t pos_y = screen_y - canvas->pfb_location_in_base_view.y;

    return &canvas->pfb[pos_y * canvas->pfb_region.size.width + pos_x];
}

__EGUI_STATIC_INLINE__ void egui_image_decode_blend_rgb565_src_pixel(egui_color_int_t *dst, uint16_t src_pixel, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }
    if (alpha > 251)
    {
        *dst = src_pixel;
        return;
    }
    if (alpha < 4)
    {
        return;
    }

    {
        uint16_t bg = *dst;
        uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
        uint32_t fg_rb_g = (src_pixel | ((uint32_t)src_pixel << 16)) & 0x07E0F81FUL;
        uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)alpha >> 3) >> 5)) & 0x07E0F81FUL;

        *dst = (uint16_t)(result | (result >> 16));
    }
}

__EGUI_STATIC_INLINE__ void egui_image_decode_blend_row_rgb565_alpha8_fast(egui_color_int_t *dst, const uint16_t *src_pixels, const uint8_t *src_alpha,
                                                                            egui_dim_t count)
{
    egui_dim_t i = 0;

    while (i < count)
    {
        while (i + 4 <= count && *(const uint32_t *)&src_alpha[i] == 0x00000000U)
        {
            i += 4;
        }
        while (i < count && src_alpha[i] == 0)
        {
            i++;
        }

        {
            egui_dim_t opaque_start = i;

            while (i + 4 <= count && *(const uint32_t *)&src_alpha[i] == 0xFFFFFFFFU)
            {
                i += 4;
            }
            while (i < count && src_alpha[i] == EGUI_ALPHA_100)
            {
                i++;
            }

            if (opaque_start < i)
            {
                memcpy(&dst[opaque_start], &src_pixels[opaque_start], (size_t)(i - opaque_start) * sizeof(uint16_t));
                continue;
            }
        }

        if (i < count)
        {
            egui_alpha_t alpha = src_alpha[i];

            if (alpha != 0)
            {
                egui_image_decode_blend_rgb565_src_pixel(&dst[i], src_pixels[i], alpha);
            }
            i++;
        }
    }
}
#endif

int egui_image_decode_get_horizontal_clip(egui_dim_t img_x, uint16_t img_width,
                                          egui_dim_t *screen_x_start, egui_dim_t *img_col_start, egui_dim_t *count)
{
    egui_region_t *work_region = egui_canvas_get_base_view_work_region();
    egui_dim_t img_x_end = img_x + (egui_dim_t)img_width;
    egui_dim_t col_start = work_region->location.x;
    egui_dim_t col_end = work_region->location.x + work_region->size.width;

    if (col_start < img_x)
    {
        col_start = img_x;
    }
    if (col_end > img_x_end)
    {
        col_end = img_x_end;
    }
    if (col_start >= col_end)
    {
        return 0;
    }

    *screen_x_start = col_start;
    *img_col_start = col_start - img_x;
    *count = col_end - col_start;
    return 1;
}

int egui_image_decode_get_fast_rgb565_dst(egui_dim_t screen_x, egui_dim_t screen_y, egui_color_int_t **dst_row, egui_dim_t *dst_stride)
{
    if (dst_row == NULL || dst_stride == NULL)
    {
        return 0;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
    {
        egui_canvas_t *canvas = egui_canvas_get_canvas();

        if (!egui_image_decode_can_use_rgb565_fast_path(canvas))
        {
            return 0;
        }

        *dst_row = egui_image_decode_get_row_dst(canvas, screen_x, screen_y);
        *dst_stride = canvas->pfb_region.size.width;
        return 1;
    }
#else
    (void)screen_x;
    (void)screen_y;
    return 0;
#endif
}

int egui_image_decode_get_rgb565_dst(egui_dim_t screen_x, egui_dim_t screen_y, egui_color_int_t **dst_row, egui_dim_t *dst_stride)
{
    if (dst_row == NULL || dst_stride == NULL)
    {
        return 0;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
    {
        egui_canvas_t *canvas = egui_canvas_get_canvas();

        if (canvas == NULL)
        {
            return 0;
        }

        *dst_row = egui_image_decode_get_row_dst(canvas, screen_x, screen_y);
        *dst_stride = canvas->pfb_region.size.width;
        return 1;
    }
#else
    (void)screen_x;
    (void)screen_y;
    return 0;
#endif
}

void egui_image_decode_blend_rgb565_alpha8_row_fast_path(egui_color_int_t *dst, const uint16_t *src_pixels, const uint8_t *src_alpha, egui_dim_t count)
{
#if EGUI_CONFIG_COLOR_DEPTH == 16
    egui_image_decode_blend_row_rgb565_alpha8_fast(dst, src_pixels, src_alpha, count);
#else
    for (egui_dim_t i = 0; i < count; i++)
    {
        egui_image_std_blend_rgb565_src_pixel_fast(&dst[i], src_pixels[i], src_alpha[i]);
    }
#endif
}

void egui_image_decode_blend_row_clipped(egui_dim_t screen_x, egui_dim_t screen_y,
                                         egui_dim_t img_col_start, egui_dim_t count,
                                         uint8_t data_type, uint8_t alpha_type,
                                         int has_alpha,
                                         const uint8_t *pixel_buf, const uint8_t *alpha_buf)
{
    if (count <= 0)
    {
        return;
    }

    if (data_type == EGUI_IMAGE_DATA_TYPE_RGB565)
    {
        const uint16_t *src_pixels = (const uint16_t *)pixel_buf + img_col_start;

        if (!has_alpha)
        {
#if EGUI_CONFIG_COLOR_DEPTH == 16
            egui_canvas_t *canvas = egui_canvas_get_canvas();
            if (canvas->mask != NULL)
            {
                egui_color_int_t *dst = egui_image_decode_get_row_dst(canvas, screen_x, screen_y);

                if (egui_image_std_blend_rgb565_masked_row(canvas, dst, src_pixels, count, screen_x, screen_y, canvas->alpha))
                {
                    return;
                }

                {
                    const uint8_t *opaque_alpha_row = egui_image_decode_get_opaque_alpha_row(count);
                    if (opaque_alpha_row == NULL)
                    {
                        return;
                    }
                    egui_image_std_blend_rgb565_alpha8_masked_row(canvas, dst, src_pixels, opaque_alpha_row, count, screen_x, screen_y, canvas->alpha);
                }
                return;
            }
            if (egui_image_decode_can_use_rgb565_fast_path(canvas))
            {
                egui_color_int_t *dst = egui_image_decode_get_row_dst(canvas, screen_x, screen_y);
                memcpy(dst, src_pixels, (size_t)count * sizeof(uint16_t));
            }
            else
#endif /* EGUI_CONFIG_COLOR_DEPTH == 16 */
            {
                for (egui_dim_t i = 0; i < count; i++)
                {
                    egui_color_t color;
                    color.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                    egui_canvas_draw_point_limit(screen_x + i, screen_y, color, EGUI_ALPHA_100);
                }
            }
        }
        else if (alpha_type == EGUI_IMAGE_ALPHA_TYPE_8)
        {
            const uint8_t *src_alpha = alpha_buf + img_col_start;
#if EGUI_CONFIG_COLOR_DEPTH == 16
            egui_canvas_t *canvas = egui_canvas_get_canvas();

            if (canvas->mask != NULL)
            {
                egui_color_int_t *dst = egui_image_decode_get_row_dst(canvas, screen_x, screen_y);
                egui_image_std_blend_rgb565_alpha8_masked_row(canvas, dst, src_pixels, src_alpha, count, screen_x, screen_y, canvas->alpha);
                return;
            }

            if (egui_image_decode_can_use_rgb565_fast_path(canvas))
            {
                egui_color_int_t *dst = egui_image_decode_get_row_dst(canvas, screen_x, screen_y);
                egui_image_decode_blend_row_rgb565_alpha8_fast(dst, src_pixels, src_alpha, count);
                return;
            }
#endif /* EGUI_CONFIG_COLOR_DEPTH == 16 */
            for (egui_dim_t i = 0; i < count; i++)
            {
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                egui_canvas_draw_point_limit(screen_x + i, screen_y, color, src_alpha[i]);
            }
        }
        else if (alpha_type == EGUI_IMAGE_ALPHA_TYPE_4)
        {
            for (egui_dim_t i = 0; i < count; i++)
            {
                egui_dim_t alpha_col = img_col_start + i;
                uint8_t packed = alpha_buf[alpha_col >> 1];
                uint8_t shift = (alpha_col & 1) << 2;
                egui_alpha_t a = egui_alpha_change_table_4[(packed >> shift) & 0x0F];
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                egui_canvas_draw_point_limit(screen_x + i, screen_y, color, a);
            }
        }
        else if (alpha_type == EGUI_IMAGE_ALPHA_TYPE_2)
        {
            for (egui_dim_t i = 0; i < count; i++)
            {
                egui_dim_t alpha_col = img_col_start + i;
                uint8_t packed = alpha_buf[alpha_col >> 2];
                uint8_t shift = (alpha_col & 3) << 1;
                egui_alpha_t a = egui_alpha_change_table_2[(packed >> shift) & 0x03];
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                egui_canvas_draw_point_limit(screen_x + i, screen_y, color, a);
            }
        }
        else
        {
            for (egui_dim_t i = 0; i < count; i++)
            {
                egui_dim_t alpha_col = img_col_start + i;
                uint8_t packed = alpha_buf[alpha_col >> 3];
                uint8_t bit = (alpha_col & 7);
                egui_alpha_t a = ((packed >> bit) & 1) ? EGUI_ALPHA_100 : 0;
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                egui_canvas_draw_point_limit(screen_x + i, screen_y, color, a);
            }
        }
    }
    else if (data_type == EGUI_IMAGE_DATA_TYPE_RGB32)
    {
        const uint8_t *src_pixels = pixel_buf + (uint32_t)img_col_start * 4;

        for (egui_dim_t i = 0; i < count; i++)
        {
            uint8_t r = src_pixels[i * 4 + 0];
            uint8_t g = src_pixels[i * 4 + 1];
            uint8_t b = src_pixels[i * 4 + 2];
            egui_alpha_t a = EGUI_ALPHA_100;

            if (has_alpha)
            {
                a = src_pixels[i * 4 + 3];
            }

            egui_color_t color = EGUI_COLOR_MAKE(r, g, b);
            egui_canvas_draw_point_limit(screen_x + i, screen_y, color, a);
        }
    }
    else if (data_type == EGUI_IMAGE_DATA_TYPE_GRAY8)
    {
        const uint8_t *src_pixels = pixel_buf + img_col_start;

        if (!has_alpha)
        {
            for (egui_dim_t i = 0; i < count; i++)
            {
                uint8_t gray = src_pixels[i];
                egui_color_t color = EGUI_COLOR_MAKE(gray, gray, gray);
                egui_canvas_draw_point_limit(screen_x + i, screen_y, color, EGUI_ALPHA_100);
            }
        }
        else if (alpha_type == EGUI_IMAGE_ALPHA_TYPE_8)
        {
            const uint8_t *src_alpha = alpha_buf + img_col_start;
            for (egui_dim_t i = 0; i < count; i++)
            {
                uint8_t gray = src_pixels[i];
                egui_color_t color = EGUI_COLOR_MAKE(gray, gray, gray);
                egui_canvas_draw_point_limit(screen_x + i, screen_y, color, src_alpha[i]);
            }
        }
    }
}

void egui_image_decode_blend_row(egui_dim_t img_x, egui_dim_t img_y, uint16_t row,
                                 uint16_t img_width, uint8_t data_type, uint8_t alpha_type,
                                 int has_alpha,
                                 const uint8_t *pixel_buf, const uint8_t *alpha_buf)
{
    egui_region_t *work_region = egui_canvas_get_base_view_work_region();

    /* Screen Y for this row */
    egui_dim_t screen_y = img_y + (egui_dim_t)row;

    /* Check if this row is within the work region vertically */
    if (screen_y < work_region->location.y ||
        screen_y >= (work_region->location.y + work_region->size.height))
    {
        return;
    }

    /* Calculate horizontal overlap between image and work region */
    egui_dim_t img_x_end = img_x + (egui_dim_t)img_width;
    egui_dim_t col_start = work_region->location.x;
    egui_dim_t col_end = work_region->location.x + work_region->size.width;

    if (col_start < img_x)
    {
        col_start = img_x;
    }
    if (col_end > img_x_end)
    {
        col_end = img_x_end;
    }
    if (col_start >= col_end)
    {
        return;
    }

    /* Image column offset */
    egui_dim_t img_col_start = col_start - img_x;
    egui_dim_t count = col_end - col_start;

    if (data_type == EGUI_IMAGE_DATA_TYPE_RGB565)
    {
        const uint16_t *src_pixels = (const uint16_t *)pixel_buf + img_col_start;

        if (!has_alpha)
        {
#if EGUI_CONFIG_COLOR_DEPTH == 16
            /* Fast path: opaque RGB565 on 16-bit target — try direct PFB copy */
            egui_canvas_t *canvas = egui_canvas_get_canvas();
            if (canvas->mask != NULL)
            {
                egui_color_int_t *dst = egui_image_decode_get_row_dst(canvas, col_start, screen_y);

                if (egui_image_std_blend_rgb565_masked_row(canvas, dst, src_pixels, count, col_start, screen_y, canvas->alpha))
                {
                    return;
                }

                {
                    const uint8_t *opaque_alpha_row = egui_image_decode_get_opaque_alpha_row(count);
                    if (opaque_alpha_row == NULL)
                    {
                        return;
                    }
                    egui_image_std_blend_rgb565_alpha8_masked_row(canvas, dst, src_pixels, opaque_alpha_row, count, col_start, screen_y, canvas->alpha);
                }
                return;
            }
            if (egui_image_decode_can_use_rgb565_fast_path(canvas))
            {
                egui_color_int_t *dst = egui_image_decode_get_row_dst(canvas, col_start, screen_y);
                memcpy(dst, src_pixels, (size_t)count * sizeof(uint16_t));
            }
            else
#endif /* EGUI_CONFIG_COLOR_DEPTH == 16 */
            {
                /* Opaque RGB565: per-pixel with mask/alpha support */
                for (egui_dim_t i = 0; i < count; i++)
                {
                    egui_color_t color;
                    color.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                    egui_canvas_draw_point_limit(col_start + i, screen_y, color, EGUI_ALPHA_100);
                }
            }
        }
        else if (alpha_type == EGUI_IMAGE_ALPHA_TYPE_8)
        {
            const uint8_t *src_alpha = alpha_buf + img_col_start;
#if EGUI_CONFIG_COLOR_DEPTH == 16
            egui_canvas_t *canvas = egui_canvas_get_canvas();

            if (canvas->mask != NULL)
            {
                egui_color_int_t *dst = egui_image_decode_get_row_dst(canvas, col_start, screen_y);
                egui_image_std_blend_rgb565_alpha8_masked_row(canvas, dst, src_pixels, src_alpha, count, col_start, screen_y, canvas->alpha);
                return;
            }

            if (egui_image_decode_can_use_rgb565_fast_path(canvas))
            {
                egui_color_int_t *dst = egui_image_decode_get_row_dst(canvas, col_start, screen_y);
                egui_image_decode_blend_row_rgb565_alpha8_fast(dst, src_pixels, src_alpha, count);
                return;
            }
#endif /* EGUI_CONFIG_COLOR_DEPTH == 16 */
            for (egui_dim_t i = 0; i < count; i++)
            {
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                egui_canvas_draw_point_limit(col_start + i, screen_y, color, src_alpha[i]);
            }
        }
        else if (alpha_type == EGUI_IMAGE_ALPHA_TYPE_4)
        {
            for (egui_dim_t i = 0; i < count; i++)
            {
                egui_dim_t alpha_col = img_col_start + i;
                uint8_t packed = alpha_buf[alpha_col >> 1];
                uint8_t shift = (alpha_col & 1) << 2;
                egui_alpha_t a = egui_alpha_change_table_4[(packed >> shift) & 0x0F];
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                egui_canvas_draw_point_limit(col_start + i, screen_y, color, a);
            }
        }
        else if (alpha_type == EGUI_IMAGE_ALPHA_TYPE_2)
        {
            for (egui_dim_t i = 0; i < count; i++)
            {
                egui_dim_t alpha_col = img_col_start + i;
                uint8_t packed = alpha_buf[alpha_col >> 2];
                uint8_t shift = (alpha_col & 3) << 1;
                egui_alpha_t a = egui_alpha_change_table_2[(packed >> shift) & 0x03];
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                egui_canvas_draw_point_limit(col_start + i, screen_y, color, a);
            }
        }
        else /* EGUI_IMAGE_ALPHA_TYPE_1 */
        {
            for (egui_dim_t i = 0; i < count; i++)
            {
                egui_dim_t alpha_col = img_col_start + i;
                uint8_t packed = alpha_buf[alpha_col >> 3];
                uint8_t bit = (alpha_col & 7);
                egui_alpha_t a = ((packed >> bit) & 1) ? EGUI_ALPHA_100 : 0;
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                egui_canvas_draw_point_limit(col_start + i, screen_y, color, a);
            }
        }
    }
    else if (data_type == EGUI_IMAGE_DATA_TYPE_RGB32)
    {
        /* RGB32: 4 bytes per pixel (R, G, B, A or R, G, B, padding) */
        const uint8_t *src_pixels = pixel_buf + (uint32_t)img_col_start * 4;

        for (egui_dim_t i = 0; i < count; i++)
        {
            uint8_t r = src_pixels[i * 4 + 0];
            uint8_t g = src_pixels[i * 4 + 1];
            uint8_t b = src_pixels[i * 4 + 2];
            egui_alpha_t a = EGUI_ALPHA_100;

            if (has_alpha)
            {
                /* For RGB32 with alpha, the alpha is embedded in pixel data byte 3 */
                a = src_pixels[i * 4 + 3];
            }

            egui_color_t color = EGUI_COLOR_MAKE(r, g, b);
            egui_canvas_draw_point_limit(col_start + i, screen_y, color, a);
        }
    }
    else if (data_type == EGUI_IMAGE_DATA_TYPE_GRAY8)
    {
        /* GRAY8: 1 byte per pixel */
        const uint8_t *src_pixels = pixel_buf + img_col_start;

        if (!has_alpha)
        {
            for (egui_dim_t i = 0; i < count; i++)
            {
                uint8_t gray = src_pixels[i];
                egui_color_t color = EGUI_COLOR_MAKE(gray, gray, gray);
                egui_canvas_draw_point_limit(col_start + i, screen_y, color, EGUI_ALPHA_100);
            }
        }
        else if (alpha_type == EGUI_IMAGE_ALPHA_TYPE_8)
        {
            const uint8_t *src_alpha = alpha_buf + img_col_start;
            for (egui_dim_t i = 0; i < count; i++)
            {
                uint8_t gray = src_pixels[i];
                egui_color_t color = EGUI_COLOR_MAKE(gray, gray, gray);
                egui_canvas_draw_point_limit(col_start + i, screen_y, color, src_alpha[i]);
            }
        }
    }
}

#endif /* EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE || EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE */

#if !EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE && !EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE
void egui_image_decode_release_frame_cache(void)
{
}
#endif
