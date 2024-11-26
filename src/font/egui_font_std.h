#ifndef _EGUI_FONT_STD_H_
#define _EGUI_FONT_STD_H_

#include "egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t idx;     // the buffer size is 32bit
    uint8_t box_w;    // we never use big font, so we don't need to support big font
    uint8_t box_h;    // we never use big font, so we don't need to support big font
    uint8_t adv;      // the distance from the origin to the next character
    int8_t off_x;     // the x offset of the character
    int8_t off_y;     // the y offset of the character
    uint8_t code_len; // the utf8 code size length
    uint32_t code;    // only support 32bit utf8 code
} egui_font_std_char_descriptor_t;

typedef struct
{
    uint8_t font_size;        // font size
    uint8_t font_bit_mode; // font bit size 1, 2, 4, 8
    uint8_t height;        // font height
    uint16_t count;        // total char count, only support 65535
    const egui_font_std_char_descriptor_t *char_array;
    const uint8_t *pixel_buffer;
} egui_font_std_info_t;

typedef struct egui_font_std egui_font_std_t;
struct egui_font_std
{
    egui_font_t base;
};

void egui_font_std_draw_string(const egui_font_t *self, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color,
                               egui_alpha_t alpha);
int egui_font_std_get_str_size(const egui_font_t *self, const void *string, egui_dim_t *width, egui_dim_t *height);
void egui_font_std_init(egui_font_t *self, const void *res);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_FONT_STD_H_ */
