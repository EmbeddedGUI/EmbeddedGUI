#ifndef _FILE_IMAGE_MOUNT_ROUTER_TEMPLATE_H_
#define _FILE_IMAGE_MOUNT_ROUTER_TEMPLATE_H_

#include "image/egui_image_file.h"

typedef struct file_image_mount_router_entry
{
    const char *prefix;
    const egui_image_file_io_t *io;
    uint8_t strip_prefix;
} file_image_mount_router_entry_t;

typedef struct file_image_mount_router_context
{
    const file_image_mount_router_entry_t *entries;
    uint32_t entry_count;
    const egui_image_file_io_t *fallback_io;
} file_image_mount_router_context_t;

void file_image_mount_router_io_init(egui_image_file_io_t *io, file_image_mount_router_context_t *ctx);

#endif /* _FILE_IMAGE_MOUNT_ROUTER_TEMPLATE_H_ */
