#ifndef _EGUI_IMAGE_FILE_H_
#define _EGUI_IMAGE_FILE_H_

#include "egui_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE

#define EGUI_IMAGE_FILE_SEEK_SET 0
#define EGUI_IMAGE_FILE_SEEK_CUR 1
#define EGUI_IMAGE_FILE_SEEK_END 2

typedef struct egui_image_file_io egui_image_file_io_t;
typedef struct egui_image_file_decoder egui_image_file_decoder_t;
typedef struct egui_image_file_open_result egui_image_file_open_result_t;
typedef struct egui_image_file egui_image_file_t;

typedef enum
{
    EGUI_IMAGE_FILE_STATUS_IDLE = 0,
    EGUI_IMAGE_FILE_STATUS_READY,
    EGUI_IMAGE_FILE_STATUS_NO_PATH,
    EGUI_IMAGE_FILE_STATUS_NO_IO,
    EGUI_IMAGE_FILE_STATUS_NO_DECODER,
    EGUI_IMAGE_FILE_STATUS_OPEN_FILE_FAIL,
    EGUI_IMAGE_FILE_STATUS_OPEN_DECODER_FAIL,
    EGUI_IMAGE_FILE_STATUS_DECODE_ROW_FAIL,
    EGUI_IMAGE_FILE_STATUS_OOM,
} egui_image_file_status_t;

struct egui_image_file_io
{
    void *user_data;
    void *(*open)(void *user_data, const char *path);
    int32_t (*read)(void *user_data, void *handle, void *buf, uint32_t size);
    int (*seek)(void *user_data, void *handle, int32_t offset, int whence);
    int32_t (*tell)(void *user_data, void *handle);
    void (*close)(void *user_data, void *handle);
};

struct egui_image_file_open_result
{
    uint16_t width;
    uint16_t height;
    uint8_t has_alpha;
    uint8_t keep_file_open;
};

struct egui_image_file_decoder
{
    const char *name;
    int (*match)(const char *path);
    int (*open)(const egui_image_file_io_t *io, void *file_handle, const char *path, void **decoder_ctx, egui_image_file_open_result_t *out_info);
    int (*read_row)(void *decoder_ctx, uint16_t row, uint16_t *rgb565_row, uint8_t *alpha_row);
    void (*close)(void *decoder_ctx);
};

struct egui_image_file
{
    egui_image_t base;

    char *path;
    const egui_image_file_io_t *io;
    const egui_image_file_io_t *active_io;
    const egui_image_file_decoder_t *decoder;
    const egui_image_t *placeholder;
    void *file_handle;
    void *decoder_ctx;

    uint16_t width;
    uint16_t height;
    uint16_t row_capacity;
    uint16_t alpha_capacity;
    uint16_t cached_row;

    uint8_t has_alpha;
    uint8_t status;
    uint8_t row_cache_valid;

    uint16_t *row_pixels;
    uint8_t *row_alpha;
};

void egui_image_file_init(egui_image_file_t *self);
void egui_image_file_deinit(egui_image_file_t *self);

void egui_image_file_set_default_io(const egui_image_file_io_t *io);
int egui_image_file_register_decoder(const egui_image_file_decoder_t *decoder);
void egui_image_file_clear_decoders(void);

int egui_image_file_set_path(egui_image_file_t *self, const char *path);
void egui_image_file_set_io(egui_image_file_t *self, const egui_image_file_io_t *io);
void egui_image_file_set_placeholder(egui_image_file_t *self, const egui_image_t *placeholder);
int egui_image_file_reload(egui_image_file_t *self);

egui_image_file_status_t egui_image_file_get_status(const egui_image_file_t *self);
const char *egui_image_file_status_to_string(egui_image_file_status_t status);

#endif /* EGUI_CONFIG_FUNCTION_IMAGE_FILE */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_FILE_H_ */
