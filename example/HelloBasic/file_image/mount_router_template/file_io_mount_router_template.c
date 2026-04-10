#include <string.h>

#include "file_io_mount_router_template.h"
#include "core/egui_common.h"

typedef struct file_image_mount_router_handle
{
    const egui_image_file_io_t *inner_io;
    void *inner_handle;
} file_image_mount_router_handle_t;

static uint32_t file_image_mount_router_prefix_len(const char *prefix)
{
    return prefix == NULL ? 0u : (uint32_t)strlen(prefix);
}

static int file_image_mount_router_path_has_prefix(const char *path, const char *prefix)
{
    uint32_t prefix_len;

    if (path == NULL || prefix == NULL)
    {
        return 0;
    }

    prefix_len = file_image_mount_router_prefix_len(prefix);
    if (prefix_len == 0)
    {
        return 0;
    }

    return strncmp(path, prefix, prefix_len) == 0;
}

static const file_image_mount_router_entry_t *file_image_mount_router_find_entry(const file_image_mount_router_context_t *ctx, const char *path)
{
    const file_image_mount_router_entry_t *best = NULL;
    uint32_t best_prefix_len = 0;
    uint32_t i;

    if (ctx == NULL || ctx->entries == NULL || path == NULL)
    {
        return NULL;
    }

    for (i = 0; i < ctx->entry_count; i++)
    {
        const file_image_mount_router_entry_t *entry = &ctx->entries[i];
        uint32_t prefix_len;

        if (entry->io == NULL || !file_image_mount_router_path_has_prefix(path, entry->prefix))
        {
            continue;
        }

        prefix_len = file_image_mount_router_prefix_len(entry->prefix);
        if (best == NULL || prefix_len > best_prefix_len)
        {
            best = entry;
            best_prefix_len = prefix_len;
        }
    }

    return best;
}

static void *file_image_mount_router_open(void *user_data, const char *path)
{
    file_image_mount_router_context_t *ctx = (file_image_mount_router_context_t *)user_data;
    const file_image_mount_router_entry_t *entry;
    const egui_image_file_io_t *inner_io;
    const char *inner_path;
    void *inner_handle;
    file_image_mount_router_handle_t *handle;

    if (path == NULL || path[0] == '\0')
    {
        return NULL;
    }

    entry = file_image_mount_router_find_entry(ctx, path);
    if (entry != NULL)
    {
        inner_io = entry->io;
        inner_path = entry->strip_prefix ? path + file_image_mount_router_prefix_len(entry->prefix) : path;
    }
    else if (ctx != NULL)
    {
        inner_io = ctx->fallback_io;
        inner_path = path;
    }
    else
    {
        inner_io = NULL;
        inner_path = path;
    }

    if (inner_io == NULL || inner_io->open == NULL)
    {
        return NULL;
    }

    inner_handle = inner_io->open(inner_io->user_data, inner_path);
    if (inner_handle == NULL)
    {
        return NULL;
    }

    handle = (file_image_mount_router_handle_t *)egui_malloc(sizeof(*handle));
    if (handle == NULL)
    {
        if (inner_io->close != NULL)
        {
            inner_io->close(inner_io->user_data, inner_handle);
        }
        return NULL;
    }

    handle->inner_io = inner_io;
    handle->inner_handle = inner_handle;
    return handle;
}

static int32_t file_image_mount_router_read(void *user_data, void *handle_ptr, void *buf, uint32_t size)
{
    file_image_mount_router_handle_t *handle = (file_image_mount_router_handle_t *)handle_ptr;

    EGUI_UNUSED(user_data);
    if (handle == NULL || handle->inner_io == NULL || handle->inner_io->read == NULL)
    {
        return -1;
    }

    return handle->inner_io->read(handle->inner_io->user_data, handle->inner_handle, buf, size);
}

static int file_image_mount_router_seek(void *user_data, void *handle_ptr, int32_t offset, int whence)
{
    file_image_mount_router_handle_t *handle = (file_image_mount_router_handle_t *)handle_ptr;

    EGUI_UNUSED(user_data);
    if (handle == NULL || handle->inner_io == NULL || handle->inner_io->seek == NULL)
    {
        return -1;
    }

    return handle->inner_io->seek(handle->inner_io->user_data, handle->inner_handle, offset, whence);
}

static int32_t file_image_mount_router_tell(void *user_data, void *handle_ptr)
{
    file_image_mount_router_handle_t *handle = (file_image_mount_router_handle_t *)handle_ptr;

    EGUI_UNUSED(user_data);
    if (handle == NULL || handle->inner_io == NULL || handle->inner_io->tell == NULL)
    {
        return -1;
    }

    return handle->inner_io->tell(handle->inner_io->user_data, handle->inner_handle);
}

static void file_image_mount_router_close(void *user_data, void *handle_ptr)
{
    file_image_mount_router_handle_t *handle = (file_image_mount_router_handle_t *)handle_ptr;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return;
    }

    if (handle->inner_io != NULL && handle->inner_io->close != NULL)
    {
        handle->inner_io->close(handle->inner_io->user_data, handle->inner_handle);
    }
    egui_free(handle);
}

void file_image_mount_router_io_init(egui_image_file_io_t *io, file_image_mount_router_context_t *ctx)
{
    if (io == NULL)
    {
        return;
    }

    memset(io, 0, sizeof(*io));
    io->user_data = ctx;
    io->open = file_image_mount_router_open;
    io->read = file_image_mount_router_read;
    io->seek = file_image_mount_router_seek;
    io->tell = file_image_mount_router_tell;
    io->close = file_image_mount_router_close;
}
