#include "egui_rotation.h"

/**
 * @file egui_rotation.c
 * @brief Software rotation helpers for rendered PFB tiles and touch coordinates.
 */

#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE

/**
 * Rotate PFB 180 degrees in-place (simple buffer reverse).
 */
static void rotate_180_inplace(egui_color_int_t *buf, int count)
{
    int i = 0;
    int j = count - 1;
    while (i < j)
    {
        egui_color_int_t tmp = buf[i];
        buf[i] = buf[j];
        buf[j] = tmp;
        i++;
        j--;
    }
}

/**
 * Rotate PFB 90 degrees clockwise.
 * src[row][col] -> dst[col][h-1-row]
 * Output dimensions: w_out = h_in, h_out = w_in
 */
static void rotate_90_cw(const egui_color_int_t *src, egui_color_int_t *dst, int16_t w, int16_t h)
{
    for (int16_t row = 0; row < h; row++)
    {
        for (int16_t col = 0; col < w; col++)
        {
            dst[col * h + (h - 1 - row)] = src[row * w + col];
        }
    }
}

/**
 * Rotate PFB 270 degrees clockwise (= 90 degrees counter-clockwise).
 * src[row][col] -> dst[(w-1-col)][row]
 * Output dimensions: w_out = h_in, h_out = w_in
 */
static void rotate_270_cw(const egui_color_int_t *src, egui_color_int_t *dst, int16_t w, int16_t h)
{
    for (int16_t row = 0; row < h; row++)
    {
        for (int16_t col = 0; col < w; col++)
        {
            dst[(w - 1 - col) * h + row] = src[row * w + col];
        }
    }
}

void egui_rotation_transform_pfb(egui_display_rotation_t rotation, int16_t phys_w, int16_t phys_h, int16_t *x, int16_t *y, int16_t *w, int16_t *h,
                                 const egui_color_int_t *src, egui_color_int_t *dst, int dst_size)
{
    int16_t lx = *x;
    int16_t ly = *y;
    int16_t lw = *w;
    int16_t lh = *h;

    // The caller already guarantees sufficient destination capacity for the selected rotation path.
    EGUI_UNUSED(dst_size);

    switch (rotation)
    {
    case EGUI_DISPLAY_ROTATION_0:
        // No coordinate or pixel transformation is needed.
        break;

    case EGUI_DISPLAY_ROTATION_90:
        // Map the logical tile rectangle back into native panel space, then rotate the pixels.
        *x = phys_w - ly - lh;
        *y = lx;
        *w = lh;
        *h = lw;
        rotate_90_cw(src, dst, lw, lh);
        break;

    case EGUI_DISPLAY_ROTATION_180:
        // 180-degree rotation keeps width/height unchanged while mirroring both axes.
        *x = phys_w - lx - lw;
        *y = phys_h - ly - lh;
        // For 180 degrees we can either reverse in place or copy into a separate buffer.
        if (dst != src)
        {
            int count = lw * lh;
            for (int i = 0; i < count; i++)
            {
                dst[count - 1 - i] = src[i];
            }
        }
        else
        {
            rotate_180_inplace(dst, lw * lh);
        }
        break;

    case EGUI_DISPLAY_ROTATION_270:
        // Map the logical tile rectangle back into native panel space, then rotate the pixels.
        *x = ly;
        *y = phys_h - lx - lw;
        *w = lh;
        *h = lw;
        rotate_270_cw(src, dst, lw, lh);
        break;

    default:
        break;
    }
}

/** Convert one raw touch coordinate from native panel space back into logical rotated space. */
void egui_rotation_transform_touch(egui_display_rotation_t rotation, int16_t phys_w, int16_t phys_h, egui_dim_t *x, egui_dim_t *y)
{
    egui_dim_t tx = *x;
    egui_dim_t ty = *y;

    switch (rotation)
    {
    case EGUI_DISPLAY_ROTATION_0:
        break;
    case EGUI_DISPLAY_ROTATION_90:
        // Physical touch -> logical rotated coordinates.
        *x = ty;
        *y = phys_w - 1 - tx;
        break;
    case EGUI_DISPLAY_ROTATION_180:
        *x = phys_w - 1 - tx;
        *y = phys_h - 1 - ty;
        break;
    case EGUI_DISPLAY_ROTATION_270:
        *x = phys_h - 1 - ty;
        *y = tx;
        break;
    default:
        break;
    }
}

#endif /* EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE */
