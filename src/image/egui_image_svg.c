#include "egui_image_svg.h"
#include "egui_image_svg_alloc.h"
#include "egui_image_std.h"
#include "core/egui_api.h"
#include "core/egui_core.h"

#include <limits.h>
#include <string.h>

typedef struct egui_svg_alloc_header
{
    size_t size;
} egui_svg_alloc_header_t;

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
        egui_api_memset(ptr, 0, (int)total);
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

void *egui_svg_alloc_plain_malloc(size_t size)
{
    if (size == 0 || size > (size_t)INT_MAX)
    {
        return NULL;
    }

    return egui_malloc(NULL, (int)size);
}

void *egui_svg_alloc_plain_calloc(size_t count, size_t size)
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
    ptr = egui_svg_alloc_plain_malloc(total);
    if (ptr != NULL)
    {
        egui_api_memset(ptr, 0, (int)total);
    }
    return ptr;
}

void *egui_svg_alloc_plain_realloc(void *ptr, size_t old_size, size_t size)
{
    void *new_ptr;
    size_t copy_size;

    if (ptr == NULL)
    {
        return egui_svg_alloc_plain_malloc(size);
    }
    if (size == 0)
    {
        egui_svg_alloc_plain_free(ptr);
        return NULL;
    }

    new_ptr = egui_svg_alloc_plain_malloc(size);
    if (new_ptr == NULL)
    {
        return NULL;
    }

    copy_size = old_size;
    if (copy_size > size)
    {
        copy_size = size;
    }
    if (copy_size > 0)
    {
        memcpy(new_ptr, ptr, copy_size);
    }
    egui_svg_alloc_plain_free(ptr);
    return new_ptr;
}

void egui_svg_alloc_plain_free(void *ptr)
{
    if (ptr != NULL)
    {
        egui_free(NULL, ptr);
    }
}

#if EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG

#ifndef PLUTOVG_BUILD_STATIC
#define PLUTOVG_BUILD_STATIC 1
#endif

#ifndef PLUTOSVG_BUILD_STATIC
#define PLUTOSVG_BUILD_STATIC 1
#endif

#include "plutosvg.h"

typedef struct
{
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t width;
    egui_dim_t height;
    egui_color_t color;
} egui_svg_rect_t;

typedef struct
{
    egui_dim_t left;
    egui_dim_t top;
    egui_dim_t right;
    egui_dim_t bottom;
    egui_color_t color;
} egui_svg_pixel_rect_t;

typedef struct
{
    egui_svg_rect_t *rects;
    egui_svg_pixel_rect_t *pixel_rects;
    uint16_t rect_count;
    uint16_t pixel_rect_count;
    uint8_t rects_sorted;
    uint8_t storage_embedded;
    egui_dim_t view_x;
    egui_dim_t view_y;
    egui_dim_t view_width;
    egui_dim_t view_height;
    egui_dim_t rect_content_left;
    egui_dim_t rect_content_top;
    egui_dim_t rect_content_right;
    egui_dim_t rect_content_bottom;
    egui_dim_t pixel_rect_width;
    egui_dim_t pixel_rect_height;
    egui_dim_t pixel_content_left;
    egui_dim_t pixel_content_top;
    egui_dim_t pixel_content_right;
    egui_dim_t pixel_content_bottom;
} egui_svg_rect_state_t;

typedef struct
{
    plutosvg_document_t *document;
    egui_image_std_info_t raster_info;
    egui_image_std_t raster_image;
    uint16_t *data_buf;
    uint8_t *alpha_buf;
    egui_dim_t cache_width;
    egui_dim_t cache_height;
    egui_dim_t cache_offset_x;
    egui_dim_t cache_offset_y;
} egui_svg_raster_state_t;

typedef struct egui_svg_doc
{
    egui_dim_t natural_width;
    egui_dim_t natural_height;
    uint8_t rect_fast_path;
    union
    {
        egui_svg_rect_state_t rect;
        egui_svg_raster_state_t raster;
    } state;
} egui_svg_doc_t;

extern const egui_image_api_t egui_image_svg_t_api_table;

#ifndef EGUI_CONFIG_IMAGE_SVG_RENDER_SURFACE_MAX_BYTES
#define EGUI_CONFIG_IMAGE_SVG_RENDER_SURFACE_MAX_BYTES ((EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT * (int)sizeof(egui_color_int_t)) / 3)
#endif

#ifndef EGUI_CONFIG_IMAGE_SVG_CACHE_MAX_BYTES
#define EGUI_CONFIG_IMAGE_SVG_CACHE_MAX_BYTES (EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT * (int)sizeof(egui_color_int_t))
#endif

#ifndef EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_MAX_CELLS
#define EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_MAX_CELLS 256
#endif

#ifndef EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_FLATTEN
#define EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_FLATTEN 0
#endif

#ifndef EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_PIXEL_CACHE
#define EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_PIXEL_CACHE 1
#endif

#ifndef EGUI_CONFIG_IMAGE_SVG_EXTERNAL_FAST_PATH_CHUNK_BYTES
#define EGUI_CONFIG_IMAGE_SVG_EXTERNAL_FAST_PATH_CHUNK_BYTES 64
#endif

#ifndef EGUI_CONFIG_IMAGE_SVG_EXTERNAL_FAST_PATH_TAG_BYTES
#define EGUI_CONFIG_IMAGE_SVG_EXTERNAL_FAST_PATH_TAG_BYTES 256
#endif

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

static int egui_svg_dim_from_integral_float(float value, egui_dim_t *out_value)
{
    int32_t raw;

    if (out_value == NULL || value < (float)(-EGUI_DIM_MAX) || value > (float)EGUI_DIM_MAX)
    {
        return 0;
    }

    raw = (int32_t)value;
    if ((float)raw != value)
    {
        return 0;
    }

    *out_value = (egui_dim_t)raw;
    return 1;
}

static __EGUI_UNUSED_ATTR__ int32_t egui_svg_dim_for_log(float value)
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

static size_t egui_svg_align_size(size_t value, size_t alignment)
{
    if (alignment <= 1u)
    {
        return value;
    }
    return ((value + alignment - 1u) / alignment) * alignment;
}

static int egui_svg_count_fits_int(uint16_t count, size_t item_size)
{
    return (size_t)count <= ((size_t)INT_MAX / item_size);
}

static int egui_svg_calc_rect_fast_path_doc_size(uint16_t rect_count, size_t *rect_offset, size_t *pixel_rect_offset, size_t *alloc_size)
{
    size_t offset = sizeof(egui_svg_doc_t);
    size_t rect_bytes;
#if EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_PIXEL_CACHE
    size_t pixel_rect_bytes;
#endif

    if (rect_count == 0 || rect_offset == NULL || pixel_rect_offset == NULL || alloc_size == NULL)
    {
        return 0;
    }
    if (!egui_svg_count_fits_int(rect_count, sizeof(egui_svg_rect_t)))
    {
        return 0;
    }

    rect_bytes = (size_t)rect_count * sizeof(egui_svg_rect_t);
    offset = egui_svg_align_size(offset, sizeof(void *));
    if (offset > (size_t)INT_MAX || rect_bytes > (size_t)INT_MAX - offset)
    {
        return 0;
    }
    *rect_offset = offset;
    offset += rect_bytes;

#if EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_PIXEL_CACHE
    if (!egui_svg_count_fits_int(rect_count, sizeof(egui_svg_pixel_rect_t)))
    {
        return 0;
    }
    pixel_rect_bytes = (size_t)rect_count * sizeof(egui_svg_pixel_rect_t);
    offset = egui_svg_align_size(offset, sizeof(void *));
    if (offset > (size_t)INT_MAX || pixel_rect_bytes > (size_t)INT_MAX - offset)
    {
        return 0;
    }
    *pixel_rect_offset = offset;
    offset += pixel_rect_bytes;
#else
    *pixel_rect_offset = 0u;
#endif

    if (offset > (size_t)INT_MAX)
    {
        return 0;
    }
    *alloc_size = offset;
    return 1;
}

static egui_svg_doc_t *egui_svg_alloc_rect_fast_path_doc(uint16_t rect_count, egui_svg_rect_t **rects)
{
    egui_svg_doc_t *doc;
    size_t rect_offset;
    size_t pixel_rect_offset;
    size_t alloc_size;

    if (rects == NULL || !egui_svg_calc_rect_fast_path_doc_size(rect_count, &rect_offset, &pixel_rect_offset, &alloc_size))
    {
        return NULL;
    }

    doc = (egui_svg_doc_t *)egui_malloc(NULL, (int)alloc_size);
    if (doc == NULL)
    {
        return NULL;
    }

    egui_api_memset(doc, 0, sizeof(*doc));
    doc->rect_fast_path = 1u;
    doc->state.rect.storage_embedded = 1u;
    doc->state.rect.rects = (egui_svg_rect_t *)((uint8_t *)doc + rect_offset);
#if EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_PIXEL_CACHE
    doc->state.rect.pixel_rects = (egui_svg_pixel_rect_t *)((uint8_t *)doc + pixel_rect_offset);
#endif
    *rects = doc->state.rect.rects;
    return doc;
}

static void egui_svg_doc_destroy(egui_svg_doc_t *doc)
{
    if (doc == NULL)
    {
        return;
    }

    if (doc->rect_fast_path)
    {
        if (doc->state.rect.rects != NULL && !doc->state.rect.storage_embedded)
        {
            egui_free(NULL, doc->state.rect.rects);
            doc->state.rect.rects = NULL;
        }
        if (doc->state.rect.pixel_rects != NULL && !doc->state.rect.storage_embedded)
        {
            egui_free(NULL, doc->state.rect.pixel_rects);
            doc->state.rect.pixel_rects = NULL;
        }
    }
    else
    {
        if (doc->state.raster.data_buf != NULL)
        {
            egui_free(NULL, doc->state.raster.data_buf);
            doc->state.raster.data_buf = NULL;
        }
        if (doc->state.raster.alpha_buf != NULL)
        {
            egui_free(NULL, doc->state.raster.alpha_buf);
            doc->state.raster.alpha_buf = NULL;
        }
        if (doc->state.raster.document != NULL)
        {
            plutosvg_document_destroy(doc->state.raster.document);
            doc->state.raster.document = NULL;
        }
    }
    egui_free(NULL, doc);
}

#if EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_PIXEL_CACHE
static void egui_svg_release_pixel_rect_cache(egui_svg_doc_t *doc)
{
    if (doc == NULL || !doc->rect_fast_path)
    {
        return;
    }

    if (doc->state.rect.pixel_rects != NULL && !doc->state.rect.storage_embedded)
    {
        egui_free(NULL, doc->state.rect.pixel_rects);
        doc->state.rect.pixel_rects = NULL;
    }
    doc->state.rect.pixel_rect_count = 0;
    doc->state.rect.pixel_rect_width = 0;
    doc->state.rect.pixel_rect_height = 0;
    doc->state.rect.pixel_content_left = 0;
    doc->state.rect.pixel_content_top = 0;
    doc->state.rect.pixel_content_right = 0;
    doc->state.rect.pixel_content_bottom = 0;
}
#endif

static void egui_svg_release_cache(egui_svg_doc_t *doc)
{
    if (doc == NULL)
    {
        return;
    }

    if (doc->state.raster.data_buf != NULL)
    {
        egui_free(NULL, doc->state.raster.data_buf);
        doc->state.raster.data_buf = NULL;
    }
    if (doc->state.raster.alpha_buf != NULL)
    {
        egui_free(NULL, doc->state.raster.alpha_buf);
        doc->state.raster.alpha_buf = NULL;
    }

    doc->state.raster.raster_info.data_buf = NULL;
    doc->state.raster.raster_info.alpha_buf = NULL;
    doc->state.raster.raster_info.width = 0;
    doc->state.raster.raster_info.height = 0;
    doc->state.raster.raster_image.base.res = NULL;
    doc->state.raster.cache_width = 0;
    doc->state.raster.cache_height = 0;
    doc->state.raster.cache_offset_x = 0;
    doc->state.raster.cache_offset_y = 0;
}

__EGUI_STATIC_INLINE__ int egui_svg_cache_matches(const egui_svg_doc_t *doc, egui_dim_t width, egui_dim_t height)
{
    return doc != NULL && !doc->rect_fast_path && doc->state.raster.raster_info.data_buf != NULL && doc->state.raster.cache_width == width &&
           doc->state.raster.cache_height == height;
}

static int32_t egui_svg_render_max_surface_pixels(void)
{
    int32_t max_bytes = EGUI_CONFIG_IMAGE_SVG_RENDER_SURFACE_MAX_BYTES;
    int32_t max_pixels;

    if (max_bytes < 4)
    {
        return 1;
    }

    max_pixels = max_bytes / 4;
    return max_pixels > 0 ? max_pixels : 1;
}

static int egui_svg_bytes_fit_limit(size_t bytes, int32_t limit)
{
    return limit > 0 && bytes <= (size_t)limit && bytes <= (size_t)INT_MAX;
}

static int egui_svg_cache_region_allowed(egui_dim_t width, egui_dim_t height)
{
    size_t pixel_count;
    size_t surface_bytes;
    size_t min_cache_bytes;

    if (width <= 0 || height <= 0)
    {
        return 0;
    }

    pixel_count = (size_t)width * (size_t)height;
    surface_bytes = pixel_count * 4u;
    min_cache_bytes = pixel_count * sizeof(uint16_t);
    if (pixel_count == 0u || surface_bytes / 4u != pixel_count || min_cache_bytes / sizeof(uint16_t) != pixel_count)
    {
        return 0;
    }

    return egui_svg_bytes_fit_limit(surface_bytes, EGUI_CONFIG_IMAGE_SVG_RENDER_SURFACE_MAX_BYTES) &&
           egui_svg_bytes_fit_limit(min_cache_bytes, EGUI_CONFIG_IMAGE_SVG_CACHE_MAX_BYTES);
}

static uint16_t egui_svg_argb32_to_rgb565(uint32_t pixel, egui_alpha_t *alpha)
{
    uint32_t a = (pixel >> 24) & 0xFFu;
    uint32_t r = (pixel >> 16) & 0xFFu;
    uint32_t g = (pixel >> 8) & 0xFFu;
    uint32_t b = pixel & 0xFFu;

    if (a != 0u && a != 0xFFu)
    {
        r = (r * 255u) / a;
        g = (g * 255u) / a;
        b = (b * 255u) / a;
        if (r > 255u)
        {
            r = 255u;
        }
        if (g > 255u)
        {
            g = 255u;
        }
        if (b > 255u)
        {
            b = 255u;
        }
    }

    if (alpha != NULL)
    {
        *alpha = (egui_alpha_t)a;
    }
    return (uint16_t)(((uint16_t)(r & 0xF8u) << 8) | ((uint16_t)(g & 0xFCu) << 3) | ((uint16_t)b >> 3));
}

static int egui_svg_is_space(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '\f';
}

static int egui_svg_name_equals(const char *begin, const char *end, const char *name)
{
    size_t len = (size_t)(end - begin);

    return strlen(name) == len && memcmp(begin, name, len) == 0;
}

static int egui_svg_find_attr_value(const char *begin, const char *end, const char *name, const char **value_begin, const char **value_end)
{
    const char *p = begin;

    while (p < end)
    {
        const char *attr_begin;
        const char *attr_end;
        const char *q;
        char quote;

        while (p < end && (egui_svg_is_space(*p) || *p == '/'))
        {
            p++;
        }
        if (p >= end)
        {
            break;
        }

        attr_begin = p;
        while (p < end && !egui_svg_is_space(*p) && *p != '=' && *p != '/' && *p != '>')
        {
            p++;
        }
        attr_end = p;
        q = p;
        while (q < end && egui_svg_is_space(*q))
        {
            q++;
        }
        if (q >= end || *q != '=')
        {
            p = q;
            continue;
        }
        q++;
        while (q < end && egui_svg_is_space(*q))
        {
            q++;
        }
        if (q >= end || (*q != '\'' && *q != '"'))
        {
            return 0;
        }

        quote = *q;
        q++;
        p = q;
        while (p < end && *p != quote)
        {
            p++;
        }
        if (p >= end)
        {
            return 0;
        }

        if (egui_svg_name_equals(attr_begin, attr_end, name))
        {
            if (value_begin != NULL)
            {
                *value_begin = q;
            }
            if (value_end != NULL)
            {
                *value_end = p;
            }
            return 1;
        }
        p++;
    }

    return 0;
}

static int egui_svg_attr_exists(const char *begin, const char *end, const char *name)
{
    return egui_svg_find_attr_value(begin, end, name, NULL, NULL);
}

static int egui_svg_parse_float_token(const char **cursor, const char *end, float *out_value)
{
    const char *p = *cursor;
    int sign = 1;
    int saw_digit = 0;
    float value = 0.0f;
    float frac_scale = 0.1f;

    while (p < end && (egui_svg_is_space(*p) || *p == ','))
    {
        p++;
    }
    if (p < end && (*p == '-' || *p == '+'))
    {
        sign = (*p == '-') ? -1 : 1;
        p++;
    }

    while (p < end && *p >= '0' && *p <= '9')
    {
        saw_digit = 1;
        value = value * 10.0f + (float)(*p - '0');
        p++;
    }

    if (p < end && *p == '.')
    {
        p++;
        while (p < end && *p >= '0' && *p <= '9')
        {
            saw_digit = 1;
            value += (float)(*p - '0') * frac_scale;
            frac_scale *= 0.1f;
            p++;
        }
    }

    if (!saw_digit)
    {
        return 0;
    }

    while (p < end && egui_svg_is_space(*p))
    {
        p++;
    }

    *cursor = p;
    *out_value = (sign < 0) ? -value : value;
    return 1;
}

static int egui_svg_parse_float_value(const char *begin, const char *end, float *out_value)
{
    const char *p = begin;

    if (!egui_svg_parse_float_token(&p, end, out_value))
    {
        return 0;
    }
    while (p < end && egui_svg_is_space(*p))
    {
        p++;
    }
    return p == end;
}

static int egui_svg_parse_float_attr(const char *begin, const char *end, const char *name, float *out_value)
{
    const char *value_begin;
    const char *value_end;

    if (!egui_svg_find_attr_value(begin, end, name, &value_begin, &value_end))
    {
        return 0;
    }
    return egui_svg_parse_float_value(value_begin, value_end, out_value);
}

static int egui_svg_parse_view_box(const char *begin, const char *end, float *view_x, float *view_y, float *view_width, float *view_height)
{
    const char *value_begin;
    const char *value_end;
    const char *p;

    if (!egui_svg_find_attr_value(begin, end, "viewBox", &value_begin, &value_end))
    {
        return 1;
    }

    p = value_begin;
    if (!egui_svg_parse_float_token(&p, value_end, view_x) || !egui_svg_parse_float_token(&p, value_end, view_y) ||
        !egui_svg_parse_float_token(&p, value_end, view_width) || !egui_svg_parse_float_token(&p, value_end, view_height))
    {
        return 0;
    }
    while (p < value_end && (egui_svg_is_space(*p) || *p == ','))
    {
        p++;
    }
    return p == value_end && *view_width > 0.0f && *view_height > 0.0f;
}

static int egui_svg_hex_value(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F')
    {
        return ch - 'A' + 10;
    }
    return -1;
}

static int egui_svg_parse_color_attr(const char *begin, const char *end, egui_color_t *color)
{
    const char *value_begin;
    const char *value_end;
    uint8_t rgb[3];

    if (!egui_svg_find_attr_value(begin, end, "fill", &value_begin, &value_end) || value_end - value_begin != 7 || value_begin[0] != '#')
    {
        return 0;
    }

    for (int i = 0; i < 3; i++)
    {
        int high = egui_svg_hex_value(value_begin[1 + i * 2]);
        int low = egui_svg_hex_value(value_begin[2 + i * 2]);

        if (high < 0 || low < 0)
        {
            return 0;
        }
        rgb[i] = (uint8_t)((high << 4) | low);
    }

    *color = EGUI_COLOR_MAKE(rgb[0], rgb[1], rgb[2]);
    return 1;
}

static int egui_svg_rect_has_unsupported_attr(const char *begin, const char *end)
{
    return egui_svg_attr_exists(begin, end, "transform") || egui_svg_attr_exists(begin, end, "stroke") || egui_svg_attr_exists(begin, end, "opacity") ||
           egui_svg_attr_exists(begin, end, "fill-opacity") || egui_svg_attr_exists(begin, end, "rx") || egui_svg_attr_exists(begin, end, "ry") ||
           egui_svg_attr_exists(begin, end, "style") || egui_svg_attr_exists(begin, end, "display") || egui_svg_attr_exists(begin, end, "visibility");
}

static int egui_svg_parse_rect_tag(const char *begin, const char *end, egui_svg_rect_t *rect)
{
    float x_raw = 0.0f;
    float y_raw = 0.0f;
    float width_raw;
    float height_raw;
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_dim_t width;
    egui_dim_t height;
    egui_color_t color;

    if (egui_svg_rect_has_unsupported_attr(begin, end))
    {
        return 0;
    }

    if (egui_svg_attr_exists(begin, end, "x") && (!egui_svg_parse_float_attr(begin, end, "x", &x_raw) || !egui_svg_dim_from_integral_float(x_raw, &x)))
    {
        return 0;
    }
    if (egui_svg_attr_exists(begin, end, "y") && (!egui_svg_parse_float_attr(begin, end, "y", &y_raw) || !egui_svg_dim_from_integral_float(y_raw, &y)))
    {
        return 0;
    }
    if (!egui_svg_parse_float_attr(begin, end, "width", &width_raw) || !egui_svg_parse_float_attr(begin, end, "height", &height_raw) || width_raw <= 0.0f ||
        height_raw <= 0.0f || !egui_svg_dim_from_integral_float(width_raw, &width) || !egui_svg_dim_from_integral_float(height_raw, &height) ||
        !egui_svg_parse_color_attr(begin, end, &color))
    {
        return 0;
    }

    rect->x = x;
    rect->y = y;
    rect->width = width;
    rect->height = height;
    rect->color = color;
    return 1;
}

static const char *egui_svg_find_tag_end(const char *begin, const char *end)
{
    while (begin < end && *begin != '>')
    {
        begin++;
    }
    return (begin < end) ? begin : NULL;
}

static int egui_svg_tag_is_self_closing(const char *begin, const char *tag_end)
{
    const char *p = tag_end;

    while (p > begin && egui_svg_is_space(*(p - 1)))
    {
        p--;
    }
    return p > begin && *(p - 1) == '/';
}

static int egui_svg_scan_rect_fast_path_tag(const char *tag_begin, const char *tag_end, egui_svg_rect_t *rects, uint16_t *count, int *svg_depth, float *view_x,
                                            float *view_y, float *view_width, float *view_height, float *natural_width, float *natural_height)
{
    const char *p = tag_begin;
    const char *name_begin;
    const char *name_end;
    int closing = 0;

    if (tag_begin == NULL || tag_end == NULL || count == NULL || svg_depth == NULL || view_x == NULL || view_y == NULL || view_width == NULL ||
        view_height == NULL || p >= tag_end)
    {
        return 0;
    }

    if (*p == '?' || *p == '!')
    {
        return 1;
    }
    if (*p == '/')
    {
        closing = 1;
        p++;
    }
    while (p < tag_end && egui_svg_is_space(*p))
    {
        p++;
    }
    name_begin = p;
    while (p < tag_end && !egui_svg_is_space(*p) && *p != '/' && *p != '>')
    {
        p++;
    }
    name_end = p;

    if (egui_svg_name_equals(name_begin, name_end, "svg"))
    {
        if (closing)
        {
            if (*svg_depth != 1)
            {
                return 0;
            }
            (*svg_depth)--;
        }
        else
        {
            float root_width;
            float root_height;

            if (*svg_depth != 0 || egui_svg_tag_is_self_closing(name_end, tag_end))
            {
                return 0;
            }
            if (natural_width != NULL || natural_height != NULL)
            {
                if (!egui_svg_parse_float_attr(name_end, tag_end, "width", &root_width) ||
                    !egui_svg_parse_float_attr(name_end, tag_end, "height", &root_height) || root_width <= 0.0f || root_height <= 0.0f)
                {
                    return 0;
                }
                if (natural_width != NULL)
                {
                    *natural_width = root_width;
                }
                if (natural_height != NULL)
                {
                    *natural_height = root_height;
                }
                if (*view_width <= 0.0f)
                {
                    *view_width = root_width;
                }
                if (*view_height <= 0.0f)
                {
                    *view_height = root_height;
                }
            }
            if (egui_svg_attr_exists(name_end, tag_end, "transform") || !egui_svg_parse_view_box(name_end, tag_end, view_x, view_y, view_width, view_height))
            {
                return 0;
            }
            (*svg_depth)++;
        }
    }
    else if (egui_svg_name_equals(name_begin, name_end, "rect"))
    {
        if (!closing)
        {
            if (*svg_depth != 1)
            {
                return 0;
            }
            if (rects != NULL)
            {
                if (!egui_svg_parse_rect_tag(name_end, tag_end, &rects[*count]))
                {
                    return 0;
                }
            }
            if (*count == UINT16_MAX)
            {
                return 0;
            }
            (*count)++;
        }
    }
    else
    {
        return 0;
    }

    return 1;
}

static int egui_svg_scan_rect_fast_path(const char *svg_text, const char *svg_end, egui_svg_rect_t *rects, uint16_t *rect_count, float *view_x, float *view_y,
                                        float *view_width, float *view_height, float *natural_width, float *natural_height)
{
    const char *p = svg_text;
    uint16_t count = 0;
    int svg_depth = 0;

    while (p < svg_end)
    {
        const char *lt = (const char *)memchr(p, '<', (size_t)(svg_end - p));
        const char *tag_end;

        if (lt == NULL)
        {
            break;
        }
        p = lt + 1;
        if (p >= svg_end)
        {
            return 0;
        }
        tag_end = egui_svg_find_tag_end(p, svg_end);
        if (tag_end == NULL)
        {
            return 0;
        }

        if (!egui_svg_scan_rect_fast_path_tag(p, tag_end, rects, &count, &svg_depth, view_x, view_y, view_width, view_height, natural_width, natural_height))
        {
            return 0;
        }

        p = tag_end + 1;
    }

    *rect_count = count;
    return count > 0 && svg_depth == 0;
}

static int egui_svg_scan_external_rect_fast_path(egui_core_t *core, const egui_svg_source_t *res, egui_svg_rect_t *rects, uint16_t *rect_count,
                                                 float *view_x, float *view_y, float *view_width, float *view_height, float *natural_width,
                                                 float *natural_height)
{
    uint8_t chunk[EGUI_CONFIG_IMAGE_SVG_EXTERNAL_FAST_PATH_CHUNK_BYTES];
    char tag_buf[EGUI_CONFIG_IMAGE_SVG_EXTERNAL_FAST_PATH_TAG_BYTES];
    uint32_t offset = 0;
    uint16_t count = 0;
    uint16_t tag_len = 0;
    int svg_depth = 0;
    int in_tag = 0;

    if (core == NULL || res == NULL || res->data_buf == NULL || res->data_size == 0 || rect_count == NULL || view_x == NULL || view_y == NULL ||
        view_width == NULL || view_height == NULL || sizeof(chunk) == 0u || sizeof(tag_buf) == 0u)
    {
        return 0;
    }

    while (offset < res->data_size)
    {
        uint32_t load_size = (uint32_t)sizeof(chunk);

        if (load_size > res->data_size - offset)
        {
            load_size = res->data_size - offset;
        }
        egui_api_load_external_resource(&core->canvas, chunk, (egui_uintptr_t)res->data_buf, offset, load_size);

        for (uint32_t i = 0; i < load_size; i++)
        {
            char ch = (char)chunk[i];

            if (!in_tag)
            {
                if (ch == '<')
                {
                    in_tag = 1;
                    tag_len = 0;
                }
                continue;
            }

            if (ch == '>')
            {
                if (!egui_svg_scan_rect_fast_path_tag(tag_buf, tag_buf + tag_len, rects, &count, &svg_depth, view_x, view_y, view_width, view_height,
                                                      natural_width, natural_height))
                {
                    return 0;
                }
                in_tag = 0;
                tag_len = 0;
                continue;
            }

            if (tag_len >= (uint16_t)sizeof(tag_buf))
            {
                return 0;
            }
            tag_buf[tag_len++] = ch;
        }

        offset += load_size;
    }

    if (in_tag)
    {
        return 0;
    }

    *rect_count = count;
    return count > 0 && svg_depth == 0;
}

static int egui_svg_try_get_rect_fast_path_size(const char *svg_text, uint32_t svg_len, egui_dim_t *natural_width, egui_dim_t *natural_height,
                                                uint16_t *fast_path_rect_count)
{
    const char *svg_end;
    uint16_t rect_count = 0;
    float view_x = 0.0f;
    float view_y = 0.0f;
    float view_width = 0.0f;
    float view_height = 0.0f;
    float natural_width_raw = 0.0f;
    float natural_height_raw = 0.0f;
    egui_dim_t view_value;

    if (svg_text == NULL || svg_len == 0 || natural_width == NULL || natural_height == NULL)
    {
        return 0;
    }

    svg_end = svg_text + svg_len;
    if (!egui_svg_scan_rect_fast_path(svg_text, svg_end, NULL, &rect_count, &view_x, &view_y, &view_width, &view_height, &natural_width_raw,
                                      &natural_height_raw))
    {
        return 0;
    }
    if (!egui_svg_dim_from_float(natural_width_raw, natural_width) || !egui_svg_dim_from_float(natural_height_raw, natural_height))
    {
        return 0;
    }
    if (!egui_svg_dim_from_integral_float(view_x, &view_value) || !egui_svg_dim_from_integral_float(view_y, &view_value) ||
        !egui_svg_dim_from_integral_float(view_width, &view_value) || !egui_svg_dim_from_integral_float(view_height, &view_value) || view_width <= 0.0f ||
        view_height <= 0.0f)
    {
        return 0;
    }
    if (fast_path_rect_count != NULL)
    {
        *fast_path_rect_count = rect_count;
    }
    return 1;
}

static void egui_svg_get_rect_content_bounds(const egui_svg_rect_t *rects, uint16_t rect_count, egui_dim_t *left, egui_dim_t *top, egui_dim_t *right,
                                             egui_dim_t *bottom)
{
    if (rects == NULL || rect_count == 0 || left == NULL || top == NULL || right == NULL || bottom == NULL)
    {
        return;
    }

    *left = rects[0].x;
    *top = rects[0].y;
    *right = rects[0].x + rects[0].width;
    *bottom = rects[0].y + rects[0].height;
    for (uint16_t i = 1; i < rect_count; i++)
    {
        const egui_svg_rect_t *rect = &rects[i];

        if (rect->x < *left)
        {
            *left = rect->x;
        }
        if (rect->y < *top)
        {
            *top = rect->y;
        }
        if (rect->x + rect->width > *right)
        {
            *right = rect->x + rect->width;
        }
        if (rect->y + rect->height > *bottom)
        {
            *bottom = rect->y + rect->height;
        }
    }
}

static void egui_svg_sort_float_edges(float *edges, uint16_t count)
{
    for (uint16_t i = 1; i < count; i++)
    {
        float value = edges[i];
        uint16_t j = i;

        while (j > 0 && edges[j - 1] > value)
        {
            edges[j] = edges[j - 1];
            j--;
        }
        edges[j] = value;
    }
}

static uint16_t egui_svg_unique_float_edges(float *edges, uint16_t count)
{
    uint16_t out_count = 0;

    for (uint16_t i = 0; i < count; i++)
    {
        if (out_count == 0 || edges[i] != edges[out_count - 1])
        {
            edges[out_count] = edges[i];
            out_count++;
        }
    }
    return out_count;
}

static int egui_svg_rect_color_at(const egui_svg_rect_t *rects, uint16_t rect_count, float x, float y, egui_color_t *color)
{
    int found = 0;

    for (uint16_t i = 0; i < rect_count; i++)
    {
        const egui_svg_rect_t *rect = &rects[i];

        if (x >= rect->x && x < rect->x + rect->width && y >= rect->y && y < rect->y + rect->height)
        {
            if (color != NULL)
            {
                *color = rect->color;
            }
            found = 1;
        }
    }
    return found;
}

static int egui_svg_rect_same_paint(const egui_svg_rect_t *a, const egui_svg_rect_t *b)
{
    return a->color.full == b->color.full;
}

static void egui_svg_flatten_append_rect(egui_svg_rect_t *rects, uint16_t *rect_count, float x, float y, float width, float height, egui_color_t color)
{
    egui_svg_rect_t rect;
    egui_dim_t rect_x;
    egui_dim_t rect_y;
    egui_dim_t rect_width;
    egui_dim_t rect_height;

    if (rects == NULL || rect_count == NULL || width <= 0.0f || height <= 0.0f)
    {
        return;
    }

    if (!egui_svg_dim_from_integral_float(x, &rect_x) || !egui_svg_dim_from_integral_float(y, &rect_y) ||
        !egui_svg_dim_from_integral_float(width, &rect_width) || !egui_svg_dim_from_integral_float(height, &rect_height))
    {
        return;
    }

    rect.x = rect_x;
    rect.y = rect_y;
    rect.width = rect_width;
    rect.height = rect_height;
    rect.color = color;

    for (uint16_t i = *rect_count; i > 0; i--)
    {
        egui_svg_rect_t *prev = &rects[i - 1];

        if (prev->y + prev->height < y)
        {
            break;
        }
        if (prev->x == rect.x && prev->width == rect.width && prev->y + prev->height == rect.y && egui_svg_rect_same_paint(prev, &rect))
        {
            prev->height += rect.height;
            return;
        }
    }

    rects[*rect_count] = rect;
    (*rect_count)++;
}

static __EGUI_UNUSED_ATTR__ int egui_svg_flatten_rects(const egui_svg_rect_t *src_rects, uint16_t src_count, egui_svg_rect_t **out_rects, uint16_t *out_count)
{
    float *x_edges;
    float *y_edges;
    egui_svg_rect_t *tmp_rects;
    egui_svg_rect_t *flat_rects;
    uint16_t edge_count;
    uint16_t x_count;
    uint16_t y_count;
    uint16_t flat_count = 0;
    size_t cell_count;

    if (src_rects == NULL || src_count == 0 || out_rects == NULL || out_count == NULL || src_count > (UINT16_MAX / 2u))
    {
        return 0;
    }

    edge_count = (uint16_t)(src_count * 2u);
    x_edges = (float *)egui_malloc(NULL, (int)((size_t)edge_count * sizeof(float)));
    if (x_edges == NULL)
    {
        return 0;
    }
    y_edges = (float *)egui_malloc(NULL, (int)((size_t)edge_count * sizeof(float)));
    if (y_edges == NULL)
    {
        egui_free(NULL, x_edges);
        return 0;
    }

    for (uint16_t i = 0; i < src_count; i++)
    {
        x_edges[i * 2u] = src_rects[i].x;
        x_edges[i * 2u + 1u] = src_rects[i].x + src_rects[i].width;
        y_edges[i * 2u] = src_rects[i].y;
        y_edges[i * 2u + 1u] = src_rects[i].y + src_rects[i].height;
    }

    egui_svg_sort_float_edges(x_edges, edge_count);
    egui_svg_sort_float_edges(y_edges, edge_count);
    x_count = egui_svg_unique_float_edges(x_edges, edge_count);
    y_count = egui_svg_unique_float_edges(y_edges, edge_count);
    if (x_count < 2 || y_count < 2)
    {
        egui_free(NULL, y_edges);
        egui_free(NULL, x_edges);
        return 0;
    }

    cell_count = (size_t)(x_count - 1u) * (size_t)(y_count - 1u);
    if (cell_count == 0u || cell_count > EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_MAX_CELLS || cell_count > (size_t)UINT16_MAX ||
        cell_count > ((size_t)INT_MAX / sizeof(*tmp_rects)))
    {
        egui_free(NULL, y_edges);
        egui_free(NULL, x_edges);
        return 0;
    }

    tmp_rects = (egui_svg_rect_t *)egui_malloc(NULL, (int)(cell_count * sizeof(*tmp_rects)));
    if (tmp_rects == NULL)
    {
        egui_free(NULL, y_edges);
        egui_free(NULL, x_edges);
        return 0;
    }

    for (uint16_t yi = 0; yi + 1u < y_count; yi++)
    {
        int has_span = 0;
        float span_left = 0.0f;
        float span_right = 0.0f;
        egui_color_t span_color = EGUI_COLOR_BLACK;

        for (uint16_t xi = 0; xi + 1u < x_count; xi++)
        {
            egui_color_t cell_color;
            float x0 = x_edges[xi];
            float x1 = x_edges[xi + 1u];
            float y0 = y_edges[yi];
            float y1 = y_edges[yi + 1u];
            float cx = (x0 + x1) * 0.5f;
            float cy = (y0 + y1) * 0.5f;
            int painted = egui_svg_rect_color_at(src_rects, src_count, cx, cy, &cell_color);

            if (painted && has_span && span_right == x0 && cell_color.full == span_color.full)
            {
                span_right = x1;
                continue;
            }
            if (has_span)
            {
                egui_svg_flatten_append_rect(tmp_rects, &flat_count, span_left, y0, span_right - span_left, y1 - y0, span_color);
            }
            has_span = painted;
            if (painted)
            {
                span_left = x0;
                span_right = x1;
                span_color = cell_color;
            }
        }

        if (has_span)
        {
            float y0 = y_edges[yi];
            float y1 = y_edges[yi + 1u];

            egui_svg_flatten_append_rect(tmp_rects, &flat_count, span_left, y0, span_right - span_left, y1 - y0, span_color);
        }
    }

    egui_free(NULL, y_edges);
    egui_free(NULL, x_edges);
    if (flat_count == 0)
    {
        egui_free(NULL, tmp_rects);
        return 0;
    }

    flat_rects = (egui_svg_rect_t *)egui_malloc(NULL, (int)((size_t)flat_count * sizeof(*flat_rects)));
    if (flat_rects == NULL)
    {
        egui_free(NULL, tmp_rects);
        return 0;
    }
    memcpy(flat_rects, tmp_rects, (size_t)flat_count * sizeof(*flat_rects));
    egui_free(NULL, tmp_rects);

    *out_rects = flat_rects;
    *out_count = flat_count;
    return 1;
}

static int egui_svg_set_rect_fast_path_doc(egui_svg_doc_t *doc, egui_svg_rect_t *rects, uint16_t rect_count, float view_x, float view_y, float view_width,
                                           float view_height)
{
    egui_dim_t doc_view_x;
    egui_dim_t doc_view_y;
    egui_dim_t doc_view_width;
    egui_dim_t doc_view_height;

    if (doc == NULL || rects == NULL || rect_count == 0)
    {
        return 0;
    }
    if (!egui_svg_dim_from_integral_float(view_x, &doc_view_x) || !egui_svg_dim_from_integral_float(view_y, &doc_view_y) ||
        !egui_svg_dim_from_integral_float(view_width, &doc_view_width) || !egui_svg_dim_from_integral_float(view_height, &doc_view_height) ||
        doc_view_width <= 0 || doc_view_height <= 0)
    {
        return 0;
    }

#if EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_FLATTEN
    {
        egui_svg_rect_t *flat_rects = NULL;
        uint16_t flat_count = 0;

        if (egui_svg_flatten_rects(rects, rect_count, &flat_rects, &flat_count))
        {
            if (doc->state.rect.storage_embedded)
            {
                if (flat_count <= rect_count)
                {
                    memcpy(rects, flat_rects, (size_t)flat_count * sizeof(*rects));
                    rect_count = flat_count;
                    doc->state.rect.rects_sorted = 1u;
                }
                egui_free(NULL, flat_rects);
            }
            else
            {
                egui_free(NULL, rects);
                rects = flat_rects;
                rect_count = flat_count;
                doc->state.rect.rects_sorted = 1u;
            }
        }
    }
#endif

    doc->state.rect.rects = rects;
    doc->state.rect.rect_count = rect_count;
    doc->rect_fast_path = 1u;
    doc->state.rect.view_x = doc_view_x;
    doc->state.rect.view_y = doc_view_y;
    doc->state.rect.view_width = doc_view_width;
    doc->state.rect.view_height = doc_view_height;
    egui_svg_get_rect_content_bounds(rects, rect_count, &doc->state.rect.rect_content_left, &doc->state.rect.rect_content_top,
                                     &doc->state.rect.rect_content_right, &doc->state.rect.rect_content_bottom);
    return 1;
}

static int egui_svg_try_build_rect_fast_path(egui_svg_doc_t *doc, const char *svg_text, uint32_t svg_len)
{
    const char *svg_end;
    egui_svg_rect_t *rects;
    uint16_t rect_count = 0;
    float view_x = 0.0f;
    float view_y = 0.0f;
    float view_width;
    float view_height;
    size_t rect_bytes;

    if (doc == NULL || svg_text == NULL || svg_len == 0)
    {
        return 0;
    }

    svg_end = svg_text + svg_len;
    view_width = (float)doc->natural_width;
    view_height = (float)doc->natural_height;
    if (!egui_svg_scan_rect_fast_path(svg_text, svg_end, NULL, &rect_count, &view_x, &view_y, &view_width, &view_height, NULL, NULL))
    {
        return 0;
    }
    rect_bytes = (size_t)rect_count * sizeof(*rects);
    if (rect_bytes == 0u || rect_bytes > (size_t)INT_MAX)
    {
        return 0;
    }

    rects = (egui_svg_rect_t *)egui_malloc(NULL, (int)rect_bytes);
    if (rects == NULL)
    {
        return 0;
    }

    view_x = 0.0f;
    view_y = 0.0f;
    view_width = (float)doc->natural_width;
    view_height = (float)doc->natural_height;
    if (!egui_svg_scan_rect_fast_path(svg_text, svg_end, rects, &rect_count, &view_x, &view_y, &view_width, &view_height, NULL, NULL))
    {
        egui_free(NULL, rects);
        return 0;
    }
    if (!egui_svg_set_rect_fast_path_doc(doc, rects, rect_count, view_x, view_y, view_width, view_height))
    {
        egui_free(NULL, rects);
        return 0;
    }
    return 1;
}

static int egui_svg_get_visible_rect(egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, int32_t *left, int32_t *top,
                                     int32_t *right, int32_t *bottom)
{
    egui_region_t *work_region;
    int32_t image_left;
    int32_t image_top;
    int32_t image_right;
    int32_t image_bottom;
    int32_t clip_left;
    int32_t clip_top;
    int32_t clip_right;
    int32_t clip_bottom;

    if (canvas == NULL || left == NULL || top == NULL || right == NULL || bottom == NULL || width <= 0 || height <= 0)
    {
        return 0;
    }

    work_region = egui_canvas_get_base_view_work_region(canvas);
    if (work_region == NULL || egui_region_is_empty(work_region))
    {
        return 0;
    }

    image_left = x;
    image_top = y;
    image_right = image_left + width;
    image_bottom = image_top + height;
    clip_left = EGUI_MAX(image_left, (int32_t)work_region->location.x);
    clip_top = EGUI_MAX(image_top, (int32_t)work_region->location.y);
    clip_right = EGUI_MIN(image_right, (int32_t)work_region->location.x + work_region->size.width);
    clip_bottom = EGUI_MIN(image_bottom, (int32_t)work_region->location.y + work_region->size.height);

    if (clip_left >= clip_right || clip_top >= clip_bottom)
    {
        return 0;
    }

    *left = clip_left;
    *top = clip_top;
    *right = clip_right;
    *bottom = clip_bottom;
    return 1;
}

static plutovg_surface_t *egui_svg_render_surface_region(const egui_svg_doc_t *doc, egui_dim_t target_width, egui_dim_t target_height, int32_t clip_x,
                                                         int32_t clip_y, int32_t clip_width, int32_t clip_height)
{
    plutovg_rect_t extents;
    plutovg_surface_t *surface;
    plutovg_canvas_t *svg_canvas;

    if (doc == NULL || doc->rect_fast_path || doc->state.raster.document == NULL || target_width <= 0 || target_height <= 0 || clip_x < 0 || clip_y < 0 ||
        clip_width <= 0 || clip_height <= 0 || clip_width > EGUI_DIM_MAX || clip_height > EGUI_DIM_MAX)
    {
        return NULL;
    }

    extents.x = 0.0f;
    extents.y = 0.0f;
    extents.w = plutosvg_document_get_width(doc->state.raster.document);
    extents.h = plutosvg_document_get_height(doc->state.raster.document);
    if (extents.w <= 0.0f || extents.h <= 0.0f)
    {
        return NULL;
    }

    surface = plutovg_surface_create((int)clip_width, (int)clip_height);
    if (surface == NULL)
    {
        return NULL;
    }

    svg_canvas = plutovg_canvas_create(surface);
    if (svg_canvas == NULL)
    {
        plutovg_surface_destroy(surface);
        return NULL;
    }

    plutovg_canvas_translate(svg_canvas, -(float)clip_x, -(float)clip_y);
    plutovg_canvas_scale(svg_canvas, (float)target_width / extents.w, (float)target_height / extents.h);
    plutovg_canvas_translate(svg_canvas, -extents.x, -extents.y);
    if (!plutosvg_document_render(doc->state.raster.document, NULL, svg_canvas, NULL, NULL, NULL))
    {
        plutovg_canvas_destroy(svg_canvas);
        plutovg_surface_destroy(surface);
        return NULL;
    }

    plutovg_canvas_destroy(svg_canvas);
    return surface;
}

static int egui_svg_query_surface_pixel(const egui_svg_doc_t *doc, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color,
                                        egui_alpha_t *alpha)
{
    plutovg_surface_t *surface;
    unsigned char *surface_data;
    int stride;
    uint32_t pixel;
    uint16_t rgb565;
    egui_alpha_t pixel_alpha;

    if (doc == NULL || x < 0 || y < 0 || width <= 0 || height <= 0 || x >= width || y >= height)
    {
        return 0;
    }

    surface = egui_svg_render_surface_region(doc, width, height, x, y, 1, 1);
    if (surface == NULL)
    {
        return 0;
    }
    plutosvg_document_release_cache(doc->state.raster.document);

    surface_data = plutovg_surface_get_data(surface);
    stride = plutovg_surface_get_stride(surface);
    if (surface_data == NULL || stride < 4)
    {
        plutovg_surface_destroy(surface);
        return 0;
    }

    pixel = *((const uint32_t *)surface_data);
    rgb565 = egui_svg_argb32_to_rgb565(pixel, &pixel_alpha);
    if (color != NULL)
    {
        color->full = EGUI_COLOR_RGB565_TRANS(rgb565);
    }
    if (alpha != NULL)
    {
        *alpha = pixel_alpha;
    }

    plutovg_surface_destroy(surface);
    return 1;
}

static int32_t egui_svg_floor_to_i32(float value)
{
    int32_t result = (int32_t)value;

    if ((float)result > value)
    {
        result--;
    }
    return result;
}

static int32_t egui_svg_ceil_to_i32(float value)
{
    int32_t result = (int32_t)value;

    if ((float)result < value)
    {
        result++;
    }
    return result;
}

static int egui_svg_get_content_bounds(const egui_svg_doc_t *doc, egui_dim_t width, egui_dim_t height, egui_dim_t *offset_x, egui_dim_t *offset_y,
                                       egui_dim_t *crop_width, egui_dim_t *crop_height)
{
    float left;
    float top;
    float right;
    float bottom;
    int32_t x0;
    int32_t y0;
    int32_t x1;
    int32_t y1;

    if (doc == NULL || offset_x == NULL || offset_y == NULL || crop_width == NULL || crop_height == NULL || width <= 0 || height <= 0)
    {
        return 0;
    }

    if (doc->rect_fast_path)
    {
        if (doc->state.rect.rect_count == 0 || doc->state.rect.view_width <= 0.0f || doc->state.rect.view_height <= 0.0f)
        {
            return 0;
        }

        left = doc->state.rect.rect_content_left;
        top = doc->state.rect.rect_content_top;
        right = doc->state.rect.rect_content_right;
        bottom = doc->state.rect.rect_content_bottom;
        x0 = egui_svg_floor_to_i32((left - doc->state.rect.view_x) * (float)width / doc->state.rect.view_width);
        y0 = egui_svg_floor_to_i32((top - doc->state.rect.view_y) * (float)height / doc->state.rect.view_height);
        x1 = egui_svg_ceil_to_i32((right - doc->state.rect.view_x) * (float)width / doc->state.rect.view_width);
        y1 = egui_svg_ceil_to_i32((bottom - doc->state.rect.view_y) * (float)height / doc->state.rect.view_height);
    }
    else
    {
        plutovg_rect_t extents;

        if (doc->state.raster.document == NULL)
        {
            return 0;
        }

        float doc_width = plutosvg_document_get_width(doc->state.raster.document);
        float doc_height = plutosvg_document_get_height(doc->state.raster.document);

        if (doc_width <= 0.0f || doc_height <= 0.0f || !plutosvg_document_extents(doc->state.raster.document, NULL, &extents) || extents.w <= 0.0f ||
            extents.h <= 0.0f)
        {
            return 0;
        }

        x0 = egui_svg_floor_to_i32((extents.x * (float)width) / doc_width) - 1;
        y0 = egui_svg_floor_to_i32((extents.y * (float)height) / doc_height) - 1;
        x1 = egui_svg_ceil_to_i32(((extents.x + extents.w) * (float)width) / doc_width) + 1;
        y1 = egui_svg_ceil_to_i32(((extents.y + extents.h) * (float)height) / doc_height) + 1;
    }

    if (x0 < 0)
    {
        x0 = 0;
    }
    if (y0 < 0)
    {
        y0 = 0;
    }
    if (x1 > width)
    {
        x1 = width;
    }
    if (y1 > height)
    {
        y1 = height;
    }
    if (x0 >= x1 || y0 >= y1 || x1 - x0 > EGUI_DIM_MAX || y1 - y0 > EGUI_DIM_MAX)
    {
        return 0;
    }

    *offset_x = (egui_dim_t)x0;
    *offset_y = (egui_dim_t)y0;
    *crop_width = (egui_dim_t)(x1 - x0);
    *crop_height = (egui_dim_t)(y1 - y0);
    return 1;
}

static int egui_svg_build_cache_from_surface(plutovg_surface_t *surface, egui_image_std_info_t *info, uint16_t **out_data_buf, uint8_t **out_alpha_buf)
{
    unsigned char *surface_data;
    int surface_width;
    int surface_height;
    int stride;
    size_t pixel_count;
    size_t data_bytes;
    size_t alpha_bytes;
    uint16_t *data_buf;
    uint8_t *alpha_buf = NULL;
    int has_alpha = 0;

    if (surface == NULL || info == NULL || out_data_buf == NULL || out_alpha_buf == NULL)
    {
        return 0;
    }

    surface_data = plutovg_surface_get_data(surface);
    surface_width = plutovg_surface_get_width(surface);
    surface_height = plutovg_surface_get_height(surface);
    stride = plutovg_surface_get_stride(surface);
    if (surface_data == NULL || surface_width <= 0 || surface_height <= 0 || surface_width > EGUI_DIM_MAX || surface_height > EGUI_DIM_MAX ||
        stride < surface_width * 4)
    {
        return 0;
    }

    pixel_count = (size_t)surface_width * (size_t)surface_height;
    data_bytes = pixel_count * sizeof(uint16_t);
    alpha_bytes = pixel_count * sizeof(uint8_t);
    if (data_bytes > (size_t)INT_MAX || alpha_bytes > (size_t)INT_MAX)
    {
        return 0;
    }

    for (int row = 0; row < surface_height && !has_alpha; row++)
    {
        const uint32_t *src_row = (const uint32_t *)(surface_data + (size_t)row * (size_t)stride);

        for (int col = 0; col < surface_width; col++)
        {
            if (((src_row[col] >> 24) & 0xFFu) != EGUI_ALPHA_100)
            {
                has_alpha = 1;
                break;
            }
        }
    }

    if (!egui_svg_bytes_fit_limit(data_bytes + (has_alpha ? alpha_bytes : 0u), EGUI_CONFIG_IMAGE_SVG_CACHE_MAX_BYTES))
    {
        return 0;
    }

    data_buf = (uint16_t *)egui_malloc(NULL, (int)data_bytes);
    if (data_buf == NULL)
    {
        return 0;
    }
    if (has_alpha)
    {
        alpha_buf = (uint8_t *)egui_malloc(NULL, (int)alpha_bytes);
        if (alpha_buf == NULL)
        {
            egui_free(NULL, data_buf);
            return 0;
        }
    }

    for (int row = 0; row < surface_height; row++)
    {
        const uint32_t *src_row = (const uint32_t *)(surface_data + (size_t)row * (size_t)stride);
        uint16_t *dst_row = data_buf + (size_t)row * (size_t)surface_width;
        uint8_t *alpha_row = (alpha_buf != NULL) ? (alpha_buf + (size_t)row * (size_t)surface_width) : NULL;

        for (int col = 0; col < surface_width; col++)
        {
            egui_alpha_t pixel_alpha;

            dst_row[col] = egui_svg_argb32_to_rgb565(src_row[col], &pixel_alpha);
            if (alpha_row != NULL)
            {
                alpha_row[col] = pixel_alpha;
            }
        }
    }

    egui_api_memset(info, 0, sizeof(*info));
    info->data_buf = data_buf;
    info->alpha_buf = alpha_buf;
    info->data_type = EGUI_IMAGE_DATA_TYPE_RGB565;
    info->alpha_type = EGUI_IMAGE_ALPHA_TYPE_8;
    info->res_type = EGUI_RESOURCE_TYPE_INTERNAL;
    info->width = (uint16_t)surface_width;
    info->height = (uint16_t)surface_height;
    *out_data_buf = data_buf;
    *out_alpha_buf = alpha_buf;
    return 1;
}

static int egui_svg_render_cache(egui_svg_doc_t *doc, egui_dim_t width, egui_dim_t height)
{
    egui_dim_t offset_x;
    egui_dim_t offset_y;
    egui_dim_t crop_width;
    egui_dim_t crop_height;
    plutovg_surface_t *surface;
    egui_image_std_info_t new_info;
    uint16_t *new_data_buf;
    uint8_t *new_alpha_buf;

    if (doc == NULL || doc->rect_fast_path || doc->state.raster.document == NULL || width <= 0 || height <= 0)
    {
        return 0;
    }
    if (egui_svg_cache_matches(doc, width, height))
    {
        return 1;
    }
    if (!egui_svg_get_content_bounds(doc, width, height, &offset_x, &offset_y, &crop_width, &crop_height))
    {
        return 0;
    }
    if (crop_width >= width && crop_height >= height)
    {
        return 0;
    }
    if (!egui_svg_cache_region_allowed(crop_width, crop_height))
    {
        return 0;
    }

    surface = egui_svg_render_surface_region(doc, width, height, offset_x, offset_y, crop_width, crop_height);
    if (surface == NULL)
    {
        return 0;
    }
    plutosvg_document_release_cache(doc->state.raster.document);

    if (!egui_svg_build_cache_from_surface(surface, &new_info, &new_data_buf, &new_alpha_buf))
    {
        plutovg_surface_destroy(surface);
        return 0;
    }

    plutovg_surface_destroy(surface);
    egui_svg_release_cache(doc);
    doc->state.raster.data_buf = new_data_buf;
    doc->state.raster.alpha_buf = new_alpha_buf;
    doc->state.raster.raster_info = new_info;
    doc->state.raster.cache_width = width;
    doc->state.raster.cache_height = height;
    doc->state.raster.cache_offset_x = offset_x;
    doc->state.raster.cache_offset_y = offset_y;
    egui_image_std_init(&doc->state.raster.raster_image.base, &doc->state.raster.raster_info);
    return 1;
}

static int egui_svg_cached_raster_query_pixel(const egui_svg_doc_t *doc, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    if (doc == NULL || doc->rect_fast_path || doc->state.raster.raster_info.data_buf == NULL)
    {
        return 0;
    }
    if (x < doc->state.raster.cache_offset_x || y < doc->state.raster.cache_offset_y)
    {
        return 0;
    }

    x -= doc->state.raster.cache_offset_x;
    y -= doc->state.raster.cache_offset_y;
    if (x < 0 || y < 0 || x >= doc->state.raster.raster_info.width || y >= doc->state.raster.raster_info.height)
    {
        return 0;
    }

    return doc->state.raster.raster_image.base.api->get_point(&doc->state.raster.raster_image.base, x, y, color, alpha);
}

static void egui_svg_cached_raster_draw(const egui_svg_doc_t *doc, egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y)
{
    if (doc == NULL || doc->rect_fast_path || doc->state.raster.raster_info.data_buf == NULL || canvas == NULL)
    {
        return;
    }
    doc->state.raster.raster_image.base.api->draw_image(&doc->state.raster.raster_image.base, canvas, x + doc->state.raster.cache_offset_x,
                                                        y + doc->state.raster.cache_offset_y);
}

static int egui_svg_query_rect_fast_path(const egui_svg_doc_t *doc, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color,
                                         egui_alpha_t *alpha)
{
    float svg_x;
    float svg_y;
    int found = 0;

    if (doc == NULL || !doc->rect_fast_path || width <= 0 || height <= 0 || x < 0 || y < 0 || x >= width || y >= height)
    {
        return 0;
    }

    svg_x = doc->state.rect.view_x + (((float)x + 0.5f) * doc->state.rect.view_width / (float)width);
    svg_y = doc->state.rect.view_y + (((float)y + 0.5f) * doc->state.rect.view_height / (float)height);

    if (color != NULL)
    {
        *color = EGUI_COLOR_BLACK;
    }
    if (alpha != NULL)
    {
        *alpha = 0;
    }

    for (uint16_t i = 0; i < doc->state.rect.rect_count; i++)
    {
        const egui_svg_rect_t *rect = &doc->state.rect.rects[i];

        if (svg_x >= rect->x && svg_x < rect->x + rect->width && svg_y >= rect->y && svg_y < rect->y + rect->height)
        {
            if (color != NULL)
            {
                *color = rect->color;
            }
            if (alpha != NULL)
            {
                *alpha = EGUI_ALPHA_100;
            }
            found = 1;
        }
    }

    return found;
}

static void egui_svg_draw_rect_clipped(egui_canvas_t *canvas, egui_region_t *work_region, int32_t rect_left, int32_t rect_top, int32_t rect_right,
                                       int32_t rect_bottom, egui_color_t color)
{
    int32_t clip_left;
    int32_t clip_top;
    int32_t clip_right;
    int32_t clip_bottom;

    if (canvas == NULL || rect_right <= rect_left || rect_bottom <= rect_top)
    {
        return;
    }

    if (work_region == NULL || egui_region_is_empty(work_region))
    {
        return;
    }

    clip_left = EGUI_MAX(rect_left, (int32_t)work_region->location.x);
    clip_top = EGUI_MAX(rect_top, (int32_t)work_region->location.y);
    clip_right = EGUI_MIN(rect_right, (int32_t)work_region->location.x + work_region->size.width);
    clip_bottom = EGUI_MIN(rect_bottom, (int32_t)work_region->location.y + work_region->size.height);
    if (clip_left >= clip_right || clip_top >= clip_bottom)
    {
        return;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
    if (canvas->mask == NULL)
    {
        egui_alpha_t alpha = canvas->alpha;
        egui_dim_t pfb_width = canvas->pfb_region.size.width;
        egui_dim_t pfb_x = (egui_dim_t)clip_left - canvas->pfb_location_in_base_view.x;
        egui_dim_t pfb_y = (egui_dim_t)clip_top - canvas->pfb_location_in_base_view.y;
        egui_dim_t fill_width = (egui_dim_t)(clip_right - clip_left);
        egui_dim_t fill_height = (egui_dim_t)(clip_bottom - clip_top);

        if (alpha == 0)
        {
            return;
        }

        for (egui_dim_t row = 0; row < fill_height; row++)
        {
            egui_color_int_t *dst = &canvas->pfb[(size_t)(pfb_y + row) * (size_t)pfb_width + (size_t)pfb_x];

            if (alpha == EGUI_ALPHA_100)
            {
                egui_canvas_fill_color_buffer(dst, fill_width, color);
            }
            else
            {
                egui_canvas_blend_color_buffer_alpha(dst, fill_width, color, alpha);
            }
        }
        return;
    }
#endif

    egui_canvas_draw_rectangle_fill(canvas, (egui_dim_t)clip_left, (egui_dim_t)clip_top, (egui_dim_t)(clip_right - clip_left),
                                    (egui_dim_t)(clip_bottom - clip_top), color, EGUI_ALPHA_100);
}

#if EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_PIXEL_CACHE
static int egui_svg_pixel_rect_cache_matches(const egui_svg_doc_t *doc, egui_dim_t width, egui_dim_t height)
{
    return doc != NULL && doc->state.rect.pixel_rects != NULL && doc->state.rect.pixel_rect_width == width && doc->state.rect.pixel_rect_height == height;
}

static int egui_svg_ensure_pixel_rect_cache(egui_svg_doc_t *doc, egui_dim_t width, egui_dim_t height)
{
    egui_svg_pixel_rect_t *pixel_rects;
    uint16_t pixel_rect_count = 0;
    egui_dim_t content_left = 0;
    egui_dim_t content_top = 0;
    egui_dim_t content_right = 0;
    egui_dim_t content_bottom = 0;
    float scale_x;
    float scale_y;

    if (doc == NULL || !doc->rect_fast_path || doc->state.rect.rect_count == 0 || width <= 0 || height <= 0 || doc->state.rect.view_width <= 0.0f ||
        doc->state.rect.view_height <= 0.0f)
    {
        return 0;
    }
    if (egui_svg_pixel_rect_cache_matches(doc, width, height))
    {
        return 1;
    }

    if (doc->state.rect.storage_embedded)
    {
        pixel_rects = doc->state.rect.pixel_rects;
    }
    else
    {
        size_t bytes = (size_t)doc->state.rect.rect_count * sizeof(*pixel_rects);

        if (bytes == 0u || bytes > (size_t)INT_MAX)
        {
            return 0;
        }
        pixel_rects = (egui_svg_pixel_rect_t *)egui_malloc(NULL, (int)bytes);
    }
    if (pixel_rects == NULL)
    {
        return 0;
    }

    scale_x = (float)width / doc->state.rect.view_width;
    scale_y = (float)height / doc->state.rect.view_height;
    for (uint16_t i = 0; i < doc->state.rect.rect_count; i++)
    {
        const egui_svg_rect_t *rect = &doc->state.rect.rects[i];
        int32_t left = egui_svg_floor_to_i32((rect->x - doc->state.rect.view_x) * scale_x);
        int32_t top = egui_svg_floor_to_i32((rect->y - doc->state.rect.view_y) * scale_y);
        int32_t right = egui_svg_ceil_to_i32((rect->x + rect->width - doc->state.rect.view_x) * scale_x);
        int32_t bottom = egui_svg_ceil_to_i32((rect->y + rect->height - doc->state.rect.view_y) * scale_y);
        egui_svg_pixel_rect_t *pixel_rect;

        if (left < 0)
        {
            left = 0;
        }
        if (top < 0)
        {
            top = 0;
        }
        if (right > width)
        {
            right = width;
        }
        if (bottom > height)
        {
            bottom = height;
        }
        if (left >= right || top >= bottom)
        {
            continue;
        }

        pixel_rect = &pixel_rects[pixel_rect_count];
        pixel_rect->left = (egui_dim_t)left;
        pixel_rect->top = (egui_dim_t)top;
        pixel_rect->right = (egui_dim_t)right;
        pixel_rect->bottom = (egui_dim_t)bottom;
        pixel_rect->color = rect->color;
        if (pixel_rect_count == 0)
        {
            content_left = pixel_rect->left;
            content_top = pixel_rect->top;
            content_right = pixel_rect->right;
            content_bottom = pixel_rect->bottom;
        }
        else
        {
            content_left = EGUI_MIN(content_left, pixel_rect->left);
            content_top = EGUI_MIN(content_top, pixel_rect->top);
            content_right = EGUI_MAX(content_right, pixel_rect->right);
            content_bottom = EGUI_MAX(content_bottom, pixel_rect->bottom);
        }
        pixel_rect_count++;
    }

    if (pixel_rect_count == 0)
    {
        if (!doc->state.rect.storage_embedded)
        {
            egui_free(NULL, pixel_rects);
        }
        egui_svg_release_pixel_rect_cache(doc);
        return 0;
    }

    egui_svg_release_pixel_rect_cache(doc);
    doc->state.rect.pixel_rects = pixel_rects;
    doc->state.rect.pixel_rect_count = pixel_rect_count;
    doc->state.rect.pixel_rect_width = width;
    doc->state.rect.pixel_rect_height = height;
    doc->state.rect.pixel_content_left = content_left;
    doc->state.rect.pixel_content_top = content_top;
    doc->state.rect.pixel_content_right = content_right;
    doc->state.rect.pixel_content_bottom = content_bottom;
    return 1;
}

static void egui_svg_draw_pixel_rect_fast_path(const egui_svg_doc_t *doc, egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y)
{
    egui_region_t *work_region;
    int32_t content_left;
    int32_t content_top;
    int32_t content_right;
    int32_t content_bottom;
    int32_t work_left;
    int32_t work_top;
    int32_t work_right;
    int32_t work_bottom;

    if (doc == NULL || doc->state.rect.pixel_rects == NULL || canvas == NULL)
    {
        return;
    }

    work_region = egui_canvas_get_base_view_work_region(canvas);
    if (work_region == NULL || egui_region_is_empty(work_region))
    {
        return;
    }

    work_left = (int32_t)work_region->location.x;
    work_top = (int32_t)work_region->location.y;
    work_right = work_left + work_region->size.width;
    work_bottom = work_top + work_region->size.height;
    content_left = (int32_t)x + doc->state.rect.pixel_content_left;
    content_top = (int32_t)y + doc->state.rect.pixel_content_top;
    content_right = (int32_t)x + doc->state.rect.pixel_content_right;
    content_bottom = (int32_t)y + doc->state.rect.pixel_content_bottom;
    if (content_left >= work_right || content_top >= work_bottom || content_right <= work_left || content_bottom <= work_top)
    {
        return;
    }

    for (uint16_t i = 0; i < doc->state.rect.pixel_rect_count; i++)
    {
        const egui_svg_pixel_rect_t *rect = &doc->state.rect.pixel_rects[i];
        int32_t rect_left = (int32_t)x + rect->left;
        int32_t rect_top = (int32_t)y + rect->top;
        int32_t rect_right = (int32_t)x + rect->right;
        int32_t rect_bottom = (int32_t)y + rect->bottom;

        if (rect_bottom <= work_top)
        {
            continue;
        }
        if (doc->state.rect.rects_sorted && rect_top >= work_bottom)
        {
            break;
        }
        if (rect_left >= work_right || rect_top >= work_bottom || rect_right <= work_left || rect_bottom <= work_top)
        {
            continue;
        }
        egui_svg_draw_rect_clipped(canvas, work_region, rect_left, rect_top, rect_right, rect_bottom, rect->color);
    }
}
#endif

static void egui_svg_draw_rect_float_fast_path(const egui_svg_doc_t *doc, egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t width,
                                               egui_dim_t height)
{
    egui_region_t *work_region;
    float scale_x;
    float scale_y;
    int32_t content_left;
    int32_t content_top;
    int32_t content_right;
    int32_t content_bottom;
    int32_t work_left;
    int32_t work_top;
    int32_t work_right;
    int32_t work_bottom;

    if (doc == NULL || !doc->rect_fast_path || canvas == NULL || width <= 0 || height <= 0 || doc->state.rect.view_width <= 0.0f ||
        doc->state.rect.view_height <= 0.0f)
    {
        return;
    }

    work_region = egui_canvas_get_base_view_work_region(canvas);
    if (work_region == NULL || egui_region_is_empty(work_region))
    {
        return;
    }

    scale_x = (float)width / doc->state.rect.view_width;
    scale_y = (float)height / doc->state.rect.view_height;
    work_left = (int32_t)work_region->location.x;
    work_top = (int32_t)work_region->location.y;
    work_right = work_left + work_region->size.width;
    work_bottom = work_top + work_region->size.height;
    content_left = (int32_t)x + egui_svg_floor_to_i32((doc->state.rect.rect_content_left - doc->state.rect.view_x) * scale_x);
    content_top = (int32_t)y + egui_svg_floor_to_i32((doc->state.rect.rect_content_top - doc->state.rect.view_y) * scale_y);
    content_right = (int32_t)x + egui_svg_ceil_to_i32((doc->state.rect.rect_content_right - doc->state.rect.view_x) * scale_x);
    content_bottom = (int32_t)y + egui_svg_ceil_to_i32((doc->state.rect.rect_content_bottom - doc->state.rect.view_y) * scale_y);
    if (content_left >= work_right || content_top >= work_bottom || content_right <= work_left || content_bottom <= work_top)
    {
        return;
    }

    for (uint16_t i = 0; i < doc->state.rect.rect_count; i++)
    {
        const egui_svg_rect_t *rect = &doc->state.rect.rects[i];
        int32_t rect_left = (int32_t)x + egui_svg_floor_to_i32((rect->x - doc->state.rect.view_x) * scale_x);
        int32_t rect_top = (int32_t)y + egui_svg_floor_to_i32((rect->y - doc->state.rect.view_y) * scale_y);
        int32_t rect_right = (int32_t)x + egui_svg_ceil_to_i32((rect->x + rect->width - doc->state.rect.view_x) * scale_x);
        int32_t rect_bottom = (int32_t)y + egui_svg_ceil_to_i32((rect->y + rect->height - doc->state.rect.view_y) * scale_y);

        if (rect_bottom <= work_top)
        {
            continue;
        }
        if (doc->state.rect.rects_sorted && rect_top >= work_bottom)
        {
            break;
        }
        if (rect_left >= work_right || rect_top >= work_bottom || rect_right <= work_left || rect_bottom <= work_top)
        {
            continue;
        }
        egui_svg_draw_rect_clipped(canvas, work_region, rect_left, rect_top, rect_right, rect_bottom, rect->color);
    }
}

static void egui_svg_draw_rect_fast_path(egui_svg_doc_t *doc, egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
#if EGUI_CONFIG_IMAGE_SVG_RECT_FAST_PATH_PIXEL_CACHE
    if (egui_svg_ensure_pixel_rect_cache(doc, width, height))
    {
        egui_svg_draw_pixel_rect_fast_path(doc, canvas, x, y);
        return;
    }
#endif

    egui_svg_draw_rect_float_fast_path(doc, canvas, x, y, width, height);
}

static void egui_svg_blend_surface_to_canvas(egui_canvas_t *canvas, plutovg_surface_t *surface, int32_t screen_x, int32_t screen_y)
{
    unsigned char *surface_data;
    int surface_width;
    int surface_height;
    int stride;

    if (canvas == NULL || canvas->pfb == NULL || surface == NULL)
    {
        return;
    }

    surface_data = plutovg_surface_get_data(surface);
    surface_width = plutovg_surface_get_width(surface);
    surface_height = plutovg_surface_get_height(surface);
    stride = plutovg_surface_get_stride(surface);
    if (surface_data == NULL || surface_width <= 0 || surface_height <= 0 || stride < surface_width * 4)
    {
        return;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
    if (canvas->mask == NULL)
    {
        egui_alpha_t canvas_alpha = canvas->alpha;

        for (int row = 0; row < surface_height; row++)
        {
            const uint32_t *src_row = (const uint32_t *)(surface_data + (size_t)row * (size_t)stride);
            int32_t pfb_x = screen_x - canvas->pfb_location_in_base_view.x;
            int32_t pfb_y = screen_y + row - canvas->pfb_location_in_base_view.y;
            egui_color_int_t *dst_row = &canvas->pfb[(size_t)pfb_y * (size_t)canvas->pfb_region.size.width + (size_t)pfb_x];

            for (int col = 0; col < surface_width; col++)
            {
                egui_alpha_t pixel_alpha;
                uint16_t rgb565 = egui_svg_argb32_to_rgb565(src_row[col], &pixel_alpha);

                if (pixel_alpha == 0)
                {
                    continue;
                }

                egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[col], rgb565, egui_color_alpha_mix(canvas_alpha, pixel_alpha));
            }
        }
        return;
    }
#endif

    for (int row = 0; row < surface_height; row++)
    {
        const uint32_t *src_row = (const uint32_t *)(surface_data + (size_t)row * (size_t)stride);

        for (int col = 0; col < surface_width; col++)
        {
            egui_alpha_t pixel_alpha;
            uint16_t rgb565 = egui_svg_argb32_to_rgb565(src_row[col], &pixel_alpha);
            egui_color_t color;

            if (pixel_alpha == 0)
            {
                continue;
            }

            color.full = EGUI_COLOR_RGB565_TRANS(rgb565);
            egui_canvas_draw_point_limit(canvas, (egui_dim_t)(screen_x + col), (egui_dim_t)(screen_y + row), color, pixel_alpha);
        }
    }
}

static void egui_svg_draw_clipped_region(const egui_svg_doc_t *doc, egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    int32_t clip_left;
    int32_t clip_top;
    int32_t clip_right;
    int32_t clip_bottom;
    int32_t visible_width;
    int32_t max_pixels;
    int32_t band_top;

    if (doc == NULL || doc->rect_fast_path || doc->state.raster.document == NULL || canvas == NULL || width <= 0 || height <= 0)
    {
        return;
    }

    if (!egui_svg_get_visible_rect(canvas, x, y, width, height, &clip_left, &clip_top, &clip_right, &clip_bottom))
    {
        return;
    }

    visible_width = clip_right - clip_left;
    max_pixels = egui_svg_render_max_surface_pixels();

    for (band_top = clip_top; band_top < clip_bottom;)
    {
        int32_t max_width_for_band = EGUI_MIN(visible_width, max_pixels);
        int32_t band_height = max_pixels / max_width_for_band;
        int32_t max_chunk_width;
        int32_t chunk_left;

        if (band_height <= 0)
        {
            band_height = 1;
        }
        if (band_height > clip_bottom - band_top)
        {
            band_height = clip_bottom - band_top;
        }

        max_chunk_width = max_pixels / band_height;
        if (max_chunk_width <= 0)
        {
            max_chunk_width = 1;
        }

        for (chunk_left = clip_left; chunk_left < clip_right;)
        {
            int32_t chunk_width = EGUI_MIN(max_chunk_width, clip_right - chunk_left);
            int32_t local_x = chunk_left - (int32_t)x;
            int32_t local_y = band_top - (int32_t)y;
            plutovg_surface_t *surface = egui_svg_render_surface_region(doc, width, height, local_x, local_y, chunk_width, band_height);

            if (surface != NULL)
            {
                egui_svg_blend_surface_to_canvas(canvas, surface, chunk_left, band_top);
                plutovg_surface_destroy(surface);
            }

            chunk_left += chunk_width;
        }

        band_top += band_height;
    }
    plutosvg_document_release_cache(doc->state.raster.document);
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

static egui_svg_doc_t *egui_svg_try_create_rect_fast_path_doc(const char *svg_text, uint32_t svg_len)
{
    egui_svg_doc_t *doc;
    egui_svg_rect_t *rects;
    egui_dim_t natural_width;
    egui_dim_t natural_height;
    uint16_t rect_count = 0;
    float view_x = 0.0f;
    float view_y = 0.0f;
    float view_width;
    float view_height;

    if (svg_text == NULL || svg_len == 0)
    {
        return NULL;
    }

    if (!egui_svg_try_get_rect_fast_path_size(svg_text, svg_len, &natural_width, &natural_height, &rect_count))
    {
        return NULL;
    }

    doc = egui_svg_alloc_rect_fast_path_doc(rect_count, &rects);
    if (doc == NULL)
    {
        return NULL;
    }
    doc->natural_width = natural_width;
    doc->natural_height = natural_height;

    view_width = (float)doc->natural_width;
    view_height = (float)doc->natural_height;
    rect_count = 0;
    if (!egui_svg_scan_rect_fast_path(svg_text, svg_text + svg_len, rects, &rect_count, &view_x, &view_y, &view_width, &view_height, NULL, NULL) ||
        !egui_svg_set_rect_fast_path_doc(doc, rects, rect_count, view_x, view_y, view_width, view_height))
    {
        egui_svg_doc_destroy(doc);
        return NULL;
    }

    return doc;
}

static egui_svg_doc_t *egui_svg_try_create_external_rect_fast_path_doc(egui_core_t *core, const egui_svg_source_t *res)
{
    egui_svg_doc_t *doc;
    egui_svg_rect_t *rects;
    egui_dim_t natural_width;
    egui_dim_t natural_height;
    uint16_t rect_count = 0;
    float view_x = 0.0f;
    float view_y = 0.0f;
    float view_width = 0.0f;
    float view_height = 0.0f;
    float natural_width_raw = 0.0f;
    float natural_height_raw = 0.0f;
    egui_dim_t doc_view_x;
    egui_dim_t doc_view_y;
    egui_dim_t doc_view_width;
    egui_dim_t doc_view_height;

    if (core == NULL || res == NULL || res->res_type != EGUI_RESOURCE_TYPE_EXTERNAL || res->data_buf == NULL || res->data_size == 0)
    {
        return NULL;
    }

    if (!egui_svg_scan_external_rect_fast_path(core, res, NULL, &rect_count, &view_x, &view_y, &view_width, &view_height, &natural_width_raw,
                                               &natural_height_raw) ||
        !egui_svg_dim_from_float(natural_width_raw, &natural_width) || !egui_svg_dim_from_float(natural_height_raw, &natural_height))
    {
        return NULL;
    }
    if (!egui_svg_dim_from_integral_float(view_x, &doc_view_x) || !egui_svg_dim_from_integral_float(view_y, &doc_view_y) ||
        !egui_svg_dim_from_integral_float(view_width, &doc_view_width) || !egui_svg_dim_from_integral_float(view_height, &doc_view_height) ||
        doc_view_width <= 0 || doc_view_height <= 0)
    {
        return NULL;
    }

    doc = egui_svg_alloc_rect_fast_path_doc(rect_count, &rects);
    if (doc == NULL)
    {
        return NULL;
    }
    doc->natural_width = natural_width;
    doc->natural_height = natural_height;

    view_x = 0.0f;
    view_y = 0.0f;
    view_width = (float)doc->natural_width;
    view_height = (float)doc->natural_height;
    rect_count = 0;
    if (!egui_svg_scan_external_rect_fast_path(core, res, rects, &rect_count, &view_x, &view_y, &view_width, &view_height, NULL, NULL))
    {
        egui_svg_doc_destroy(doc);
        return NULL;
    }
    if (!egui_svg_set_rect_fast_path_doc(doc, rects, rect_count, view_x, view_y, view_width, view_height))
    {
        egui_svg_doc_destroy(doc);
        return NULL;
    }
    return doc;
}

static int egui_image_svg_try_assign_rect_fast_path_doc(egui_image_svg_t *self, const char *svg_text, uint32_t svg_len)
{
    egui_svg_doc_t *doc;

    if (self == NULL)
    {
        return 0;
    }

    doc = egui_svg_try_create_rect_fast_path_doc(svg_text, svg_len);
    if (doc == NULL)
    {
        return 0;
    }

    egui_image_svg_reset(self);
    self->base.api = &egui_image_svg_t_api_table;
    self->doc = doc;
    self->owned_data_buf = NULL;
    self->base.res = doc;
    return 1;
}

static int egui_image_svg_try_assign_external_rect_fast_path_doc(egui_image_svg_t *self, egui_core_t *core, const egui_svg_source_t *res)
{
    egui_svg_doc_t *doc;

    if (self == NULL)
    {
        return 0;
    }

    doc = egui_svg_try_create_external_rect_fast_path_doc(core, res);
    if (doc == NULL)
    {
        return 0;
    }

    egui_image_svg_reset(self);
    self->base.api = &egui_image_svg_t_api_table;
    self->doc = doc;
    self->owned_data_buf = NULL;
    self->base.res = doc;
    return 1;
}

static void egui_image_svg_free_owned_load_data(uint8_t *owned_data_buf)
{
    if (owned_data_buf != NULL)
    {
        egui_free(NULL, owned_data_buf);
    }
}

static int egui_image_svg_finish_load(egui_image_svg_t *self, const char *svg_text, uint8_t *owned_data_buf, uint32_t svg_len)
{
    plutosvg_document_t *document;
    egui_svg_doc_t *doc;
    float natural_width_raw;
    float natural_height_raw;
    egui_dim_t natural_width;
    egui_dim_t natural_height;

    if (self == NULL || svg_text == NULL || svg_len == 0 || svg_len >= (uint32_t)INT_MAX)
    {
        egui_image_svg_free_owned_load_data(owned_data_buf);
        return 0;
    }

    self->base.api = &egui_image_svg_t_api_table;
    doc = egui_svg_try_create_rect_fast_path_doc(svg_text, svg_len);
    if (doc != NULL)
    {
        self->doc = doc;
        self->owned_data_buf = NULL;
        self->base.res = doc;
        egui_image_svg_free_owned_load_data(owned_data_buf);
        return 1;
    }

    document = plutosvg_document_load_from_data(svg_text, (int)svg_len, -1.0f, -1.0f, NULL, NULL);
    if (document == NULL)
    {
        EGUI_LOG_WRN("PlutoSVG parse failed.\n");
        egui_image_svg_free_owned_load_data(owned_data_buf);
        return 0;
    }

    natural_width_raw = plutosvg_document_get_width(document);
    natural_height_raw = plutosvg_document_get_height(document);
    if (!(natural_width_raw > 0.0f) || !(natural_height_raw > 0.0f))
    {
        EGUI_LOG_WRN("SVG root size invalid, discard document.\n");
        plutosvg_document_destroy(document);
        egui_image_svg_free_owned_load_data(owned_data_buf);
        return 0;
    }
    if (natural_width_raw > (float)EGUI_DIM_MAX || natural_height_raw > (float)EGUI_DIM_MAX || !egui_svg_dim_from_float(natural_width_raw, &natural_width) ||
        !egui_svg_dim_from_float(natural_height_raw, &natural_height))
    {
        EGUI_LOG_WRN("SVG root size %ld x %ld exceeds egui_dim_t max %d, discard document.\n", (long)egui_svg_dim_for_log(natural_width_raw),
                     (long)egui_svg_dim_for_log(natural_height_raw), EGUI_DIM_MAX);
        plutosvg_document_destroy(document);
        egui_image_svg_free_owned_load_data(owned_data_buf);
        return 0;
    }

    doc = (egui_svg_doc_t *)egui_malloc(NULL, sizeof(*doc));
    if (doc == NULL)
    {
        plutosvg_document_destroy(document);
        egui_image_svg_free_owned_load_data(owned_data_buf);
        return 0;
    }
    egui_api_memset(doc, 0, sizeof(*doc));
    doc->natural_width = natural_width;
    doc->natural_height = natural_height;
    if (egui_svg_try_build_rect_fast_path(doc, svg_text, svg_len))
    {
        plutosvg_document_destroy(document);
        self->doc = doc;
        self->owned_data_buf = NULL;
        self->base.res = doc;
        egui_image_svg_free_owned_load_data(owned_data_buf);
        return 1;
    }

    doc->state.raster.document = document;
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
    const egui_image_svg_t *svg = (const egui_image_svg_t *)self;
    const egui_svg_doc_t *doc = (const egui_svg_doc_t *)svg->doc;

    if (doc == NULL)
    {
        return 0;
    }
    if (doc->rect_fast_path)
    {
        return egui_svg_query_rect_fast_path(doc, x, y, doc->natural_width, doc->natural_height, color, alpha);
    }
    if (egui_svg_render_cache((egui_svg_doc_t *)doc, doc->natural_width, doc->natural_height))
    {
        return egui_svg_cached_raster_query_pixel(doc, x, y, color, alpha);
    }
    return egui_svg_query_surface_pixel(doc, x, y, doc->natural_width, doc->natural_height, color, alpha);
}

static int egui_image_svg_get_point_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color,
                                           egui_alpha_t *alpha)
{
    const egui_image_svg_t *svg = (const egui_image_svg_t *)self;
    const egui_svg_doc_t *doc = (const egui_svg_doc_t *)svg->doc;

    if (doc == NULL)
    {
        return 0;
    }
    if (doc->rect_fast_path)
    {
        return egui_svg_query_rect_fast_path(doc, x, y, width, height, color, alpha);
    }
    if (egui_svg_render_cache((egui_svg_doc_t *)doc, width, height))
    {
        return egui_svg_cached_raster_query_pixel(doc, x, y, color, alpha);
    }
    return egui_svg_query_surface_pixel(doc, x, y, width, height, color, alpha);
}

static void egui_image_svg_draw_image(const egui_image_t *self, egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y)
{
    const egui_image_svg_t *svg = (const egui_image_svg_t *)self;
    const egui_svg_doc_t *doc = (const egui_svg_doc_t *)svg->doc;

    if (doc == NULL)
    {
        return;
    }
    if (doc->rect_fast_path)
    {
        egui_svg_draw_rect_fast_path((egui_svg_doc_t *)doc, canvas, x, y, doc->natural_width, doc->natural_height);
        return;
    }
    if (egui_svg_render_cache((egui_svg_doc_t *)doc, doc->natural_width, doc->natural_height))
    {
        egui_svg_cached_raster_draw(doc, canvas, x, y);
        return;
    }
    egui_svg_draw_clipped_region(doc, canvas, x, y, doc->natural_width, doc->natural_height);
}

static void egui_image_svg_draw_image_resize(const egui_image_t *self, egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    const egui_image_svg_t *svg = (const egui_image_svg_t *)self;
    const egui_svg_doc_t *doc = (const egui_svg_doc_t *)svg->doc;

    if (doc == NULL)
    {
        return;
    }
    if (doc->rect_fast_path)
    {
        egui_svg_draw_rect_fast_path((egui_svg_doc_t *)doc, canvas, x, y, width, height);
        return;
    }
    if (egui_svg_render_cache((egui_svg_doc_t *)doc, width, height))
    {
        egui_svg_cached_raster_draw(doc, canvas, x, y);
        return;
    }
    egui_svg_draw_clipped_region(doc, canvas, x, y, width, height);
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

    if (egui_image_svg_try_assign_rect_fast_path_doc(self, svg_text, svg_len))
    {
        return 1;
    }

    owned_data_buf = (uint8_t *)egui_malloc(NULL, (int)svg_len + 1);
    if (owned_data_buf == NULL)
    {
        return 0;
    }
    memcpy(owned_data_buf, svg_text, svg_len);
    owned_data_buf[svg_len] = '\0';

    egui_image_svg_reset(self);
    return egui_image_svg_finish_load(self, (const char *)owned_data_buf, owned_data_buf, svg_len);
}

int egui_image_svg_load_memory_borrowed(egui_image_svg_t *self, const char *svg_text)
{
    size_t svg_len;

    if (self == NULL || svg_text == NULL)
    {
        return 0;
    }

    svg_len = strlen(svg_text);
    if (svg_len == 0 || svg_len >= (size_t)INT_MAX)
    {
        return 0;
    }

    egui_image_svg_reset(self);
    return egui_image_svg_finish_load(self, svg_text, NULL, (uint32_t)svg_len);
}

int egui_image_svg_load_resource(egui_image_svg_t *self, egui_core_t *core, const egui_svg_source_t *res)
{
    uint8_t *owned_data_buf;

    if (self == NULL || res == NULL || res->data_buf == NULL || res->data_size == 0 || res->data_size >= (uint32_t)INT_MAX)
    {
        return 0;
    }
    if (res->res_type == EGUI_RESOURCE_TYPE_EXTERNAL && core == NULL)
    {
        return 0;
    }

    if (res->res_type == EGUI_RESOURCE_TYPE_INTERNAL && egui_image_svg_try_assign_rect_fast_path_doc(self, (const char *)res->data_buf, res->data_size))
    {
        return 1;
    }
    if (res->res_type == EGUI_RESOURCE_TYPE_EXTERNAL && egui_image_svg_try_assign_external_rect_fast_path_doc(self, core, res))
    {
        return 1;
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
        egui_api_load_external_resource(&core->canvas, owned_data_buf, (egui_uintptr_t)res->data_buf, 0, res->data_size);
    }
    else
    {
        egui_free(NULL, owned_data_buf);
        return 0;
    }
    owned_data_buf[res->data_size] = '\0';

    egui_image_svg_reset(self);
    return egui_image_svg_finish_load(self, (const char *)owned_data_buf, owned_data_buf, res->data_size);
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

int egui_image_svg_load_memory_borrowed(egui_image_svg_t *self, const char *svg_text)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(svg_text);
    return 0;
}

int egui_image_svg_load_resource(egui_image_svg_t *self, egui_core_t *core, const egui_svg_source_t *res)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(core);
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
