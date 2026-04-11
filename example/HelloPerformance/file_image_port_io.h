#ifndef _FILE_IMAGE_PORT_IO_H_
#define _FILE_IMAGE_PORT_IO_H_

#include "image/egui_image_file.h"

typedef struct file_image_port_context
{
    const char *root_prefix;
    const char *alt_root_prefix;
} file_image_port_context_t;

void file_image_port_io_init(egui_image_file_io_t *io, file_image_port_context_t *ctx);

#endif /* _FILE_IMAGE_PORT_IO_H_ */
