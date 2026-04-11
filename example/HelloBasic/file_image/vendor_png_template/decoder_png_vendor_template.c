#include <string.h>

#include "decoder_png_vendor_template.h"
#include "core/egui_common.h"

typedef struct file_image_vendor_png_ctx
{
    void *vendor_session;
    uint16_t width;
    uint16_t height;
    uint8_t has_alpha;
    uint16_t *pixels;
    uint8_t *alpha;
} file_image_vendor_png_ctx_t;

static char file_image_vendor_png_ascii_tolower(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return (char)(ch - 'A' + 'a');
    }
    return ch;
}

static int file_image_vendor_png_path_has_ext(const char *path, const char *ext)
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
        if (file_image_vendor_png_ascii_tolower(path[i]) != file_image_vendor_png_ascii_tolower(ext[i]))
        {
            return 0;
        }
    }
    return 1;
}

static int file_image_vendor_png_match(const char *path)
{
    return file_image_vendor_png_path_has_ext(path, ".png");
}

static int file_image_vendor_png_prepare(file_image_vendor_png_ctx_t *ctx, const char *path, uint16_t *width, uint16_t *height, uint8_t *has_alpha)
{
    /*
     * Replace this stub with your chip/vendor PNG library, for example:
     * 1. Bind the input file/path to the library session.
     * 2. Parse the PNG header to get width, height and alpha information.
     * 3. Create the decode session and store it in ctx->vendor_session.
     */
    EGUI_UNUSED(ctx);
    EGUI_UNUSED(path);
    EGUI_UNUSED(width);
    EGUI_UNUSED(height);
    EGUI_UNUSED(has_alpha);
    return 0;
}

static int file_image_vendor_png_decode(file_image_vendor_png_ctx_t *ctx, uint16_t *dst_rgb565, uint8_t *dst_alpha)
{
    /*
     * Replace this stub with the real PNG decode call.
     * Fill dst_rgb565 as tightly packed RGB565 pixels for the whole image.
     * If ctx->has_alpha != 0, also fill dst_alpha with one alpha byte per pixel.
     *
     * If the vendor library returns RGBA8888, convert it here before storing:
     *   rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
     *   alpha  = a
     */
    EGUI_UNUSED(ctx);
    EGUI_UNUSED(dst_rgb565);
    EGUI_UNUSED(dst_alpha);
    return 0;
}

static void file_image_vendor_png_release(file_image_vendor_png_ctx_t *ctx)
{
    /*
     * Release the vendor PNG session here, for example:
     * vendor_png_close(ctx->vendor_session);
     */
    EGUI_UNUSED(ctx);
}

static void file_image_vendor_png_close(void *decoder_ctx)
{
    file_image_vendor_png_ctx_t *ctx = (file_image_vendor_png_ctx_t *)decoder_ctx;

    if (ctx == NULL)
    {
        return;
    }

    file_image_vendor_png_release(ctx);
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

static int file_image_vendor_png_open(const egui_image_file_io_t *io, void *file_handle, const char *path, void **decoder_ctx, egui_image_file_open_result_t *out_info)
{
    file_image_vendor_png_ctx_t *ctx;
    uint16_t width = 0;
    uint16_t height = 0;
    uint8_t has_alpha = 0;
    uint32_t pixel_count;

    EGUI_UNUSED(io);
    EGUI_UNUSED(file_handle);
    if (decoder_ctx == NULL || out_info == NULL)
    {
        return 0;
    }

    ctx = (file_image_vendor_png_ctx_t *)egui_malloc(sizeof(*ctx));
    if (ctx == NULL)
    {
        return 0;
    }

    memset(ctx, 0, sizeof(*ctx));
    if (!file_image_vendor_png_prepare(ctx, path, &width, &height, &has_alpha))
    {
        file_image_vendor_png_close(ctx);
        return 0;
    }

    if (width == 0 || height == 0)
    {
        file_image_vendor_png_close(ctx);
        return 0;
    }

    pixel_count = (uint32_t)width * height;
    if (pixel_count == 0 || pixel_count > (0x7FFFFFFFul / sizeof(uint16_t)))
    {
        file_image_vendor_png_close(ctx);
        return 0;
    }

    ctx->pixels = (uint16_t *)egui_malloc((int)(pixel_count * sizeof(uint16_t)));
    if (ctx->pixels == NULL)
    {
        file_image_vendor_png_close(ctx);
        return 0;
    }

    if (has_alpha)
    {
        ctx->alpha = (uint8_t *)egui_malloc((int)pixel_count);
        if (ctx->alpha == NULL)
        {
            file_image_vendor_png_close(ctx);
            return 0;
        }
    }

    ctx->width = width;
    ctx->height = height;
    ctx->has_alpha = has_alpha ? 1u : 0u;

    if (!file_image_vendor_png_decode(ctx, ctx->pixels, ctx->alpha))
    {
        file_image_vendor_png_close(ctx);
        return 0;
    }

    out_info->width = width;
    out_info->height = height;
    out_info->has_alpha = ctx->has_alpha;
    out_info->keep_file_open = 0;
    *decoder_ctx = ctx;
    return 1;
}

static int file_image_vendor_png_read_row(void *decoder_ctx, uint16_t row, uint16_t *rgb565_row, uint8_t *alpha_row)
{
    file_image_vendor_png_ctx_t *ctx = (file_image_vendor_png_ctx_t *)decoder_ctx;
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

const egui_image_file_decoder_t g_file_image_png_vendor_template_decoder = {
        .name = "vendor_png_template",
        .match = file_image_vendor_png_match,
        .open = file_image_vendor_png_open,
        .read_row = file_image_vendor_png_read_row,
        .close = file_image_vendor_png_close,
};
