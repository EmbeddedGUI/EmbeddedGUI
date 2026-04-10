#ifndef _FILE_IMAGE_STDIO_H_
#define _FILE_IMAGE_STDIO_H_

#include "image/egui_image_file.h"

typedef struct file_image_stdio_context
{
    const char *root_prefix;
} file_image_stdio_context_t;

void file_image_stdio_io_init(egui_image_file_io_t *io, file_image_stdio_context_t *ctx);

#endif /* _FILE_IMAGE_STDIO_H_ */
