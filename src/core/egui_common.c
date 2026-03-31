#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "egui_api.h"

#define EGUI_ALPHA_VALUE(_val, _bit_size) ((255 * (_val)) / ((1 << (_bit_size)) - 1))

#define EGUI_ALPHA_VALUE_2(_val) EGUI_ALPHA_VALUE(_val, 2)
// we don't use multiple alpha value, so we don't need to calculate it.
const uint8_t egui_alpha_change_table_2[4] = {
        EGUI_ALPHA_VALUE_2(0x00),
        EGUI_ALPHA_VALUE_2(0x01),
        EGUI_ALPHA_VALUE_2(0x02),
        EGUI_ALPHA_VALUE_2(0x03),
};

#define EGUI_ALPHA_VALUE_4(_val) EGUI_ALPHA_VALUE(_val, 4)
// we don't use multiple alpha value, so we don't need to calculate it.
const uint8_t egui_alpha_change_table_4[16] = {
        EGUI_ALPHA_VALUE_4(0x00), EGUI_ALPHA_VALUE_4(0x01), EGUI_ALPHA_VALUE_4(0x02), EGUI_ALPHA_VALUE_4(0x03),
        EGUI_ALPHA_VALUE_4(0x04), EGUI_ALPHA_VALUE_4(0x05), EGUI_ALPHA_VALUE_4(0x06), EGUI_ALPHA_VALUE_4(0x07),
        EGUI_ALPHA_VALUE_4(0x08), EGUI_ALPHA_VALUE_4(0x09), EGUI_ALPHA_VALUE_4(0x0a), EGUI_ALPHA_VALUE_4(0x0b),
        EGUI_ALPHA_VALUE_4(0x0c), EGUI_ALPHA_VALUE_4(0x0d), EGUI_ALPHA_VALUE_4(0x0e), EGUI_ALPHA_VALUE_4(0x0f),
};

#if 0
void egui_argb8888_mix_rgb565(egui_color_rgb565_t *p_back_color, egui_color_bgra8888_t *p_fore_color, egui_color_rgb565_t *p_out_color)
{
    p_out_color->u5R = (((uint32_t)(p_fore_color->u8A) * (p_fore_color->u8R >> 3)) + (uint32_t)(255 - p_fore_color->u8A) * p_back_color->u5R) / 255;
    p_out_color->u6G = (((uint32_t)(p_fore_color->u8A) * (p_fore_color->u8G >> 2)) + (uint32_t)(255 - p_fore_color->u8A) * p_back_color->u6G) / 255;
    p_out_color->u5B = (((uint32_t)(p_fore_color->u8A) * (p_fore_color->u8B >> 3)) + (uint32_t)(255 - p_fore_color->u8A) * p_back_color->u5B) / 255;
}

void egui_argb8888_mix_argb8888(egui_color_bgra8888_t *p_back_color, egui_color_bgra8888_t *p_fore_color, egui_color_bgra8888_t *p_out_color)
{
    p_out_color->u8R = (((uint32_t)(p_fore_color->u8A) * (p_fore_color->u8R)) + (uint32_t)(255 - p_fore_color->u8A) * p_back_color->u8R) / 255;
    p_out_color->u8G = (((uint32_t)(p_fore_color->u8A) * (p_fore_color->u8G)) + (uint32_t)(255 - p_fore_color->u8A) * p_back_color->u8G) / 255;
    p_out_color->u8B = (((uint32_t)(p_fore_color->u8A) * (p_fore_color->u8B)) + (uint32_t)(255 - p_fore_color->u8A) * p_back_color->u8B) / 255;
    p_out_color->u8A = 0xFF;
}
#endif

void egui_common_align_get_x_y(egui_dim_t parent_width, egui_dim_t parent_height, egui_dim_t child_width, egui_dim_t child_height, uint8_t align_type,
                               egui_dim_t *x, egui_dim_t *y)
{
    *x = *y = 0;
    switch (align_type & EGUI_ALIGN_HMASK)
    {
    case EGUI_ALIGN_HCENTER:
        if (parent_width > child_width)
        {
            *x = (parent_width - child_width) >> 1;
        }
        break;
    case EGUI_ALIGN_LEFT:
        *x = 0;
        break;
    case EGUI_ALIGN_RIGHT:
        if (parent_width > child_width)
        {
            *x = parent_width - child_width;
        }
        break;
    default:
        break;
    }
    switch (align_type & EGUI_ALIGN_VMASK)
    {
    case EGUI_ALIGN_VCENTER:
        if (parent_height > child_height)
        {
            *y = (parent_height - child_height) >> 1;
        }
        break;
    case EGUI_ALIGN_TOP:
        *y = 0;
        break;
    case EGUI_ALIGN_BOTTOM:
        if (parent_height > child_height)
        {
            *y = (parent_height - child_height);
        }
        break;
    default:
        break;
    }
}

void egui_memcpy(void *dest, const void *src, uint32_t n)
{
    egui_api_memcpy(dest, src, (int)n);
}

void *egui_malloc(int size)
{
    return egui_api_malloc(size);
}

void egui_free(void *ptr)
{
    egui_api_free(ptr);
}
