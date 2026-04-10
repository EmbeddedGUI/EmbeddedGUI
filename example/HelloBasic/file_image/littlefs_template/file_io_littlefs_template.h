#ifndef _FILE_IMAGE_LITTLEFS_TEMPLATE_H_
#define _FILE_IMAGE_LITTLEFS_TEMPLATE_H_

#include "image/egui_image_file.h"
#include "lfs.h"

typedef struct file_image_littlefs_context
{
    lfs_t *lfs;
    const char *root_prefix;
} file_image_littlefs_context_t;

void file_image_littlefs_io_init(egui_image_file_io_t *io, file_image_littlefs_context_t *ctx);

#endif /* _FILE_IMAGE_LITTLEFS_TEMPLATE_H_ */
