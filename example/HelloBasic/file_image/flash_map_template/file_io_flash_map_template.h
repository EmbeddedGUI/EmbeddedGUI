#ifndef _FILE_IMAGE_FLASH_MAP_TEMPLATE_H_
#define _FILE_IMAGE_FLASH_MAP_TEMPLATE_H_

#include "image/egui_image_file.h"

typedef int32_t (*file_image_flash_map_read_cb_t)(void *user_data, uint32_t address, void *buf, uint32_t size);

typedef struct file_image_flash_map_entry
{
    const char *path;
    uint32_t offset;
    uint32_t size;
} file_image_flash_map_entry_t;

typedef struct file_image_flash_map_context
{
    uint32_t base_address;
    const file_image_flash_map_entry_t *entries;
    uint32_t entry_count;
    void *storage_user_data;
    file_image_flash_map_read_cb_t storage_read;
} file_image_flash_map_context_t;

void file_image_flash_map_io_init(egui_image_file_io_t *io, file_image_flash_map_context_t *ctx);

#endif /* _FILE_IMAGE_FLASH_MAP_TEMPLATE_H_ */
