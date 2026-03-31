#ifndef _EGUI_IMAGE_RLE_H_
#define _EGUI_IMAGE_RLE_H_

#include "egui_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE

typedef struct
{
    /* Common prefix – binary-compatible with egui_image_std_info_t */
    const uint8_t *data_buf;  /* RLE compressed pixel data stream */
    const uint8_t *alpha_buf; /* RLE compressed alpha stream (NULL if no alpha) */
    uint8_t data_type;        /* EGUI_IMAGE_DATA_TYPE_RGB565 / RGB32 / GRAY8 */
    uint8_t alpha_type;       /* EGUI_IMAGE_ALPHA_TYPE_1/2/4/8 */
    uint8_t res_type;         /* EGUI_RESOURCE_TYPE_INTERNAL / EXTERNAL */
    uint16_t width;
    uint16_t height;
    /* RLE-specific fields */
    uint32_t data_size;         /* compressed pixel data length in bytes */
    uint32_t alpha_size;        /* compressed alpha length in bytes */
    uint32_t decompressed_size; /* decompressed pixel data size for validation */
} egui_image_rle_info_t;

typedef struct egui_image_rle egui_image_rle_t;
struct egui_image_rle
{
    egui_image_t base;
};

void egui_image_rle_init(egui_image_t *self, const void *res);
void egui_image_rle_release_frame_cache(void);

#endif /* EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_RLE_H_ */
