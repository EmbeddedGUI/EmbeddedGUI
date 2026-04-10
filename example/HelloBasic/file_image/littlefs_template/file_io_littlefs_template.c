#include <string.h>

#include "file_io_littlefs_template.h"
#include "core/egui_common.h"

#define FILE_IMAGE_LITTLEFS_MAX_PATH 256u

typedef struct file_image_littlefs_handle
{
    lfs_t *lfs;
    lfs_file_t file;
} file_image_littlefs_handle_t;

static int file_image_littlefs_is_empty(const char *text)
{
    return text == NULL || text[0] == '\0';
}

static int file_image_littlefs_build_path(const file_image_littlefs_context_t *ctx, const char *path, char *dst, uint32_t dst_size)
{
    uint32_t prefix_len = 0;
    uint32_t path_len;

    if (dst == NULL || dst_size == 0 || path == NULL || path[0] == '\0')
    {
        return 0;
    }

    path_len = (uint32_t)strlen(path);
    if (ctx != NULL && !file_image_littlefs_is_empty(ctx->root_prefix))
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

static void *file_image_littlefs_open(void *user_data, const char *path)
{
    file_image_littlefs_context_t *ctx = (file_image_littlefs_context_t *)user_data;
    file_image_littlefs_handle_t *handle;
    char full_path[FILE_IMAGE_LITTLEFS_MAX_PATH];

    if (ctx == NULL || ctx->lfs == NULL)
    {
        return NULL;
    }

    if (!file_image_littlefs_build_path(ctx, path, full_path, sizeof(full_path)))
    {
        return NULL;
    }

    handle = (file_image_littlefs_handle_t *)egui_malloc(sizeof(*handle));
    if (handle == NULL)
    {
        return NULL;
    }

    memset(handle, 0, sizeof(*handle));
    handle->lfs = ctx->lfs;
    if (lfs_file_open(handle->lfs, &handle->file, full_path, LFS_O_RDONLY) < 0)
    {
        egui_free(handle);
        return NULL;
    }

    return handle;
}

static int32_t file_image_littlefs_read(void *user_data, void *handle_ptr, void *buf, uint32_t size)
{
    file_image_littlefs_handle_t *handle = (file_image_littlefs_handle_t *)handle_ptr;
    lfs_ssize_t read_len;

    EGUI_UNUSED(user_data);
    if (handle == NULL || buf == NULL || size == 0)
    {
        return 0;
    }

    read_len = lfs_file_read(handle->lfs, &handle->file, buf, size);
    if (read_len < 0)
    {
        return -1;
    }

    if ((uint32_t)read_len > 0x7FFFFFFFul)
    {
        return -1;
    }

    return (int32_t)read_len;
}

static int file_image_littlefs_seek(void *user_data, void *handle_ptr, int32_t offset, int whence)
{
    file_image_littlefs_handle_t *handle = (file_image_littlefs_handle_t *)handle_ptr;
    int origin;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return -1;
    }

    switch (whence)
    {
    case EGUI_IMAGE_FILE_SEEK_CUR:
        origin = LFS_SEEK_CUR;
        break;
    case EGUI_IMAGE_FILE_SEEK_END:
        origin = LFS_SEEK_END;
        break;
    case EGUI_IMAGE_FILE_SEEK_SET:
    default:
        origin = LFS_SEEK_SET;
        break;
    }

    return lfs_file_seek(handle->lfs, &handle->file, offset, origin) < 0 ? -1 : 0;
}

static int32_t file_image_littlefs_tell(void *user_data, void *handle_ptr)
{
    file_image_littlefs_handle_t *handle = (file_image_littlefs_handle_t *)handle_ptr;
    lfs_soff_t position;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return -1;
    }

    position = lfs_file_tell(handle->lfs, &handle->file);
    if (position < 0 || (uint32_t)position > 0x7FFFFFFFul)
    {
        return -1;
    }

    return (int32_t)position;
}

static void file_image_littlefs_close(void *user_data, void *handle_ptr)
{
    file_image_littlefs_handle_t *handle = (file_image_littlefs_handle_t *)handle_ptr;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return;
    }

    lfs_file_close(handle->lfs, &handle->file);
    egui_free(handle);
}

void file_image_littlefs_io_init(egui_image_file_io_t *io, file_image_littlefs_context_t *ctx)
{
    if (io == NULL)
    {
        return;
    }

    memset(io, 0, sizeof(*io));
    io->user_data = ctx;
    io->open = file_image_littlefs_open;
    io->read = file_image_littlefs_read;
    io->seek = file_image_littlefs_seek;
    io->tell = file_image_littlefs_tell;
    io->close = file_image_littlefs_close;
}
