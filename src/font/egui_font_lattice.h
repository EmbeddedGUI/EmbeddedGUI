#ifndef _EGUI_FONT_LATTICE_H_
#define _EGUI_FONT_LATTICE_H_

#include "egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// FONT - LATTICE
typedef struct struct_lattice
{
    uint32_t utf8_code;
    uint8_t width;
    const uint8_t *pixel_buffer;
} LATTICE;

typedef struct struct_lattice_font_info
{
    uint8_t height;
    uint32_t count;
    const LATTICE *lattice_array;
} LATTICE_FONT_INFO;

typedef struct egui_font_lattice egui_font_lattice_t;
struct egui_font_lattice
{
    egui_font_t base;
};

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_FONT_LATTICE_H_ */
