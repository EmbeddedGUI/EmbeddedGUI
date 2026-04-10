#include <string.h>

#include "decoder_stb.h"
#include "core/egui_common.h"

#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

typedef struct file_image_stb_ctx
{
    uint16_t width;
    uint16_t height;
    uint16_t *pixels;
    uint8_t *alpha;
} file_image_stb_ctx_t;

static char file_image_ascii_tolower(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return (char)(ch - 'A' + 'a');
    }
    return ch;
}

static int file_image_path_has_ext(const char *path, const char *ext)
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
        if (file_image_ascii_tolower(path[i]) != file_image_ascii_tolower(ext[i]))
        {
            return 0;
        }
    }
    return 1;
}

static int file_image_stb_match(const char *path)
{
    return file_image_path_has_ext(path, ".jpg") || file_image_path_has_ext(path, ".jpeg") || file_image_path_has_ext(path, ".png") ||
           file_image_path_has_ext(path, ".bmp");
}

static int file_image_stb_get_file_size(const egui_image_file_io_t *io, void *file_handle, int32_t *out_size)
{
    int32_t size;

    if (io == NULL || io->seek == NULL || io->tell == NULL || out_size == NULL)
    {
        return 0;
    }
    if (io->seek(io->user_data, file_handle, 0, EGUI_IMAGE_FILE_SEEK_END) != 0)
    {
        return 0;
    }

    size = io->tell(io->user_data, file_handle);
    if (size <= 0)
    {
        io->seek(io->user_data, file_handle, 0, EGUI_IMAGE_FILE_SEEK_SET);
        return 0;
    }

    if (io->seek(io->user_data, file_handle, 0, EGUI_IMAGE_FILE_SEEK_SET) != 0)
    {
        return 0;
    }

    *out_size = size;
    return 1;
}

static void file_image_stb_close(void *decoder_ctx)
{
    file_image_stb_ctx_t *ctx = (file_image_stb_ctx_t *)decoder_ctx;

    if (ctx == NULL)
    {
        return;
    }
    if (ctx->pixels != NULL)
    {
        egui_free(ctx->pixels);
    }
    if (ctx->alpha != NULL)
    {
        egui_free(ctx->alpha);
    }
    egui_free(ctx);
}

static int file_image_stb_open(const egui_image_file_io_t *io, void *file_handle, const char *path, void **decoder_ctx, egui_image_file_open_result_t *out_info)
{
    int32_t file_size;
    uint8_t *file_buf = NULL;
    stbi_uc *rgba = NULL;
    int width = 0;
    int height = 0;
    int channels = 0;
    int pixel_count;
    int i;
    int has_alpha = 0;
    file_image_stb_ctx_t *ctx = NULL;

    EGUI_UNUSED(path);
    if (io == NULL || io->read == NULL || decoder_ctx == NULL || out_info == NULL || file_handle == NULL)
    {
        return 0;
    }
    if (!file_image_stb_get_file_size(io, file_handle, &file_size))
    {
        return 0;
    }

    file_buf = (uint8_t *)egui_malloc(file_size);
    if (file_buf == NULL)
    {
        return 0;
    }
    if (io->read(io->user_data, file_handle, file_buf, (uint32_t)file_size) != file_size)
    {
        egui_free(file_buf);
        return 0;
    }

    rgba = stbi_load_from_memory(file_buf, file_size, &width, &height, &channels, 4);
    egui_free(file_buf);
    if (rgba == NULL || width <= 0 || height <= 0 || width > 0xFFFF || height > 0xFFFF)
    {
        if (rgba != NULL)
        {
            stbi_image_free(rgba);
        }
        return 0;
    }

    pixel_count = width * height;
    for (i = 0; i < pixel_count; i++)
    {
        if (rgba[i * 4 + 3] != 0xFF)
        {
            has_alpha = 1;
            break;
        }
    }

    ctx = (file_image_stb_ctx_t *)egui_malloc(sizeof(*ctx));
    if (ctx == NULL)
    {
        stbi_image_free(rgba);
        return 0;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->width = (uint16_t)width;
    ctx->height = (uint16_t)height;
    ctx->pixels = (uint16_t *)egui_malloc(pixel_count * (int)sizeof(uint16_t));
    if (ctx->pixels == NULL)
    {
        file_image_stb_close(ctx);
        stbi_image_free(rgba);
        return 0;
    }
    if (has_alpha)
    {
        ctx->alpha = (uint8_t *)egui_malloc(pixel_count);
        if (ctx->alpha == NULL)
        {
            file_image_stb_close(ctx);
            stbi_image_free(rgba);
            return 0;
        }
    }

    for (i = 0; i < pixel_count; i++)
    {
        uint8_t r = rgba[i * 4 + 0];
        uint8_t g = rgba[i * 4 + 1];
        uint8_t b = rgba[i * 4 + 2];

        ctx->pixels[i] = (uint16_t)(((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | ((uint16_t)b >> 3));
        if (ctx->alpha != NULL)
        {
            ctx->alpha[i] = rgba[i * 4 + 3];
        }
    }

    stbi_image_free(rgba);

    out_info->width = ctx->width;
    out_info->height = ctx->height;
    out_info->has_alpha = has_alpha ? 1u : 0u;
    out_info->keep_file_open = 0;
    *decoder_ctx = ctx;
    return 1;
}

static int file_image_stb_read_row(void *decoder_ctx, uint16_t row, uint16_t *rgb565_row, uint8_t *alpha_row)
{
    file_image_stb_ctx_t *ctx = (file_image_stb_ctx_t *)decoder_ctx;
    uint32_t offset;

    if (ctx == NULL || rgb565_row == NULL || row >= ctx->height)
    {
        return 0;
    }

    offset = (uint32_t)row * ctx->width;
    memcpy(rgb565_row, &ctx->pixels[offset], (uint32_t)ctx->width * sizeof(uint16_t));
    if (ctx->alpha != NULL && alpha_row != NULL)
    {
        memcpy(alpha_row, &ctx->alpha[offset], ctx->width);
    }
    return 1;
}

const egui_image_file_decoder_t g_file_image_stb_decoder = {
        .name = "stb_image",
        .match = file_image_stb_match,
        .open = file_image_stb_open,
        .read_row = file_image_stb_read_row,
        .close = file_image_stb_close,
};
