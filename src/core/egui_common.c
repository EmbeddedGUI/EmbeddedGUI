#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "egui_api.h"

/**
 * @file egui_common.c
 * @brief Shared tiny helpers for alpha conversion, color blending, alignment, and allocator wrappers.
 */

#define EGUI_ALPHA_VALUE(_val, _bit_size) ((255 * (_val)) / ((1 << (_bit_size)) - 1))

#define EGUI_ALPHA_VALUE_2(_val) EGUI_ALPHA_VALUE(_val, 2)
// Pre-expanded lookup table that maps packed 2-bit alpha values onto the renderer's 0..255 scale.
const uint8_t egui_alpha_change_table_2[4] = {
        EGUI_ALPHA_VALUE_2(0x00),
        EGUI_ALPHA_VALUE_2(0x01),
        EGUI_ALPHA_VALUE_2(0x02),
        EGUI_ALPHA_VALUE_2(0x03),
};

#define EGUI_ALPHA_VALUE_4(_val) EGUI_ALPHA_VALUE(_val, 4)
// Pre-expanded lookup table that maps packed 4-bit alpha values onto the renderer's 0..255 scale.
const uint8_t egui_alpha_change_table_4[16] = {
        EGUI_ALPHA_VALUE_4(0x00), EGUI_ALPHA_VALUE_4(0x01), EGUI_ALPHA_VALUE_4(0x02), EGUI_ALPHA_VALUE_4(0x03),
        EGUI_ALPHA_VALUE_4(0x04), EGUI_ALPHA_VALUE_4(0x05), EGUI_ALPHA_VALUE_4(0x06), EGUI_ALPHA_VALUE_4(0x07),
        EGUI_ALPHA_VALUE_4(0x08), EGUI_ALPHA_VALUE_4(0x09), EGUI_ALPHA_VALUE_4(0x0a), EGUI_ALPHA_VALUE_4(0x0b),
        EGUI_ALPHA_VALUE_4(0x0c), EGUI_ALPHA_VALUE_4(0x0d), EGUI_ALPHA_VALUE_4(0x0e), EGUI_ALPHA_VALUE_4(0x0f),
};

/** Blend one BGRA8888 foreground pixel over one RGB565 background pixel. */
void egui_argb8888_mix_rgb565(egui_color_rgb565_t *p_back_color, egui_color_bgra8888_t *p_fore_color, egui_color_rgb565_t *p_out_color)
{
    uint32_t back_red;
    uint32_t back_green;
    uint32_t back_blue;
    uint32_t fore_red;
    uint32_t fore_green;
    uint32_t fore_blue;
    uint32_t fore_alpha;

    if (p_back_color == NULL || p_fore_color == NULL || p_out_color == NULL)
    {
        return;
    }

    // Convert the foreground to RGB565 channel widths, then do one alpha-weighted interpolation per channel.
    back_red = p_back_color->color.red;
    back_green = p_back_color->color.green;
    back_blue = p_back_color->color.blue;
    fore_red = (uint32_t)p_fore_color->color.red >> 3;
    fore_green = (uint32_t)p_fore_color->color.green >> 2;
    fore_blue = (uint32_t)p_fore_color->color.blue >> 3;
    fore_alpha = p_fore_color->color.alpha;

    p_out_color->color.red = (uint16_t)((fore_alpha * fore_red + (255U - fore_alpha) * back_red + 128U) >> 8);
    p_out_color->color.green = (uint16_t)((fore_alpha * fore_green + (255U - fore_alpha) * back_green + 128U) >> 8);
    p_out_color->color.blue = (uint16_t)((fore_alpha * fore_blue + (255U - fore_alpha) * back_blue + 128U) >> 8);
}

/** Blend one BGRA8888 foreground pixel over one BGRA8888 background pixel and force the result fully opaque. */
void egui_argb8888_mix_argb8888(egui_color_bgra8888_t *p_back_color, egui_color_bgra8888_t *p_fore_color, egui_color_bgra8888_t *p_out_color)
{
    uint32_t back_red;
    uint32_t back_green;
    uint32_t back_blue;
    uint32_t fore_red;
    uint32_t fore_green;
    uint32_t fore_blue;
    uint32_t fore_alpha;

    if (p_back_color == NULL || p_fore_color == NULL || p_out_color == NULL)
    {
        return;
    }

    back_red = p_back_color->color.red;
    back_green = p_back_color->color.green;
    back_blue = p_back_color->color.blue;
    fore_red = p_fore_color->color.red;
    fore_green = p_fore_color->color.green;
    fore_blue = p_fore_color->color.blue;
    fore_alpha = p_fore_color->color.alpha;

    p_out_color->color.red = (uint32_t)((fore_alpha * fore_red + (255U - fore_alpha) * back_red + 128U) >> 8);
    p_out_color->color.green = (uint32_t)((fore_alpha * fore_green + (255U - fore_alpha) * back_green + 128U) >> 8);
    p_out_color->color.blue = (uint32_t)((fore_alpha * fore_blue + (255U - fore_alpha) * back_blue + 128U) >> 8);
    p_out_color->color.alpha = 0xFF;
}

/** Resolve the aligned child offset inside a parent rectangle. */
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

/** Compatibility wrapper that forwards to the active platform-aware memcpy implementation. */
void egui_memcpy(void *dest, const void *src, uint32_t n)
{
    egui_api_memcpy(dest, src, (int)n);
}

/** Allocate memory through the currently active core-aware platform API. */
void *egui_malloc(egui_core_t *core, int size)
{
    return egui_api_malloc(core, size);
}

/** Free memory through the currently active core-aware platform API. */
void egui_free(egui_core_t *core, void *ptr)
{
    egui_api_free(core, ptr);
}
