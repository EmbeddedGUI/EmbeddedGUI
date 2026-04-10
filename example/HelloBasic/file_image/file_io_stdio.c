#include <stdio.h>

#include "file_io_stdio.h"
#include "core/egui_common.h"

static void *file_image_stdio_open(void *user_data, const char *path)
{
    EGUI_UNUSED(user_data);
    return fopen(path, "rb");
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

const egui_image_file_io_t g_file_image_stdio_io = {
        .user_data = NULL,
        .open = file_image_stdio_open,
        .read = file_image_stdio_read,
        .seek = file_image_stdio_seek,
        .tell = file_image_stdio_tell,
        .close = file_image_stdio_close,
};
