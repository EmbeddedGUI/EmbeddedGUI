#include <string.h>

#include "file_io_flash_map_template.h"
#include "core/egui_common.h"

typedef struct file_image_flash_map_handle
{
    file_image_flash_map_context_t *ctx;
    const file_image_flash_map_entry_t *entry;
    uint32_t cursor;
} file_image_flash_map_handle_t;

static const file_image_flash_map_entry_t *file_image_flash_map_find_entry(const file_image_flash_map_context_t *ctx, const char *path)
{
    uint32_t i;

    if (ctx == NULL || path == NULL || path[0] == '\0')
    {
        return NULL;
    }

    for (i = 0; i < ctx->entry_count; i++)
    {
        const file_image_flash_map_entry_t *entry = &ctx->entries[i];

        if (entry->path != NULL && strcmp(entry->path, path) == 0)
        {
            return entry;
        }
    }

    return NULL;
}

static uint32_t file_image_flash_map_calc_seek_target(const file_image_flash_map_handle_t *handle, int32_t offset, int whence)
{
    int64_t base = 0;
    int64_t target;

    if (handle == NULL || handle->entry == NULL)
    {
        return 0;
    }

    switch (whence)
    {
    case EGUI_IMAGE_FILE_SEEK_CUR:
        base = handle->cursor;
        break;
    case EGUI_IMAGE_FILE_SEEK_END:
        base = handle->entry->size;
        break;
    case EGUI_IMAGE_FILE_SEEK_SET:
    default:
        base = 0;
        break;
    }

    target = base + offset;
    if (target <= 0)
    {
        return 0;
    }
    if ((uint64_t)target >= handle->entry->size)
    {
        return handle->entry->size;
    }

    return (uint32_t)target;
}

static void *file_image_flash_map_open(void *user_data, const char *path)
{
    file_image_flash_map_context_t *ctx = (file_image_flash_map_context_t *)user_data;
    file_image_flash_map_handle_t *handle;
    const file_image_flash_map_entry_t *entry;

    if (ctx == NULL || ctx->entries == NULL || ctx->entry_count == 0 || ctx->storage_read == NULL)
    {
        return NULL;
    }

    entry = file_image_flash_map_find_entry(ctx, path);
    if (entry == NULL)
    {
        return NULL;
    }

    handle = (file_image_flash_map_handle_t *)egui_malloc(sizeof(*handle));
    if (handle == NULL)
    {
        return NULL;
    }

    memset(handle, 0, sizeof(*handle));
    handle->ctx = ctx;
    handle->entry = entry;
    return handle;
}

static int32_t file_image_flash_map_read(void *user_data, void *handle_ptr, void *buf, uint32_t size)
{
    file_image_flash_map_handle_t *handle = (file_image_flash_map_handle_t *)handle_ptr;
    uint32_t remain;
    uint32_t read_size;
    uint32_t address;
    int32_t read_len;

    EGUI_UNUSED(user_data);
    if (handle == NULL || handle->ctx == NULL || handle->entry == NULL || buf == NULL || size == 0)
    {
        return 0;
    }

    remain = handle->entry->size - handle->cursor;
    if (remain == 0)
    {
        return 0;
    }

    read_size = EGUI_MIN(size, remain);
    address = handle->ctx->base_address + handle->entry->offset + handle->cursor;
    read_len = handle->ctx->storage_read(handle->ctx->storage_user_data, address, buf, read_size);
    if (read_len <= 0)
    {
        return read_len;
    }

    if ((uint32_t)read_len > read_size)
    {
        read_len = (int32_t)read_size;
    }
    handle->cursor += (uint32_t)read_len;
    return read_len;
}

static int file_image_flash_map_seek(void *user_data, void *handle_ptr, int32_t offset, int whence)
{
    file_image_flash_map_handle_t *handle = (file_image_flash_map_handle_t *)handle_ptr;

    EGUI_UNUSED(user_data);
    if (handle == NULL || handle->entry == NULL)
    {
        return -1;
    }

    handle->cursor = file_image_flash_map_calc_seek_target(handle, offset, whence);
    return 0;
}

static int32_t file_image_flash_map_tell(void *user_data, void *handle_ptr)
{
    file_image_flash_map_handle_t *handle = (file_image_flash_map_handle_t *)handle_ptr;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return -1;
    }

    return (int32_t)handle->cursor;
}

static void file_image_flash_map_close(void *user_data, void *handle_ptr)
{
    file_image_flash_map_handle_t *handle = (file_image_flash_map_handle_t *)handle_ptr;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return;
    }

    egui_free(handle);
}

void file_image_flash_map_io_init(egui_image_file_io_t *io, file_image_flash_map_context_t *ctx)
{
    if (io == NULL)
    {
        return;
    }

    memset(io, 0, sizeof(*io));
    io->user_data = ctx;
    io->open = file_image_flash_map_open;
    io->read = file_image_flash_map_read;
    io->seek = file_image_flash_map_seek;
    io->tell = file_image_flash_map_tell;
    io->close = file_image_flash_map_close;
}
