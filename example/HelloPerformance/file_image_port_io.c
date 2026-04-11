#include <string.h>

#if EGUI_PORT != EGUI_PORT_TYPE_QEMU
#include <stdio.h>
#endif

#include "egui.h"

#include "file_image_port_io.h"

#define FILE_IMAGE_PORT_MAX_PATH 256u

static int file_image_port_is_empty(const char *text)
{
    return text == NULL || text[0] == '\0';
}

static int file_image_port_build_path(const char *prefix, const char *path, char *dst, uint32_t dst_size)
{
    uint32_t prefix_len = 0;
    uint32_t path_len;

    if (dst == NULL || dst_size == 0 || path == NULL || path[0] == '\0')
    {
        return 0;
    }

    path_len = (uint32_t)strlen(path);
    if (!file_image_port_is_empty(prefix))
    {
        prefix_len = (uint32_t)strlen(prefix);
    }

    if (prefix_len + path_len + 1 > dst_size)
    {
        return 0;
    }

    if (prefix_len > 0)
    {
        memcpy(dst, prefix, prefix_len);
    }
    memcpy(dst + prefix_len, path, path_len + 1);
    return 1;
}

#if EGUI_PORT == EGUI_PORT_TYPE_QEMU

enum
{
    FILE_IMAGE_PORT_QEMU_SYS_OPEN = 0x01U,
    FILE_IMAGE_PORT_QEMU_SYS_CLOSE = 0x02U,
    FILE_IMAGE_PORT_QEMU_SYS_READ = 0x06U,
    FILE_IMAGE_PORT_QEMU_SYS_SEEK = 0x0AU,
    FILE_IMAGE_PORT_QEMU_SYS_FLEN = 0x0CU,
    FILE_IMAGE_PORT_QEMU_MODE_READ_BINARY = 0x01U,
};

typedef struct file_image_port_qemu_open_args
{
    const char *path;
    uint32_t mode;
    uint32_t path_length;
} file_image_port_qemu_open_args_t;

typedef struct file_image_port_qemu_read_args
{
    int handle;
    void *buffer;
    uint32_t size;
} file_image_port_qemu_read_args_t;

typedef struct file_image_port_qemu_seek_args
{
    int handle;
    uint32_t offset;
} file_image_port_qemu_seek_args_t;

typedef struct file_image_port_qemu_handle
{
    int semihosting_handle;
    uint32_t position;
    uint32_t size;
} file_image_port_qemu_handle_t;

static int file_image_port_qemu_semihosting_call(uint32_t operation, void *arguments)
{
    register uint32_t r0 __asm__("r0") = operation;
    register void *r1 __asm__("r1") = arguments;

    __asm__ volatile("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
    return (int)r0;
}

static int file_image_port_qemu_open_raw(const char *full_path)
{
    file_image_port_qemu_open_args_t args = {
            .path = full_path,
            .mode = FILE_IMAGE_PORT_QEMU_MODE_READ_BINARY,
            .path_length = (uint32_t)strlen(full_path),
    };

    return file_image_port_qemu_semihosting_call(FILE_IMAGE_PORT_QEMU_SYS_OPEN, &args);
}

static int file_image_port_qemu_close_raw(int handle)
{
    return file_image_port_qemu_semihosting_call(FILE_IMAGE_PORT_QEMU_SYS_CLOSE, &handle);
}

static int file_image_port_qemu_seek_raw(int handle, uint32_t offset)
{
    file_image_port_qemu_seek_args_t args = {
            .handle = handle,
            .offset = offset,
    };

    return file_image_port_qemu_semihosting_call(FILE_IMAGE_PORT_QEMU_SYS_SEEK, &args);
}

static int file_image_port_qemu_get_size_raw(int handle, uint32_t *out_size)
{
    int size;

    if (out_size == NULL)
    {
        return 0;
    }

    size = file_image_port_qemu_semihosting_call(FILE_IMAGE_PORT_QEMU_SYS_FLEN, &handle);
    if (size < 0)
    {
        return 0;
    }

    *out_size = (uint32_t)size;
    return 1;
}

static void *file_image_port_open_one(const char *full_path)
{
    int semi_handle;
    uint32_t size;
    file_image_port_qemu_handle_t *handle;

    semi_handle = file_image_port_qemu_open_raw(full_path);
    if (semi_handle < 0)
    {
        return NULL;
    }
    if (!file_image_port_qemu_get_size_raw(semi_handle, &size))
    {
        file_image_port_qemu_close_raw(semi_handle);
        return NULL;
    }

    handle = (file_image_port_qemu_handle_t *)egui_malloc(sizeof(*handle));
    if (handle == NULL)
    {
        file_image_port_qemu_close_raw(semi_handle);
        return NULL;
    }

    handle->semihosting_handle = semi_handle;
    handle->position = 0;
    handle->size = size;
    return handle;
}

#else

static void *file_image_port_open_one(const char *full_path)
{
    return fopen(full_path, "rb");
}

#endif

static void *file_image_port_open(void *user_data, const char *path)
{
    file_image_port_context_t *ctx = (file_image_port_context_t *)user_data;
    const char *prefixes[3];
    char full_path[FILE_IMAGE_PORT_MAX_PATH];
    uint32_t i;

    if (path == NULL || path[0] == '\0')
    {
        return NULL;
    }

    prefixes[0] = ctx != NULL ? ctx->root_prefix : NULL;
    prefixes[1] = ctx != NULL ? ctx->alt_root_prefix : NULL;
    prefixes[2] = "";

    for (i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); i++)
    {
        const char *prefix = prefixes[i];

        if (prefix == NULL)
        {
            continue;
        }
        if (!file_image_port_build_path(prefix, path, full_path, sizeof(full_path)))
        {
            continue;
        }

        {
            void *handle = file_image_port_open_one(full_path);

            if (handle != NULL)
            {
                return handle;
            }
        }
    }

    return NULL;
}

#if EGUI_PORT == EGUI_PORT_TYPE_QEMU

static int32_t file_image_port_read(void *user_data, void *handle_ptr, void *buf, uint32_t size)
{
    file_image_port_qemu_handle_t *handle = (file_image_port_qemu_handle_t *)handle_ptr;
    file_image_port_qemu_read_args_t args;
    int unread_count;
    uint32_t actual_size;

    EGUI_UNUSED(user_data);
    if (handle == NULL || buf == NULL || size == 0)
    {
        return 0;
    }

    args.handle = handle->semihosting_handle;
    args.buffer = buf;
    args.size = size;
    unread_count = file_image_port_qemu_semihosting_call(FILE_IMAGE_PORT_QEMU_SYS_READ, &args);
    if (unread_count < 0 || (uint32_t)unread_count > size)
    {
        return -1;
    }

    actual_size = size - (uint32_t)unread_count;
    handle->position += actual_size;
    return (int32_t)actual_size;
}

static int file_image_port_seek(void *user_data, void *handle_ptr, int32_t offset, int whence)
{
    file_image_port_qemu_handle_t *handle = (file_image_port_qemu_handle_t *)handle_ptr;
    int64_t base = 0;
    int64_t target;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return -1;
    }

    switch (whence)
    {
    case EGUI_IMAGE_FILE_SEEK_SET:
        base = 0;
        break;
    case EGUI_IMAGE_FILE_SEEK_CUR:
        base = handle->position;
        break;
    case EGUI_IMAGE_FILE_SEEK_END:
        base = handle->size;
        break;
    default:
        return -1;
    }

    target = base + offset;
    if (target < 0)
    {
        return -1;
    }
    if ((uint64_t)target > handle->size)
    {
        target = handle->size;
    }

    if (file_image_port_qemu_seek_raw(handle->semihosting_handle, (uint32_t)target) != 0)
    {
        return -1;
    }

    handle->position = (uint32_t)target;
    return 0;
}

static int32_t file_image_port_tell(void *user_data, void *handle_ptr)
{
    file_image_port_qemu_handle_t *handle = (file_image_port_qemu_handle_t *)handle_ptr;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return -1;
    }

    return (int32_t)handle->position;
}

static void file_image_port_close(void *user_data, void *handle_ptr)
{
    file_image_port_qemu_handle_t *handle = (file_image_port_qemu_handle_t *)handle_ptr;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return;
    }

    file_image_port_qemu_close_raw(handle->semihosting_handle);
    egui_free(handle);
}

#else

static int32_t file_image_port_read(void *user_data, void *handle, void *buf, uint32_t size)
{
    EGUI_UNUSED(user_data);
    if (handle == NULL || buf == NULL || size == 0)
    {
        return 0;
    }

    return (int32_t)fread(buf, 1, size, (FILE *)handle);
}

static int file_image_port_seek(void *user_data, void *handle, int32_t offset, int whence)
{
    int file_whence;

    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return -1;
    }

    switch (whence)
    {
    case EGUI_IMAGE_FILE_SEEK_SET:
        file_whence = SEEK_SET;
        break;
    case EGUI_IMAGE_FILE_SEEK_CUR:
        file_whence = SEEK_CUR;
        break;
    case EGUI_IMAGE_FILE_SEEK_END:
        file_whence = SEEK_END;
        break;
    default:
        return -1;
    }

    return fseek((FILE *)handle, offset, file_whence);
}

static int32_t file_image_port_tell(void *user_data, void *handle)
{
    EGUI_UNUSED(user_data);
    if (handle == NULL)
    {
        return -1;
    }

    return (int32_t)ftell((FILE *)handle);
}

static void file_image_port_close(void *user_data, void *handle)
{
    EGUI_UNUSED(user_data);
    if (handle != NULL)
    {
        fclose((FILE *)handle);
    }
}

#endif

void file_image_port_io_init(egui_image_file_io_t *io, file_image_port_context_t *ctx)
{
    if (io == NULL)
    {
        return;
    }

    memset(io, 0, sizeof(*io));
    io->user_data = ctx;
    io->open = file_image_port_open;
    io->read = file_image_port_read;
    io->seek = file_image_port_seek;
    io->tell = file_image_port_tell;
    io->close = file_image_port_close;
}
