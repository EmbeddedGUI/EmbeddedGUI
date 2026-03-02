#include "egui_rotation.h"

#if EGUI_CONFIG_SOFTWARE_ROTATION

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

    EGUI_UNUSED(dst_size);

    switch (rotation)
    {
    case EGUI_DISPLAY_ROTATION_0:
        // No transformation needed
        break;

    case EGUI_DISPLAY_ROTATION_90:
        // Logical (lx, ly, lw, lh) on rotated screen (phys_h x phys_w)
        // -> Physical coordinates on phys_w x phys_h screen
        *x = phys_w - ly - lh;
        *y = lx;
        *w = lh;
        *h = lw;
        rotate_90_cw(src, dst, lw, lh);
        break;

    case EGUI_DISPLAY_ROTATION_180:
        // Logical (lx, ly, lw, lh) on (phys_w x phys_h)
        // -> Physical on same dimensions but inverted
        *x = phys_w - lx - lw;
        *y = phys_h - ly - lh;
        // w, h unchanged
        // For 180, we can rotate in-place if dst == src, or copy+reverse
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
        // Logical (lx, ly, lw, lh) on rotated screen (phys_h x phys_w)
        // -> Physical coordinates on phys_w x phys_h screen
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

void egui_rotation_transform_touch(egui_display_rotation_t rotation, int16_t phys_w, int16_t phys_h, egui_dim_t *x, egui_dim_t *y)
{
    egui_dim_t tx = *x;
    egui_dim_t ty = *y;

    switch (rotation)
    {
    case EGUI_DISPLAY_ROTATION_0:
        break;
    case EGUI_DISPLAY_ROTATION_90:
        // Physical touch -> logical coordinates
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

#endif /* EGUI_CONFIG_SOFTWARE_ROTATION */
