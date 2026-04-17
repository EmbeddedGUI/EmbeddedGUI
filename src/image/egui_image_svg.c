#include "egui_image_svg.h"
#include "egui_image_std.h"
#include "core/egui_api.h"

#include <limits.h>
#include <string.h>

#if EGUI_CONFIG_IMAGE_RUNTIME_SVG_ENABLE

#if !EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565 || !EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8
#error "PlutoSVG backend requires EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565=1 and EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8=1."
#endif

#include "plutosvg.h"

typedef struct egui_svg_doc
{
    plutosvg_document_t *document;
    egui_image_std_info_t raster_info;
    uint16_t *data_buf;
    uint8_t *alpha_buf;
    egui_dim_t natural_width;
    egui_dim_t natural_height;
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

static int egui_svg_cache_matches(const egui_svg_doc_t *doc, egui_dim_t width, egui_dim_t height)
{
    return doc != NULL && doc->raster_info.data_buf != NULL && doc->raster_info.width == (uint16_t)width && doc->raster_info.height == (uint16_t)height;
}

static void egui_svg_release_raster(egui_svg_doc_t *doc)
{
    if (doc == NULL)
    {
        return;
    }

    if (doc->data_buf != NULL)
    {
        egui_free(doc->data_buf);
        doc->data_buf = NULL;
    }
    if (doc->alpha_buf != NULL)
    {
        egui_free(doc->alpha_buf);
        doc->alpha_buf = NULL;
    }

    doc->raster_info.data_buf = NULL;
    doc->raster_info.alpha_buf = NULL;
    doc->raster_info.width = 0;
    doc->raster_info.height = 0;
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
    egui_free(doc);
}

static int egui_svg_build_raster_from_rgba(int width, int height, int stride, const unsigned char *surface_data, uint16_t **out_data_buf,
                                           uint8_t **out_alpha_buf, egui_image_std_info_t *out_info)
{
    size_t pixel_count;
    size_t data_bytes;
    size_t alpha_bytes;
    uint16_t *data_buf;
    uint8_t *alpha_buf;
    int y;

    if (out_data_buf == NULL || out_alpha_buf == NULL || out_info == NULL || surface_data == NULL || width <= 0 || height <= 0 || stride < width * 4)
    {
        return 0;
    }

    pixel_count = (size_t)width * (size_t)height;
    data_bytes = pixel_count * sizeof(uint16_t);
    alpha_bytes = pixel_count * sizeof(uint8_t);
    if (data_bytes > (size_t)INT_MAX || alpha_bytes > (size_t)INT_MAX)
    {
        return 0;
    }

    data_buf = (uint16_t *)egui_malloc((int)data_bytes);
    if (data_buf == NULL)
    {
        return 0;
    }
    alpha_buf = (uint8_t *)egui_malloc((int)alpha_bytes);
    if (alpha_buf == NULL)
    {
        egui_free(data_buf);
        return 0;
    }

    for (y = 0; y < height; y++)
    {
        const unsigned char *src_row = surface_data + (size_t)y * (size_t)stride;
        uint16_t *dst_data_row = data_buf + (size_t)y * (size_t)width;
        uint8_t *dst_alpha_row = alpha_buf + (size_t)y * (size_t)width;
        int x;

        for (x = 0; x < width; x++)
        {
            const unsigned char *rgba = src_row + (size_t)x * 4u;
            uint8_t r = rgba[0];
            uint8_t g = rgba[1];
            uint8_t b = rgba[2];

            dst_data_row[x] = (uint16_t)(((uint16_t)(r & 0xF8u) << 8) | ((uint16_t)(g & 0xFCu) << 3) | ((uint16_t)b >> 3));
            dst_alpha_row[x] = rgba[3];
        }
    }

    memset(out_info, 0, sizeof(*out_info));
    out_info->data_buf = data_buf;
    out_info->alpha_buf = alpha_buf;
    out_info->data_type = EGUI_IMAGE_DATA_TYPE_RGB565;
    out_info->alpha_type = EGUI_IMAGE_ALPHA_TYPE_8;
    out_info->res_type = EGUI_RESOURCE_TYPE_INTERNAL;
    out_info->width = (uint16_t)width;
    out_info->height = (uint16_t)height;
    *out_data_buf = data_buf;
    *out_alpha_buf = alpha_buf;
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
    egui_image_std_info_t new_info;

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

    plutovg_convert_argb_to_rgba(surface_data, surface_data, actual_width, actual_height, stride);
    if (!egui_svg_build_raster_from_rgba(actual_width, actual_height, stride, surface_data, &new_data_buf, &new_alpha_buf, &new_info))
    {
        EGUI_LOG_WRN("PlutoSVG raster cache allocate failed for %d x %d.\n", actual_width, actual_height);
        plutovg_surface_destroy(surface);
        return 0;
    }

    plutovg_surface_destroy(surface);
    egui_svg_release_raster(doc);
    doc->data_buf = new_data_buf;
    doc->alpha_buf = new_alpha_buf;
    doc->raster_info = new_info;
    return 1;
}

static void egui_image_svg_release_owned_data(egui_image_svg_t *self)
{
    if (self->owned_data_buf != NULL)
    {
        egui_free(self->owned_data_buf);
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
            egui_free(owned_data_buf);
        }
        return 0;
    }

    self->base.api = &egui_image_svg_t_api_table;
    document = plutosvg_document_load_from_data((const char *)owned_data_buf, (int)svg_len, -1.0f, -1.0f, NULL, NULL);
    if (document == NULL)
    {
        EGUI_LOG_WRN("PlutoSVG parse failed.\n");
        egui_free(owned_data_buf);
        return 0;
    }

    natural_width_raw = plutosvg_document_get_width(document);
    natural_height_raw = plutosvg_document_get_height(document);
    if (!(natural_width_raw > 0.0f) || !(natural_height_raw > 0.0f))
    {
        EGUI_LOG_WRN("SVG root size invalid, discard document.\n");
        plutosvg_document_destroy(document);
        egui_free(owned_data_buf);
        return 0;
    }
    if (natural_width_raw > (float)EGUI_DIM_MAX || natural_height_raw > (float)EGUI_DIM_MAX || !egui_svg_dim_from_float(natural_width_raw, &natural_width) ||
        !egui_svg_dim_from_float(natural_height_raw, &natural_height))
    {
        EGUI_LOG_WRN("SVG root size %ld x %ld exceeds egui_dim_t max %d, discard document.\n", (long)egui_svg_dim_for_log(natural_width_raw),
                     (long)egui_svg_dim_for_log(natural_height_raw), EGUI_DIM_MAX);
        plutosvg_document_destroy(document);
        egui_free(owned_data_buf);
        return 0;
    }

    doc = (egui_svg_doc_t *)egui_malloc(sizeof(*doc));
    if (doc == NULL)
    {
        plutosvg_document_destroy(document);
        egui_free(owned_data_buf);
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

static int egui_image_svg_get_point(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_image_svg_t *svg = (egui_image_svg_t *)self;
    egui_svg_doc_t *doc = (egui_svg_doc_t *)svg->doc;
    egui_image_std_t raster_image;

    if (doc == NULL || !egui_svg_render_cache(doc, doc->natural_width, doc->natural_height))
    {
        return 0;
    }

    egui_image_std_init(&raster_image.base, &doc->raster_info);
    if (egui_svg_cache_matches(doc, doc->natural_width, doc->natural_height))
    {
        return raster_image.base.api->get_point(&raster_image.base, x, y, color, alpha);
    }
    return raster_image.base.api->get_point_resize(&raster_image.base, x, y, doc->natural_width, doc->natural_height, color, alpha);
}

static int egui_image_svg_get_point_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color,
                                           egui_alpha_t *alpha)
{
    egui_image_svg_t *svg = (egui_image_svg_t *)self;
    egui_svg_doc_t *doc = (egui_svg_doc_t *)svg->doc;
    egui_image_std_t raster_image;

    if (doc == NULL || width <= 0 || height <= 0 || !egui_svg_render_cache(doc, width, height))
    {
        return 0;
    }

    egui_image_std_init(&raster_image.base, &doc->raster_info);
    if (egui_svg_cache_matches(doc, width, height))
    {
        return raster_image.base.api->get_point(&raster_image.base, x, y, color, alpha);
    }
    return raster_image.base.api->get_point_resize(&raster_image.base, x, y, width, height, color, alpha);
}

static void egui_image_svg_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_image_svg_t *svg = (egui_image_svg_t *)self;
    egui_svg_doc_t *doc = (egui_svg_doc_t *)svg->doc;
    egui_image_std_t raster_image;

    if (doc == NULL || !egui_svg_render_cache(doc, doc->natural_width, doc->natural_height))
    {
        return;
    }

    egui_image_std_init(&raster_image.base, &doc->raster_info);
    if (egui_svg_cache_matches(doc, doc->natural_width, doc->natural_height))
    {
        raster_image.base.api->draw_image(&raster_image.base, x, y);
    }
    else
    {
        raster_image.base.api->draw_image_resize(&raster_image.base, x, y, doc->natural_width, doc->natural_height);
    }
}

static void egui_image_svg_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_image_svg_t *svg = (egui_image_svg_t *)self;
    egui_svg_doc_t *doc = (egui_svg_doc_t *)svg->doc;
    egui_image_std_t raster_image;

    if (doc == NULL || width <= 0 || height <= 0 || !egui_svg_render_cache(doc, width, height))
    {
        return;
    }

    egui_image_std_init(&raster_image.base, &doc->raster_info);
    if (egui_svg_cache_matches(doc, width, height))
    {
        raster_image.base.api->draw_image(&raster_image.base, x, y);
    }
    else
    {
        raster_image.base.api->draw_image_resize(&raster_image.base, x, y, width, height);
    }
}

const egui_image_api_t egui_image_svg_t_api_table = {
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

    owned_data_buf = (uint8_t *)egui_malloc((int)svg_len + 1);
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

    owned_data_buf = (uint8_t *)egui_malloc((int)res->data_size + 1);
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
        egui_api_load_external_resource(owned_data_buf, (egui_uintptr_t)res->data_buf, 0, res->data_size);
    }
    else
    {
        egui_free(owned_data_buf);
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
