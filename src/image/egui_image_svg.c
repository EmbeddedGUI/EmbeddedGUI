#include "egui_image_svg.h"
#include "egui_image_svg_alloc.h"
#include "egui_image_std.h"
#include "core/egui_api.h"

#include <limits.h>
#include <string.h>

typedef struct egui_svg_alloc_header
{
    size_t size;
} egui_svg_alloc_header_t;

typedef struct
{
    uint16_t visible_start;
    uint16_t visible_end;
    uint16_t opaque_start;
    uint16_t opaque_end;
    uint8_t all_opaque;
} egui_svg_raster_row_meta_t;

#ifndef EGUI_SVG_ROW_META_MIN_OPAQUE_SPAN
#define EGUI_SVG_ROW_META_MIN_OPAQUE_SPAN 8
#endif

#ifndef EGUI_SVG_ROW_META_MAX_BYTES
#define EGUI_SVG_ROW_META_MAX_BYTES 1024
#endif

static egui_svg_alloc_header_t *egui_svg_alloc_header_from_ptr(void *ptr)
{
    if (ptr == NULL)
    {
        return NULL;
    }
    return ((egui_svg_alloc_header_t *)ptr) - 1;
}

void *egui_svg_alloc_malloc(size_t size)
{
    egui_svg_alloc_header_t *header;
    size_t total;

    if (size == 0 || size > (size_t)INT_MAX)
    {
        return NULL;
    }

    total = sizeof(*header) + size;
    if (total < size || total > (size_t)INT_MAX)
    {
        return NULL;
    }

    header = (egui_svg_alloc_header_t *)egui_malloc(NULL, (int)total);
    if (header == NULL)
    {
        return NULL;
    }

    header->size = size;
    return (void *)(header + 1);
}

void *egui_svg_alloc_calloc(size_t count, size_t size)
{
    size_t total;
    void *ptr;

    if (count == 0 || size == 0)
    {
        return NULL;
    }
    if (count > (SIZE_MAX / size))
    {
        return NULL;
    }

    total = count * size;
    ptr = egui_svg_alloc_malloc(total);
    if (ptr != NULL)
    {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *egui_svg_alloc_realloc(void *ptr, size_t size)
{
    egui_svg_alloc_header_t *header;
    void *new_ptr;
    size_t copy_size;

    if (ptr == NULL)
    {
        return egui_svg_alloc_malloc(size);
    }
    if (size == 0)
    {
        egui_svg_alloc_free(ptr);
        return NULL;
    }

    header = egui_svg_alloc_header_from_ptr(ptr);
    new_ptr = egui_svg_alloc_malloc(size);
    if (new_ptr == NULL)
    {
        return NULL;
    }

    copy_size = header->size;
    if (copy_size > size)
    {
        copy_size = size;
    }
    memcpy(new_ptr, ptr, copy_size);
    egui_svg_alloc_free(ptr);
    return new_ptr;
}

void egui_svg_alloc_free(void *ptr)
{
    egui_svg_alloc_header_t *header = egui_svg_alloc_header_from_ptr(ptr);

    if (header != NULL)
    {
        egui_free(NULL, header);
    }
}

#if EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG

#if !EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565 || !EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
#error "PlutoSVG backend requires EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565=1 and EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8=1."
#endif

#include "plutosvg.h"

typedef struct egui_svg_doc
{
    plutosvg_document_t *document;
    egui_image_std_info_t raster_info;
    egui_image_std_t raster_image;
    uint16_t *data_buf;
    uint8_t *alpha_buf;
    egui_svg_raster_row_meta_t *row_meta;
    uint8_t row_meta_fast_path_enabled;
    egui_dim_t natural_width;
    egui_dim_t natural_height;
    egui_dim_t cache_width;
    egui_dim_t cache_height;
    egui_dim_t cache_offset_x;
    egui_dim_t cache_offset_y;
} egui_svg_doc_t;

extern const egui_image_api_t egui_image_svg_t_api_table;

static int egui_svg_dim_from_float(float value, egui_dim_t *out_value)
{
    int32_t rounded;

    if (out_value == NULL)
    {
        return 0;
    }
    if (!(value > 0.0f) || value > (float)EGUI_DIM_MAX)
    {
        return 0;
    }

    rounded = (int32_t)value;
    if ((float)rounded < value)
    {
        rounded++;
    }
    if (rounded <= 0 || rounded > EGUI_DIM_MAX)
    {
        return 0;
    }

    *out_value = (egui_dim_t)rounded;
    return 1;
}

static __attribute__((unused)) int32_t egui_svg_dim_for_log(float value)
{
    int32_t rounded;

    if (!(value > 0.0f))
    {
        return 0;
    }
    if (value >= (float)INT_MAX)
    {
        return INT_MAX;
    }

    rounded = (int32_t)value;
    if ((float)rounded < value)
    {
        rounded++;
    }
    return rounded;
}

__EGUI_STATIC_INLINE__ int egui_svg_cache_matches(const egui_svg_doc_t *doc, egui_dim_t width, egui_dim_t height)
{
    return doc != NULL && doc->raster_info.data_buf != NULL && doc->cache_width == width && doc->cache_height == height;
}

static void egui_svg_release_raster(egui_svg_doc_t *doc)
{
    if (doc == NULL)
    {
        return;
    }

    if (doc->data_buf != NULL)
    {
        egui_free(NULL, doc->data_buf);
        doc->data_buf = NULL;
    }
    if (doc->alpha_buf != NULL)
    {
        egui_free(NULL, doc->alpha_buf);
        doc->alpha_buf = NULL;
    }
    if (doc->row_meta != NULL)
    {
        egui_free(NULL, doc->row_meta);
        doc->row_meta = NULL;
    }

    doc->raster_info.data_buf = NULL;
    doc->raster_info.alpha_buf = NULL;
    doc->raster_info.width = 0;
    doc->raster_info.height = 0;
    doc->raster_image.base.res = NULL;
    doc->cache_width = 0;
    doc->cache_height = 0;
    doc->cache_offset_x = 0;
    doc->cache_offset_y = 0;
    doc->row_meta_fast_path_enabled = 0;
}

static void egui_svg_doc_destroy(egui_svg_doc_t *doc)
{
    if (doc == NULL)
    {
        return;
    }

    egui_svg_release_raster(doc);
    if (doc->document != NULL)
    {
        plutosvg_document_destroy(doc->document);
        doc->document = NULL;
    }
    egui_free(NULL, doc);
}

static int egui_svg_build_raster_from_argb32(int width, int height, int stride, const unsigned char *surface_data, uint16_t **out_data_buf,
                                             uint8_t **out_alpha_buf, egui_svg_raster_row_meta_t **out_row_meta, egui_image_std_info_t *out_info,
                                             uint8_t *out_row_meta_fast_path_enabled, egui_dim_t *out_offset_x, egui_dim_t *out_offset_y)
{
    size_t pixel_count;
    size_t nonzero_pixel_count = 0u;
    size_t data_bytes;
    size_t alpha_bytes;
    size_t row_meta_bytes;
    uint16_t *data_buf;
    uint8_t *alpha_buf;
    egui_svg_raster_row_meta_t *row_meta;
    int enable_row_meta;
    uint8_t has_partial_alpha = 0u;
    uint8_t crop_is_opaque = 0u;
    uint8_t row_meta_fast_path_enabled = 0u;
    int crop_x_min = width;
    int crop_y_min = height;
    int crop_x_max = -1;
    int crop_y_max = -1;
    int crop_width;
    int crop_height;
    int y;

    if (out_data_buf == NULL || out_alpha_buf == NULL || out_row_meta == NULL || out_info == NULL || out_row_meta_fast_path_enabled == NULL ||
        out_offset_x == NULL || out_offset_y == NULL || surface_data == NULL || width <= 0 || height <= 0 || stride < width * 4)
    {
        return 0;
    }

    for (y = 0; y < height; y++)
    {
        const uint32_t *src_row = (const uint32_t *)(surface_data + (size_t)y * (size_t)stride);
        int x;

        for (x = 0; x < width; x++)
        {
            uint8_t alpha = (uint8_t)(src_row[x] >> 24);

            if (alpha == 0u)
            {
                continue;
            }

            nonzero_pixel_count++;
            if (alpha != EGUI_ALPHA_100)
            {
                has_partial_alpha = 1u;
            }

            if (x < crop_x_min)
            {
                crop_x_min = x;
            }
            if (x > crop_x_max)
            {
                crop_x_max = x;
            }
            if (y < crop_y_min)
            {
                crop_y_min = y;
            }
            if (y > crop_y_max)
            {
                crop_y_max = y;
            }
        }
    }

    if (crop_x_max < crop_x_min || crop_y_max < crop_y_min)
    {
        crop_x_min = 0;
        crop_y_min = 0;
        crop_width = 1;
        crop_height = 1;
    }
    else
    {
        crop_width = crop_x_max - crop_x_min + 1;
        crop_height = crop_y_max - crop_y_min + 1;
    }

    pixel_count = (size_t)crop_width * (size_t)crop_height;
    crop_is_opaque = (uint8_t)(!has_partial_alpha && nonzero_pixel_count == pixel_count);
    data_bytes = pixel_count * sizeof(uint16_t);
    alpha_bytes = crop_is_opaque ? 0u : pixel_count * sizeof(uint8_t);
    row_meta_bytes = crop_is_opaque ? 0u : (size_t)crop_height * sizeof(*row_meta);
    if (data_bytes > (size_t)INT_MAX || alpha_bytes > (size_t)INT_MAX || row_meta_bytes > (size_t)INT_MAX)
    {
        return 0;
    }

    data_buf = (uint16_t *)egui_malloc(NULL, (int)data_bytes);
    if (data_buf == NULL)
    {
        return 0;
    }
    alpha_buf = NULL;
    if (!crop_is_opaque)
    {
        alpha_buf = (uint8_t *)egui_malloc(NULL, (int)alpha_bytes);
        if (alpha_buf == NULL)
        {
            egui_free(NULL, data_buf);
            return 0;
        }
    }
    enable_row_meta = !crop_is_opaque && row_meta_bytes <= (size_t)EGUI_SVG_ROW_META_MAX_BYTES;
    row_meta = NULL;
    if (enable_row_meta)
    {
        row_meta = (egui_svg_raster_row_meta_t *)egui_malloc(NULL, (int)row_meta_bytes);
        if (row_meta == NULL)
        {
            egui_free(NULL, alpha_buf);
            egui_free(NULL, data_buf);
            return 0;
        }
    }

    if (crop_x_max < crop_x_min || crop_y_max < crop_y_min)
    {
        data_buf[0] = 0u;
        alpha_buf[0] = 0u;
        if (row_meta != NULL)
        {
            row_meta[0].visible_start = 0u;
            row_meta[0].visible_end = 0u;
            row_meta[0].opaque_start = 0u;
            row_meta[0].opaque_end = 0u;
            row_meta[0].all_opaque = 0u;
        }
    }

    for (y = 0; y < crop_height; y++)
    {
        const uint32_t *src_row = (const uint32_t *)(surface_data + (size_t)(crop_y_min + y) * (size_t)stride) + crop_x_min;
        uint16_t *dst_data_row = data_buf + (size_t)y * (size_t)crop_width;
        uint8_t *dst_alpha_row = (alpha_buf != NULL) ? (alpha_buf + (size_t)y * (size_t)crop_width) : NULL;
        egui_svg_raster_row_meta_t *row = row_meta != NULL ? &row_meta[y] : NULL;
        int row_visible_start = 0;
        int row_visible_end = 0;
        int row_opaque_start = 0;
        int row_opaque_end = 0;
        int opaque_run_start = 0;
        uint8_t row_all_opaque = 0u;
        int x;

        if (row != NULL)
        {
            row_visible_start = -1;
            row_opaque_start = -1;
            opaque_run_start = -1;
            row_all_opaque = 1u;
        }

        for (x = 0; x < crop_width; x++)
        {
            uint32_t pixel = src_row[x];
            uint32_t a = (pixel >> 24) & 0xFFu;
            uint32_t r = (pixel >> 16) & 0xFFu;
            uint32_t g = (pixel >> 8) & 0xFFu;
            uint32_t b = pixel & 0xFFu;

            if (a != 0u && a != 0xFFu)
            {
                r = (r * 255u) / a;
                g = (g * 255u) / a;
                b = (b * 255u) / a;
            }

            dst_data_row[x] = (uint16_t)(((uint16_t)(r & 0xF8u) << 8) | ((uint16_t)(g & 0xFCu) << 3) | ((uint16_t)b >> 3));
            if (dst_alpha_row != NULL)
            {
                dst_alpha_row[x] = (uint8_t)a;
            }

            if (row == NULL)
            {
                continue;
            }

            if (a == 0u)
            {
                if (opaque_run_start >= 0 && x - opaque_run_start > row_opaque_end - row_opaque_start)
                {
                    row_opaque_start = opaque_run_start;
                    row_opaque_end = x;
                }
                opaque_run_start = -1;
                continue;
            }

            if (row_visible_start < 0)
            {
                row_visible_start = x;
            }
            row_visible_end = x + 1;
            if (a != EGUI_ALPHA_100)
            {
                row_all_opaque = 0u;
                if (opaque_run_start >= 0 && x - opaque_run_start > row_opaque_end - row_opaque_start)
                {
                    row_opaque_start = opaque_run_start;
                    row_opaque_end = x;
                }
                opaque_run_start = -1;
            }
            else if (opaque_run_start < 0)
            {
                opaque_run_start = x;
            }
        }
        if (row != NULL)
        {
            if (opaque_run_start >= 0 && crop_width - opaque_run_start > row_opaque_end - row_opaque_start)
            {
                row_opaque_start = opaque_run_start;
                row_opaque_end = crop_width;
            }

            if (row_visible_start < 0)
            {
                row->visible_start = 0u;
                row->visible_end = 0u;
                row->opaque_start = 0u;
                row->opaque_end = 0u;
                row->all_opaque = 0u;
                row_meta_fast_path_enabled = 1u;
            }
            else
            {
                row->visible_start = (uint16_t)row_visible_start;
                row->visible_end = (uint16_t)row_visible_end;
                row->opaque_start = (row_opaque_start >= 0) ? (uint16_t)row_opaque_start : 0u;
                row->opaque_end = (row_opaque_start >= 0) ? (uint16_t)row_opaque_end : 0u;
                row->all_opaque = row_all_opaque;
                if (row_all_opaque || row_visible_start > 0 || row_visible_end < crop_width ||
                    row_opaque_end - row_opaque_start >= EGUI_SVG_ROW_META_MIN_OPAQUE_SPAN)
                {
                    row_meta_fast_path_enabled = 1u;
                }
            }
        }
    }

    memset(out_info, 0, sizeof(*out_info));
    out_info->data_buf = data_buf;
    out_info->alpha_buf = alpha_buf;
    out_info->data_type = EGUI_IMAGE_DATA_TYPE_RGB565;
    out_info->alpha_type = EGUI_IMAGE_ALPHA_TYPE_8;
    out_info->res_type = EGUI_RESOURCE_TYPE_INTERNAL;
    out_info->width = (uint16_t)crop_width;
    out_info->height = (uint16_t)crop_height;
    *out_offset_x = (egui_dim_t)crop_x_min;
    *out_offset_y = (egui_dim_t)crop_y_min;
    *out_data_buf = data_buf;
    *out_alpha_buf = alpha_buf;
    *out_row_meta = row_meta;
    *out_row_meta_fast_path_enabled = row_meta_fast_path_enabled;
    return 1;
}

static int egui_svg_render_cache(egui_svg_doc_t *doc, egui_dim_t width, egui_dim_t height)
{
    plutovg_surface_t *surface;
    unsigned char *surface_data;
    int actual_width;
    int actual_height;
    int stride;
    uint16_t *new_data_buf;
    uint8_t *new_alpha_buf;
    egui_svg_raster_row_meta_t *new_row_meta;
    uint8_t new_row_meta_fast_path_enabled;
    egui_image_std_info_t new_info;
    egui_dim_t new_offset_x;
    egui_dim_t new_offset_y;

    if (doc == NULL || doc->document == NULL || width <= 0 || height <= 0)
    {
        return 0;
    }
    if (egui_svg_cache_matches(doc, width, height))
    {
        return 1;
    }

    surface = plutosvg_document_render_to_surface(doc->document, NULL, width, height, NULL, NULL, NULL);
    if (surface == NULL)
    {
        EGUI_LOG_WRN("PlutoSVG render failed for %d x %d.\n", width, height);
        return 0;
    }

    surface_data = plutovg_surface_get_data(surface);
    actual_width = plutovg_surface_get_width(surface);
    actual_height = plutovg_surface_get_height(surface);
    stride = plutovg_surface_get_stride(surface);
    if (surface_data == NULL || actual_width <= 0 || actual_height <= 0 || actual_width > EGUI_DIM_MAX || actual_height > EGUI_DIM_MAX ||
        stride < actual_width * 4)
    {
        EGUI_LOG_WRN("PlutoSVG render produced invalid surface %d x %d.\n", actual_width, actual_height);
        plutovg_surface_destroy(surface);
        return 0;
    }

    if (!egui_svg_build_raster_from_argb32(actual_width, actual_height, stride, surface_data, &new_data_buf, &new_alpha_buf, &new_row_meta, &new_info,
                                           &new_row_meta_fast_path_enabled, &new_offset_x, &new_offset_y))
    {
        EGUI_LOG_WRN("PlutoSVG raster cache allocate failed for %d x %d.\n", actual_width, actual_height);
        plutovg_surface_destroy(surface);
        return 0;
    }

    plutovg_surface_destroy(surface);
    egui_svg_release_raster(doc);
    doc->data_buf = new_data_buf;
    doc->alpha_buf = new_alpha_buf;
    doc->row_meta = new_row_meta;
    doc->raster_info = new_info;
    doc->cache_width = width;
    doc->cache_height = height;
    doc->cache_offset_x = new_offset_x;
    doc->cache_offset_y = new_offset_y;
    doc->row_meta_fast_path_enabled = new_row_meta_fast_path_enabled;
    egui_image_std_init(&doc->raster_image.base, &doc->raster_info);
    return 1;
}

__EGUI_STATIC_INLINE__ int egui_svg_ensure_cache(egui_svg_doc_t *doc, egui_dim_t width, egui_dim_t height)
{
    if (doc == NULL || width <= 0 || height <= 0)
    {
        return 0;
    }

    if (egui_svg_cache_matches(doc, width, height))
    {
        return 1;
    }

    return egui_svg_render_cache(doc, width, height);
}

static int egui_svg_cached_raster_query_pixel(const egui_svg_doc_t *doc, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    if (doc == NULL || doc->raster_info.data_buf == NULL)
    {
        return 0;
    }
    if (x < doc->cache_offset_x || y < doc->cache_offset_y)
    {
        return 0;
    }

    x -= doc->cache_offset_x;
    y -= doc->cache_offset_y;
    if (x < 0 || y < 0 || x >= doc->raster_info.width || y >= doc->raster_info.height)
    {
        return 0;
    }

    return doc->raster_image.base.api->get_point(&doc->raster_image.base, x, y, color, alpha);
}

static void egui_svg_cached_raster_draw(const egui_svg_doc_t *doc, egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y)
{
    if (doc == NULL || doc->raster_info.data_buf == NULL || canvas == NULL)
    {
        return;
    }
    doc->raster_image.base.api->draw_image(&doc->raster_image.base, canvas, x + doc->cache_offset_x, y + doc->cache_offset_y);
}

static void egui_image_svg_release_owned_data(egui_image_svg_t *self)
{
    if (self->owned_data_buf != NULL)
    {
        egui_free(NULL, self->owned_data_buf);
        self->owned_data_buf = NULL;
    }
}

static void egui_image_svg_reset(egui_image_svg_t *self)
{
    if (self == NULL)
    {
        return;
    }

    egui_svg_doc_destroy((egui_svg_doc_t *)self->doc);
    self->doc = NULL;
    self->base.res = NULL;
    egui_image_svg_release_owned_data(self);
}

static int egui_image_svg_finish_load(egui_image_svg_t *self, uint8_t *owned_data_buf, uint32_t svg_len)
{
    plutosvg_document_t *document;
    egui_svg_doc_t *doc;
    float natural_width_raw;
    float natural_height_raw;
    egui_dim_t natural_width;
    egui_dim_t natural_height;

    if (self == NULL || owned_data_buf == NULL || svg_len == 0 || svg_len >= (uint32_t)INT_MAX)
    {
        if (owned_data_buf != NULL)
        {
            egui_free(NULL, owned_data_buf);
        }
        return 0;
    }

    self->base.api = &egui_image_svg_t_api_table;
    document = plutosvg_document_load_from_data((const char *)owned_data_buf, -1, -1.0f, -1.0f, NULL, NULL);
    if (document == NULL)
    {
        EGUI_LOG_WRN("PlutoSVG parse failed.\n");
        egui_free(NULL, owned_data_buf);
        return 0;
    }

    natural_width_raw = plutosvg_document_get_width(document);
    natural_height_raw = plutosvg_document_get_height(document);
    if (!(natural_width_raw > 0.0f) || !(natural_height_raw > 0.0f))
    {
        EGUI_LOG_WRN("SVG root size invalid, discard document.\n");
        plutosvg_document_destroy(document);
        egui_free(NULL, owned_data_buf);
        return 0;
    }
    if (natural_width_raw > (float)EGUI_DIM_MAX || natural_height_raw > (float)EGUI_DIM_MAX || !egui_svg_dim_from_float(natural_width_raw, &natural_width) ||
        !egui_svg_dim_from_float(natural_height_raw, &natural_height))
    {
        EGUI_LOG_WRN("SVG root size %ld x %ld exceeds egui_dim_t max %d, discard document.\n", (long)egui_svg_dim_for_log(natural_width_raw),
                     (long)egui_svg_dim_for_log(natural_height_raw), EGUI_DIM_MAX);
        plutosvg_document_destroy(document);
        egui_free(NULL, owned_data_buf);
        return 0;
    }

    doc = (egui_svg_doc_t *)egui_malloc(NULL, sizeof(*doc));
    if (doc == NULL)
    {
        plutosvg_document_destroy(document);
        egui_free(NULL, owned_data_buf);
        return 0;
    }
    memset(doc, 0, sizeof(*doc));
    doc->document = document;
    doc->natural_width = natural_width;
    doc->natural_height = natural_height;
    doc->raster_info.data_type = EGUI_IMAGE_DATA_TYPE_RGB565;
    doc->raster_info.alpha_type = EGUI_IMAGE_ALPHA_TYPE_8;
    doc->raster_info.res_type = EGUI_RESOURCE_TYPE_INTERNAL;
    self->doc = doc;
    self->owned_data_buf = owned_data_buf;
    self->base.res = doc;
    return 1;
}

static int egui_image_svg_get_size(const egui_image_t *self, egui_dim_t *width, egui_dim_t *height)
{
    const egui_image_svg_t *svg = (const egui_image_svg_t *)self;
    const egui_svg_doc_t *doc = (const egui_svg_doc_t *)svg->doc;

    if (width != NULL)
    {
        *width = (doc != NULL) ? doc->natural_width : 0;
    }
    if (height != NULL)
    {
        *height = (doc != NULL) ? doc->natural_height : 0;
    }
    return doc != NULL;
}

static int egui_image_svg_get_point(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_image_svg_t *svg = (egui_image_svg_t *)self;
    egui_svg_doc_t *doc = (egui_svg_doc_t *)svg->doc;

    if (!egui_svg_ensure_cache(doc, doc->natural_width, doc->natural_height))
    {
        return 0;
    }
    return egui_svg_cached_raster_query_pixel(doc, x, y, color, alpha);
}

static int egui_image_svg_get_point_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color,
                                           egui_alpha_t *alpha)
{
    egui_image_svg_t *svg = (egui_image_svg_t *)self;
    egui_svg_doc_t *doc = (egui_svg_doc_t *)svg->doc;

    if (!egui_svg_ensure_cache(doc, width, height))
    {
        return 0;
    }
    return egui_svg_cached_raster_query_pixel(doc, x, y, color, alpha);
}

static void egui_image_svg_draw_image(const egui_image_t *self, egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y)
{
    egui_image_svg_t *svg = (egui_image_svg_t *)self;
    egui_svg_doc_t *doc = (egui_svg_doc_t *)svg->doc;

    if (!egui_svg_ensure_cache(doc, doc->natural_width, doc->natural_height))
    {
        return;
    }
    egui_svg_cached_raster_draw(doc, canvas, x, y);
}

static void egui_image_svg_draw_image_resize(const egui_image_t *self, egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_image_svg_t *svg = (egui_image_svg_t *)self;
    egui_svg_doc_t *doc = (egui_svg_doc_t *)svg->doc;

    if (!egui_svg_ensure_cache(doc, width, height))
    {
        return;
    }
    egui_svg_cached_raster_draw(doc, canvas, x, y);
}

const egui_image_api_t egui_image_svg_t_api_table = {
        .get_size = egui_image_svg_get_size,
        .get_point = egui_image_svg_get_point,
        .get_point_resize = egui_image_svg_get_point_resize,
        .draw_image = egui_image_svg_draw_image,
        .draw_image_resize = egui_image_svg_draw_image_resize,
};

void egui_image_svg_init(egui_image_svg_t *self)
{
    if (self == NULL)
    {
        return;
    }

    egui_image_init(&self->base, NULL);
    self->base.api = &egui_image_svg_t_api_table;
    self->doc = NULL;
    self->owned_data_buf = NULL;
}

void egui_image_svg_deinit(egui_image_svg_t *self)
{
    if (self == NULL)
    {
        return;
    }

    egui_image_svg_reset(self);
}

int egui_image_svg_load_memory(egui_image_svg_t *self, const char *svg_text)
{
    if (svg_text == NULL)
    {
        return 0;
    }
    return egui_image_svg_load_memory_len(self, svg_text, (uint32_t)strlen(svg_text));
}

int egui_image_svg_load_memory_len(egui_image_svg_t *self, const char *svg_text, uint32_t svg_len)
{
    uint8_t *owned_data_buf;

    if (self == NULL || svg_text == NULL || svg_len == 0 || svg_len >= (uint32_t)INT_MAX)
    {
        return 0;
    }

    owned_data_buf = (uint8_t *)egui_malloc(NULL, (int)svg_len + 1);
    if (owned_data_buf == NULL)
    {
        return 0;
    }
    memcpy(owned_data_buf, svg_text, svg_len);
    owned_data_buf[svg_len] = '\0';

    egui_image_svg_reset(self);
    return egui_image_svg_finish_load(self, owned_data_buf, svg_len);
}

int egui_image_svg_load_resource(egui_image_svg_t *self, const egui_svg_source_t *res)
{
    uint8_t *owned_data_buf;

    if (self == NULL || res == NULL || res->data_buf == NULL || res->data_size == 0 || res->data_size >= (uint32_t)INT_MAX)
    {
        return 0;
    }

    owned_data_buf = (uint8_t *)egui_malloc(NULL, (int)res->data_size + 1);
    if (owned_data_buf == NULL)
    {
        return 0;
    }

    if (res->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
    {
        memcpy(owned_data_buf, res->data_buf, res->data_size);
    }
    else if (res->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_api_load_external_resource(NULL, owned_data_buf, (egui_uintptr_t)res->data_buf, 0, res->data_size);
    }
    else
    {
        egui_free(NULL, owned_data_buf);
        return 0;
    }
    owned_data_buf[res->data_size] = '\0';

    egui_image_svg_reset(self);
    return egui_image_svg_finish_load(self, owned_data_buf, res->data_size);
}

int egui_image_svg_is_valid(const egui_image_svg_t *self)
{
    return self != NULL && self->doc != NULL;
}

void egui_image_svg_get_width_height(const egui_image_t *self, egui_dim_t *width, egui_dim_t *height)
{
    const egui_image_svg_t *svg = (const egui_image_svg_t *)self;
    const egui_svg_doc_t *doc = (const egui_svg_doc_t *)svg->doc;

    if (width != NULL)
    {
        *width = (doc != NULL) ? doc->natural_width : 0;
    }
    if (height != NULL)
    {
        *height = (doc != NULL) ? doc->natural_height : 0;
    }
}

void egui_image_svg_release_frame_cache(void)
{
}

#else

void egui_image_svg_init(egui_image_svg_t *self)
{
    if (self == NULL)
    {
        return;
    }
    egui_image_init(&self->base, NULL);
    self->doc = NULL;
    self->owned_data_buf = NULL;
}

void egui_image_svg_deinit(egui_image_svg_t *self)
{
    if (self == NULL)
    {
        return;
    }
    self->doc = NULL;
    self->base.res = NULL;
    self->owned_data_buf = NULL;
}

int egui_image_svg_load_memory(egui_image_svg_t *self, const char *svg_text)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(svg_text);
    return 0;
}

int egui_image_svg_load_memory_len(egui_image_svg_t *self, const char *svg_text, uint32_t svg_len)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(svg_text);
    EGUI_UNUSED(svg_len);
    return 0;
}

int egui_image_svg_load_resource(egui_image_svg_t *self, const egui_svg_source_t *res)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(res);
    return 0;
}

int egui_image_svg_is_valid(const egui_image_svg_t *self)
{
    EGUI_UNUSED(self);
    return 0;
}

void egui_image_svg_get_width_height(const egui_image_t *self, egui_dim_t *width, egui_dim_t *height)
{
    EGUI_UNUSED(self);
    if (width != NULL)
    {
        *width = 0;
    }
    if (height != NULL)
    {
        *height = 0;
    }
}

void egui_image_svg_release_frame_cache(void)
{
}

#endif
