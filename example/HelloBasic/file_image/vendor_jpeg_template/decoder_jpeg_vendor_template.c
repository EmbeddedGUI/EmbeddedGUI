#include <string.h>

#include "decoder_jpeg_vendor_template.h"
#include "core/egui_common.h"

#define FILE_IMAGE_VENDOR_JPEG_DEFAULT_BAND_HEIGHT 16u

typedef struct file_image_vendor_jpeg_ctx
{
    const egui_image_file_io_t *io;
    void *file_handle;
    void *vendor_session;
    uint16_t *band_pixels;
    uint32_t band_pixel_count;
    uint16_t width;
    uint16_t height;
    uint16_t band_height;
    uint16_t cached_band_start;
    uint16_t cached_band_end;
} file_image_vendor_jpeg_ctx_t;

static char file_image_vendor_jpeg_ascii_tolower(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return (char)(ch - 'A' + 'a');
    }
    return ch;
}

static int file_image_vendor_jpeg_path_has_ext(const char *path, const char *ext)
{
    size_t path_len;
    size_t ext_len;
    size_t i;

    if (path == NULL || ext == NULL)
    {
        return 0;
    }

    path_len = strlen(path);
    ext_len = strlen(ext);
    if (path_len < ext_len)
    {
        return 0;
    }

    path += path_len - ext_len;
    for (i = 0; i < ext_len; i++)
    {
        if (file_image_vendor_jpeg_ascii_tolower(path[i]) != file_image_vendor_jpeg_ascii_tolower(ext[i]))
        {
            return 0;
        }
    }
    return 1;
}

static int file_image_vendor_jpeg_match(const char *path)
{
    return file_image_vendor_jpeg_path_has_ext(path, ".jpg") || file_image_vendor_jpeg_path_has_ext(path, ".jpeg");
}

static void file_image_vendor_jpeg_invalidate_band(file_image_vendor_jpeg_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    ctx->cached_band_start = 0xFFFFu;
    ctx->cached_band_end = 0u;
}

static int file_image_vendor_jpeg_prepare(file_image_vendor_jpeg_ctx_t *ctx, const char *path, uint16_t *width, uint16_t *height, uint16_t *band_height)
{
    /*
     * Replace this stub with your chip/vendor JPEG driver, for example:
     * 1. Bind the SD/FATFS file handle or reopen from path.
     * 2. Parse JPEG header to get width and height.
     * 3. Create the vendor decode session and store it in ctx->vendor_session.
     * 4. Choose how many rows the hardware returns per job and write it to band_height.
     */
    EGUI_UNUSED(ctx);
    EGUI_UNUSED(path);
    EGUI_UNUSED(width);
    EGUI_UNUSED(height);
    EGUI_UNUSED(band_height);
    return 0;
}

static int file_image_vendor_jpeg_decode_band(file_image_vendor_jpeg_ctx_t *ctx, uint16_t band_start, uint16_t band_rows, uint16_t *dst_rgb565)
{
    /*
     * Replace this stub with the real hardware/vendor decode call.
     * Fill dst_rgb565 as tightly packed row-major RGB565 pixels:
     * row0(width pixels), row1(width pixels), ...
     */
    EGUI_UNUSED(ctx);
    EGUI_UNUSED(band_start);
    EGUI_UNUSED(band_rows);
    EGUI_UNUSED(dst_rgb565);
    return 0;
}

static void file_image_vendor_jpeg_release(file_image_vendor_jpeg_ctx_t *ctx)
{
    /*
     * Release the vendor session here, for example:
     * vendor_jpeg_close(ctx->vendor_session);
     */
    EGUI_UNUSED(ctx);
}

static void file_image_vendor_jpeg_close(void *decoder_ctx)
{
    file_image_vendor_jpeg_ctx_t *ctx = (file_image_vendor_jpeg_ctx_t *)decoder_ctx;

    if (ctx == NULL)
    {
        return;
    }

    file_image_vendor_jpeg_release(ctx);
    if (ctx->band_pixels != NULL)
    {
        egui_free(ctx->band_pixels);
    }
    egui_free(ctx);
}

static int file_image_vendor_jpeg_open(const egui_image_file_io_t *io, void *file_handle, const char *path, void **decoder_ctx,
                                       egui_image_file_open_result_t *out_info)
{
    file_image_vendor_jpeg_ctx_t *ctx;
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t band_height = 0;
    uint32_t band_pixel_count;

    if (io == NULL || file_handle == NULL || decoder_ctx == NULL || out_info == NULL)
    {
        return 0;
    }

    ctx = (file_image_vendor_jpeg_ctx_t *)egui_malloc(sizeof(*ctx));
    if (ctx == NULL)
    {
        return 0;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->io = io;
    ctx->file_handle = file_handle;
    file_image_vendor_jpeg_invalidate_band(ctx);

    if (!file_image_vendor_jpeg_prepare(ctx, path, &width, &height, &band_height))
    {
        file_image_vendor_jpeg_close(ctx);
        return 0;
    }

    if (width == 0 || height == 0)
    {
        file_image_vendor_jpeg_close(ctx);
        return 0;
    }

    if (band_height == 0)
    {
        band_height = FILE_IMAGE_VENDOR_JPEG_DEFAULT_BAND_HEIGHT;
    }
    if (band_height > height)
    {
        band_height = height;
    }

    band_pixel_count = (uint32_t)width * band_height;
    ctx->band_pixels = (uint16_t *)egui_malloc((int)(band_pixel_count * sizeof(uint16_t)));
    if (ctx->band_pixels == NULL)
    {
        file_image_vendor_jpeg_close(ctx);
        return 0;
    }

    ctx->width = width;
    ctx->height = height;
    ctx->band_height = band_height;
    ctx->band_pixel_count = band_pixel_count;

    out_info->width = width;
    out_info->height = height;
    out_info->has_alpha = 0;
    out_info->keep_file_open = 1;
    *decoder_ctx = ctx;
    return 1;
}

static int file_image_vendor_jpeg_decode_cached_band(file_image_vendor_jpeg_ctx_t *ctx, uint16_t row)
{
    uint16_t band_start;
    uint16_t band_rows;

    if (ctx == NULL || row >= ctx->height)
    {
        return 0;
    }

    if (ctx->cached_band_start != 0xFFFFu && row >= ctx->cached_band_start && row < ctx->cached_band_end)
    {
        return 1;
    }

    band_start = (uint16_t)((row / ctx->band_height) * ctx->band_height);
    band_rows = (uint16_t)EGUI_MIN(ctx->band_height, (uint16_t)(ctx->height - band_start));
    if (!file_image_vendor_jpeg_decode_band(ctx, band_start, band_rows, ctx->band_pixels))
    {
        file_image_vendor_jpeg_invalidate_band(ctx);
        return 0;
    }

    ctx->cached_band_start = band_start;
    ctx->cached_band_end = (uint16_t)(band_start + band_rows);
    return 1;
}

static int file_image_vendor_jpeg_read_row(void *decoder_ctx, uint16_t row, uint16_t *rgb565_row, uint8_t *alpha_row)
{
    file_image_vendor_jpeg_ctx_t *ctx = (file_image_vendor_jpeg_ctx_t *)decoder_ctx;
    uint16_t row_offset;

    EGUI_UNUSED(alpha_row);
    if (ctx == NULL || rgb565_row == NULL || row >= ctx->height)
    {
        return 0;
    }

    if (!file_image_vendor_jpeg_decode_cached_band(ctx, row))
    {
        return 0;
    }

    row_offset = (uint16_t)(row - ctx->cached_band_start);
    memcpy(rgb565_row, ctx->band_pixels + (uint32_t)row_offset * ctx->width, (uint32_t)ctx->width * sizeof(uint16_t));
    return 1;
}

const egui_image_file_decoder_t g_file_image_jpeg_vendor_template_decoder = {
        .name = "vendor_jpeg_template",
        .match = file_image_vendor_jpeg_match,
        .open = file_image_vendor_jpeg_open,
        .read_row = file_image_vendor_jpeg_read_row,
        .close = file_image_vendor_jpeg_close,
};
