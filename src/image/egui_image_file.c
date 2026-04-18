#include <string.h>

#include "egui_image_file.h"
#include "egui_image_std.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE

#ifndef EGUI_IMAGE_FILE_FAST_PATH_ENABLE
#define EGUI_IMAGE_FILE_FAST_PATH_ENABLE 1
#endif

#ifndef EGUI_IMAGE_FILE_STREAM_BAND_MAX_BYTES
#define EGUI_IMAGE_FILE_STREAM_BAND_MAX_BYTES 8192
#endif

#ifndef EGUI_IMAGE_FILE_RESIZE_COLUMN_CACHE_MAX_BYTES
#define EGUI_IMAGE_FILE_RESIZE_COLUMN_CACHE_MAX_BYTES 1024
#endif

#ifndef EGUI_IMAGE_FILE_RESIZE_ROW_CACHE_MAX_BYTES
#define EGUI_IMAGE_FILE_RESIZE_ROW_CACHE_MAX_BYTES 1024
#endif

#define EGUI_IMAGE_FILE_STREAM_BAND_MODE_NONE    0u
#define EGUI_IMAGE_FILE_STREAM_BAND_MODE_ROWS    1u
#define EGUI_IMAGE_FILE_STREAM_BAND_MODE_COLUMNS 2u

static const egui_image_file_io_t *g_egui_image_file_default_io = NULL;
static const egui_image_file_decoder_t *g_egui_image_file_decoders[EGUI_CONFIG_IMAGE_FILE_DECODER_MAX_COUNT];
static uint8_t g_egui_image_file_decoder_count = 0;

static void egui_image_file_api_draw_image_resize(const egui_image_t *base, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height);
static int egui_image_file_load_row(egui_image_file_t *self, uint16_t row);
static uint16_t egui_image_file_resize_axis_map(uint16_t src_extent, egui_dim_t dest_extent, egui_dim_t dest_pos);
static egui_dim_t egui_image_file_resize_axis_run_end(uint16_t src_extent, egui_dim_t dest_extent, uint16_t src_pos);

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

static void egui_image_file_invalidate_stream_band(egui_image_file_t *self)
{
    if (self == NULL)
    {
        return;
    }

    self->stream_band_row_start = 0xFFFFu;
    self->stream_band_row_end = 0u;
    self->stream_band_target_row_start = 0xFFFFu;
    self->stream_band_target_row_end = 0u;
    self->stream_band_col_start = 0xFFFFu;
    self->stream_band_col_end = 0u;
    self->stream_band_valid = 0u;
    self->stream_band_mode = EGUI_IMAGE_FILE_STREAM_BAND_MODE_NONE;
}

static void egui_image_file_invalidate_resize_column_cache(egui_image_file_t *self)
{
    if (self == NULL)
    {
        return;
    }

    self->resize_column_row_start = 0xFFFFu;
    self->resize_column_row_end = 0u;
    self->resize_column_src_col = 0xFFFFu;
    self->resize_column_valid = 0u;
}

static void egui_image_file_invalidate_resize_row_cache(egui_image_file_t *self)
{
    if (self == NULL)
    {
        return;
    }

    self->resize_row_src_y = 0xFFFFu;
    self->resize_row_dest_width = 0u;
    self->resize_row_valid = 0u;
}

static void egui_image_file_release_resize_column_cache(egui_image_file_t *self)
{
    if (self == NULL)
    {
        return;
    }

    if (self->resize_column_pixels != NULL)
    {
        egui_free(self->resize_column_pixels);
        self->resize_column_pixels = NULL;
    }
    if (self->resize_column_alpha != NULL)
    {
        egui_free(self->resize_column_alpha);
        self->resize_column_alpha = NULL;
    }
    self->resize_column_capacity = 0u;
    egui_image_file_invalidate_resize_column_cache(self);
}

static void egui_image_file_release_resize_row_cache(egui_image_file_t *self)
{
    if (self == NULL)
    {
        return;
    }

    if (self->resize_row_pixels != NULL)
    {
        egui_free(self->resize_row_pixels);
        self->resize_row_pixels = NULL;
    }
    self->resize_row_capacity = 0u;
    egui_image_file_invalidate_resize_row_cache(self);
}

static uint16_t egui_image_file_get_stream_band_max_rows(const egui_image_file_t *self)
{
    uint32_t bytes_per_row;
    uint32_t max_rows;

    if (self == NULL || self->width == 0 || EGUI_IMAGE_FILE_STREAM_BAND_MAX_BYTES == 0)
    {
        return 0u;
    }

    bytes_per_row = (uint32_t)self->width * (self->has_alpha ? 3u : 2u);
    if (bytes_per_row == 0u)
    {
        return 0u;
    }

    max_rows = (uint32_t)EGUI_IMAGE_FILE_STREAM_BAND_MAX_BYTES / bytes_per_row;
    if (max_rows == 0u)
    {
        max_rows = 1u;
    }
    if (max_rows > self->height)
    {
        max_rows = self->height;
    }
    if (max_rows > 0xFFFFu)
    {
        max_rows = 0xFFFFu;
    }
    return (uint16_t)max_rows;
}

static uint16_t egui_image_file_get_stream_band_window_end(const egui_image_file_t *self, uint16_t row_start, uint16_t row_limit)
{
    uint16_t max_rows;
    uint32_t row_end;

    if (self == NULL || row_start >= row_limit)
    {
        return row_start;
    }

    max_rows = egui_image_file_get_stream_band_max_rows(self);
    if (max_rows == 0u)
    {
        return row_start;
    }

    row_end = (uint32_t)row_start + (uint32_t)max_rows;
    if (row_end > row_limit)
    {
        row_end = row_limit;
    }
    return (uint16_t)row_end;
}

static uint16_t egui_image_file_get_stream_column_band_max_cols(const egui_image_file_t *self, uint16_t row_count)
{
    uint32_t bytes_per_col;
    uint32_t max_cols;

    if (self == NULL || self->width == 0 || row_count == 0 || EGUI_IMAGE_FILE_STREAM_BAND_MAX_BYTES == 0)
    {
        return 0u;
    }

    bytes_per_col = (uint32_t)row_count * (self->has_alpha ? 3u : 2u);
    if (bytes_per_col == 0u)
    {
        return 0u;
    }

    max_cols = (uint32_t)EGUI_IMAGE_FILE_STREAM_BAND_MAX_BYTES / bytes_per_col;
    if (max_cols == 0u)
    {
        max_cols = 1u;
    }
    if (max_cols > self->width)
    {
        max_cols = self->width;
    }
    if (max_cols > 0xFFFFu)
    {
        max_cols = 0xFFFFu;
    }
    return (uint16_t)max_cols;
}

static int egui_image_file_should_use_stream_column_band(const egui_image_file_t *self, egui_dim_t visible_width, egui_dim_t row_count)
{
    return self != NULL && self->keep_file_open != 0u && visible_width == 1 &&
           (row_count >= 16 || row_count > (egui_dim_t)egui_image_file_get_stream_band_max_rows(self));
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
    self->keep_file_open = 0;
    egui_image_file_invalidate_row_cache(self);
    egui_image_file_invalidate_stream_band(self);
    egui_image_file_invalidate_resize_column_cache(self);
    egui_image_file_invalidate_resize_row_cache(self);
}

static int egui_image_file_has_intrinsic_resize(const egui_image_file_t *self)
{
    return self != NULL && self->resize_enabled != 0U && self->resize_width > 0U && self->resize_height > 0U;
}

static int egui_image_file_get_intrinsic_resize(const egui_image_file_t *self, egui_dim_t *width, egui_dim_t *height)
{
    if (!egui_image_file_has_intrinsic_resize(self))
    {
        return 0;
    }

    if (width != NULL)
    {
        *width = self->resize_width;
    }
    if (height != NULL)
    {
        *height = self->resize_height;
    }
    return 1;
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

static int egui_image_file_prepare_stream_band_buffers(egui_image_file_t *self, uint16_t row_count)
{
    uint32_t pixel_count;
    uint32_t pixel_bytes;
    uint16_t *new_pixels = self->stream_band_pixels;
    uint8_t *new_alpha = self->stream_band_alpha;

    if (self == NULL || self->width == 0 || row_count == 0)
    {
        return 0;
    }

    pixel_count = (uint32_t)self->width * (uint32_t)row_count;
    pixel_bytes = pixel_count * (uint32_t)sizeof(uint16_t);
    if (pixel_count == 0 || pixel_bytes > 0x7FFFFFFFu || pixel_count > 0x7FFFFFFFu)
    {
        return 0;
    }

    if (self->stream_band_pixel_capacity < pixel_count)
    {
        new_pixels = (uint16_t *)egui_malloc((int)pixel_bytes);
        if (new_pixels == NULL)
        {
            return 0;
        }
    }

    if (self->has_alpha)
    {
        if (self->stream_band_alpha == NULL || self->stream_band_pixel_capacity < pixel_count)
        {
            new_alpha = (uint8_t *)egui_malloc((int)pixel_count);
            if (new_alpha == NULL)
            {
                if (new_pixels != self->stream_band_pixels)
                {
                    egui_free(new_pixels);
                }
                return 0;
            }
        }
    }
    else if (self->stream_band_alpha != NULL)
    {
        egui_free(self->stream_band_alpha);
        self->stream_band_alpha = NULL;
    }

    if (new_pixels != self->stream_band_pixels)
    {
        if (self->stream_band_pixels != NULL)
        {
            egui_free(self->stream_band_pixels);
        }
        self->stream_band_pixels = new_pixels;
        self->stream_band_pixel_capacity = pixel_count;
    }

    if (self->has_alpha && new_alpha != self->stream_band_alpha)
    {
        if (self->stream_band_alpha != NULL)
        {
            egui_free(self->stream_band_alpha);
        }
        self->stream_band_alpha = new_alpha;
    }

    return 1;
}

static int egui_image_file_prepare_resize_column_cache_buffers(egui_image_file_t *self, uint16_t row_count)
{
    uint16_t *new_pixels = self->resize_column_pixels;
    uint8_t *new_alpha = self->resize_column_alpha;
    uint32_t pixel_bytes;

    if (self == NULL || row_count == 0)
    {
        return 0;
    }

    pixel_bytes = (uint32_t)row_count * (uint32_t)(self->has_alpha ? 3u : 2u);
    if (pixel_bytes == 0u || pixel_bytes > (uint32_t)EGUI_IMAGE_FILE_RESIZE_COLUMN_CACHE_MAX_BYTES)
    {
        return 0;
    }

    egui_image_file_release_resize_row_cache(self);

    if (self->resize_column_capacity < row_count)
    {
        new_pixels = (uint16_t *)egui_malloc((int)((uint32_t)row_count * sizeof(uint16_t)));
        if (new_pixels == NULL)
        {
            return 0;
        }
    }

    if (self->has_alpha)
    {
        if (self->resize_column_alpha == NULL || self->resize_column_capacity < row_count)
        {
            new_alpha = (uint8_t *)egui_malloc((int)row_count);
            if (new_alpha == NULL)
            {
                if (new_pixels != self->resize_column_pixels)
                {
                    egui_free(new_pixels);
                }
                return 0;
            }
        }
    }
    else if (self->resize_column_alpha != NULL)
    {
        egui_free(self->resize_column_alpha);
        self->resize_column_alpha = NULL;
    }

    if (new_pixels != self->resize_column_pixels)
    {
        if (self->resize_column_pixels != NULL)
        {
            egui_free(self->resize_column_pixels);
        }
        self->resize_column_pixels = new_pixels;
        self->resize_column_capacity = row_count;
    }

    if (self->has_alpha && new_alpha != self->resize_column_alpha)
    {
        if (self->resize_column_alpha != NULL)
        {
            egui_free(self->resize_column_alpha);
        }
        self->resize_column_alpha = new_alpha;
    }

    return 1;
}

static int egui_image_file_prepare_resize_row_cache_buffers(egui_image_file_t *self, uint16_t width)
{
    uint16_t *new_pixels = self->resize_row_pixels;
    uint32_t pixel_bytes;

    if (self == NULL || width == 0u || self->has_alpha)
    {
        return 0;
    }

    pixel_bytes = (uint32_t)width * (uint32_t)sizeof(uint16_t);
    if (pixel_bytes == 0u || pixel_bytes > (uint32_t)EGUI_IMAGE_FILE_RESIZE_ROW_CACHE_MAX_BYTES)
    {
        return 0;
    }

    egui_image_file_release_resize_column_cache(self);

    if (self->resize_row_capacity >= width && self->resize_row_pixels != NULL)
    {
        return 1;
    }

    new_pixels = (uint16_t *)egui_malloc((int)pixel_bytes);
    if (new_pixels == NULL)
    {
        return 0;
    }

    if (self->resize_row_pixels != NULL)
    {
        egui_free(self->resize_row_pixels);
    }
    self->resize_row_pixels = new_pixels;
    self->resize_row_capacity = width;
    return 1;
}

static int egui_image_file_stream_band_contains_row(const egui_image_file_t *self, uint16_t row)
{
    if (self == NULL || self->stream_band_valid == 0u || self->stream_band_mode != EGUI_IMAGE_FILE_STREAM_BAND_MODE_ROWS)
    {
        return 0;
    }

    return row >= self->stream_band_row_start && row < self->stream_band_row_end;
}

static int egui_image_file_stream_band_contains_column_window(const egui_image_file_t *self, uint16_t col, uint16_t row_start, uint16_t row_end)
{
    if (self == NULL || self->stream_band_valid == 0u || self->stream_band_mode != EGUI_IMAGE_FILE_STREAM_BAND_MODE_COLUMNS)
    {
        return 0;
    }

    return row_start >= self->stream_band_row_start && row_end <= self->stream_band_row_end && col >= self->stream_band_col_start &&
           col < self->stream_band_col_end;
}

static const uint16_t *egui_image_file_get_stream_band_row_pixels_ptr(const egui_image_file_t *self, uint16_t row)
{
    uint32_t row_offset;

    if (self == NULL || self->stream_band_pixels == NULL || !egui_image_file_stream_band_contains_row(self, row))
    {
        return NULL;
    }

    row_offset = (uint32_t)(row - self->stream_band_row_start) * (uint32_t)self->width;
    return &self->stream_band_pixels[row_offset];
}

static const uint8_t *egui_image_file_get_stream_band_row_alpha_ptr(const egui_image_file_t *self, uint16_t row)
{
    uint32_t row_offset;

    if (self == NULL || self->stream_band_alpha == NULL || !egui_image_file_stream_band_contains_row(self, row))
    {
        return NULL;
    }

    row_offset = (uint32_t)(row - self->stream_band_row_start) * (uint32_t)self->width;
    return &self->stream_band_alpha[row_offset];
}

static uint16_t egui_image_file_get_stream_band_column_window_row_count(const egui_image_file_t *self)
{
    if (self == NULL || self->stream_band_mode != EGUI_IMAGE_FILE_STREAM_BAND_MODE_COLUMNS || self->stream_band_row_end <= self->stream_band_row_start)
    {
        return 0u;
    }

    return (uint16_t)(self->stream_band_row_end - self->stream_band_row_start);
}

static const uint16_t *egui_image_file_get_stream_band_column_window_col_pixels_ptr(const egui_image_file_t *self, uint16_t col)
{
    uint16_t row_count;
    uint32_t col_offset;

    if (self == NULL || self->stream_band_pixels == NULL || self->stream_band_mode != EGUI_IMAGE_FILE_STREAM_BAND_MODE_COLUMNS ||
        col < self->stream_band_col_start || col >= self->stream_band_col_end)
    {
        return NULL;
    }

    row_count = egui_image_file_get_stream_band_column_window_row_count(self);
    if (row_count == 0u)
    {
        return NULL;
    }

    col_offset = (uint32_t)(col - self->stream_band_col_start) * (uint32_t)row_count;
    return &self->stream_band_pixels[col_offset];
}

static const uint8_t *egui_image_file_get_stream_band_column_window_col_alpha_ptr(const egui_image_file_t *self, uint16_t col)
{
    uint16_t row_count;
    uint32_t col_offset;

    if (self == NULL || self->stream_band_alpha == NULL || self->stream_band_mode != EGUI_IMAGE_FILE_STREAM_BAND_MODE_COLUMNS ||
        col < self->stream_band_col_start || col >= self->stream_band_col_end)
    {
        return NULL;
    }

    row_count = egui_image_file_get_stream_band_column_window_row_count(self);
    if (row_count == 0u)
    {
        return NULL;
    }

    col_offset = (uint32_t)(col - self->stream_band_col_start) * (uint32_t)row_count;
    return &self->stream_band_alpha[col_offset];
}

static const uint16_t *egui_image_file_get_cached_row_pixels_ptr(const egui_image_file_t *self)
{
    if (self == NULL || self->row_cache_valid == 0u)
    {
        return NULL;
    }

    if (egui_image_file_stream_band_contains_row(self, self->cached_row))
    {
        return egui_image_file_get_stream_band_row_pixels_ptr(self, self->cached_row);
    }

    return self->row_pixels;
}

static const uint8_t *egui_image_file_get_cached_row_alpha_ptr(const egui_image_file_t *self)
{
    if (self == NULL || self->has_alpha == 0u || self->row_cache_valid == 0u)
    {
        return NULL;
    }

    if (egui_image_file_stream_band_contains_row(self, self->cached_row))
    {
        return egui_image_file_get_stream_band_row_alpha_ptr(self, self->cached_row);
    }

    return self->row_alpha;
}

static int egui_image_file_fill_stream_band(egui_image_file_t *self, uint16_t row_start, uint16_t row_end)
{
    uint16_t row;
    uint16_t row_count;
    uint32_t row_width;

    if (self == NULL || row_end <= row_start || row_end > self->height || self->decoder == NULL || self->decoder->read_row == NULL ||
        self->stream_band_pixels == NULL)
    {
        return 0;
    }

    row_count = (uint16_t)(row_end - row_start);
    row_width = self->width;

    for (row = 0; row < row_count; row++)
    {
        uint32_t row_offset = (uint32_t)row * row_width;
        uint8_t *alpha_row = NULL;

        if (self->has_alpha)
        {
            if (self->stream_band_alpha == NULL)
            {
                return 0;
            }
            alpha_row = &self->stream_band_alpha[row_offset];
        }

        if (!self->decoder->read_row(self->decoder_ctx, (uint16_t)(row_start + row), &self->stream_band_pixels[row_offset], alpha_row))
        {
            egui_image_file_close_runtime(self);
            self->status = EGUI_IMAGE_FILE_STATUS_DECODE_ROW_FAIL;
            return 0;
        }
    }

    self->stream_band_row_start = row_start;
    self->stream_band_row_end = row_end;
    self->stream_band_col_start = 0u;
    self->stream_band_col_end = self->width;
    self->stream_band_valid = 1u;
    self->stream_band_mode = EGUI_IMAGE_FILE_STREAM_BAND_MODE_ROWS;
    egui_image_file_invalidate_row_cache(self);
    return 1;
}

static int egui_image_file_prepare_stream_column_band_for_column(egui_image_file_t *self, uint16_t col, uint16_t row_start, uint16_t row_end)
{
    uint16_t row_count;
    uint16_t col_count;
    uint16_t col_end;
    uint16_t row;

    if (self == NULL || self->keep_file_open == 0u || col >= self->width || row_end <= row_start || row_end > self->height || self->decoder == NULL ||
        self->decoder->read_row == NULL)
    {
        return 0;
    }
    if (egui_image_file_stream_band_contains_column_window(self, col, row_start, row_end))
    {
        return 1;
    }

    row_count = (uint16_t)(row_end - row_start);
    col_count = egui_image_file_get_stream_column_band_max_cols(self, row_count);
    if (col_count == 0u)
    {
        return 0;
    }
    col_end = (uint16_t)EGUI_MIN((uint32_t)self->width, (uint32_t)col + (uint32_t)col_count);
    col_count = (uint16_t)(col_end - col);
    if (col_count == 0u)
    {
        return 0;
    }
    if (!egui_image_file_prepare_stream_band_buffers(self, (uint16_t)(row_count * col_count)))
    {
        return 0;
    }
    if (!egui_image_file_prepare_row_buffers(self))
    {
        return 0;
    }

    for (row = 0; row < row_count; row++)
    {
        if (!self->decoder->read_row(self->decoder_ctx, (uint16_t)(row_start + row), self->row_pixels, self->has_alpha ? self->row_alpha : NULL))
        {
            egui_image_file_close_runtime(self);
            self->status = EGUI_IMAGE_FILE_STATUS_DECODE_ROW_FAIL;
            return 0;
        }

        for (uint16_t col_index = 0; col_index < col_count; col_index++)
        {
            uint32_t col_offset = (uint32_t)col_index * (uint32_t)row_count + (uint32_t)row;

            self->stream_band_pixels[col_offset] = self->row_pixels[col + col_index];
            if (self->has_alpha)
            {
                self->stream_band_alpha[col_offset] = self->row_alpha[col + col_index];
            }
        }
    }

    self->stream_band_row_start = row_start;
    self->stream_band_row_end = row_end;
    self->stream_band_col_start = col;
    self->stream_band_col_end = col_end;
    self->stream_band_valid = 1u;
    self->stream_band_mode = EGUI_IMAGE_FILE_STREAM_BAND_MODE_COLUMNS;
    egui_image_file_invalidate_row_cache(self);
    self->status = EGUI_IMAGE_FILE_STATUS_READY;
    return 1;
}

static int egui_image_file_fill_stream_band_window(egui_image_file_t *self, uint16_t row)
{
    uint16_t row_start;
    uint16_t row_end;

    if (self == NULL || self->keep_file_open == 0u)
    {
        return 0;
    }
    if (self->stream_band_target_row_end <= self->stream_band_target_row_start)
    {
        return 0;
    }
    if (row < self->stream_band_target_row_start)
    {
        row = self->stream_band_target_row_start;
    }
    if (row >= self->stream_band_target_row_end)
    {
        return 0;
    }

    row_start = row;
    row_end = egui_image_file_get_stream_band_window_end(self, row_start, self->stream_band_target_row_end);
    if (row_end <= row_start)
    {
        return 0;
    }
    if (self->stream_band_valid != 0u && row_start >= self->stream_band_row_start && row_end <= self->stream_band_row_end)
    {
        return 1;
    }
    if (!egui_image_file_prepare_stream_band_buffers(self, (uint16_t)(row_end - row_start)))
    {
        return 0;
    }

    return egui_image_file_fill_stream_band(self, row_start, row_end);
}

static int egui_image_file_ensure_stream_band_row(egui_image_file_t *self, uint16_t row)
{
    if (self == NULL)
    {
        return 0;
    }
    if (egui_image_file_stream_band_contains_row(self, row))
    {
        return 1;
    }

    return egui_image_file_fill_stream_band_window(self, row) && egui_image_file_stream_band_contains_row(self, row);
}

static void egui_image_file_prepare_stream_band_for_rows(egui_image_file_t *self, uint16_t row_start, uint16_t row_end)
{
    if (self == NULL || self->keep_file_open == 0u || row_end <= row_start)
    {
        return;
    }

    if (row_end > self->height)
    {
        row_end = self->height;
    }
    if (row_start >= row_end)
    {
        return;
    }

    self->stream_band_target_row_start = row_start;
    self->stream_band_target_row_end = row_end;

    if ((uint16_t)(row_end - row_start) <= 1u)
    {
        return;
    }

    (void)egui_image_file_fill_stream_band_window(self, row_start);
}

static int egui_image_file_load_row_from_stream_band(egui_image_file_t *self, uint16_t row)
{
    if (self == NULL || !egui_image_file_stream_band_contains_row(self, row))
    {
        return 0;
    }

    self->cached_row = row;
    self->row_cache_valid = 1u;
    self->status = EGUI_IMAGE_FILE_STATUS_READY;
    return 1;
}

static int egui_image_file_prepare_resize_column_cache(egui_image_file_t *self, uint16_t src_col, uint16_t row_start, uint16_t row_end)
{
    uint16_t row;
    uint16_t row_count;

    if (self == NULL || src_col >= self->width || row_end <= row_start || row_end > self->height)
    {
        return 0;
    }
    if (self->resize_column_valid != 0u && self->resize_column_src_col == src_col && self->resize_column_row_start == row_start &&
        self->resize_column_row_end == row_end)
    {
        return 1;
    }

    row_count = (uint16_t)(row_end - row_start);
    if (!egui_image_file_prepare_resize_column_cache_buffers(self, row_count))
    {
        return 0;
    }

    for (row = row_start; row < row_end; row++)
    {
        const uint16_t *row_pixels;
        const uint8_t *row_alpha;
        uint16_t row_offset = (uint16_t)(row - row_start);

        if (!egui_image_file_load_row(self, row))
        {
            egui_image_file_invalidate_resize_column_cache(self);
            return 0;
        }

        row_pixels = egui_image_file_get_cached_row_pixels_ptr(self);
        row_alpha = egui_image_file_get_cached_row_alpha_ptr(self);
        if (row_pixels == NULL)
        {
            egui_image_file_invalidate_resize_column_cache(self);
            return 0;
        }

        self->resize_column_pixels[row_offset] = row_pixels[src_col];
        if (self->has_alpha)
        {
            if (row_alpha == NULL)
            {
                egui_image_file_invalidate_resize_column_cache(self);
                return 0;
            }
            self->resize_column_alpha[row_offset] = row_alpha[src_col];
        }
    }

    self->resize_column_row_start = row_start;
    self->resize_column_row_end = row_end;
    self->resize_column_src_col = src_col;
    self->resize_column_valid = 1u;
    return 1;
}

static int egui_image_file_prepare_resize_row_cache(egui_image_file_t *self, uint16_t src_y, egui_dim_t dest_width)
{
    const uint16_t *row_pixels;
    egui_dim_t dest_x = 0;
    uint16_t width;

    if (self == NULL || src_y >= self->height || dest_width <= 0 || dest_width > 0xFFFF || self->has_alpha)
    {
        return 0;
    }
    width = (uint16_t)dest_width;
    if (self->resize_row_valid != 0u && self->resize_row_src_y == src_y && self->resize_row_dest_width == width)
    {
        return 1;
    }
    if (!egui_image_file_prepare_resize_row_cache_buffers(self, width) || !egui_image_file_load_row(self, src_y))
    {
        egui_image_file_invalidate_resize_row_cache(self);
        return 0;
    }

    row_pixels = egui_image_file_get_cached_row_pixels_ptr(self);
    if (row_pixels == NULL)
    {
        egui_image_file_invalidate_resize_row_cache(self);
        return 0;
    }

    while (dest_x < dest_width)
    {
        egui_dim_t dest_x_next;
        uint16_t src_x = egui_image_file_resize_axis_map(self->width, dest_width, dest_x);
        uint16_t pixel = row_pixels[src_x];

        dest_x_next = egui_image_file_resize_axis_run_end(self->width, dest_width, src_x);
        if (dest_x_next <= dest_x)
        {
            dest_x_next = dest_x + 1;
        }
        if (dest_x_next > dest_width)
        {
            dest_x_next = dest_width;
        }

        for (; dest_x < dest_x_next; dest_x++)
        {
            self->resize_row_pixels[dest_x] = pixel;
        }
    }

    self->resize_row_src_y = src_y;
    self->resize_row_dest_width = width;
    self->resize_row_valid = 1u;
    return 1;
}

static uint16_t egui_image_file_resize_axis_map(uint16_t src_extent, egui_dim_t dest_extent, egui_dim_t dest_pos)
{
    uint32_t scaled_pos;
    uint16_t mapped_pos = 0;

    if (src_extent > 0 && dest_extent > 0 && dest_pos > 0)
    {
        scaled_pos = (uint32_t)dest_pos * (uint32_t)src_extent;
        mapped_pos = (uint16_t)(scaled_pos / (uint32_t)dest_extent);
        if (mapped_pos >= src_extent)
        {
            mapped_pos = src_extent - 1;
        }
    }

    return mapped_pos;
}

static egui_dim_t egui_image_file_resize_axis_run_end(uint16_t src_extent, egui_dim_t dest_extent, uint16_t src_pos)
{
    if (src_extent == 0 || dest_extent <= 0)
    {
        return 0;
    }
    if ((uint16_t)(src_pos + 1u) >= src_extent)
    {
        return dest_extent;
    }

    return (egui_dim_t)(((uint32_t)(src_pos + 1u) * (uint32_t)dest_extent + (uint32_t)src_extent - 1u) / (uint32_t)src_extent);
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
        self->keep_file_open = open_result.keep_file_open ? 1u : 0u;
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

static int egui_image_file_decode_row_to_cache(egui_image_file_t *self, uint16_t row)
{
    if (self == NULL || row >= self->height)
    {
        return 0;
    }
    if (!egui_image_file_try_open(self))
    {
        return 0;
    }
    if (self->decoder == NULL || self->decoder->read_row == NULL)
    {
        self->status = EGUI_IMAGE_FILE_STATUS_OPEN_DECODER_FAIL;
        return 0;
    }
    if (!egui_image_file_prepare_row_buffers(self))
    {
        return 0;
    }
    if (!self->decoder->read_row(self->decoder_ctx, row, self->row_pixels, self->has_alpha ? self->row_alpha : NULL))
    {
        egui_image_file_close_runtime(self);
        self->status = EGUI_IMAGE_FILE_STATUS_DECODE_ROW_FAIL;
        return 0;
    }

    self->cached_row = row;
    self->row_cache_valid = 1u;
    self->status = EGUI_IMAGE_FILE_STATUS_READY;
    return 1;
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
    if (self->row_cache_valid && self->cached_row == row)
    {
        return 1;
    }
    if (egui_image_file_load_row_from_stream_band(self, row))
    {
        return 1;
    }
    if (row >= self->stream_band_target_row_start && row < self->stream_band_target_row_end)
    {
        if ((uint16_t)(self->stream_band_target_row_end - self->stream_band_target_row_start) <= 1u)
        {
            return egui_image_file_decode_row_to_cache(self, row);
        }
        if (egui_image_file_ensure_stream_band_row(self, row) && egui_image_file_load_row_from_stream_band(self, row))
        {
            return 1;
        }
    }
    return egui_image_file_decode_row_to_cache(self, row);
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

static int egui_image_file_get_source_point(egui_image_file_t *self, uint16_t src_x, uint16_t src_y, egui_color_t *color, egui_alpha_t *alpha)
{
    const uint16_t *row_pixels;
    const uint8_t *row_alpha;

    if (self == NULL || src_x >= self->width || src_y >= self->height)
    {
        return 0;
    }
    if (!egui_image_file_load_row(self, src_y))
    {
        return 0;
    }

    row_pixels = egui_image_file_get_cached_row_pixels_ptr(self);
    row_alpha = egui_image_file_get_cached_row_alpha_ptr(self);
    if (row_pixels == NULL)
    {
        return 0;
    }

    if (color != NULL)
    {
        color->full = EGUI_COLOR_RGB565_TRANS(row_pixels[src_x]);
    }
    if (alpha != NULL)
    {
        *alpha = (self->has_alpha && row_alpha != NULL) ? row_alpha[src_x] : EGUI_ALPHA_100;
    }
    return 1;
}

static int egui_image_file_get_rgb565_dst_row(egui_dim_t screen_x, egui_dim_t screen_y, egui_color_int_t **dst_row, egui_dim_t *dst_stride)
{
#if EGUI_CONFIG_COLOR_DEPTH == 16
    egui_canvas_t *canvas;

    if (dst_row == NULL || dst_stride == NULL)
    {
        return 0;
    }

    canvas = egui_canvas_get_canvas();
    if (canvas == NULL)
    {
        return 0;
    }

    *dst_stride = canvas->pfb_region.size.width;
    *dst_row = &canvas->pfb[(screen_y - canvas->pfb_location_in_base_view.y) * (*dst_stride) + (screen_x - canvas->pfb_location_in_base_view.x)];
    return 1;
#else
    EGUI_UNUSED(screen_x);
    EGUI_UNUSED(screen_y);
    EGUI_UNUSED(dst_row);
    EGUI_UNUSED(dst_stride);
    return 0;
#endif
}

static int egui_image_file_draw_visible_row_fast(const egui_image_file_t *self, egui_dim_t screen_x_start, egui_dim_t screen_y, egui_dim_t dest_width,
                                                 egui_dim_t src_x_offset)
{
#if !EGUI_IMAGE_FILE_FAST_PATH_ENABLE
    EGUI_UNUSED(self);
    EGUI_UNUSED(screen_x_start);
    EGUI_UNUSED(screen_y);
    EGUI_UNUSED(dest_width);
    EGUI_UNUSED(src_x_offset);
    return 0;
#else
#if EGUI_CONFIG_COLOR_DEPTH == 16
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_color_int_t *dst_row;
    egui_dim_t dst_stride;
    const uint16_t *src_row;
    const uint8_t *src_alpha_row;

    if (self == NULL || dest_width <= 0 || canvas == NULL || !egui_image_file_get_rgb565_dst_row(screen_x_start, screen_y, &dst_row, &dst_stride))
    {
        return 0;
    }
    EGUI_UNUSED(dst_stride);

    src_row = egui_image_file_get_cached_row_pixels_ptr(self);
    src_alpha_row = egui_image_file_get_cached_row_alpha_ptr(self);
    if (src_row == NULL)
    {
        return 0;
    }
    src_row = &src_row[src_x_offset];
    if (canvas->mask != NULL)
    {
        if (!self->has_alpha)
        {
            return egui_image_std_blend_rgb565_masked_row(canvas, dst_row, src_row, dest_width, screen_x_start, screen_y, canvas->alpha);
        }

        egui_image_std_blend_rgb565_alpha8_masked_row(canvas, dst_row, src_row, &src_alpha_row[src_x_offset], dest_width, screen_x_start, screen_y,
                                                      canvas->alpha);
        return 1;
    }

    if (!self->has_alpha)
    {
        if (canvas->alpha == 0)
        {
            return 1;
        }
        if (canvas->alpha == EGUI_ALPHA_100)
        {
            if (dest_width == 1)
            {
                dst_row[0] = src_row[0];
            }
            else
            {
                egui_api_memcpy(dst_row, src_row, (size_t)dest_width * sizeof(uint16_t));
            }
            return 1;
        }

        if (dest_width == 1)
        {
            egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[0], src_row[0], canvas->alpha);
            return 1;
        }

        for (egui_dim_t i = 0; i < dest_width; i++)
        {
            egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[i], canvas->alpha);
        }
        return 1;
    }

    egui_image_std_blend_rgb565_alpha8_masked_row(canvas, dst_row, src_row, &src_alpha_row[src_x_offset], dest_width, screen_x_start, screen_y, canvas->alpha);
    return 1;
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(screen_x_start);
    EGUI_UNUSED(screen_y);
    EGUI_UNUSED(dest_width);
    EGUI_UNUSED(src_x_offset);
    return 0;
#endif
#endif
}

static int egui_image_file_draw_visible_column_fast(egui_image_file_t *self, egui_dim_t screen_x, egui_dim_t screen_y_start, egui_dim_t screen_y_end,
                                                    egui_dim_t src_x, uint16_t src_y_start)
{
#if !EGUI_IMAGE_FILE_FAST_PATH_ENABLE
    EGUI_UNUSED(self);
    EGUI_UNUSED(screen_x);
    EGUI_UNUSED(screen_y_start);
    EGUI_UNUSED(screen_y_end);
    EGUI_UNUSED(src_x);
    EGUI_UNUSED(src_y_start);
    return 0;
#else
#if EGUI_CONFIG_COLOR_DEPTH == 16
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_dim_t row_count = screen_y_end - screen_y_start;
    egui_dim_t screen_y = screen_y_start;
    uint16_t src_y = src_y_start;

    if (self == NULL || src_x < 0 || src_x >= self->width || screen_y_start >= screen_y_end || canvas == NULL || canvas->mask != NULL)
    {
        return 0;
    }
    if (egui_image_file_should_use_stream_column_band(self, 1, row_count))
    {
        egui_color_int_t *dst_row;
        egui_dim_t dst_stride;
        uint16_t src_col = (uint16_t)src_x;
        uint16_t row_end = (uint16_t)(src_y_start + (uint16_t)row_count);
        uint16_t row_offset;
        const uint16_t *src_col_pixels;

        if (!egui_image_file_prepare_stream_column_band_for_column(self, src_col, src_y_start, row_end))
        {
            return 0;
        }
        if (!egui_image_file_get_rgb565_dst_row(screen_x, screen_y_start, &dst_row, &dst_stride))
        {
            return 0;
        }

        row_offset = (uint16_t)(src_y_start - self->stream_band_row_start);
        src_col_pixels = egui_image_file_get_stream_band_column_window_col_pixels_ptr(self, src_col);
        if (src_col_pixels == NULL)
        {
            return 0;
        }
        src_col_pixels += row_offset;

        if (!self->has_alpha)
        {
            if (canvas->alpha == 0)
            {
                return 1;
            }

            if (canvas->alpha == EGUI_ALPHA_100 && dst_stride == 1)
            {
                egui_api_memcpy(dst_row, src_col_pixels, (size_t)row_count * sizeof(uint16_t));
                return 1;
            }

            for (egui_dim_t i = 0; i < row_count; i++, dst_row += dst_stride, src_col_pixels++)
            {
                if (canvas->alpha == EGUI_ALPHA_100)
                {
                    dst_row[0] = src_col_pixels[0];
                }
                else
                {
                    egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[0], src_col_pixels[0], canvas->alpha);
                }
            }
            return 1;
        }

        if (canvas->alpha == 0)
        {
            return 1;
        }

        {
            const uint8_t *src_col_alpha = egui_image_file_get_stream_band_column_window_col_alpha_ptr(self, src_col);

            if (src_col_alpha == NULL)
            {
                return 0;
            }
            src_col_alpha += row_offset;

            for (egui_dim_t i = 0; i < row_count; i++, dst_row += dst_stride, src_col_pixels++, src_col_alpha++)
            {
                egui_alpha_t alpha;

                alpha = src_col_alpha[0];
                if (alpha == 0)
                {
                    continue;
                }
                if (canvas->alpha != EGUI_ALPHA_100)
                {
                    alpha = egui_color_alpha_mix(canvas->alpha, alpha);
                }
                egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[0], src_col_pixels[0], alpha);
            }
        }
        return 1;
    }

    if (!self->has_alpha)
    {
        if (canvas->alpha == 0)
        {
            return 1;
        }

        while (screen_y < screen_y_end)
        {
            egui_color_int_t *dst_row;
            egui_dim_t dst_stride;
            egui_dim_t chunk_rows;

            if (!egui_image_file_ensure_stream_band_row(self, src_y))
            {
                return 0;
            }
            if (!egui_image_file_get_rgb565_dst_row(screen_x, screen_y, &dst_row, &dst_stride))
            {
                return 0;
            }

            chunk_rows = EGUI_MIN(screen_y_end - screen_y, (egui_dim_t)(self->stream_band_row_end - src_y));
            for (egui_dim_t i = 0; i < chunk_rows; i++, dst_row += dst_stride)
            {
                const uint16_t *src_row = egui_image_file_get_stream_band_row_pixels_ptr(self, (uint16_t)(src_y + i));

                if (src_row == NULL)
                {
                    return 0;
                }

                if (canvas->alpha == EGUI_ALPHA_100)
                {
                    dst_row[0] = src_row[src_x];
                }
                else
                {
                    egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[0], src_row[src_x], canvas->alpha);
                }
            }

            screen_y += chunk_rows;
            src_y = (uint16_t)(src_y + (uint16_t)chunk_rows);
        }
        return 1;
    }

    if (canvas->alpha == 0)
    {
        return 1;
    }

    while (screen_y < screen_y_end)
    {
        egui_color_int_t *dst_row;
        egui_dim_t dst_stride;
        egui_dim_t chunk_rows;

        if (!egui_image_file_ensure_stream_band_row(self, src_y))
        {
            return 0;
        }
        if (!egui_image_file_get_rgb565_dst_row(screen_x, screen_y, &dst_row, &dst_stride))
        {
            return 0;
        }

        chunk_rows = EGUI_MIN(screen_y_end - screen_y, (egui_dim_t)(self->stream_band_row_end - src_y));
        for (egui_dim_t i = 0; i < chunk_rows; i++, dst_row += dst_stride)
        {
            const uint16_t *src_row = egui_image_file_get_stream_band_row_pixels_ptr(self, (uint16_t)(src_y + i));
            const uint8_t *src_alpha_row = egui_image_file_get_stream_band_row_alpha_ptr(self, (uint16_t)(src_y + i));
            egui_alpha_t alpha;

            if (src_row == NULL || src_alpha_row == NULL)
            {
                return 0;
            }

            alpha = src_alpha_row[src_x];
            if (alpha == 0)
            {
                continue;
            }
            if (canvas->alpha != EGUI_ALPHA_100)
            {
                alpha = egui_color_alpha_mix(canvas->alpha, alpha);
            }
            egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[0], src_row[src_x], alpha);
        }

        screen_y += chunk_rows;
        src_y = (uint16_t)(src_y + (uint16_t)chunk_rows);
    }
    return 1;
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(screen_x);
    EGUI_UNUSED(screen_y_start);
    EGUI_UNUSED(screen_y_end);
    EGUI_UNUSED(src_x);
    EGUI_UNUSED(src_y_start);
    return 0;
#endif
#endif
}

static int egui_image_file_draw_resize_visible_column_fast(egui_image_file_t *self, egui_dim_t screen_x, egui_dim_t screen_y_start, egui_dim_t screen_y_end,
                                                           egui_dim_t image_x, egui_dim_t image_y, egui_dim_t dest_width, egui_dim_t dest_height)
{
#if !EGUI_IMAGE_FILE_FAST_PATH_ENABLE
    EGUI_UNUSED(self);
    EGUI_UNUSED(screen_x);
    EGUI_UNUSED(screen_y_start);
    EGUI_UNUSED(screen_y_end);
    EGUI_UNUSED(image_x);
    EGUI_UNUSED(image_y);
    EGUI_UNUSED(dest_width);
    EGUI_UNUSED(dest_height);
    return 0;
#else
#if EGUI_CONFIG_COLOR_DEPTH == 16
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_dim_t screen_y = screen_y_start;
    egui_color_int_t *dst_row;
    egui_dim_t dst_stride;
    uint16_t src_x;
    uint16_t src_y_start;
    uint16_t src_y_end;

    if (self == NULL || canvas == NULL || canvas->mask != NULL || dest_width <= 0 || dest_height <= 0 || screen_y_start >= screen_y_end || screen_x < image_x ||
        screen_x >= image_x + dest_width)
    {
        return 0;
    }
    if (canvas->alpha == 0)
    {
        return 1;
    }

    src_x = egui_image_file_resize_axis_map(self->width, dest_width, screen_x - image_x);
    src_y_start = egui_image_file_resize_axis_map(self->height, dest_height, screen_y_start - image_y);
    src_y_end = (uint16_t)(egui_image_file_resize_axis_map(self->height, dest_height, screen_y_end - 1 - image_y) + 1u);
    if (src_y_end <= src_y_start)
    {
        src_y_end = (uint16_t)(src_y_start + 1u);
    }
    if (src_y_end > self->height)
    {
        src_y_end = self->height;
    }
    if (src_y_start >= src_y_end || !egui_image_file_prepare_resize_column_cache(self, src_x, src_y_start, src_y_end))
    {
        return 0;
    }
    if (!egui_image_file_get_rgb565_dst_row(screen_x, screen_y_start, &dst_row, &dst_stride))
    {
        return 0;
    }

    while (screen_y < screen_y_end)
    {
        egui_dim_t screen_y_next;
        egui_dim_t run_height;
        uint16_t src_y = egui_image_file_resize_axis_map(self->height, dest_height, screen_y - image_y);
        uint16_t row_offset;
        egui_alpha_t alpha = EGUI_ALPHA_100;
        uint16_t pixel;

        if (src_y < self->resize_column_row_start || src_y >= self->resize_column_row_end)
        {
            return 0;
        }

        row_offset = (uint16_t)(src_y - self->resize_column_row_start);
        pixel = self->resize_column_pixels[row_offset];
        if (self->has_alpha)
        {
            alpha = self->resize_column_alpha[row_offset];
            if (alpha == 0)
            {
                screen_y_next = image_y + egui_image_file_resize_axis_run_end(self->height, dest_height, src_y);
                if (screen_y_next <= screen_y)
                {
                    screen_y_next = screen_y + 1;
                }
                if (screen_y_next > screen_y_end)
                {
                    screen_y_next = screen_y_end;
                }
                dst_row += (screen_y_next - screen_y) * dst_stride;
                screen_y = screen_y_next;
                continue;
            }
        }

        if (canvas->alpha != EGUI_ALPHA_100)
        {
            alpha = egui_color_alpha_mix(canvas->alpha, alpha);
        }

        screen_y_next = image_y + egui_image_file_resize_axis_run_end(self->height, dest_height, src_y);
        if (screen_y_next <= screen_y)
        {
            screen_y_next = screen_y + 1;
        }
        if (screen_y_next > screen_y_end)
        {
            screen_y_next = screen_y_end;
        }
        run_height = screen_y_next - screen_y;

        if (alpha == EGUI_ALPHA_100)
        {
            for (egui_dim_t row = 0; row < run_height; row++, dst_row += dst_stride)
            {
                dst_row[0] = pixel;
            }
        }
        else
        {
            for (egui_dim_t row = 0; row < run_height; row++, dst_row += dst_stride)
            {
                egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[0], pixel, alpha);
            }
        }

        screen_y = screen_y_next;
    }
    return 1;
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(screen_x);
    EGUI_UNUSED(screen_y_start);
    EGUI_UNUSED(screen_y_end);
    EGUI_UNUSED(image_x);
    EGUI_UNUSED(image_y);
    EGUI_UNUSED(dest_width);
    EGUI_UNUSED(dest_height);
    return 0;
#endif
#endif
}

static int egui_image_file_draw_visible_block_fast(egui_image_file_t *self, egui_dim_t screen_x_start, egui_dim_t screen_y_start, egui_dim_t screen_x_end,
                                                   egui_dim_t screen_y_end, egui_dim_t src_x_offset, uint16_t src_y_start)
{
#if !EGUI_IMAGE_FILE_FAST_PATH_ENABLE
    EGUI_UNUSED(self);
    EGUI_UNUSED(screen_x_start);
    EGUI_UNUSED(screen_y_start);
    EGUI_UNUSED(screen_x_end);
    EGUI_UNUSED(screen_y_end);
    EGUI_UNUSED(src_x_offset);
    EGUI_UNUSED(src_y_start);
    return 0;
#else
#if EGUI_CONFIG_COLOR_DEPTH == 16
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_dim_t screen_y = screen_y_start;
    uint16_t src_y = src_y_start;
    egui_dim_t row_count = screen_y_end - screen_y_start;
    egui_dim_t count = screen_x_end - screen_x_start;

    if (self == NULL || count <= 1 || row_count <= 1 || src_x_offset < 0 || src_x_offset + count > self->width || canvas == NULL)
    {
        return 0;
    }

    if (canvas->mask != NULL)
    {
        while (screen_y < screen_y_end)
        {
            egui_color_int_t *dst_row;
            egui_dim_t dst_stride;
            egui_dim_t chunk_rows;
            const uint16_t *src_row = NULL;
            const uint8_t *src_alpha_row = NULL;

            if (!egui_image_file_ensure_stream_band_row(self, src_y))
            {
                return 0;
            }
            if (!egui_image_file_get_rgb565_dst_row(screen_x_start, screen_y, &dst_row, &dst_stride))
            {
                return 0;
            }

            chunk_rows = EGUI_MIN(screen_y_end - screen_y, (egui_dim_t)(self->stream_band_row_end - src_y));
            src_row = egui_image_file_get_stream_band_row_pixels_ptr(self, src_y);
            if (src_row == NULL)
            {
                return 0;
            }

            if (!self->has_alpha)
            {
                if (!egui_image_std_blend_rgb565_masked_row_block(canvas, dst_row, dst_stride, src_row + src_x_offset, self->width, chunk_rows, count,
                                                                  screen_x_start, screen_y, canvas->alpha))
                {
                    return 0;
                }
            }
            else
            {
                src_alpha_row = egui_image_file_get_stream_band_row_alpha_ptr(self, src_y);
                if (src_alpha_row == NULL)
                {
                    return 0;
                }
                if (!egui_image_std_blend_rgb565_alpha8_masked_row_block(canvas, dst_row, dst_stride, src_row + src_x_offset, self->width,
                                                                         src_alpha_row + src_x_offset, self->width, chunk_rows, count, screen_x_start, screen_y,
                                                                         canvas->alpha))
                {
                    return 0;
                }
            }

            screen_y += chunk_rows;
            src_y = (uint16_t)(src_y + (uint16_t)chunk_rows);
        }
        return 1;
    }

    if (!self->has_alpha)
    {
        if (canvas->alpha == 0)
        {
            return 1;
        }

        if (canvas->alpha == EGUI_ALPHA_100 && self->stream_band_valid != 0u && self->stream_band_mode == EGUI_IMAGE_FILE_STREAM_BAND_MODE_ROWS &&
            self->stream_band_row_start == src_y_start && self->stream_band_row_end > src_y_start &&
            self->stream_band_row_end < (uint16_t)(src_y_start + (uint16_t)row_count))
        {
            egui_color_int_t *dst_row;
            egui_dim_t dst_stride;
            egui_dim_t anchored_rows = (egui_dim_t)(self->stream_band_row_end - src_y_start);
            const uint16_t *src_row;

            if (!egui_image_file_get_rgb565_dst_row(screen_x_start, screen_y, &dst_row, &dst_stride))
            {
                return 0;
            }

            src_row = egui_image_file_get_stream_band_row_pixels_ptr(self, src_y);
            if (src_row == NULL)
            {
                return 0;
            }
            src_row += src_x_offset;

            for (egui_dim_t row = 0; row < anchored_rows; row++, dst_row += dst_stride, src_row += self->width)
            {
                egui_api_memcpy(dst_row, src_row, (size_t)count * sizeof(uint16_t));
            }

            screen_y += anchored_rows;
            src_y = (uint16_t)(src_y + (uint16_t)anchored_rows);
            while (screen_y < screen_y_end)
            {
                const uint16_t *tail_row;

                if (!egui_image_file_decode_row_to_cache(self, src_y))
                {
                    return 0;
                }
                tail_row = egui_image_file_get_cached_row_pixels_ptr(self);
                if (tail_row == NULL)
                {
                    return 0;
                }
                egui_api_memcpy(dst_row, tail_row + src_x_offset, (size_t)count * sizeof(uint16_t));
                dst_row += dst_stride;
                screen_y++;
                src_y++;
            }
            return 1;
        }

        while (screen_y < screen_y_end)
        {
            egui_color_int_t *dst_row;
            egui_dim_t dst_stride;
            egui_dim_t chunk_rows;
            const uint16_t *src_row;

            if (!egui_image_file_ensure_stream_band_row(self, src_y))
            {
                return 0;
            }
            if (!egui_image_file_get_rgb565_dst_row(screen_x_start, screen_y, &dst_row, &dst_stride))
            {
                return 0;
            }

            chunk_rows = EGUI_MIN(screen_y_end - screen_y, (egui_dim_t)(self->stream_band_row_end - src_y));
            src_row = egui_image_file_get_stream_band_row_pixels_ptr(self, src_y);
            if (src_row == NULL)
            {
                return 0;
            }
            src_row += src_x_offset;

            if (canvas->alpha == EGUI_ALPHA_100)
            {
                for (egui_dim_t row = 0; row < chunk_rows; row++, dst_row += dst_stride, src_row += self->width)
                {
                    egui_api_memcpy(dst_row, src_row, (size_t)count * sizeof(uint16_t));
                }
            }
            else
            {
                for (egui_dim_t row = 0; row < chunk_rows; row++, dst_row += dst_stride, src_row += self->width)
                {
                    for (egui_dim_t i = 0; i < count; i++)
                    {
                        egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[i], canvas->alpha);
                    }
                }
            }

            screen_y += chunk_rows;
            src_y = (uint16_t)(src_y + (uint16_t)chunk_rows);
        }
        return 1;
    }

    while (screen_y < screen_y_end)
    {
        egui_color_int_t *dst_row;
        egui_dim_t dst_stride;
        egui_dim_t chunk_rows;
        const uint16_t *src_row;
        const uint8_t *src_alpha_row;

        if (!egui_image_file_ensure_stream_band_row(self, src_y))
        {
            return 0;
        }
        if (!egui_image_file_get_rgb565_dst_row(screen_x_start, screen_y, &dst_row, &dst_stride))
        {
            return 0;
        }

        chunk_rows = EGUI_MIN(screen_y_end - screen_y, (egui_dim_t)(self->stream_band_row_end - src_y));
        src_row = egui_image_file_get_stream_band_row_pixels_ptr(self, src_y);
        src_alpha_row = egui_image_file_get_stream_band_row_alpha_ptr(self, src_y);
        if (src_row == NULL || src_alpha_row == NULL)
        {
            return 0;
        }

        src_row += src_x_offset;
        src_alpha_row += src_x_offset;
        for (egui_dim_t row = 0; row < chunk_rows; row++, dst_row += dst_stride, src_row += self->width, src_alpha_row += self->width)
        {
            egui_image_std_blend_rgb565_alpha8_masked_row(canvas, dst_row, src_row, src_alpha_row, count, screen_x_start, screen_y + row, canvas->alpha);
        }

        screen_y += chunk_rows;
        src_y = (uint16_t)(src_y + (uint16_t)chunk_rows);
    }
    return 1;
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(screen_x_start);
    EGUI_UNUSED(screen_y_start);
    EGUI_UNUSED(screen_x_end);
    EGUI_UNUSED(screen_y_end);
    EGUI_UNUSED(src_x_offset);
    EGUI_UNUSED(src_y_start);
    return 0;
#endif
#endif
}

static void egui_image_file_draw_visible_row(const egui_image_file_t *self, egui_dim_t screen_x_start, egui_dim_t screen_y, egui_dim_t dest_width,
                                             egui_dim_t src_x_offset)
{
    egui_mask_t *mask = egui_canvas_get_mask();
    egui_dim_t visible_count = dest_width;
    egui_dim_t i;
    const uint16_t *row_pixels;
    const uint8_t *row_alpha;

    if (egui_image_file_draw_visible_row_fast(self, screen_x_start, screen_y, dest_width, src_x_offset))
    {
        return;
    }

    row_pixels = egui_image_file_get_cached_row_pixels_ptr(self);
    row_alpha = egui_image_file_get_cached_row_alpha_ptr(self);
    if (row_pixels == NULL)
    {
        return;
    }

    if (!self->has_alpha)
    {
        if (mask == NULL)
        {
            for (i = 0; i < visible_count; i++)
            {
                egui_color_t color;

                color.full = EGUI_COLOR_RGB565_TRANS(row_pixels[src_x_offset + i]);
                egui_canvas_draw_point_limit_skip_mask(screen_x_start + i, screen_y, color, EGUI_ALPHA_100);
            }
        }
        else
        {
            for (i = 0; i < visible_count; i++)
            {
                egui_color_t color;

                color.full = EGUI_COLOR_RGB565_TRANS(row_pixels[src_x_offset + i]);
                egui_canvas_draw_point_limit(screen_x_start + i, screen_y, color, EGUI_ALPHA_100);
            }
        }
        return;
    }

    if (mask == NULL)
    {
        for (i = 0; i < visible_count; i++)
        {
            egui_dim_t src_x = src_x_offset + i;
            egui_alpha_t alpha = row_alpha[src_x];
            egui_color_t color;

            if (alpha == 0)
            {
                continue;
            }
            color.full = EGUI_COLOR_RGB565_TRANS(row_pixels[src_x]);
            egui_canvas_draw_point_limit_skip_mask(screen_x_start + i, screen_y, color, alpha);
        }
    }
    else
    {
        for (i = 0; i < visible_count; i++)
        {
            egui_dim_t src_x = src_x_offset + i;
            egui_alpha_t alpha = row_alpha[src_x];
            egui_color_t color;

            if (alpha == 0)
            {
                continue;
            }
            color.full = EGUI_COLOR_RGB565_TRANS(row_pixels[src_x]);
            egui_canvas_draw_point_limit(screen_x_start + i, screen_y, color, alpha);
        }
    }
}

static void egui_image_file_blend_resize_row_nomask(egui_color_int_t *dst_row, const egui_image_file_t *self, egui_dim_t screen_x_start,
                                                    egui_dim_t screen_x_end, egui_dim_t image_x, egui_dim_t dest_width, egui_alpha_t canvas_alpha)
{
    egui_dim_t screen_x = screen_x_start;
    const uint16_t *row_pixels = egui_image_file_get_cached_row_pixels_ptr(self);
    const uint8_t *row_alpha = egui_image_file_get_cached_row_alpha_ptr(self);

    if (row_pixels == NULL)
    {
        return;
    }

    while (screen_x < screen_x_end)
    {
        egui_dim_t screen_x_next;
        egui_dim_t run_width;
        egui_dim_t dst_offset = screen_x - screen_x_start;
        uint16_t src_x = egui_image_file_resize_axis_map(self->width, dest_width, screen_x - image_x);
        egui_alpha_t alpha = (self->has_alpha && row_alpha != NULL) ? row_alpha[src_x] : EGUI_ALPHA_100;
        uint16_t pixel = row_pixels[src_x];

        screen_x_next = image_x + egui_image_file_resize_axis_run_end(self->width, dest_width, src_x);
        if (screen_x_next <= screen_x)
        {
            screen_x_next = screen_x + 1;
        }
        if (screen_x_next > screen_x_end)
        {
            screen_x_next = screen_x_end;
        }
        run_width = screen_x_next - screen_x;

        if (alpha != 0)
        {
            if (canvas_alpha != EGUI_ALPHA_100)
            {
                alpha = egui_color_alpha_mix(canvas_alpha, alpha);
            }

            if (alpha == EGUI_ALPHA_100)
            {
                for (egui_dim_t i = 0; i < run_width; i++)
                {
                    dst_row[dst_offset + i] = pixel;
                }
            }
            else if (alpha != 0)
            {
                for (egui_dim_t i = 0; i < run_width; i++)
                {
                    egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[dst_offset + i], pixel, alpha);
                }
            }
        }

        screen_x = screen_x_next;
    }
}

static void egui_image_file_blend_resize_row_cached_nomask(egui_color_int_t *dst_row, const egui_image_file_t *self, egui_dim_t screen_x_start,
                                                           egui_dim_t screen_x_end, egui_dim_t image_x, egui_alpha_t canvas_alpha)
{
    const uint16_t *src_row;
    egui_dim_t count;

    if (self == NULL || dst_row == NULL || self->resize_row_pixels == NULL || screen_x_end <= screen_x_start || screen_x_start < image_x)
    {
        return;
    }

    count = screen_x_end - screen_x_start;
    src_row = self->resize_row_pixels + (screen_x_start - image_x);

    if (canvas_alpha == EGUI_ALPHA_100)
    {
        egui_api_memcpy(dst_row, src_row, (size_t)count * sizeof(uint16_t));
        return;
    }

    for (egui_dim_t i = 0; i < count; i++)
    {
        egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[i], canvas_alpha);
    }
}

static int egui_image_file_draw_resize_fast_nomask(const egui_image_file_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height,
                                                   egui_dim_t screen_x_start, egui_dim_t screen_y_start, egui_dim_t screen_x_end, egui_dim_t screen_y_end)
{
#if !EGUI_IMAGE_FILE_FAST_PATH_ENABLE
    EGUI_UNUSED(self);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(width);
    EGUI_UNUSED(height);
    EGUI_UNUSED(screen_x_start);
    EGUI_UNUSED(screen_y_start);
    EGUI_UNUSED(screen_x_end);
    EGUI_UNUSED(screen_y_end);
    return 0;
#else
#if EGUI_CONFIG_COLOR_DEPTH == 16
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_dim_t screen_y = screen_y_start;
    int allow_resize_row_cache = (screen_y_end - screen_y_start) == 1;

    if (self == NULL || canvas == NULL || canvas->mask != NULL || width <= 0 || height <= 0 || self->has_alpha)
    {
        return 0;
    }
    if (canvas->alpha == 0)
    {
        return 1;
    }

    while (screen_y < screen_y_end)
    {
        egui_color_int_t *dst_row;
        egui_dim_t dst_stride;
        egui_dim_t screen_y_next;
        egui_dim_t run_height;
        uint16_t src_y = egui_image_file_resize_axis_map(self->height, height, screen_y - y);
        int use_resize_row_cache = 0;

        use_resize_row_cache = allow_resize_row_cache && egui_image_file_prepare_resize_row_cache((egui_image_file_t *)self, src_y, width);
        if ((!use_resize_row_cache && !egui_image_file_load_row((egui_image_file_t *)self, src_y)) ||
            !egui_image_file_get_rgb565_dst_row(screen_x_start, screen_y, &dst_row, &dst_stride))
        {
            return 0;
        }

        screen_y_next = y + egui_image_file_resize_axis_run_end(self->height, height, src_y);
        if (screen_y_next <= screen_y)
        {
            screen_y_next = screen_y + 1;
        }
        if (screen_y_next > screen_y_end)
        {
            screen_y_next = screen_y_end;
        }
        run_height = screen_y_next - screen_y;

        if (use_resize_row_cache)
        {
            egui_image_file_blend_resize_row_cached_nomask(dst_row, self, screen_x_start, screen_x_end, x, canvas->alpha);
        }
        else
        {
            egui_image_file_blend_resize_row_nomask(dst_row, self, screen_x_start, screen_x_end, x, width, canvas->alpha);
        }

        if (!self->has_alpha && canvas->alpha == EGUI_ALPHA_100)
        {
            egui_dim_t row_bytes = (screen_x_end - screen_x_start) * (egui_dim_t)sizeof(uint16_t);

            for (egui_dim_t row = 1; row < run_height; row++)
            {
                egui_api_memcpy(dst_row + row * dst_stride, dst_row, (size_t)row_bytes);
            }
        }
        else
        {
            for (egui_dim_t row = 1; row < run_height; row++)
            {
                if (use_resize_row_cache)
                {
                    egui_image_file_blend_resize_row_cached_nomask(dst_row + row * dst_stride, self, screen_x_start, screen_x_end, x, canvas->alpha);
                }
                else
                {
                    egui_image_file_blend_resize_row_nomask(dst_row + row * dst_stride, self, screen_x_start, screen_x_end, x, width, canvas->alpha);
                }
            }
        }

        screen_y = screen_y_next;
    }

    return 1;
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(width);
    EGUI_UNUSED(height);
    EGUI_UNUSED(screen_x_start);
    EGUI_UNUSED(screen_y_start);
    EGUI_UNUSED(screen_x_end);
    EGUI_UNUSED(screen_y_end);
    return 0;
#endif
#endif
}

static int egui_image_file_api_get_size(const egui_image_t *base, egui_dim_t *width, egui_dim_t *height)
{
    egui_image_file_t *self = egui_image_file_from_base(base);

    if (self == NULL)
    {
        return 0;
    }
    if (egui_image_file_get_intrinsic_resize(self, width, height))
    {
        return 1;
    }
    if (!egui_image_file_try_open(self))
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
    uint16_t src_x;
    uint16_t src_y;
    egui_dim_t resize_width;
    egui_dim_t resize_height;

    if (self == NULL)
    {
        return 0;
    }
    if (!egui_image_file_try_open(self))
    {
        return 0;
    }
    if (egui_image_file_get_intrinsic_resize(self, &resize_width, &resize_height))
    {
        if (x < 0 || y < 0 || x >= resize_width || y >= resize_height)
        {
            return 0;
        }

        src_x = egui_image_file_resize_axis_map(self->width, resize_width, x);
        src_y = egui_image_file_resize_axis_map(self->height, resize_height, y);
        return egui_image_file_get_source_point(self, src_x, src_y, color, alpha);
    }

    if (x < 0 || y < 0 || x >= self->width || y >= self->height)
    {
        return 0;
    }
    return egui_image_file_get_source_point(self, (uint16_t)x, (uint16_t)y, color, alpha);
}

static int egui_image_file_api_get_point_resize(const egui_image_t *base, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color,
                                                egui_alpha_t *alpha)
{
    egui_image_file_t *self = egui_image_file_from_base(base);
    egui_dim_t src_x;
    egui_dim_t src_y;
    uint32_t scaled_x;
    uint32_t scaled_y;

    if (self == NULL || width <= 0 || height <= 0 || x < 0 || y < 0 || x >= width || y >= height)
    {
        return 0;
    }
    if (!egui_image_file_try_open(self))
    {
        return 0;
    }

    scaled_x = (uint32_t)x * (uint32_t)self->width;
    scaled_y = (uint32_t)y * (uint32_t)self->height;
    src_x = (egui_dim_t)(scaled_x / (uint32_t)width);
    src_y = (egui_dim_t)(scaled_y / (uint32_t)height);
    if (src_x >= self->width)
    {
        src_x = self->width - 1;
    }
    if (src_y >= self->height)
    {
        src_y = self->height - 1;
    }
    return egui_image_file_get_source_point(self, (uint16_t)src_x, (uint16_t)src_y, color, alpha);
}

static void egui_image_file_api_draw_image(const egui_image_t *base, egui_dim_t x, egui_dim_t y)
{
    egui_image_file_t *self = egui_image_file_from_base(base);
    egui_dim_t draw_width = 0;
    egui_dim_t draw_height = 0;
    egui_dim_t screen_x_start;
    egui_dim_t screen_y_start;
    egui_dim_t screen_x_end;
    egui_dim_t screen_y_end;
    egui_dim_t screen_y;

    if (self == NULL)
    {
        egui_image_file_draw_fallback(self, x, y, 0, 0, 0);
        return;
    }
    if (egui_image_file_get_intrinsic_resize(self, &draw_width, &draw_height))
    {
        egui_image_file_api_draw_image_resize(base, x, y, draw_width, draw_height);
        return;
    }
    if (!egui_image_file_try_open(self))
    {
        egui_image_file_draw_fallback(self, x, y, 0, 0, 0);
        return;
    }
    if (!egui_image_file_get_visible_rect(x, y, self->width, self->height, &screen_x_start, &screen_y_start, &screen_x_end, &screen_y_end))
    {
        return;
    }

    if (!egui_image_file_should_use_stream_column_band(self, screen_x_end - screen_x_start, screen_y_end - screen_y_start))
    {
        egui_image_file_prepare_stream_band_for_rows(self, (uint16_t)(screen_y_start - y), (uint16_t)(screen_y_end - y));
    }

    if (screen_x_end - screen_x_start == 1 &&
        egui_image_file_draw_visible_column_fast(self, screen_x_start, screen_y_start, screen_y_end, screen_x_start - x, (uint16_t)(screen_y_start - y)))
    {
        return;
    }
    if (egui_image_file_draw_visible_block_fast(self, screen_x_start, screen_y_start, screen_x_end, screen_y_end, screen_x_start - x,
                                                (uint16_t)(screen_y_start - y)))
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

    if (self == NULL || !egui_image_file_try_open(self))
    {
        egui_image_file_draw_fallback(self, x, y, width, height, 1);
        return;
    }
    if (!egui_image_file_get_visible_rect(x, y, width, height, &screen_x_start, &screen_y_start, &screen_x_end, &screen_y_end))
    {
        return;
    }

    if (screen_y_start < screen_y_end)
    {
        uint16_t src_y_start = egui_image_file_resize_axis_map(self->height, height, screen_y_start - y);
        uint16_t src_y_end = (uint16_t)(egui_image_file_resize_axis_map(self->height, height, screen_y_end - 1 - y) + 1u);

        if (!(width == self->width && height == self->height &&
              egui_image_file_should_use_stream_column_band(self, screen_x_end - screen_x_start, screen_y_end - screen_y_start)))
        {
            egui_image_file_prepare_stream_band_for_rows(self, src_y_start, src_y_end);
        }
    }

    if (width == self->width && height == self->height)
    {
        if (screen_x_end - screen_x_start == 1 &&
            egui_image_file_draw_visible_column_fast(self, screen_x_start, screen_y_start, screen_y_end, screen_x_start - x, (uint16_t)(screen_y_start - y)))
        {
            return;
        }
        if (egui_image_file_draw_visible_block_fast(self, screen_x_start, screen_y_start, screen_x_end, screen_y_end, screen_x_start - x,
                                                    (uint16_t)(screen_y_start - y)))
        {
            return;
        }

        for (screen_y = screen_y_start; screen_y < screen_y_end; screen_y++)
        {
            uint16_t src_y = (uint16_t)(screen_y - y);
            egui_dim_t src_x_offset = screen_x_start - x;

            if (!egui_image_file_load_row(self, src_y))
            {
                egui_image_file_draw_fallback(self, x, y, width, height, 1);
                return;
            }

            egui_image_file_draw_visible_row(self, screen_x_start, screen_y, screen_x_end - screen_x_start, src_x_offset);
        }
        return;
    }
    if (screen_x_end - screen_x_start == 1 &&
        egui_image_file_draw_resize_visible_column_fast(self, screen_x_start, screen_y_start, screen_y_end, x, y, width, height))
    {
        return;
    }
    if (egui_image_file_draw_resize_fast_nomask(self, x, y, width, height, screen_x_start, screen_y_start, screen_x_end, screen_y_end))
    {
        return;
    }

    for (screen_y = screen_y_start; screen_y < screen_y_end;)
    {
        egui_dim_t screen_y_next;
        egui_dim_t run_height;
        uint16_t src_y = egui_image_file_resize_axis_map(self->height, height, screen_y - y);
        const uint16_t *row_pixels;
        const uint8_t *row_alpha;

        if (!egui_image_file_load_row(self, src_y))
        {
            egui_image_file_draw_fallback(self, x, y, width, height, 1);
            return;
        }
        row_pixels = egui_image_file_get_cached_row_pixels_ptr(self);
        row_alpha = egui_image_file_get_cached_row_alpha_ptr(self);
        if (row_pixels == NULL)
        {
            egui_image_file_draw_fallback(self, x, y, width, height, 1);
            return;
        }

        screen_y_next = y + egui_image_file_resize_axis_run_end(self->height, height, src_y);
        if (screen_y_next <= screen_y)
        {
            screen_y_next = screen_y + 1;
        }
        if (screen_y_next > screen_y_end)
        {
            screen_y_next = screen_y_end;
        }
        run_height = screen_y_next - screen_y;

        for (egui_dim_t screen_x = screen_x_start; screen_x < screen_x_end;)
        {
            egui_dim_t screen_x_next;
            egui_dim_t run_width;
            uint16_t src_x = egui_image_file_resize_axis_map(self->width, width, screen_x - x);
            egui_alpha_t alpha = EGUI_ALPHA_100;
            egui_color_t color;

            screen_x_next = x + egui_image_file_resize_axis_run_end(self->width, width, src_x);
            if (screen_x_next <= screen_x)
            {
                screen_x_next = screen_x + 1;
            }
            if (screen_x_next > screen_x_end)
            {
                screen_x_next = screen_x_end;
            }
            run_width = screen_x_next - screen_x;

            if (self->has_alpha)
            {
                alpha = row_alpha != NULL ? row_alpha[src_x] : 0;
                if (alpha == 0)
                {
                    screen_x = screen_x_next;
                    continue;
                }
            }

            color.full = EGUI_COLOR_RGB565_TRANS(row_pixels[src_x]);
            egui_canvas_draw_fillrect(screen_x, screen_y, run_width, run_height, color, alpha);
            screen_x = screen_x_next;
        }

        screen_y = screen_y_next;
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
    self->stream_band_pixels = NULL;
    self->stream_band_alpha = NULL;
    self->resize_column_pixels = NULL;
    self->resize_column_alpha = NULL;
    self->resize_row_pixels = NULL;
    self->row_capacity = 0;
    self->alpha_capacity = 0;
    self->stream_band_pixel_capacity = 0;
    self->resize_column_capacity = 0;
    self->resize_row_capacity = 0;
    self->resize_width = 0;
    self->resize_height = 0;
    self->resize_enabled = 0;
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
    if (self->stream_band_pixels != NULL)
    {
        egui_free(self->stream_band_pixels);
        self->stream_band_pixels = NULL;
    }
    if (self->stream_band_alpha != NULL)
    {
        egui_free(self->stream_band_alpha);
        self->stream_band_alpha = NULL;
    }
    egui_image_file_release_resize_column_cache(self);
    egui_image_file_release_resize_row_cache(self);
    self->row_capacity = 0;
    self->alpha_capacity = 0;
    self->stream_band_pixel_capacity = 0;
    self->resize_width = 0;
    self->resize_height = 0;
    self->resize_enabled = 0;
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

void egui_image_file_set_resize(egui_image_file_t *self, egui_dim_t width, egui_dim_t height)
{
    if (self == NULL)
    {
        return;
    }
    if (width <= 0 || height <= 0)
    {
        egui_image_file_clear_resize(self);
        return;
    }
    if (self->resize_enabled != 0U && self->resize_width == (uint16_t)width && self->resize_height == (uint16_t)height)
    {
        return;
    }

    self->resize_width = (uint16_t)width;
    self->resize_height = (uint16_t)height;
    self->resize_enabled = 1U;
}

void egui_image_file_clear_resize(egui_image_file_t *self)
{
    if (self == NULL)
    {
        return;
    }

    self->resize_width = 0;
    self->resize_height = 0;
    self->resize_enabled = 0;
}

int egui_image_file_get_resize(const egui_image_file_t *self, egui_dim_t *width, egui_dim_t *height)
{
    return egui_image_file_get_intrinsic_resize(self, width, height);
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
