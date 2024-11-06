#ifndef _EGUI_IMAGE_STD_H_
#define _EGUI_IMAGE_STD_H_

#include "egui_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_IMAGE_DATA_TYPE_RGB32  0
#define EGUI_IMAGE_DATA_TYPE_RGB565 1
#define EGUI_IMAGE_DATA_TYPE_GRAY8  2

#define EGUI_IMAGE_ALPHA_TYPE_1 0
#define EGUI_IMAGE_ALPHA_TYPE_2 1
#define EGUI_IMAGE_ALPHA_TYPE_4 2
#define EGUI_IMAGE_ALPHA_TYPE_8 3

typedef struct
{
    const void *data_buf;
    const void *alpha_buf;
    uint8_t data_type;  // image data type, EGUI_IMAGE_DATA_TYPE_RGB32, EGUI_IMAGE_DATA_TYPE_RGB565, EGUI_IMAGE_DATA_TYPE_GRAY8
    uint8_t alpha_type; // image bit size 1, 2, 4, 8
    uint8_t res_type; // EGUI_RESOURCE_TYPE_INTERNAL, EGUI_RESOURCE_TYPE_EXTERNAL
    uint16_t width;     // image width
    uint16_t height;    // image height
} egui_image_std_info_t;

typedef struct egui_image_std egui_image_std_t;
struct egui_image_std
{
    egui_image_t base;
};

void egui_image_std_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y);
void egui_image_std_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height);

void egui_image_std_get_width_height(egui_image_t *self, egui_dim_t *width, egui_dim_t *height);
void egui_image_std_init(egui_image_t *self, const void *res);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_STD_H_ */
