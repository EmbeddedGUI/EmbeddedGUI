#ifndef _EGUI_IMAGE_SVG_H_
#define _EGUI_IMAGE_SVG_H_

#include "egui_image.h"

/* Runtime SVG feature options. Keep SVG-specific defaults close to the SVG module. */
#ifndef EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_IMAGE_ELEMENT
#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_IMAGE_ELEMENT 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_GRADIENT
#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_GRADIENT 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PATH_ARC
#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PATH_ARC 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PATH_LENGTH
#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PATH_LENGTH 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_APPROX_TRIG
#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_APPROX_TRIG 0
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_TRANSFORM_SKEW
#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_TRANSFORM_SKEW 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PERCENT_DIAGONAL
#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PERCENT_DIAGONAL 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_SIMPLE_COLOR
#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_SIMPLE_COLOR 0
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_STROKE_DASH
#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_STROKE_DASH 1
#endif

#ifndef EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_APPROX_STROKE_SCALE
#define EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_APPROX_STROKE_SCALE 0
#endif

#if (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_IMAGE_ELEMENT != 0) && (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_IMAGE_ELEMENT != 1)
#error "EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_IMAGE_ELEMENT must be 0 or 1."
#endif

#if (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_GRADIENT != 0) && (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_GRADIENT != 1)
#error "EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_GRADIENT must be 0 or 1."
#endif

#if (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PATH_ARC != 0) && (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PATH_ARC != 1)
#error "EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PATH_ARC must be 0 or 1."
#endif

#if (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PATH_LENGTH != 0) && (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PATH_LENGTH != 1)
#error "EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PATH_LENGTH must be 0 or 1."
#endif

#if (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_APPROX_TRIG != 0) && (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_APPROX_TRIG != 1)
#error "EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_APPROX_TRIG must be 0 or 1."
#endif

#if (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_TRANSFORM_SKEW != 0) && (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_TRANSFORM_SKEW != 1)
#error "EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_TRANSFORM_SKEW must be 0 or 1."
#endif

#if (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PERCENT_DIAGONAL != 0) && (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PERCENT_DIAGONAL != 1)
#error "EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_PERCENT_DIAGONAL must be 0 or 1."
#endif

#if (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_SIMPLE_COLOR != 0) && (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_SIMPLE_COLOR != 1)
#error "EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_SIMPLE_COLOR must be 0 or 1."
#endif

#if (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_STROKE_DASH != 0) && (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_STROKE_DASH != 1)
#error "EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_STROKE_DASH must be 0 or 1."
#endif

#if (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_APPROX_STROKE_SCALE != 0) && (EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_APPROX_STROKE_SCALE != 1)
#error "EGUI_CONFIG_FUNCTION_IMAGE_RUNTIME_SVG_APPROX_STROKE_SCALE must be 0 or 1."
#endif

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
/* svg_text must stay valid and unchanged until this image is reset, reloaded, or deinitialized. */
int egui_image_svg_load_memory_borrowed(egui_image_svg_t *self, const char *svg_text);
int egui_image_svg_load_resource(egui_image_svg_t *self, egui_core_t *core, const egui_svg_source_t *res);
int egui_image_svg_is_valid(const egui_image_svg_t *self);
void egui_image_svg_get_width_height(const egui_image_t *self, egui_dim_t *width, egui_dim_t *height);
void egui_image_svg_release_frame_cache(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_SVG_H_ */
