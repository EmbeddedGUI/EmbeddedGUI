#include <string.h>
#include <stdio.h>

#include "egui_image_qoi.h"
#include "egui_image_decode_utils.h"
#include "core/egui_api.h"

#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE

/*
 * QOI (Quite OK Image) format opcodes
 * Reference: https://qoiformat.org/qoi-specification.pdf
 *
 * Note: Our QOI stream is "headerless" — the Python encoder strips the 14-byte
 * QOI header and 8-byte end marker. The stream contains only raw QOI opcodes.
 * Image metadata (width, height, channels) is stored in egui_image_qoi_info_t.
 */
#define QOI_OP_INDEX 0x00 /* 00xxxxxx */
#define QOI_OP_DIFF  0x40 /* 01xxxxxx */
#define QOI_OP_LUMA  0x80 /* 10xxxxxx */
#define QOI_OP_RUN   0xC0 /* 11xxxxxx */
#define QOI_OP_RGB   0xFE /* 11111110 */
#define QOI_OP_RGBA  0xFF /* 11111111 */

#define QOI_MASK_2 0xC0 /* top 2 bits mask */

#define QOI_COLOR_HASH(r, g, b, a) (((r) * 3 + (g) * 5 + (b) * 7 + (a) * 11) & 63)
#define QOI_RGBA_PACK(r, g, b, a) ((uint32_t)(r) | ((uint32_t)(g) << 8) | ((uint32_t)(b) << 16) | ((uint32_t)(a) << 24))

/* Persistent decode state for PFB tile rendering */
typedef struct
{
    const egui_image_qoi_info_t *info;
    uint32_t data_pos;    /* current read position in QOI stream */
    uint16_t current_row; /* next row to decode */

    /* QOI decoder state */
    uint8_t prev_r, prev_g, prev_b, prev_a;
    uint16_t prev_rgb565;
    uint16_t index_rgb565[64];
    uint32_t index_rgba[64];
    uint8_t run; /* remaining run-length count */
} egui_image_qoi_decode_state_t;

static egui_image_qoi_decode_state_t qoi_state;

#define EGUI_IMAGE_QOI_CHECKPOINT_COUNT 4

typedef struct
{
    egui_image_qoi_decode_state_t state;
    uint16_t row;
    const egui_image_qoi_info_t *info;
} egui_image_qoi_checkpoint_t;

/*
 * Row-band checkpoints: save decoder state at the start of recent PFB row bands
 * so horizontal tile neighbors and repeated small images at different Y offsets
 * can restore instead of re-decoding from the beginning.
 */
static egui_image_qoi_checkpoint_t qoi_checkpoints[EGUI_IMAGE_QOI_CHECKPOINT_COUNT];
static uint8_t qoi_checkpoint_next = 0;

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
typedef struct
{
    const egui_image_qoi_info_t *info;
    const void *data_addr;
    uint32_t data_size;
    uint8_t *data_buf;
} egui_image_qoi_external_cache_t;

static egui_image_qoi_external_cache_t qoi_external_cache;

static void egui_image_qoi_release_external_cache(void)
{
    if (qoi_external_cache.data_buf != NULL)
    {
        egui_free(qoi_external_cache.data_buf);
    }
    memset(&qoi_external_cache, 0, sizeof(qoi_external_cache));
}
#endif

static int egui_image_qoi_prepare_decode_info(const egui_image_qoi_info_t *info, egui_image_qoi_info_t *decode_info)
{
    *decode_info = *info;

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        int cache_hit = qoi_external_cache.info == info &&
                        qoi_external_cache.data_addr == info->data_buf &&
                        qoi_external_cache.data_size == info->data_size &&
                        qoi_external_cache.data_buf != NULL;

        if (!cache_hit)
        {
            uint8_t *new_data_buf = (uint8_t *)egui_malloc((int)info->data_size);
            if (new_data_buf == NULL)
            {
                return 0;
            }

            egui_api_load_external_resource(new_data_buf, (egui_uintptr_t)(info->data_buf), 0, info->data_size);
            egui_image_qoi_release_external_cache();

            qoi_external_cache.info = info;
            qoi_external_cache.data_addr = info->data_buf;
            qoi_external_cache.data_size = info->data_size;
            qoi_external_cache.data_buf = new_data_buf;
        }

        decode_info->data_buf = qoi_external_cache.data_buf;
        decode_info->res_type = EGUI_RESOURCE_TYPE_INTERNAL;
    }
#else
    if (info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        EGUI_ASSERT(0);
        return 0;
    }
#endif

    return 1;
}

static void egui_image_qoi_reset_state(const egui_image_qoi_info_t *info)
{
    memset(&qoi_state, 0, sizeof(qoi_state));
    qoi_state.info = info;
    qoi_state.prev_r = 0;
    qoi_state.prev_g = 0;
    qoi_state.prev_b = 0;
    qoi_state.prev_a = 255;
    qoi_state.prev_rgb565 = 0;
}

static void egui_image_qoi_save_checkpoint(const egui_image_qoi_info_t *info, uint16_t row)
{
    uint8_t i;

    for (i = 0; i < EGUI_IMAGE_QOI_CHECKPOINT_COUNT; i++)
    {
        if (qoi_checkpoints[i].info == info && qoi_checkpoints[i].row == row)
        {
            qoi_checkpoints[i].state = qoi_state;
            return;
        }
    }

    qoi_checkpoints[qoi_checkpoint_next].state = qoi_state;
    qoi_checkpoints[qoi_checkpoint_next].row = row;
    qoi_checkpoints[qoi_checkpoint_next].info = info;
    qoi_checkpoint_next++;
    qoi_checkpoint_next &= (EGUI_IMAGE_QOI_CHECKPOINT_COUNT - 1);
}

static int egui_image_qoi_restore_checkpoint(const egui_image_qoi_info_t *info, uint16_t target_row)
{
    uint8_t i;

    for (i = 0; i < EGUI_IMAGE_QOI_CHECKPOINT_COUNT; i++)
    {
        if (qoi_checkpoints[i].info == info && qoi_checkpoints[i].row == target_row)
        {
            qoi_state = qoi_checkpoints[i].state;
            return 1;
        }
    }

    return 0;
}

__EGUI_STATIC_INLINE__ uint16_t egui_image_qoi_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint16_t)(r >> 3) << 11) | ((uint16_t)(g >> 2) << 5) | (uint16_t)(b >> 3);
}

__EGUI_STATIC_INLINE__ void egui_image_qoi_fill_rgb565(uint16_t *dst, uint16_t pixel, uint16_t count)
{
    if (count == 0)
    {
        return;
    }

    if (count <= 4)
    {
        switch (count)
        {
        case 4:
            dst[3] = pixel;
        case 3:
            dst[2] = pixel;
        case 2:
            dst[1] = pixel;
        case 1:
            dst[0] = pixel;
        default:
            break;
        }
        return;
    }

    uint32_t packed = ((uint32_t)pixel << 16) | (uint32_t)pixel;

    if ((((uintptr_t)dst) & 0x02U) != 0U)
    {
        *dst++ = pixel;
        count--;
    }

    while (count >= 4)
    {
        ((uint32_t *)dst)[0] = packed;
        ((uint32_t *)dst)[1] = packed;
        dst += 4;
        count -= 4;
    }

    if (count >= 2)
    {
        *(uint32_t *)dst = packed;
        dst += 2;
        count -= 2;
    }

    if (count != 0)
    {
        *dst = pixel;
    }
}

__EGUI_STATIC_INLINE__ void egui_image_qoi_fill_rgb32(uint8_t *dst, uint8_t r, uint8_t g, uint8_t b, uint8_t a, uint16_t count)
{
    for (uint16_t i = 0; i < count; i++)
    {
        dst[0] = r;
        dst[1] = g;
        dst[2] = b;
        dst[3] = a;
        dst += 4;
    }
}

__EGUI_STATIC_INLINE__ void egui_image_qoi_decode_opcode(const uint8_t **data_ptr, const uint8_t *src_end,
                                                         uint8_t *prev_r, uint8_t *prev_g, uint8_t *prev_b, uint8_t *prev_a,
                                                         uint16_t *prev_rgb565, uint16_t *index_rgb565, uint32_t *index_rgba, uint8_t *run)
{
    const uint8_t *data = *data_ptr;
    uint8_t hash;
    uint8_t b1;

    if (data >= src_end)
    {
        return;
    }

    b1 = *data++;
    if (b1 < QOI_OP_RUN)
    {
        if (b1 < QOI_OP_DIFF)
        {
            uint8_t index = b1 & 0x3F;
            uint32_t rgba = index_rgba[index];
            *prev_r = (uint8_t)rgba;
            *prev_g = (uint8_t)(rgba >> 8);
            *prev_b = (uint8_t)(rgba >> 16);
            *prev_a = (uint8_t)(rgba >> 24);
            *prev_rgb565 = index_rgb565[index];
            *data_ptr = data;
            return;
        }

        if (b1 < QOI_OP_LUMA)
        {
            *prev_r += ((b1 >> 4) & 0x03) - 2;
            *prev_g += ((b1 >> 2) & 0x03) - 2;
            *prev_b += (b1 & 0x03) - 2;
        }
        else
        {
            uint8_t b2 = *data++;
            int8_t vg = (b1 & 0x3F) - 32;
            *prev_r += vg - 8 + ((b2 >> 4) & 0x0F);
            *prev_g += vg;
            *prev_b += vg - 8 + (b2 & 0x0F);
        }
    }
    else if (b1 < QOI_OP_RGB)
    {
        *run = (b1 & 0x3F); /* 0..61, bias -1 already in format: run = tag + 1 */
        /* QOI spec: run-length bias is -1, value 0 means run of 1.
         * We already consumed the first pixel (current), so remaining = value */
        *data_ptr = data;
        return;
    }
    else if (b1 == QOI_OP_RGB)
    {
        *prev_r = *data++;
        *prev_g = *data++;
        *prev_b = *data++;
    }
    else
    {
        *prev_r = *data++;
        *prev_g = *data++;
        *prev_b = *data++;
        *prev_a = *data++;
    }

    *prev_rgb565 = egui_image_qoi_rgb888_to_rgb565(*prev_r, *prev_g, *prev_b);
    hash = QOI_COLOR_HASH(*prev_r, *prev_g, *prev_b, *prev_a);
    index_rgb565[hash] = *prev_rgb565;
    index_rgba[hash] = QOI_RGBA_PACK(*prev_r, *prev_g, *prev_b, *prev_a);
    *data_ptr = data;
}

__EGUI_STATIC_INLINE__ void egui_image_qoi_decode_opcode_rgb(const uint8_t **data_ptr, const uint8_t *src_end,
                                                             uint8_t *prev_r, uint8_t *prev_g, uint8_t *prev_b,
                                                             uint16_t *prev_rgb565, uint16_t *index_rgb565, uint32_t *index_rgba, uint8_t *run)
{
    const uint8_t *data = *data_ptr;
    uint8_t hash;
    uint8_t b1;

    if (data >= src_end)
    {
        return;
    }

    b1 = *data++;
    if (b1 < QOI_OP_RUN)
    {
        if (b1 < QOI_OP_DIFF)
        {
            uint8_t index = b1 & 0x3F;
            uint32_t rgb = index_rgba[index];
            *prev_r = (uint8_t)rgb;
            *prev_g = (uint8_t)(rgb >> 8);
            *prev_b = (uint8_t)(rgb >> 16);
            *prev_rgb565 = index_rgb565[index];
            *data_ptr = data;
            return;
        }
        else if (b1 < QOI_OP_LUMA)
        {
            *prev_r += ((b1 >> 4) & 0x03) - 2;
            *prev_g += ((b1 >> 2) & 0x03) - 2;
            *prev_b += (b1 & 0x03) - 2;
        }
        else
        {
            uint8_t b2 = *data++;
            int8_t vg = (b1 & 0x3F) - 32;
            *prev_r += vg - 8 + ((b2 >> 4) & 0x0F);
            *prev_g += vg;
            *prev_b += vg - 8 + (b2 & 0x0F);
        }
    }
    else if (b1 == QOI_OP_RGB)
    {
        *prev_r = *data++;
        *prev_g = *data++;
        *prev_b = *data++;
    }
    else if (b1 < QOI_OP_RGB)
    {
        *run = (b1 & 0x3F);
        *data_ptr = data;
        return;
    }
    else
    {
        return;
    }

    *prev_rgb565 = egui_image_qoi_rgb888_to_rgb565(*prev_r, *prev_g, *prev_b);
    hash = QOI_COLOR_HASH(*prev_r, *prev_g, *prev_b, 255);
    index_rgb565[hash] = *prev_rgb565;
    index_rgba[hash] = QOI_RGBA_PACK(*prev_r, *prev_g, *prev_b, 255);
    *data_ptr = data;
}

static void egui_image_qoi_decode_row_rgb565_opaque(const egui_image_qoi_info_t *info, uint8_t *pixel_buf)
{
    const uint8_t *src = info->data_buf;
    const uint8_t *src_end = src + info->data_size;
    const uint8_t *data = src + qoi_state.data_pos;
    uint8_t prev_r = qoi_state.prev_r;
    uint8_t prev_g = qoi_state.prev_g;
    uint8_t prev_b = qoi_state.prev_b;
    uint16_t prev_pixel = qoi_state.prev_rgb565;
    uint8_t prev_hash = QOI_COLOR_HASH(prev_r, prev_g, prev_b, 255);
    uint8_t run = qoi_state.run;
    uint16_t *index_rgb565 = qoi_state.index_rgb565;
    uint32_t *index_rgba = qoi_state.index_rgba;
    uint16_t *dst = (uint16_t *)pixel_buf;
    uint16_t remaining = info->width;

    while (remaining > 0)
    {
        if (run > 0)
        {
            uint16_t repeat = remaining;
            if (repeat > run)
            {
                repeat = run;
            }
            egui_image_qoi_fill_rgb565(dst, prev_pixel, repeat);
            run = (uint8_t)(run - repeat);
            dst += repeat;
            remaining = (uint16_t)(remaining - repeat);
            continue;
        }

        if (data >= src_end)
        {
            break;
        }

        {
            uint8_t b1 = *data++;

            if (b1 < QOI_OP_RUN)
            {
                if (b1 < QOI_OP_DIFF)
                {
                    uint8_t index = b1 & 0x3F;
                    uint32_t rgb = index_rgba[index];

                    prev_r = (uint8_t)rgb;
                    prev_g = (uint8_t)(rgb >> 8);
                    prev_b = (uint8_t)(rgb >> 16);
                    prev_pixel = index_rgb565[index];
                    prev_hash = index;
                    *dst++ = prev_pixel;
                    remaining--;
                    continue;
                }
                else if (b1 < QOI_OP_LUMA)
                {
                    int8_t dr = (int8_t)(((b1 >> 4) & 0x03) - 2);
                    int8_t dg = (int8_t)(((b1 >> 2) & 0x03) - 2);
                    int8_t db = (int8_t)((b1 & 0x03) - 2);

                    prev_r = (uint8_t)(prev_r + dr);
                    prev_g = (uint8_t)(prev_g + dg);
                    prev_b = (uint8_t)(prev_b + db);
                    prev_hash = (uint8_t)((prev_hash + dr * 3 + dg * 5 + db * 7) & 63);
                }
                else
                {
                    uint8_t b2 = *data++;
                    int8_t vg = (int8_t)((b1 & 0x3F) - 32);
                    int8_t dr = (int8_t)(vg - 8 + ((b2 >> 4) & 0x0F));
                    int8_t db = (int8_t)(vg - 8 + (b2 & 0x0F));

                    prev_r = (uint8_t)(prev_r + dr);
                    prev_g = (uint8_t)(prev_g + vg);
                    prev_b = (uint8_t)(prev_b + db);
                    prev_hash = (uint8_t)((prev_hash + dr * 3 + vg * 5 + db * 7) & 63);
                }
            }
            else if (b1 == QOI_OP_RGB)
            {
                prev_r = *data++;
                prev_g = *data++;
                prev_b = *data++;
                prev_hash = QOI_COLOR_HASH(prev_r, prev_g, prev_b, 255);
            }
            else if (b1 < QOI_OP_RGB)
            {
                run = (b1 & 0x3F);
                *dst++ = prev_pixel;
                remaining--;
                continue;
            }
            else
            {
                break;
            }
        }

        prev_pixel = egui_image_qoi_rgb888_to_rgb565(prev_r, prev_g, prev_b);
        index_rgb565[prev_hash] = prev_pixel;
        index_rgba[prev_hash] = QOI_RGBA_PACK(prev_r, prev_g, prev_b, 255);
        *dst++ = prev_pixel;
        remaining--;
    }

    qoi_state.data_pos = (uint32_t)(data - src);
    qoi_state.prev_r = prev_r;
    qoi_state.prev_g = prev_g;
    qoi_state.prev_b = prev_b;
    qoi_state.prev_rgb565 = prev_pixel;
    qoi_state.run = run;
}

static void egui_image_qoi_decode_row_rgb565(const egui_image_qoi_info_t *info, uint8_t *pixel_buf, uint8_t *alpha_buf)
{
    const uint8_t *src = info->data_buf;
    const uint8_t *src_end = src + info->data_size;
    const uint8_t *data = src + qoi_state.data_pos;
    uint8_t prev_r = qoi_state.prev_r;
    uint8_t prev_g = qoi_state.prev_g;
    uint8_t prev_b = qoi_state.prev_b;
    uint8_t prev_a = qoi_state.prev_a;
    uint16_t prev_pixel = qoi_state.prev_rgb565;
    uint8_t run = qoi_state.run;
    uint16_t *index_rgb565 = qoi_state.index_rgb565;
    uint32_t *index_rgba = qoi_state.index_rgba;
    uint16_t *dst = (uint16_t *)pixel_buf;
    uint8_t *alpha = alpha_buf;
    uint16_t remaining = info->width;

    if (alpha_buf == NULL && info->channels == 3)
    {
        egui_image_qoi_decode_row_rgb565_opaque(info, pixel_buf);
        return;
    }

    if (alpha_buf == NULL)
    {
        while (remaining > 0)
        {
            if (run > 0)
            {
                uint16_t repeat = remaining;
                if (repeat > run)
                {
                    repeat = run;
                }
                egui_image_qoi_fill_rgb565(dst, prev_pixel, repeat);
                run = (uint8_t)(run - repeat);
                dst += repeat;
                remaining = (uint16_t)(remaining - repeat);
                continue;
            }

            egui_image_qoi_decode_opcode(&data, src_end, &prev_r, &prev_g, &prev_b, &prev_a, &prev_pixel, index_rgb565, index_rgba, &run);
            *dst++ = prev_pixel;
            remaining--;
        }
    }
    else
    {
        while (remaining > 0)
        {
            if (run > 0)
            {
                uint16_t repeat = remaining;
                if (repeat > run)
                {
                    repeat = run;
                }
                egui_image_qoi_fill_rgb565(dst, prev_pixel, repeat);
                memset(alpha, prev_a, repeat);
                run = (uint8_t)(run - repeat);
                dst += repeat;
                alpha += repeat;
                remaining = (uint16_t)(remaining - repeat);
                continue;
            }

            egui_image_qoi_decode_opcode(&data, src_end, &prev_r, &prev_g, &prev_b, &prev_a, &prev_pixel, index_rgb565, index_rgba, &run);
            *dst++ = prev_pixel;
            *alpha++ = prev_a;
            remaining--;
        }
    }

    qoi_state.data_pos = (uint32_t)(data - src);
    qoi_state.prev_r = prev_r;
    qoi_state.prev_g = prev_g;
    qoi_state.prev_b = prev_b;
    qoi_state.prev_a = prev_a;
    qoi_state.prev_rgb565 = prev_pixel;
    qoi_state.run = run;
}

static void egui_image_qoi_decode_row_rgb32(const egui_image_qoi_info_t *info, uint8_t *pixel_buf)
{
    const uint8_t *src = info->data_buf;
    const uint8_t *src_end = src + info->data_size;
    const uint8_t *data = src + qoi_state.data_pos;
    uint8_t prev_r = qoi_state.prev_r;
    uint8_t prev_g = qoi_state.prev_g;
    uint8_t prev_b = qoi_state.prev_b;
    uint8_t prev_a = qoi_state.prev_a;
    uint16_t prev_rgb565 = qoi_state.prev_rgb565;
    uint8_t run = qoi_state.run;
    uint16_t *index_rgb565 = qoi_state.index_rgb565;
    uint32_t *index_rgba = qoi_state.index_rgba;
    uint16_t remaining = info->width;

    while (remaining > 0)
    {
        if (run > 0)
        {
            uint16_t repeat = remaining;
            if (repeat > run)
            {
                repeat = run;
            }
            egui_image_qoi_fill_rgb32(pixel_buf, prev_r, prev_g, prev_b, prev_a, repeat);
            run = (uint8_t)(run - repeat);
            pixel_buf += (uint32_t)repeat << 2;
            remaining = (uint16_t)(remaining - repeat);
            continue;
        }

        egui_image_qoi_decode_opcode(&data, src_end, &prev_r, &prev_g, &prev_b, &prev_a, &prev_rgb565, index_rgb565, index_rgba, &run);
        pixel_buf[0] = prev_r;
        pixel_buf[1] = prev_g;
        pixel_buf[2] = prev_b;
        pixel_buf[3] = prev_a;
        pixel_buf += 4;
        remaining--;
    }

    qoi_state.data_pos = (uint32_t)(data - src);
    qoi_state.prev_r = prev_r;
    qoi_state.prev_g = prev_g;
    qoi_state.prev_b = prev_b;
    qoi_state.prev_a = prev_a;
    qoi_state.prev_rgb565 = prev_rgb565;
    qoi_state.run = run;
}

static void egui_image_qoi_decode_row(const egui_image_qoi_info_t *info, uint8_t *pixel_buf, uint8_t *alpha_buf)
{
    if (info->data_type == EGUI_IMAGE_DATA_TYPE_RGB565)
    {
        egui_image_qoi_decode_row_rgb565(info, pixel_buf, alpha_buf);
        return;
    }

    if (info->data_type == EGUI_IMAGE_DATA_TYPE_RGB32)
    {
        egui_image_qoi_decode_row_rgb32(info, pixel_buf);
    }
}

/**
 * Skip one row in the QOI stream without format conversion or buffer write.
 * Only advances the QOI state machine (hash table, prev color, run count).
 */
static void egui_image_qoi_skip_row_rgb(const egui_image_qoi_info_t *info)
{
    const uint8_t *src = info->data_buf;
    const uint8_t *src_end = src + info->data_size;
    const uint8_t *data = src + qoi_state.data_pos;
    uint8_t prev_r = qoi_state.prev_r;
    uint8_t prev_g = qoi_state.prev_g;
    uint8_t prev_b = qoi_state.prev_b;
    uint16_t prev_rgb565 = qoi_state.prev_rgb565;
    uint8_t run = qoi_state.run;
    uint16_t *index_rgb565 = qoi_state.index_rgb565;
    uint32_t *index_rgba = qoi_state.index_rgba;
    uint16_t remaining = info->width;

    while (remaining > 0)
    {
        if (run > 0)
        {
            uint16_t repeat = remaining;
            if (repeat > run)
            {
                repeat = run;
            }
            run = (uint8_t)(run - repeat);
            remaining = (uint16_t)(remaining - repeat);
            continue;
        }

        egui_image_qoi_decode_opcode_rgb(&data, src_end, &prev_r, &prev_g, &prev_b, &prev_rgb565, index_rgb565, index_rgba, &run);
        remaining--;
    }

    qoi_state.data_pos = (uint32_t)(data - src);
    qoi_state.prev_r = prev_r;
    qoi_state.prev_g = prev_g;
    qoi_state.prev_b = prev_b;
    qoi_state.prev_rgb565 = prev_rgb565;
    qoi_state.run = run;
}

static void egui_image_qoi_skip_row(const egui_image_qoi_info_t *info)
{
    if (info->channels == 3)
    {
        egui_image_qoi_skip_row_rgb(info);
        return;
    }

    const uint8_t *src = info->data_buf;
    const uint8_t *src_end = src + info->data_size;
    const uint8_t *data = src + qoi_state.data_pos;
    uint8_t prev_r = qoi_state.prev_r;
    uint8_t prev_g = qoi_state.prev_g;
    uint8_t prev_b = qoi_state.prev_b;
    uint8_t prev_a = qoi_state.prev_a;
    uint16_t prev_rgb565 = qoi_state.prev_rgb565;
    uint8_t run = qoi_state.run;
    uint16_t *index_rgb565 = qoi_state.index_rgb565;
    uint32_t *index_rgba = qoi_state.index_rgba;
    uint16_t remaining = info->width;

    while (remaining > 0)
    {
        if (run > 0)
        {
            uint16_t repeat = remaining;
            if (repeat > run)
            {
                repeat = run;
            }
            run = (uint8_t)(run - repeat);
            remaining = (uint16_t)(remaining - repeat);
            continue;
        }

        egui_image_qoi_decode_opcode(&data, src_end, &prev_r, &prev_g, &prev_b, &prev_a, &prev_rgb565, index_rgb565, index_rgba, &run);
        remaining--;
    }

    qoi_state.data_pos = (uint32_t)(data - src);
    qoi_state.prev_r = prev_r;
    qoi_state.prev_g = prev_g;
    qoi_state.prev_b = prev_b;
    qoi_state.prev_a = prev_a;
    qoi_state.prev_rgb565 = prev_rgb565;
    qoi_state.run = run;
}

static int egui_image_qoi_get_point(const egui_image_t *self, egui_dim_t x, egui_dim_t y,
                                    egui_color_t *color, egui_alpha_t *alpha)
{
    /* QOI is sequential-only; random access not efficiently supported */
    color->full = 0;
    *alpha = 0;
    return 0;
}

static int egui_image_qoi_get_point_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y,
                                           egui_dim_t width, egui_dim_t height,
                                           egui_color_t *color, egui_alpha_t *alpha)
{
    /* Resize not supported for compressed images */
    color->full = 0;
    *alpha = 0;
    return 0;
}

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
static void egui_image_qoi_blend_cached_rows(const egui_image_qoi_info_t *info, egui_dim_t y,
                                             egui_dim_t screen_x_start, egui_dim_t img_col_start, egui_dim_t count,
                                             egui_dim_t img_y_start, egui_dim_t img_y_end,
                                             egui_dim_t cache_row_start)
{
    int has_alpha = (info->channels == 4);
    uint8_t pixel_size = (info->data_type == EGUI_IMAGE_DATA_TYPE_RGB565) ? 2 : 4;
    uint8_t alpha_type = has_alpha ? EGUI_IMAGE_ALPHA_TYPE_8 : EGUI_IMAGE_ALPHA_TYPE_1;
    egui_dim_t screen_y = y + img_y_start;

    for (egui_dim_t row = img_y_start; row < img_y_end; row++)
    {
        uint16_t row_in_cache = (uint16_t)(row - cache_row_start);
        const uint8_t *pixel_buf = egui_image_decode_cache_pixel_row(row_in_cache, info->width, pixel_size);
        const uint8_t *alpha_buf = has_alpha ? egui_image_decode_cache_alpha_row(row_in_cache, info->width) : NULL;

        egui_image_decode_blend_row_clipped(screen_x_start, screen_y, img_col_start, count,
                                            info->data_type, alpha_type, has_alpha, pixel_buf, alpha_buf);
        screen_y++;
    }
}
#endif

#if EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES > 0
static void egui_image_qoi_blend_persistent_cached_rows(const egui_image_qoi_info_t *info, egui_dim_t y,
                                                        egui_dim_t screen_x_start, egui_dim_t img_col_start, egui_dim_t count,
                                                        egui_dim_t img_y_start, egui_dim_t img_y_end)
{
    int has_alpha = (info->channels == 4);
    uint8_t alpha_type = has_alpha ? EGUI_IMAGE_ALPHA_TYPE_8 : EGUI_IMAGE_ALPHA_TYPE_1;
    egui_dim_t screen_y = y + img_y_start;

    for (egui_dim_t row = img_y_start; row < img_y_end; row++)
    {
        const uint8_t *pixel_buf = egui_image_decode_persistent_cache_pixel_row((uint16_t)row);
        const uint8_t *alpha_buf = has_alpha ? egui_image_decode_persistent_cache_alpha_row_bytes((uint16_t)row) : NULL;

        egui_image_decode_blend_row_clipped(screen_x_start, screen_y, img_col_start, count,
                                            info->data_type, alpha_type, has_alpha, pixel_buf, alpha_buf);
        screen_y++;
    }
}
#endif

static void egui_image_qoi_draw_image(const egui_image_t *self, egui_dim_t x, egui_dim_t y)
{
    const egui_image_qoi_info_t *info = (const egui_image_qoi_info_t *)self->res;
    egui_image_qoi_info_t decode_info;
    const egui_image_qoi_info_t *draw_info;

    if (info == NULL || info->data_buf == NULL)
    {
        return;
    }

    if (!egui_image_qoi_prepare_decode_info(info, &decode_info))
    {
        return;
    }
    draw_info = &decode_info;

    /* Check if info changed or state needs reset */
    if (qoi_state.info != info || qoi_state.current_row > info->height)
    {
        egui_image_qoi_reset_state(info);
    }

    egui_region_t *work_region = egui_canvas_get_base_view_work_region();

    /* Calculate which rows of the image overlap with the current PFB tile */
    egui_dim_t img_y_start = work_region->location.y - y;
    egui_dim_t img_y_end = img_y_start + work_region->size.height;

    if (img_y_start < 0)
    {
        img_y_start = 0;
    }
    if (img_y_end > draw_info->height)
    {
        img_y_end = draw_info->height;
    }
    if (img_y_start >= img_y_end)
    {
        return;
    }

    egui_dim_t screen_x_start;
    egui_dim_t img_col_start;
    egui_dim_t count;
    int has_alpha = (draw_info->channels == 4);
    uint8_t pixel_size = (draw_info->data_type == EGUI_IMAGE_DATA_TYPE_RGB565) ? 2 : 4;
    uint16_t alpha_row_bytes = has_alpha ? draw_info->width : 0;
    uint8_t alpha_type = has_alpha ? EGUI_IMAGE_ALPHA_TYPE_8 : EGUI_IMAGE_ALPHA_TYPE_1;

    if (!egui_image_decode_get_horizontal_clip(x, draw_info->width, &screen_x_start, &img_col_start, &count))
    {
        return;
    }

#if EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES > 0
    if (egui_image_decode_persistent_cache_is_hit((const void *)info))
    {
        egui_image_qoi_blend_persistent_cached_rows(draw_info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end);
        return;
    }

    if (egui_image_decode_persistent_cache_prepare(draw_info->width, draw_info->height, pixel_size, alpha_row_bytes))
    {
        egui_image_qoi_reset_state(info);

        for (egui_dim_t row = 0; row < draw_info->height; row++)
        {
            uint8_t *pixel_buf = egui_image_decode_persistent_cache_pixel_row((uint16_t)row);
            uint8_t *alpha_buf = has_alpha ? egui_image_decode_persistent_cache_alpha_row_bytes((uint16_t)row) : NULL;

            egui_image_qoi_decode_row(draw_info, pixel_buf, alpha_buf);
            qoi_state.current_row++;
        }

        egui_image_decode_persistent_cache_set_image((const void *)info);
        egui_image_qoi_blend_persistent_cached_rows(draw_info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end);
        return;
    }
#endif

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
    if (egui_image_decode_cache_is_full_image_hit((const void *)info))
    {
        egui_image_qoi_blend_cached_rows(info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end, 0);
        return;
    }

    if (egui_image_decode_cache_can_hold_full_image(draw_info->width, draw_info->height, pixel_size, has_alpha ? draw_info->width : 0))
    {
        egui_image_qoi_reset_state(info);

        for (egui_dim_t row = 0; row < draw_info->height; row++)
        {
            uint8_t *pixel_buf = egui_image_decode_cache_pixel_row((uint16_t)row, draw_info->width, pixel_size);
            uint8_t *alpha_buf = has_alpha ? egui_image_decode_cache_alpha_row((uint16_t)row, draw_info->width) : NULL;

            egui_image_qoi_decode_row(draw_info, pixel_buf, alpha_buf);
            qoi_state.current_row++;
        }

        egui_image_decode_cache_set_full_image((const void *)info, draw_info->height);
        egui_image_qoi_blend_cached_rows(info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end, 0);
        return;
    }

    /* Row-band cache: check if this row band is already cached */
    if (egui_image_decode_cache_state.image_info == (const void *)info &&
        egui_image_decode_cache_state.row_band_start == (uint16_t)img_y_start)
    {
        /* Cache hit — blend directly from cached rows without decoding */
        egui_dim_t screen_y = y + img_y_start;

        for (egui_dim_t row = img_y_start; row < img_y_end; row++)
        {
            uint16_t row_in_band = (uint16_t)row - (uint16_t)img_y_start;
            const uint8_t *pixel_buf = egui_image_decode_cache_pixel_row(row_in_band, draw_info->width, pixel_size);
            const uint8_t *alpha_buf = has_alpha ?
                egui_image_decode_cache_alpha_row(row_in_band, draw_info->width) : NULL;

            egui_image_decode_blend_row_clipped(screen_x_start, screen_y, img_col_start, count,
                                                draw_info->data_type, alpha_type, has_alpha, pixel_buf, alpha_buf);
            screen_y++;
        }
        return;
    }
#endif /* EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE */

    /* If PFB requests rows before current state, try checkpoint restore.
     * QOI is sequential — cannot seek backwards without full re-decode. */
    if ((uint16_t)img_y_start < qoi_state.current_row)
    {
        if (!egui_image_qoi_restore_checkpoint(info, (uint16_t)img_y_start))
        {
            egui_image_qoi_reset_state(info);
        }
    }

    /* Skip rows to reach img_y_start (lightweight: no format conversion) */
    while (qoi_state.current_row < (uint16_t)img_y_start)
    {
        egui_image_qoi_skip_row(draw_info);
        qoi_state.current_row++;
    }

    /* Save checkpoint at the start of this row band so horizontal tile
     * neighbors can restore directly instead of re-decoding from row 0. */
    egui_image_qoi_save_checkpoint(info, (uint16_t)img_y_start);

    /* Decode and blend visible rows */
    egui_dim_t screen_y = y + img_y_start;
    for (egui_dim_t row = img_y_start; row < img_y_end; row++)
    {
#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
        /* Decode into cache buffer and blend from it */
        uint16_t row_in_band = (uint16_t)row - (uint16_t)img_y_start;
        uint8_t *pixel_buf = egui_image_decode_cache_pixel_row(row_in_band, draw_info->width, pixel_size);
        uint8_t *alpha_buf = has_alpha ?
            egui_image_decode_cache_alpha_row(row_in_band, draw_info->width) : NULL;
#else
        uint8_t *pixel_buf = egui_image_decode_row_pixel_buf;
        uint8_t *alpha_buf = has_alpha ? egui_image_decode_row_alpha_buf : NULL;
#endif
        egui_image_qoi_decode_row(draw_info, pixel_buf, alpha_buf);
        qoi_state.current_row++;

        /* Blend into PFB */
        egui_image_decode_blend_row_clipped(screen_x_start, screen_y, img_col_start, count,
                                            draw_info->data_type, alpha_type, has_alpha, pixel_buf, alpha_buf);
        screen_y++;
    }

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
    /* Mark this row band as cached for subsequent horizontal tiles */
    egui_image_decode_cache_set_row_band((const void *)info, (uint16_t)img_y_start, (uint16_t)(img_y_end - img_y_start));
#endif
}

static void egui_image_qoi_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y,
                                             egui_dim_t width, egui_dim_t height)
{
    /* Resize not supported for compressed images */
    (void)self;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

const egui_image_api_t egui_image_qoi_t_api_table = {
    .get_point = egui_image_qoi_get_point,
    .get_point_resize = egui_image_qoi_get_point_resize,
    .draw_image = egui_image_qoi_draw_image,
    .draw_image_resize = egui_image_qoi_draw_image_resize,
};

void egui_image_qoi_init(egui_image_t *self, const void *res)
{
    /* Call base init */
    egui_image_init(self, res);

    /* Update vtable to QOI implementation */
    self->api = &egui_image_qoi_t_api_table;
}

#endif /* EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE */
