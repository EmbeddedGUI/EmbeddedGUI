#ifndef _FILE_IMAGE_STACK_H_
#define _FILE_IMAGE_STACK_H_

#include "decoder_registry.h"
#include "mount_router_template/file_io_mount_router_template.h"

typedef struct file_image_stack_state
{
    egui_image_file_io_t router_io;
    file_image_mount_router_context_t router_ctx;
} file_image_stack_state_t;

typedef struct file_image_stack_config
{
    const egui_image_file_io_t *default_io;
    const file_image_mount_router_entry_t *mount_entries;
    uint32_t mount_entry_count;
    const egui_image_file_io_t *fallback_io;
    const file_image_decoder_registry_config_t *decoder_config;
} file_image_stack_config_t;

/* state must outlive the file image usage because default io keeps pointers into it. */
int file_image_stack_apply(file_image_stack_state_t *state, const file_image_stack_config_t *config);

#endif /* _FILE_IMAGE_STACK_H_ */
