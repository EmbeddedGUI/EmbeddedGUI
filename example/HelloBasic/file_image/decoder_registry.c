#include "decoder_registry.h"

static int file_image_decoder_registry_add(egui_core_t *core, const egui_image_file_decoder_t *decoder)
{
    if (decoder == NULL)
    {
        return 1;
    }

    return egui_image_file_register_decoder(core, decoder);
}

int file_image_decoder_registry_apply(egui_core_t *core, const file_image_decoder_registry_config_t *config)
{
    if (config == NULL)
    {
        return 0;
    }

    if (config->clear_first)
    {
        egui_image_file_clear_decoders(core);
    }

    if (!file_image_decoder_registry_add(core, config->bmp_stream))
    {
        return 0;
    }
    if (!file_image_decoder_registry_add(core, config->jpeg_vendor))
    {
        return 0;
    }
    if (!file_image_decoder_registry_add(core, config->jpeg_stream))
    {
        return 0;
    }
    if (!file_image_decoder_registry_add(core, config->png_vendor))
    {
        return 0;
    }
    if (!file_image_decoder_registry_add(core, config->generic_fallback))
    {
        return 0;
    }

    return 1;
}
