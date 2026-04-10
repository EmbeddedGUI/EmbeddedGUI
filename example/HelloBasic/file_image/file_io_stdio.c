#include <stdio.h>
#include <string.h>

#include "file_io_stdio.h"
#include "core/egui_common.h"

#define FILE_IMAGE_STDIO_MAX_PATH 256u

static int file_image_stdio_is_empty(const char *text)
{
    return text == NULL || text[0] == '\0';
}

static int file_image_stdio_build_path(const file_image_stdio_context_t *ctx, const char *path, char *dst, uint32_t dst_size)
{
    uint32_t prefix_len = 0;
    uint32_t path_len;

    if (dst == NULL || dst_size == 0 || path == NULL || path[0] == '\0')
    {
        return 0;
    }

    path_len = (uint32_t)strlen(path);
    if (ctx != NULL && !file_image_stdio_is_empty(ctx->root_prefix))
    {
        prefix_len = (uint32_t)strlen(ctx->root_prefix);
    }

    if (prefix_len + path_len + 1 > dst_size)
    {
        return 0;
    }

    if (prefix_len > 0)
    {
        memcpy(dst, ctx->root_prefix, prefix_len);
    }
    memcpy(dst + prefix_len, path, path_len + 1);
    return 1;
}

static void *file_image_stdio_open(void *user_data, const char *path)
{
    file_image_stdio_context_t *ctx = (file_image_stdio_context_t *)user_data;
    char full_path[FILE_IMAGE_STDIO_MAX_PATH];

    if (!file_image_stdio_build_path(ctx, path, full_path, sizeof(full_path)))
    {
        return NULL;
    }

    return fopen(full_path, "rb");
}

static int32_t file_image_stdio_read(void *user_data, void *handle, void *buf, uint32_t size)
{
    EGUI_UNUSED(user_data);
    if (handle == NULL || buf == NULL || size == 0)
    {
        return 0;
    }

    return (int32_t)fread(buf, 1, size, (FILE *)handle);
}

static int file_image_stdio_seek(void *user_data, void *handle, int32_t offset, int whence)
{
    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return -1;
    }

    return fseek((FILE *)handle, offset, whence);
}

static int32_t file_image_stdio_tell(void *user_data, void *handle)
{
    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return -1;
    }

    return (int32_t)ftell((FILE *)handle);
}

static void file_image_stdio_close(void *user_data, void *handle)
{
    EGUI_UNUSED(user_data);
    if (handle != NULL)
    {
        fclose((FILE *)handle);
    }
}

void file_image_stdio_io_init(egui_image_file_io_t *io, file_image_stdio_context_t *ctx)
{
    if (io == NULL)
    {
        return;
    }

    memset(io, 0, sizeof(*io));
    io->user_data = ctx;
    io->open = file_image_stdio_open;
    io->read = file_image_stdio_read;
    io->seek = file_image_stdio_seek;
    io->tell = file_image_stdio_tell;
    io->close = file_image_stdio_close;
}
