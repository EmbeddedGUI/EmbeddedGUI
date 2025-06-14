#ifndef _EGUI_FONT_STD_H_
#define _EGUI_FONT_STD_H_

#include "egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// avoid size mismatch warning in some compilers
#define EGUI_FONT_STD_EXT_CHAR_DESC_ITEM_SIZE 12

#define EGUI_FONT_STD_GET_FONT_HEIGHT(_font) (((egui_font_std_info_t*)(_font)->res)->height)

typedef struct
{
    uint32_t code;    // only support 32bit utf8 code
} egui_font_std_code_descriptor_t;

typedef struct
{
    uint32_t idx;     // the buffer size is 32bit
    uint16_t size;    // the buffer size
    uint8_t box_w;    // we never use big font, so we don't need to support big font
    uint8_t box_h;    // we never use big font, so we don't need to support big font
    uint8_t adv;      // the distance from the origin to the next character
    int8_t off_x;     // the x offset of the character
    int8_t off_y;     // the y offset of the character
} egui_font_std_char_descriptor_t;

typedef struct
{
    uint8_t font_size;        // font size
    uint8_t font_bit_mode; // font bit size 1, 2, 4, 8
    uint8_t height;        // font height
    uint8_t res_type; // EGUI_RESOURCE_TYPE_INTERNAL, EGUI_RESOURCE_TYPE_EXTERNAL
    uint16_t count;        // total char count, only support 65535
    const egui_font_std_code_descriptor_t *code_array;
    const egui_font_std_char_descriptor_t *char_array;
    const uint8_t *pixel_buffer;
} egui_font_std_info_t;

struct egui_font_std
{
    egui_font_t base;
};

void egui_font_std_init(egui_font_t *self, const void *res);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_FONT_STD_H_ */
