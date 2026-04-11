#ifndef _FILE_IMAGE_DECODER_REGISTRY_H_
#define _FILE_IMAGE_DECODER_REGISTRY_H_

#include "image/egui_image_file.h"

typedef struct file_image_decoder_registry_config
{
    const egui_image_file_decoder_t *bmp_stream;
    const egui_image_file_decoder_t *jpeg_vendor;
    const egui_image_file_decoder_t *jpeg_stream;
    const egui_image_file_decoder_t *png_vendor;
    const egui_image_file_decoder_t *generic_fallback;
    uint8_t clear_first;
} file_image_decoder_registry_config_t;

int file_image_decoder_registry_apply(const file_image_decoder_registry_config_t *config);

#endif /* _FILE_IMAGE_DECODER_REGISTRY_H_ */
