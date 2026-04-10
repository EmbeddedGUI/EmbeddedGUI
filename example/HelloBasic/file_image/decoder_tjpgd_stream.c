#include <string.h>

#include "decoder_tjpgd_stream.h"
#include "core/egui_common.h"
#include "third_party/tjpgd.h"

#define FILE_IMAGE_TJPGD_WORKBUF_SIZE 4096u
#define FILE_IMAGE_TJPGD_SKIP_CHUNK   64u

typedef struct file_image_tjpgd_stream_ctx
{
    const egui_image_file_io_t *io;
    void *file_handle;
    uint8_t *workbuf;
    uint16_t *band_pixels;
    uint32_t band_pixel_count;
    uint16_t width;
    uint16_t height;
    uint16_t mcu_height;
    uint16_t cached_band_start;
    uint16_t cached_band_end;
} file_image_tjpgd_stream_ctx_t;

typedef struct file_image_tjpgd_session
{
    file_image_tjpgd_stream_ctx_t *ctx;
    uint16_t band_start;
    uint16_t band_end;
} file_image_tjpgd_session_t;

static char file_image_tjpgd_ascii_tolower(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return (char)(ch - 'A' + 'a');
    }
    return ch;
}

static int file_image_tjpgd_path_has_ext(const char *path, const char *ext)
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
        if (file_image_tjpgd_ascii_tolower(path[i]) != file_image_tjpgd_ascii_tolower(ext[i]))
        {
            return 0;
        }
    }
    return 1;
}

static int file_image_tjpgd_match(const char *path)
{
    return file_image_tjpgd_path_has_ext(path, ".jpg") || file_image_tjpgd_path_has_ext(path, ".jpeg");
}

static void file_image_tjpgd_invalidate_band(file_image_tjpgd_stream_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    ctx->cached_band_start = 0xFFFFu;
    ctx->cached_band_end = 0u;
}

static int file_image_tjpgd_seek_file_start(const file_image_tjpgd_stream_ctx_t *ctx)
{
    if (ctx == NULL || ctx->io == NULL || ctx->io->seek == NULL || ctx->file_handle == NULL)
    {
        return 0;
    }

    return ctx->io->seek(ctx->io->user_data, ctx->file_handle, 0, EGUI_IMAGE_FILE_SEEK_SET) == 0;
}

static size_t file_image_tjpgd_input(JDEC *jd, uint8_t *buf, size_t ndata)
{
    file_image_tjpgd_session_t *session = (file_image_tjpgd_session_t *)jd->device;
    file_image_tjpgd_stream_ctx_t *ctx = session != NULL ? session->ctx : NULL;

    if (ctx == NULL || ctx->io == NULL || ctx->io->read == NULL || ctx->file_handle == NULL)
    {
        return 0;
    }

    if (buf == NULL)
    {
        if (ctx->io->seek != NULL)
        {
            return ctx->io->seek(ctx->io->user_data, ctx->file_handle, (int32_t)ndata, EGUI_IMAGE_FILE_SEEK_CUR) == 0 ? ndata : 0;
        }

        {
            uint8_t skip_buf[FILE_IMAGE_TJPGD_SKIP_CHUNK];
            size_t remain = ndata;

            while (remain > 0)
            {
                size_t chunk = remain > sizeof(skip_buf) ? sizeof(skip_buf) : remain;
                int32_t read_len = ctx->io->read(ctx->io->user_data, ctx->file_handle, skip_buf, (uint32_t)chunk);

                if (read_len != (int32_t)chunk)
                {
                    return ndata - remain;
                }
                remain -= chunk;
            }
        }
        return ndata;
    }

    {
        int32_t read_len = ctx->io->read(ctx->io->user_data, ctx->file_handle, buf, (uint32_t)ndata);

        return read_len > 0 ? (size_t)read_len : 0;
    }
}

static JRESULT file_image_tjpgd_prepare_session(file_image_tjpgd_stream_ctx_t *ctx, file_image_tjpgd_session_t *session, JDEC *jd)
{
    if (ctx == NULL || session == NULL || jd == NULL || ctx->workbuf == NULL)
    {
        return JDR_PAR;
    }
    if (!file_image_tjpgd_seek_file_start(ctx))
    {
        return JDR_INP;
    }

    session->ctx = ctx;
    session->band_start = 0u;
    session->band_end = 0u;
    return jd_prepare(jd, file_image_tjpgd_input, ctx->workbuf, FILE_IMAGE_TJPGD_WORKBUF_SIZE, session);
}

static void file_image_tjpgd_stream_close(void *decoder_ctx)
{
    file_image_tjpgd_stream_ctx_t *ctx = (file_image_tjpgd_stream_ctx_t *)decoder_ctx;

    if (ctx == NULL)
    {
        return;
    }

    if (ctx->band_pixels != NULL)
    {
        egui_free(ctx->band_pixels);
    }
    if (ctx->workbuf != NULL)
    {
        egui_free(ctx->workbuf);
    }
    egui_free(ctx);
}

static int file_image_tjpgd_stream_open(const egui_image_file_io_t *io, void *file_handle, const char *path, void **decoder_ctx,
                                        egui_image_file_open_result_t *out_info)
{
    file_image_tjpgd_stream_ctx_t *ctx;
    JDEC jd;
    JRESULT rc;
    file_image_tjpgd_session_t session;
    uint16_t mcu_height;
    uint32_t band_pixel_count;

    EGUI_UNUSED(path);
    if (io == NULL || io->read == NULL || io->seek == NULL || file_handle == NULL || decoder_ctx == NULL || out_info == NULL)
    {
        return 0;
    }

    ctx = (file_image_tjpgd_stream_ctx_t *)egui_malloc(sizeof(*ctx));
    if (ctx == NULL)
    {
        return 0;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->io = io;
    ctx->file_handle = file_handle;
    file_image_tjpgd_invalidate_band(ctx);

    ctx->workbuf = (uint8_t *)egui_malloc(FILE_IMAGE_TJPGD_WORKBUF_SIZE);
    if (ctx->workbuf == NULL)
    {
        file_image_tjpgd_stream_close(ctx);
        return 0;
    }

    memset(&session, 0, sizeof(session));
    rc = file_image_tjpgd_prepare_session(ctx, &session, &jd);
    if (rc != JDR_OK || jd.width == 0 || jd.height == 0)
    {
        file_image_tjpgd_stream_close(ctx);
        return 0;
    }

    mcu_height = (uint16_t)(jd.msy * 8u);
    if (mcu_height == 0)
    {
        file_image_tjpgd_stream_close(ctx);
        return 0;
    }

    band_pixel_count = (uint32_t)jd.width * mcu_height;
    ctx->band_pixels = (uint16_t *)egui_malloc((int)(band_pixel_count * sizeof(uint16_t)));
    if (ctx->band_pixels == NULL)
    {
        file_image_tjpgd_stream_close(ctx);
        return 0;
    }

    ctx->width = jd.width;
    ctx->height = jd.height;
    ctx->mcu_height = mcu_height;
    ctx->band_pixel_count = band_pixel_count;

    out_info->width = ctx->width;
    out_info->height = ctx->height;
    out_info->has_alpha = 0;
    out_info->keep_file_open = 1;
    *decoder_ctx = ctx;
    return 1;
}

static int file_image_tjpgd_output(JDEC *jd, void *bitmap, JRECT *rect)
{
    file_image_tjpgd_session_t *session = (file_image_tjpgd_session_t *)jd->device;
    file_image_tjpgd_stream_ctx_t *ctx = session->ctx;
    uint16_t rect_width = (uint16_t)(rect->right - rect->left + 1u);

    if (rect->top >= session->band_end)
    {
        return 0;
    }

    if (rect->bottom >= session->band_start)
    {
        uint16_t copy_top = rect->top > session->band_start ? rect->top : session->band_start;
        uint16_t copy_bottom = rect->bottom < (uint16_t)(session->band_end - 1u) ? rect->bottom : (uint16_t)(session->band_end - 1u);
        uint16_t src_y;

        for (src_y = copy_top; src_y <= copy_bottom; src_y++)
        {
            uint16_t *src_row = (uint16_t *)bitmap + (uint32_t)(src_y - rect->top) * rect_width;
            uint16_t *dst_row = ctx->band_pixels + (uint32_t)(src_y - session->band_start) * ctx->width + rect->left;

            memcpy(dst_row, src_row, (uint32_t)rect_width * sizeof(uint16_t));
        }
    }

    if (rect->bottom + 1u >= session->band_end && rect->right + 1u >= ctx->width)
    {
        return 0;
    }

    return 1;
}

static int file_image_tjpgd_decode_band(file_image_tjpgd_stream_ctx_t *ctx, uint16_t row)
{
    JDEC jd;
    JRESULT rc;
    file_image_tjpgd_session_t session;

    if (ctx == NULL || row >= ctx->height)
    {
        return 0;
    }
    if (ctx->cached_band_start != 0xFFFFu && row >= ctx->cached_band_start && row < ctx->cached_band_end)
    {
        return 1;
    }

    memset(&session, 0, sizeof(session));
    rc = file_image_tjpgd_prepare_session(ctx, &session, &jd);
    if (rc != JDR_OK || jd.width != ctx->width || jd.height != ctx->height)
    {
        file_image_tjpgd_invalidate_band(ctx);
        return 0;
    }

    session.ctx = ctx;
    session.band_start = (uint16_t)((row / ctx->mcu_height) * ctx->mcu_height);
    session.band_end = (uint16_t)(session.band_start + ctx->mcu_height);
    if (session.band_end > ctx->height)
    {
        session.band_end = ctx->height;
    }

    rc = jd_decomp(&jd, file_image_tjpgd_output, 0);
    if (rc != JDR_OK && rc != JDR_INTR)
    {
        file_image_tjpgd_invalidate_band(ctx);
        return 0;
    }

    ctx->cached_band_start = session.band_start;
    ctx->cached_band_end = session.band_end;
    return 1;
}

static int file_image_tjpgd_stream_read_row(void *decoder_ctx, uint16_t row, uint16_t *rgb565_row, uint8_t *alpha_row)
{
    file_image_tjpgd_stream_ctx_t *ctx = (file_image_tjpgd_stream_ctx_t *)decoder_ctx;
    uint16_t row_offset;

    EGUI_UNUSED(alpha_row);
    if (ctx == NULL || rgb565_row == NULL || row >= ctx->height)
    {
        return 0;
    }
    if (!file_image_tjpgd_decode_band(ctx, row))
    {
        return 0;
    }

    row_offset = (uint16_t)(row - ctx->cached_band_start);
    memcpy(rgb565_row, ctx->band_pixels + (uint32_t)row_offset * ctx->width, (uint32_t)ctx->width * sizeof(uint16_t));
    return 1;
}

const egui_image_file_decoder_t g_file_image_tjpgd_stream_decoder = {
        .name = "tjpgd_stream",
        .match = file_image_tjpgd_match,
        .open = file_image_tjpgd_stream_open,
        .read_row = file_image_tjpgd_stream_read_row,
        .close = file_image_tjpgd_stream_close,
};
