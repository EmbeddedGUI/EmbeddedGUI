#include <string.h>

#include "file_io_fatfs_template.h"
#include "core/egui_common.h"
#include "ff.h"

#define FILE_IMAGE_FATFS_MAX_PATH 256u

typedef struct file_image_fatfs_handle
{
    FIL fil;
} file_image_fatfs_handle_t;

static int file_image_fatfs_is_empty(const char *text)
{
    return text == NULL || text[0] == '\0';
}

static int file_image_fatfs_build_path(const file_image_fatfs_context_t *ctx, const char *path, char *dst, uint32_t dst_size)
{
    uint32_t prefix_len = 0;
    uint32_t path_len;

    if (dst == NULL || dst_size == 0 || path == NULL || path[0] == '\0')
    {
        return 0;
    }

    path_len = (uint32_t)strlen(path);
    if (ctx != NULL && !file_image_fatfs_is_empty(ctx->root_prefix))
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

static FSIZE_t file_image_fatfs_calc_seek_target(file_image_fatfs_handle_t *handle, int32_t offset, int whence)
{
    FSIZE_t base = 0;

    if (handle == NULL)
    {
        return 0;
    }

    switch (whence)
    {
    case EGUI_IMAGE_FILE_SEEK_CUR:
        base = f_tell(&handle->fil);
        break;
    case EGUI_IMAGE_FILE_SEEK_END:
        base = f_size(&handle->fil);
        break;
    case EGUI_IMAGE_FILE_SEEK_SET:
    default:
        base = 0;
        break;
    }

    if (offset < 0)
    {
        uint32_t delta = (uint32_t)(-offset);
        return base > delta ? (FSIZE_t)(base - delta) : 0;
    }

    return (FSIZE_t)(base + (FSIZE_t)(uint32_t)offset);
}

static void *file_image_fatfs_open(void *user_data, const char *path)
{
    file_image_fatfs_context_t *ctx = (file_image_fatfs_context_t *)user_data;
    file_image_fatfs_handle_t *handle;
    char full_path[FILE_IMAGE_FATFS_MAX_PATH];
    FRESULT fr;

    if (!file_image_fatfs_build_path(ctx, path, full_path, sizeof(full_path)))
    {
        return NULL;
    }

    handle = (file_image_fatfs_handle_t *)egui_malloc(sizeof(*handle));
    if (handle == NULL)
    {
        return NULL;
    }

    memset(handle, 0, sizeof(*handle));
    fr = f_open(&handle->fil, full_path, FA_READ);
    if (fr != FR_OK)
    {
        egui_free(handle);
        return NULL;
    }

    return handle;
}

static int32_t file_image_fatfs_read(void *user_data, void *handle_ptr, void *buf, uint32_t size)
{
    file_image_fatfs_handle_t *handle = (file_image_fatfs_handle_t *)handle_ptr;
    UINT read_len = 0;

    EGUI_UNUSED(user_data);
    if (handle == NULL || buf == NULL || size == 0)
    {
        return 0;
    }

    if (f_read(&handle->fil, buf, (UINT)size, &read_len) != FR_OK)
    {
        return -1;
    }

    return (int32_t)read_len;
}

static int file_image_fatfs_seek(void *user_data, void *handle_ptr, int32_t offset, int whence)
{
    file_image_fatfs_handle_t *handle = (file_image_fatfs_handle_t *)handle_ptr;
    FSIZE_t target;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return -1;
    }

    target = file_image_fatfs_calc_seek_target(handle, offset, whence);
    return f_lseek(&handle->fil, target) == FR_OK ? 0 : -1;
}

static int32_t file_image_fatfs_tell(void *user_data, void *handle_ptr)
{
    file_image_fatfs_handle_t *handle = (file_image_fatfs_handle_t *)handle_ptr;
    FSIZE_t position;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return -1;
    }

    position = f_tell(&handle->fil);
    if (position > 0x7FFFFFFFul)
    {
        return -1;
    }

    return (int32_t)position;
}

static void file_image_fatfs_close(void *user_data, void *handle_ptr)
{
    file_image_fatfs_handle_t *handle = (file_image_fatfs_handle_t *)handle_ptr;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return;
    }

    f_close(&handle->fil);
    egui_free(handle);
}

void file_image_fatfs_io_init(egui_image_file_io_t *io, file_image_fatfs_context_t *ctx)
{
    if (io == NULL)
    {
        return;
    }

    memset(io, 0, sizeof(*io));
    io->user_data = ctx;
    io->open = file_image_fatfs_open;
    io->read = file_image_fatfs_read;
    io->seek = file_image_fatfs_seek;
    io->tell = file_image_fatfs_tell;
    io->close = file_image_fatfs_close;
}
