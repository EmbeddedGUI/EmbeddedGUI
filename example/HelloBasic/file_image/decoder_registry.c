#include "decoder_registry.h"

static int file_image_decoder_registry_add(const egui_image_file_decoder_t *decoder)
{
    if (decoder == NULL)
    {
        return 1;
    }

    return egui_image_file_register_decoder(decoder);
}

int file_image_decoder_registry_apply(const file_image_decoder_registry_config_t *config)
{
    if (config == NULL)
    {
        return 0;
    }

    if (config->clear_first)
    {
        egui_image_file_clear_decoders();
    }

    if (!file_image_decoder_registry_add(config->bmp_stream))
    {
        return 0;
    }
    if (!file_image_decoder_registry_add(config->jpeg_vendor))
    {
        return 0;
    }
    if (!file_image_decoder_registry_add(config->jpeg_stream))
    {
        return 0;
    }
    if (!file_image_decoder_registry_add(config->png_vendor))
    {
        return 0;
    }
    if (!file_image_decoder_registry_add(config->generic_fallback))
    {
        return 0;
    }

    return 1;
}
