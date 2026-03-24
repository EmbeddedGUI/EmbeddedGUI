#ifndef _EGUI_IMAGE_QOI_H_
#define _EGUI_IMAGE_QOI_H_

#include "egui_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE

typedef struct
{
    /* Common prefix – binary-compatible with egui_image_std_info_t */
    const uint8_t *data_buf;    /* QOI compressed data stream */
    const void *alpha_buf;      /* NULL for QOI (alpha embedded in stream) */
    uint8_t data_type;          /* EGUI_IMAGE_DATA_TYPE_RGB565 / RGB32 */
    uint8_t alpha_type;         /* EGUI_IMAGE_ALPHA_TYPE_8 or no alpha */
    uint8_t res_type;           /* EGUI_RESOURCE_TYPE_INTERNAL / EXTERNAL */
    uint8_t channels;           /* 3 = RGB, 4 = RGBA (in QOI stream) */
    uint16_t width;
    uint16_t height;
    /* QOI-specific fields */
    uint32_t data_size;         /* compressed data length in bytes */
    uint32_t decompressed_size; /* decompressed size for validation */
} egui_image_qoi_info_t;

typedef struct egui_image_qoi egui_image_qoi_t;
struct egui_image_qoi
{
    egui_image_t base;
};

void egui_image_qoi_init(egui_image_t *self, const void *res);

#endif /* EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_QOI_H_ */
