#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_image_std.h"
#include "core/egui_api.h"
#include "mask/egui_mask_circle.h"
#include "mask/egui_mask_round_rectangle.h"
#include "mask/egui_mask_image.h"

const uint8_t egui_image_data_type_size_table[] = {
        4, /* EGUI_IMAGE_DATA_TYPE_RGB32 */
        2, /* EGUI_IMAGE_DATA_TYPE_RGB565 */
        1, /* EGUI_IMAGE_DATA_TYPE_GRAY8 */
        0, /* EGUI_IMAGE_DATA_TYPE_ALPHA */
};

const uint8_t egui_image_alpha_type_size_table[] = {
        1, /* EGUI_IMAGE_ALPHA_TYPE_1 */
        2, /* EGUI_IMAGE_ALPHA_TYPE_2 */
        4, /* EGUI_IMAGE_ALPHA_TYPE_4 */
        8, /* EGUI_IMAGE_ALPHA_TYPE_8 */
};

int egui_image_std_get_linear_src_x_segment(const egui_dim_t *src_x_map, egui_dim_t start, egui_dim_t end, egui_dim_t *src_x_start)
{
    if (src_x_start == NULL || start >= end)
    {
        return 0;
    }

    if (src_x_map == NULL)
    {
        *src_x_start = start;
        return 1;
    }

    *src_x_start = src_x_map[start];
    for (egui_dim_t i = start + 1; i < end; i++)
    {
        if (src_x_map[i] != (*src_x_start + (i - start)))
        {
            return 0;
        }
    }

    return 1;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_get_src_x(const egui_dim_t *src_x_map, egui_dim_t index)
{
    return (src_x_map != NULL) ? src_x_map[index] : index;
}

__EGUI_STATIC_INLINE__ const egui_dim_t *egui_image_std_get_src_x_sub_map(const egui_dim_t *src_x_map, egui_dim_t offset)
{
    return (src_x_map != NULL) ? &src_x_map[offset] : NULL;
}

typedef void(egui_image_std_get_pixel)(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
typedef void(egui_image_std_get_col_pixel_with_alpha)(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color,
                                                      egui_alpha_t *alpha);

typedef void(egui_image_std_blend_mapped_row_func)(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                   const egui_dim_t *src_x_map, egui_dim_t count, egui_alpha_t canvas_alpha);
typedef void(egui_image_std_blend_repeat2_row_func)(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row, egui_dim_t src_x_start,
                                                    egui_dim_t src_count, egui_alpha_t canvas_alpha);

void egui_image_std_load_data_resource(void *dest, egui_image_std_info_t *image, uint32_t start_offset, uint32_t size);
void egui_image_std_load_alpha_resource(void *dest, egui_image_std_info_t *image, uint32_t start_offset, uint32_t size);

#define EGUI_IMAGE_STD_EXTERNAL_DATA_CACHE_MAX_BYTES EGUI_CONFIG_IMAGE_EXTERNAL_DATA_CACHE_MAX_BYTES
#define EGUI_IMAGE_STD_EXTERNAL_ALPHA_CACHE_MAX_BYTES EGUI_CONFIG_IMAGE_EXTERNAL_ALPHA_CACHE_MAX_BYTES

#if EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS
static uint16_t g_egui_image_std_shared_external_data_cache[(EGUI_IMAGE_STD_EXTERNAL_DATA_CACHE_MAX_BYTES + sizeof(uint16_t) - 1) / sizeof(uint16_t)];
static uint8_t g_egui_image_std_shared_external_alpha_cache[EGUI_IMAGE_STD_EXTERNAL_ALPHA_CACHE_MAX_BYTES];
static uint8_t g_egui_image_std_shared_external_cache_owner = EGUI_IMAGE_EXTERNAL_ROW_CACHE_OWNER_NONE;
static uint32_t g_egui_image_std_shared_external_cache_generation = 1U;

uint32_t egui_image_std_claim_shared_external_row_cache(egui_image_external_row_cache_owner_t owner)
{
    if ((uint8_t)owner != g_egui_image_std_shared_external_cache_owner)
    {
        g_egui_image_std_shared_external_cache_owner = (uint8_t)owner;
        g_egui_image_std_shared_external_cache_generation++;
        if (g_egui_image_std_shared_external_cache_generation == 0U)
        {
            g_egui_image_std_shared_external_cache_generation = 1U;
        }
    }

    return g_egui_image_std_shared_external_cache_generation;
}

uint16_t *egui_image_std_get_shared_external_data_cache(void)
{
    return g_egui_image_std_shared_external_data_cache;
}

uint8_t *egui_image_std_get_shared_external_alpha_cache(void)
{
    return g_egui_image_std_shared_external_alpha_cache;
}
#endif

typedef struct
{
    const egui_image_std_info_t *image;
    uint32_t data_source_row_size;
    uint32_t alpha_source_row_size;
    uint32_t data_row_start_offset;
    uint32_t alpha_row_start_offset;
    uint32_t data_row_size;
    uint32_t alpha_row_size;
    egui_dim_t chunk_row_start;
    egui_dim_t chunk_row_count;
    egui_dim_t rows_per_chunk;
    egui_dim_t src_x_start;
#if EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS
    uint32_t shared_generation;
#else
    uint16_t data_buf[(EGUI_IMAGE_STD_EXTERNAL_DATA_CACHE_MAX_BYTES + sizeof(uint16_t) - 1) / sizeof(uint16_t)];
    uint8_t alpha_buf[EGUI_IMAGE_STD_EXTERNAL_ALPHA_CACHE_MAX_BYTES];
#endif
} egui_image_std_external_alpha_row_persistent_cache_t;

#if EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS
static void egui_image_std_sync_external_row_cache_generation(uint32_t *shared_generation, egui_dim_t *chunk_row_start, egui_dim_t *chunk_row_count)
{
    uint32_t generation = egui_image_std_claim_shared_external_row_cache(EGUI_IMAGE_EXTERNAL_ROW_CACHE_OWNER_STD);

    if (shared_generation != NULL && *shared_generation != generation)
    {
        if (chunk_row_start != NULL)
        {
            *chunk_row_start = -1;
        }
        if (chunk_row_count != NULL)
        {
            *chunk_row_count = 0;
        }
        *shared_generation = generation;
    }
}
#endif

__EGUI_STATIC_INLINE__ uint16_t *egui_image_std_get_external_alpha_row_persistent_cache_data_buf(egui_image_std_external_alpha_row_persistent_cache_t *cache)
{
#if EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS
    EGUI_UNUSED(cache);
    return egui_image_std_get_shared_external_data_cache();
#else
    return cache->data_buf;
#endif
}

__EGUI_STATIC_INLINE__ uint8_t *egui_image_std_get_external_alpha_row_persistent_cache_alpha_buf(egui_image_std_external_alpha_row_persistent_cache_t *cache)
{
#if EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS
    EGUI_UNUSED(cache);
    return egui_image_std_get_shared_external_alpha_cache();
#else
    return cache->alpha_buf;
#endif
}

static int egui_image_std_prepare_external_alpha_row_persistent_cache_range_rows(egui_image_std_external_alpha_row_persistent_cache_t *cache,
                                                                                 const egui_image_std_info_t *image, uint32_t data_source_row_size,
                                                                                 uint32_t data_row_start_offset, uint32_t data_row_size,
                                                                                 uint32_t alpha_source_row_size, uint32_t alpha_row_start_offset,
                                                                                 uint32_t alpha_row_size, egui_dim_t src_x_start, egui_dim_t min_rows_per_chunk)
{
    uint32_t data_rows;
    uint32_t alpha_rows;
    uint32_t base_rows_per_chunk;
    uint32_t rows_per_chunk;

    if (cache == NULL || image == NULL || data_source_row_size == 0 || alpha_source_row_size == 0 || data_row_size == 0 || alpha_row_size == 0)
    {
        return 0;
    }

#if EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS
    egui_image_std_sync_external_row_cache_generation(&cache->shared_generation, &cache->chunk_row_start, &cache->chunk_row_count);
#endif

    EGUI_UNUSED(min_rows_per_chunk);

    if (data_row_size > EGUI_IMAGE_STD_EXTERNAL_DATA_CACHE_MAX_BYTES || alpha_row_size > EGUI_IMAGE_STD_EXTERNAL_ALPHA_CACHE_MAX_BYTES)
    {
        return 0;
    }

    data_rows = EGUI_IMAGE_STD_EXTERNAL_DATA_CACHE_MAX_BYTES / data_row_size;
    alpha_rows = EGUI_IMAGE_STD_EXTERNAL_ALPHA_CACHE_MAX_BYTES / alpha_row_size;
    base_rows_per_chunk = (data_rows < alpha_rows) ? data_rows : alpha_rows;
    rows_per_chunk = base_rows_per_chunk;
    if ((uint32_t)image->height < rows_per_chunk)
    {
        rows_per_chunk = (uint32_t)image->height;
    }

    if (cache->image != image || cache->data_source_row_size != data_source_row_size || cache->alpha_source_row_size != alpha_source_row_size ||
        cache->data_row_start_offset != data_row_start_offset || cache->alpha_row_start_offset != alpha_row_start_offset ||
        cache->data_row_size != data_row_size || cache->alpha_row_size != alpha_row_size || cache->rows_per_chunk != (egui_dim_t)rows_per_chunk ||
        cache->src_x_start != src_x_start)
    {
        cache->chunk_row_start = -1;
        cache->chunk_row_count = 0;
    }

    cache->image = image;
    cache->data_source_row_size = data_source_row_size;
    cache->alpha_source_row_size = alpha_source_row_size;
    cache->data_row_start_offset = data_row_start_offset;
    cache->alpha_row_start_offset = alpha_row_start_offset;
    cache->data_row_size = data_row_size;
    cache->alpha_row_size = alpha_row_size;
    cache->rows_per_chunk = (egui_dim_t)rows_per_chunk;
    cache->src_x_start = src_x_start;
    return 1;
}

static int egui_image_std_prepare_external_alpha_row_persistent_cache_min_rows(egui_image_std_external_alpha_row_persistent_cache_t *cache,
                                                                               const egui_image_std_info_t *image, uint32_t data_row_size,
                                                                               uint32_t alpha_row_size, egui_dim_t min_rows_per_chunk)
{
    return egui_image_std_prepare_external_alpha_row_persistent_cache_range_rows(cache, image, data_row_size, 0, data_row_size, alpha_row_size, 0,
                                                                                 alpha_row_size, 0, min_rows_per_chunk);
}

static int egui_image_std_load_external_alpha_row_persistent_cache(egui_image_std_external_alpha_row_persistent_cache_t *cache, egui_dim_t row)
{
    egui_dim_t rows_to_load;
    uint16_t *data_buf;
    uint8_t *alpha_buf;

    if (cache == NULL || cache->image == NULL || row < 0 || row >= cache->image->height)
    {
        return 0;
    }

    data_buf = egui_image_std_get_external_alpha_row_persistent_cache_data_buf(cache);
    alpha_buf = egui_image_std_get_external_alpha_row_persistent_cache_alpha_buf(cache);

    if (row >= cache->chunk_row_start && row < (cache->chunk_row_start + cache->chunk_row_count))
    {
        return 1;
    }

    rows_to_load = cache->image->height - row;
    if (rows_to_load > cache->rows_per_chunk)
    {
        rows_to_load = cache->rows_per_chunk;
    }

    if (cache->data_source_row_size == cache->data_row_size && cache->data_row_start_offset == 0)
    {
        egui_image_std_load_data_resource(data_buf, (egui_image_std_info_t *)cache->image, (uint32_t)row * cache->data_source_row_size,
                                          (uint32_t)rows_to_load * cache->data_row_size);
    }
    else
    {
        for (egui_dim_t i = 0; i < rows_to_load; i++)
        {
            egui_image_std_load_data_resource((uint8_t *)data_buf + (uint32_t)i * cache->data_row_size, (egui_image_std_info_t *)cache->image,
                                              (uint32_t)(row + i) * cache->data_source_row_size + cache->data_row_start_offset, cache->data_row_size);
        }
    }

    if (cache->alpha_source_row_size == cache->alpha_row_size && cache->alpha_row_start_offset == 0)
    {
        egui_image_std_load_alpha_resource(alpha_buf, (egui_image_std_info_t *)cache->image, (uint32_t)row * cache->alpha_source_row_size,
                                           (uint32_t)rows_to_load * cache->alpha_row_size);
    }
    else
    {
        for (egui_dim_t i = 0; i < rows_to_load; i++)
        {
            egui_image_std_load_alpha_resource(alpha_buf + (uint32_t)i * cache->alpha_row_size, (egui_image_std_info_t *)cache->image,
                                               (uint32_t)(row + i) * cache->alpha_source_row_size + cache->alpha_row_start_offset, cache->alpha_row_size);
        }
    }

    cache->chunk_row_start = row;
    cache->chunk_row_count = rows_to_load;
    return 1;
}

__EGUI_STATIC_INLINE__ const uint16_t *egui_image_std_get_external_alpha_row_persistent_data(const egui_image_std_external_alpha_row_persistent_cache_t *cache,
                                                                                             egui_dim_t row)
{
    return (const uint16_t *)((const uint8_t *)egui_image_std_get_external_alpha_row_persistent_cache_data_buf(
                                      (egui_image_std_external_alpha_row_persistent_cache_t *)cache) +
                              (uint32_t)(row - cache->chunk_row_start) * cache->data_row_size);
}

__EGUI_STATIC_INLINE__ const uint8_t *egui_image_std_get_external_alpha_row_persistent_alpha(const egui_image_std_external_alpha_row_persistent_cache_t *cache,
                                                                                             egui_dim_t row)
{
    return egui_image_std_get_external_alpha_row_persistent_cache_alpha_buf((egui_image_std_external_alpha_row_persistent_cache_t *)cache) +
           (uint32_t)(row - cache->chunk_row_start) * cache->alpha_row_size;
}

typedef struct
{
    const egui_image_std_info_t *image;
    uint32_t data_source_row_size;
    uint32_t data_row_start_offset;
    uint32_t data_row_size;
    egui_dim_t chunk_row_start;
    egui_dim_t chunk_row_count;
    egui_dim_t rows_per_chunk;
#if EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS
    uint32_t shared_generation;
#else
    uint16_t data_buf[(EGUI_IMAGE_STD_EXTERNAL_DATA_CACHE_MAX_BYTES + sizeof(uint16_t) - 1) / sizeof(uint16_t)];
#endif
} egui_image_std_external_data_row_persistent_cache_t;

typedef union
{
    egui_image_std_external_alpha_row_persistent_cache_t alpha;
    egui_image_std_external_data_row_persistent_cache_t data;
} egui_image_std_external_row_persistent_cache_storage_t;

static egui_image_std_external_row_persistent_cache_storage_t g_egui_image_std_external_row_persistent_cache_storage = {0};

__EGUI_STATIC_INLINE__ egui_image_std_external_alpha_row_persistent_cache_t *egui_image_std_get_external_alpha_row_persistent_cache(void)
{
    return &g_egui_image_std_external_row_persistent_cache_storage.alpha;
}

__EGUI_STATIC_INLINE__ egui_image_std_external_data_row_persistent_cache_t *egui_image_std_get_external_data_row_persistent_cache(void)
{
    return &g_egui_image_std_external_row_persistent_cache_storage.data;
}

__EGUI_STATIC_INLINE__ uint8_t *egui_image_std_get_external_alpha_probe_buf(void)
{
#if EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS
    egui_image_std_claim_shared_external_row_cache(EGUI_IMAGE_EXTERNAL_ROW_CACHE_OWNER_STD);
    return egui_image_std_get_shared_external_alpha_cache();
#else
    return g_egui_image_std_external_row_persistent_cache_storage.alpha.alpha_buf;
#endif
}

__EGUI_STATIC_INLINE__ uint16_t *egui_image_std_get_external_data_row_persistent_cache_data_buf(egui_image_std_external_data_row_persistent_cache_t *cache)
{
#if EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS
    EGUI_UNUSED(cache);
    return egui_image_std_get_shared_external_data_cache();
#else
    return cache->data_buf;
#endif
}

static int egui_image_std_prepare_external_data_row_persistent_cache_range_rows(egui_image_std_external_data_row_persistent_cache_t *cache,
                                                                                const egui_image_std_info_t *image, uint32_t data_source_row_size,
                                                                                uint32_t data_row_start_offset, uint32_t data_row_size,
                                                                                egui_dim_t min_rows_per_chunk)
{
    uint32_t base_rows_per_chunk;
    uint32_t rows_per_chunk;

    if (cache == NULL || image == NULL || data_source_row_size == 0 || data_row_size == 0)
    {
        return 0;
    }

#if EGUI_CONFIG_IMAGE_EXTERNAL_ROW_CACHE_SHARE_BUFFERS
    egui_image_std_sync_external_row_cache_generation(&cache->shared_generation, &cache->chunk_row_start, &cache->chunk_row_count);
#endif

    EGUI_UNUSED(min_rows_per_chunk);

    if (data_row_size > EGUI_IMAGE_STD_EXTERNAL_DATA_CACHE_MAX_BYTES)
    {
        return 0;
    }

    base_rows_per_chunk = EGUI_IMAGE_STD_EXTERNAL_DATA_CACHE_MAX_BYTES / data_row_size;
    rows_per_chunk = base_rows_per_chunk;
    if ((uint32_t)image->height < rows_per_chunk)
    {
        rows_per_chunk = (uint32_t)image->height;
    }

    if (cache->image != image || cache->data_source_row_size != data_source_row_size || cache->data_row_start_offset != data_row_start_offset ||
        cache->data_row_size != data_row_size || cache->rows_per_chunk != (egui_dim_t)rows_per_chunk)
    {
        cache->chunk_row_start = -1;
        cache->chunk_row_count = 0;
    }

    cache->image = image;
    cache->data_source_row_size = data_source_row_size;
    cache->data_row_start_offset = data_row_start_offset;
    cache->data_row_size = data_row_size;
    cache->rows_per_chunk = (egui_dim_t)rows_per_chunk;
    return 1;
}

static int egui_image_std_prepare_external_data_row_persistent_cache_min_rows(egui_image_std_external_data_row_persistent_cache_t *cache,
                                                                              const egui_image_std_info_t *image, uint32_t data_row_size,
                                                                              egui_dim_t min_rows_per_chunk)
{
    return egui_image_std_prepare_external_data_row_persistent_cache_range_rows(cache, image, data_row_size, 0, data_row_size, min_rows_per_chunk);
}

static int egui_image_std_load_external_data_row_persistent_cache(egui_image_std_external_data_row_persistent_cache_t *cache, egui_dim_t row)
{
    egui_dim_t rows_to_load;
    uint16_t *data_buf;

    if (cache == NULL || cache->image == NULL || row < 0 || row >= cache->image->height)
    {
        return 0;
    }

    data_buf = egui_image_std_get_external_data_row_persistent_cache_data_buf(cache);

    if (row >= cache->chunk_row_start && row < (cache->chunk_row_start + cache->chunk_row_count))
    {
        return 1;
    }

    rows_to_load = cache->image->height - row;
    if (rows_to_load > cache->rows_per_chunk)
    {
        rows_to_load = cache->rows_per_chunk;
    }

    if (cache->data_source_row_size == cache->data_row_size && cache->data_row_start_offset == 0)
    {
        egui_image_std_load_data_resource(data_buf, (egui_image_std_info_t *)cache->image, (uint32_t)row * cache->data_source_row_size,
                                          (uint32_t)rows_to_load * cache->data_row_size);
    }
    else
    {
        for (egui_dim_t i = 0; i < rows_to_load; i++)
        {
            egui_image_std_load_data_resource((uint8_t *)data_buf + (uint32_t)i * cache->data_row_size, (egui_image_std_info_t *)cache->image,
                                              (uint32_t)(row + i) * cache->data_source_row_size + cache->data_row_start_offset, cache->data_row_size);
        }
    }

    cache->chunk_row_start = row;
    cache->chunk_row_count = rows_to_load;
    return 1;
}

__EGUI_STATIC_INLINE__ const uint16_t *egui_image_std_get_external_data_row_persistent_data(const egui_image_std_external_data_row_persistent_cache_t *cache,
                                                                                            egui_dim_t row)
{
    return (const uint16_t *)((const uint8_t *)egui_image_std_get_external_data_row_persistent_cache_data_buf(
                                      (egui_image_std_external_data_row_persistent_cache_t *)cache) +
                              (uint32_t)(row - cache->chunk_row_start) * cache->data_row_size);
}

__EGUI_STATIC_INLINE__ uint32_t egui_image_std_alpha_buf_size(egui_dim_t width, egui_dim_t height, uint8_t alpha_type)
{
    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_8:
        return (uint32_t)width * height;
    case EGUI_IMAGE_ALPHA_TYPE_4:
        return (uint32_t)((width + 1) >> 1) * height;
    case EGUI_IMAGE_ALPHA_TYPE_2:
        return (uint32_t)((width + 3) >> 2) * height;
    case EGUI_IMAGE_ALPHA_TYPE_1:
        return (uint32_t)((width + 7) >> 3) * height;
    default:
        return 0;
    }
}

#if EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES > 0
typedef struct
{
    const egui_image_std_info_t *image;
    const void *data_addr;
    const void *alpha_addr;
    void *data_buf;
    void *alpha_buf;
    uint32_t data_size;
    uint32_t alpha_size;
    egui_image_std_info_t cached_info;
} egui_image_std_external_persistent_cache_t;

static egui_image_std_external_persistent_cache_t g_egui_image_std_external_persistent_cache = {0};

static uint32_t egui_image_std_external_persistent_cache_data_size(const egui_image_std_info_t *image)
{
    if (image == NULL)
    {
        return 0;
    }

    switch (image->data_type)
    {
    case EGUI_IMAGE_DATA_TYPE_RGB32:
        return (uint32_t)image->width * image->height * sizeof(uint32_t);
    case EGUI_IMAGE_DATA_TYPE_RGB565:
        return (uint32_t)image->width * image->height * sizeof(uint16_t);
    case EGUI_IMAGE_DATA_TYPE_GRAY8:
        return (uint32_t)image->width * image->height;
    case EGUI_IMAGE_DATA_TYPE_ALPHA:
        return egui_image_std_alpha_buf_size(image->width, image->height, image->alpha_type);
    default:
        return 0;
    }
}

static uint32_t egui_image_std_external_persistent_cache_alpha_size(const egui_image_std_info_t *image)
{
    if (image == NULL || image->alpha_buf == NULL)
    {
        return 0;
    }

    return egui_image_std_alpha_buf_size(image->width, image->height, image->alpha_type);
}

static void egui_image_std_release_external_persistent_cache(egui_image_std_external_persistent_cache_t *cache)
{
    if (cache->data_buf != NULL)
    {
        egui_free(cache->data_buf);
    }
    if (cache->alpha_buf != NULL)
    {
        egui_free(cache->alpha_buf);
    }

    memset(cache, 0, sizeof(*cache));
}

const egui_image_std_info_t *egui_image_std_prepare_external_persistent_cache(const egui_image_std_info_t *image)
{
    egui_image_std_external_persistent_cache_t *cache = &g_egui_image_std_external_persistent_cache;
    uint32_t data_size;
    uint32_t alpha_size;
    uint32_t total_size;
    int data_hit;
    int alpha_hit;

    if (image == NULL || image->res_type != EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        return NULL;
    }

    if (cache->image == image && cache->data_addr == image->data_buf && cache->data_buf != NULL &&
        (image->alpha_buf == NULL || (cache->alpha_addr == image->alpha_buf && cache->alpha_buf != NULL)))
    {
        return &cache->cached_info;
    }

    data_size = egui_image_std_external_persistent_cache_data_size(image);
    alpha_size = egui_image_std_external_persistent_cache_alpha_size(image);
    total_size = data_size + alpha_size;
    if (data_size == 0 || total_size == 0 || total_size > EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES)
    {
        return NULL;
    }

    data_hit = cache->image == image && cache->data_addr == image->data_buf && cache->data_size == data_size && cache->data_buf != NULL;
    alpha_hit = alpha_size == 0 || (cache->image == image && cache->alpha_addr == image->alpha_buf && cache->alpha_size == alpha_size && cache->alpha_buf != NULL);

    if (!data_hit || !alpha_hit)
    {
        void *new_data_buf = egui_malloc((int)data_size);
        void *new_alpha_buf = NULL;

        if (new_data_buf == NULL)
        {
            return NULL;
        }

        egui_api_load_external_resource(new_data_buf, (egui_uintptr_t)(image->data_buf), 0, data_size);

        if (alpha_size > 0)
        {
            new_alpha_buf = egui_malloc((int)alpha_size);
            if (new_alpha_buf == NULL)
            {
                egui_free(new_data_buf);
                return NULL;
            }

            egui_api_load_external_resource(new_alpha_buf, (egui_uintptr_t)(image->alpha_buf), 0, alpha_size);
        }

        egui_image_std_release_external_persistent_cache(cache);
        cache->image = image;
        cache->data_addr = image->data_buf;
        cache->alpha_addr = image->alpha_buf;
        cache->data_buf = new_data_buf;
        cache->alpha_buf = new_alpha_buf;
        cache->data_size = data_size;
        cache->alpha_size = alpha_size;
        cache->cached_info = *image;
        cache->cached_info.data_buf = cache->data_buf;
        cache->cached_info.alpha_buf = cache->alpha_buf;
        cache->cached_info.res_type = EGUI_RESOURCE_TYPE_INTERNAL;
    }

    return &cache->cached_info;
}
#else
const egui_image_std_info_t *egui_image_std_prepare_external_persistent_cache(const egui_image_std_info_t *image)
{
    (void)image;
    return NULL;
}
#endif

__EGUI_STATIC_INLINE__ const egui_image_t *egui_image_std_resolve_external_persistent_image(const egui_image_t *self, egui_image_t *resolved_self)
{
    const egui_image_std_info_t *cached_info =
            egui_image_std_prepare_external_persistent_cache((const egui_image_std_info_t *)self->res);

    if (cached_info == NULL)
    {
        return self;
    }

    *resolved_self = *self;
    resolved_self->res = cached_info;
    return resolved_self;
}
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb32(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    uint32_t sel_color = ((uint32_t *)image->data_buf)[x + y * image->width];
    color->full = EGUI_COLOR_RGB888_TRANS(sel_color);
    *alpha = (sel_color >> 24) & 0xFF;
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_image_std_set_image_resize_rgb565_external(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                            egui_dim_t x_base, egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio);
#endif

__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_limit(const uint16_t *p_data, egui_dim_t col_index, egui_color_t *color)
{
    uint32_t sel_pos = col_index;
    uint16_t sel_color = p_data[sel_pos];

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_limit(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color)
{
    uint32_t row_start = y * image->width;
    const uint16_t *p_data = image->data_buf;

    egui_image_std_get_col_pixel_rgb565_limit(&p_data[row_start], x, color);
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_image_std_get_pixel_rgb565_limit(image, x, y, color);
    *alpha = EGUI_ALPHA_100;
}

// __EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color,
// egui_alpha_t *alpha)
// {
//     egui_image_std_get_col_pixel_rgb565_limit(p_data, col_index, color);
//     *alpha = EGUI_ALPHA_100;
// }
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_8(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color,
                                                                  egui_alpha_t *alpha)
{
    uint32_t sel_pos = col_index;
    uint16_t sel_color = p_data[sel_pos];
    uint8_t sel_alpha = p_alpha[sel_pos];

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
    *alpha = sel_alpha;
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_8(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color,
                                                              egui_alpha_t *alpha)
{
    uint32_t row_start = y * image->width;
    const uint16_t *p_data = image->data_buf;
    const uint8_t *p_alpha = image->alpha_buf;

    egui_image_std_get_col_pixel_rgb565_8(&p_data[row_start], &p_alpha[row_start], x, color, alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_4(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color,
                                                                  egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index;
    // get alpha row position.
    uint32_t sel_alpha_pos = col_index >> 1; // same to: x / 2
    uint16_t sel_color = p_data[sel_pos];
    uint8_t sel_alpha;

    bit_pos = col_index & 0x01; // 0x01
    bit_pos = bit_pos << 2;     // same to: bit_pos * 4

    sel_alpha = ((p_alpha[sel_alpha_pos]) >> bit_pos) & 0x0F;

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
    *alpha = egui_alpha_change_table_4[sel_alpha];
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_4(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color,
                                                              egui_alpha_t *alpha)
{
    uint32_t row_start = y * image->width;
    uint32_t row_start_alpha = y * ((image->width + 1) >> 1); // same to: ((image->width + 1) / 2);
    const uint16_t *p_data = image->data_buf;
    const uint8_t *p_alpha = image->alpha_buf;

    egui_image_std_get_col_pixel_rgb565_4(&p_data[row_start], &p_alpha[row_start_alpha], x, color, alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_2(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color,
                                                                  egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index;
    // get alpha row position.
    uint32_t sel_alpha_pos = col_index >> 2; // same to: x / 4
    uint16_t sel_color = p_data[sel_pos];
    uint8_t sel_alpha;

    bit_pos = col_index & 0x03; // 0x03
    bit_pos = bit_pos << 1;     // same to: bit_pos * 2

    sel_alpha = ((p_alpha[sel_alpha_pos]) >> bit_pos) & 0x03;

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
    *alpha = egui_alpha_change_table_2[sel_alpha];
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_2(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color,
                                                              egui_alpha_t *alpha)
{
    uint32_t row_start = y * image->width;
    uint32_t row_start_alpha = y * ((image->width + 3) >> 2); // same to: ((image->width + 3) / 4);
    const uint16_t *p_data = image->data_buf;
    const uint8_t *p_alpha = image->alpha_buf;

    egui_image_std_get_col_pixel_rgb565_2(&p_data[row_start], &p_alpha[row_start_alpha], x, color, alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_pixel_rgb565_1(const uint16_t *p_data, const uint8_t *p_alpha, egui_dim_t col_index, egui_color_t *color,
                                                                  egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index;
    // get alpha row position.
    uint32_t sel_alpha_pos = col_index >> 3; // same to x / 8
    uint16_t sel_color = p_data[sel_pos];
    uint8_t sel_alpha;

    bit_pos = col_index & 0x07; // 0x07

    sel_alpha = ((p_alpha[sel_alpha_pos]) >> bit_pos) & 0x01;

    color->full = EGUI_COLOR_RGB565_TRANS(sel_color);
    *alpha = (sel_alpha ? 0xff : 0x00);
}

__EGUI_STATIC_INLINE__ void egui_image_std_get_pixel_rgb565_1(egui_image_std_info_t *image, egui_dim_t x, egui_dim_t y, egui_color_t *color,
                                                              egui_alpha_t *alpha)
{
    uint32_t row_start = y * image->width;
    uint32_t row_start_alpha = y * ((image->width + 7) >> 3); // same to ((image->width + 7) / 8);
    const uint16_t *p_data = image->data_buf;
    const uint8_t *p_alpha = image->alpha_buf;

    egui_image_std_get_col_pixel_rgb565_1(&p_data[row_start], &p_alpha[row_start_alpha], x, color, alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_prepare_resize_src_x_map(egui_dim_t *src_x_map, egui_dim_t x, egui_dim_t x_total, egui_float_t width_radio)
{
    egui_dim_t count = x_total - x;
    egui_float_t src_x_acc = (egui_float_t)((int64_t)x * (int64_t)width_radio);

    EGUI_ASSERT(count <= EGUI_CONFIG_PFB_WIDTH);
    for (egui_dim_t i = 0; i < count; i++)
    {
        src_x_map[i] = EGUI_FLOAT_INT_PART(src_x_acc);
        src_x_acc += width_radio;
    }

    return count;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_prepare_resize_src_x_map_limit(egui_dim_t *src_x_map, egui_dim_t x, egui_dim_t x_total,
                                                                                egui_float_t width_radio)
{
    egui_dim_t count = x_total - x;
    egui_float_t width_step = (width_radio >> 8);
    egui_float_t src_x_acc = x * width_step;

    EGUI_ASSERT(count <= EGUI_CONFIG_PFB_WIDTH);
    for (egui_dim_t i = 0; i < count; i++)
    {
        src_x_map[i] = src_x_acc >> 8;
        src_x_acc += width_step;
    }

    return count;
}

__EGUI_STATIC_INLINE__ int egui_image_std_can_use_resize_repeat2_fast_path(egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                                           egui_float_t width_radio, egui_float_t height_radio)
{
    return (width_radio == EGUI_FLOAT_VALUE(0.5f)) && (height_radio == EGUI_FLOAT_VALUE(0.5f)) && ((x & 0x01) == 0) && ((y & 0x01) == 0) &&
           (((x_total - x) & 0x01) == 0) && (((y_total - y) & 0x01) == 0);
}

__EGUI_STATIC_INLINE__ int egui_image_std_can_use_resize_repeat4_fast_path(egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                                           egui_float_t width_radio, egui_float_t height_radio)
{
    return (width_radio == EGUI_FLOAT_VALUE(0.25f)) && (height_radio == EGUI_FLOAT_VALUE(0.25f)) && ((x & 0x03) == 0) && ((y & 0x03) == 0) &&
           (((x_total - x) & 0x03) == 0) && (((y_total - y) & 0x03) == 0);
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_resize_pixel(egui_color_int_t *dst, egui_color_t color, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        *dst = color.full;
    }
    else
    {
        egui_color_t *back_color = (egui_color_t *)dst;
        egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_src_pixel(egui_color_int_t *dst, uint16_t src_pixel, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
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
#else
    {
        egui_color_t color;
        color.full = EGUI_COLOR_RGB565_TRANS(src_pixel);
        egui_image_std_blend_resize_pixel(dst, color, alpha);
    }
#endif
}

#if EGUI_CONFIG_COLOR_DEPTH == 16
#define EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA 4
#else
#define EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA 1
#endif

__EGUI_STATIC_INLINE__ void egui_image_std_copy_rgb565_row(egui_color_int_t *dst_row, const uint16_t *src_row, egui_dim_t count)
{
    if (count <= 0)
    {
        return;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
    if ((((egui_uintptr_t)dst_row ^ (egui_uintptr_t)src_row) & 0x03U) != 0U)
    {
        for (egui_dim_t i = 0; i < count; i++)
        {
            dst_row[i] = src_row[i];
        }
        return;
    }

    if ((((egui_uintptr_t)dst_row) & 0x03U) != 0U)
    {
        *dst_row++ = *src_row++;
        count--;
    }

    if (count >= 2)
    {
        const uint32_t *src32 = (const uint32_t *)src_row;
        uint32_t *dst32 = (uint32_t *)dst_row;

        while (count >= 8)
        {
            dst32[0] = src32[0];
            dst32[1] = src32[1];
            dst32[2] = src32[2];
            dst32[3] = src32[3];
            src32 += 4;
            dst32 += 4;
            count -= 8;
        }

        while (count >= 2)
        {
            *dst32++ = *src32++;
            count -= 2;
        }

        src_row = (const uint16_t *)src32;
        dst_row = (egui_color_int_t *)dst32;
    }

    if (count != 0)
    {
        *dst_row = *src_row;
    }
#else
    for (egui_dim_t i = 0; i < count; i++)
    {
        dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[i]);
    }
#endif
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_row(egui_color_int_t *dst_row, const uint16_t *src_row, egui_dim_t count, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        egui_image_std_copy_rgb565_row(dst_row, src_row, count);
        return;
    }

    for (egui_dim_t i = 0; i < count; i++)
    {
        egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_mapped_row(egui_color_int_t *dst_row, const uint16_t *src_row, const egui_dim_t *src_x_map,
                                                                   egui_dim_t count, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t i = 0; i < count; i++)
        {
            dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
        }
        return;
    }

    for (egui_dim_t i = 0; i < count; i++)
    {
        egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x_map[i]], alpha);
    }
}

#define EGUI_IMAGE_STD_BLEND_RGB565_PACKED_ALPHA_REPEAT2_ROW_DEFINE(_func_name, _get_alpha_func)                                                               \
    __EGUI_STATIC_INLINE__ void _func_name(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row, egui_dim_t src_x_start,           \
                                           egui_dim_t src_count, egui_alpha_t canvas_alpha)                                                                    \
    {                                                                                                                                                          \
        if (canvas_alpha == 0 || src_count <= 0)                                                                                                               \
        {                                                                                                                                                      \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
                                                                                                                                                               \
        if (canvas_alpha == EGUI_ALPHA_100)                                                                                                                    \
        {                                                                                                                                                      \
            for (egui_dim_t i = 0; i < src_count; i++)                                                                                                         \
            {                                                                                                                                                  \
                egui_dim_t src_x = src_x_start + i;                                                                                                            \
                egui_alpha_t src_alpha = _get_alpha_func(src_alpha_row, src_x);                                                                                \
                egui_dim_t dst_x = i << 1;                                                                                                                     \
                                                                                                                                                               \
                if (src_alpha == 0)                                                                                                                            \
                {                                                                                                                                              \
                    continue;                                                                                                                                  \
                }                                                                                                                                              \
                                                                                                                                                               \
                egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);                                                                              \
                if (src_alpha == EGUI_ALPHA_100)                                                                                                               \
                {                                                                                                                                              \
                    dst_row[dst_x] = pixel;                                                                                                                    \
                    dst_row[dst_x + 1] = pixel;                                                                                                                \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x], pixel, src_alpha);                                                                  \
                    egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 1], pixel, src_alpha);                                                              \
                }                                                                                                                                              \
            }                                                                                                                                                  \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
                                                                                                                                                               \
        for (egui_dim_t i = 0; i < src_count; i++)                                                                                                             \
        {                                                                                                                                                      \
            egui_dim_t src_x = src_x_start + i;                                                                                                                \
            egui_alpha_t src_alpha = _get_alpha_func(src_alpha_row, src_x);                                                                                    \
            egui_dim_t dst_x = i << 1;                                                                                                                         \
            egui_alpha_t alpha;                                                                                                                                \
                                                                                                                                                               \
            if (src_alpha == 0)                                                                                                                                \
            {                                                                                                                                                  \
                continue;                                                                                                                                      \
            }                                                                                                                                                  \
                                                                                                                                                               \
            alpha = (src_alpha == EGUI_ALPHA_100) ? canvas_alpha : egui_color_alpha_mix(canvas_alpha, src_alpha);                                              \
            if (alpha == 0)                                                                                                                                    \
            {                                                                                                                                                  \
                continue;                                                                                                                                      \
            }                                                                                                                                                  \
                                                                                                                                                               \
            egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);                                                                                  \
            egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x], pixel, alpha);                                                                              \
            egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 1], pixel, alpha);                                                                          \
        }                                                                                                                                                      \
    }

#define EGUI_IMAGE_STD_BLEND_RGB565_PACKED_ALPHA_REPEAT4_ROW_DEFINE(_func_name, _get_alpha_func)                                                               \
    __EGUI_STATIC_INLINE__ void _func_name(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row, egui_dim_t src_x_start,           \
                                           egui_dim_t src_count, egui_alpha_t canvas_alpha)                                                                    \
    {                                                                                                                                                          \
        if (canvas_alpha == 0 || src_count <= 0)                                                                                                               \
        {                                                                                                                                                      \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
                                                                                                                                                               \
        if (canvas_alpha == EGUI_ALPHA_100)                                                                                                                    \
        {                                                                                                                                                      \
            for (egui_dim_t i = 0; i < src_count; i++)                                                                                                         \
            {                                                                                                                                                  \
                egui_dim_t src_x = src_x_start + i;                                                                                                            \
                egui_alpha_t src_alpha = _get_alpha_func(src_alpha_row, src_x);                                                                                \
                egui_dim_t dst_x = i << 2;                                                                                                                     \
                                                                                                                                                               \
                if (src_alpha == 0)                                                                                                                            \
                {                                                                                                                                              \
                    continue;                                                                                                                                  \
                }                                                                                                                                              \
                                                                                                                                                               \
                egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);                                                                              \
                if (src_alpha == EGUI_ALPHA_100)                                                                                                               \
                {                                                                                                                                              \
                    dst_row[dst_x] = pixel;                                                                                                                    \
                    dst_row[dst_x + 1] = pixel;                                                                                                                \
                    dst_row[dst_x + 2] = pixel;                                                                                                                \
                    dst_row[dst_x + 3] = pixel;                                                                                                                \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x], pixel, src_alpha);                                                                  \
                    egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 1], pixel, src_alpha);                                                              \
                    egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 2], pixel, src_alpha);                                                              \
                    egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 3], pixel, src_alpha);                                                              \
                }                                                                                                                                              \
            }                                                                                                                                                  \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
                                                                                                                                                               \
        for (egui_dim_t i = 0; i < src_count; i++)                                                                                                             \
        {                                                                                                                                                      \
            egui_dim_t src_x = src_x_start + i;                                                                                                                \
            egui_alpha_t src_alpha = _get_alpha_func(src_alpha_row, src_x);                                                                                    \
            egui_dim_t dst_x = i << 2;                                                                                                                         \
            egui_alpha_t alpha;                                                                                                                                \
                                                                                                                                                               \
            if (src_alpha == 0)                                                                                                                                \
            {                                                                                                                                                  \
                continue;                                                                                                                                      \
            }                                                                                                                                                  \
                                                                                                                                                               \
            alpha = (src_alpha == EGUI_ALPHA_100) ? canvas_alpha : egui_color_alpha_mix(canvas_alpha, src_alpha);                                              \
            if (alpha == 0)                                                                                                                                    \
            {                                                                                                                                                  \
                continue;                                                                                                                                      \
            }                                                                                                                                                  \
                                                                                                                                                               \
            egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);                                                                                  \
            egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x], pixel, alpha);                                                                              \
            egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 1], pixel, alpha);                                                                          \
            egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 2], pixel, alpha);                                                                          \
            egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 3], pixel, alpha);                                                                          \
        }                                                                                                                                                      \
    }

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_repeat2_row(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                           egui_dim_t src_x_start, egui_dim_t src_count, egui_alpha_t canvas_alpha)
{
    if (canvas_alpha == 0 || src_count <= 0)
    {
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t i = 0; i < src_count; i++)
        {
            egui_dim_t src_x = src_x_start + i;
            egui_alpha_t src_alpha = src_alpha_row[src_x];
            egui_dim_t dst_x = i << 1;

            if (src_alpha == 0)
            {
                continue;
            }

            if (src_alpha == EGUI_ALPHA_100)
            {
                egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                dst_row[dst_x] = pixel;
                dst_row[dst_x + 1] = pixel;
            }
            else
            {
                egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x], pixel, src_alpha);
                egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 1], pixel, src_alpha);
            }
        }
        return;
    }

    for (egui_dim_t i = 0; i < src_count; i++)
    {
        egui_dim_t src_x = src_x_start + i;
        egui_alpha_t src_alpha = src_alpha_row[src_x];
        egui_dim_t dst_x = i << 1;
        egui_alpha_t alpha;

        if (src_alpha == 0)
        {
            continue;
        }

        alpha = (src_alpha == EGUI_ALPHA_100) ? canvas_alpha : egui_color_alpha_mix(canvas_alpha, src_alpha);
        if (alpha == 0)
        {
            continue;
        }

        {
            egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);

            egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x], pixel, alpha);
            egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 1], pixel, alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_repeat4_row(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                           egui_dim_t src_x_start, egui_dim_t src_count, egui_alpha_t canvas_alpha)
{
    if (canvas_alpha == 0 || src_count <= 0)
    {
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t i = 0; i < src_count; i++)
        {
            egui_dim_t src_x = src_x_start + i;
            egui_alpha_t src_alpha = src_alpha_row[src_x];
            egui_dim_t dst_x = i << 2;

            if (src_alpha == 0)
            {
                continue;
            }

            if (src_alpha == EGUI_ALPHA_100)
            {
                egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                dst_row[dst_x] = pixel;
                dst_row[dst_x + 1] = pixel;
                dst_row[dst_x + 2] = pixel;
                dst_row[dst_x + 3] = pixel;
            }
            else
            {
                egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x], pixel, src_alpha);
                egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 1], pixel, src_alpha);
                egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 2], pixel, src_alpha);
                egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 3], pixel, src_alpha);
            }
        }
        return;
    }

    for (egui_dim_t i = 0; i < src_count; i++)
    {
        egui_dim_t src_x = src_x_start + i;
        egui_alpha_t src_alpha = src_alpha_row[src_x];
        egui_dim_t dst_x = i << 2;
        egui_alpha_t alpha;

        if (src_alpha == 0)
        {
            continue;
        }

        alpha = (src_alpha == EGUI_ALPHA_100) ? canvas_alpha : egui_color_alpha_mix(canvas_alpha, src_alpha);
        if (alpha == 0)
        {
            continue;
        }

        {
            egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);

            egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x], pixel, alpha);
            egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 1], pixel, alpha);
            egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 2], pixel, alpha);
            egui_image_std_blend_rgb565_src_pixel(&dst_row[dst_x + 3], pixel, alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ int egui_image_std_blend_rgb565_alpha8_repeat_segment(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                             egui_dim_t dst_img_x, egui_dim_t count, egui_alpha_t canvas_alpha,
                                                                             int use_repeat2_fast, int use_repeat4_fast)
{
    if (count <= 0)
    {
        return 0;
    }

    if (use_repeat4_fast && ((dst_img_x & 0x03) == 0) && ((count & 0x03) == 0))
    {
        egui_image_std_blend_rgb565_alpha8_repeat4_row(dst_row, src_row, src_alpha_row, dst_img_x >> 2, count >> 2, canvas_alpha);
        return 1;
    }

    if (use_repeat2_fast && ((dst_img_x & 0x01) == 0) && ((count & 0x01) == 0))
    {
        egui_image_std_blend_rgb565_alpha8_repeat2_row(dst_row, src_row, src_alpha_row, dst_img_x >> 1, count >> 1, canvas_alpha);
        return 1;
    }

    return 0;
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_row(egui_color_int_t *dst_row, const uint16_t *src_pixels, const uint8_t *src_alpha_row,
                                                                   egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    if (canvas_alpha == 0)
    {
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        while (i < count)
        {
            while (i + 4 <= count && *(const uint32_t *)&src_alpha_row[i] == 0x00000000)
            {
                i += 4;
            }
            while (i < count && src_alpha_row[i] == 0)
            {
                i++;
            }

            {
                egui_dim_t opaque_start = i;

                while (i + 4 <= count && *(const uint32_t *)&src_alpha_row[i] == 0xFFFFFFFF)
                {
                    i += 4;
                }
                while (i < count && src_alpha_row[i] == EGUI_ALPHA_100)
                {
                    i++;
                }

                if (opaque_start < i)
                {
                    egui_image_std_copy_rgb565_row(&dst_row[opaque_start], &src_pixels[opaque_start], i - opaque_start);
                    continue;
                }
            }

            if (i < count)
            {
                egui_alpha_t alpha = src_alpha_row[i];
                if (alpha >= EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                {
                    egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_pixels[i], alpha);
                }
                i++;
            }
        }
        return;
    }

    while (i < count)
    {
        while (i + 4 <= count && *(const uint32_t *)&src_alpha_row[i] == 0x00000000)
        {
            i += 4;
        }
        while (i < count && src_alpha_row[i] == 0)
        {
            i++;
        }

        egui_dim_t opaque_start = i;
        while (i + 4 <= count && *(const uint32_t *)&src_alpha_row[i] == 0xFFFFFFFF)
        {
            i += 4;
        }
        while (i < count && src_alpha_row[i] == EGUI_ALPHA_100)
        {
            i++;
        }

        if (opaque_start < i)
        {
            egui_image_std_blend_rgb565_row(&dst_row[opaque_start], &src_pixels[opaque_start], i - opaque_start, canvas_alpha);
            continue;
        }

        if (i < count)
        {
            egui_alpha_t src_alpha = src_alpha_row[i];

            if (src_alpha >= EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, src_alpha);

                if (alpha != 0)
                {
                    egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_pixels[i], alpha);
                }
            }
            i++;
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_mapped_row(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                          const egui_dim_t *src_x_map, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    if (canvas_alpha == 0)
    {
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        while (i < count)
        {
            while (i < count && src_alpha_row[src_x_map[i]] == 0)
            {
                i++;
            }

            egui_dim_t opaque_start = i;
            while (i < count && src_alpha_row[src_x_map[i]] == EGUI_ALPHA_100)
            {
                i++;
            }

            if (opaque_start < i)
            {
                egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, EGUI_ALPHA_100);
                continue;
            }

            if (i < count)
            {
                egui_dim_t src_x = src_x_map[i];
                egui_alpha_t alpha = src_alpha_row[src_x];

                if (alpha >= EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                {
                    egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
                }
                i++;
            }
        }
        return;
    }

    while (i < count)
    {
        while (i < count && src_alpha_row[src_x_map[i]] == 0)
        {
            i++;
        }

        egui_dim_t opaque_start = i;
        while (i < count && src_alpha_row[src_x_map[i]] == EGUI_ALPHA_100)
        {
            i++;
        }

        if (opaque_start < i)
        {
            egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, canvas_alpha);
            continue;
        }

        if (i < count)
        {
            egui_dim_t src_x = src_x_map[i];
            egui_alpha_t src_alpha = src_alpha_row[src_x];

            if (src_alpha >= EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, src_alpha);

                if (alpha != 0)
                {
                    egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
                }
            }
            i++;
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_circle_masked_mapped_segment_fixed_row(
        egui_mask_circle_t *circle_mask, egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row, const egui_dim_t *src_x_map,
        egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y, egui_dim_t center_x, egui_dim_t radius, egui_dim_t row_index, egui_alpha_t canvas_alpha,
        const egui_circle_info_t *info, const egui_circle_item_t *items);

__EGUI_STATIC_INLINE__ egui_alpha_t egui_image_std_get_circle_corner_alpha_fixed_row(egui_dim_t radius, egui_dim_t row_index, egui_dim_t corner_col,
                                                                                      const egui_circle_info_t *info,
                                                                                      const egui_circle_item_t *items)
{
    if (info != NULL)
    {
        return egui_canvas_get_circle_corner_value_fixed_row(row_index, corner_col, info, items);
    }

    EGUI_UNUSED(items);
    return egui_mask_circle_get_corner_alpha(radius, row_index, corner_col);
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_masked_mapped_segment(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                     const uint8_t *src_alpha_row, const egui_dim_t *src_x_map,
                                                                                     egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y,
                                                                                     egui_alpha_t canvas_alpha)
{
    egui_mask_t *mask = canvas->mask;

    if (mask == NULL || count == 0 || canvas_alpha == 0)
    {
        return;
    }

    if (mask->api->kind == EGUI_MASK_KIND_CIRCLE)
    {
        egui_mask_circle_t *circle_mask = (egui_mask_circle_t *)mask;
        const egui_circle_info_t *info;
        const egui_circle_item_t *items;
        egui_dim_t row_index;
        egui_dim_t center_x = circle_mask->center_x;
        egui_dim_t radius = circle_mask->radius;
        egui_dim_t screen_x_end = screen_x + count;

        if (screen_y == circle_mask->point_cached_y)
        {
            if (!circle_mask->point_cached_row_valid)
            {
                return;
            }
            row_index = circle_mask->point_cached_row_index;
        }
        else
        {
            egui_dim_t dy = (screen_y > circle_mask->center_y) ? (screen_y - circle_mask->center_y) : (circle_mask->center_y - screen_y);
            if (dy > radius)
            {
                circle_mask->point_cached_y = screen_y;
                circle_mask->point_cached_row_valid = 0;
                return;
            }
            row_index = radius - dy;
            circle_mask->point_cached_y = screen_y;
            circle_mask->point_cached_row_index = row_index;
            circle_mask->point_cached_row_valid = 1;
        }

        info = circle_mask->info;
        items = (info != NULL) ? (const egui_circle_item_t *)info->items : NULL;

        if (screen_x <= center_x && screen_x_end > center_x)
        {
            egui_image_std_blend_rgb565_alpha8_circle_masked_mapped_segment_fixed_row(circle_mask, dst_row, src_row, src_alpha_row, src_x_map, count, screen_x,
                                                                                       screen_y, center_x, radius, row_index, canvas_alpha, info, items);
            return;
        }

        if (canvas_alpha == EGUI_ALPHA_100)
        {
            if (screen_x_end <= center_x)
            {
                egui_dim_t seg_start = EGUI_MAX(screen_x, center_x - radius);
                egui_dim_t seg_end = EGUI_MIN(screen_x_end, center_x);
                egui_dim_t corner_col = seg_start - (center_x - radius);
                egui_dim_t i = seg_start - screen_x;
                egui_dim_t end_index = seg_end - screen_x;

                for (; i < end_index; i++, corner_col++)
                {
                    egui_dim_t src_x = egui_image_std_get_src_x(src_x_map, i);
                    egui_alpha_t alpha = src_alpha_row[src_x];
                    egui_alpha_t mask_alpha;

                    if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                    {
                        continue;
                    }

                    mask_alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, corner_col, info, items);
                    if (mask_alpha == 0)
                    {
                        continue;
                    }

                    alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
                    if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                    {
                        continue;
                    }

                    egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
                }
                return;
            }

            if (screen_x > center_x)
            {
                egui_dim_t seg_start = EGUI_MAX(screen_x, center_x + 1);
                egui_dim_t seg_end = EGUI_MIN(screen_x_end, center_x + radius + 1);
                egui_dim_t corner_col = (center_x + radius) - seg_start;
                egui_dim_t i = seg_start - screen_x;
                egui_dim_t end_index = seg_end - screen_x;

                for (; i < end_index; i++, corner_col--)
                {
                    egui_dim_t src_x = egui_image_std_get_src_x(src_x_map, i);
                    egui_alpha_t alpha = src_alpha_row[src_x];
                    egui_alpha_t mask_alpha;

                    if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                    {
                        continue;
                    }

                    mask_alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, corner_col, info, items);
                    if (mask_alpha == 0)
                    {
                        continue;
                    }

                    alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
                    if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                    {
                        continue;
                    }

                    egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
                }
                return;
            }
        }
        else
        {
            if (screen_x_end <= center_x)
            {
                egui_dim_t seg_start = EGUI_MAX(screen_x, center_x - radius);
                egui_dim_t seg_end = EGUI_MIN(screen_x_end, center_x);
                egui_dim_t corner_col = seg_start - (center_x - radius);
                egui_dim_t i = seg_start - screen_x;
                egui_dim_t end_index = seg_end - screen_x;

                for (; i < end_index; i++, corner_col++)
                {
                    egui_dim_t src_x = egui_image_std_get_src_x(src_x_map, i);
                    egui_alpha_t alpha = src_alpha_row[src_x];
                    egui_alpha_t mask_alpha;

                    if (alpha == 0)
                    {
                        continue;
                    }

                    mask_alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, corner_col, info, items);
                    if (mask_alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                    {
                        continue;
                    }

                    alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
                    alpha = egui_color_alpha_mix(canvas_alpha, alpha);
                    if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                    {
                        continue;
                    }

                    egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
                }
                return;
            }

            if (screen_x > center_x)
            {
                egui_dim_t seg_start = EGUI_MAX(screen_x, center_x + 1);
                egui_dim_t seg_end = EGUI_MIN(screen_x_end, center_x + radius + 1);
                egui_dim_t corner_col = (center_x + radius) - seg_start;
                egui_dim_t i = seg_start - screen_x;
                egui_dim_t end_index = seg_end - screen_x;

                for (; i < end_index; i++, corner_col--)
                {
                    egui_dim_t src_x = egui_image_std_get_src_x(src_x_map, i);
                    egui_alpha_t alpha = src_alpha_row[src_x];
                    egui_alpha_t mask_alpha;

                    if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                    {
                        continue;
                    }

                    mask_alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, corner_col, info, items);
                    if (mask_alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                    {
                        continue;
                    }

                    alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
                    alpha = egui_color_alpha_mix(canvas_alpha, alpha);
                    if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                    {
                        continue;
                    }

                    egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
                }
                return;
            }
        }
        return;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    if (mask->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE)
    {
        if (egui_mask_round_rectangle_blend_rgb565_alpha8_segment(mask, dst_row, src_row, src_alpha_row, src_x_map, count, screen_x, screen_y, canvas_alpha))
        {
            return;
        }
    }

    if (mask->api->kind == EGUI_MASK_KIND_IMAGE)
    {
        if (egui_mask_image_blend_rgb565_alpha8_segment(mask, dst_row, src_row, src_alpha_row, src_x_map, count, screen_x, screen_y, canvas_alpha))
        {
            return;
        }
    }
#endif

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t i = 0; i < count; i++)
        {
            egui_dim_t src_x = egui_image_std_get_src_x(src_x_map, i);
            egui_color_t color;
            egui_alpha_t alpha = src_alpha_row[src_x];

            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
            mask->api->mask_point(mask, screen_x + i, screen_y, &color, &alpha);

            if (alpha == 0)
            {
                continue;
            }

            if (alpha == EGUI_ALPHA_100)
            {
                dst_row[i] = color.full;
            }
            else
            {
                egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
            }
        }
        return;
    }

    for (egui_dim_t i = 0; i < count; i++)
    {
        egui_dim_t src_x = egui_image_std_get_src_x(src_x_map, i);
        egui_color_t color;
        egui_alpha_t alpha = src_alpha_row[src_x];

        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
        mask->api->mask_point(mask, screen_x + i, screen_y, &color, &alpha);
        alpha = egui_color_alpha_mix(canvas_alpha, alpha);

        if (alpha == 0)
        {
            continue;
        }

        if (alpha == EGUI_ALPHA_100)
        {
            dst_row[i] = color.full;
        }
        else
        {
            egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ int egui_image_std_blend_rgb565_alpha8_round_rect_masked_row_fast(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                          const uint8_t *src_alpha_row, egui_dim_t count, egui_dim_t screen_x,
                                                                                          egui_dim_t screen_y, egui_alpha_t canvas_alpha);
__EGUI_STATIC_INLINE__ int egui_image_std_blend_rgb565_alpha8_circle_masked_row_fast(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                     const uint8_t *src_alpha_row, egui_dim_t count, egui_dim_t screen_x,
                                                                                     egui_dim_t screen_y, egui_alpha_t canvas_alpha);

void egui_image_std_blend_rgb565_alpha8_masked_row(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                    egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y, egui_alpha_t canvas_alpha)
{
#if EGUI_CONFIG_COLOR_DEPTH == 16
    if (count <= 0)
    {
        return;
    }

    if (canvas != NULL && canvas->mask != NULL && canvas->mask->api->kind == EGUI_MASK_KIND_CIRCLE)
    {
        if (egui_image_std_blend_rgb565_alpha8_circle_masked_row_fast(canvas, dst_row, src_row, src_alpha_row, count, screen_x, screen_y, canvas_alpha))
        {
            return;
        }
    }

    if (canvas != NULL && canvas->mask != NULL && canvas->mask->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE)
    {
        if (egui_image_std_blend_rgb565_alpha8_round_rect_masked_row_fast(canvas, dst_row, src_row, src_alpha_row, count, screen_x, screen_y, canvas_alpha))
        {
            return;
        }
    }

    egui_image_std_blend_rgb565_alpha8_masked_mapped_segment(canvas, dst_row, src_row, src_alpha_row, NULL, count, screen_x, screen_y, canvas_alpha);
#else
    EGUI_UNUSED(canvas);
    EGUI_UNUSED(dst_row);
    EGUI_UNUSED(src_row);
    EGUI_UNUSED(src_alpha_row);
    EGUI_UNUSED(count);
    EGUI_UNUSED(screen_x);
    EGUI_UNUSED(screen_y);
    EGUI_UNUSED(canvas_alpha);
#endif
}

__EGUI_STATIC_INLINE__ void egui_image_std_refresh_circle_mask_fast_cache(egui_mask_circle_t *circle_mask)
{
    egui_mask_t *mask = (egui_mask_t *)circle_mask;

    if (circle_mask->cached_x == mask->region.location.x && circle_mask->cached_y == mask->region.location.y &&
        circle_mask->cached_width == mask->region.size.width && circle_mask->cached_height == mask->region.size.height)
    {
        return;
    }

    circle_mask->cached_x = mask->region.location.x;
    circle_mask->cached_y = mask->region.location.y;
    circle_mask->cached_width = mask->region.size.width;
    circle_mask->cached_height = mask->region.size.height;
    circle_mask->cached_x_end = mask->region.location.x + mask->region.size.width;
    circle_mask->cached_y_end = mask->region.location.y + mask->region.size.height;
    circle_mask->center_x = mask->region.location.x + (mask->region.size.width >> 1);
    circle_mask->center_y = mask->region.location.y + (mask->region.size.height >> 1);
    circle_mask->radius = EGUI_MIN(mask->region.size.width, mask->region.size.height);
    circle_mask->radius = (circle_mask->radius >> 1) - 1;
    if (circle_mask->radius < 0)
    {
        circle_mask->radius = 0;
    }
    circle_mask->info = egui_canvas_get_circle_item(circle_mask->radius);
    circle_mask->point_cached_y = -32768;
    circle_mask->point_cached_row_index = 0;
    circle_mask->point_cached_row_valid = 0;

    for (egui_dim_t i = 0; i < EGUI_CONFIG_PFB_HEIGHT; i++)
    {
        circle_mask->row_cache_y[i] = -32768;
    }
}

__EGUI_STATIC_INLINE__ int egui_image_std_prepare_circle_mask_row_fast(egui_mask_circle_t *circle_mask, egui_dim_t screen_y, egui_dim_t *row_index,
                                                                       egui_dim_t *visible_half, egui_dim_t *opaque_boundary)
{
    egui_dim_t dy;
    egui_dim_t current_row_index;
    egui_dim_t current_visible_half;
    egui_dim_t current_opaque_boundary;
    egui_dim_t row_cache_slot;

    egui_image_std_refresh_circle_mask_fast_cache(circle_mask);

    if (screen_y < circle_mask->center_y - circle_mask->radius || screen_y > circle_mask->center_y + circle_mask->radius)
    {
        return 0;
    }

    dy = (screen_y < circle_mask->center_y) ? (circle_mask->center_y - screen_y) : (screen_y - circle_mask->center_y);
    current_row_index = circle_mask->radius - dy;
    row_cache_slot = ((uint16_t)screen_y) % EGUI_CONFIG_PFB_HEIGHT;

    if (circle_mask->row_cache_y[row_cache_slot] == screen_y)
    {
        current_visible_half = circle_mask->row_cache_visible_half[row_cache_slot];
        current_opaque_boundary = circle_mask->row_cache_opaque_boundary[row_cache_slot];
    }
    else
    {
        egui_mask_circle_get_row_metrics(circle_mask->radius, current_row_index, &current_visible_half, &current_opaque_boundary);
        circle_mask->row_cache_y[row_cache_slot] = screen_y;
        circle_mask->row_cache_visible_half[row_cache_slot] = current_visible_half;
        circle_mask->row_cache_opaque_boundary[row_cache_slot] = current_opaque_boundary;
    }

    if (row_index != NULL)
    {
        *row_index = current_row_index;
    }

    if (visible_half != NULL)
    {
        *visible_half = current_visible_half;
    }

    if (opaque_boundary != NULL)
    {
        *opaque_boundary = current_opaque_boundary;
    }

    return 1;
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_circle_masked_left_segment_fixed_row(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                                const egui_dim_t *src_x_map, egui_dim_t count, egui_dim_t screen_x,
                                                                                              egui_dim_t center_x, egui_dim_t radius, egui_dim_t row_index,
                                                                                              egui_alpha_t canvas_alpha, const egui_circle_info_t *info,
                                                                                              const egui_circle_item_t *items)
{
    egui_dim_t corner_col = screen_x - (center_x - radius);

    for (egui_dim_t i = 0; i < count; i++, corner_col++)
    {
        egui_alpha_t alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, corner_col, info, items);
        egui_dim_t src_x;

        if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
        {
            continue;
        }

        if (canvas_alpha != EGUI_ALPHA_100)
        {
            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }
        }

        src_x = egui_image_std_get_src_x(src_x_map, i);
        egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_circle_masked_segment_fixed_row_direct(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                                 egui_dim_t count, egui_dim_t corner_col, int corner_step,
                                                                                                 egui_dim_t radius, egui_dim_t row_index, egui_alpha_t canvas_alpha,
                                                                                                 const egui_circle_info_t *info, const egui_circle_item_t *items)
{
    if (canvas_alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t i = 0; i < count; i++, corner_col += corner_step)
        {
            egui_alpha_t alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, corner_col, info, items);

            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
        }
        return;
    }

    for (egui_dim_t i = 0; i < count; i++, corner_col += corner_step)
    {
        egui_alpha_t alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, corner_col, info, items);

        if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
        {
            continue;
        }

        alpha = egui_color_alpha_mix(canvas_alpha, alpha);
        if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
        {
            continue;
        }

        egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_circle_masked_right_segment_fixed_row(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                              const egui_dim_t *src_x_map, egui_dim_t count,
                                                                                              egui_dim_t screen_x, egui_dim_t center_x, egui_dim_t radius,
                                                                                              egui_dim_t row_index, egui_alpha_t canvas_alpha,
                                                                                              const egui_circle_info_t *info, const egui_circle_item_t *items)
{
    egui_dim_t corner_col = (center_x + radius) - screen_x;

    for (egui_dim_t i = 0; i < count; i++, corner_col--)
    {
        egui_alpha_t alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, corner_col, info, items);
        egui_dim_t src_x;

        if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
        {
            continue;
        }

        if (canvas_alpha != EGUI_ALPHA_100)
        {
            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }
        }

        src_x = egui_image_std_get_src_x(src_x_map, i);
        egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_circle_masked_mapped_segment_fixed_row(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                               const egui_dim_t *src_x_map, egui_dim_t count,
                                                                                               egui_dim_t screen_x, egui_dim_t center_x, egui_dim_t radius,
                                                                                               egui_dim_t row_index, egui_alpha_t canvas_alpha,
                                                                                               const egui_circle_info_t *info, const egui_circle_item_t *items)
{
    egui_dim_t processed = 0;

    if (count == 0)
    {
        return;
    }

    if (screen_x < center_x)
    {
        egui_dim_t left_count = EGUI_MIN(count, center_x - screen_x);
        if (left_count > 0)
        {
            egui_image_std_blend_rgb565_circle_masked_left_segment_fixed_row(dst_row, src_row, src_x_map, left_count, screen_x, center_x, radius, row_index,
                                                                             canvas_alpha, info, items);
            processed = left_count;
        }
    }

    if (processed < count && screen_x + processed == center_x)
    {
        egui_alpha_t alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, radius, info, items);

        if (alpha != 0)
        {
            egui_dim_t src_x = egui_image_std_get_src_x(src_x_map, processed);
            if (canvas_alpha != EGUI_ALPHA_100)
            {
                alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            }
            if (alpha != 0)
            {
                egui_image_std_blend_rgb565_src_pixel(&dst_row[processed], src_row[src_x], alpha);
            }
        }
        processed++;
    }

    if (processed < count)
    {
        egui_image_std_blend_rgb565_circle_masked_right_segment_fixed_row(&dst_row[processed], src_row, egui_image_std_get_src_x_sub_map(src_x_map, processed), count - processed,
                                                                          screen_x + processed, center_x, radius, row_index, canvas_alpha, info, items);
    }
}

__EGUI_STATIC_INLINE__ uint32_t egui_image_std_circle_isqrt(uint32_t n)
{
    uint32_t root = 0;
    uint32_t bit = 1UL << 30;

    while (bit > n)
    {
        bit >>= 2;
    }

    while (bit != 0)
    {
        if (n >= root + bit)
        {
            n -= root + bit;
            root = (root >> 1) + bit;
        }
        else
        {
            root >>= 1;
        }
        bit >>= 2;
    }

    return root;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_get_circle_opaque_boundary_fixed_row(egui_dim_t row_index, const egui_circle_info_t *info,
                                                                                      const egui_circle_item_t *items)
{
    egui_dim_t item_count = (egui_dim_t)info->item_count;
    egui_dim_t boundary;
    egui_dim_t mirror_limit = EGUI_MIN(row_index, item_count);

    if (row_index >= item_count)
    {
        boundary = item_count;
    }
    else
    {
        const egui_circle_item_t *item = &items[row_index];
        boundary = (egui_dim_t)item->start_offset + (egui_dim_t)item->valid_count;
    }

    if (mirror_limit > 0)
    {
        egui_dim_t low = 0;
        egui_dim_t high = mirror_limit;

        while (low < high)
        {
            egui_dim_t mid = low + ((high - low) >> 1);
            const egui_circle_item_t *item = &items[mid];
            egui_dim_t threshold = (egui_dim_t)item->start_offset + (egui_dim_t)item->valid_count;

            if (row_index >= threshold)
            {
                high = mid;
            }
            else
            {
                low = mid + 1;
            }
        }

        if (low < mirror_limit)
        {
            boundary = EGUI_MIN(boundary, low);
        }
    }

    return boundary;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_image_std_get_circle_visible_boundary_fixed_row(egui_dim_t row_index, const egui_circle_info_t *info,
                                                                                       const egui_circle_item_t *items)
{
    egui_dim_t item_count = (egui_dim_t)info->item_count;
    egui_dim_t boundary;
    egui_dim_t mirror_limit = EGUI_MIN(row_index, item_count);

    if (row_index >= item_count)
    {
        boundary = item_count;
    }
    else
    {
        boundary = (egui_dim_t)items[row_index].start_offset;
    }

    if (mirror_limit > 0)
    {
        egui_dim_t low = 0;
        egui_dim_t high = mirror_limit;

        while (low < high)
        {
            egui_dim_t mid = low + ((high - low) >> 1);
            if (row_index >= (egui_dim_t)items[mid].start_offset)
            {
                high = mid;
            }
            else
            {
                low = mid + 1;
            }
        }

        if (low < mirror_limit)
        {
            boundary = EGUI_MIN(boundary, low);
        }
    }

    return boundary;
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_circle_masked_partial_range_fixed_row(
        egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row, const egui_dim_t *src_x_map, egui_dim_t count,
        egui_dim_t corner_col, int corner_step, egui_dim_t radius, egui_dim_t row_index, egui_alpha_t canvas_alpha,
        const egui_circle_info_t *info, const egui_circle_item_t *items)
{
    if (info != NULL)
    {
        if (canvas_alpha == EGUI_ALPHA_100)
        {
            for (egui_dim_t i = 0; i < count; i++, corner_col += corner_step)
            {
                egui_dim_t src_x = src_x_map[i];
                egui_alpha_t alpha = src_alpha_row[src_x];
                egui_alpha_t mask_alpha;

                if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                {
                    continue;
                }

                mask_alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, corner_col, info, items);
                if (mask_alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                {
                    continue;
                }

                alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
                if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                {
                    continue;
                }

                egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
            }
            return;
        }

        for (egui_dim_t i = 0; i < count; i++, corner_col += corner_step)
        {
            egui_dim_t src_x = src_x_map[i];
            egui_alpha_t alpha = src_alpha_row[src_x];
            egui_alpha_t mask_alpha;

            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            mask_alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, corner_col, info, items);
            if (mask_alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
        }
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t i = 0; i < count; i++, corner_col += corner_step)
        {
            egui_dim_t src_x = src_x_map[i];
            egui_alpha_t alpha = src_alpha_row[src_x];
            egui_alpha_t mask_alpha;

            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            mask_alpha = egui_mask_circle_get_corner_alpha(radius, row_index, corner_col);
            if (mask_alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
        }
        return;
    }

    for (egui_dim_t i = 0; i < count; i++, corner_col += corner_step)
    {
        egui_dim_t src_x = src_x_map[i];
        egui_alpha_t alpha = src_alpha_row[src_x];
        egui_alpha_t mask_alpha;

        if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
        {
            continue;
        }

        mask_alpha = egui_mask_circle_get_corner_alpha(radius, row_index, corner_col);
        if (mask_alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
        {
            continue;
        }

        alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
        if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
        {
            continue;
        }

        alpha = egui_color_alpha_mix(canvas_alpha, alpha);
        if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
        {
            continue;
        }

        egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_circle_masked_segment_fixed_row_direct(
        egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row, egui_dim_t count, egui_dim_t corner_col, int corner_step,
        egui_dim_t radius, egui_dim_t row_index, egui_alpha_t canvas_alpha, const egui_circle_info_t *info, const egui_circle_item_t *items)
{
    if (info != NULL)
    {
        if (canvas_alpha == EGUI_ALPHA_100)
        {
            for (egui_dim_t i = 0; i < count; i++, corner_col += corner_step)
            {
                egui_alpha_t alpha = src_alpha_row[i];
                egui_alpha_t mask_alpha;

                if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                {
                    continue;
                }

                mask_alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, corner_col, info, items);
                if (mask_alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                {
                    continue;
                }

                alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
                if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
                {
                    continue;
                }

                egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
            }
            return;
        }

        for (egui_dim_t i = 0; i < count; i++, corner_col += corner_step)
        {
            egui_alpha_t alpha = src_alpha_row[i];
            egui_alpha_t mask_alpha;

            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            mask_alpha = egui_image_std_get_circle_corner_alpha_fixed_row(radius, row_index, corner_col, info, items);
            if (mask_alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
        }
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t i = 0; i < count; i++, corner_col += corner_step)
        {
            egui_alpha_t alpha = src_alpha_row[i];
            egui_alpha_t mask_alpha;

            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            mask_alpha = egui_mask_circle_get_corner_alpha(radius, row_index, corner_col);
            if (mask_alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
            if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
        }
        return;
    }

    for (egui_dim_t i = 0; i < count; i++, corner_col += corner_step)
    {
        egui_alpha_t alpha = src_alpha_row[i];
        egui_alpha_t mask_alpha;

        if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
        {
            continue;
        }

        mask_alpha = egui_mask_circle_get_corner_alpha(radius, row_index, corner_col);
        if (mask_alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
        {
            continue;
        }

        alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
        if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
        {
            continue;
        }

        alpha = egui_color_alpha_mix(canvas_alpha, alpha);
        if (alpha < EGUI_IMAGE_STD_RGB565_MIN_VISIBLE_ALPHA)
        {
            continue;
        }

        egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_circle_masked_mapped_segment_fixed_row(
        egui_mask_circle_t *circle_mask, egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row, const egui_dim_t *src_x_map,
        egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y, egui_dim_t center_x, egui_dim_t radius, egui_dim_t row_index, egui_alpha_t canvas_alpha,
        const egui_circle_info_t *info, const egui_circle_item_t *items)
{
    egui_dim_t screen_x_end = screen_x + count;
    egui_dim_t visible_half;
    egui_dim_t opaque_boundary;
    egui_dim_t row_cache_slot = ((uint16_t)screen_y) % EGUI_CONFIG_PFB_HEIGHT;
    egui_dim_t seg_start;
    egui_dim_t seg_end;
    egui_dim_t opaque_start;
    egui_dim_t opaque_end;

    if (count == 0)
    {
        return;
    }

    if (circle_mask->row_cache_y[row_cache_slot] == screen_y)
    {
        visible_half = circle_mask->row_cache_visible_half[row_cache_slot];
        opaque_boundary = circle_mask->row_cache_opaque_boundary[row_cache_slot];
    }
    else
    {
        egui_mask_circle_get_row_metrics(radius, row_index, &visible_half, &opaque_boundary);
        circle_mask->row_cache_y[row_cache_slot] = screen_y;
        circle_mask->row_cache_visible_half[row_cache_slot] = visible_half;
        circle_mask->row_cache_opaque_boundary[row_cache_slot] = opaque_boundary;
    }

    seg_start = EGUI_MAX(screen_x, center_x - visible_half);
    seg_end = EGUI_MIN(screen_x_end, center_x + visible_half + 1);
    if (seg_start >= seg_end)
    {
        return;
    }

    opaque_start = center_x - radius + opaque_boundary;
    opaque_end = center_x + radius - opaque_boundary + 1;

    {
        egui_dim_t left_end = EGUI_MIN(seg_end, opaque_start);

        if (seg_start < left_end)
        {
            egui_dim_t left_offset = seg_start - screen_x;

            egui_image_std_blend_rgb565_alpha8_circle_masked_partial_range_fixed_row(
                    &dst_row[left_offset], src_row, src_alpha_row, egui_image_std_get_src_x_sub_map(src_x_map, left_offset), left_end - seg_start,
                    seg_start - (center_x - radius), 1, radius,
                    row_index, canvas_alpha, info, items);
        }
    }

    {
        egui_dim_t mid_start = EGUI_MAX(seg_start, opaque_start);
        egui_dim_t mid_end = EGUI_MIN(seg_end, opaque_end);

        if (mid_start < mid_end)
        {
            egui_image_std_blend_rgb565_alpha8_mapped_row(&dst_row[mid_start - screen_x], src_row, src_alpha_row,
                                                          egui_image_std_get_src_x_sub_map(src_x_map, mid_start - screen_x), mid_end - mid_start, canvas_alpha);
        }
    }

    {
        egui_dim_t right_start = EGUI_MAX(seg_start, opaque_end);

        if (right_start < seg_end)
        {
            egui_dim_t right_offset = right_start - screen_x;

            egui_image_std_blend_rgb565_alpha8_circle_masked_partial_range_fixed_row(
                    &dst_row[right_offset], src_row, src_alpha_row, egui_image_std_get_src_x_sub_map(src_x_map, right_offset), seg_end - right_start,
                    center_x + radius - right_start, -1,
                    radius, row_index, canvas_alpha, info, items);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_round_rect_masked_left_segment_fixed_row(
        egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row, egui_dim_t count, egui_dim_t screen_x,
        egui_dim_t mask_x, egui_dim_t row_index, egui_alpha_t canvas_alpha, const egui_circle_info_t *info, const egui_circle_item_t *items)
{
    egui_dim_t corner_col = screen_x - mask_x;
    egui_dim_t i = 0;
    egui_dim_t end_index = count;

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        if (corner_col < row_index)
        {
            egui_dim_t mirror_end = EGUI_MIN(end_index, row_index - corner_col);

            for (; i < mirror_end; i++, corner_col++)
            {
                egui_alpha_t alpha = src_alpha_row[i];

                if (alpha == 0)
                {
                    continue;
                }

                {
                    const egui_circle_item_t *item = &items[corner_col];
                    egui_dim_t start_offset = (egui_dim_t)item->start_offset;
                    egui_dim_t valid_count = (egui_dim_t)item->valid_count;
                    egui_dim_t opaque_start = start_offset + valid_count;

                    if (row_index < start_offset)
                    {
                        continue;
                    }

                    if (row_index < opaque_start)
                    {
                        egui_alpha_t mask_alpha = info->data[item->data_offset + row_index - start_offset];

                        if (mask_alpha == 0)
                        {
                            continue;
                        }

                        alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
                    }
                }

                egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
            }
        }

        if (i < end_index)
        {
            const egui_circle_item_t *row_item = &items[row_index];
            egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
            egui_dim_t row_valid_count = (egui_dim_t)row_item->valid_count;
            egui_dim_t row_opaque_start = row_start + row_valid_count;
            const uint8_t *row_data = &info->data[row_item->data_offset];

            if (corner_col < row_start)
            {
                egui_dim_t skip = EGUI_MIN(end_index - i, row_start - corner_col);
                i += skip;
                corner_col += skip;
            }

            for (; i < end_index; i++, corner_col++)
            {
                egui_alpha_t alpha = src_alpha_row[i];

                if (alpha == 0)
                {
                    continue;
                }

                if (corner_col < row_opaque_start)
                {
                    egui_alpha_t mask_alpha = row_data[corner_col - row_start];

                    if (mask_alpha == 0)
                    {
                        continue;
                    }

                    alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
                }

                egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
            }
        }
        return;
    }

    if (corner_col < row_index)
    {
        egui_dim_t mirror_end = EGUI_MIN(end_index, row_index - corner_col);

        for (; i < mirror_end; i++, corner_col++)
        {
            egui_alpha_t alpha = src_alpha_row[i];

            if (alpha == 0)
            {
                continue;
            }

            {
                const egui_circle_item_t *item = &items[corner_col];
                egui_dim_t start_offset = (egui_dim_t)item->start_offset;
                egui_dim_t valid_count = (egui_dim_t)item->valid_count;
                egui_dim_t opaque_start = start_offset + valid_count;

                if (row_index < start_offset)
                {
                    continue;
                }

                if (row_index < opaque_start)
                {
                    egui_alpha_t mask_alpha = info->data[item->data_offset + row_index - start_offset];

                    if (mask_alpha == 0)
                    {
                        continue;
                    }

                    alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
                }
            }

            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            if (alpha == 0)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
        }
    }

    if (i < end_index)
    {
        const egui_circle_item_t *row_item = &items[row_index];
        egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
        egui_dim_t row_valid_count = (egui_dim_t)row_item->valid_count;
        egui_dim_t row_opaque_start = row_start + row_valid_count;
        const uint8_t *row_data = &info->data[row_item->data_offset];

        if (corner_col < row_start)
        {
            egui_dim_t skip = EGUI_MIN(end_index - i, row_start - corner_col);
            i += skip;
            corner_col += skip;
        }

        for (; i < end_index; i++, corner_col++)
        {
            egui_alpha_t alpha = src_alpha_row[i];

            if (alpha == 0)
            {
                continue;
            }

            if (corner_col < row_opaque_start)
            {
                egui_alpha_t mask_alpha = row_data[corner_col - row_start];

                if (mask_alpha == 0)
                {
                    continue;
                }

                alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
            }

            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            if (alpha == 0)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha8_round_rect_masked_right_segment_fixed_row(
        egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row, egui_dim_t count, egui_dim_t screen_x,
        egui_dim_t mask_x_end, egui_dim_t row_index, egui_alpha_t canvas_alpha, const egui_circle_info_t *info, const egui_circle_item_t *items)
{
    egui_dim_t corner_col = mask_x_end - 1 - screen_x;
    egui_dim_t i = 0;
    egui_dim_t end_index = count;

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        if (corner_col > row_index)
        {
            const egui_circle_item_t *row_item = &items[row_index];
            egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
            egui_dim_t row_valid_count = (egui_dim_t)row_item->valid_count;
            egui_dim_t row_opaque_start = row_start + row_valid_count;
            const uint8_t *row_data = &info->data[row_item->data_offset];
            egui_dim_t row_phase_end = EGUI_MIN(end_index, corner_col - row_index);

            for (; i < row_phase_end; i++, corner_col--)
            {
                egui_alpha_t alpha = src_alpha_row[i];

                if (alpha == 0 || corner_col < row_start)
                {
                    continue;
                }

                if (corner_col < row_opaque_start)
                {
                    egui_alpha_t mask_alpha = row_data[corner_col - row_start];

                    if (mask_alpha == 0)
                    {
                        continue;
                    }

                    alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
                }

                egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
            }
        }

        for (; i < end_index; i++, corner_col--)
        {
            egui_alpha_t alpha = src_alpha_row[i];

            if (alpha == 0)
            {
                continue;
            }

            {
                const egui_circle_item_t *item = &items[corner_col];
                egui_dim_t start_offset = (egui_dim_t)item->start_offset;
                egui_dim_t valid_count = (egui_dim_t)item->valid_count;
                egui_dim_t opaque_start = start_offset + valid_count;

                if (row_index < start_offset)
                {
                    continue;
                }

                if (row_index < opaque_start)
                {
                    egui_alpha_t mask_alpha = info->data[item->data_offset + row_index - start_offset];

                    if (mask_alpha == 0)
                    {
                        continue;
                    }

                    alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
                }
            }

            egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
        }
        return;
    }

    if (corner_col > row_index)
    {
        const egui_circle_item_t *row_item = &items[row_index];
        egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
        egui_dim_t row_valid_count = (egui_dim_t)row_item->valid_count;
        egui_dim_t row_opaque_start = row_start + row_valid_count;
        const uint8_t *row_data = &info->data[row_item->data_offset];
        egui_dim_t row_phase_end = EGUI_MIN(end_index, corner_col - row_index);

        for (; i < row_phase_end; i++, corner_col--)
        {
            egui_alpha_t alpha = src_alpha_row[i];

            if (alpha == 0 || corner_col < row_start)
            {
                continue;
            }

            if (corner_col < row_opaque_start)
            {
                egui_alpha_t mask_alpha = row_data[corner_col - row_start];

                if (mask_alpha == 0)
                {
                    continue;
                }

                alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
            }

            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            if (alpha == 0)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
        }
    }

    for (; i < end_index; i++, corner_col--)
    {
        egui_alpha_t alpha = src_alpha_row[i];

        if (alpha == 0)
        {
            continue;
        }

        {
            const egui_circle_item_t *item = &items[corner_col];
            egui_dim_t start_offset = (egui_dim_t)item->start_offset;
            egui_dim_t valid_count = (egui_dim_t)item->valid_count;
            egui_dim_t opaque_start = start_offset + valid_count;

            if (row_index < start_offset)
            {
                continue;
            }

            if (row_index < opaque_start)
            {
                egui_alpha_t mask_alpha = info->data[item->data_offset + row_index - start_offset];

                if (mask_alpha == 0)
                {
                    continue;
                }

                alpha = (alpha == EGUI_ALPHA_100) ? mask_alpha : egui_color_alpha_mix(mask_alpha, alpha);
            }
        }

        alpha = egui_color_alpha_mix(canvas_alpha, alpha);
        if (alpha == 0)
        {
            continue;
        }

        egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_round_rect_masked_left_segment_fixed_row(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                                 const egui_dim_t *src_x_map, egui_dim_t count,
                                                                                                 egui_dim_t screen_x, egui_dim_t mask_x, egui_dim_t row_index,
                                                                                                 const egui_circle_info_t *info,
                                                                                                 const egui_circle_item_t *items)
{
    egui_dim_t corner_col = screen_x - mask_x;
    egui_dim_t i = 0;
    egui_dim_t end_index = count;
    egui_dim_t item_count = (egui_dim_t)info->item_count;

    if (corner_col < row_index)
    {
        egui_dim_t mirror_end = EGUI_MIN(end_index, row_index - corner_col);
        for (; i < mirror_end; i++, corner_col++)
        {
            egui_alpha_t alpha;

            {
                const egui_circle_item_t *item = &items[corner_col];
                egui_dim_t start_offset = (egui_dim_t)item->start_offset;
                egui_dim_t valid_count = (egui_dim_t)item->valid_count;
                egui_dim_t opaque_start = start_offset + valid_count;

                if (row_index < start_offset)
                {
                    continue;
                }

                if (row_index >= opaque_start)
                {
                    alpha = EGUI_ALPHA_100;
                }
                else
                {
                    alpha = info->data[item->data_offset + row_index - start_offset];
                }
            }

            if (alpha == 0)
            {
                continue;
            }

            {
                egui_dim_t src_x = src_x_map[i];
                egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
            }
        }
    }

    if (i < end_index && row_index < item_count)
    {
        const egui_circle_item_t *row_item = &items[row_index];
        egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
        egui_dim_t row_valid_count = (egui_dim_t)row_item->valid_count;
        egui_dim_t row_opaque_start = row_start + row_valid_count;
        const uint8_t *row_data = &info->data[row_item->data_offset];

        if (corner_col < row_start)
        {
            egui_dim_t skip = EGUI_MIN(end_index - i, row_start - corner_col);
            i += skip;
            corner_col += skip;
        }

        for (; i < end_index; i++, corner_col++)
        {
            egui_alpha_t alpha;

            if (corner_col >= row_opaque_start)
            {
                alpha = EGUI_ALPHA_100;
            }
            else
            {
                alpha = row_data[corner_col - row_start];
            }

            if (alpha == 0)
            {
                continue;
            }

            {
                egui_dim_t src_x = src_x_map[i];
                egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
            }
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_round_rect_masked_left_segment_fixed_row_direct(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                                         egui_dim_t count, egui_dim_t screen_x, egui_dim_t mask_x,
                                                                                                         egui_dim_t row_index, const egui_circle_info_t *info,
                                                                                                         const egui_circle_item_t *items)
{
    egui_dim_t corner_col = screen_x - mask_x;
    egui_dim_t i = 0;
    egui_dim_t end_index = count;
    egui_dim_t item_count = (egui_dim_t)info->item_count;

    if (corner_col < row_index)
    {
        egui_dim_t mirror_end = EGUI_MIN(end_index, row_index - corner_col);

        for (; i < mirror_end; i++, corner_col++)
        {
            egui_alpha_t alpha;

            {
                const egui_circle_item_t *item = &items[corner_col];
                egui_dim_t start_offset = (egui_dim_t)item->start_offset;
                egui_dim_t valid_count = (egui_dim_t)item->valid_count;
                egui_dim_t opaque_start = start_offset + valid_count;

                if (row_index < start_offset)
                {
                    continue;
                }

                if (row_index >= opaque_start)
                {
                    alpha = EGUI_ALPHA_100;
                }
                else
                {
                    alpha = info->data[item->data_offset + row_index - start_offset];
                }
            }

            if (alpha == 0)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
        }
    }

    if (i < end_index && row_index < item_count)
    {
        const egui_circle_item_t *row_item = &items[row_index];
        egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
        egui_dim_t row_valid_count = (egui_dim_t)row_item->valid_count;
        egui_dim_t row_opaque_start = row_start + row_valid_count;
        const uint8_t *row_data = &info->data[row_item->data_offset];

        if (corner_col < row_start)
        {
            egui_dim_t skip = EGUI_MIN(end_index - i, row_start - corner_col);
            i += skip;
            corner_col += skip;
        }

        for (; i < end_index; i++, corner_col++)
        {
            egui_alpha_t alpha;

            if (corner_col >= row_opaque_start)
            {
                alpha = EGUI_ALPHA_100;
            }
            else
            {
                alpha = row_data[corner_col - row_start];
            }

            if (alpha == 0)
            {
                continue;
            }

            egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_round_rect_masked_right_segment_fixed_row(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                                  const egui_dim_t *src_x_map, egui_dim_t count,
                                                                                                  egui_dim_t screen_x, egui_dim_t mask_x_end,
                                                                                                  egui_dim_t row_index, const egui_circle_info_t *info,
                                                                                                  const egui_circle_item_t *items)
{
    egui_dim_t corner_col = mask_x_end - 1 - screen_x;
    egui_dim_t i = 0;
    egui_dim_t end_index = count;
    egui_dim_t item_count = (egui_dim_t)info->item_count;

    if (corner_col > row_index && row_index < item_count)
    {
        const egui_circle_item_t *row_item = &items[row_index];
        egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
        egui_dim_t row_valid_count = (egui_dim_t)row_item->valid_count;
        egui_dim_t row_opaque_start = row_start + row_valid_count;
        const uint8_t *row_data = &info->data[row_item->data_offset];
        egui_dim_t row_phase_end = EGUI_MIN(end_index, corner_col - row_index);

        for (; i < row_phase_end; i++, corner_col--)
        {
            if (corner_col < row_start)
            {
                continue;
            }

            {
                egui_alpha_t alpha;

                if (corner_col >= row_opaque_start)
                {
                    alpha = EGUI_ALPHA_100;
                }
                else
                {
                    alpha = row_data[corner_col - row_start];
                }

                if (alpha == 0)
                {
                    continue;
                }

                {
                    egui_dim_t src_x = src_x_map[i];
                    egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
                }
            }
        }
    }

    for (; i < end_index; i++, corner_col--)
    {
        egui_alpha_t alpha;

        {
            const egui_circle_item_t *item = &items[corner_col];
            egui_dim_t start_offset = (egui_dim_t)item->start_offset;
            egui_dim_t valid_count = (egui_dim_t)item->valid_count;
            egui_dim_t opaque_start = start_offset + valid_count;

            if (row_index < start_offset)
            {
                continue;
            }

            if (row_index >= opaque_start)
            {
                alpha = EGUI_ALPHA_100;
            }
            else
            {
                alpha = info->data[item->data_offset + row_index - start_offset];
            }
        }

        if (alpha == 0)
        {
            continue;
        }

        {
            egui_dim_t src_x = src_x_map[i];
            egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[src_x], alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_round_rect_masked_right_segment_fixed_row_direct(egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                                          egui_dim_t count, egui_dim_t screen_x,
                                                                                                          egui_dim_t mask_x_end, egui_dim_t row_index,
                                                                                                          const egui_circle_info_t *info,
                                                                                                          const egui_circle_item_t *items)
{
    egui_dim_t corner_col = mask_x_end - 1 - screen_x;
    egui_dim_t i = 0;
    egui_dim_t end_index = count;
    egui_dim_t item_count = (egui_dim_t)info->item_count;

    if (corner_col > row_index && row_index < item_count)
    {
        const egui_circle_item_t *row_item = &items[row_index];
        egui_dim_t row_start = (egui_dim_t)row_item->start_offset;
        egui_dim_t row_valid_count = (egui_dim_t)row_item->valid_count;
        egui_dim_t row_opaque_start = row_start + row_valid_count;
        const uint8_t *row_data = &info->data[row_item->data_offset];
        egui_dim_t row_phase_end = EGUI_MIN(end_index, corner_col - row_index);

        for (; i < row_phase_end; i++, corner_col--)
        {
            if (corner_col < row_start)
            {
                continue;
            }

            {
                egui_alpha_t alpha;

                if (corner_col >= row_opaque_start)
                {
                    alpha = EGUI_ALPHA_100;
                }
                else
                {
                    alpha = row_data[corner_col - row_start];
                }

                if (alpha == 0)
                {
                    continue;
                }

                egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
            }
        }
    }

    for (; i < end_index; i++, corner_col--)
    {
        egui_alpha_t alpha;

        {
            const egui_circle_item_t *item = &items[corner_col];
            egui_dim_t start_offset = (egui_dim_t)item->start_offset;
            egui_dim_t valid_count = (egui_dim_t)item->valid_count;
            egui_dim_t opaque_start = start_offset + valid_count;

            if (row_index < start_offset)
            {
                continue;
            }

            if (row_index >= opaque_start)
            {
                alpha = EGUI_ALPHA_100;
            }
            else
            {
                alpha = info->data[item->data_offset + row_index - start_offset];
            }
        }

        if (alpha == 0)
        {
            continue;
        }

        egui_image_std_blend_rgb565_src_pixel(&dst_row[i], src_row[i], alpha);
    }
}

typedef struct
{
    const egui_mask_t *mask;
    egui_dim_t cached_x;
    egui_dim_t cached_y;
    egui_dim_t cached_width;
    egui_dim_t cached_height;
    egui_dim_t cached_radius;
    egui_dim_t row_cache_y[EGUI_CONFIG_PFB_HEIGHT];
    egui_dim_t row_cache_visible_boundary[EGUI_CONFIG_PFB_HEIGHT];
    egui_dim_t row_cache_opaque_boundary[EGUI_CONFIG_PFB_HEIGHT];
    const egui_circle_info_t *info;
} egui_image_std_round_rect_fast_cache_t;

__EGUI_STATIC_INLINE__ void egui_image_std_round_rect_fast_cache_invalidate(egui_image_std_round_rect_fast_cache_t *cache)
{
    if (cache == NULL)
    {
        return;
    }

    for (egui_dim_t i = 0; i < EGUI_CONFIG_PFB_HEIGHT; i++)
    {
        cache->row_cache_y[i] = -32768;
    }
}

__EGUI_STATIC_INLINE__ void egui_image_std_round_rect_fast_cache_init(egui_image_std_round_rect_fast_cache_t *cache)
{
    if (cache == NULL)
    {
        return;
    }

    cache->mask = NULL;
    cache->cached_x = -1;
    cache->cached_y = -1;
    cache->cached_width = -1;
    cache->cached_height = -1;
    cache->cached_radius = -1;
    cache->info = NULL;
    egui_image_std_round_rect_fast_cache_invalidate(cache);
}

__EGUI_STATIC_INLINE__ int egui_image_std_round_rect_fast_cache_prepare(egui_image_std_round_rect_fast_cache_t *cache, const egui_mask_t *mask,
                                                                        egui_dim_t radius)
{
    if (cache == NULL || mask == NULL)
    {
        return 0;
    }

    if (cache->mask != mask)
    {
        cache->mask = mask;
        cache->cached_x = -1;
        cache->cached_y = -1;
        cache->cached_width = -1;
        cache->cached_height = -1;
        cache->cached_radius = -1;
        cache->info = NULL;
        egui_image_std_round_rect_fast_cache_invalidate(cache);
    }

    if (cache->cached_x == mask->region.location.x && cache->cached_y == mask->region.location.y && cache->cached_width == mask->region.size.width &&
        cache->cached_height == mask->region.size.height && cache->cached_radius == radius)
    {
        return cache->info != NULL;
    }

    cache->cached_x = mask->region.location.x;
    cache->cached_y = mask->region.location.y;
    cache->cached_width = mask->region.size.width;
    cache->cached_height = mask->region.size.height;
    cache->cached_radius = radius;
    cache->info = egui_canvas_get_circle_item(radius);
    egui_image_std_round_rect_fast_cache_invalidate(cache);
    return cache->info != NULL;
}

__EGUI_STATIC_INLINE__ int egui_image_std_round_rect_fast_cache_get_boundaries(egui_image_std_round_rect_fast_cache_t *cache, egui_dim_t screen_y,
                                                                               egui_dim_t radius, egui_dim_t *row_index, egui_dim_t *visible_boundary,
                                                                               egui_dim_t *opaque_boundary)
{
    egui_dim_t row_cache_slot;
    const egui_circle_info_t *info;
    const egui_circle_item_t *items;

    if (cache == NULL || cache->info == NULL || row_index == NULL || visible_boundary == NULL || opaque_boundary == NULL)
    {
        return 0;
    }

    info = cache->info;
    items = (const egui_circle_item_t *)info->items;
    *row_index = (screen_y < cache->cached_y + radius) ? (screen_y - cache->cached_y) : (cache->cached_y + cache->cached_height - 1 - screen_y);
    row_cache_slot = ((uint16_t)screen_y) % EGUI_CONFIG_PFB_HEIGHT;

    if (cache->row_cache_y[row_cache_slot] != screen_y)
    {
        cache->row_cache_y[row_cache_slot] = screen_y;
        cache->row_cache_visible_boundary[row_cache_slot] = egui_image_std_get_circle_visible_boundary_fixed_row(*row_index, info, items);
        cache->row_cache_opaque_boundary[row_cache_slot] = egui_image_std_get_circle_opaque_boundary_fixed_row(*row_index, info, items);
    }

    *visible_boundary = cache->row_cache_visible_boundary[row_cache_slot];
    *opaque_boundary = cache->row_cache_opaque_boundary[row_cache_slot];
    return 1;
}

__EGUI_STATIC_INLINE__ int egui_image_std_blend_rgb565_alpha8_round_rect_masked_row_fast_with_cache(egui_canvas_t *canvas,
                                                                                                     egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                                     const uint8_t *src_alpha_row, egui_dim_t count,
                                                                                                     egui_dim_t screen_x, egui_dim_t screen_y,
                                                                                                     egui_alpha_t canvas_alpha,
                                                                                                     egui_image_std_round_rect_fast_cache_t *round_rect_cache)
{
    egui_mask_round_rectangle_t *round_rect_mask = (egui_mask_round_rectangle_t *)canvas->mask;
    egui_dim_t radius = round_rect_mask->radius;
    egui_dim_t x_end = screen_x + count;
    const egui_circle_info_t *info;
    const egui_circle_item_t *items;
    egui_dim_t round_rect_x;
    egui_dim_t round_rect_y;
    egui_dim_t round_rect_x_end;
    egui_dim_t round_rect_y_end;
    egui_dim_t rr_x_start;
    egui_dim_t rr_x_end;
    egui_dim_t visible_x_start;
    egui_dim_t visible_x_end;

    if (count == 0)
    {
        return 1;
    }

    if (!egui_image_std_round_rect_fast_cache_prepare(round_rect_cache, canvas->mask, radius))
    {
        egui_image_std_blend_rgb565_alpha8_masked_mapped_segment(canvas, dst_row, src_row, src_alpha_row, NULL, count, screen_x, screen_y, canvas_alpha);
        return 1;
    }

    round_rect_x = round_rect_cache->cached_x;
    round_rect_y = round_rect_cache->cached_y;
    round_rect_x_end = round_rect_x + round_rect_cache->cached_width;
    round_rect_y_end = round_rect_y + round_rect_cache->cached_height;

    if (screen_y < round_rect_y || screen_y >= round_rect_y_end)
    {
        return 1;
    }

    rr_x_start = EGUI_MAX(round_rect_x, screen_x);
    rr_x_end = EGUI_MIN(round_rect_x_end, x_end);
    if (rr_x_start >= rr_x_end)
    {
        return 1;
    }

    if (radius <= 0 || (screen_y >= round_rect_y + radius && screen_y < round_rect_y_end - radius))
    {
        egui_dim_t mid_offset = rr_x_start - screen_x;

        egui_image_std_blend_rgb565_alpha8_row(&dst_row[mid_offset], &src_row[mid_offset], &src_alpha_row[mid_offset], rr_x_end - rr_x_start, canvas_alpha);
        return 1;
    }

    {
        egui_dim_t row_index;
        egui_dim_t visible_boundary;
        egui_dim_t opaque_boundary;

        info = round_rect_cache->info;
        if (info == NULL)
        {
            return 0;
        }

        items = (const egui_circle_item_t *)info->items;
        if (!egui_image_std_round_rect_fast_cache_get_boundaries(round_rect_cache, screen_y, radius, &row_index, &visible_boundary, &opaque_boundary))
        {
            return 0;
        }

        visible_x_start = EGUI_MAX(round_rect_x + visible_boundary, screen_x);
        visible_x_end = EGUI_MIN(round_rect_x_end - visible_boundary, x_end);
        if (visible_x_start >= visible_x_end)
        {
            return 1;
        }

        rr_x_start = EGUI_MAX(round_rect_x + opaque_boundary, screen_x);
        rr_x_end = EGUI_MIN(round_rect_x_end - opaque_boundary, x_end);

        {
            egui_dim_t left_x_end = EGUI_MIN(rr_x_start, visible_x_end);

            if (visible_x_start < left_x_end)
            {
                egui_dim_t left_offset = visible_x_start - screen_x;

                egui_image_std_blend_rgb565_alpha8_round_rect_masked_left_segment_fixed_row(
                        &dst_row[left_offset], &src_row[left_offset], &src_alpha_row[left_offset], left_x_end - visible_x_start, visible_x_start,
                        round_rect_cache->cached_x, row_index, canvas_alpha, info, items);
            }
        }

        if (rr_x_start < rr_x_end)
        {
            egui_dim_t mid_offset = rr_x_start - screen_x;

            egui_image_std_blend_rgb565_alpha8_row(&dst_row[mid_offset], &src_row[mid_offset], &src_alpha_row[mid_offset], rr_x_end - rr_x_start,
                                                   canvas_alpha);
        }

        {
            egui_dim_t right_x_start = EGUI_MAX(rr_x_end, visible_x_start);

            if (right_x_start < visible_x_end)
            {
                egui_dim_t right_offset = right_x_start - screen_x;

                egui_image_std_blend_rgb565_alpha8_round_rect_masked_right_segment_fixed_row(
                        &dst_row[right_offset], &src_row[right_offset], &src_alpha_row[right_offset], visible_x_end - right_x_start, right_x_start,
                        round_rect_cache->cached_x + round_rect_cache->cached_width, row_index, canvas_alpha, info, items);
            }
        }
    }

    return 1;
}

__EGUI_STATIC_INLINE__ int egui_image_std_blend_rgb565_alpha8_round_rect_masked_row_fast(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                          const uint8_t *src_alpha_row, egui_dim_t count, egui_dim_t screen_x,
                                                                                          egui_dim_t screen_y, egui_alpha_t canvas_alpha)
{
    egui_image_std_round_rect_fast_cache_t round_rect_cache;

    egui_image_std_round_rect_fast_cache_init(&round_rect_cache);
    return egui_image_std_blend_rgb565_alpha8_round_rect_masked_row_fast_with_cache(canvas, dst_row, src_row, src_alpha_row, count, screen_x, screen_y,
                                                                                     canvas_alpha, &round_rect_cache);
}

__EGUI_STATIC_INLINE__ int egui_image_std_blend_rgb565_alpha8_circle_masked_row_fast(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                      const uint8_t *src_alpha_row, egui_dim_t count, egui_dim_t screen_x,
                                                                                      egui_dim_t screen_y, egui_alpha_t canvas_alpha)
{
    egui_mask_circle_t *circle_mask = (egui_mask_circle_t *)canvas->mask;
    egui_dim_t row_index;
    egui_dim_t screen_x_end = screen_x + count;
    egui_dim_t visible_half;
    egui_dim_t opaque_boundary;
    egui_dim_t seg_start;
    egui_dim_t seg_end;
    egui_dim_t opaque_start;
    egui_dim_t opaque_end;
    egui_dim_t rr_x_start;
    egui_dim_t rr_x_end;

    if (count == 0)
    {
        return 1;
    }

    if (!egui_image_std_prepare_circle_mask_row_fast(circle_mask, screen_y, &row_index, &visible_half, &opaque_boundary))
    {
        return 1;
    }

    seg_start = EGUI_MAX(screen_x, circle_mask->center_x - visible_half);
    seg_end = EGUI_MIN(screen_x_end, circle_mask->center_x + visible_half + 1);
    if (seg_start >= seg_end)
    {
        return 1;
    }

    opaque_start = circle_mask->center_x - circle_mask->radius + opaque_boundary;
    opaque_end = circle_mask->center_x + circle_mask->radius - opaque_boundary + 1;
    rr_x_start = EGUI_MAX(opaque_start, screen_x);
    rr_x_end = EGUI_MIN(opaque_end, screen_x_end);

    if (rr_x_start <= screen_x && rr_x_end >= screen_x_end)
    {
        egui_image_std_blend_rgb565_alpha8_row(dst_row, src_row, src_alpha_row, count, canvas_alpha);
        return 1;
    }

    {
        egui_dim_t left_end = EGUI_MIN(seg_end, opaque_start);

        if (seg_start < left_end)
        {
            egui_dim_t left_offset = seg_start - screen_x;

            egui_image_std_blend_rgb565_alpha8_circle_masked_segment_fixed_row_direct(
                    &dst_row[left_offset], &src_row[left_offset], &src_alpha_row[left_offset], left_end - seg_start,
                    seg_start - (circle_mask->center_x - circle_mask->radius), 1, circle_mask->radius, row_index, canvas_alpha, circle_mask->info,
                    (circle_mask->info != NULL) ? (const egui_circle_item_t *)circle_mask->info->items : NULL);
        }
    }

    {
        egui_dim_t mid_start = EGUI_MAX(seg_start, opaque_start);
        egui_dim_t mid_end = EGUI_MIN(seg_end, opaque_end);

        if (mid_start < mid_end)
        {
            egui_dim_t mid_offset = mid_start - screen_x;

            egui_image_std_blend_rgb565_alpha8_row(&dst_row[mid_offset], &src_row[mid_offset], &src_alpha_row[mid_offset], mid_end - mid_start, canvas_alpha);
        }
    }

    {
        egui_dim_t right_start = EGUI_MAX(seg_start, opaque_end);

        if (right_start < seg_end)
        {
            egui_dim_t right_offset = right_start - screen_x;

            egui_image_std_blend_rgb565_alpha8_circle_masked_segment_fixed_row_direct(
                    &dst_row[right_offset], &src_row[right_offset], &src_alpha_row[right_offset], seg_end - right_start,
                    circle_mask->center_x + circle_mask->radius - right_start, -1, circle_mask->radius, row_index, canvas_alpha, circle_mask->info,
                    (circle_mask->info != NULL) ? (const egui_circle_item_t *)circle_mask->info->items : NULL);
        }
    }

    return 1;
}

__EGUI_STATIC_INLINE__ int egui_image_std_blend_rgb565_circle_masked_row_fast(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y,
                                                                                egui_alpha_t canvas_alpha)
{
    egui_mask_circle_t *circle_mask = (egui_mask_circle_t *)canvas->mask;
    egui_dim_t row_index;
    egui_dim_t screen_x_end = screen_x + count;
    egui_dim_t visible_half;
    egui_dim_t opaque_boundary;
    egui_dim_t seg_start;
    egui_dim_t seg_end;
    egui_dim_t opaque_start;
    egui_dim_t opaque_end;
    egui_dim_t rr_x_start;
    egui_dim_t rr_x_end;

    if (count == 0)
    {
        return 1;
    }

    if (!egui_image_std_prepare_circle_mask_row_fast(circle_mask, screen_y, &row_index, &visible_half, &opaque_boundary))
    {
        return 1;
    }

    seg_start = EGUI_MAX(screen_x, circle_mask->center_x - visible_half);
    seg_end = EGUI_MIN(screen_x_end, circle_mask->center_x + visible_half + 1);
    if (seg_start >= seg_end)
    {
        return 1;
    }

    opaque_start = circle_mask->center_x - circle_mask->radius + opaque_boundary;
    opaque_end = circle_mask->center_x + circle_mask->radius - opaque_boundary + 1;
    rr_x_start = EGUI_MAX(opaque_start, screen_x);
    rr_x_end = EGUI_MIN(opaque_end, screen_x_end);

    if (rr_x_start <= screen_x && rr_x_end >= screen_x_end)
    {
        egui_image_std_blend_rgb565_row(dst_row, src_row, count, canvas_alpha);
        return 1;
    }

    {
        egui_dim_t left_end = EGUI_MIN(seg_end, opaque_start);

        if (seg_start < left_end)
        {
            egui_dim_t left_offset = seg_start - screen_x;

            egui_image_std_blend_rgb565_circle_masked_segment_fixed_row_direct(
                    &dst_row[left_offset], &src_row[left_offset], left_end - seg_start, seg_start - (circle_mask->center_x - circle_mask->radius), 1,
                    circle_mask->radius, row_index, canvas_alpha, circle_mask->info,
                    (circle_mask->info != NULL) ? (const egui_circle_item_t *)circle_mask->info->items : NULL);
        }
    }

    {
        egui_dim_t mid_start = EGUI_MAX(seg_start, opaque_start);
        egui_dim_t mid_end = EGUI_MIN(seg_end, opaque_end);

        if (mid_start < mid_end)
        {
            egui_dim_t mid_offset = mid_start - screen_x;

            egui_image_std_blend_rgb565_row(&dst_row[mid_offset], &src_row[mid_offset], mid_end - mid_start, canvas_alpha);
        }
    }

    {
        egui_dim_t right_start = EGUI_MAX(seg_start, opaque_end);

        if (right_start < seg_end)
        {
            egui_dim_t right_offset = right_start - screen_x;

            egui_image_std_blend_rgb565_circle_masked_segment_fixed_row_direct(
                    &dst_row[right_offset], &src_row[right_offset], seg_end - right_start, circle_mask->center_x + circle_mask->radius - right_start, -1,
                    circle_mask->radius, row_index, canvas_alpha, circle_mask->info,
                    (circle_mask->info != NULL) ? (const egui_circle_item_t *)circle_mask->info->items : NULL);
        }
    }

    return 1;
}

__EGUI_STATIC_INLINE__ int egui_image_std_blend_rgb565_round_rect_masked_row_fast_with_cache(egui_canvas_t *canvas, egui_color_int_t *dst_row,
                                                                                              const uint16_t *src_row, egui_dim_t count, egui_dim_t screen_x,
                                                                                              egui_dim_t screen_y, egui_alpha_t canvas_alpha,
                                                                                              egui_image_std_round_rect_fast_cache_t *round_rect_cache)
{
    egui_mask_round_rectangle_t *round_rect_mask = (egui_mask_round_rectangle_t *)canvas->mask;
    egui_dim_t radius = round_rect_mask->radius;
    egui_dim_t x_end = screen_x + count;
    const egui_circle_info_t *info;
    const egui_circle_item_t *items;
    egui_dim_t round_rect_x;
    egui_dim_t round_rect_y;
    egui_dim_t round_rect_x_end;
    egui_dim_t round_rect_y_end;
    egui_dim_t rr_x_start;
    egui_dim_t rr_x_end;
    egui_dim_t visible_x_start;
    egui_dim_t visible_x_end;

    if (count == 0)
    {
        return 1;
    }

    if (!egui_image_std_round_rect_fast_cache_prepare(round_rect_cache, canvas->mask, radius))
    {
        if (canvas_alpha == EGUI_ALPHA_100)
        {
            for (egui_dim_t i = 0; i < count; i++)
            {
                egui_color_t color;
                egui_alpha_t alpha = EGUI_ALPHA_100;

                color.full = EGUI_COLOR_RGB565_TRANS(src_row[i]);
                canvas->mask->api->mask_point(canvas->mask, screen_x + i, screen_y, &color, &alpha);

                if (alpha == 0)
                {
                    continue;
                }

                if (alpha == EGUI_ALPHA_100)
                {
                    dst_row[i] = color.full;
                }
                else
                {
                    egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                }
            }
        }
        else
        {
            for (egui_dim_t i = 0; i < count; i++)
            {
                egui_color_t color;
                egui_alpha_t alpha = EGUI_ALPHA_100;

                color.full = EGUI_COLOR_RGB565_TRANS(src_row[i]);
                canvas->mask->api->mask_point(canvas->mask, screen_x + i, screen_y, &color, &alpha);
                alpha = egui_color_alpha_mix(canvas_alpha, alpha);

                if (alpha == 0)
                {
                    continue;
                }

                if (alpha == EGUI_ALPHA_100)
                {
                    dst_row[i] = color.full;
                }
                else
                {
                    egui_rgb_mix_ptr((egui_color_t *)&dst_row[i], &color, (egui_color_t *)&dst_row[i], alpha);
                }
            }
        }
        return 1;
    }

    round_rect_x = round_rect_cache->cached_x;
    round_rect_y = round_rect_cache->cached_y;
    round_rect_x_end = round_rect_x + round_rect_cache->cached_width;
    round_rect_y_end = round_rect_y + round_rect_cache->cached_height;

    if (screen_y < round_rect_y || screen_y >= round_rect_y_end)
    {
        return 1;
    }

    rr_x_start = EGUI_MAX(round_rect_x, screen_x);
    rr_x_end = EGUI_MIN(round_rect_x_end, x_end);
    if (rr_x_start >= rr_x_end)
    {
        return 1;
    }

    if (radius <= 0 || (screen_y >= round_rect_y + radius && screen_y < round_rect_y_end - radius))
    {
        egui_dim_t mid_offset = rr_x_start - screen_x;

        egui_image_std_blend_rgb565_row(&dst_row[mid_offset], &src_row[mid_offset], rr_x_end - rr_x_start, canvas_alpha);
        return 1;
    }

    {
        egui_dim_t row_index;
        egui_dim_t visible_boundary;
        egui_dim_t opaque_boundary;

        info = round_rect_cache->info;
        if (info == NULL)
        {
            return 0;
        }

        items = (const egui_circle_item_t *)info->items;
        if (!egui_image_std_round_rect_fast_cache_get_boundaries(round_rect_cache, screen_y, radius, &row_index, &visible_boundary, &opaque_boundary))
        {
            return 0;
        }

        visible_x_start = EGUI_MAX(round_rect_x + visible_boundary, screen_x);
        visible_x_end = EGUI_MIN(round_rect_x_end - visible_boundary, x_end);
        if (visible_x_start >= visible_x_end)
        {
            return 1;
        }

        rr_x_start = EGUI_MAX(round_rect_x + opaque_boundary, screen_x);
        rr_x_end = EGUI_MIN(round_rect_x_end - opaque_boundary, x_end);

        {
            egui_dim_t left_x_end = EGUI_MIN(rr_x_start, visible_x_end);

            if (visible_x_start < left_x_end)
            {
                egui_dim_t left_offset = visible_x_start - screen_x;

                egui_image_std_blend_rgb565_round_rect_masked_left_segment_fixed_row_direct(
                        &dst_row[left_offset], &src_row[left_offset], left_x_end - visible_x_start, visible_x_start, round_rect_cache->cached_x, row_index,
                        info, items);
            }
        }

        if (rr_x_start < rr_x_end)
        {
            egui_dim_t mid_offset = rr_x_start - screen_x;

            egui_image_std_blend_rgb565_row(&dst_row[mid_offset], &src_row[mid_offset], rr_x_end - rr_x_start, canvas_alpha);
        }

        {
            egui_dim_t right_x_start = EGUI_MAX(rr_x_end, visible_x_start);

            if (right_x_start < visible_x_end)
            {
                egui_dim_t right_offset = right_x_start - screen_x;

                egui_image_std_blend_rgb565_round_rect_masked_right_segment_fixed_row_direct(
                        &dst_row[right_offset], &src_row[right_offset], visible_x_end - right_x_start, right_x_start,
                        round_rect_cache->cached_x + round_rect_cache->cached_width, row_index, info, items);
            }
        }
    }

    return 1;
}

__EGUI_STATIC_INLINE__ int egui_image_std_blend_rgb565_round_rect_masked_row_fast(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_row,
                                                                                   egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y,
                                                                                   egui_alpha_t canvas_alpha)
{
    egui_image_std_round_rect_fast_cache_t round_rect_cache;

    egui_image_std_round_rect_fast_cache_init(&round_rect_cache);
    return egui_image_std_blend_rgb565_round_rect_masked_row_fast_with_cache(canvas, dst_row, src_row, count, screen_x, screen_y, canvas_alpha,
                                                                              &round_rect_cache);
}

int egui_image_std_blend_rgb565_masked_row(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_row, egui_dim_t count, egui_dim_t screen_x,
                                           egui_dim_t screen_y, egui_alpha_t canvas_alpha)
{
#if EGUI_CONFIG_COLOR_DEPTH == 16 && EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    if (canvas == NULL || canvas->mask == NULL || count <= 0 || canvas_alpha == 0)
    {
        return 0;
    }

    if (canvas->mask->api->kind == EGUI_MASK_KIND_CIRCLE)
    {
        return egui_image_std_blend_rgb565_circle_masked_row_fast(canvas, dst_row, src_row, count, screen_x, screen_y, canvas_alpha);
    }

    if (canvas->mask->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE)
    {
        return egui_image_std_blend_rgb565_round_rect_masked_row_fast(canvas, dst_row, src_row, count, screen_x, screen_y, canvas_alpha);
    }

    return 0;
#else
    EGUI_UNUSED(canvas);
    EGUI_UNUSED(dst_row);
    EGUI_UNUSED(src_row);
    EGUI_UNUSED(count);
    EGUI_UNUSED(screen_x);
    EGUI_UNUSED(screen_y);
    EGUI_UNUSED(canvas_alpha);
    return 0;
#endif
}

int egui_image_std_blend_rgb565_masked_row_block(egui_canvas_t *canvas, egui_color_int_t *dst_row, egui_dim_t dst_stride, const uint16_t *src_row,
                                                 egui_dim_t src_stride, egui_dim_t row_count, egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y,
                                                 egui_alpha_t canvas_alpha)
{
#if EGUI_CONFIG_COLOR_DEPTH == 16 && EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    if (count <= 0 || row_count <= 0 || canvas_alpha == 0)
    {
        return 1;
    }

    if (canvas == NULL || canvas->mask == NULL || dst_row == NULL || src_row == NULL)
    {
        return 0;
    }

    if (canvas->mask->api->kind == EGUI_MASK_KIND_CIRCLE)
    {
        for (egui_dim_t row = 0; row < row_count; row++)
        {
            egui_image_std_blend_rgb565_circle_masked_row_fast(canvas, dst_row, src_row, count, screen_x, screen_y, canvas_alpha);
            dst_row += dst_stride;
            src_row += src_stride;
            screen_y++;
        }
        return 1;
    }

    if (canvas->mask->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE)
    {
        egui_image_std_round_rect_fast_cache_t round_rect_cache;

        egui_image_std_round_rect_fast_cache_init(&round_rect_cache);
        for (egui_dim_t row = 0; row < row_count; row++)
        {
            egui_image_std_blend_rgb565_round_rect_masked_row_fast_with_cache(canvas, dst_row, src_row, count, screen_x, screen_y, canvas_alpha,
                                                                              &round_rect_cache);
            dst_row += dst_stride;
            src_row += src_stride;
            screen_y++;
        }
        return 1;
    }

    return 0;
#else
    EGUI_UNUSED(canvas);
    EGUI_UNUSED(dst_row);
    EGUI_UNUSED(dst_stride);
    EGUI_UNUSED(src_row);
    EGUI_UNUSED(src_stride);
    EGUI_UNUSED(row_count);
    EGUI_UNUSED(count);
    EGUI_UNUSED(screen_x);
    EGUI_UNUSED(screen_y);
    EGUI_UNUSED(canvas_alpha);
    return 0;
#endif
}

int egui_image_std_blend_rgb565_alpha8_masked_row_block(egui_canvas_t *canvas, egui_color_int_t *dst_row, egui_dim_t dst_stride, const uint16_t *src_row,
                                                        egui_dim_t src_stride, const uint8_t *src_alpha_row, egui_dim_t alpha_stride, egui_dim_t row_count,
                                                        egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y, egui_alpha_t canvas_alpha)
{
#if EGUI_CONFIG_COLOR_DEPTH == 16 && EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    if (count <= 0 || row_count <= 0 || canvas_alpha == 0)
    {
        return 1;
    }

    if (canvas == NULL || canvas->mask == NULL || dst_row == NULL || src_row == NULL || src_alpha_row == NULL)
    {
        return 0;
    }

    if (canvas->mask->api->kind == EGUI_MASK_KIND_CIRCLE)
    {
        for (egui_dim_t row = 0; row < row_count; row++)
        {
            egui_image_std_blend_rgb565_alpha8_circle_masked_row_fast(canvas, dst_row, src_row, src_alpha_row, count, screen_x, screen_y, canvas_alpha);
            dst_row += dst_stride;
            src_row += src_stride;
            src_alpha_row += alpha_stride;
            screen_y++;
        }
        return 1;
    }

    if (canvas->mask->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE)
    {
        egui_image_std_round_rect_fast_cache_t round_rect_cache;

        egui_image_std_round_rect_fast_cache_init(&round_rect_cache);
        for (egui_dim_t row = 0; row < row_count; row++)
        {
            egui_image_std_blend_rgb565_alpha8_round_rect_masked_row_fast_with_cache(canvas, dst_row, src_row, src_alpha_row, count, screen_x, screen_y,
                                                                                     canvas_alpha, &round_rect_cache);
            dst_row += dst_stride;
            src_row += src_stride;
            src_alpha_row += alpha_stride;
            screen_y++;
        }
        return 1;
    }

    return 0;
#else
    EGUI_UNUSED(canvas);
    EGUI_UNUSED(dst_row);
    EGUI_UNUSED(dst_stride);
    EGUI_UNUSED(src_row);
    EGUI_UNUSED(src_stride);
    EGUI_UNUSED(src_alpha_row);
    EGUI_UNUSED(alpha_stride);
    EGUI_UNUSED(row_count);
    EGUI_UNUSED(count);
    EGUI_UNUSED(screen_x);
    EGUI_UNUSED(screen_y);
    EGUI_UNUSED(canvas_alpha);
    return 0;
#endif
}

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
__EGUI_STATIC_INLINE__ egui_alpha_t egui_image_std_get_alpha_rgb565_4_row(const uint8_t *src_alpha_row, egui_dim_t src_x)
{
    uint8_t packed = src_alpha_row[src_x >> 1];
    uint8_t shift = (src_x & 0x01) << 2;

    return egui_alpha_change_table_4[(packed >> shift) & 0x0F];
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha4_mapped_row(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                          const egui_dim_t *src_x_map, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        while (i < count)
        {
            while (i < count && egui_image_std_get_alpha_rgb565_4_row(src_alpha_row, src_x_map[i]) == 0)
            {
                i++;
            }

            egui_dim_t opaque_start = i;
            while (i < count && egui_image_std_get_alpha_rgb565_4_row(src_alpha_row, src_x_map[i]) == EGUI_ALPHA_100)
            {
                i++;
            }

            if (opaque_start < i)
            {
                egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, EGUI_ALPHA_100);
                continue;
            }

            if (i < count)
            {
                egui_dim_t src_x = src_x_map[i];
                egui_alpha_t alpha = egui_image_std_get_alpha_rgb565_4_row(src_alpha_row, src_x);
                if (alpha != 0)
                {
                    egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                    if (alpha == EGUI_ALPHA_100)
                    {
                        dst_row[i] = pixel;
                    }
                    else
                    {
                        egui_color_t color;
                        color.full = pixel;
                        egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
                    }
                }
                i++;
            }
        }
        return;
    }

    while (i < count)
    {
        while (i < count && egui_image_std_get_alpha_rgb565_4_row(src_alpha_row, src_x_map[i]) == 0)
        {
            i++;
        }

        egui_dim_t opaque_start = i;
        while (i < count && egui_image_std_get_alpha_rgb565_4_row(src_alpha_row, src_x_map[i]) == EGUI_ALPHA_100)
        {
            i++;
        }

        if (opaque_start < i)
        {
            egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, canvas_alpha);
            continue;
        }

        if (i < count)
        {
            egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_4_row(src_alpha_row, src_x_map[i]));
            if (alpha != 0)
            {
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
            }
            i++;
        }
    }
}

// Row-level blend with opaque-run batch copy for 4-bit alpha.
// Typical images have opaque interiors: 0xFF byte = 2 pixels both fully opaque.
__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha4_row(egui_color_int_t *dst_row, const uint16_t *src_pixels, const uint8_t *alpha_buf,
                                                                   egui_dim_t start_col, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t end_col = start_col + count;
    egui_dim_t col = start_col;
    egui_dim_t dst_i = 0;

    if (canvas_alpha == 0)
    {
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        if ((col & 1) && col < end_col)
        {
            egui_alpha_t alpha = egui_image_std_get_alpha_rgb565_4_row(alpha_buf, col);
            if (alpha != 0)
            {
                egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
                if (alpha == EGUI_ALPHA_100)
                {
                    dst_row[dst_i] = pixel;
                }
                else
                {
                    egui_color_t c;
                    c.full = pixel;
                    egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
                }
            }
            col++;
            dst_i++;
        }

        while (col + 2 <= end_col)
        {
            uint8_t ab = alpha_buf[col >> 1];
            if (ab == 0xFF)
            {
                egui_dim_t run_start_col = col;
                egui_dim_t run_start_dst = dst_i;
                do
                {
                    col += 2;
                    dst_i += 2;
                } while (col + 2 <= end_col && alpha_buf[col >> 1] == 0xFF);
                egui_image_std_copy_rgb565_row(&dst_row[run_start_dst], &src_pixels[run_start_col], col - run_start_col);
            }
            else if (ab == 0x00)
            {
                col += 2;
                dst_i += 2;
            }
            else
            {
                for (int p = 0; p < 2; p++)
                {
                    egui_alpha_t alpha = egui_image_std_get_alpha_rgb565_4_row(alpha_buf, col);
                    if (alpha != 0)
                    {
                        egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
                        if (alpha == EGUI_ALPHA_100)
                        {
                            dst_row[dst_i] = pixel;
                        }
                        else
                        {
                            egui_color_t c;
                            c.full = pixel;
                            egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
                        }
                    }
                    col++;
                    dst_i++;
                }
            }
        }

        if (col < end_col)
        {
            egui_alpha_t alpha = egui_image_std_get_alpha_rgb565_4_row(alpha_buf, col);
            if (alpha != 0)
            {
                egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
                if (alpha == EGUI_ALPHA_100)
                {
                    dst_row[dst_i] = pixel;
                }
                else
                {
                    egui_color_t c;
                    c.full = pixel;
                    egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
                }
            }
        }
        return;
    }

    // Handle leading unaligned pixel (if start_col is odd)
    if ((col & 1) && col < end_col)
    {
        egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_4_row(alpha_buf, col));
        if (alpha != 0)
        {
            egui_color_t c;
            c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
            egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
        }
        col++;
        dst_i++;
    }

    // Process byte-aligned ranges (2 pixels per byte, 0xFF = both fully opaque)
    while (col + 2 <= end_col)
    {
        uint8_t ab = alpha_buf[col >> 1];
        if (ab == 0xFF)
        {
            // Both pixels fully opaque - find run of consecutive all-opaque bytes
            egui_dim_t run_start_col = col;
            egui_dim_t run_start_dst = dst_i;
            do
            {
                col += 2;
                dst_i += 2;
            } while (col + 2 <= end_col && alpha_buf[col >> 1] == 0xFF);
            // Batch copy entire opaque run
            egui_image_std_blend_rgb565_row(&dst_row[run_start_dst], &src_pixels[run_start_col], col - run_start_col, canvas_alpha);
        }
        else if (ab == 0x00)
        {
            // Both transparent - skip
            col += 2;
            dst_i += 2;
        }
        else
        {
            // Mixed byte - process individual pixels
            for (int p = 0; p < 2; p++)
            {
                egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_4_row(alpha_buf, col));
                if (alpha != 0)
                {
                    egui_color_t c;
                    c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
                    egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
                }
                col++;
                dst_i++;
            }
        }
    }

    // Handle trailing pixel
    if (col < end_col)
    {
        egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_4_row(alpha_buf, col));
        if (alpha != 0)
        {
            egui_color_t c;
            c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
            egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
        }
    }
}

EGUI_IMAGE_STD_BLEND_RGB565_PACKED_ALPHA_REPEAT2_ROW_DEFINE(egui_image_std_blend_rgb565_alpha4_repeat2_row, egui_image_std_get_alpha_rgb565_4_row)
EGUI_IMAGE_STD_BLEND_RGB565_PACKED_ALPHA_REPEAT4_ROW_DEFINE(egui_image_std_blend_rgb565_alpha4_repeat4_row, egui_image_std_get_alpha_rgb565_4_row)
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
__EGUI_STATIC_INLINE__ egui_alpha_t egui_image_std_get_alpha_rgb565_2_row(const uint8_t *src_alpha_row, egui_dim_t src_x)
{
    uint8_t packed = src_alpha_row[src_x >> 2];
    uint8_t shift = (src_x & 0x03) << 1;

    return egui_alpha_change_table_2[(packed >> shift) & 0x03];
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha2_mapped_row(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                          const egui_dim_t *src_x_map, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        while (i < count)
        {
            while (i < count && egui_image_std_get_alpha_rgb565_2_row(src_alpha_row, src_x_map[i]) == 0)
            {
                i++;
            }

            egui_dim_t opaque_start = i;
            while (i < count && egui_image_std_get_alpha_rgb565_2_row(src_alpha_row, src_x_map[i]) == EGUI_ALPHA_100)
            {
                i++;
            }

            if (opaque_start < i)
            {
                egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, EGUI_ALPHA_100);
                continue;
            }

            if (i < count)
            {
                egui_dim_t src_x = src_x_map[i];
                egui_alpha_t alpha = egui_image_std_get_alpha_rgb565_2_row(src_alpha_row, src_x);
                if (alpha != 0)
                {
                    egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                    if (alpha == EGUI_ALPHA_100)
                    {
                        dst_row[i] = pixel;
                    }
                    else
                    {
                        egui_color_t color;
                        color.full = pixel;
                        egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
                    }
                }
                i++;
            }
        }
        return;
    }

    while (i < count)
    {
        while (i < count && egui_image_std_get_alpha_rgb565_2_row(src_alpha_row, src_x_map[i]) == 0)
        {
            i++;
        }

        egui_dim_t opaque_start = i;
        while (i < count && egui_image_std_get_alpha_rgb565_2_row(src_alpha_row, src_x_map[i]) == EGUI_ALPHA_100)
        {
            i++;
        }

        if (opaque_start < i)
        {
            egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, canvas_alpha);
            continue;
        }

        if (i < count)
        {
            egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_2_row(src_alpha_row, src_x_map[i]));
            if (alpha != 0)
            {
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
            }
            i++;
        }
    }
}

// Row-level blend with opaque-run batch copy for 2-bit alpha.
// Typical images have opaque interiors: 0xFF byte = 4 pixels all fully opaque.
__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha2_row(egui_color_int_t *dst_row, const uint16_t *src_pixels, const uint8_t *alpha_buf,
                                                                   egui_dim_t start_col, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t end_col = start_col + count;
    egui_dim_t col = start_col;
    egui_dim_t dst_i = 0;

    if (canvas_alpha == 0)
    {
        return;
    }

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        while (col < end_col && (col & 3))
        {
            egui_alpha_t alpha = egui_image_std_get_alpha_rgb565_2_row(alpha_buf, col);
            if (alpha != 0)
            {
                egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
                if (alpha == EGUI_ALPHA_100)
                {
                    dst_row[dst_i] = pixel;
                }
                else
                {
                    egui_color_t c;
                    c.full = pixel;
                    egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
                }
            }
            col++;
            dst_i++;
        }

        while (col + 4 <= end_col)
        {
            uint8_t ab = alpha_buf[col >> 2];
            if (ab == 0xFF)
            {
                egui_dim_t run_start_col = col;
                egui_dim_t run_start_dst = dst_i;
                do
                {
                    col += 4;
                    dst_i += 4;
                } while (col + 4 <= end_col && alpha_buf[col >> 2] == 0xFF);
                egui_image_std_copy_rgb565_row(&dst_row[run_start_dst], &src_pixels[run_start_col], col - run_start_col);
            }
            else if (ab == 0x00)
            {
                col += 4;
                dst_i += 4;
            }
            else
            {
                for (int p = 0; p < 4; p++)
                {
                    egui_alpha_t alpha = egui_image_std_get_alpha_rgb565_2_row(alpha_buf, col);
                    if (alpha != 0)
                    {
                        egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
                        if (alpha == EGUI_ALPHA_100)
                        {
                            dst_row[dst_i] = pixel;
                        }
                        else
                        {
                            egui_color_t c;
                            c.full = pixel;
                            egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
                        }
                    }
                    col++;
                    dst_i++;
                }
            }
        }

        while (col < end_col)
        {
            egui_alpha_t alpha = egui_image_std_get_alpha_rgb565_2_row(alpha_buf, col);
            if (alpha != 0)
            {
                egui_color_int_t pixel = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
                if (alpha == EGUI_ALPHA_100)
                {
                    dst_row[dst_i] = pixel;
                }
                else
                {
                    egui_color_t c;
                    c.full = pixel;
                    egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
                }
            }
            col++;
            dst_i++;
        }
        return;
    }

    // Handle leading unaligned pixels until 4-pixel (byte) boundary
    while (col < end_col && (col & 3))
    {
        egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_2_row(alpha_buf, col));
        if (alpha != 0)
        {
            egui_color_t c;
            c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
            egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
        }
        col++;
        dst_i++;
    }

    // Process byte-aligned ranges (4 pixels per byte, 0xFF = all fully opaque)
    while (col + 4 <= end_col)
    {
        uint8_t ab = alpha_buf[col >> 2];
        if (ab == 0xFF)
        {
            // All 4 pixels fully opaque - find run of consecutive all-opaque bytes
            egui_dim_t run_start_col = col;
            egui_dim_t run_start_dst = dst_i;
            do
            {
                col += 4;
                dst_i += 4;
            } while (col + 4 <= end_col && alpha_buf[col >> 2] == 0xFF);
            // Batch copy entire opaque run
            egui_image_std_blend_rgb565_row(&dst_row[run_start_dst], &src_pixels[run_start_col], col - run_start_col, canvas_alpha);
        }
        else if (ab == 0x00)
        {
            // All transparent - skip
            col += 4;
            dst_i += 4;
        }
        else
        {
            // Mixed byte - process individual pixels
            for (int p = 0; p < 4; p++)
            {
                egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_2_row(alpha_buf, col));
                if (alpha != 0)
                {
                    egui_color_t c;
                    c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
                    egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
                }
                col++;
                dst_i++;
            }
        }
    }

    // Handle trailing pixels
    while (col < end_col)
    {
        egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_2_row(alpha_buf, col));
        if (alpha != 0)
        {
            egui_color_t c;
            c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
            egui_image_std_blend_resize_pixel(&dst_row[dst_i], c, alpha);
        }
        col++;
        dst_i++;
    }
}

EGUI_IMAGE_STD_BLEND_RGB565_PACKED_ALPHA_REPEAT2_ROW_DEFINE(egui_image_std_blend_rgb565_alpha2_repeat2_row, egui_image_std_get_alpha_rgb565_2_row)
EGUI_IMAGE_STD_BLEND_RGB565_PACKED_ALPHA_REPEAT4_ROW_DEFINE(egui_image_std_blend_rgb565_alpha2_repeat4_row, egui_image_std_get_alpha_rgb565_2_row)
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
__EGUI_STATIC_INLINE__ egui_alpha_t egui_image_std_get_alpha_rgb565_1_row(const uint8_t *src_alpha_row, egui_dim_t src_x)
{
    return ((src_alpha_row[src_x >> 3] >> (src_x & 0x07)) & 0x01) ? EGUI_ALPHA_100 : 0;
}

__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha1_mapped_row(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                          const egui_dim_t *src_x_map, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t i = 0;

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        while (i < count)
        {
            while (i < count && egui_image_std_get_alpha_rgb565_1_row(src_alpha_row, src_x_map[i]) == 0)
            {
                i++;
            }

            egui_dim_t opaque_start = i;
            while (i < count && egui_image_std_get_alpha_rgb565_1_row(src_alpha_row, src_x_map[i]) == EGUI_ALPHA_100)
            {
                i++;
            }

            if (opaque_start < i)
            {
                egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, EGUI_ALPHA_100);
                continue;
            }

            if (i < count)
            {
                egui_dim_t src_x = src_x_map[i];
                if (egui_image_std_get_alpha_rgb565_1_row(src_alpha_row, src_x) != 0)
                {
                    dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                }
                i++;
            }
        }
        return;
    }

    while (i < count)
    {
        while (i < count && egui_image_std_get_alpha_rgb565_1_row(src_alpha_row, src_x_map[i]) == 0)
        {
            i++;
        }

        egui_dim_t opaque_start = i;
        while (i < count && egui_image_std_get_alpha_rgb565_1_row(src_alpha_row, src_x_map[i]) == EGUI_ALPHA_100)
        {
            i++;
        }

        if (opaque_start < i)
        {
            egui_image_std_blend_rgb565_mapped_row(&dst_row[opaque_start], src_row, &src_x_map[opaque_start], i - opaque_start, canvas_alpha);
            continue;
        }

        if (i < count)
        {
            egui_alpha_t alpha = egui_color_alpha_mix(canvas_alpha, egui_image_std_get_alpha_rgb565_1_row(src_alpha_row, src_x_map[i]));
            if (alpha != 0)
            {
                egui_color_t color;
                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
            }
            i++;
        }
    }
}

// Row-level blend with opaque-run batch copy for 1-bit alpha.
// Typical images have opaque interiors: 0xFF byte = 8 pixels all fully opaque.
__EGUI_STATIC_INLINE__ void egui_image_std_blend_rgb565_alpha1_row(egui_color_int_t *dst_row, const uint16_t *src_pixels, const uint8_t *alpha_buf,
                                                                   egui_dim_t start_col, egui_dim_t count, egui_alpha_t canvas_alpha)
{
    egui_dim_t end_col = start_col + count;
    egui_dim_t col = start_col;
    egui_dim_t dst_i = 0;

    // Handle leading unaligned bits until 8-pixel (byte) boundary
    while (col < end_col && (col & 7))
    {
        if ((alpha_buf[col >> 3] >> (col & 7)) & 1)
        {
            dst_row[dst_i] = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
        }
        col++;
        dst_i++;
    }

    // Process byte-aligned ranges (8 pixels per byte, 0xFF = all fully opaque)
    while (col + 8 <= end_col)
    {
        uint8_t ab = alpha_buf[col >> 3];
        if (ab == 0xFF)
        {
            // All 8 pixels fully opaque - find run of consecutive all-opaque bytes
            egui_dim_t run_start_col = col;
            egui_dim_t run_start_dst = dst_i;
            do
            {
                col += 8;
                dst_i += 8;
            } while (col + 8 <= end_col && alpha_buf[col >> 3] == 0xFF);
            // Batch copy entire opaque run
            egui_image_std_blend_rgb565_row(&dst_row[run_start_dst], &src_pixels[run_start_col], col - run_start_col, canvas_alpha);
        }
        else if (ab == 0x00)
        {
            // All 8 pixels transparent - skip
            col += 8;
            dst_i += 8;
        }
        else
        {
            // Mixed byte - process individual bits
            for (int bit = 0; bit < 8; bit++)
            {
                if ((ab >> bit) & 1)
                {
                    dst_row[dst_i] = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
                }
                col++;
                dst_i++;
            }
        }
    }

    // Handle trailing unaligned bits
    while (col < end_col)
    {
        if ((alpha_buf[col >> 3] >> (col & 7)) & 1)
        {
            dst_row[dst_i] = EGUI_COLOR_RGB565_TRANS(src_pixels[col]);
        }
        col++;
        dst_i++;
    }
}

EGUI_IMAGE_STD_BLEND_RGB565_PACKED_ALPHA_REPEAT2_ROW_DEFINE(egui_image_std_blend_rgb565_alpha1_repeat2_row, egui_image_std_get_alpha_rgb565_1_row)
EGUI_IMAGE_STD_BLEND_RGB565_PACKED_ALPHA_REPEAT4_ROW_DEFINE(egui_image_std_blend_rgb565_alpha1_repeat4_row, egui_image_std_get_alpha_rgb565_1_row)
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_alpha_8(const uint8_t *p_data, egui_dim_t col_index, egui_alpha_t *alpha)
{
    *alpha = p_data[col_index];
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_alpha_4(const uint8_t *p_data, egui_dim_t col_index, egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index >> 1; // same to: col_index / 2
    uint8_t sel_alpha;

    bit_pos = col_index & 0x01; // 0x01
    bit_pos = bit_pos << 2;     // same to: bit_pos * 4

    sel_alpha = ((p_data[sel_pos]) >> bit_pos) & 0x0F;

    *alpha = egui_alpha_change_table_4[sel_alpha];
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_alpha_2(const uint8_t *p_data, egui_dim_t col_index, egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index >> 2; // same to: col_index / 4
    uint8_t sel_alpha;

    bit_pos = col_index & 0x03; // 0x03
    bit_pos = bit_pos << 1;     // same to: bit_pos * 2

    sel_alpha = ((p_data[sel_pos]) >> bit_pos) & 0x03;

    *alpha = egui_alpha_change_table_2[sel_alpha];
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
__EGUI_STATIC_INLINE__ void egui_image_std_get_col_alpha_1(const uint8_t *p_data, egui_dim_t col_index, egui_alpha_t *alpha)
{
    uint8_t bit_pos;
    uint32_t sel_pos = col_index >> 3; // same to col_index / 8
    uint8_t sel_alpha;

    bit_pos = col_index & 0x07; // 0x07

    sel_alpha = ((p_data[sel_pos]) >> bit_pos) & 0x01;

    *alpha = (sel_alpha ? 0xff : 0x00);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1

egui_image_std_get_pixel *egui_image_get_point_func(const egui_image_t *self)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    // select get_pixel function.
    egui_image_std_get_pixel *get_pixel = NULL;
    if (image)
    {
        switch (image->data_type)
        {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB32:
            get_pixel = egui_image_std_get_pixel_rgb32;
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB565:
            if (image->alpha_buf == NULL
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                || (image->res_type == EGUI_RESOURCE_TYPE_INTERNAL && egui_image_std_rgb565_is_opaque_source(image))
#else
                || egui_image_std_rgb565_is_opaque_source(image)
#endif
            )
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
                get_pixel = egui_image_std_get_pixel_rgb565;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
            }
            else
            {
                switch (image->alpha_type)
                {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
                case EGUI_IMAGE_ALPHA_TYPE_1:
                    get_pixel = egui_image_std_get_pixel_rgb565_1;
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
                case EGUI_IMAGE_ALPHA_TYPE_2:
                    get_pixel = egui_image_std_get_pixel_rgb565_2;
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
                case EGUI_IMAGE_ALPHA_TYPE_4:
                    get_pixel = egui_image_std_get_pixel_rgb565_4;
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                case EGUI_IMAGE_ALPHA_TYPE_8:
                    get_pixel = egui_image_std_get_pixel_rgb565_8;
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                default:
                    EGUI_ASSERT(0);
                    break;
                }
            }
            break;
        default:
            EGUI_ASSERT(0);
            break;
        }
    }

    return get_pixel;
}

int egui_image_std_get_point(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_image_std_get_pixel *get_pixel = egui_image_get_point_func(self);

    if ((x >= image->width) || (y >= image->height))
    {
        return 0;
    }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_t cached_self;
        const egui_image_t *resolved_self = egui_image_std_resolve_external_persistent_image(self, &cached_self);

        if (resolved_self == self)
        {
            return 0;
        }

        image = (egui_image_std_info_t *)resolved_self->res;
    }
#endif

    get_pixel(image, x, y, color, alpha);

    return 1;
}

int egui_image_std_get_point_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color,
                                    egui_alpha_t *alpha)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    if (width == 0 || height == 0)
    {
        return 0;
    }
    egui_float_t width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(width));
    egui_float_t height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(height));
    egui_dim_t src_x;
    egui_dim_t src_y;

    if ((x >= width) || (y >= height))
    {
        return 0;
    }

    // select get_pixel function.
    egui_image_std_get_pixel *get_pixel = egui_image_get_point_func(self);

    // For speed, use nearestScaler to scale the image.
    src_x = (egui_dim_t)EGUI_FLOAT_MULT(x, width_radio);
    src_y = (egui_dim_t)EGUI_FLOAT_MULT(y, height_radio);

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_t cached_self;
        const egui_image_t *resolved_self = egui_image_std_resolve_external_persistent_image(self, &cached_self);

        if (resolved_self == self)
        {
            return 0;
        }

        image = (egui_image_std_info_t *)resolved_self->res;
    }
#endif

    get_pixel(image, src_x, src_y, color, alpha);

    return 1;
}

void egui_image_std_load_data_resource(void *dest, egui_image_std_info_t *image, uint32_t start_offset, uint32_t size)
{
    // EGUI_LOG_INF("egui_image_std_load_data_resource, start_offset: %d, size: %d\n", start_offset, size);
    if (image->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
    {
        egui_memcpy(dest, (const void *)((const uint8_t *)image->data_buf + start_offset), size);
    }
    else
    {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_api_load_external_resource(dest, (egui_uintptr_t)(image->data_buf), start_offset, size);
#else
        EGUI_ASSERT(0);
#endif
    }

    // EGUI_LOG_INF("egui_image_std_load_data_resource, data: %08x:%08x:%08x:%08x:%08x:%08x:%08x:%08x\n", ((uint32_t *)dest)[0], ((uint32_t *)dest)[1],
    // ((uint32_t *)dest)[2], ((uint32_t *)dest)[3], ((uint32_t *)dest)[4], ((uint32_t *)dest)[5], ((uint32_t *)dest)[6], ((uint32_t *)dest)[7]);
}

void egui_image_std_load_alpha_resource(void *dest, egui_image_std_info_t *image, uint32_t start_offset, uint32_t size)
{
    // EGUI_LOG_INF("egui_image_std_load_alpha_resource, start_offset: %d, size: %d\n", start_offset, size);
    if (image->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
    {
        egui_memcpy(dest, (const void *)((const uint8_t *)image->alpha_buf + start_offset), size);
    }
    else
    {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_api_load_external_resource(dest, (egui_uintptr_t)(image->alpha_buf), start_offset, size);
#else
        EGUI_ASSERT(0);
#endif
    }
    // EGUI_LOG_INF("egui_image_std_load_alpha_resource, data: %08x:%08x:%08x:%08x:%08x:%08x:%08x:%08x\n", ((uint32_t *)dest)[0], ((uint32_t *)dest)[1],
    // ((uint32_t *)dest)[2], ((uint32_t *)dest)[3], ((uint32_t *)dest)[4], ((uint32_t *)dest)[5], ((uint32_t *)dest)[6], ((uint32_t *)dest)[7]);
}

#if EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE
void egui_image_std_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_color_t color;
    egui_alpha_t alpha;
    egui_dim_t width = image->width;
    egui_dim_t height = image->height;
    const uint32_t *p_data = image->data_buf;
    // const uint8_t* p_alpha = image->alpha_buf;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // select get_pixel function.
    egui_image_std_get_pixel *get_pixel = egui_image_get_point_func(self);

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);
    if (egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

    if (canvas->mask != NULL)
    {
        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t x_start, x_end;
            for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
            {
                egui_dim_t screen_y = y + y_;
                egui_dim_t screen_x_min = x + region.location.x;
                egui_dim_t screen_x_max = x + x_total;
                int result = canvas->mask->api->mask_get_row_range(canvas->mask, screen_y, screen_x_min, screen_x_max, &x_start, &x_end);
                if (result == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }
                if (result == EGUI_MASK_ROW_INSIDE)
                {
                    for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
                    {
                        get_pixel(image, x_, y_, &color, &alpha);
                        egui_canvas_draw_point_limit_skip_mask((x + x_), screen_y, color, alpha);
                    }
                }
                else // PARTIAL
                {
                    egui_dim_t img_x_start = x_start - x;
                    egui_dim_t img_x_end = x_end - x;
                    // Left edge: per-pixel with mask
                    for (egui_dim_t x_ = region.location.x; x_ < img_x_start; x_++)
                    {
                        get_pixel(image, x_, y_, &color, &alpha);
                        egui_canvas_draw_point_limit((x + x_), screen_y, color, alpha);
                    }
                    // Middle: skip mask
                    for (egui_dim_t x_ = img_x_start; x_ < img_x_end; x_++)
                    {
                        get_pixel(image, x_, y_, &color, &alpha);
                        egui_canvas_draw_point_limit_skip_mask((x + x_), screen_y, color, alpha);
                    }
                    // Right edge: per-pixel with mask
                    for (egui_dim_t x_ = img_x_end; x_ < x_total; x_++)
                    {
                        get_pixel(image, x_, y_, &color, &alpha);
                        egui_canvas_draw_point_limit((x + x_), screen_y, color, alpha);
                    }
                }
            }
        }
        else
        {
            for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
            {
                for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
                {
                    get_pixel(image, x_, y_, &color, &alpha);
                    egui_canvas_draw_point_limit((x + x_), (y + y_), color, alpha);
                }
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
        {
            for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
            {
                get_pixel(image, x_, y_, &color, &alpha);

                // change to real position in canvas.
                egui_canvas_draw_point_limit_skip_mask((x + x_), (y + y_), color, alpha);
            }
        }
    }
}

void egui_image_std_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    if (width == image->width && height == image->height)
    {
        egui_image_std_draw_image(self, x, y);
        return;
    }
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_color_t color;
    egui_alpha_t alpha;
    egui_float_t width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(width));
    egui_float_t height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(height));
    const uint32_t *p_data = image->data_buf;
    // const uint8_t* p_alpha = image->alpha_buf;
    egui_dim_t src_x;
    egui_dim_t src_y;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // select get_pixel function.
    egui_image_std_get_pixel *get_pixel = egui_image_get_point_func(self);

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);
    if (egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

    if (canvas->mask != NULL)
    {
        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t rr_x_start, rr_x_end;
            for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
            {
                egui_dim_t screen_y = y + y_;
                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, screen_y, x + region.location.x, x + x_total, &rr_x_start, &rr_x_end);
                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                    continue;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
                    {
                        src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                        get_pixel(image, src_x, src_y, &color, &alpha);
                        egui_canvas_draw_point_limit_skip_mask((x + x_), screen_y, color, alpha);
                    }
                }
                else
                {
                    egui_dim_t img_xs = rr_x_start - x;
                    egui_dim_t img_xe = rr_x_end - x;
                    for (egui_dim_t x_ = region.location.x; x_ < img_xs; x_++)
                    {
                        src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                        get_pixel(image, src_x, src_y, &color, &alpha);
                        egui_canvas_draw_point_limit((x + x_), screen_y, color, alpha);
                    }
                    for (egui_dim_t x_ = img_xs; x_ < img_xe; x_++)
                    {
                        src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                        get_pixel(image, src_x, src_y, &color, &alpha);
                        egui_canvas_draw_point_limit_skip_mask((x + x_), screen_y, color, alpha);
                    }
                    for (egui_dim_t x_ = img_xe; x_ < x_total; x_++)
                    {
                        src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                        get_pixel(image, src_x, src_y, &color, &alpha);
                        egui_canvas_draw_point_limit((x + x_), screen_y, color, alpha);
                    }
                }
            }
        }
        else
        {
            for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
            {
                for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
                {
                    src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                    src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                    get_pixel(image, src_x, src_y, &color, &alpha);
                    egui_canvas_draw_point_limit((x + x_), (y + y_), color, alpha);
                }
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)
        {
            for (egui_dim_t x_ = region.location.x; x_ < x_total; x_++)
            {
                src_x = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(x_, width_radio);
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                get_pixel(image, src_x, src_y, &color, &alpha);
                egui_canvas_draw_point_limit_skip_mask((x + x_), (y + y_), color, alpha);
            }
        }
    }
}

#else // EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE

#define EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(_get_pixel_func, self, x, y, x_total, y_total, x_base, y_base)                                                   \
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;                                                                                         \
    egui_color_t color;                                                                                                                                        \
    egui_alpha_t alpha;                                                                                                                                        \
    egui_canvas_t *canvas = egui_canvas_get_canvas();                                                                                                          \
    if (canvas->mask != NULL)                                                                                                                                  \
    {                                                                                                                                                          \
        if (canvas->mask->api->mask_get_row_range != NULL)                                                                                                     \
        {                                                                                                                                                      \
            egui_dim_t rr_x_start, rr_x_end;                                                                                                                   \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                egui_dim_t rr_sy = y_base + y_;                                                                                                                \
                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &rr_x_start, &rr_x_end);                 \
                if (rr_res == EGUI_MASK_ROW_OUTSIDE)                                                                                                           \
                    continue;                                                                                                                                  \
                if (rr_res == EGUI_MASK_ROW_INSIDE)                                                                                                            \
                {                                                                                                                                              \
                    for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                \
                    {                                                                                                                                          \
                        _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                        \
                        egui_canvas_draw_point_limit_skip_mask((x_base + x_), rr_sy, color, alpha);                                                            \
                    }                                                                                                                                          \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    egui_dim_t rr_img_xs = rr_x_start - x_base;                                                                                                \
                    egui_dim_t rr_img_xe = rr_x_end - x_base;                                                                                                  \
                    for (egui_dim_t x_ = x; x_ < rr_img_xs; x_++)                                                                                              \
                    {                                                                                                                                          \
                        _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                        \
                        egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);                                                                      \
                    }                                                                                                                                          \
                    for (egui_dim_t x_ = rr_img_xs; x_ < rr_img_xe; x_++)                                                                                      \
                    {                                                                                                                                          \
                        _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                        \
                        egui_canvas_draw_point_limit_skip_mask((x_base + x_), rr_sy, color, alpha);                                                            \
                    }                                                                                                                                          \
                    for (egui_dim_t x_ = rr_img_xe; x_ < x_total; x_++)                                                                                        \
                    {                                                                                                                                          \
                        _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                        \
                        egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);                                                                      \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        else if (canvas->mask->api->mask_get_row_overlay != NULL)                                                                                              \
        {                                                                                                                                                      \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                egui_dim_t ov_sy = y_base + y_;                                                                                                                \
                egui_color_t ov_color;                                                                                                                         \
                egui_alpha_t ov_alpha;                                                                                                                         \
                if (canvas->mask->api->mask_get_row_overlay(canvas->mask, ov_sy, &ov_color, &ov_alpha))                                                        \
                {                                                                                                                                              \
                    for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                \
                    {                                                                                                                                          \
                        _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                        \
                        if (ov_alpha > 0)                                                                                                                      \
                        {                                                                                                                                      \
                            egui_rgb_mix_ptr(&color, &ov_color, &color, ov_alpha);                                                                             \
                        }                                                                                                                                      \
                        egui_canvas_draw_point_limit_skip_mask((x_base + x_), ov_sy, color, alpha);                                                            \
                    }                                                                                                                                          \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                \
                    {                                                                                                                                          \
                        _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                        \
                        egui_canvas_draw_point_limit((x_base + x_), ov_sy, color, alpha);                                                                      \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                    \
                {                                                                                                                                              \
                    _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                            \
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);                                                                  \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }                                                                                                                                                          \
    else                                                                                                                                                       \
    {                                                                                                                                                          \
        for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                            \
        {                                                                                                                                                      \
            for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                        \
            {                                                                                                                                                  \
                _get_pixel_func(image, x_, y_, &color, &alpha);                                                                                                \
                                                                                                                                                               \
                /* change to real position in canvas. */                                                                                                       \
                egui_canvas_draw_point_limit_skip_mask((x_base + x_), (y_base + y_), color, alpha);                                                            \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
void egui_image_std_set_image_rgb565_8(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                       egui_dim_t y_base)
{
    // EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_8, self, x, y, x_total, y_total, x_base, y_base);

    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    uint16_t data_row_size = image->width << 1; // same to image->width * 2
    uint16_t alpha_row_size = image->width;
    egui_dim_t src_x_base = 0;
    EGUI_UNUSED(data_row_size);
    EGUI_UNUSED(alpha_row_size);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_external_alpha_row_persistent_cache_t *row_cache = egui_image_std_get_external_alpha_row_persistent_cache();
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        if (!egui_image_std_prepare_external_alpha_row_persistent_cache_min_rows(row_cache, image, data_row_size, alpha_row_size,
                                                                                 EGUI_MIN(EGUI_CONFIG_PFB_HEIGHT, image->height)))
        {
            return;
        }
    }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        egui_dim_t rr_sy = y_base + y_;
        // Check mask row range BEFORE loading data (avoids wasted I/O for extern resources)
        egui_dim_t rr_x_start = 0, rr_x_end = 0;
        int rr_res = -1; // -1 = no mask or no row_range API
        int image_mask_full_row_fast_path = (canvas->mask != NULL && canvas->mask->api->kind == EGUI_MASK_KIND_IMAGE);
        if (!image_mask_full_row_fast_path && canvas->mask != NULL && canvas->mask->api->mask_get_row_range != NULL)
        {
            rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &rr_x_start, &rr_x_end);
            if (rr_res == EGUI_MASK_ROW_OUTSIDE)
            {
                continue; // Skip data loading entirely for this row
            }
        }

        uint32_t row_start = y_ * image->width;
        const void *p_data = NULL;
        const void *p_alpha = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, y_))
            {
                continue;
            }

            p_data = egui_image_std_get_external_alpha_row_persistent_data(row_cache, y_);
            p_alpha = egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, y_);
        }
        else
#endif
        {
            p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
            p_alpha = (const void *)((const uint8_t *)image->alpha_buf + (row_start));
        }
        if (canvas->mask != NULL)
        {
            if (image_mask_full_row_fast_path)
            {
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
                egui_alpha_t canvas_alpha = canvas->alpha;
                egui_dim_t pfb_width = canvas->pfb_region.size.width;
                egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                const uint16_t *src_pixels = (const uint16_t *)p_data + (x - src_x_base);
                const uint8_t *src_alpha_row = (const uint8_t *)p_alpha + (x - src_x_base);
                egui_dim_t count = x_total - x;

                if (egui_mask_image_blend_rgb565_alpha8_row_segment(canvas->mask, dst_row, src_pixels, src_alpha_row, count, x_base + x, rr_sy, canvas_alpha))
                {
                    continue;
                }
#endif

                image_mask_full_row_fast_path = 0;
            }

            if (rr_res < 0 && canvas->mask->api->mask_get_row_range != NULL)
            {
                rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &rr_x_start, &rr_x_end);
                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }
            }

            if (rr_res >= 0) // mask_get_row_range was available
            {
                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    // Fast path: row fully inside mask — direct PFB access
                    egui_alpha_t canvas_alpha = canvas->alpha;
                    egui_dim_t pfb_width = canvas->pfb_region.size.width;
                    egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    const uint16_t *src_pixels = (const uint16_t *)p_data + (x - src_x_base);
                    const uint8_t *src_alpha_row = (const uint8_t *)p_alpha + (x - src_x_base);
                    egui_dim_t count = x_total - x;

                    egui_image_std_blend_rgb565_alpha8_row(dst_row, src_pixels, src_alpha_row, count, canvas_alpha);
                }
                else // PARTIAL
                {
                    int image_mask_fast_path = (canvas->mask->api->kind == EGUI_MASK_KIND_IMAGE);
                    egui_dim_t rr_img_xs = rr_x_start - x_base;
                    egui_dim_t rr_img_xe = rr_x_end - x_base;
                    egui_dim_t visible_x_start = x_base + x;
                    egui_dim_t visible_x_end = x_base + x_total;
                    int has_visible_range = 0;

                    if (image_mask_fast_path && canvas->mask->api->mask_get_row_visible_range != NULL)
                    {
                        has_visible_range = canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &visible_x_start,
                                                                                          &visible_x_end);
                        if (!has_visible_range)
                        {
                            continue;
                        }
                    }

                    if (has_visible_range)
                    {
                        egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                        egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);
                        egui_dim_t left_img_xe = EGUI_MIN(rr_img_xs, vis_img_xe);

                        if (vis_img_xs < left_img_xe)
                        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
                            egui_alpha_t canvas_alpha = canvas->alpha;
                            egui_dim_t pfb_width = canvas->pfb_region.size.width;
                            egui_dim_t dst_x = (x_base + vis_img_xs) - canvas->pfb_location_in_base_view.x;
                            egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x];
                            const uint16_t *src_pixels = (const uint16_t *)p_data + (vis_img_xs - src_x_base);
                            const uint8_t *src_alpha_seg = (const uint8_t *)p_alpha + (vis_img_xs - src_x_base);

                            if (!egui_mask_image_blend_rgb565_alpha8_row_segment(canvas->mask, dst_row, src_pixels, src_alpha_seg, left_img_xe - vis_img_xs,
                                                                                 x_base + vis_img_xs, rr_sy, canvas_alpha))
#endif
                            {
                                for (egui_dim_t x_ = vis_img_xs; x_ < left_img_xe; x_++)
                                {
                                    egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                                }
                            }
                        }
                    }
                    else
                    {
                        for (egui_dim_t x_ = x; x_ < rr_img_xs; x_++)
                        {
                            egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                            egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                        }
                    }

                    // Middle: direct PFB access (inside mask)
                    {
                        egui_alpha_t canvas_alpha = canvas->alpha;
                        egui_dim_t pfb_width = canvas->pfb_region.size.width;
                        egui_dim_t dst_x = rr_x_start - canvas->pfb_location_in_base_view.x;
                        egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x];
                        const uint16_t *src_pixels = (const uint16_t *)p_data + (rr_img_xs - src_x_base);
                        const uint8_t *src_alpha_mid = (const uint8_t *)p_alpha + (rr_img_xs - src_x_base);
                        egui_dim_t mid_count = rr_img_xe - rr_img_xs;

                        egui_image_std_blend_rgb565_alpha8_row(dst_row, src_pixels, src_alpha_mid, mid_count, canvas_alpha);
                    }

                    if (has_visible_range)
                    {
                        egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                        egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);
                        egui_dim_t right_img_xs = EGUI_MAX(rr_img_xe, vis_img_xs);

                        if (right_img_xs < vis_img_xe)
                        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
                            egui_alpha_t canvas_alpha = canvas->alpha;
                            egui_dim_t pfb_width = canvas->pfb_region.size.width;
                            egui_dim_t dst_x = (x_base + right_img_xs) - canvas->pfb_location_in_base_view.x;
                            egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x];
                            const uint16_t *src_pixels = (const uint16_t *)p_data + (right_img_xs - src_x_base);
                            const uint8_t *src_alpha_seg = (const uint8_t *)p_alpha + (right_img_xs - src_x_base);

                            if (!egui_mask_image_blend_rgb565_alpha8_row_segment(canvas->mask, dst_row, src_pixels, src_alpha_seg, vis_img_xe - right_img_xs,
                                                                                 x_base + right_img_xs, rr_sy, canvas_alpha))
#endif
                            {
                                for (egui_dim_t x_ = right_img_xs; x_ < vis_img_xe; x_++)
                                {
                                    egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                                }
                            }
                        }
                    }
                    else
                    {
                        for (egui_dim_t x_ = rr_img_xe; x_ < x_total; x_++)
                        {
                            egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                            egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                        }
                    }
                }
            }
            else if (canvas->mask->api->mask_get_row_overlay != NULL)
            {
                egui_color_t ov_color;
                egui_alpha_t ov_alpha;
                if (canvas->mask->api->mask_get_row_overlay(canvas->mask, rr_sy, &ov_color, &ov_alpha))
                {
                    // Row-uniform overlay: apply overlay to each pixel, skip mask_point
                    egui_alpha_t canvas_alpha = canvas->alpha;
                    egui_dim_t pfb_width = canvas->pfb_region.size.width;
                    egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    const uint16_t *src_pixels = (const uint16_t *)p_data + (x - src_x_base);
                    const uint8_t *src_alpha_row = (const uint8_t *)p_alpha + (x - src_x_base);
                    egui_dim_t count = x_total - x;

                    if (ov_alpha == 0)
                    {
                        egui_image_std_blend_rgb565_alpha8_row(dst_row, src_pixels, src_alpha_row, count, canvas_alpha);
                    }
                    else
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            egui_alpha_t sa = src_alpha_row[i];
                            if (sa == 0)
                            {
                                continue;
                            }
                            egui_color_t c;
                            c.full = EGUI_COLOR_RGB565_TRANS(src_pixels[i]);
                            egui_rgb_mix_ptr(&c, &ov_color, &c, ov_alpha);
                            egui_alpha_t a = egui_color_alpha_mix(canvas_alpha, sa);
                            if (a != 0)
                            {
                                egui_image_std_blend_resize_pixel(&dst_row[i], c, a);
                            }
                        }
                    }
                }
                else
                {
                    for (egui_dim_t x_ = x; x_ < x_total; x_++)
                    {
                        egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                        egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                    }
                }
            }
            else
            {
                for (egui_dim_t x_ = x; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_8(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);
                }
            }
        }
        else
        {
            // Fast path: direct PFB access, no mask, pre-calculate row pointers
            egui_alpha_t canvas_alpha = canvas->alpha;
            egui_dim_t pfb_width = canvas->pfb_region.size.width;
            egui_dim_t count = x_total - x;
            egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

            const uint16_t *src_pixels = (const uint16_t *)p_data + (x - src_x_base);
            const uint8_t *src_alpha_row = (const uint8_t *)p_alpha + (x - src_x_base);

            egui_image_std_blend_rgb565_alpha8_row(dst_row, src_pixels, src_alpha_row, count, canvas_alpha);
        }
    }
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
void egui_image_std_set_image_rgb565_4(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                       egui_dim_t y_base)
{
    // EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_4, self, x, y, x_total, y_total, x_base, y_base);

    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    uint16_t data_row_size = image->width << 1;          // same to image->width * 2
    uint16_t alpha_row_size = ((image->width + 1) >> 1); // same to: ((image->width + 1) / 2);
    egui_dim_t src_x_base = 0;
    EGUI_UNUSED(data_row_size);
    EGUI_UNUSED(alpha_row_size);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_external_alpha_row_persistent_cache_t *row_cache = egui_image_std_get_external_alpha_row_persistent_cache();
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        if (!egui_image_std_prepare_external_alpha_row_persistent_cache_min_rows(row_cache, image, data_row_size, alpha_row_size,
                                                                                 EGUI_MIN(EGUI_CONFIG_PFB_HEIGHT, image->height)))
        {
            return;
        }
    }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        egui_dim_t rr_sy = y_base + y_;
        // Check mask row range BEFORE loading data (avoids wasted I/O for extern resources)
        egui_dim_t rr_x_start = 0, rr_x_end = 0;
        int rr_res = -1; // -1 = no mask or no row_range API
        if (canvas->mask != NULL && canvas->mask->api->mask_get_row_range != NULL)
        {
            rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &rr_x_start, &rr_x_end);
            if (rr_res == EGUI_MASK_ROW_OUTSIDE)
            {
                continue; // Skip data loading entirely for this row
            }
        }

        uint32_t row_start = y_ * image->width;
        uint32_t row_start_alpha = y_ * alpha_row_size;
        const void *p_data = NULL;
        const void *p_alpha = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            EGUI_UNUSED(row_start);
            EGUI_UNUSED(row_start_alpha);
            if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, y_))
            {
                continue;
            }
            p_data = egui_image_std_get_external_alpha_row_persistent_data(row_cache, y_);
            p_alpha = egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, y_);
        }
        else
#endif
        {
            p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
            p_alpha = (const void *)((const uint8_t *)image->alpha_buf + (row_start_alpha));
        }

        if (canvas->mask != NULL)
        {
            if (rr_res == EGUI_MASK_ROW_INSIDE)
            {
                // Fast path: row fully inside mask - direct PFB access with batch copy
                egui_alpha_t canvas_alpha = canvas->alpha;
                egui_dim_t pfb_width = canvas->pfb_region.size.width;
                egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

                egui_image_std_blend_rgb565_alpha4_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, x - src_x_base, x_total - x, canvas_alpha);
            }
            else if (rr_res == EGUI_MASK_ROW_PARTIAL)
            {
                egui_dim_t rr_img_xs = rr_x_start - x_base;
                egui_dim_t rr_img_xe = rr_x_end - x_base;
                // Left edge: per-pixel with mask
                for (egui_dim_t x_ = x; x_ < rr_img_xs; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_4(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                }
                // Middle: direct PFB access with batch copy
                {
                    egui_alpha_t canvas_alpha = canvas->alpha;
                    egui_dim_t pfb_width = canvas->pfb_region.size.width;
                    egui_dim_t dst_x = rr_x_start - canvas->pfb_location_in_base_view.x;
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x];

                    egui_image_std_blend_rgb565_alpha4_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, rr_img_xs - src_x_base,
                                                           rr_img_xe - rr_img_xs, canvas_alpha);
                }
                // Right edge: per-pixel with mask
                for (egui_dim_t x_ = rr_img_xe; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_4(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                }
            }
            else
            {
                for (egui_dim_t x_ = x; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_4(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);
                }
            }
        }
        else
        {
            // Fast path: direct PFB access with opaque-run batch copy
            egui_alpha_t canvas_alpha = canvas->alpha;
            egui_dim_t pfb_width = canvas->pfb_region.size.width;
            egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

            egui_image_std_blend_rgb565_alpha4_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, x - src_x_base, x_total - x, canvas_alpha);
        }
    }
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
void egui_image_std_set_image_rgb565_2(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                       egui_dim_t y_base)
{
    // EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_2, self, x, y, x_total, y_total, x_base, y_base);

    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    uint16_t data_row_size = image->width << 1;          // same to image->width * 2
    uint16_t alpha_row_size = ((image->width + 3) >> 2); // same to: ((image->width + 3) / 4);
    egui_dim_t src_x_base = 0;
    EGUI_UNUSED(data_row_size);
    EGUI_UNUSED(alpha_row_size);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_external_alpha_row_persistent_cache_t *row_cache = egui_image_std_get_external_alpha_row_persistent_cache();
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        if (!egui_image_std_prepare_external_alpha_row_persistent_cache_min_rows(row_cache, image, data_row_size, alpha_row_size,
                                                                                 EGUI_MIN(EGUI_CONFIG_PFB_HEIGHT, image->height)))
        {
            return;
        }
    }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        egui_dim_t rr_sy = y_base + y_;
        // Check mask row range BEFORE loading data (avoids wasted I/O for extern resources)
        egui_dim_t rr_x_start = 0, rr_x_end = 0;
        int rr_res = -1; // -1 = no mask or no row_range API
        if (canvas->mask != NULL && canvas->mask->api->mask_get_row_range != NULL)
        {
            rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &rr_x_start, &rr_x_end);
            if (rr_res == EGUI_MASK_ROW_OUTSIDE)
            {
                continue; // Skip data loading entirely for this row
            }
        }

        uint32_t row_start = y_ * image->width;
        uint32_t row_start_alpha = y_ * alpha_row_size;
        const void *p_data = NULL;
        const void *p_alpha = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            EGUI_UNUSED(row_start);
            EGUI_UNUSED(row_start_alpha);
            if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, y_))
            {
                continue;
            }
            p_data = egui_image_std_get_external_alpha_row_persistent_data(row_cache, y_);
            p_alpha = egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, y_);
        }
        else
#endif
        {
            p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
            p_alpha = (const void *)((const uint8_t *)image->alpha_buf + (row_start_alpha));
        }

        if (canvas->mask != NULL)
        {
            if (rr_res == EGUI_MASK_ROW_INSIDE)
            {
                // Fast path: row fully inside mask - direct PFB access with batch copy
                egui_alpha_t canvas_alpha = canvas->alpha;
                egui_dim_t pfb_width = canvas->pfb_region.size.width;
                egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

                egui_image_std_blend_rgb565_alpha2_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, x - src_x_base, x_total - x, canvas_alpha);
            }
            else if (rr_res == EGUI_MASK_ROW_PARTIAL)
            {
                egui_dim_t rr_img_xs = rr_x_start - x_base;
                egui_dim_t rr_img_xe = rr_x_end - x_base;
                // Left edge: per-pixel with mask
                for (egui_dim_t x_ = x; x_ < rr_img_xs; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_2(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                }
                // Middle: direct PFB access with batch copy
                {
                    egui_alpha_t canvas_alpha = canvas->alpha;
                    egui_dim_t pfb_width = canvas->pfb_region.size.width;
                    egui_dim_t dst_x = rr_x_start - canvas->pfb_location_in_base_view.x;
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x];

                    egui_image_std_blend_rgb565_alpha2_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, rr_img_xs - src_x_base,
                                                           rr_img_xe - rr_img_xs, canvas_alpha);
                }
                // Right edge: per-pixel with mask
                for (egui_dim_t x_ = rr_img_xe; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_2(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                }
            }
            else
            {
                for (egui_dim_t x_ = x; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_2(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);
                }
            }
        }
        else
        {
            // Fast path: direct PFB access with opaque-run batch copy
            egui_alpha_t canvas_alpha = canvas->alpha;
            egui_dim_t pfb_width = canvas->pfb_region.size.width;
            egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

            egui_image_std_blend_rgb565_alpha2_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, x - src_x_base, x_total - x, canvas_alpha);
        }
    }
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
void egui_image_std_set_image_rgb565_1(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                       egui_dim_t y_base)
{
    // EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565_1, self, x, y, x_total, y_total, x_base, y_base);

    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    uint16_t data_row_size = image->width << 1;          // same to image->width * 2
    uint16_t alpha_row_size = ((image->width + 7) >> 3); // same to ((image->width + 7) / 8);
    egui_dim_t src_x_base = 0;
    EGUI_UNUSED(data_row_size);
    EGUI_UNUSED(alpha_row_size);
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_external_alpha_row_persistent_cache_t *row_cache = egui_image_std_get_external_alpha_row_persistent_cache();
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        if (!egui_image_std_prepare_external_alpha_row_persistent_cache_min_rows(row_cache, image, data_row_size, alpha_row_size,
                                                                                 EGUI_MIN(EGUI_CONFIG_PFB_HEIGHT, image->height)))
        {
            return;
        }
    }
#endif // EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        egui_dim_t rr_sy = y_base + y_;
        // Check mask row range BEFORE loading data (avoids wasted I/O for extern resources)
        egui_dim_t rr_x_start = 0, rr_x_end = 0;
        int rr_res = -1; // -1 = no mask or no row_range API
        if (canvas->mask != NULL && canvas->mask->api->mask_get_row_range != NULL)
        {
            rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, x_base + x, x_base + x_total, &rr_x_start, &rr_x_end);
            if (rr_res == EGUI_MASK_ROW_OUTSIDE)
            {
                continue; // Skip data loading entirely for this row
            }
        }

        uint32_t row_start = y_ * image->width;
        uint32_t row_start_alpha = y_ * alpha_row_size;
        const void *p_data = NULL;
        const void *p_alpha = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            EGUI_UNUSED(row_start);
            EGUI_UNUSED(row_start_alpha);
            if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, y_))
            {
                continue;
            }
            p_data = egui_image_std_get_external_alpha_row_persistent_data(row_cache, y_);
            p_alpha = egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, y_);
        }
        else
#endif
        {
            p_data = (const void *)((const uint8_t *)image->data_buf + (row_start << 1));
            p_alpha = (const void *)((const uint8_t *)image->alpha_buf + (row_start_alpha));
        }

        if (canvas->mask != NULL)
        {
            if (rr_res == EGUI_MASK_ROW_INSIDE)
            {
                // Fast path: row fully inside mask - direct PFB access with batch copy
                egui_alpha_t canvas_alpha = canvas->alpha;
                egui_dim_t pfb_width = canvas->pfb_region.size.width;
                egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

                egui_image_std_blend_rgb565_alpha1_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, x - src_x_base, x_total - x, canvas_alpha);
            }
            else if (rr_res == EGUI_MASK_ROW_PARTIAL)
            {
                egui_dim_t rr_img_xs = rr_x_start - x_base;
                egui_dim_t rr_img_xe = rr_x_end - x_base;
                // Left edge: per-pixel with mask
                for (egui_dim_t x_ = x; x_ < rr_img_xs; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_1(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                }
                // Middle: direct PFB access with batch copy
                {
                    egui_alpha_t canvas_alpha = canvas->alpha;
                    egui_dim_t pfb_width = canvas->pfb_region.size.width;
                    egui_dim_t dst_x = rr_x_start - canvas->pfb_location_in_base_view.x;
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x];

                    egui_image_std_blend_rgb565_alpha1_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, rr_img_xs - src_x_base,
                                                           rr_img_xe - rr_img_xs, canvas_alpha);
                }
                // Right edge: per-pixel with mask
                for (egui_dim_t x_ = rr_img_xe; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_1(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), rr_sy, color, alpha);
                }
            }
            else
            {
                for (egui_dim_t x_ = x; x_ < x_total; x_++)
                {
                    egui_image_std_get_col_pixel_rgb565_1(p_data, p_alpha, x_ - src_x_base, &color, &alpha);
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, alpha);
                }
            }
        }
        else
        {
            // Fast path: direct PFB access with opaque-run batch copy
            egui_alpha_t canvas_alpha = canvas->alpha;
            egui_dim_t pfb_width = canvas->pfb_region.size.width;
            egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];

            egui_image_std_blend_rgb565_alpha1_row(dst_row, (const uint16_t *)p_data, (const uint8_t *)p_alpha, x - src_x_base, x_total - x, canvas_alpha);
        }
    }
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
void egui_image_std_set_image_rgb565(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                     egui_dim_t y_base)
{
    if ((egui_canvas_get_canvas()->alpha == EGUI_ALPHA_100) && (egui_canvas_get_canvas()->mask == NULL))
    {
        egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
        egui_canvas_t *canvas = egui_canvas_get_canvas();
        egui_dim_t pfb_width = canvas->pfb_region.size.width;
        egui_dim_t count = x_total - x;
        egui_dim_t img_width = image->width;

        // Pre-calculate starting pointers (avoid per-row multiply)
        egui_dim_t dst_y_start = (y_base + y) - canvas->pfb_location_in_base_view.y;
        egui_dim_t dst_x_start = (x_base + x) - canvas->pfb_location_in_base_view.x;
        egui_color_int_t *dst_row = &canvas->pfb[dst_y_start * pfb_width + dst_x_start];

        const uint16_t *src_row = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        egui_image_std_external_data_row_persistent_cache_t *row_cache = egui_image_std_get_external_data_row_persistent_cache();
        if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            uint32_t data_source_row_size = (uint32_t)img_width << 1;
            uint32_t data_row_start_offset = (uint32_t)x << 1;
            uint32_t data_row_size = (uint32_t)count << 1;

            if (!egui_image_std_prepare_external_data_row_persistent_cache_range_rows(row_cache, image, data_source_row_size, data_row_start_offset,
                                                                                      data_row_size, EGUI_MIN(EGUI_CONFIG_PFB_HEIGHT, image->height)))
            {
                return;
            }
        }
        else
#endif
        {
            src_row = (const uint16_t *)image->data_buf + (y * img_width) + x;
        }

        for (egui_dim_t y_ = y; y_ < y_total; y_++)
        {
            const uint16_t *src = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
            {
                if (!egui_image_std_load_external_data_row_persistent_cache(row_cache, y_))
                {
                    continue;
                }
                src = egui_image_std_get_external_data_row_persistent_data(row_cache, y_);
            }
            else
#endif
            {
                src = src_row;
                src_row += img_width;
            }

            egui_image_std_copy_rgb565_row(dst_row, src, count);

            // Advance to next row using addition (no multiply)
            dst_row += pfb_width;
        }
    }
    else if ((egui_canvas_get_canvas()->alpha == EGUI_ALPHA_100) && (egui_canvas_get_canvas()->mask != NULL) &&
             (egui_canvas_get_canvas()->mask->api->mask_get_row_overlay != NULL))
    {
        // Fast path: RGB565 image with row-level overlay (e.g. linear-vertical gradient overlay).
        // Pre-compute blend factors per row and write directly to PFB.
        egui_image_std_info_t *image_info = (egui_image_std_info_t *)self->res;
        egui_canvas_t *canvas_ctx = egui_canvas_get_canvas();
        egui_dim_t pfb_width = canvas_ctx->pfb_region.size.width;
        egui_dim_t count = x_total - x;
        egui_dim_t img_width = image_info->width;

        egui_dim_t dst_y_start = (y_base + y) - canvas_ctx->pfb_location_in_base_view.y;
        egui_dim_t dst_x_start = (x_base + x) - canvas_ctx->pfb_location_in_base_view.x;
        egui_color_int_t *dst_row = &canvas_ctx->pfb[dst_y_start * pfb_width + dst_x_start];

        const uint16_t *src_row = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (image_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            egui_image_std_set_image_resize_rgb565_external(self, x, y, x_total, y_total, x_base, y_base, EGUI_FLOAT_VALUE_INT(1), EGUI_FLOAT_VALUE_INT(1));
            return;
        }
        else
#endif
        {
            src_row = (const uint16_t *)image_info->data_buf + (y * img_width) + x;

            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t screen_y = y_base + y_;
                egui_color_t ov_color;
                egui_alpha_t ov_alpha;

                if (canvas_ctx->mask->api->mask_get_row_overlay(canvas_ctx->mask, screen_y, &ov_color, &ov_alpha))
                {
                    if (ov_alpha > 251)
                    {
                        // Nearly fully opaque overlay: fill with overlay color
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            dst_row[i] = ov_color.full;
                        }
                    }
                    else if (ov_alpha < 4)
                    {
                        // Nearly transparent overlay: just copy source
                        egui_image_std_copy_rgb565_row(dst_row, src_row, count);
                    }
                    else
                    {
                        // Typical case: blend overlay color into each source pixel
                        uint16_t fg = ov_color.full;
                        uint32_t fg_rb_g = (fg | ((uint32_t)fg << 16)) & 0x07E0F81FUL;
                        uint32_t alpha5 = (uint32_t)ov_alpha >> 3;

                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            uint16_t bg = src_row[i];
                            uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                            uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * alpha5 >> 5)) & 0x07E0F81FUL;
                            dst_row[i] = (uint16_t)(result | (result >> 16));
                        }
                    }
                }
                else
                {
                    // Overlay not applicable: per-pixel mask fallback
                    for (egui_dim_t i = 0; i < count; i++)
                    {
                        egui_color_t color;
                        egui_alpha_t alpha;
                        color.full = src_row[i];
                        alpha = EGUI_ALPHA_100;
                        canvas_ctx->mask->api->mask_point(canvas_ctx->mask, x_base + x + i, screen_y, &color, &alpha);
                        if (alpha == 0)
                            continue;
                        if (alpha == EGUI_ALPHA_100)
                        {
                            dst_row[i] = color.full;
                        }
                        else
                        {
                            egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);
                        }
                    }
                }

                src_row += img_width;
                dst_row += pfb_width;
            }
        }
    }
    else
    {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        if (((egui_image_std_info_t *)self->res)->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
        {
            egui_image_std_set_image_resize_rgb565_external(self, x, y, x_total, y_total, x_base, y_base, EGUI_FLOAT_VALUE_INT(1), EGUI_FLOAT_VALUE_INT(1));
            return;
        }
#endif
        EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb565, self, x, y, x_total, y_total, x_base, y_base);
    }
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
void egui_image_std_set_image_rgb32(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                    egui_dim_t y_base)
{
    EGUI_IMAGE_STD_DRAW_IMAGE_FUNC_DEFINE(egui_image_std_get_pixel_rgb32, self, x, y, x_total, y_total, x_base, y_base);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
typedef struct
{
    const egui_image_std_info_t *image;
    uint8_t is_all_opaque;
} egui_image_std_alpha_opaque_cache_t;

static egui_image_std_alpha_opaque_cache_t g_egui_image_std_alpha_opaque_cache[4];
static uint8_t g_egui_image_std_alpha_opaque_cache_next = 0;

__EGUI_STATIC_INLINE__ uint16_t egui_image_std_rgb565_alpha_row_size(egui_dim_t width, uint8_t alpha_type)
{
    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_1:
        return (uint16_t)((width + 7) >> 3);
    case EGUI_IMAGE_ALPHA_TYPE_2:
        return (uint16_t)((width + 3) >> 2);
    case EGUI_IMAGE_ALPHA_TYPE_4:
        return (uint16_t)((width + 1) >> 1);
    case EGUI_IMAGE_ALPHA_TYPE_8:
        return (uint16_t)(width);
    default:
        return 0;
    }
}

__EGUI_STATIC_INLINE__ int egui_image_std_rgb565_alpha_row_is_all_opaque(const uint8_t *alpha_row, egui_dim_t width, uint8_t alpha_type)
{
    uint16_t full_bytes = 0;
    uint8_t partial_bits = 0;

    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_1:
        full_bytes = (uint16_t)(width >> 3);
        partial_bits = (uint8_t)(width & 0x07);
        break;
    case EGUI_IMAGE_ALPHA_TYPE_2:
        full_bytes = (uint16_t)(width >> 2);
        partial_bits = (uint8_t)((width & 0x03) << 1);
        break;
    case EGUI_IMAGE_ALPHA_TYPE_4:
        full_bytes = (uint16_t)(width >> 1);
        partial_bits = (uint8_t)((width & 0x01) << 2);
        break;
    case EGUI_IMAGE_ALPHA_TYPE_8:
        full_bytes = (uint16_t)(width);
        partial_bits = 0;
        break;
    default:
        return 0;
    }

    for (uint16_t i = 0; i < full_bytes; i++)
    {
        if (alpha_row[i] != 0xFF)
        {
            return 0;
        }
    }

    if (partial_bits != 0)
    {
        uint8_t mask = (uint8_t)((1u << partial_bits) - 1u);
        if ((alpha_row[full_bytes] & mask) != mask)
        {
            return 0;
        }
    }

    return 1;
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
__EGUI_STATIC_INLINE__ int egui_image_std_rgb565_external_alpha_is_all_opaque(const egui_image_std_info_t *image, uint16_t alpha_row_size)
{
    uint8_t *alpha_buf = egui_image_std_get_external_alpha_probe_buf();
    uint32_t chunk_rows;
    int is_all_opaque = 1;

    if (alpha_row_size == 0 || alpha_row_size > EGUI_IMAGE_STD_EXTERNAL_ALPHA_CACHE_MAX_BYTES)
    {
        return 0;
    }

    chunk_rows = (uint32_t)(EGUI_IMAGE_STD_EXTERNAL_ALPHA_CACHE_MAX_BYTES / alpha_row_size);

    if (chunk_rows == 0)
    {
        chunk_rows = 1;
    }

    for (egui_dim_t y = 0; y < image->height; y += (egui_dim_t)chunk_rows)
    {
        egui_dim_t rows_in_chunk = image->height - y;
        const uint8_t *chunk_ptr = alpha_buf;

        if (rows_in_chunk > (egui_dim_t)chunk_rows)
        {
            rows_in_chunk = (egui_dim_t)chunk_rows;
        }

        egui_image_std_load_alpha_resource(alpha_buf, (egui_image_std_info_t *)image, (uint32_t)y * alpha_row_size, (uint32_t)rows_in_chunk * alpha_row_size);

        for (egui_dim_t row = 0; row < rows_in_chunk; row++)
        {
            if (!egui_image_std_rgb565_alpha_row_is_all_opaque(chunk_ptr, image->width, image->alpha_type))
            {
                is_all_opaque = 0;
                break;
            }
            chunk_ptr += alpha_row_size;
        }

        if (!is_all_opaque)
        {
            break;
        }
    }

    return is_all_opaque;
}
#endif

__EGUI_STATIC_INLINE__ int egui_image_std_rgb565_alpha_is_all_opaque(const egui_image_std_info_t *image)
{
    if (image == NULL || image->data_type != EGUI_IMAGE_DATA_TYPE_RGB565 || image->alpha_buf == NULL)
    {
        return 0;
    }

    for (uint8_t i = 0; i < (uint8_t)(sizeof(g_egui_image_std_alpha_opaque_cache) / sizeof(g_egui_image_std_alpha_opaque_cache[0])); i++)
    {
        if (g_egui_image_std_alpha_opaque_cache[i].image == image)
        {
            return g_egui_image_std_alpha_opaque_cache[i].is_all_opaque;
        }
    }

    {
        const uint8_t *alpha = (const uint8_t *)image->alpha_buf;
        uint16_t alpha_row_size = egui_image_std_rgb565_alpha_row_size(image->width, image->alpha_type);
        int is_all_opaque = 1;

        if (image->width == 0 || image->height == 0 || alpha_row_size == 0)
        {
            is_all_opaque = 0;
        }
        else
        {
            if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
            {
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                is_all_opaque = egui_image_std_rgb565_external_alpha_is_all_opaque(image, alpha_row_size);
#else
                is_all_opaque = 0;
#endif
            }
            else
            {
                for (egui_dim_t y = 0; y < image->height; y++)
                {
                    if (!egui_image_std_rgb565_alpha_row_is_all_opaque(alpha, image->width, image->alpha_type))
                    {
                        is_all_opaque = 0;
                        break;
                    }
                    alpha += alpha_row_size;
                }
            }
        }

        g_egui_image_std_alpha_opaque_cache[g_egui_image_std_alpha_opaque_cache_next].image = image;
        g_egui_image_std_alpha_opaque_cache[g_egui_image_std_alpha_opaque_cache_next].is_all_opaque = (uint8_t)is_all_opaque;
        g_egui_image_std_alpha_opaque_cache_next++;
        g_egui_image_std_alpha_opaque_cache_next &= 0x03;
        return is_all_opaque;
    }
}

int egui_image_std_rgb565_is_opaque_source(const egui_image_std_info_t *image)
{
    if (image == NULL || image->data_type != EGUI_IMAGE_DATA_TYPE_RGB565)
    {
        return 0;
    }

    if (image->alpha_buf == NULL)
    {
        return 1;
    }

    return egui_image_std_rgb565_alpha_is_all_opaque(image);
}

__EGUI_STATIC_INLINE__ int egui_image_std_rgb565_can_use_opaque_draw_fast_path(const egui_image_std_info_t *image, const egui_canvas_t *canvas)
{
    if (!egui_image_std_rgb565_is_opaque_source(image))
    {
        return 0;
    }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        return canvas == NULL || (canvas->alpha == EGUI_ALPHA_100 && canvas->mask == NULL);
    }
#endif

    if (canvas == NULL || canvas->mask == NULL)
    {
        return 1;
    }

    return canvas->mask->api->kind == EGUI_MASK_KIND_CIRCLE || canvas->mask->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE;
}

__EGUI_STATIC_INLINE__ int egui_image_std_rgb565_can_use_opaque_resize_fast_path(const egui_image_std_info_t *image, const egui_canvas_t *canvas)
{
    if (!egui_image_std_rgb565_is_opaque_source(image))
    {
        return 0;
    }

    if (canvas == NULL || canvas->mask == NULL)
    {
        return 1;
    }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        return 1;
    }
#endif

    return canvas->mask->api->kind == EGUI_MASK_KIND_CIRCLE || canvas->mask->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE;
}
#endif

void egui_image_std_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    const egui_image_t *draw_self = self;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_t cached_self;
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        draw_self = egui_image_std_resolve_external_persistent_image(self, &cached_self);
    }
#endif
    // egui_color_t color;
    // egui_alpha_t alpha;
    egui_dim_t width = image->width;
    egui_dim_t height = image->height;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);

    if (egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
    if (image->alpha_buf != NULL && egui_image_std_rgb565_can_use_opaque_draw_fast_path(image, egui_canvas_get_canvas()))
    {
        egui_image_std_set_image_rgb565(draw_self, region.location.x, region.location.y, x_total, y_total, x, y);
        return;
    }
#endif

    if (image)
    {
        switch (image->data_type)
        {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB32:
            egui_image_std_set_image_rgb32(draw_self, region.location.x, region.location.y, x_total, y_total, x, y);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB565:
            if (image->alpha_buf == NULL)
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
                egui_image_std_set_image_rgb565(draw_self, region.location.x, region.location.y, x_total, y_total, x, y);
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
            }
            else
            {
                switch (image->alpha_type)
                {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
                case EGUI_IMAGE_ALPHA_TYPE_1:
                    egui_image_std_set_image_rgb565_1(draw_self, region.location.x, region.location.y, x_total, y_total, x, y);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
                case EGUI_IMAGE_ALPHA_TYPE_2:
                    egui_image_std_set_image_rgb565_2(draw_self, region.location.x, region.location.y, x_total, y_total, x, y);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
                case EGUI_IMAGE_ALPHA_TYPE_4:
                    egui_image_std_set_image_rgb565_4(draw_self, region.location.x, region.location.y, x_total, y_total, x, y);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                case EGUI_IMAGE_ALPHA_TYPE_8:
                    egui_image_std_set_image_rgb565_8(draw_self, region.location.x, region.location.y, x_total, y_total, x, y);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                default:
                    EGUI_ASSERT(0);
                    break;
                }
            }
            break;
        default:
            EGUI_ASSERT(0);
            break;
        }
    }
}

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
__EGUI_STATIC_INLINE__ int egui_image_std_set_image_resize_rgb565_8_round_rect_fast(const egui_image_std_info_t *image, const egui_dim_t *src_x_map,
                                                                                    egui_dim_t count, egui_dim_t x, egui_dim_t y, egui_dim_t x_total,
                                                                                    egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base,
                                                                                    egui_float_t height_radio, egui_alpha_t canvas_alpha, int use_repeat2_fast,
                                                                                    int use_repeat4_fast
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                                                                                    ,
                                                                                    egui_image_std_external_alpha_row_persistent_cache_t *row_cache
#endif
);

static void egui_image_std_set_image_resize_rgb565_8_common(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                            egui_dim_t x_base, egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_dim_t src_y;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_alpha_t canvas_alpha = canvas->alpha;
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t screen_x_start = x_base + x;
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;
    egui_dim_t count = x_total - x;
    egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];
    const uint16_t *src_row = NULL;
    const uint8_t *src_alpha_row = NULL;
    int cached_src_y = -1;
    int use_repeat4_fast = 0;
    int use_repeat2_fast = egui_image_std_can_use_resize_repeat2_fast_path(x, y, x_total, y_total, width_radio, height_radio);
    int use_mask_inside_repeat_fast = 0;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_external_alpha_row_persistent_cache_t *row_cache = egui_image_std_get_external_alpha_row_persistent_cache();
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        if (!egui_image_std_prepare_external_alpha_row_persistent_cache_min_rows(row_cache, image, image->width << 1, image->width,
                                                                                 EGUI_MIN(EGUI_CONFIG_PFB_HEIGHT, image->height)))
        {
            return;
        }
    }
#endif

    count = egui_image_std_prepare_resize_src_x_map_limit(src_x_map, x, x_total, width_radio);

    if (canvas->mask != NULL)
    {
        egui_dim_t mask_x = canvas->mask->region.location.x;
        egui_dim_t mask_x_end = mask_x + canvas->mask->region.size.width;

        use_mask_inside_repeat_fast = (use_repeat2_fast || use_repeat4_fast) && screen_x_start <= mask_x && (x_base + x_total) >= mask_x_end;

        if (egui_image_std_set_image_resize_rgb565_8_round_rect_fast(image, src_x_map, count, x, y, x_total, y_total, x_base, y_base, height_radio,
                                                                     canvas_alpha, use_repeat2_fast, use_mask_inside_repeat_fast ? use_repeat4_fast : 0
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                                                                     ,
                                                                     row_cache
#endif
                                                                     ))
        {
            return;
        }

        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t rr_x_start, rr_x_end;
            int use_circle_edge_fast_path = (canvas->mask->api->kind == EGUI_MASK_KIND_CIRCLE);
            egui_mask_circle_t *circle_mask_fast = NULL;

            if (use_circle_edge_fast_path)
            {
                circle_mask_fast = (egui_mask_circle_t *)canvas->mask;
            }

            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                egui_dim_t circle_visible_half = 0;
                egui_dim_t visible_x_start = screen_x_start;
                egui_dim_t visible_x_end = x_base + x_total;
                int rr_res;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    uint32_t row_start = src_y * image->width;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
                    {
                        if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, src_y))
                        {
                            continue;
                        }
                        src_row = egui_image_std_get_external_alpha_row_persistent_data(row_cache, src_y);
                        src_alpha_row = egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, src_y);
                    }
                    else
#endif
                    {
                        src_row = (const uint16_t *)image->data_buf + row_start;
                        src_alpha_row = (const uint8_t *)image->alpha_buf + row_start;
                    }
                    cached_src_y = src_y;
                }

                if (use_circle_edge_fast_path)
                {
                    egui_dim_t opaque_boundary;

                    if (!egui_image_std_prepare_circle_mask_row_fast(circle_mask_fast, rr_sy, NULL, &circle_visible_half, &opaque_boundary))
                    {
                        continue;
                    }

                    visible_x_start = EGUI_MAX(circle_mask_fast->center_x - circle_visible_half, screen_x_start);
                    visible_x_end = EGUI_MIN(circle_mask_fast->center_x + circle_visible_half + 1, x_base + x_total);
                    if (visible_x_start >= visible_x_end)
                    {
                        continue;
                    }

                    rr_x_start = EGUI_MAX(circle_mask_fast->center_x - circle_mask_fast->radius + opaque_boundary, screen_x_start);
                    rr_x_end = EGUI_MIN(circle_mask_fast->center_x + circle_mask_fast->radius - opaque_boundary + 1, x_base + x_total);
                    if (rr_x_start >= rr_x_end)
                    {
                        rr_x_start = screen_x_start;
                        rr_x_end = screen_x_start;
                        rr_res = EGUI_MASK_ROW_PARTIAL;
                    }
                    else if (rr_x_start <= screen_x_start && rr_x_end >= x_base + x_total)
                    {
                        rr_res = EGUI_MASK_ROW_INSIDE;
                    }
                    else
                    {
                        rr_res = EGUI_MASK_ROW_PARTIAL;
                    }
                }
                else
                {
                    rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);
                }

                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }

                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    if (use_mask_inside_repeat_fast && egui_image_std_blend_rgb565_alpha8_repeat_segment(dst_row, src_row, src_alpha_row, x, count,
                                                                                                         canvas_alpha, use_repeat2_fast, use_repeat4_fast))
                    {
                        continue;
                    }
                    egui_image_std_blend_rgb565_alpha8_mapped_row(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);
                }
                else
                {
                    if (!use_circle_edge_fast_path && canvas->mask->api->mask_get_row_visible_range != NULL &&
                        !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start, &visible_x_end))
                    {
                        continue;
                    }

                    egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);
                    egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);
                    egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                    egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);

                    egui_dim_t left_img_xe = EGUI_MIN(rr_img_xs, vis_img_xe);
                    if (vis_img_xs < left_img_xe)
                    {
                        egui_dim_t left_offset = vis_img_xs - x;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + vis_img_xs - canvas->pfb_location_in_base_view.x)];

                        egui_image_std_blend_rgb565_alpha8_masked_mapped_segment(canvas, dst_row, src_row, src_alpha_row, &src_x_map[left_offset],
                                                                                 left_img_xe - vis_img_xs, x_base + vis_img_xs, rr_sy, canvas_alpha);
                    }

                    if (rr_img_xs < rr_img_xe)
                    {
                        egui_dim_t mid_offset = rr_img_xs - x;
                        egui_dim_t mid_count = rr_img_xe - rr_img_xs;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];

                        egui_image_std_blend_rgb565_alpha8_mapped_row(dst_row, src_row, src_alpha_row, &src_x_map[mid_offset], mid_count, canvas_alpha);
                    }

                    egui_dim_t right_img_xs = EGUI_MAX(rr_img_xe, vis_img_xs);
                    if (right_img_xs < vis_img_xe)
                    {
                        egui_dim_t right_offset = right_img_xs - x;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + right_img_xs - canvas->pfb_location_in_base_view.x)];

                        egui_image_std_blend_rgb565_alpha8_masked_mapped_segment(canvas, dst_row, src_row, src_alpha_row, &src_x_map[right_offset],
                                                                                 vis_img_xe - right_img_xs, x_base + right_img_xs, rr_sy, canvas_alpha);
                    }
                }
            }
        }
        else if (canvas->mask->api->mask_get_row_overlay != NULL)
        {
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    uint32_t row_start = src_y * image->width;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
                    {
                        if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, src_y))
                        {
                            continue;
                        }
                        src_row = egui_image_std_get_external_alpha_row_persistent_data(row_cache, src_y);
                        src_alpha_row = egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, src_y);
                    }
                    else
#endif
                    {
                        src_row = (const uint16_t *)image->data_buf + row_start;
                        src_alpha_row = (const uint8_t *)image->alpha_buf + row_start;
                    }
                    cached_src_y = src_y;
                }

                egui_color_t ov_color;
                egui_alpha_t ov_alpha;
                if (canvas->mask->api->mask_get_row_overlay(canvas->mask, rr_sy, &ov_color, &ov_alpha))
                {
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    if (ov_alpha == 0)
                    {
                        egui_image_std_blend_rgb565_alpha8_mapped_row(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);
                    }
                    else
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            egui_dim_t src_x = src_x_map[i];
                            egui_alpha_t sa = src_alpha_row[src_x];
                            if (sa == 0)
                            {
                                continue;
                            }
                            egui_color_t c;
                            c.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                            egui_rgb_mix_ptr(&c, &ov_color, &c, ov_alpha);
                            egui_alpha_t a = egui_color_alpha_mix(canvas_alpha, sa);
                            if (a != 0)
                            {
                                egui_image_std_blend_resize_pixel(&dst_row[i], c, a);
                            }
                        }
                    }
                }
                else
                {
                    for (egui_dim_t i = 0; i < count; i++)
                    {
                        egui_dim_t src_x = src_x_map[i];
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                        alpha = src_alpha_row[src_x];
                        egui_canvas_draw_point_limit(screen_x_start + i, rr_sy, color, alpha);
                    }
                }
            }
        }
        else
        {
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    uint32_t row_start = src_y * image->width;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
                    {
                        if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, src_y))
                        {
                            continue;
                        }
                        src_row = egui_image_std_get_external_alpha_row_persistent_data(row_cache, src_y);
                        src_alpha_row = egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, src_y);
                    }
                    else
#endif
                    {
                        src_row = (const uint16_t *)image->data_buf + row_start;
                        src_alpha_row = (const uint8_t *)image->alpha_buf + row_start;
                    }
                    cached_src_y = src_y;
                }

                for (egui_dim_t i = 0; i < count; i++)
                {
                    egui_dim_t src_x = src_x_map[i];
                    color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);
                    alpha = src_alpha_row[src_x];
                    egui_canvas_draw_point_limit(screen_x_start + i, y_base + y_, color, alpha);
                }
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = y; y_ < y_total; y_++)
        {
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);
            if (cached_src_y != src_y)
            {
                uint32_t row_start = src_y * image->width;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
                {
                    if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, src_y))
                    {
                        continue;
                    }
                    src_row = egui_image_std_get_external_alpha_row_persistent_data(row_cache, src_y);
                    src_alpha_row = egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, src_y);
                }
                else
#endif
                {
                    src_row = (const uint16_t *)image->data_buf + row_start;
                    src_alpha_row = (const uint8_t *)image->alpha_buf + row_start;
                }
                cached_src_y = src_y;
            }

            if ((use_repeat2_fast || use_repeat4_fast) &&
                egui_image_std_blend_rgb565_alpha8_repeat_segment(dst_row, src_row, src_alpha_row, x, count, canvas_alpha, use_repeat2_fast, use_repeat4_fast))
            {
                continue;
            }
            egui_image_std_blend_rgb565_alpha8_mapped_row(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);
        }
    }
}
#endif

#define EGUI_IMAGE_STD_SET_IMAGE_RESIZE_RGB565_PACKED_ALPHA_COMMON(_func_name, _alpha_row_size_expr, _get_alpha_func, _blend_row_func,                         \
                                                                   _blend_repeat2_row_func, _blend_repeat4_row_func)                                           \
    static void _func_name(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base, \
                           egui_float_t width_radio, egui_float_t height_radio)                                                                                \
    {                                                                                                                                                          \
        egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;                                                                                     \
        egui_color_t color;                                                                                                                                    \
        egui_alpha_t alpha;                                                                                                                                    \
        egui_dim_t src_y;                                                                                                                                      \
        egui_canvas_t *canvas = egui_canvas_get_canvas();                                                                                                      \
        egui_alpha_t canvas_alpha = canvas->alpha;                                                                                                             \
        egui_dim_t pfb_width = canvas->pfb_region.size.width;                                                                                                  \
        egui_dim_t screen_x_start = x_base + x;                                                                                                                \
        egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;                                                                         \
        egui_dim_t count = x_total - x;                                                                                                                        \
        egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];                                                                                                           \
        const uint16_t *src_row;                                                                                                                               \
        const uint8_t *src_alpha_row;                                                                                                                          \
        uint32_t alpha_row_size = (_alpha_row_size_expr);                                                                                                      \
                                                                                                                                                               \
        if (canvas->mask == NULL && _blend_repeat4_row_func != NULL &&                                                                                         \
            egui_image_std_can_use_resize_repeat4_fast_path(x, y, x_total, y_total, width_radio, height_radio))                                                \
        {                                                                                                                                                      \
            egui_dim_t src_x_start = x >> 2;                                                                                                                   \
            egui_dim_t src_count = (x_total - x) >> 2;                                                                                                         \
                                                                                                                                                               \
            for (egui_dim_t y_ = y; y_ < y_total; y_ += 4)                                                                                                     \
            {                                                                                                                                                  \
                egui_dim_t src_row_index = y_ >> 2;                                                                                                            \
                egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;                                                                        \
                egui_color_int_t *dst_row0 = &canvas->pfb[dst_y * pfb_width + dst_x_start];                                                                    \
                egui_color_int_t *dst_row1 = &canvas->pfb[(dst_y + 1) * pfb_width + dst_x_start];                                                              \
                egui_color_int_t *dst_row2 = &canvas->pfb[(dst_y + 2) * pfb_width + dst_x_start];                                                              \
                egui_color_int_t *dst_row3 = &canvas->pfb[(dst_y + 3) * pfb_width + dst_x_start];                                                              \
                                                                                                                                                               \
                src_row = (const uint16_t *)image->data_buf + (src_row_index * image->width);                                                                  \
                src_alpha_row = (const uint8_t *)image->alpha_buf + (src_row_index * alpha_row_size);                                                          \
                _blend_repeat4_row_func(dst_row0, src_row, src_alpha_row, src_x_start, src_count, canvas_alpha);                                               \
                _blend_repeat4_row_func(dst_row1, src_row, src_alpha_row, src_x_start, src_count, canvas_alpha);                                               \
                _blend_repeat4_row_func(dst_row2, src_row, src_alpha_row, src_x_start, src_count, canvas_alpha);                                               \
                _blend_repeat4_row_func(dst_row3, src_row, src_alpha_row, src_x_start, src_count, canvas_alpha);                                               \
            }                                                                                                                                                  \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
                                                                                                                                                               \
        if (canvas->mask == NULL && _blend_repeat2_row_func != NULL &&                                                                                         \
            egui_image_std_can_use_resize_repeat2_fast_path(x, y, x_total, y_total, width_radio, height_radio))                                                \
        {                                                                                                                                                      \
            egui_dim_t src_x_start = x >> 1;                                                                                                                   \
            egui_dim_t src_count = (x_total - x) >> 1;                                                                                                         \
                                                                                                                                                               \
            for (egui_dim_t y_ = y; y_ < y_total; y_ += 2)                                                                                                     \
            {                                                                                                                                                  \
                egui_dim_t src_row_index = y_ >> 1;                                                                                                            \
                egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;                                                                        \
                egui_color_int_t *dst_row0 = &canvas->pfb[dst_y * pfb_width + dst_x_start];                                                                    \
                egui_color_int_t *dst_row1 = &canvas->pfb[(dst_y + 1) * pfb_width + dst_x_start];                                                              \
                                                                                                                                                               \
                src_row = (const uint16_t *)image->data_buf + (src_row_index * image->width);                                                                  \
                src_alpha_row = (const uint8_t *)image->alpha_buf + (src_row_index * alpha_row_size);                                                          \
                _blend_repeat2_row_func(dst_row0, src_row, src_alpha_row, src_x_start, src_count, canvas_alpha);                                               \
                _blend_repeat2_row_func(dst_row1, src_row, src_alpha_row, src_x_start, src_count, canvas_alpha);                                               \
            }                                                                                                                                                  \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
                                                                                                                                                               \
        count = egui_image_std_prepare_resize_src_x_map_limit(src_x_map, x, x_total, width_radio);                                                             \
        if (canvas->mask != NULL)                                                                                                                              \
        {                                                                                                                                                      \
            if (canvas->mask->api->mask_get_row_range != NULL)                                                                                                 \
            {                                                                                                                                                  \
                egui_dim_t rr_x_start, rr_x_end;                                                                                                               \
                for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                    \
                {                                                                                                                                              \
                    egui_dim_t rr_sy = y_base + y_;                                                                                                            \
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;                                                                            \
                    src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);                                                                               \
                    src_row = (const uint16_t *)image->data_buf + (src_y * image->width);                                                                      \
                    src_alpha_row = (const uint8_t *)image->alpha_buf + (src_y * alpha_row_size);                                                              \
                                                                                                                                                               \
                    int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);         \
                    if (rr_res == EGUI_MASK_ROW_OUTSIDE)                                                                                                       \
                    {                                                                                                                                          \
                        continue;                                                                                                                              \
                    }                                                                                                                                          \
                                                                                                                                                               \
                    if (rr_res == EGUI_MASK_ROW_INSIDE)                                                                                                        \
                    {                                                                                                                                          \
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];                                                             \
                        _blend_row_func(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);                                                      \
                    }                                                                                                                                          \
                    else                                                                                                                                       \
                    {                                                                                                                                          \
                        egui_dim_t visible_x_start = screen_x_start;                                                                                           \
                        egui_dim_t visible_x_end = x_base + x_total;                                                                                           \
                        if (canvas->mask->api->mask_get_row_visible_range != NULL &&                                                                           \
                            !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start,            \
                                                                           &visible_x_end))                                                                    \
                        {                                                                                                                                      \
                            continue;                                                                                                                          \
                        }                                                                                                                                      \
                                                                                                                                                               \
                        egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);                                                                               \
                        egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);                                                                           \
                        egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);                                                                         \
                        egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);                                                                     \
                                                                                                                                                               \
                        for (egui_dim_t x_ = vis_img_xs; x_ < EGUI_MIN(rr_img_xs, vis_img_xe); x_++)                                                           \
                        {                                                                                                                                      \
                            egui_dim_t src_x = src_x_map[x_ - x];                                                                                              \
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);                                                                              \
                            alpha = _get_alpha_func(src_alpha_row, src_x);                                                                                     \
                            egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);                                                                    \
                        }                                                                                                                                      \
                                                                                                                                                               \
                        if (rr_img_xs < rr_img_xe)                                                                                                             \
                        {                                                                                                                                      \
                            egui_dim_t mid_offset = rr_img_xs - x;                                                                                             \
                            egui_dim_t mid_count = rr_img_xe - rr_img_xs;                                                                                      \
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];          \
                                                                                                                                                               \
                            _blend_row_func(dst_row, src_row, src_alpha_row, &src_x_map[mid_offset], mid_count, canvas_alpha);                                 \
                        }                                                                                                                                      \
                                                                                                                                                               \
                        for (egui_dim_t x_ = EGUI_MAX(rr_img_xe, vis_img_xs); x_ < vis_img_xe; x_++)                                                           \
                        {                                                                                                                                      \
                            egui_dim_t src_x = src_x_map[x_ - x];                                                                                              \
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);                                                                              \
                            alpha = _get_alpha_func(src_alpha_row, src_x);                                                                                     \
                            egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);                                                                    \
                        }                                                                                                                                      \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
            else                                                                                                                                               \
            {                                                                                                                                                  \
                for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                    \
                {                                                                                                                                              \
                    src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);                                                                               \
                    src_row = (const uint16_t *)image->data_buf + (src_y * image->width);                                                                      \
                    src_alpha_row = (const uint8_t *)image->alpha_buf + (src_y * alpha_row_size);                                                              \
                                                                                                                                                               \
                    for (egui_dim_t i = 0; i < count; i++)                                                                                                     \
                    {                                                                                                                                          \
                        egui_dim_t src_x = src_x_map[i];                                                                                                       \
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x]);                                                                                  \
                        alpha = _get_alpha_func(src_alpha_row, src_x);                                                                                         \
                        egui_canvas_draw_point_limit(screen_x_start + i, y_base + y_, color, alpha);                                                           \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;                                                                        \
                egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];                                                                     \
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);                                                                                   \
                src_row = (const uint16_t *)image->data_buf + (src_y * image->width);                                                                          \
                src_alpha_row = (const uint8_t *)image->alpha_buf + (src_y * alpha_row_size);                                                                  \
                                                                                                                                                               \
                _blend_row_func(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
EGUI_IMAGE_STD_SET_IMAGE_RESIZE_RGB565_PACKED_ALPHA_COMMON(egui_image_std_set_image_resize_rgb565_4_common, ((image->width + 1) >> 1),
                                                           egui_image_std_get_alpha_rgb565_4_row, egui_image_std_blend_rgb565_alpha4_mapped_row,
                                                           egui_image_std_blend_rgb565_alpha4_repeat2_row, egui_image_std_blend_rgb565_alpha4_repeat4_row)
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
EGUI_IMAGE_STD_SET_IMAGE_RESIZE_RGB565_PACKED_ALPHA_COMMON(egui_image_std_set_image_resize_rgb565_2_common, ((image->width + 3) >> 2),
                                                           egui_image_std_get_alpha_rgb565_2_row, egui_image_std_blend_rgb565_alpha2_mapped_row,
                                                           egui_image_std_blend_rgb565_alpha2_repeat2_row, egui_image_std_blend_rgb565_alpha2_repeat4_row)
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
EGUI_IMAGE_STD_SET_IMAGE_RESIZE_RGB565_PACKED_ALPHA_COMMON(egui_image_std_set_image_resize_rgb565_1_common, ((image->width + 7) >> 3),
                                                           egui_image_std_get_alpha_rgb565_1_row, egui_image_std_blend_rgb565_alpha1_mapped_row,
                                                           egui_image_std_blend_rgb565_alpha1_repeat2_row, egui_image_std_blend_rgb565_alpha1_repeat4_row)
#endif

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_image_std_draw_image_resize_external_alpha(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                            egui_dim_t x_base, egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio,
                                                            uint32_t alpha_row_size, egui_image_std_get_col_pixel_with_alpha *get_col_pixel,
                                                            egui_image_std_blend_mapped_row_func *blend_row_func,
                                                            egui_image_std_blend_repeat2_row_func *blend_repeat2_row_func)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_color_t color;
    egui_alpha_t alpha;
    egui_dim_t src_y;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_alpha_t canvas_alpha = canvas->alpha;
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t screen_x_start = x_base + x;
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;
    egui_dim_t count = x_total - x;
    egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];
    uint32_t data_row_size = image->width << 1;
    egui_image_std_external_alpha_row_persistent_cache_t *row_cache = egui_image_std_get_external_alpha_row_persistent_cache();
    int cached_src_y = -1;

    if (!egui_image_std_prepare_external_alpha_row_persistent_cache_min_rows(row_cache, image, data_row_size, alpha_row_size,
                                                                             EGUI_MIN(EGUI_CONFIG_PFB_HEIGHT, image->height)))
    {
        return;
    }

    if (canvas->mask == NULL && blend_repeat2_row_func != NULL &&
        egui_image_std_can_use_resize_repeat2_fast_path(x, y, x_total, y_total, width_radio, height_radio))
    {
        egui_dim_t src_x_start = x >> 1;
        egui_dim_t src_count = (x_total - x) >> 1;

        for (egui_dim_t y_ = y; y_ < y_total; y_ += 2)
        {
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row0 = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            egui_color_int_t *dst_row1 = &canvas->pfb[(dst_y + 1) * pfb_width + dst_x_start];

            src_y = y_ >> 1;
            if (cached_src_y != src_y)
            {
                if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, src_y))
                {
                    continue;
                }
                cached_src_y = src_y;
            }

            const uint16_t *src_row = egui_image_std_get_external_alpha_row_persistent_data(row_cache, src_y);
            const uint8_t *src_alpha_row = egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, src_y);

            blend_repeat2_row_func(dst_row0, src_row, src_alpha_row, src_x_start, src_count, canvas_alpha);
            blend_repeat2_row_func(dst_row1, src_row, src_alpha_row, src_x_start, src_count, canvas_alpha);
        }

        return;
    }

    count = egui_image_std_prepare_resize_src_x_map_limit(src_x_map, x, x_total, width_radio);

    if (canvas->mask != NULL)
    {
        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t rr_x_start, rr_x_end;
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, src_y))
                    {
                        continue;
                    }
                    cached_src_y = src_y;
                }

                const uint16_t *src_row = egui_image_std_get_external_alpha_row_persistent_data(row_cache, src_y);
                const uint8_t *src_alpha_row = egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, src_y);
                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);
                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }

                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    /* Use batch row blender: single call instead of per-pixel function pointer */
                    blend_row_func(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);
                }
                else
                {
                    egui_dim_t visible_x_start = screen_x_start;
                    egui_dim_t visible_x_end = x_base + x_total;
                    if (canvas->mask->api->mask_get_row_visible_range != NULL &&
                        !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start, &visible_x_end))
                    {
                        continue;
                    }
                    egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);
                    egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);
                    egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                    egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);

                    for (egui_dim_t x_ = vis_img_xs; x_ < EGUI_MIN(rr_img_xs, vis_img_xe); x_++)
                    {
                        get_col_pixel(src_row, src_alpha_row, src_x_map[x_ - x], &color, &alpha);
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);
                    }

                    if (rr_img_xs < rr_img_xe)
                    {
                        egui_dim_t mid_offset = rr_img_xs - x;
                        egui_dim_t mid_count = rr_img_xe - rr_img_xs;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];
                        /* Use batch row blender for the unmasked middle section */
                        blend_row_func(dst_row, src_row, src_alpha_row, &src_x_map[mid_offset], mid_count, canvas_alpha);
                    }

                    for (egui_dim_t x_ = EGUI_MAX(rr_img_xe, vis_img_xs); x_ < vis_img_xe; x_++)
                    {
                        get_col_pixel(src_row, src_alpha_row, src_x_map[x_ - x], &color, &alpha);
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);
                    }
                }
            }
        }
        else
        {
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, src_y))
                    {
                        continue;
                    }
                    cached_src_y = src_y;
                }

                const uint16_t *src_row = egui_image_std_get_external_alpha_row_persistent_data(row_cache, src_y);
                const uint8_t *src_alpha_row = egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, src_y);

                for (egui_dim_t i = 0; i < count; i++)
                {
                    get_col_pixel(src_row, src_alpha_row, src_x_map[i], &color, &alpha);
                    egui_canvas_draw_point_limit(screen_x_start + i, y_base + y_, color, alpha);
                }
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = y; y_ < y_total; y_++)
        {
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
            if (cached_src_y != src_y)
            {
                if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, src_y))
                {
                    continue;
                }
                cached_src_y = src_y;
            }

            /* Use batch row blender: single call instead of per-pixel function pointer */
            blend_row_func(dst_row, egui_image_std_get_external_alpha_row_persistent_data(row_cache, src_y),
                           egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, src_y), src_x_map, count, canvas_alpha);
        }
    }
}

static void egui_image_std_set_image_resize_rgb565_external(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                            egui_dim_t x_base, egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_color_t color;
    egui_alpha_t canvas_alpha = canvas->alpha;
    egui_dim_t src_y;
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t screen_x_start = x_base + x;
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;
    egui_dim_t count = x_total - x;
    egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];
    uint32_t data_row_size = image->width << 1;
    egui_image_std_external_data_row_persistent_cache_t *row_cache = egui_image_std_get_external_data_row_persistent_cache();
    int cached_src_y = -1;

    if (!egui_image_std_prepare_external_data_row_persistent_cache_min_rows(row_cache, image, data_row_size, EGUI_MIN(EGUI_CONFIG_PFB_HEIGHT, image->height)))
    {
        return;
    }

    count = egui_image_std_prepare_resize_src_x_map_limit(src_x_map, x, x_total, width_radio);

    if (canvas->mask != NULL)
    {
        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t rr_x_start, rr_x_end;
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    if (!egui_image_std_load_external_data_row_persistent_cache(row_cache, src_y))
                    {
                        continue;
                    }
                    cached_src_y = src_y;
                }

                const uint16_t *src_row = egui_image_std_get_external_data_row_persistent_data(row_cache, src_y);
                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);

                if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                {
                    continue;
                }

                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    if (canvas_alpha == EGUI_ALPHA_100)
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                        }
                    }
                    else
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                            egui_image_std_blend_resize_pixel(&dst_row[i], color, canvas_alpha);
                        }
                    }
                }
                else
                {
                    egui_dim_t visible_x_start = screen_x_start;
                    egui_dim_t visible_x_end = x_base + x_total;
                    if (canvas->mask->api->mask_get_row_visible_range != NULL &&
                        !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start, &visible_x_end))
                    {
                        continue;
                    }
                    egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);
                    egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);
                    egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                    egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);

                    for (egui_dim_t x_ = vis_img_xs; x_ < EGUI_MIN(rr_img_xs, vis_img_xe); x_++)
                    {
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[x_ - x]]);
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, EGUI_ALPHA_100);
                    }

                    if (rr_img_xs < rr_img_xe)
                    {
                        egui_dim_t mid_offset = rr_img_xs - x;
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];
                        if (canvas_alpha == EGUI_ALPHA_100)
                        {
                            for (egui_dim_t i = mid_offset; i < (rr_img_xe - x); i++)
                            {
                                dst_row[i - mid_offset] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                            }
                        }
                        else
                        {
                            for (egui_dim_t i = mid_offset; i < (rr_img_xe - x); i++)
                            {
                                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                                egui_image_std_blend_resize_pixel(&dst_row[i - mid_offset], color, canvas_alpha);
                            }
                        }
                    }

                    for (egui_dim_t x_ = EGUI_MAX(rr_img_xe, vis_img_xs); x_ < vis_img_xe; x_++)
                    {
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[x_ - x]]);
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, EGUI_ALPHA_100);
                    }
                }
            }
        }
        else
        {
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                if (cached_src_y != src_y)
                {
                    if (!egui_image_std_load_external_data_row_persistent_cache(row_cache, src_y))
                    {
                        continue;
                    }
                    cached_src_y = src_y;
                }

                const uint16_t *src_row = egui_image_std_get_external_data_row_persistent_data(row_cache, src_y);
                for (egui_dim_t i = 0; i < count; i++)
                {
                    color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                    egui_canvas_draw_point_limit(screen_x_start + i, y_base + y_, color, EGUI_ALPHA_100);
                }
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = y; y_ < y_total; y_++)
        {
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
            if (cached_src_y != src_y)
            {
                if (!egui_image_std_load_external_data_row_persistent_cache(row_cache, src_y))
                {
                    continue;
                }
                cached_src_y = src_y;
            }

            const uint16_t *src_row = egui_image_std_get_external_data_row_persistent_data(row_cache, src_y);
            if (canvas_alpha == EGUI_ALPHA_100)
            {
                for (egui_dim_t i = 0; i < count; i++)
                {
                    dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                }
            }
            else
            {
                for (egui_dim_t i = 0; i < count; i++)
                {
                    color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                    egui_image_std_blend_resize_pixel(&dst_row[i], color, canvas_alpha);
                }
            }
        }
    }
}
#endif

#define EGUI_IMAGE_STD_DRAW_IMAGE_RESIZE_FUNC_DEFINE(_get_pixel_func, self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio)                 \
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;                                                                                         \
    egui_color_t color;                                                                                                                                        \
    egui_alpha_t alpha;                                                                                                                                        \
    egui_dim_t src_y;                                                                                                                                          \
    egui_canvas_t *canvas = egui_canvas_get_canvas();                                                                                                          \
    egui_alpha_t canvas_alpha = canvas->alpha;                                                                                                                 \
    egui_dim_t pfb_width = canvas->pfb_region.size.width;                                                                                                      \
    egui_dim_t screen_x_start = x_base + x;                                                                                                                    \
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;                                                                             \
    egui_dim_t count = x_total - x;                                                                                                                            \
    egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];                                                                                                               \
    count = egui_image_std_prepare_resize_src_x_map(src_x_map, x, x_total, width_radio);                                                                       \
    if (canvas->mask != NULL)                                                                                                                                  \
    {                                                                                                                                                          \
        if (canvas->mask->api->mask_get_row_range != NULL)                                                                                                     \
        {                                                                                                                                                      \
            egui_dim_t rr_x_start, rr_x_end;                                                                                                                   \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                egui_dim_t rr_sy = y_base + y_;                                                                                                                \
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;                                                                                \
                src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);                                                                                         \
                int rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);             \
                if (rr_res == EGUI_MASK_ROW_OUTSIDE)                                                                                                           \
                {                                                                                                                                              \
                    continue;                                                                                                                                  \
                }                                                                                                                                              \
                if (rr_res == EGUI_MASK_ROW_INSIDE)                                                                                                            \
                {                                                                                                                                              \
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];                                                                 \
                    for (egui_dim_t i = 0; i < count; i++)                                                                                                     \
                    {                                                                                                                                          \
                        _get_pixel_func(image, src_x_map[i], src_y, &color, &alpha);                                                                           \
                        alpha = egui_color_alpha_mix(canvas_alpha, alpha);                                                                                     \
                        egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);                                                                          \
                    }                                                                                                                                          \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    egui_dim_t visible_x_start = screen_x_start;                                                                                               \
                    egui_dim_t visible_x_end = x_base + x_total;                                                                                               \
                    if (canvas->mask->api->mask_get_row_visible_range != NULL &&                                                                               \
                        !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start,                \
                                                                       &visible_x_end))                                                                        \
                    {                                                                                                                                          \
                        continue;                                                                                                                              \
                    }                                                                                                                                          \
                    egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);                                                                                   \
                    egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);                                                                               \
                    egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);                                                                             \
                    egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);                                                                         \
                    for (egui_dim_t x_ = vis_img_xs; x_ < EGUI_MIN(rr_img_xs, vis_img_xe); x_++)                                                               \
                    {                                                                                                                                          \
                        _get_pixel_func(image, src_x_map[x_ - x], src_y, &color, &alpha);                                                                      \
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);                                                                        \
                    }                                                                                                                                          \
                    if (rr_img_xs < rr_img_xe)                                                                                                                 \
                    {                                                                                                                                          \
                        egui_dim_t mid_offset = rr_img_xs - x;                                                                                                 \
                        egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];              \
                        for (egui_dim_t i = mid_offset; i < (rr_img_xe - x); i++)                                                                              \
                        {                                                                                                                                      \
                            _get_pixel_func(image, src_x_map[i], src_y, &color, &alpha);                                                                       \
                            alpha = egui_color_alpha_mix(canvas_alpha, alpha);                                                                                 \
                            egui_image_std_blend_resize_pixel(&dst_row[i - mid_offset], color, alpha);                                                         \
                        }                                                                                                                                      \
                    }                                                                                                                                          \
                    for (egui_dim_t x_ = EGUI_MAX(rr_img_xe, vis_img_xs); x_ < vis_img_xe; x_++)                                                               \
                    {                                                                                                                                          \
                        _get_pixel_func(image, src_x_map[x_ - x], src_y, &color, &alpha);                                                                      \
                        egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, alpha);                                                                        \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        else if (canvas->mask->api->mask_get_row_overlay != NULL)                                                                                              \
        {                                                                                                                                                      \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                egui_dim_t ov_sy = y_base + y_;                                                                                                                \
                src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);                                                                                         \
                egui_color_t ov_color;                                                                                                                         \
                egui_alpha_t ov_alpha;                                                                                                                         \
                if (canvas->mask->api->mask_get_row_overlay(canvas->mask, ov_sy, &ov_color, &ov_alpha))                                                        \
                {                                                                                                                                              \
                    egui_dim_t ov_dst_y = ov_sy - canvas->pfb_location_in_base_view.y;                                                                         \
                    egui_color_int_t *ov_dst_row = &canvas->pfb[ov_dst_y * pfb_width + dst_x_start];                                                           \
                    for (egui_dim_t i = 0; i < count; i++)                                                                                                     \
                    {                                                                                                                                          \
                        _get_pixel_func(image, src_x_map[i], src_y, &color, &alpha);                                                                           \
                        if (ov_alpha > 0)                                                                                                                      \
                        {                                                                                                                                      \
                            egui_rgb_mix_ptr(&color, &ov_color, &color, ov_alpha);                                                                             \
                        }                                                                                                                                      \
                        alpha = egui_color_alpha_mix(canvas_alpha, alpha);                                                                                     \
                        egui_image_std_blend_resize_pixel(&ov_dst_row[i], color, alpha);                                                                       \
                    }                                                                                                                                          \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    for (egui_dim_t i = 0; i < count; i++)                                                                                                     \
                    {                                                                                                                                          \
                        _get_pixel_func(image, src_x_map[i], src_y, &color, &alpha);                                                                           \
                        egui_canvas_draw_point_limit(screen_x_start + i, ov_sy, color, alpha);                                                                 \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                        \
            {                                                                                                                                                  \
                src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);                                                                                         \
                for (egui_dim_t i = 0; i < count; i++)                                                                                                         \
                {                                                                                                                                              \
                    _get_pixel_func(image, src_x_map[i], src_y, &color, &alpha);                                                                               \
                    egui_canvas_draw_point_limit(screen_x_start + i, y_base + y_, color, alpha);                                                               \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }                                                                                                                                                          \
    else                                                                                                                                                       \
    {                                                                                                                                                          \
        for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                            \
        {                                                                                                                                                      \
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;                                                                            \
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];                                                                         \
            src_y = (egui_dim_t)EGUI_FLOAT_MULT(y_, height_radio);                                                                                             \
            for (egui_dim_t i = 0; i < count; i++)                                                                                                             \
            {                                                                                                                                                  \
                _get_pixel_func(image, src_x_map[i], src_y, &color, &alpha);                                                                                   \
                alpha = egui_color_alpha_mix(canvas_alpha, alpha);                                                                                             \
                egui_image_std_blend_resize_pixel(&dst_row[i], color, alpha);                                                                                  \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
void egui_image_std_set_image_resize_rgb565_8(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                              egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
    egui_image_std_set_image_resize_rgb565_8_common(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
void egui_image_std_set_image_resize_rgb565_4(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                              egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_info_t *image_info = (egui_image_std_info_t *)self->res;
    if (image_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_std_draw_image_resize_external_alpha(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio, (image_info->width + 1) >> 1,
                                                        egui_image_std_get_col_pixel_rgb565_4, egui_image_std_blend_rgb565_alpha4_mapped_row,
                                                        egui_image_std_blend_rgb565_alpha4_repeat2_row);
        return;
    }
#endif
    egui_image_std_set_image_resize_rgb565_4_common(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
void egui_image_std_set_image_resize_rgb565_2(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                              egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_info_t *image_info = (egui_image_std_info_t *)self->res;
    if (image_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_std_draw_image_resize_external_alpha(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio, (image_info->width + 3) >> 2,
                                                        egui_image_std_get_col_pixel_rgb565_2, egui_image_std_blend_rgb565_alpha2_mapped_row,
                                                        egui_image_std_blend_rgb565_alpha2_repeat2_row);
        return;
    }
#endif
    egui_image_std_set_image_resize_rgb565_2_common(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
void egui_image_std_set_image_resize_rgb565_1(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                              egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_info_t *image_info = (egui_image_std_info_t *)self->res;
    if (image_info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_std_draw_image_resize_external_alpha(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio, (image_info->width + 7) >> 3,
                                                        egui_image_std_get_col_pixel_rgb565_1, egui_image_std_blend_rgb565_alpha1_mapped_row,
                                                        egui_image_std_blend_rgb565_alpha1_repeat2_row);
        return;
    }
#endif
    egui_image_std_set_image_resize_rgb565_1_common(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
__EGUI_STATIC_INLINE__ int egui_image_std_set_image_resize_rgb565_8_round_rect_fast(const egui_image_std_info_t *image, const egui_dim_t *src_x_map,
                                                                                    egui_dim_t count, egui_dim_t x, egui_dim_t y, egui_dim_t x_total,
                                                                                    egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base,
                                                                                    egui_float_t height_radio, egui_alpha_t canvas_alpha, int use_repeat2_fast,
                                                                                    int use_repeat4_fast
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                                                                                    ,
                                                                                    egui_image_std_external_alpha_row_persistent_cache_t *row_cache
#endif
)
{
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_mask_round_rectangle_t *round_rect_mask;
    egui_image_std_round_rect_fast_cache_t round_rect_cache_storage;
    egui_image_std_round_rect_fast_cache_t *round_rect_cache = &round_rect_cache_storage;
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t screen_x_start = x_base + x;
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;
    egui_dim_t round_rect_x;
    egui_dim_t round_rect_y;
    egui_dim_t round_rect_x_end;
    egui_dim_t round_rect_y_end;
    egui_dim_t round_rect_radius;
    egui_dim_t src_y;
    int cached_src_y = -1;
    const uint16_t *src_row = NULL;
    const uint8_t *src_alpha_row = NULL;

    if (canvas->mask == NULL || canvas->mask->api->kind != EGUI_MASK_KIND_ROUND_RECTANGLE)
    {
        return 0;
    }

    round_rect_mask = (egui_mask_round_rectangle_t *)canvas->mask;
    egui_image_std_round_rect_fast_cache_init(round_rect_cache);
    if (!egui_image_std_round_rect_fast_cache_prepare(round_rect_cache, canvas->mask, round_rect_mask->radius))
    {
        return 0;
    }

    round_rect_radius = round_rect_mask->radius;
    round_rect_x = round_rect_cache->cached_x;
    round_rect_y = round_rect_cache->cached_y;
    round_rect_x_end = round_rect_x + round_rect_cache->cached_width;
    round_rect_y_end = round_rect_y + round_rect_cache->cached_height;
    if (round_rect_cache->info == NULL)
    {
        return 0;
    }

    {
        egui_dim_t screen_x_end = x_base + x_total;
        use_repeat4_fast = use_repeat4_fast && screen_x_start <= round_rect_x && screen_x_end >= round_rect_x_end;
        use_repeat2_fast = use_repeat2_fast && screen_x_start <= round_rect_x && screen_x_end >= round_rect_x_end;
    }

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        egui_dim_t rr_sy = y_base + y_;
        egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;

        if (rr_sy < round_rect_y || rr_sy >= round_rect_y_end)
        {
            continue;
        }

        src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
        if (cached_src_y != src_y)
        {
            uint32_t row_start = src_y * image->width;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
            if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
            {
                if (!egui_image_std_load_external_alpha_row_persistent_cache(row_cache, src_y))
                {
                    continue;
                }
                src_row = egui_image_std_get_external_alpha_row_persistent_data(row_cache, src_y);
                src_alpha_row = egui_image_std_get_external_alpha_row_persistent_alpha(row_cache, src_y);
            }
            else
#endif
            {
                src_row = (const uint16_t *)image->data_buf + row_start;
                src_alpha_row = (const uint8_t *)image->alpha_buf + row_start;
            }
            cached_src_y = src_y;
        }

        if (round_rect_radius <= 0 || (rr_sy >= round_rect_y + round_rect_radius && rr_sy < round_rect_y_end - round_rect_radius))
        {
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            if ((use_repeat2_fast || use_repeat4_fast) &&
                egui_image_std_blend_rgb565_alpha8_repeat_segment(dst_row, src_row, src_alpha_row, x, count, canvas_alpha, use_repeat2_fast, use_repeat4_fast))
            {
                continue;
            }
            egui_image_std_blend_rgb565_alpha8_mapped_row(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);
            continue;
        }

        {
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
            egui_mask_round_rectangle_blend_rgb565_alpha8_segment(canvas->mask, dst_row, src_row, src_alpha_row, src_x_map, count, screen_x_start, rr_sy,
                                                                  canvas_alpha);
#else
            egui_image_std_blend_rgb565_alpha8_mapped_row(dst_row, src_row, src_alpha_row, src_x_map, count, canvas_alpha);
#endif
        }
    }

    return 1;
}

#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
__EGUI_STATIC_INLINE__ int egui_image_std_set_image_resize_rgb565_round_rect_fast(const egui_image_std_info_t *image, const egui_dim_t *src_x_map,
                                                                                  egui_dim_t count, egui_dim_t x, egui_dim_t y, egui_dim_t x_total,
                                                                                  egui_dim_t y_total, egui_dim_t x_base, egui_dim_t y_base,
                                                                                  egui_float_t height_radio)
{
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_mask_round_rectangle_t *round_rect_mask;
    egui_image_std_round_rect_fast_cache_t round_rect_cache_storage;
    egui_image_std_round_rect_fast_cache_t *round_rect_cache = &round_rect_cache_storage;
    const egui_circle_info_t *round_rect_info;
    const egui_circle_item_t *round_rect_items;
    const uint16_t *src_pixels = image->data_buf;
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t screen_x_start = x_base + x;
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;
    egui_dim_t round_rect_x;
    egui_dim_t round_rect_y;
    egui_dim_t round_rect_x_end;
    egui_dim_t round_rect_y_end;
    egui_dim_t round_rect_radius;
    egui_dim_t src_y;

    if (canvas->mask == NULL || canvas->mask->api->kind != EGUI_MASK_KIND_ROUND_RECTANGLE || canvas->alpha != EGUI_ALPHA_100)
    {
        return 0;
    }

    round_rect_mask = (egui_mask_round_rectangle_t *)canvas->mask;
    egui_image_std_round_rect_fast_cache_init(round_rect_cache);
    if (!egui_image_std_round_rect_fast_cache_prepare(round_rect_cache, canvas->mask, round_rect_mask->radius))
    {
        return 0;
    }

    round_rect_radius = round_rect_mask->radius;
    round_rect_x = round_rect_cache->cached_x;
    round_rect_y = round_rect_cache->cached_y;
    round_rect_x_end = round_rect_x + round_rect_cache->cached_width;
    round_rect_y_end = round_rect_y + round_rect_cache->cached_height;
    round_rect_info = round_rect_cache->info;
    if (round_rect_info == NULL)
    {
        return 0;
    }

    round_rect_items = (const egui_circle_item_t *)round_rect_info->items;

    for (egui_dim_t y_ = y; y_ < y_total; y_++)
    {
        egui_dim_t rr_sy = y_base + y_;
        egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
        egui_dim_t rr_x_start;
        egui_dim_t rr_x_end;

        if (rr_sy < round_rect_y || rr_sy >= round_rect_y_end)
        {
            continue;
        }

        src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);

        if (round_rect_radius <= 0 || (rr_sy >= round_rect_y + round_rect_radius && rr_sy < round_rect_y_end - round_rect_radius))
        {
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            const uint16_t *src_row = &src_pixels[src_y * image->width];
            egui_image_std_blend_rgb565_mapped_row(dst_row, src_row, src_x_map, count, EGUI_ALPHA_100);
            continue;
        }

        {
            egui_dim_t row_index = (rr_sy < round_rect_y + round_rect_radius) ? (rr_sy - round_rect_y) : (round_rect_y_end - 1 - rr_sy);
            egui_dim_t visible_boundary;
            egui_dim_t opaque_boundary;
            egui_dim_t visible_x_start;
            egui_dim_t visible_x_end;
            egui_dim_t rr_img_xs;
            egui_dim_t rr_img_xe;
            egui_dim_t vis_img_xs;
            egui_dim_t vis_img_xe;
            const uint16_t *src_row = &src_pixels[src_y * image->width];

            visible_boundary = egui_image_std_get_circle_visible_boundary_fixed_row(row_index, round_rect_info, round_rect_items);
            opaque_boundary = egui_image_std_get_circle_opaque_boundary_fixed_row(row_index, round_rect_info, round_rect_items);

            visible_x_start = EGUI_MAX(round_rect_x + visible_boundary, screen_x_start);
            visible_x_end = EGUI_MIN(round_rect_x_end - visible_boundary, x_base + x_total);

            if (visible_x_start >= visible_x_end)
            {
                continue;
            }

            rr_x_start = EGUI_MAX(round_rect_x + opaque_boundary, screen_x_start);
            rr_x_end = EGUI_MIN(round_rect_x_end - opaque_boundary, x_base + x_total);
            rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);
            rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);
            vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
            vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);

            {
                egui_dim_t left_img_xe = EGUI_MIN(rr_img_xs, vis_img_xe);
                if (vis_img_xs < left_img_xe)
                {
                    egui_dim_t left_offset = vis_img_xs - x;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + vis_img_xs - canvas->pfb_location_in_base_view.x)];

                    egui_image_std_blend_rgb565_round_rect_masked_left_segment_fixed_row(dst_row, src_row, &src_x_map[left_offset], left_img_xe - vis_img_xs,
                                                                                         x_base + vis_img_xs, round_rect_x, row_index, round_rect_info,
                                                                                         round_rect_items);
                }
            }

            if (rr_img_xs < rr_img_xe)
            {
                egui_dim_t mid_offset = rr_img_xs - x;
                egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];
                egui_image_std_blend_rgb565_mapped_row(dst_row, src_row, &src_x_map[mid_offset], rr_img_xe - rr_img_xs, EGUI_ALPHA_100);
            }

            {
                egui_dim_t right_img_xs = EGUI_MAX(rr_img_xe, vis_img_xs);
                if (right_img_xs < vis_img_xe)
                {
                    egui_dim_t right_offset = right_img_xs - x;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + right_img_xs - canvas->pfb_location_in_base_view.x)];

                    egui_image_std_blend_rgb565_round_rect_masked_right_segment_fixed_row(dst_row, src_row, &src_x_map[right_offset], vis_img_xe - right_img_xs,
                                                                                          x_base + right_img_xs, round_rect_x_end, row_index, round_rect_info,
                                                                                          round_rect_items);
                }
            }
        }
    }

    return 1;
}

void egui_image_std_set_image_resize_rgb565(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                            egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_std_set_image_resize_rgb565_external(self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
        return;
    }
#endif
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_color_t color;
    egui_alpha_t canvas_alpha = canvas->alpha;
    egui_dim_t src_y;
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    egui_dim_t screen_x_start = x_base + x;
    egui_dim_t dst_x_start = screen_x_start - canvas->pfb_location_in_base_view.x;
    egui_dim_t count = x_total - x;
    egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];
    const uint16_t *src_pixels = image->data_buf;

    count = egui_image_std_prepare_resize_src_x_map_limit(src_x_map, x, x_total, width_radio);

    if (canvas->mask != NULL)
    {
        if (egui_image_std_set_image_resize_rgb565_round_rect_fast(image, src_x_map, count, x, y, x_total, y_total, x_base, y_base, height_radio))
        {
            return;
        }

        if (canvas->mask->api->mask_get_row_range != NULL)
        {
            egui_dim_t rr_x_start, rr_x_end;
            int use_circle_edge_fast_path = (canvas->mask->api->kind == EGUI_MASK_KIND_CIRCLE);
            egui_mask_circle_t *circle_mask_fast = NULL;

            if (use_circle_edge_fast_path)
            {
                circle_mask_fast = (egui_mask_circle_t *)canvas->mask;
            }

            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
                egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                egui_dim_t circle_row_index = 0;
                egui_dim_t circle_visible_half = 0;
                int circle_row_ready = 0;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                const uint16_t *src_row = &src_pixels[src_y * image->width];
                int rr_res;
                if (use_circle_edge_fast_path)
                {
                    egui_dim_t opaque_boundary;

                    if (!egui_image_std_prepare_circle_mask_row_fast(circle_mask_fast, rr_sy, &circle_row_index, &circle_visible_half, &opaque_boundary))
                    {
                        continue;
                    }

                    rr_x_start = EGUI_MAX(circle_mask_fast->center_x - circle_mask_fast->radius + opaque_boundary, screen_x_start);
                    rr_x_end = EGUI_MIN(circle_mask_fast->center_x + circle_mask_fast->radius - opaque_boundary + 1, x_base + x_total);
                    if (rr_x_start >= rr_x_end)
                    {
                        rr_x_start = screen_x_start;
                        rr_x_end = screen_x_start;
                        rr_res = EGUI_MASK_ROW_PARTIAL;
                    }
                    else if (rr_x_start <= screen_x_start && rr_x_end >= x_base + x_total)
                    {
                        rr_res = EGUI_MASK_ROW_INSIDE;
                    }
                    else
                    {
                        rr_res = EGUI_MASK_ROW_PARTIAL;
                    }

                    circle_row_ready = 1;
                }
                else
                {
                    rr_res = canvas->mask->api->mask_get_row_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &rr_x_start, &rr_x_end);

                    if (rr_res == EGUI_MASK_ROW_OUTSIDE)
                    {
                        continue;
                    }
                }

                if (rr_res == EGUI_MASK_ROW_INSIDE)
                {
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    if (canvas_alpha == EGUI_ALPHA_100)
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                        }
                    }
                    else
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                            egui_image_std_blend_resize_pixel(&dst_row[i], color, canvas_alpha);
                        }
                    }
                }
                else
                {
                    egui_dim_t visible_x_start = screen_x_start;
                    egui_dim_t visible_x_end = x_base + x_total;

                    if (circle_row_ready)
                    {
                        visible_x_start = EGUI_MAX(circle_mask_fast->center_x - circle_visible_half, screen_x_start);
                        visible_x_end = EGUI_MIN(circle_mask_fast->center_x + circle_visible_half + 1, x_base + x_total);
                        if (visible_x_start >= visible_x_end)
                        {
                            continue;
                        }
                    }
                    else if (canvas->mask->api->mask_get_row_visible_range != NULL &&
                             !canvas->mask->api->mask_get_row_visible_range(canvas->mask, rr_sy, screen_x_start, x_base + x_total, &visible_x_start,
                                                                            &visible_x_end))
                    {
                        continue;
                    }
                    egui_dim_t rr_img_xs = EGUI_MAX(x, rr_x_start - x_base);
                    egui_dim_t rr_img_xe = EGUI_MIN(x_total, rr_x_end - x_base);
                    egui_dim_t vis_img_xs = EGUI_MAX(x, visible_x_start - x_base);
                    egui_dim_t vis_img_xe = EGUI_MIN(x_total, visible_x_end - x_base);

                    if (circle_row_ready)
                    {
                        egui_dim_t left_img_xe = EGUI_MIN(rr_img_xs, vis_img_xe);
                        if (vis_img_xs < left_img_xe)
                        {
                            egui_dim_t left_offset = vis_img_xs - x;
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + vis_img_xs - canvas->pfb_location_in_base_view.x)];
                            const egui_circle_info_t *circle_info = circle_mask_fast->info;
                            const egui_circle_item_t *circle_items =
                                    (circle_info != NULL) ? (const egui_circle_item_t *)circle_info->items : NULL;

                            egui_image_std_blend_rgb565_circle_masked_mapped_segment_fixed_row(
                                    dst_row, src_row, &src_x_map[left_offset], left_img_xe - vis_img_xs, x_base + vis_img_xs, circle_mask_fast->center_x,
                                    circle_mask_fast->radius,
                                    circle_row_index, canvas_alpha, circle_info, circle_items);
                        }

                        if (rr_img_xs < rr_img_xe)
                        {
                            egui_dim_t mid_offset = rr_img_xs - x;
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];
                            egui_image_std_blend_rgb565_mapped_row(dst_row, src_row, &src_x_map[mid_offset], rr_img_xe - rr_img_xs, canvas_alpha);
                        }

                        egui_dim_t right_img_xs = EGUI_MAX(rr_img_xe, vis_img_xs);
                        if (right_img_xs < vis_img_xe)
                        {
                            egui_dim_t right_offset = right_img_xs - x;
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + right_img_xs - canvas->pfb_location_in_base_view.x)];
                            const egui_circle_info_t *circle_info = circle_mask_fast->info;
                            const egui_circle_item_t *circle_items =
                                    (circle_info != NULL) ? (const egui_circle_item_t *)circle_info->items : NULL;

                            egui_image_std_blend_rgb565_circle_masked_mapped_segment_fixed_row(
                                    dst_row, src_row, &src_x_map[right_offset], vis_img_xe - right_img_xs, x_base + right_img_xs, circle_mask_fast->center_x,
                                    circle_mask_fast->radius, circle_row_index, canvas_alpha, circle_info, circle_items);
                        }
                    }
                    else
                    {
                        egui_dim_t left_img_xe = EGUI_MIN(rr_img_xs, vis_img_xe);
                        if (vis_img_xs < left_img_xe)
                        {
                            for (egui_dim_t x_ = vis_img_xs; x_ < left_img_xe; x_++)
                            {
                                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[x_ - x]]);
                                egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, EGUI_ALPHA_100);
                            }
                        }

                        if (rr_img_xs < rr_img_xe)
                        {
                            egui_dim_t mid_offset = rr_img_xs - x;
                            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + (x_base + rr_img_xs - canvas->pfb_location_in_base_view.x)];
                            if (canvas_alpha == EGUI_ALPHA_100)
                            {
                                for (egui_dim_t i = mid_offset; i < (rr_img_xe - x); i++)
                                {
                                    dst_row[i - mid_offset] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                                }
                            }
                            else
                            {
                                for (egui_dim_t i = mid_offset; i < (rr_img_xe - x); i++)
                                {
                                    color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                                    egui_image_std_blend_resize_pixel(&dst_row[i - mid_offset], color, canvas_alpha);
                                }
                            }
                        }

                        egui_dim_t right_img_xs = EGUI_MAX(rr_img_xe, vis_img_xs);
                        if (right_img_xs < vis_img_xe)
                        {
                            for (egui_dim_t x_ = right_img_xs; x_ < vis_img_xe; x_++)
                            {
                                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[x_ - x]]);
                                egui_canvas_draw_point_limit(x_base + x_, rr_sy, color, EGUI_ALPHA_100);
                            }
                        }
                    }
                }
            }
        }
        else if (canvas->mask->api->mask_get_row_overlay != NULL)
        {
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                egui_dim_t rr_sy = y_base + y_;
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                const uint16_t *src_row = &src_pixels[src_y * image->width];
                egui_color_t ov_color;
                egui_alpha_t ov_alpha;
                if (canvas->mask->api->mask_get_row_overlay(canvas->mask, rr_sy, &ov_color, &ov_alpha))
                {
                    egui_dim_t dst_y = rr_sy - canvas->pfb_location_in_base_view.y;
                    egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
                    if (ov_alpha == 0)
                    {
                        if (canvas_alpha == EGUI_ALPHA_100)
                        {
                            for (egui_dim_t i = 0; i < count; i++)
                            {
                                dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                            }
                        }
                        else
                        {
                            for (egui_dim_t i = 0; i < count; i++)
                            {
                                color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                                egui_image_std_blend_resize_pixel(&dst_row[i], color, canvas_alpha);
                            }
                        }
                    }
                    else
                    {
                        // Pre-compute packed blend factors for overlay
                        uint16_t fg = ov_color.full;
                        uint32_t fg_rb_g = (fg | ((uint32_t)fg << 16)) & 0x07E0F81FUL;
                        uint32_t alpha5 = (uint32_t)ov_alpha >> 3;

                        if (canvas_alpha == EGUI_ALPHA_100)
                        {
                            for (egui_dim_t i = 0; i < count; i++)
                            {
                                uint16_t bg = src_row[src_x_map[i]];
                                uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * alpha5 >> 5)) & 0x07E0F81FUL;
                                dst_row[i] = (uint16_t)(result | (result >> 16));
                            }
                        }
                        else
                        {
                            for (egui_dim_t i = 0; i < count; i++)
                            {
                                uint16_t bg = src_row[src_x_map[i]];
                                uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
                                uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * alpha5 >> 5)) & 0x07E0F81FUL;
                                color.full = (uint16_t)(result | (result >> 16));
                                egui_image_std_blend_resize_pixel(&dst_row[i], color, canvas_alpha);
                            }
                        }
                    }
                }
                else
                {
                    for (egui_dim_t i = 0; i < count; i++)
                    {
                        color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                        egui_canvas_draw_point_limit(screen_x_start + i, rr_sy, color, EGUI_ALPHA_100);
                    }
                }
            }
        }
        else
        {
            for (egui_dim_t y_ = y; y_ < y_total; y_++)
            {
                src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
                const uint16_t *src_row = &src_pixels[src_y * image->width];
                for (egui_dim_t i = 0; i < count; i++)
                {
                    color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                    egui_canvas_draw_point_limit(screen_x_start + i, y_base + y_, color, EGUI_ALPHA_100);
                }
            }
        }
    }
    else
    {
        for (egui_dim_t y_ = y; y_ < y_total; y_++)
        {
            egui_dim_t dst_y = (y_base + y_) - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[dst_y * pfb_width + dst_x_start];
            src_y = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);
            const uint16_t *src_row = &src_pixels[src_y * image->width];

            if (canvas_alpha == EGUI_ALPHA_100)
            {
                for (egui_dim_t i = 0; i < count; i++)
                {
                    dst_row[i] = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                }
            }
            else
            {
                for (egui_dim_t i = 0; i < count; i++)
                {
                    color.full = EGUI_COLOR_RGB565_TRANS(src_row[src_x_map[i]]);
                    egui_image_std_blend_resize_pixel(&dst_row[i], color, canvas_alpha);
                }
            }
        }
    }
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
void egui_image_std_set_image_resize_rgb32(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total, egui_dim_t x_base,
                                           egui_dim_t y_base, egui_float_t width_radio, egui_float_t height_radio)
{
    EGUI_IMAGE_STD_DRAW_IMAGE_RESIZE_FUNC_DEFINE(egui_image_std_get_pixel_rgb32, self, x, y, x_total, y_total, x_base, y_base, width_radio, height_radio);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32

void egui_image_std_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    const egui_image_t *draw_self = self;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_t cached_self;
    if (image->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        draw_self = egui_image_std_resolve_external_persistent_image(self, &cached_self);
    }
#endif
    if (width == 0 || height == 0)
    {
        return;
    }
    if (width == image->width && height == image->height)
    {
        egui_image_std_draw_image(self, x, y);
        return;
    }
    // egui_color_t color;
    // egui_alpha_t alpha;
    egui_float_t width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(width));
    egui_float_t height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(height));
    // const uint32_t *p_data = image->data_buf;
    // const uint8_t* p_alpha = image->alpha_buf;
    // egui_dim_t src_x;
    // egui_dim_t src_y;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);

    if (egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
    if (image->alpha_buf != NULL && egui_image_std_rgb565_can_use_opaque_resize_fast_path(image, egui_canvas_get_canvas()))
    {
        egui_image_std_set_image_resize_rgb565(draw_self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
        return;
    }
#endif

    if (image)
    {
        switch (image->data_type)
        {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB32:
            egui_image_std_set_image_resize_rgb32(draw_self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB32
        case EGUI_IMAGE_DATA_TYPE_RGB565:
            if (image->alpha_buf == NULL)
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
                egui_image_std_set_image_resize_rgb565(draw_self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
            }
            else
            {
                switch (image->alpha_type)
                {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
                case EGUI_IMAGE_ALPHA_TYPE_1:
                    egui_image_std_set_image_resize_rgb565_1(draw_self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
                case EGUI_IMAGE_ALPHA_TYPE_2:
                    egui_image_std_set_image_resize_rgb565_2(draw_self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
                case EGUI_IMAGE_ALPHA_TYPE_4:
                    egui_image_std_set_image_resize_rgb565_4(draw_self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                case EGUI_IMAGE_ALPHA_TYPE_8:
                    egui_image_std_set_image_resize_rgb565_8(draw_self, region.location.x, region.location.y, x_total, y_total, x, y, width_radio, height_radio);
                    break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
                default:
                    EGUI_ASSERT(0);
                    break;
                }
            }
            break;
        default:
            EGUI_ASSERT(0);
            break;
        }
    }
}

#endif // EGUI_CONFIG_REDUCE_IMAGE_CODE_SIZE

// Alpha-only image color draw functions
// These functions draw alpha-only images (EGUI_IMAGE_DATA_TYPE_ALPHA) with a specified color,
// following the same pattern as font rendering (egui_font_std_draw_4).

#define EGUI_IMAGE_STD_DRAW_ALPHA_COLOR_FUNC(_get_col_alpha_func, _alpha_row_size_expr, self, x, y, x_total, y_total, x_base, y_base, color, color_alpha)      \
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;                                                                                         \
    egui_alpha_t pixel_alpha;                                                                                                                                  \
    egui_canvas_t *canvas = egui_canvas_get_canvas();                                                                                                          \
    uint16_t alpha_row_size = _alpha_row_size_expr;                                                                                                            \
    EGUI_UNUSED(alpha_row_size);                                                                                                                               \
    if (canvas->mask != NULL)                                                                                                                                  \
    {                                                                                                                                                          \
        for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                            \
        {                                                                                                                                                      \
            const uint8_t *p_data = (const uint8_t *)image->data_buf + (uint32_t)y_ * alpha_row_size;                                                          \
            for (egui_dim_t x_ = x; x_ < x_total; x_++)                                                                                                        \
            {                                                                                                                                                  \
                _get_col_alpha_func(p_data, x_, &pixel_alpha);                                                                                                 \
                if (pixel_alpha)                                                                                                                               \
                {                                                                                                                                              \
                    egui_alpha_t fa_ = (color_alpha == EGUI_ALPHA_100) ? pixel_alpha : (egui_alpha_t)(pixel_alpha * color_alpha / EGUI_ALPHA_100);             \
                    egui_canvas_draw_point_limit((x_base + x_), (y_base + y_), color, fa_);                                                                    \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }                                                                                                                                                          \
    else                                                                                                                                                       \
    {                                                                                                                                                          \
        egui_dim_t pfb_w_ = canvas->pfb_region.size.width;                                                                                                     \
        egui_dim_t pfb_x0_ = (x_base + x) - canvas->pfb_location_in_base_view.x;                                                                               \
        egui_dim_t pfb_yoff_ = y_base - canvas->pfb_location_in_base_view.y;                                                                                   \
        egui_alpha_t comb_a_ = egui_color_alpha_mix(canvas->alpha, color_alpha);                                                                               \
        int comb_is_100_ = (comb_a_ == EGUI_ALPHA_100);                                                                                                        \
        egui_dim_t col_count_ = x_total - x;                                                                                                                   \
        for (egui_dim_t y_ = y; y_ < y_total; y_++)                                                                                                            \
        {                                                                                                                                                      \
            const uint8_t *p_data = (const uint8_t *)image->data_buf + (uint32_t)y_ * alpha_row_size;                                                          \
            egui_color_int_t *dst_row_ = &canvas->pfb[(pfb_yoff_ + y_) * pfb_w_ + pfb_x0_];                                                                    \
            for (egui_dim_t i_ = 0; i_ < col_count_; i_++)                                                                                                     \
            {                                                                                                                                                  \
                _get_col_alpha_func(p_data, x + i_, &pixel_alpha);                                                                                             \
                if (pixel_alpha)                                                                                                                               \
                {                                                                                                                                              \
                    egui_alpha_t fa_ = comb_is_100_ ? pixel_alpha : (egui_alpha_t)(((uint16_t)comb_a_ * pixel_alpha + 128) >> 8);                              \
                    egui_image_std_blend_resize_pixel(&dst_row_[i_], color, fa_);                                                                              \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    }

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
static void egui_image_std_set_image_alpha_color_8(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                   egui_dim_t x_base, egui_dim_t y_base, egui_color_t color, egui_alpha_t color_alpha)
{
    EGUI_IMAGE_STD_DRAW_ALPHA_COLOR_FUNC(egui_image_std_get_col_alpha_8, image->width, self, x, y, x_total, y_total, x_base, y_base, color, color_alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
static void egui_image_std_set_image_alpha_color_4(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                   egui_dim_t x_base, egui_dim_t y_base, egui_color_t color, egui_alpha_t color_alpha)
{
    EGUI_IMAGE_STD_DRAW_ALPHA_COLOR_FUNC(egui_image_std_get_col_alpha_4, ((image->width + 1) >> 1), self, x, y, x_total, y_total, x_base, y_base, color,
                                         color_alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
static void egui_image_std_set_image_alpha_color_2(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                   egui_dim_t x_base, egui_dim_t y_base, egui_color_t color, egui_alpha_t color_alpha)
{
    EGUI_IMAGE_STD_DRAW_ALPHA_COLOR_FUNC(egui_image_std_get_col_alpha_2, ((image->width + 3) >> 2), self, x, y, x_total, y_total, x_base, y_base, color,
                                         color_alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2

#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
static void egui_image_std_set_image_alpha_color_1(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t x_total, egui_dim_t y_total,
                                                   egui_dim_t x_base, egui_dim_t y_base, egui_color_t color, egui_alpha_t color_alpha)
{
    EGUI_IMAGE_STD_DRAW_ALPHA_COLOR_FUNC(egui_image_std_get_col_alpha_1, ((image->width + 7) >> 3), self, x, y, x_total, y_total, x_base, y_base, color,
                                         color_alpha);
}
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1

void egui_image_std_draw_image_color(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t color_alpha)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    egui_dim_t width = image->width;
    egui_dim_t height = image->height;

    egui_dim_t x_total;
    egui_dim_t y_total;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);

    if (egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;
    EGUI_UNUSED(x_total);
    EGUI_UNUSED(y_total);

    if (image)
    {
        switch (image->alpha_type)
        {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
        case EGUI_IMAGE_ALPHA_TYPE_1:
            egui_image_std_set_image_alpha_color_1(self, region.location.x, region.location.y, x_total, y_total, x, y, color, color_alpha);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
        case EGUI_IMAGE_ALPHA_TYPE_2:
            egui_image_std_set_image_alpha_color_2(self, region.location.x, region.location.y, x_total, y_total, x, y, color, color_alpha);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
        case EGUI_IMAGE_ALPHA_TYPE_4:
            egui_image_std_set_image_alpha_color_4(self, region.location.x, region.location.y, x_total, y_total, x, y, color, color_alpha);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
        case EGUI_IMAGE_ALPHA_TYPE_8:
            egui_image_std_set_image_alpha_color_8(self, region.location.x, region.location.y, x_total, y_total, x, y, color, color_alpha);
            break;
#endif // EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
        default:
            EGUI_ASSERT(0);
            break;
        }
    }
}

/**
 * Inner loop macro for resize-color no-mask path.
 * Hoists src_y to outer loop, caches p_data_row per source row,
 * uses pre-computed src_x_map, and writes directly to PFB.
 */
#define EGUI_IMAGE_RESIZE_COLOR_FAST_LOOP(_get_alpha_func, _row_size)                                                                                          \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        int cached_src_y_ = -1;                                                                                                                                \
        const uint8_t *p_row_ = NULL;                                                                                                                          \
        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)                                                                                            \
        {                                                                                                                                                      \
            egui_dim_t sy_ = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);                                                                              \
            if (cached_src_y_ != sy_)                                                                                                                          \
            {                                                                                                                                                  \
                p_row_ = (const uint8_t *)image->data_buf + (uint32_t)sy_ * (_row_size);                                                                       \
                cached_src_y_ = sy_;                                                                                                                           \
            }                                                                                                                                                  \
            egui_color_int_t *dst_row_ = &canvas->pfb[(pfb_y_off + y_) * pfb_width + pfb_x_start];                                                             \
            for (egui_dim_t i_ = 0; i_ < count; i_++)                                                                                                          \
            {                                                                                                                                                  \
                egui_alpha_t pa_;                                                                                                                              \
                _get_alpha_func(p_row_, src_x_map[i_], &pa_);                                                                                                  \
                if (pa_)                                                                                                                                       \
                {                                                                                                                                              \
                    egui_alpha_t fa_ = combined_is_100 ? pa_ : (egui_alpha_t)(((uint16_t)combined_alpha * pa_ + 128) >> 8);                                    \
                    egui_image_std_blend_resize_pixel(&dst_row_[i_], color, fa_);                                                                              \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    } while (0)

/**
 * Inner loop macro for resize-color mask path.
 * Uses src_x_map and hoisted src_y, but still calls draw_point_limit for mask support.
 */
#define EGUI_IMAGE_RESIZE_COLOR_MASK_LOOP(_get_alpha_func, _row_size)                                                                                          \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        int cached_src_y_ = -1;                                                                                                                                \
        const uint8_t *p_row_ = NULL;                                                                                                                          \
        for (egui_dim_t y_ = region.location.y; y_ < y_total; y_++)                                                                                            \
        {                                                                                                                                                      \
            egui_dim_t sy_ = (egui_dim_t)EGUI_FLOAT_MULT_LIMIT(y_, height_radio);                                                                              \
            if (cached_src_y_ != sy_)                                                                                                                          \
            {                                                                                                                                                  \
                p_row_ = (const uint8_t *)image->data_buf + (uint32_t)sy_ * (_row_size);                                                                       \
                cached_src_y_ = sy_;                                                                                                                           \
            }                                                                                                                                                  \
            egui_dim_t screen_y_ = y + y_;                                                                                                                     \
            for (egui_dim_t i_ = 0; i_ < count; i_++)                                                                                                          \
            {                                                                                                                                                  \
                egui_alpha_t pa_;                                                                                                                              \
                _get_alpha_func(p_row_, src_x_map[i_], &pa_);                                                                                                  \
                if (pa_)                                                                                                                                       \
                {                                                                                                                                              \
                    egui_alpha_t fa_ = (color_alpha == EGUI_ALPHA_100) ? pa_ : (egui_alpha_t)(pa_ * color_alpha / EGUI_ALPHA_100);                             \
                    egui_canvas_draw_point_limit(screen_x_start + i_, screen_y_, color, fa_);                                                                  \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    } while (0)

void egui_image_std_draw_image_resize_color(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                            egui_alpha_t color_alpha)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    if (width == 0 || height == 0)
    {
        return;
    }
    egui_float_t width_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->width), EGUI_FLOAT_VALUE_INT(width));
    egui_float_t height_radio = EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(image->height), EGUI_FLOAT_VALUE_INT(height));
    EGUI_UNUSED(height_radio);

    egui_dim_t x_total;
    egui_dim_t y_total;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, egui_canvas_get_base_view_work_region(), &region);

    if (egui_region_is_empty(&region))
    {
        return;
    }

    // change to image coordinate.
    region.location.x -= x;
    region.location.y -= y;

    // for speed, calculate total positions outside of the loop
    x_total = region.location.x + region.size.width;
    y_total = region.location.y + region.size.height;
    EGUI_UNUSED(y_total);

    if (image)
    {
        egui_canvas_t *canvas = egui_canvas_get_canvas();

        // Pre-compute src_x mapping to eliminate per-pixel FLOAT_MULT
        egui_dim_t src_x_map[EGUI_CONFIG_PFB_WIDTH];
        egui_dim_t count = egui_image_std_prepare_resize_src_x_map_limit(src_x_map, region.location.x, x_total, width_radio);
        EGUI_UNUSED(count);

        if (canvas->mask != NULL)
        {
            // Mask path: use per-pixel draw_point_limit for mask support
            // Still benefits from src_x_map and hoisted src_y
            egui_dim_t screen_x_start = x + region.location.x;
            EGUI_UNUSED(screen_x_start);

            switch (image->alpha_type)
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
            case EGUI_IMAGE_ALPHA_TYPE_8:
                EGUI_IMAGE_RESIZE_COLOR_MASK_LOOP(egui_image_std_get_col_alpha_8, image->width);
                break;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
            case EGUI_IMAGE_ALPHA_TYPE_4:
                EGUI_IMAGE_RESIZE_COLOR_MASK_LOOP(egui_image_std_get_col_alpha_4, ((image->width + 1) >> 1));
                break;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
            case EGUI_IMAGE_ALPHA_TYPE_2:
                EGUI_IMAGE_RESIZE_COLOR_MASK_LOOP(egui_image_std_get_col_alpha_2, ((image->width + 3) >> 2));
                break;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
            case EGUI_IMAGE_ALPHA_TYPE_1:
                EGUI_IMAGE_RESIZE_COLOR_MASK_LOOP(egui_image_std_get_col_alpha_1, ((image->width + 7) >> 3));
                break;
#endif
            default:
                EGUI_ASSERT(0);
                break;
            }
        }
        else
        {
            // No-mask fast path: direct PFB writes
            egui_dim_t pfb_width = canvas->pfb_region.size.width;
            egui_dim_t pfb_x_start = (x + region.location.x) - canvas->pfb_location_in_base_view.x;
            egui_dim_t pfb_y_off = y - canvas->pfb_location_in_base_view.y;
            EGUI_UNUSED(pfb_width);
            EGUI_UNUSED(pfb_x_start);
            EGUI_UNUSED(pfb_y_off);

            // Pre-combine canvas alpha and color alpha
            egui_alpha_t combined_alpha = egui_color_alpha_mix(canvas->alpha, color_alpha);
            int combined_is_100 = (combined_alpha == EGUI_ALPHA_100);
            EGUI_UNUSED(combined_is_100);

            switch (image->alpha_type)
            {
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8
            case EGUI_IMAGE_ALPHA_TYPE_8:
                EGUI_IMAGE_RESIZE_COLOR_FAST_LOOP(egui_image_std_get_col_alpha_8, image->width);
                break;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_4
            case EGUI_IMAGE_ALPHA_TYPE_4:
                EGUI_IMAGE_RESIZE_COLOR_FAST_LOOP(egui_image_std_get_col_alpha_4, ((image->width + 1) >> 1));
                break;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_2
            case EGUI_IMAGE_ALPHA_TYPE_2:
                EGUI_IMAGE_RESIZE_COLOR_FAST_LOOP(egui_image_std_get_col_alpha_2, ((image->width + 3) >> 2));
                break;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_1
            case EGUI_IMAGE_ALPHA_TYPE_1:
                EGUI_IMAGE_RESIZE_COLOR_FAST_LOOP(egui_image_std_get_col_alpha_1, ((image->width + 7) >> 3));
                break;
#endif
            default:
                EGUI_ASSERT(0);
                break;
            }
        }
    }
}

void egui_image_std_get_width_height(const egui_image_t *self, egui_dim_t *width, egui_dim_t *height)
{
    egui_image_std_info_t *image = (egui_image_std_info_t *)self->res;
    *width = image->width;
    *height = image->height;
}

const egui_image_api_t egui_image_std_t_api_table = {.get_point = egui_image_std_get_point,
                                                     .get_point_resize = egui_image_std_get_point_resize,
                                                     .draw_image = egui_image_std_draw_image,
                                                     .draw_image_resize = egui_image_std_draw_image_resize};

void egui_image_std_init(egui_image_t *self, const void *res)
{
    EGUI_LOCAL_INIT(egui_image_std_t);
    // call super init.
    egui_image_init(self, res);

    // update api.
    self->api = &egui_image_std_t_api_table;
}

void egui_image_std_release_frame_cache(void)
{
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    memset(&g_egui_image_std_external_row_persistent_cache_storage, 0, sizeof(g_egui_image_std_external_row_persistent_cache_storage));
#endif
}
