#include <string.h>

#include "egui_image_file.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE

static const egui_image_file_io_t *g_egui_image_file_default_io = NULL;
static const egui_image_file_decoder_t *g_egui_image_file_decoders[EGUI_CONFIG_IMAGE_FILE_DECODER_MAX_COUNT];
static uint8_t g_egui_image_file_decoder_count = 0;

static egui_image_file_t *egui_image_file_from_base(const egui_image_t *self)
{
    if (self == NULL || self->res == NULL)
    {
        return NULL;
    }

    return (egui_image_file_t *)self->res;
}

static void egui_image_file_invalidate_row_cache(egui_image_file_t *self)
{
    self->cached_row = 0xFFFFu;
    self->row_cache_valid = 0;
}

static void egui_image_file_invalidate_resize_cache(egui_image_file_t *self)
{
    self->resize_x_map_width = 0;
    self->resize_y_map_height = 0;
    self->resize_x_map_src_width = 0;
    self->resize_y_map_src_height = 0;
}

static void egui_image_file_reset_runtime(egui_image_file_t *self)
{
    self->decoder = NULL;
    self->decoder_ctx = NULL;
    self->file_handle = NULL;
    self->active_io = NULL;
    self->width = 0;
    self->height = 0;
    self->has_alpha = 0;
    egui_image_file_invalidate_row_cache(self);
    egui_image_file_invalidate_resize_cache(self);
}

static void egui_image_file_close_runtime(egui_image_file_t *self)
{
    if (self->decoder != NULL && self->decoder->close != NULL && self->decoder_ctx != NULL)
    {
        self->decoder->close(self->decoder_ctx);
    }
    if (self->active_io != NULL && self->active_io->close != NULL && self->file_handle != NULL)
    {
        self->active_io->close(self->active_io->user_data, self->file_handle);
    }

    egui_image_file_reset_runtime(self);
}

static int egui_image_file_alloc_copy_string(char **target, const char *value)
{
    size_t len;
    char *copy;

    if (target == NULL)
    {
        return 0;
    }

    if (value == NULL || value[0] == '\0')
    {
        if (*target != NULL)
        {
            egui_free(*target);
            *target = NULL;
        }
        return 1;
    }

    len = strlen(value);
    copy = (char *)egui_malloc((int)(len + 1));
    if (copy == NULL)
    {
        return 0;
    }

    memcpy(copy, value, len + 1);
    if (*target != NULL)
    {
        egui_free(*target);
    }
    *target = copy;
    return 1;
}

static int egui_image_file_prepare_row_buffers(egui_image_file_t *self)
{
    uint16_t *new_pixels = self->row_pixels;
    uint8_t *new_alpha = self->row_alpha;
    uint16_t width = self->width;

    if (width == 0)
    {
        self->status = EGUI_IMAGE_FILE_STATUS_OPEN_DECODER_FAIL;
        return 0;
    }

    if (self->row_capacity < width)
    {
        new_pixels = (uint16_t *)egui_malloc((int)((uint32_t)width * sizeof(uint16_t)));
        if (new_pixels == NULL)
        {
            self->status = EGUI_IMAGE_FILE_STATUS_OOM;
            return 0;
        }
    }

    if (self->has_alpha)
    {
        if (self->alpha_capacity < width)
        {
            new_alpha = (uint8_t *)egui_malloc((int)width);
            if (new_alpha == NULL)
            {
                if (new_pixels != self->row_pixels)
                {
                    egui_free(new_pixels);
                }
                self->status = EGUI_IMAGE_FILE_STATUS_OOM;
                return 0;
            }
        }
    }
    else if (self->row_alpha != NULL)
    {
        egui_free(self->row_alpha);
        self->row_alpha = NULL;
        self->alpha_capacity = 0;
    }

    if (new_pixels != self->row_pixels)
    {
        if (self->row_pixels != NULL)
        {
            egui_free(self->row_pixels);
        }
        self->row_pixels = new_pixels;
        self->row_capacity = width;
    }

    if (self->has_alpha && new_alpha != self->row_alpha)
    {
        if (self->row_alpha != NULL)
        {
            egui_free(self->row_alpha);
        }
        self->row_alpha = new_alpha;
        self->alpha_capacity = width;
    }

    return 1;
}

static int egui_image_file_prepare_resize_map_storage(uint16_t **target, uint16_t *capacity, uint16_t required)
{
    uint16_t *new_map;

    if (target == NULL || capacity == NULL)
    {
        return 0;
    }
    if (*capacity >= required)
    {
        return 1;
    }

    new_map = (uint16_t *)egui_malloc((int)((uint32_t)required * sizeof(uint16_t)));
    if (new_map == NULL)
    {
        return 0;
    }

    if (*target != NULL)
    {
        egui_free(*target);
    }

    *target = new_map;
    *capacity = required;
    return 1;
}

static int egui_image_file_prepare_resize_maps(egui_image_file_t *self, egui_dim_t width, egui_dim_t height)
{
    uint16_t dest_width;
    uint16_t dest_height;
    uint16_t i;

    if (self == NULL || width <= 0 || height <= 0 || self->width == 0 || self->height == 0)
    {
        return 0;
    }

    dest_width = (uint16_t)width;
    dest_height = (uint16_t)height;

    if (self->resize_x_map_width != dest_width || self->resize_x_map_src_width != self->width)
    {
        if (!egui_image_file_prepare_resize_map_storage(&self->resize_x_map, &self->resize_x_map_capacity, dest_width))
        {
            self->status = EGUI_IMAGE_FILE_STATUS_OOM;
            return 0;
        }

        for (i = 0; i < dest_width; i++)
        {
            uint16_t src_x = (uint16_t)(((uint32_t)i * self->width) / dest_width);

            if (src_x >= self->width)
            {
                src_x = self->width - 1;
            }
            self->resize_x_map[i] = src_x;
        }

        self->resize_x_map_width = dest_width;
        self->resize_x_map_src_width = self->width;
    }

    if (self->resize_y_map_height != dest_height || self->resize_y_map_src_height != self->height)
    {
        if (!egui_image_file_prepare_resize_map_storage(&self->resize_y_map, &self->resize_y_map_capacity, dest_height))
        {
            self->status = EGUI_IMAGE_FILE_STATUS_OOM;
            return 0;
        }

        for (i = 0; i < dest_height; i++)
        {
            uint16_t src_y = (uint16_t)(((uint32_t)i * self->height) / dest_height);

            if (src_y >= self->height)
            {
                src_y = self->height - 1;
            }
            self->resize_y_map[i] = src_y;
        }

        self->resize_y_map_height = dest_height;
        self->resize_y_map_src_height = self->height;
    }

    return 1;
}

static int egui_image_file_try_open(egui_image_file_t *self)
{
    const egui_image_file_io_t *io;
    void *file_handle;
    uint8_t i;
    int saw_matching_decoder = 0;

    if (self == NULL)
    {
        return 0;
    }
    if (self->decoder_ctx != NULL)
    {
        self->status = EGUI_IMAGE_FILE_STATUS_READY;
        return 1;
    }
    if (self->path == NULL || self->path[0] == '\0')
    {
        self->status = EGUI_IMAGE_FILE_STATUS_NO_PATH;
        return 0;
    }

    io = (self->io != NULL) ? self->io : g_egui_image_file_default_io;
    if (io == NULL || io->open == NULL || io->close == NULL)
    {
        self->status = EGUI_IMAGE_FILE_STATUS_NO_IO;
        return 0;
    }

    file_handle = io->open(io->user_data, self->path);
    if (file_handle == NULL)
    {
        self->status = EGUI_IMAGE_FILE_STATUS_OPEN_FILE_FAIL;
        return 0;
    }

    for (i = 0; i < g_egui_image_file_decoder_count; i++)
    {
        const egui_image_file_decoder_t *decoder = g_egui_image_file_decoders[i];
        void *decoder_ctx = NULL;
        egui_image_file_open_result_t open_result = {0, 0, 0, 0};

        if (decoder == NULL || decoder->open == NULL || decoder->read_row == NULL)
        {
            continue;
        }
        if (decoder->match != NULL && !decoder->match(self->path))
        {
            continue;
        }

        saw_matching_decoder = 1;
        if (io->seek != NULL)
        {
            io->seek(io->user_data, file_handle, 0, EGUI_IMAGE_FILE_SEEK_SET);
        }
        if (!decoder->open(io, file_handle, self->path, &decoder_ctx, &open_result))
        {
            if (decoder->close != NULL && decoder_ctx != NULL)
            {
                decoder->close(decoder_ctx);
            }
            continue;
        }
        if (open_result.width == 0 || open_result.height == 0)
        {
            if (decoder->close != NULL && decoder_ctx != NULL)
            {
                decoder->close(decoder_ctx);
            }
            continue;
        }

        self->decoder = decoder;
        self->decoder_ctx = decoder_ctx;
        self->width = open_result.width;
        self->height = open_result.height;
        self->has_alpha = open_result.has_alpha ? 1u : 0u;
        if (open_result.keep_file_open)
        {
            self->file_handle = file_handle;
            self->active_io = io;
        }
        else
        {
            io->close(io->user_data, file_handle);
            self->file_handle = NULL;
            self->active_io = NULL;
        }
        self->status = EGUI_IMAGE_FILE_STATUS_READY;
        egui_image_file_invalidate_row_cache(self);
        return 1;
    }

    io->close(io->user_data, file_handle);
    self->status = saw_matching_decoder ? EGUI_IMAGE_FILE_STATUS_OPEN_DECODER_FAIL : EGUI_IMAGE_FILE_STATUS_NO_DECODER;
    return 0;
}

static int egui_image_file_load_row(egui_image_file_t *self, uint16_t row)
{
    if (self == NULL)
    {
        return 0;
    }
    if (row >= self->height)
    {
        return 0;
    }
    if (!egui_image_file_try_open(self))
    {
        return 0;
    }
    if (!egui_image_file_prepare_row_buffers(self))
    {
        return 0;
    }
    if (self->row_cache_valid && self->cached_row == row)
    {
        return 1;
    }
    if (self->decoder == NULL || self->decoder->read_row == NULL)
    {
        self->status = EGUI_IMAGE_FILE_STATUS_OPEN_DECODER_FAIL;
        return 0;
    }
    if (!self->decoder->read_row(self->decoder_ctx, row, self->row_pixels, self->has_alpha ? self->row_alpha : NULL))
    {
        egui_image_file_close_runtime(self);
        self->status = EGUI_IMAGE_FILE_STATUS_DECODE_ROW_FAIL;
        return 0;
    }

    self->cached_row = row;
    self->row_cache_valid = 1;
    self->status = EGUI_IMAGE_FILE_STATUS_READY;
    return 1;
}

static void egui_image_file_draw_fallback(const egui_image_file_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, int resize)
{
    if (self == NULL || self->placeholder == NULL || self->placeholder == (const egui_image_t *)&self->base)
    {
        return;
    }

    if (resize)
    {
        egui_image_draw_image_resize(self->placeholder, x, y, width, height);
    }
    else
    {
        egui_image_draw_image(self->placeholder, x, y);
    }
}

static int egui_image_file_get_visible_rect(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t *screen_x_start,
                                            egui_dim_t *screen_y_start, egui_dim_t *screen_x_end, egui_dim_t *screen_y_end)
{
    egui_region_t *work = egui_canvas_get_base_view_work_region();
    egui_dim_t x_end = x + width;
    egui_dim_t y_end = y + height;

    if (work == NULL || width <= 0 || height <= 0)
    {
        return 0;
    }

    *screen_x_start = EGUI_MAX(x, work->location.x);
    *screen_y_start = EGUI_MAX(y, work->location.y);
    *screen_x_end = EGUI_MIN(x_end, work->location.x + work->size.width);
    *screen_y_end = EGUI_MIN(y_end, work->location.y + work->size.height);
    return *screen_x_start < *screen_x_end && *screen_y_start < *screen_y_end;
}

static void egui_image_file_draw_visible_row(const egui_image_file_t *self, egui_dim_t screen_x_start, egui_dim_t screen_y, egui_dim_t dest_width,
                                             egui_dim_t src_x_offset)
{
    egui_mask_t *mask = egui_canvas_get_mask();
    egui_dim_t visible_count = dest_width;
    egui_dim_t i;

    for (i = 0; i < visible_count; i++)
    {
        egui_dim_t src_x = src_x_offset + i;
        egui_color_t color;
        egui_alpha_t alpha = EGUI_ALPHA_100;

        color.full = EGUI_COLOR_RGB565_TRANS(self->row_pixels[src_x]);
        if (self->has_alpha)
        {
            alpha = self->row_alpha[src_x];
            if (alpha == 0)
            {
                continue;
            }
        }

        if (mask == NULL)
        {
            egui_canvas_draw_point_limit_skip_mask(screen_x_start + i, screen_y, color, alpha);
        }
        else
        {
            egui_canvas_draw_point_limit(screen_x_start + i, screen_y, color, alpha);
        }
    }
}

static int egui_image_file_api_get_size(const egui_image_t *base, egui_dim_t *width, egui_dim_t *height)
{
    egui_image_file_t *self = egui_image_file_from_base(base);

    if (self == NULL || !egui_image_file_try_open(self))
    {
        return 0;
    }
    if (width != NULL)
    {
        *width = self->width;
    }
    if (height != NULL)
    {
        *height = self->height;
    }
    return 1;
}

static int egui_image_file_api_get_point(const egui_image_t *base, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_image_file_t *self = egui_image_file_from_base(base);

    if (self == NULL)
    {
        return 0;
    }
    if (!egui_image_file_try_open(self))
    {
        return 0;
    }
    if (x < 0 || y < 0 || x >= self->width || y >= self->height)
    {
        return 0;
    }
    if (!egui_image_file_load_row(self, (uint16_t)y))
    {
        return 0;
    }

    if (color != NULL)
    {
        color->full = EGUI_COLOR_RGB565_TRANS(self->row_pixels[x]);
    }
    if (alpha != NULL)
    {
        *alpha = self->has_alpha ? self->row_alpha[x] : EGUI_ALPHA_100;
    }
    return 1;
}

static int egui_image_file_api_get_point_resize(const egui_image_t *base, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color,
                                                egui_alpha_t *alpha)
{
    egui_image_file_t *self = egui_image_file_from_base(base);
    egui_dim_t src_x;
    egui_dim_t src_y;

    if (self == NULL || width <= 0 || height <= 0 || x < 0 || y < 0 || x >= width || y >= height)
    {
        return 0;
    }
    if (!egui_image_file_try_open(self))
    {
        return 0;
    }

    src_x = (egui_dim_t)(((int32_t)x * self->width) / width);
    src_y = (egui_dim_t)(((int32_t)y * self->height) / height);
    if (src_x >= self->width)
    {
        src_x = self->width - 1;
    }
    if (src_y >= self->height)
    {
        src_y = self->height - 1;
    }
    return egui_image_file_api_get_point(base, src_x, src_y, color, alpha);
}

static void egui_image_file_api_draw_image(const egui_image_t *base, egui_dim_t x, egui_dim_t y)
{
    egui_image_file_t *self = egui_image_file_from_base(base);
    egui_dim_t screen_x_start;
    egui_dim_t screen_y_start;
    egui_dim_t screen_x_end;
    egui_dim_t screen_y_end;
    egui_dim_t screen_y;

    if (self == NULL || !egui_image_file_try_open(self))
    {
        egui_image_file_draw_fallback(self, x, y, 0, 0, 0);
        return;
    }
    if (!egui_image_file_get_visible_rect(x, y, self->width, self->height, &screen_x_start, &screen_y_start, &screen_x_end, &screen_y_end))
    {
        return;
    }

    for (screen_y = screen_y_start; screen_y < screen_y_end; screen_y++)
    {
        uint16_t src_y = (uint16_t)(screen_y - y);
        egui_dim_t src_x_offset = screen_x_start - x;

        if (!egui_image_file_load_row(self, src_y))
        {
            egui_image_file_draw_fallback(self, x, y, 0, 0, 0);
            return;
        }

        egui_image_file_draw_visible_row(self, screen_x_start, screen_y, screen_x_end - screen_x_start, src_x_offset);
    }
}

static void egui_image_file_api_draw_image_resize(const egui_image_t *base, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_image_file_t *self = egui_image_file_from_base(base);
    egui_dim_t screen_x_start;
    egui_dim_t screen_y_start;
    egui_dim_t screen_x_end;
    egui_dim_t screen_y_end;
    egui_dim_t screen_y;
    egui_mask_t *mask = egui_canvas_get_mask();

    if (self == NULL || !egui_image_file_try_open(self))
    {
        egui_image_file_draw_fallback(self, x, y, width, height, 1);
        return;
    }
    if (!egui_image_file_get_visible_rect(x, y, width, height, &screen_x_start, &screen_y_start, &screen_x_end, &screen_y_end))
    {
        return;
    }
    if (!egui_image_file_prepare_resize_maps(self, width, height))
    {
        egui_image_file_draw_fallback(self, x, y, width, height, 1);
        return;
    }

    for (screen_y = screen_y_start; screen_y < screen_y_end; screen_y++)
    {
        egui_dim_t dest_y = screen_y - y;
        uint16_t src_y = self->resize_y_map[dest_y];
        egui_dim_t screen_x;
        const uint16_t *src_x_map = &self->resize_x_map[screen_x_start - x];

        if (!egui_image_file_load_row(self, src_y))
        {
            egui_image_file_draw_fallback(self, x, y, width, height, 1);
            return;
        }

        for (screen_x = screen_x_start; screen_x < screen_x_end; screen_x++, src_x_map++)
        {
            egui_dim_t src_x = *src_x_map;
            egui_color_t color;
            egui_alpha_t alpha = EGUI_ALPHA_100;

            color.full = EGUI_COLOR_RGB565_TRANS(self->row_pixels[src_x]);
            if (self->has_alpha)
            {
                alpha = self->row_alpha[src_x];
                if (alpha == 0)
                {
                    continue;
                }
            }

            if (mask == NULL)
            {
                egui_canvas_draw_point_limit_skip_mask(screen_x, screen_y, color, alpha);
            }
            else
            {
                egui_canvas_draw_point_limit(screen_x, screen_y, color, alpha);
            }
        }
    }
}

static const egui_image_api_t egui_image_file_api_table = {
        .get_size = egui_image_file_api_get_size,
        .get_point = egui_image_file_api_get_point,
        .get_point_resize = egui_image_file_api_get_point_resize,
        .draw_image = egui_image_file_api_draw_image,
        .draw_image_resize = egui_image_file_api_draw_image_resize,
};

void egui_image_file_init(egui_image_file_t *self)
{
    if (self == NULL)
    {
        return;
    }

    egui_image_init(&self->base, self);
    self->base.api = &egui_image_file_api_table;

    self->path = NULL;
    self->io = NULL;
    self->placeholder = NULL;
    self->row_pixels = NULL;
    self->row_alpha = NULL;
    self->resize_x_map = NULL;
    self->resize_y_map = NULL;
    self->row_capacity = 0;
    self->alpha_capacity = 0;
    self->resize_x_map_capacity = 0;
    self->resize_y_map_capacity = 0;
    self->status = EGUI_IMAGE_FILE_STATUS_IDLE;
    egui_image_file_reset_runtime(self);
}

void egui_image_file_deinit(egui_image_file_t *self)
{
    if (self == NULL)
    {
        return;
    }

    egui_image_file_close_runtime(self);
    if (self->path != NULL)
    {
        egui_free(self->path);
        self->path = NULL;
    }
    if (self->row_pixels != NULL)
    {
        egui_free(self->row_pixels);
        self->row_pixels = NULL;
    }
    if (self->row_alpha != NULL)
    {
        egui_free(self->row_alpha);
        self->row_alpha = NULL;
    }
    if (self->resize_x_map != NULL)
    {
        egui_free(self->resize_x_map);
        self->resize_x_map = NULL;
    }
    if (self->resize_y_map != NULL)
    {
        egui_free(self->resize_y_map);
        self->resize_y_map = NULL;
    }
    self->resize_x_map_capacity = 0;
    self->resize_y_map_capacity = 0;
    self->row_capacity = 0;
    self->alpha_capacity = 0;
    self->status = EGUI_IMAGE_FILE_STATUS_IDLE;
}

void egui_image_file_set_default_io(const egui_image_file_io_t *io)
{
    g_egui_image_file_default_io = io;
}

int egui_image_file_register_decoder(const egui_image_file_decoder_t *decoder)
{
    uint8_t i;

    if (decoder == NULL)
    {
        return 0;
    }
    for (i = 0; i < g_egui_image_file_decoder_count; i++)
    {
        if (g_egui_image_file_decoders[i] == decoder)
        {
            return 1;
        }
    }
    if (g_egui_image_file_decoder_count >= EGUI_CONFIG_IMAGE_FILE_DECODER_MAX_COUNT)
    {
        return 0;
    }

    g_egui_image_file_decoders[g_egui_image_file_decoder_count++] = decoder;
    return 1;
}

void egui_image_file_clear_decoders(void)
{
    g_egui_image_file_decoder_count = 0;
    memset(g_egui_image_file_decoders, 0, sizeof(g_egui_image_file_decoders));
}

int egui_image_file_set_path(egui_image_file_t *self, const char *path)
{
    if (self == NULL)
    {
        return 0;
    }
    if (self->path != NULL && path != NULL && strcmp(self->path, path) == 0)
    {
        return 1;
    }
    if (!egui_image_file_alloc_copy_string(&self->path, path))
    {
        self->status = EGUI_IMAGE_FILE_STATUS_OOM;
        return 0;
    }

    egui_image_file_close_runtime(self);
    self->status = (self->path != NULL) ? EGUI_IMAGE_FILE_STATUS_IDLE : EGUI_IMAGE_FILE_STATUS_NO_PATH;
    return 1;
}

void egui_image_file_set_io(egui_image_file_t *self, const egui_image_file_io_t *io)
{
    if (self == NULL || self->io == io)
    {
        return;
    }

    egui_image_file_close_runtime(self);
    self->io = io;
    self->status = EGUI_IMAGE_FILE_STATUS_IDLE;
}

void egui_image_file_set_placeholder(egui_image_file_t *self, const egui_image_t *placeholder)
{
    if (self == NULL)
    {
        return;
    }

    self->placeholder = placeholder;
}

int egui_image_file_reload(egui_image_file_t *self)
{
    if (self == NULL)
    {
        return 0;
    }

    egui_image_file_close_runtime(self);
    self->status = EGUI_IMAGE_FILE_STATUS_IDLE;
    return egui_image_file_try_open(self);
}

egui_image_file_status_t egui_image_file_get_status(const egui_image_file_t *self)
{
    if (self == NULL)
    {
        return EGUI_IMAGE_FILE_STATUS_IDLE;
    }

    return (egui_image_file_status_t)self->status;
}

const char *egui_image_file_get_decoder_name(const egui_image_file_t *self)
{
    if (self == NULL || self->decoder == NULL || self->decoder->name == NULL || self->decoder->name[0] == '\0')
    {
        return NULL;
    }

    return self->decoder->name;
}

const char *egui_image_file_status_to_string(egui_image_file_status_t status)
{
    switch (status)
    {
    case EGUI_IMAGE_FILE_STATUS_READY:
        return "ready";
    case EGUI_IMAGE_FILE_STATUS_NO_PATH:
        return "no_path";
    case EGUI_IMAGE_FILE_STATUS_NO_IO:
        return "no_io";
    case EGUI_IMAGE_FILE_STATUS_NO_DECODER:
        return "no_decoder";
    case EGUI_IMAGE_FILE_STATUS_OPEN_FILE_FAIL:
        return "open_file_fail";
    case EGUI_IMAGE_FILE_STATUS_OPEN_DECODER_FAIL:
        return "open_decoder_fail";
    case EGUI_IMAGE_FILE_STATUS_DECODE_ROW_FAIL:
        return "decode_row_fail";
    case EGUI_IMAGE_FILE_STATUS_OOM:
        return "oom";
    case EGUI_IMAGE_FILE_STATUS_IDLE:
    default:
        return "idle";
    }
}

#endif /* EGUI_CONFIG_FUNCTION_IMAGE_FILE */
