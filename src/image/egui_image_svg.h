#ifndef _EGUI_IMAGE_SVG_H_
#define _EGUI_IMAGE_SVG_H_

#include "egui_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_image_svg egui_image_svg_t;

typedef struct
{
    const void *data_buf;
    uint32_t data_size;
    uint8_t res_type;
} egui_svg_source_t;

struct egui_image_svg
{
    egui_image_t base;
    void *doc;
    uint8_t *owned_data_buf;
};

void egui_image_svg_init(egui_image_svg_t *self);
void egui_image_svg_deinit(egui_image_svg_t *self);
int egui_image_svg_load_memory(egui_image_svg_t *self, const char *svg_text);
int egui_image_svg_load_memory_len(egui_image_svg_t *self, const char *svg_text, uint32_t svg_len);
int egui_image_svg_load_resource(egui_image_svg_t *self, const egui_svg_source_t *res);
int egui_image_svg_is_valid(const egui_image_svg_t *self);
void egui_image_svg_get_width_height(const egui_image_t *self, egui_dim_t *width, egui_dim_t *height);
void egui_image_svg_release_frame_cache(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_SVG_H_ */
