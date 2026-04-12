#include <string.h>

#include "decoder_bmp_stream.h"
#include "core/egui_common.h"

#define FILE_IMAGE_BMP_COMPRESSION_RGB       0u
#define FILE_IMAGE_BMP_COMPRESSION_BITFIELDS 3u

typedef struct file_image_bmp_stream_ctx
{
    const egui_image_file_io_t *io;
    void *file_handle;
    uint8_t *row_buf;
    uint32_t pixel_offset;
    uint32_t row_stride;
    uint16_t width;
    uint16_t height;
    uint8_t bytes_per_pixel;
    uint8_t top_down;
    uint8_t format;
} file_image_bmp_stream_ctx_t;

typedef enum
{
    FILE_IMAGE_BMP_FORMAT_BGR24 = 0,
    FILE_IMAGE_BMP_FORMAT_BGRA32,
    FILE_IMAGE_BMP_FORMAT_RGB565,
} file_image_bmp_stream_format_t;

static char file_image_bmp_ascii_tolower(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return (char)(ch - 'A' + 'a');
    }
    return ch;
}

static int file_image_bmp_path_has_ext(const char *path, const char *ext)
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
        if (file_image_bmp_ascii_tolower(path[i]) != file_image_bmp_ascii_tolower(ext[i]))
        {
            return 0;
        }
    }
    return 1;
}

static uint16_t file_image_bmp_read_le16(const uint8_t *buf)
{
    return (uint16_t)(((uint16_t)buf[1] << 8) | buf[0]);
}

static uint32_t file_image_bmp_read_le32(const uint8_t *buf)
{
    return ((uint32_t)buf[3] << 24) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[1] << 8) | buf[0];
}

static int32_t file_image_bmp_read_le32s(const uint8_t *buf)
{
    return (int32_t)file_image_bmp_read_le32(buf);
}

static int file_image_bmp_read_exact(const egui_image_file_io_t *io, void *file_handle, void *buf, uint32_t size)
{
    if (io == NULL || io->read == NULL || file_handle == NULL || buf == NULL)
    {
        return 0;
    }

    return io->read(io->user_data, file_handle, buf, size) == (int32_t)size;
}

static int file_image_bmp_seek_abs(const egui_image_file_io_t *io, void *file_handle, uint32_t offset)
{
    if (io == NULL || io->seek == NULL || file_handle == NULL)
    {
        return 0;
    }

    return io->seek(io->user_data, file_handle, (int32_t)offset, EGUI_IMAGE_FILE_SEEK_SET) == 0;
}

static int file_image_bmp_match(const char *path)
{
    return file_image_bmp_path_has_ext(path, ".bmp");
}

static void file_image_bmp_stream_close(void *decoder_ctx)
{
    file_image_bmp_stream_ctx_t *ctx = (file_image_bmp_stream_ctx_t *)decoder_ctx;

    if (ctx == NULL)
    {
        return;
    }

    if (ctx->row_buf != NULL)
    {
        egui_free(ctx->row_buf);
    }
    egui_free(ctx);
}

static int file_image_bmp_parse_headers(const egui_image_file_io_t *io, void *file_handle, file_image_bmp_stream_ctx_t *ctx,
                                        egui_image_file_open_result_t *out_info)
{
    uint8_t file_header[14];
    uint8_t dib_header[56];
    uint32_t dib_size;
    int32_t width_signed;
    int32_t height_signed;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t row_stride;
    uint32_t red_mask = 0;
    uint32_t green_mask = 0;
    uint32_t blue_mask = 0;

    if (!file_image_bmp_seek_abs(io, file_handle, 0) || !file_image_bmp_read_exact(io, file_handle, file_header, sizeof(file_header)))
    {
        return 0;
    }
    if (file_header[0] != 'B' || file_header[1] != 'M')
    {
        return 0;
    }

    ctx->pixel_offset = file_image_bmp_read_le32(&file_header[10]);
    if (!file_image_bmp_read_exact(io, file_handle, dib_header, 40))
    {
        return 0;
    }

    dib_size = file_image_bmp_read_le32(&dib_header[0]);
    if (dib_size < 40)
    {
        return 0;
    }

    width_signed = file_image_bmp_read_le32s(&dib_header[4]);
    height_signed = file_image_bmp_read_le32s(&dib_header[8]);
    bits_per_pixel = file_image_bmp_read_le16(&dib_header[14]);
    compression = file_image_bmp_read_le32(&dib_header[16]);

    if (width_signed <= 0 || height_signed == 0 || width_signed > 0xFFFF)
    {
        return 0;
    }

    ctx->top_down = height_signed < 0 ? 1u : 0u;
    if (height_signed < 0)
    {
        height_signed = -height_signed;
    }
    if (height_signed <= 0 || height_signed > 0xFFFF)
    {
        return 0;
    }

    if (bits_per_pixel == 24 && compression == FILE_IMAGE_BMP_COMPRESSION_RGB)
    {
        ctx->format = FILE_IMAGE_BMP_FORMAT_BGR24;
        ctx->bytes_per_pixel = 3;
    }
    else if (bits_per_pixel == 32 && compression == FILE_IMAGE_BMP_COMPRESSION_RGB)
    {
        ctx->format = FILE_IMAGE_BMP_FORMAT_BGRA32;
        ctx->bytes_per_pixel = 4;
    }
    else if (bits_per_pixel == 16 && compression == FILE_IMAGE_BMP_COMPRESSION_BITFIELDS)
    {
        if (dib_size > 40)
        {
            uint32_t remaining = dib_size - 40;

            if (remaining > sizeof(dib_header) - 40)
            {
                remaining = sizeof(dib_header) - 40;
            }
            if (!file_image_bmp_read_exact(io, file_handle, &dib_header[40], remaining))
            {
                return 0;
            }
        }
        else
        {
            if (!file_image_bmp_read_exact(io, file_handle, &dib_header[40], 12))
            {
                return 0;
            }
        }

        red_mask = file_image_bmp_read_le32(&dib_header[40]);
        green_mask = file_image_bmp_read_le32(&dib_header[44]);
        blue_mask = file_image_bmp_read_le32(&dib_header[48]);
        if (red_mask != 0xF800u || green_mask != 0x07E0u || blue_mask != 0x001Fu)
        {
            return 0;
        }

        ctx->format = FILE_IMAGE_BMP_FORMAT_RGB565;
        ctx->bytes_per_pixel = 2;
    }
    else
    {
        return 0;
    }

    row_stride = (uint32_t)(((uint32_t)bits_per_pixel * (uint32_t)width_signed + 31u) / 32u) * 4u;
    if (row_stride == 0)
    {
        return 0;
    }

    ctx->width = (uint16_t)width_signed;
    ctx->height = (uint16_t)height_signed;
    ctx->row_stride = row_stride;

    out_info->width = ctx->width;
    out_info->height = ctx->height;
    out_info->has_alpha = 0;
    out_info->keep_file_open = 1;
    return 1;
}

static int file_image_bmp_stream_open(const egui_image_file_io_t *io, void *file_handle, const char *path, void **decoder_ctx,
                                      egui_image_file_open_result_t *out_info)
{
    file_image_bmp_stream_ctx_t *ctx;

    EGUI_UNUSED(path);
    if (io == NULL || io->seek == NULL || io->read == NULL || file_handle == NULL || decoder_ctx == NULL || out_info == NULL)
    {
        return 0;
    }

    ctx = (file_image_bmp_stream_ctx_t *)egui_malloc(sizeof(*ctx));
    if (ctx == NULL)
    {
        return 0;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->io = io;
    ctx->file_handle = file_handle;

    if (!file_image_bmp_parse_headers(io, file_handle, ctx, out_info))
    {
        file_image_bmp_stream_close(ctx);
        return 0;
    }

    ctx->row_buf = (uint8_t *)egui_malloc((int)ctx->row_stride);
    if (ctx->row_buf == NULL)
    {
        file_image_bmp_stream_close(ctx);
        return 0;
    }

    *decoder_ctx = ctx;
    return 1;
}

static int file_image_bmp_stream_read_row(void *decoder_ctx, uint16_t row, uint16_t *rgb565_row, uint8_t *alpha_row)
{
    file_image_bmp_stream_ctx_t *ctx = (file_image_bmp_stream_ctx_t *)decoder_ctx;
    uint32_t file_row;
    uint32_t row_offset;
    uint16_t x;

    EGUI_UNUSED(alpha_row);
    if (ctx == NULL || rgb565_row == NULL || row >= ctx->height)
    {
        return 0;
    }

    file_row = ctx->top_down ? row : (uint32_t)(ctx->height - 1u - row);
    row_offset = ctx->pixel_offset + file_row * ctx->row_stride;
    if (!file_image_bmp_seek_abs(ctx->io, ctx->file_handle, row_offset) || !file_image_bmp_read_exact(ctx->io, ctx->file_handle, ctx->row_buf, ctx->row_stride))
    {
        return 0;
    }

    if (ctx->format == FILE_IMAGE_BMP_FORMAT_RGB565)
    {
        for (x = 0; x < ctx->width; x++)
        {
            uint32_t src_offset = (uint32_t)x * 2u;

            rgb565_row[x] = file_image_bmp_read_le16(&ctx->row_buf[src_offset]);
        }
        return 1;
    }

    for (x = 0; x < ctx->width; x++)
    {
        uint32_t src_offset = (uint32_t)x * ctx->bytes_per_pixel;
        uint8_t b = ctx->row_buf[src_offset + 0u];
        uint8_t g = ctx->row_buf[src_offset + 1u];
        uint8_t r = ctx->row_buf[src_offset + 2u];

        rgb565_row[x] = (uint16_t)(((uint16_t)(r & 0xF8u) << 8) | ((uint16_t)(g & 0xFCu) << 3) | ((uint16_t)b >> 3));
    }

    return 1;
}

const egui_image_file_decoder_t g_file_image_bmp_stream_decoder = {
        .name = "bmp_stream",
        .match = file_image_bmp_match,
        .open = file_image_bmp_stream_open,
        .read_row = file_image_bmp_stream_read_row,
        .close = file_image_bmp_stream_close,
};
