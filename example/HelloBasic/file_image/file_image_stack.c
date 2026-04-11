#include "file_image_stack.h"

static int file_image_stack_apply_default_io(file_image_stack_state_t *state, const file_image_stack_config_t *config)
{
    int use_router;
    const egui_image_file_io_t *fallback_io;

    if (config == NULL)
    {
        return 0;
    }

    use_router = (config->mount_entries != NULL) || (config->mount_entry_count != 0) || (config->fallback_io != NULL);
    if (!use_router)
    {
        if (config->default_io == NULL)
        {
            return 0;
        }

        egui_image_file_set_default_io(config->default_io);
        return 1;
    }

    if (state == NULL)
    {
        return 0;
    }
    if (config->mount_entry_count != 0 && config->mount_entries == NULL)
    {
        return 0;
    }

    fallback_io = config->fallback_io != NULL ? config->fallback_io : config->default_io;
    if (config->mount_entry_count == 0 && fallback_io == NULL)
    {
        return 0;
    }

    state->router_ctx.entries = config->mount_entries;
    state->router_ctx.entry_count = config->mount_entry_count;
    state->router_ctx.fallback_io = fallback_io;
    file_image_mount_router_io_init(&state->router_io, &state->router_ctx);
    egui_image_file_set_default_io(&state->router_io);
    return 1;
}

int file_image_stack_apply(file_image_stack_state_t *state, const file_image_stack_config_t *config)
{
    if (!file_image_stack_apply_default_io(state, config))
    {
        return 0;
    }

    if (config != NULL && config->decoder_config != NULL && !file_image_decoder_registry_apply(config->decoder_config))
    {
        return 0;
    }

    return 1;
}
