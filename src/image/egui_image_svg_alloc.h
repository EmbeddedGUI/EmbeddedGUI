#ifndef _EGUI_IMAGE_SVG_ALLOC_H_
#define _EGUI_IMAGE_SVG_ALLOC_H_

#include <stddef.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

void *egui_svg_alloc_malloc(size_t size);
void *egui_svg_alloc_calloc(size_t count, size_t size);
void *egui_svg_alloc_realloc(void *ptr, size_t size);
void egui_svg_alloc_free(void *ptr);
void *egui_svg_alloc_plain_malloc(size_t size);
void *egui_svg_alloc_plain_calloc(size_t count, size_t size);
void *egui_svg_alloc_plain_realloc(void *ptr, size_t old_size, size_t size);
void egui_svg_alloc_plain_free(void *ptr);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_IMAGE_SVG_ALLOC_H_ */
