#include <string.h>
#include <stdio.h>

#include "egui_image_qoi.h"
#include "egui_image_decode_utils.h"
#include "core/egui_api.h"
#include "mask/egui_mask_image.h"

/* Shipped QOI decode-state layout keeps the only variants still worth carrying.
 * Lower-yield micro toggles were removed instead of staying as private branches.
 */
enum
{
    EGUI_IMAGE_QOI_CHECKPOINT_COUNT = 2,
};

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
#define QOI_RGBA_PACK(r, g, b, a)  ((uint32_t)(r) | ((uint32_t)(g) << 8) | ((uint32_t)(b) << 16) | ((uint32_t)(a) << 24))

typedef uint16_t egui_image_qoi_row_index_t;
#define EGUI_IMAGE_QOI_ROW_INDEX_MAX UINT16_MAX

/* Keep cached RGB565 conversion alongside the RGBA index table.
 * Turning this off only saved 64B text / 128B heap per active instance.
 */
#define EGUI_IMAGE_QOI_INDEX_STATE_MEMBER                                                                                                                      \
    uint16_t index_rgb565[64];                                                                                                                                 \
    uint32_t index_rgba[64];
#define EGUI_IMAGE_QOI_INDEX_PARAM   , uint16_t *index_rgb565, uint32_t *index_rgba
#define EGUI_IMAGE_QOI_INDEX_ARG     , index_rgb565, index_rgba
#define EGUI_IMAGE_QOI_INDEX_DECLARE                                                                                                                           \
    uint16_t *index_rgb565 = qoi_state.index_rgb565;                                                                                                           \
    uint32_t *index_rgba = qoi_state.index_rgba;
#define EGUI_IMAGE_QOI_INDEX_RGB565_LOAD(_index, _prev_rgb565, _prev_r, _prev_g, _prev_b) (*(_prev_rgb565) = index_rgb565[_index])
#define EGUI_IMAGE_QOI_INDEX_RGB565_STORE(_hash, _prev_rgb565)                            (index_rgb565[_hash] = *(_prev_rgb565))
#define EGUI_IMAGE_QOI_INDEX_LOAD_RGB(_index, _prev_r, _prev_g, _prev_b, _prev_rgb565)                                                                         \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        uint32_t _rgb = index_rgba[_index];                                                                                                                    \
        *(_prev_r) = (uint8_t)_rgb;                                                                                                                            \
        *(_prev_g) = (uint8_t)(_rgb >> 8);                                                                                                                     \
        *(_prev_b) = (uint8_t)(_rgb >> 16);                                                                                                                    \
        EGUI_IMAGE_QOI_INDEX_RGB565_LOAD(_index, _prev_rgb565, _prev_r, _prev_g, _prev_b);                                                                     \
    } while (0)
#define EGUI_IMAGE_QOI_INDEX_LOAD_RGBA(_index, _prev_r, _prev_g, _prev_b, _prev_a, _prev_rgb565)                                                               \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        uint32_t _rgba = index_rgba[_index];                                                                                                                   \
        *(_prev_r) = (uint8_t)_rgba;                                                                                                                           \
        *(_prev_g) = (uint8_t)(_rgba >> 8);                                                                                                                    \
        *(_prev_b) = (uint8_t)(_rgba >> 16);                                                                                                                   \
        *(_prev_a) = (uint8_t)(_rgba >> 24);                                                                                                                   \
        EGUI_IMAGE_QOI_INDEX_RGB565_LOAD(_index, _prev_rgb565, _prev_r, _prev_g, _prev_b);                                                                     \
    } while (0)
#define EGUI_IMAGE_QOI_INDEX_STORE_RGB(_hash, _prev_r, _prev_g, _prev_b, _prev_rgb565)                                                                         \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        EGUI_IMAGE_QOI_INDEX_RGB565_STORE(_hash, _prev_rgb565);                                                                                                \
        index_rgba[_hash] = QOI_RGBA_PACK(*(_prev_r), *(_prev_g), *(_prev_b), 255);                                                                            \
    } while (0)
#define EGUI_IMAGE_QOI_INDEX_STORE_RGBA(_hash, _prev_r, _prev_g, _prev_b, _prev_a, _prev_rgb565)                                                               \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        EGUI_IMAGE_QOI_INDEX_RGB565_STORE(_hash, _prev_rgb565);                                                                                                \
        index_rgba[_hash] = QOI_RGBA_PACK(*(_prev_r), *(_prev_g), *(_prev_b), *(_prev_a));                                                                     \
    } while (0)

/* Persistent decode state for PFB tile rendering */
typedef struct
{
    const egui_image_qoi_info_t *info;
    uint32_t data_pos;                      /* current read position in QOI stream */
    egui_image_qoi_row_index_t current_row; /* next row to decode */

    /* QOI decoder state */
    uint8_t run; /* remaining run-length count */
    uint16_t prev_rgb565;
    uint8_t prev_r, prev_g, prev_b, prev_a;
    EGUI_IMAGE_QOI_INDEX_STATE_MEMBER
} egui_image_qoi_decode_state_t;

static egui_image_qoi_decode_state_t *qoi_state_ptr = NULL;
#define qoi_state (*qoi_state_ptr)

static void egui_image_qoi_blend_masked_rgb565_image_row(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_pixels,
                                                         const uint8_t *src_alpha_row, egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y,
                                                         egui_alpha_t canvas_alpha, const uint8_t **opaque_alpha_row);

static int egui_image_qoi_prepare_state(void)
{
    if (qoi_state_ptr != NULL)
    {
        return 1;
    }

    qoi_state_ptr = (egui_image_qoi_decode_state_t *)egui_malloc(sizeof(egui_image_qoi_decode_state_t));
    return qoi_state_ptr != NULL;
}

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
static egui_image_qoi_checkpoint_t *qoi_checkpoints = NULL;
static uint8_t qoi_checkpoint_next = 0;

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
#ifndef EGUI_IMAGE_QOI_EXTERNAL_STREAM_WINDOW_SIZE
#define EGUI_IMAGE_QOI_EXTERNAL_STREAM_WINDOW_SIZE 128
#endif

typedef struct
{
    const egui_image_qoi_info_t *info;
    uint32_t pos;
    uint32_t window_offset;
    uint16_t window_size;
    uint8_t window[EGUI_IMAGE_QOI_EXTERNAL_STREAM_WINDOW_SIZE];
} egui_image_qoi_external_stream_t;
#endif

static egui_image_qoi_checkpoint_t *egui_image_qoi_get_checkpoints(void)
{
    if (qoi_checkpoints == NULL)
    {
        qoi_checkpoints = (egui_image_qoi_checkpoint_t *)egui_malloc(sizeof(egui_image_qoi_checkpoint_t) * EGUI_IMAGE_QOI_CHECKPOINT_COUNT);
        if (qoi_checkpoints == NULL)
        {
            return NULL;
        }

        egui_api_memset(qoi_checkpoints, 0, sizeof(egui_image_qoi_checkpoint_t) * EGUI_IMAGE_QOI_CHECKPOINT_COUNT);
        qoi_checkpoint_next = 0;
    }

    return qoi_checkpoints;
}

void egui_image_qoi_release_checkpoints(void)
{
    if (qoi_checkpoints != NULL)
    {
        egui_free(qoi_checkpoints);
        qoi_checkpoints = NULL;
    }

    qoi_checkpoint_next = 0;
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_image_qoi_external_stream_init(egui_image_qoi_external_stream_t *stream, const egui_image_qoi_info_t *info, uint32_t pos)
{
    stream->info = info;
    stream->pos = pos;
    stream->window_offset = 0;
    stream->window_size = 0;
}

static int egui_image_qoi_external_stream_refill(egui_image_qoi_external_stream_t *stream)
{
    uint32_t remaining;
    uint32_t load_size;

    if (stream->pos >= stream->info->data_size)
    {
        return 0;
    }

    remaining = stream->info->data_size - stream->pos;
    load_size = remaining;
    if (load_size > EGUI_IMAGE_QOI_EXTERNAL_STREAM_WINDOW_SIZE)
    {
        load_size = EGUI_IMAGE_QOI_EXTERNAL_STREAM_WINDOW_SIZE;
    }

    egui_api_load_external_resource(stream->window, (egui_uintptr_t)stream->info->data_buf, stream->pos, load_size);
    stream->window_offset = stream->pos;
    stream->window_size = (uint16_t)load_size;
    return 1;
}

__EGUI_STATIC_INLINE__ int egui_image_qoi_external_stream_read_u8(egui_image_qoi_external_stream_t *stream, uint8_t *value)
{
    if (stream->pos >= stream->info->data_size)
    {
        return 0;
    }

    if (stream->window_size == 0 || stream->pos < stream->window_offset || stream->pos >= (stream->window_offset + stream->window_size))
    {
        if (!egui_image_qoi_external_stream_refill(stream))
        {
            return 0;
        }
    }

    *value = stream->window[stream->pos - stream->window_offset];
    stream->pos++;
    return 1;
}

static int egui_image_qoi_external_stream_read_bytes(egui_image_qoi_external_stream_t *stream, uint8_t *dst, uint32_t size)
{
    uint32_t remaining = size;

    while (remaining > 0)
    {
        uint32_t available;
        uint32_t chunk;

        if (stream->pos >= stream->info->data_size)
        {
            return 0;
        }

        if (stream->window_size == 0 || stream->pos < stream->window_offset || stream->pos >= (stream->window_offset + stream->window_size))
        {
            if (!egui_image_qoi_external_stream_refill(stream))
            {
                return 0;
            }
        }

        available = (stream->window_offset + stream->window_size) - stream->pos;
        chunk = remaining;
        if (chunk > available)
        {
            chunk = available;
        }

        egui_api_memcpy(dst, stream->window + (stream->pos - stream->window_offset), chunk);
        dst += chunk;
        stream->pos += chunk;
        remaining -= chunk;
    }

    return 1;
}
#endif

static int egui_image_qoi_prepare_decode_info(const egui_image_qoi_info_t *info, egui_image_qoi_info_t *decode_info)
{
    *decode_info = *info;

    if (decode_info->height > EGUI_IMAGE_QOI_ROW_INDEX_MAX)
    {
        EGUI_ASSERT(0);
        return 0;
    }

#if !EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
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
    egui_api_memset(&qoi_state, 0, sizeof(qoi_state));
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
    egui_image_qoi_checkpoint_t *checkpoints = egui_image_qoi_get_checkpoints();

    if (checkpoints == NULL)
    {
        return;
    }

    for (i = 0; i < EGUI_IMAGE_QOI_CHECKPOINT_COUNT; i++)
    {
        if (checkpoints[i].info == info && checkpoints[i].row == row)
        {
            checkpoints[i].state = qoi_state;
            return;
        }
    }

    checkpoints[qoi_checkpoint_next].state = qoi_state;
    checkpoints[qoi_checkpoint_next].row = row;
    checkpoints[qoi_checkpoint_next].info = info;
    qoi_checkpoint_next++;
    qoi_checkpoint_next &= (EGUI_IMAGE_QOI_CHECKPOINT_COUNT - 1);
}

static int egui_image_qoi_restore_checkpoint(const egui_image_qoi_info_t *info, uint16_t target_row)
{
    uint8_t i;
    egui_image_qoi_checkpoint_t *checkpoints = qoi_checkpoints;

    if (checkpoints == NULL)
    {
        return 0;
    }

    for (i = 0; i < EGUI_IMAGE_QOI_CHECKPOINT_COUNT; i++)
    {
        if (checkpoints[i].info == info && checkpoints[i].row == target_row)
        {
            qoi_state = checkpoints[i].state;
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

__EGUI_STATIC_INLINE__ void egui_image_qoi_decode_opcode(const uint8_t **data_ptr, const uint8_t *src_end, uint8_t *prev_r, uint8_t *prev_g, uint8_t *prev_b,
                                                         uint8_t *prev_a, uint16_t *prev_rgb565 EGUI_IMAGE_QOI_INDEX_PARAM, uint8_t *run)
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
            EGUI_IMAGE_QOI_INDEX_LOAD_RGBA(index, prev_r, prev_g, prev_b, prev_a, prev_rgb565);
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
    EGUI_IMAGE_QOI_INDEX_STORE_RGBA(hash, prev_r, prev_g, prev_b, prev_a, prev_rgb565);
    *data_ptr = data;
}

__EGUI_STATIC_INLINE__ void egui_image_qoi_decode_opcode_rgb(const uint8_t **data_ptr, const uint8_t *src_end, uint8_t *prev_r, uint8_t *prev_g,
                                                             uint8_t *prev_b, uint16_t *prev_rgb565 EGUI_IMAGE_QOI_INDEX_PARAM, uint8_t *run)
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
            EGUI_IMAGE_QOI_INDEX_LOAD_RGB(index, prev_r, prev_g, prev_b, prev_rgb565);
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
    EGUI_IMAGE_QOI_INDEX_STORE_RGB(hash, prev_r, prev_g, prev_b, prev_rgb565);
    *data_ptr = data;
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
__EGUI_STATIC_INLINE__ void egui_image_qoi_decode_opcode_external(egui_image_qoi_external_stream_t *stream, uint8_t *prev_r, uint8_t *prev_g, uint8_t *prev_b,
                                                                  uint8_t *prev_a, uint16_t *prev_rgb565 EGUI_IMAGE_QOI_INDEX_PARAM, uint8_t *run)
{
    uint8_t hash;
    uint8_t b1;

    if (!egui_image_qoi_external_stream_read_u8(stream, &b1))
    {
        return;
    }

    if (b1 < QOI_OP_RUN)
    {
        if (b1 < QOI_OP_DIFF)
        {
            uint8_t index = b1 & 0x3F;
            EGUI_IMAGE_QOI_INDEX_LOAD_RGBA(index, prev_r, prev_g, prev_b, prev_a, prev_rgb565);
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
            uint8_t b2;
            int8_t vg;

            if (!egui_image_qoi_external_stream_read_u8(stream, &b2))
            {
                return;
            }

            vg = (int8_t)((b1 & 0x3F) - 32);
            *prev_r += vg - 8 + ((b2 >> 4) & 0x0F);
            *prev_g += vg;
            *prev_b += vg - 8 + (b2 & 0x0F);
        }
    }
    else if (b1 < QOI_OP_RGB)
    {
        *run = (b1 & 0x3F);
        return;
    }
    else if (b1 == QOI_OP_RGB)
    {
        uint8_t rgb[3];

        if (!egui_image_qoi_external_stream_read_bytes(stream, rgb, sizeof(rgb)))
        {
            return;
        }

        *prev_r = rgb[0];
        *prev_g = rgb[1];
        *prev_b = rgb[2];
    }
    else
    {
        uint8_t rgba[4];

        if (!egui_image_qoi_external_stream_read_bytes(stream, rgba, sizeof(rgba)))
        {
            return;
        }

        *prev_r = rgba[0];
        *prev_g = rgba[1];
        *prev_b = rgba[2];
        *prev_a = rgba[3];
    }

    *prev_rgb565 = egui_image_qoi_rgb888_to_rgb565(*prev_r, *prev_g, *prev_b);
    hash = QOI_COLOR_HASH(*prev_r, *prev_g, *prev_b, *prev_a);
    EGUI_IMAGE_QOI_INDEX_STORE_RGBA(hash, prev_r, prev_g, prev_b, prev_a, prev_rgb565);
}

__EGUI_STATIC_INLINE__ void egui_image_qoi_decode_opcode_rgb_external(egui_image_qoi_external_stream_t *stream, uint8_t *prev_r, uint8_t *prev_g,
                                                                      uint8_t *prev_b, uint16_t *prev_rgb565 EGUI_IMAGE_QOI_INDEX_PARAM, uint8_t *run)
{
    uint8_t hash;
    uint8_t b1;

    if (!egui_image_qoi_external_stream_read_u8(stream, &b1))
    {
        return;
    }

    if (b1 < QOI_OP_RUN)
    {
        if (b1 < QOI_OP_DIFF)
        {
            uint8_t index = b1 & 0x3F;
            EGUI_IMAGE_QOI_INDEX_LOAD_RGB(index, prev_r, prev_g, prev_b, prev_rgb565);
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
            uint8_t b2;
            int8_t vg;

            if (!egui_image_qoi_external_stream_read_u8(stream, &b2))
            {
                return;
            }

            vg = (int8_t)((b1 & 0x3F) - 32);
            *prev_r += vg - 8 + ((b2 >> 4) & 0x0F);
            *prev_g += vg;
            *prev_b += vg - 8 + (b2 & 0x0F);
        }
    }
    else if (b1 == QOI_OP_RGB)
    {
        uint8_t rgb[3];

        if (!egui_image_qoi_external_stream_read_bytes(stream, rgb, sizeof(rgb)))
        {
            return;
        }

        *prev_r = rgb[0];
        *prev_g = rgb[1];
        *prev_b = rgb[2];
    }
    else if (b1 < QOI_OP_RGB)
    {
        *run = (b1 & 0x3F);
        return;
    }
    else
    {
        return;
    }

    *prev_rgb565 = egui_image_qoi_rgb888_to_rgb565(*prev_r, *prev_g, *prev_b);
    hash = QOI_COLOR_HASH(*prev_r, *prev_g, *prev_b, 255);
    EGUI_IMAGE_QOI_INDEX_STORE_RGB(hash, prev_r, prev_g, prev_b, prev_rgb565);
}
#endif

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
    EGUI_IMAGE_QOI_INDEX_DECLARE
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
                    EGUI_IMAGE_QOI_INDEX_LOAD_RGB(index, &prev_r, &prev_g, &prev_b, &prev_pixel);
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
        EGUI_IMAGE_QOI_INDEX_STORE_RGB(prev_hash, &prev_r, &prev_g, &prev_b, &prev_pixel);
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
    EGUI_IMAGE_QOI_INDEX_DECLARE
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

            egui_image_qoi_decode_opcode(&data, src_end, &prev_r, &prev_g, &prev_b, &prev_a, &prev_pixel EGUI_IMAGE_QOI_INDEX_ARG, &run);
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
                egui_api_memset(alpha, prev_a, repeat);
                run = (uint8_t)(run - repeat);
                dst += repeat;
                alpha += repeat;
                remaining = (uint16_t)(remaining - repeat);
                continue;
            }

            egui_image_qoi_decode_opcode(&data, src_end, &prev_r, &prev_g, &prev_b, &prev_a, &prev_pixel EGUI_IMAGE_QOI_INDEX_ARG, &run);
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

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
static void egui_image_qoi_decode_pixels_rgb565_opaque_partial(const egui_image_qoi_info_t *info, uint8_t *pixel_buf, uint16_t pixel_count)
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
    EGUI_IMAGE_QOI_INDEX_DECLARE
    uint16_t *dst = pixel_buf != NULL ? (uint16_t *)pixel_buf : NULL;
    uint16_t remaining = pixel_count;

    while (remaining > 0)
    {
        if (run > 0)
        {
            uint16_t repeat = remaining;
            if (repeat > run)
            {
                repeat = run;
            }
            if (dst != NULL)
            {
                egui_image_qoi_fill_rgb565(dst, prev_pixel, repeat);
                dst += repeat;
            }
            run = (uint8_t)(run - repeat);
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
                    EGUI_IMAGE_QOI_INDEX_LOAD_RGB(index, &prev_r, &prev_g, &prev_b, &prev_pixel);
                    prev_hash = index;
                    if (dst != NULL)
                    {
                        *dst++ = prev_pixel;
                    }
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
                if (dst != NULL)
                {
                    *dst++ = prev_pixel;
                }
                remaining--;
                continue;
            }
            else
            {
                break;
            }
        }

        prev_pixel = egui_image_qoi_rgb888_to_rgb565(prev_r, prev_g, prev_b);
        EGUI_IMAGE_QOI_INDEX_STORE_RGB(prev_hash, &prev_r, &prev_g, &prev_b, &prev_pixel);
        if (dst != NULL)
        {
            *dst++ = prev_pixel;
        }
        remaining--;
    }

    qoi_state.data_pos = (uint32_t)(data - src);
    qoi_state.prev_r = prev_r;
    qoi_state.prev_g = prev_g;
    qoi_state.prev_b = prev_b;
    qoi_state.prev_rgb565 = prev_pixel;
    qoi_state.run = run;
}
#endif
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
static void egui_image_qoi_decode_pixels_rgb565_partial(const egui_image_qoi_info_t *info, uint8_t *pixel_buf, uint8_t *alpha_buf, uint16_t pixel_count)
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
    EGUI_IMAGE_QOI_INDEX_DECLARE
    uint16_t *dst = pixel_buf != NULL ? (uint16_t *)pixel_buf : NULL;
    uint8_t *alpha = alpha_buf;
    uint16_t remaining = pixel_count;

    if (alpha_buf == NULL && info->channels == 3)
    {
        egui_image_qoi_decode_pixels_rgb565_opaque_partial(info, pixel_buf, pixel_count);
        return;
    }

    while (remaining > 0)
    {
        if (run > 0)
        {
            uint16_t repeat = remaining;
            if (repeat > run)
            {
                repeat = run;
            }
            if (dst != NULL)
            {
                egui_image_qoi_fill_rgb565(dst, prev_pixel, repeat);
                dst += repeat;
            }
            if (alpha != NULL)
            {
                egui_api_memset(alpha, prev_a, repeat);
                alpha += repeat;
            }
            run = (uint8_t)(run - repeat);
            remaining = (uint16_t)(remaining - repeat);
            continue;
        }

        egui_image_qoi_decode_opcode(&data, src_end, &prev_r, &prev_g, &prev_b, &prev_a, &prev_pixel EGUI_IMAGE_QOI_INDEX_ARG, &run);
        if (dst != NULL)
        {
            *dst++ = prev_pixel;
        }
        if (alpha != NULL)
        {
            *alpha++ = prev_a;
        }
        remaining--;
    }

    qoi_state.data_pos = (uint32_t)(data - src);
    qoi_state.prev_r = prev_r;
    qoi_state.prev_g = prev_g;
    qoi_state.prev_b = prev_b;
    qoi_state.prev_a = prev_a;
    qoi_state.prev_rgb565 = prev_pixel;
    qoi_state.run = run;
}
#endif
#endif

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
    EGUI_IMAGE_QOI_INDEX_DECLARE
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

        egui_image_qoi_decode_opcode(&data, src_end, &prev_r, &prev_g, &prev_b, &prev_a, &prev_rgb565 EGUI_IMAGE_QOI_INDEX_ARG, &run);
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

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_image_qoi_decode_row_external(const egui_image_qoi_info_t *info, uint8_t *pixel_buf, uint8_t *alpha_buf)
{
    egui_image_qoi_external_stream_t stream;
    uint8_t prev_r = qoi_state.prev_r;
    uint8_t prev_g = qoi_state.prev_g;
    uint8_t prev_b = qoi_state.prev_b;
    uint8_t prev_a = qoi_state.prev_a;
    uint16_t prev_rgb565 = qoi_state.prev_rgb565;
    uint8_t run = qoi_state.run;
    EGUI_IMAGE_QOI_INDEX_DECLARE
    uint16_t remaining = info->width;

    egui_image_qoi_external_stream_init(&stream, info, qoi_state.data_pos);

    if (info->data_type == EGUI_IMAGE_DATA_TYPE_RGB565)
    {
        uint16_t *dst = (uint16_t *)pixel_buf;
        uint8_t *alpha = alpha_buf;

        while (remaining > 0)
        {
            if (run > 0)
            {
                uint16_t repeat = remaining;

                if (repeat > run)
                {
                    repeat = run;
                }

                egui_image_qoi_fill_rgb565(dst, prev_rgb565, repeat);
                if (alpha != NULL)
                {
                    egui_api_memset(alpha, prev_a, repeat);
                    alpha += repeat;
                }

                run = (uint8_t)(run - repeat);
                dst += repeat;
                remaining = (uint16_t)(remaining - repeat);
                continue;
            }

            if (info->channels == 3)
            {
                egui_image_qoi_decode_opcode_rgb_external(&stream, &prev_r, &prev_g, &prev_b, &prev_rgb565 EGUI_IMAGE_QOI_INDEX_ARG, &run);
            }
            else
            {
                egui_image_qoi_decode_opcode_external(&stream, &prev_r, &prev_g, &prev_b, &prev_a, &prev_rgb565 EGUI_IMAGE_QOI_INDEX_ARG, &run);
            }

            *dst++ = prev_rgb565;
            if (alpha != NULL)
            {
                *alpha++ = prev_a;
            }
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

                egui_image_qoi_fill_rgb32(pixel_buf, prev_r, prev_g, prev_b, prev_a, repeat);
                run = (uint8_t)(run - repeat);
                pixel_buf += (uint32_t)repeat << 2;
                remaining = (uint16_t)(remaining - repeat);
                continue;
            }

            if (info->channels == 3)
            {
                egui_image_qoi_decode_opcode_rgb_external(&stream, &prev_r, &prev_g, &prev_b, &prev_rgb565 EGUI_IMAGE_QOI_INDEX_ARG, &run);
            }
            else
            {
                egui_image_qoi_decode_opcode_external(&stream, &prev_r, &prev_g, &prev_b, &prev_a, &prev_rgb565 EGUI_IMAGE_QOI_INDEX_ARG, &run);
            }

            pixel_buf[0] = prev_r;
            pixel_buf[1] = prev_g;
            pixel_buf[2] = prev_b;
            pixel_buf[3] = prev_a;
            pixel_buf += 4;
            remaining--;
        }
    }

    qoi_state.data_pos = stream.pos;
    qoi_state.prev_r = prev_r;
    qoi_state.prev_g = prev_g;
    qoi_state.prev_b = prev_b;
    qoi_state.prev_a = prev_a;
    qoi_state.prev_rgb565 = prev_rgb565;
    qoi_state.run = run;
}

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
static void egui_image_qoi_decode_pixels_rgb565_external_partial(const egui_image_qoi_info_t *info, uint8_t *pixel_buf, uint8_t *alpha_buf,
                                                                 uint16_t pixel_count)
{
    egui_image_qoi_external_stream_t stream;
    uint8_t prev_r = qoi_state.prev_r;
    uint8_t prev_g = qoi_state.prev_g;
    uint8_t prev_b = qoi_state.prev_b;
    uint8_t prev_a = qoi_state.prev_a;
    uint16_t prev_rgb565 = qoi_state.prev_rgb565;
    uint8_t run = qoi_state.run;
    EGUI_IMAGE_QOI_INDEX_DECLARE
    uint16_t remaining = pixel_count;
    uint16_t *dst = pixel_buf != NULL ? (uint16_t *)pixel_buf : NULL;
    uint8_t *alpha = alpha_buf;

    egui_image_qoi_external_stream_init(&stream, info, qoi_state.data_pos);

    while (remaining > 0)
    {
        if (run > 0)
        {
            uint16_t repeat = remaining;

            if (repeat > run)
            {
                repeat = run;
            }

            if (dst != NULL)
            {
                egui_image_qoi_fill_rgb565(dst, prev_rgb565, repeat);
                dst += repeat;
            }
            if (alpha != NULL)
            {
                egui_api_memset(alpha, prev_a, repeat);
                alpha += repeat;
            }

            run = (uint8_t)(run - repeat);
            remaining = (uint16_t)(remaining - repeat);
            continue;
        }

        if (info->channels == 3)
        {
            egui_image_qoi_decode_opcode_rgb_external(&stream, &prev_r, &prev_g, &prev_b, &prev_rgb565 EGUI_IMAGE_QOI_INDEX_ARG, &run);
        }
        else
        {
            egui_image_qoi_decode_opcode_external(&stream, &prev_r, &prev_g, &prev_b, &prev_a, &prev_rgb565 EGUI_IMAGE_QOI_INDEX_ARG, &run);
        }

        if (dst != NULL)
        {
            *dst++ = prev_rgb565;
        }
        if (alpha != NULL)
        {
            *alpha++ = prev_a;
        }
        remaining--;
    }

    qoi_state.data_pos = stream.pos;
    qoi_state.prev_r = prev_r;
    qoi_state.prev_g = prev_g;
    qoi_state.prev_b = prev_b;
    qoi_state.prev_a = prev_a;
    qoi_state.prev_rgb565 = prev_rgb565;
    qoi_state.run = run;
}
#endif

static void egui_image_qoi_skip_row_external(const egui_image_qoi_info_t *info)
{
    egui_image_qoi_external_stream_t stream;
    uint8_t prev_r = qoi_state.prev_r;
    uint8_t prev_g = qoi_state.prev_g;
    uint8_t prev_b = qoi_state.prev_b;
    uint8_t prev_a = qoi_state.prev_a;
    uint16_t prev_rgb565 = qoi_state.prev_rgb565;
    uint8_t run = qoi_state.run;
    EGUI_IMAGE_QOI_INDEX_DECLARE
    uint16_t remaining = info->width;

    egui_image_qoi_external_stream_init(&stream, info, qoi_state.data_pos);

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

        if (info->channels == 3)
        {
            egui_image_qoi_decode_opcode_rgb_external(&stream, &prev_r, &prev_g, &prev_b, &prev_rgb565 EGUI_IMAGE_QOI_INDEX_ARG, &run);
        }
        else
        {
            egui_image_qoi_decode_opcode_external(&stream, &prev_r, &prev_g, &prev_b, &prev_a, &prev_rgb565 EGUI_IMAGE_QOI_INDEX_ARG, &run);
        }
        remaining--;
    }

    qoi_state.data_pos = stream.pos;
    qoi_state.prev_r = prev_r;
    qoi_state.prev_g = prev_g;
    qoi_state.prev_b = prev_b;
    qoi_state.prev_a = prev_a;
    qoi_state.prev_rgb565 = prev_rgb565;
    qoi_state.run = run;
}
#endif

static void egui_image_qoi_decode_row(const egui_image_qoi_info_t *info, uint8_t *pixel_buf, uint8_t *alpha_buf)
{
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_qoi_decode_row_external(info, pixel_buf, alpha_buf);
        return;
    }
#endif

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

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
static void egui_image_qoi_decode_pixels_rgb565_partial_any(const egui_image_qoi_info_t *info, uint8_t *pixel_buf, uint8_t *alpha_buf, uint16_t pixel_count)
{
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_qoi_decode_pixels_rgb565_external_partial(info, pixel_buf, alpha_buf, pixel_count);
        return;
    }
#endif
    egui_image_qoi_decode_pixels_rgb565_partial(info, pixel_buf, alpha_buf, pixel_count);
}

static void egui_image_qoi_blend_repeat_rgb565_alpha8_fast(egui_color_int_t *dst, uint16_t pixel, uint8_t alpha, uint16_t count)
{
    if (count == 0 || alpha == 0)
    {
        return;
    }

    if (alpha >= EGUI_ALPHA_100)
    {
        egui_image_qoi_fill_rgb565((uint16_t *)dst, pixel, count);
        return;
    }

    for (uint16_t i = 0; i < count; i++)
    {
        egui_image_std_blend_rgb565_src_pixel_fast(&dst[i], pixel, alpha);
    }
}

static void egui_image_qoi_decode_pixels_rgb565_alpha8_blend_partial(const egui_image_qoi_info_t *info, egui_color_int_t *dst, uint16_t pixel_count)
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
    EGUI_IMAGE_QOI_INDEX_DECLARE
    uint16_t remaining = pixel_count;

    while (remaining > 0)
    {
        if (run > 0)
        {
            uint16_t repeat = remaining;

            if (repeat > run)
            {
                repeat = run;
            }

            egui_image_qoi_blend_repeat_rgb565_alpha8_fast(dst, prev_pixel, prev_a, repeat);
            dst += repeat;
            run = (uint8_t)(run - repeat);
            remaining = (uint16_t)(remaining - repeat);
            continue;
        }

        egui_image_qoi_decode_opcode(&data, src_end, &prev_r, &prev_g, &prev_b, &prev_a, &prev_pixel EGUI_IMAGE_QOI_INDEX_ARG, &run);
        egui_image_std_blend_rgb565_src_pixel_fast(dst, prev_pixel, prev_a);
        dst++;
        remaining--;
    }

    qoi_state.data_pos = (uint32_t)(data - src);
    qoi_state.prev_r = prev_r;
    qoi_state.prev_g = prev_g;
    qoi_state.prev_b = prev_b;
    qoi_state.prev_a = prev_a;
    qoi_state.prev_rgb565 = prev_pixel;
    qoi_state.run = run;
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static void egui_image_qoi_decode_pixels_rgb565_alpha8_blend_external_partial(const egui_image_qoi_info_t *info, egui_color_int_t *dst, uint16_t pixel_count)
{
    egui_image_qoi_external_stream_t stream;
    uint8_t prev_r = qoi_state.prev_r;
    uint8_t prev_g = qoi_state.prev_g;
    uint8_t prev_b = qoi_state.prev_b;
    uint8_t prev_a = qoi_state.prev_a;
    uint16_t prev_rgb565 = qoi_state.prev_rgb565;
    uint8_t run = qoi_state.run;
    EGUI_IMAGE_QOI_INDEX_DECLARE
    uint16_t remaining = pixel_count;

    egui_image_qoi_external_stream_init(&stream, info, qoi_state.data_pos);

    while (remaining > 0)
    {
        if (run > 0)
        {
            uint16_t repeat = remaining;

            if (repeat > run)
            {
                repeat = run;
            }

            egui_image_qoi_blend_repeat_rgb565_alpha8_fast(dst, prev_rgb565, prev_a, repeat);
            dst += repeat;
            run = (uint8_t)(run - repeat);
            remaining = (uint16_t)(remaining - repeat);
            continue;
        }

        egui_image_qoi_decode_opcode_external(&stream, &prev_r, &prev_g, &prev_b, &prev_a, &prev_rgb565 EGUI_IMAGE_QOI_INDEX_ARG, &run);
        egui_image_std_blend_rgb565_src_pixel_fast(dst, prev_rgb565, prev_a);
        dst++;
        remaining--;
    }

    qoi_state.data_pos = stream.pos;
    qoi_state.prev_r = prev_r;
    qoi_state.prev_g = prev_g;
    qoi_state.prev_b = prev_b;
    qoi_state.prev_a = prev_a;
    qoi_state.prev_rgb565 = prev_rgb565;
    qoi_state.run = run;
}
#endif

static void egui_image_qoi_decode_pixels_rgb565_alpha8_blend_partial_any(const egui_image_qoi_info_t *info, egui_color_int_t *dst, uint16_t pixel_count)
{
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_qoi_decode_pixels_rgb565_alpha8_blend_external_partial(info, dst, pixel_count);
        return;
    }
#endif
    egui_image_qoi_decode_pixels_rgb565_alpha8_blend_partial(info, dst, pixel_count);
}

static void egui_image_qoi_decode_row_rgb565_split(const egui_image_qoi_info_t *info, uint8_t *visible_pixel_buf, uint8_t *visible_alpha_buf,
                                                   uint16_t visible_col_start, uint16_t visible_col_count, uint8_t *tail_pixel_buf, uint8_t *tail_alpha_buf,
                                                   uint16_t tail_col_start, uint16_t tail_col_count)
{
    uint16_t current_col = 0;

    EGUI_ASSERT(visible_col_count > 0);
    EGUI_ASSERT(visible_col_start + visible_col_count <= info->width);
    EGUI_ASSERT(tail_col_count == 0 || tail_col_start >= (uint16_t)(visible_col_start + visible_col_count));
    EGUI_ASSERT(tail_col_start + tail_col_count <= info->width);

    if (visible_col_start > 0)
    {
        egui_image_qoi_decode_pixels_rgb565_partial_any(info, NULL, NULL, visible_col_start);
    }

    egui_image_qoi_decode_pixels_rgb565_partial_any(info, visible_pixel_buf, visible_alpha_buf, visible_col_count);
    current_col = (uint16_t)(visible_col_start + visible_col_count);

    if (tail_col_count > 0)
    {
        if (tail_col_start > current_col)
        {
            egui_image_qoi_decode_pixels_rgb565_partial_any(info, NULL, NULL, (uint16_t)(tail_col_start - current_col));
        }

        egui_image_qoi_decode_pixels_rgb565_partial_any(info, tail_pixel_buf, tail_alpha_buf, tail_col_count);
        current_col = (uint16_t)(tail_col_start + tail_col_count);
    }

    if (current_col < info->width)
    {
        egui_image_qoi_decode_pixels_rgb565_partial_any(info, NULL, NULL, (uint16_t)(info->width - current_col));
    }
}

static void egui_image_qoi_decode_row_rgb565_alpha8_blend_split(const egui_image_qoi_info_t *info, egui_color_int_t *visible_dst, uint16_t visible_col_start,
                                                                uint16_t visible_col_count, uint8_t *tail_pixel_buf, uint8_t *tail_alpha_buf,
                                                                uint16_t tail_col_start, uint16_t tail_col_count)
{
    uint16_t current_col = 0;

    EGUI_ASSERT(info->channels == 4);
    EGUI_ASSERT(visible_col_count > 0);
    EGUI_ASSERT(visible_col_start + visible_col_count <= info->width);
    EGUI_ASSERT(tail_col_count == 0 || tail_col_start >= (uint16_t)(visible_col_start + visible_col_count));
    EGUI_ASSERT(tail_col_start + tail_col_count <= info->width);

    if (visible_col_start > 0)
    {
        egui_image_qoi_decode_pixels_rgb565_partial_any(info, NULL, NULL, visible_col_start);
    }

    egui_image_qoi_decode_pixels_rgb565_alpha8_blend_partial_any(info, visible_dst, visible_col_count);
    current_col = (uint16_t)(visible_col_start + visible_col_count);

    if (tail_col_count > 0)
    {
        if (tail_col_start > current_col)
        {
            egui_image_qoi_decode_pixels_rgb565_partial_any(info, NULL, NULL, (uint16_t)(tail_col_start - current_col));
        }

        egui_image_qoi_decode_pixels_rgb565_partial_any(info, tail_pixel_buf, tail_alpha_buf, tail_col_count);
        current_col = (uint16_t)(tail_col_start + tail_col_count);
    }

    if (current_col < info->width)
    {
        egui_image_qoi_decode_pixels_rgb565_partial_any(info, NULL, NULL, (uint16_t)(info->width - current_col));
    }
}

static void egui_image_qoi_decode_row_rgb565_alpha8_masked_split(const egui_image_qoi_info_t *info, egui_canvas_t *canvas, egui_color_int_t *dst_row,
                                                                 egui_dim_t screen_x, egui_dim_t screen_y, egui_alpha_t canvas_alpha,
                                                                 uint16_t visible_col_start, uint16_t visible_col_count, uint8_t *visible_pixel_buf,
                                                                 uint8_t *visible_alpha_buf, uint8_t *tail_pixel_buf, uint8_t *tail_alpha_buf,
                                                                 uint16_t tail_col_start, uint16_t tail_col_count)
{
    uint16_t current_col = 0;
    int use_image_mask = (canvas != NULL && canvas->mask != NULL && canvas->mask->api->kind == EGUI_MASK_KIND_IMAGE);

    EGUI_ASSERT(info->channels == 4);
    EGUI_ASSERT(visible_col_count > 0);
    EGUI_ASSERT(visible_col_start + visible_col_count <= info->width);
    EGUI_ASSERT(tail_col_count == 0 || tail_col_start >= (uint16_t)(visible_col_start + visible_col_count));
    EGUI_ASSERT(tail_col_start + tail_col_count <= info->width);
    EGUI_ASSERT(visible_pixel_buf != NULL);
    EGUI_ASSERT(visible_alpha_buf != NULL);

    if (visible_col_start > 0)
    {
        egui_image_qoi_decode_pixels_rgb565_partial_any(info, NULL, NULL, visible_col_start);
    }

    egui_image_qoi_decode_pixels_rgb565_partial_any(info, visible_pixel_buf, visible_alpha_buf, visible_col_count);

    if (use_image_mask)
    {
        egui_image_qoi_blend_masked_rgb565_image_row(canvas, dst_row, (const uint16_t *)visible_pixel_buf, visible_alpha_buf, visible_col_count, screen_x,
                                                     screen_y, canvas_alpha, NULL);
    }
    else
    {
        egui_image_std_blend_rgb565_alpha8_masked_row(canvas, dst_row, (const uint16_t *)visible_pixel_buf, visible_alpha_buf, visible_col_count, screen_x,
                                                      screen_y, canvas_alpha);
    }

    current_col = (uint16_t)(visible_col_start + visible_col_count);

    if (tail_col_count > 0)
    {
        if (tail_col_start > current_col)
        {
            egui_image_qoi_decode_pixels_rgb565_partial_any(info, NULL, NULL, (uint16_t)(tail_col_start - current_col));
        }

        egui_image_qoi_decode_pixels_rgb565_partial_any(info, tail_pixel_buf, tail_alpha_buf, tail_col_count);
        current_col = (uint16_t)(tail_col_start + tail_col_count);
    }

    if (current_col < info->width)
    {
        egui_image_qoi_decode_pixels_rgb565_partial_any(info, NULL, NULL, (uint16_t)(info->width - current_col));
    }
}
#endif
#endif

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
    EGUI_IMAGE_QOI_INDEX_DECLARE
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

        egui_image_qoi_decode_opcode_rgb(&data, src_end, &prev_r, &prev_g, &prev_b, &prev_rgb565 EGUI_IMAGE_QOI_INDEX_ARG, &run);
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
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    if (info->res_type == EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        egui_image_qoi_skip_row_external(info);
        return;
    }
#endif

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
    EGUI_IMAGE_QOI_INDEX_DECLARE
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

        egui_image_qoi_decode_opcode(&data, src_end, &prev_r, &prev_g, &prev_b, &prev_a, &prev_rgb565 EGUI_IMAGE_QOI_INDEX_ARG, &run);
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

static int egui_image_qoi_get_point(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    /* QOI is sequential-only; random access not efficiently supported */
    color->full = 0;
    *alpha = 0;
    return 0;
}

static int egui_image_qoi_get_point_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t *color,
                                           egui_alpha_t *alpha)
{
    /* Resize not supported for compressed images */
    color->full = 0;
    *alpha = 0;
    return 0;
}

static void egui_image_qoi_blend_masked_rgb565_image_row(egui_canvas_t *canvas, egui_color_int_t *dst_row, const uint16_t *src_pixels,
                                                         const uint8_t *src_alpha_row, egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y,
                                                         egui_alpha_t canvas_alpha, const uint8_t **opaque_alpha_row)
{
#if EGUI_CONFIG_COLOR_DEPTH == 16 && EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    if (src_alpha_row == NULL)
    {
        if (egui_mask_image_blend_rgb565_row_segment(canvas->mask, dst_row, src_pixels, count, screen_x, screen_y, canvas_alpha))
        {
            return;
        }
    }
    else if (egui_mask_image_blend_rgb565_alpha8_row_segment(canvas->mask, dst_row, src_pixels, src_alpha_row, count, screen_x, screen_y, canvas_alpha))
    {
        return;
    }

    if (src_alpha_row == NULL)
    {
        const uint8_t *alpha_row = NULL;

        if (opaque_alpha_row != NULL)
        {
            if (*opaque_alpha_row == NULL)
            {
                *opaque_alpha_row = egui_image_decode_get_opaque_alpha_row(count);
            }
            alpha_row = *opaque_alpha_row;
        }

        if (alpha_row != NULL)
        {
            egui_image_std_blend_rgb565_alpha8_masked_row(canvas, dst_row, src_pixels, alpha_row, count, screen_x, screen_y, canvas_alpha);
        }
        return;
    }

    egui_image_std_blend_rgb565_alpha8_masked_row(canvas, dst_row, src_pixels, src_alpha_row, count, screen_x, screen_y, canvas_alpha);
#else
    EGUI_UNUSED(canvas);
    EGUI_UNUSED(dst_row);
    EGUI_UNUSED(src_pixels);
    EGUI_UNUSED(src_alpha_row);
    EGUI_UNUSED(count);
    EGUI_UNUSED(screen_x);
    EGUI_UNUSED(screen_y);
    EGUI_UNUSED(canvas_alpha);
    EGUI_UNUSED(opaque_alpha_row);
#endif
}

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
static int egui_image_qoi_can_use_tail_row_cache(const egui_image_qoi_info_t *info, egui_dim_t img_col_start, egui_dim_t count)
{
    return EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE && info->data_type == EGUI_IMAGE_DATA_TYPE_RGB565 && img_col_start >= 0 && count > 0 &&
           ((uint32_t)img_col_start + (uint32_t)count) < info->width &&
           egui_image_decode_limit_tail_cache_cols((uint16_t)(info->width - (uint16_t)(img_col_start + count))) > 0;
}

static void egui_image_qoi_blend_cached_rows(const egui_image_qoi_info_t *info, egui_dim_t y, egui_dim_t screen_x_start, egui_dim_t img_col_start,
                                             egui_dim_t count, egui_dim_t img_y_start, egui_dim_t img_y_end, egui_dim_t cache_row_start,
                                             egui_color_int_t *fast_dst_row, egui_dim_t fast_dst_stride, egui_canvas_t *masked_canvas,
                                             egui_color_int_t *masked_dst_row, egui_dim_t masked_dst_stride, int use_fast_copy, int use_fast_alpha8,
                                             int use_masked_opaque, int use_masked_alpha8)
{
    int has_alpha = (info->channels == 4);
    uint8_t pixel_size = (info->data_type == EGUI_IMAGE_DATA_TYPE_RGB565) ? 2 : 4;
    uint8_t alpha_type = has_alpha ? EGUI_IMAGE_ALPHA_TYPE_8 : EGUI_IMAGE_ALPHA_TYPE_1;
    egui_dim_t row_count = img_y_end - img_y_start;
    egui_dim_t screen_y = y + img_y_start;
    uint16_t cache_col_start = egui_image_decode_cache_col_start();
    uint16_t cache_row_width = egui_image_decode_cache_row_width();
    uint16_t row_in_cache = (uint16_t)(img_y_start - cache_row_start);
    egui_dim_t cache_img_col_start = img_col_start - cache_col_start;
    uint32_t pixel_row_bytes = (uint32_t)cache_row_width * pixel_size;
    uint32_t alpha_row_bytes = has_alpha ? cache_row_width : 0;
    const uint8_t *pixel_buf = egui_image_decode_cache_pixel_row(row_in_cache, cache_row_width, pixel_size);
    const uint8_t *alpha_buf = has_alpha ? egui_image_decode_cache_alpha_row(row_in_cache, cache_row_width) : NULL;

    EGUI_ASSERT(cache_row_width > 0);
    EGUI_ASSERT(img_col_start >= cache_col_start);

    if (fast_dst_row != NULL && use_fast_copy)
    {
        const uint16_t *src_pixels = (const uint16_t *)pixel_buf + cache_img_col_start;

        for (egui_dim_t row = 0; row < row_count; row++)
        {
            egui_api_memcpy(fast_dst_row, src_pixels, (size_t)count * sizeof(uint16_t));
            fast_dst_row += fast_dst_stride;
            src_pixels += cache_row_width;
        }
        return;
    }

    if (fast_dst_row != NULL && use_fast_alpha8)
    {
        const uint16_t *src_pixels = (const uint16_t *)pixel_buf + cache_img_col_start;
        const uint8_t *src_alpha = alpha_buf + cache_img_col_start;

        for (egui_dim_t row = 0; row < row_count; row++)
        {
            egui_image_decode_blend_rgb565_alpha8_row_fast_path(fast_dst_row, src_pixels, src_alpha, count);
            fast_dst_row += fast_dst_stride;
            src_pixels += cache_row_width;
            src_alpha += alpha_row_bytes;
        }
        return;
    }

    if (masked_canvas != NULL && masked_dst_row != NULL && use_masked_opaque)
    {
        const uint16_t *src_pixels = (const uint16_t *)pixel_buf + cache_img_col_start;
        const uint8_t *opaque_alpha_row = NULL;
        egui_alpha_t canvas_alpha = masked_canvas->alpha;
        int use_image_mask = (masked_canvas->mask != NULL && masked_canvas->mask->api->kind == EGUI_MASK_KIND_IMAGE);

        if (use_image_mask)
        {
            if (egui_mask_image_blend_rgb565_row_block(masked_canvas->mask, masked_dst_row, masked_dst_stride, src_pixels, cache_row_width, row_count, count,
                                                       screen_x_start, screen_y, canvas_alpha))
            {
                return;
            }

            for (egui_dim_t row = 0; row < row_count; row++)
            {
                egui_image_qoi_blend_masked_rgb565_image_row(masked_canvas, masked_dst_row, src_pixels, NULL, count, screen_x_start, screen_y, canvas_alpha,
                                                             &opaque_alpha_row);
                masked_dst_row += masked_dst_stride;
                src_pixels += cache_row_width;
                screen_y++;
            }
        }
        else
        {
            for (egui_dim_t row = 0; row < row_count; row++)
            {
                if (!egui_image_std_blend_rgb565_masked_row(masked_canvas, masked_dst_row, src_pixels, count, screen_x_start, screen_y, canvas_alpha))
                {
                    if (opaque_alpha_row == NULL)
                    {
                        opaque_alpha_row = egui_image_decode_get_opaque_alpha_row(count);
                        if (opaque_alpha_row == NULL)
                        {
                            return;
                        }
                    }
                    egui_image_std_blend_rgb565_alpha8_masked_row(masked_canvas, masked_dst_row, src_pixels, opaque_alpha_row, count, screen_x_start, screen_y,
                                                                  canvas_alpha);
                }
                masked_dst_row += masked_dst_stride;
                src_pixels += cache_row_width;
                screen_y++;
            }
        }
        return;
    }

    if (masked_canvas != NULL && masked_dst_row != NULL && use_masked_alpha8)
    {
        const uint16_t *src_pixels = (const uint16_t *)pixel_buf + cache_img_col_start;
        const uint8_t *src_alpha = alpha_buf + cache_img_col_start;
        egui_alpha_t canvas_alpha = masked_canvas->alpha;
        int use_image_mask = (masked_canvas->mask != NULL && masked_canvas->mask->api->kind == EGUI_MASK_KIND_IMAGE);

        if (use_image_mask)
        {
            if (egui_mask_image_blend_rgb565_alpha8_row_block(masked_canvas->mask, masked_dst_row, masked_dst_stride, src_pixels, cache_row_width, src_alpha,
                                                              alpha_row_bytes, row_count, count, screen_x_start, screen_y, canvas_alpha))
            {
                return;
            }

            for (egui_dim_t row = 0; row < row_count; row++)
            {
                egui_image_qoi_blend_masked_rgb565_image_row(masked_canvas, masked_dst_row, src_pixels, src_alpha, count, screen_x_start, screen_y,
                                                             canvas_alpha, NULL);
                masked_dst_row += masked_dst_stride;
                src_pixels += cache_row_width;
                src_alpha += alpha_row_bytes;
                screen_y++;
            }
        }
        else
        {
            for (egui_dim_t row = 0; row < row_count; row++)
            {
                egui_image_std_blend_rgb565_alpha8_masked_row(masked_canvas, masked_dst_row, src_pixels, src_alpha, count, screen_x_start, screen_y,
                                                              canvas_alpha);
                masked_dst_row += masked_dst_stride;
                src_pixels += cache_row_width;
                src_alpha += alpha_row_bytes;
                screen_y++;
            }
        }
        return;
    }

    for (egui_dim_t row = 0; row < row_count; row++)
    {
        egui_image_decode_blend_row_clipped(screen_x_start, screen_y, cache_img_col_start, count, info->data_type, alpha_type, has_alpha, pixel_buf, alpha_buf);

        pixel_buf += pixel_row_bytes;
        if (alpha_buf != NULL)
        {
            alpha_buf += alpha_row_bytes;
        }
        screen_y++;
    }
}
#endif

#if EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES > 0
static void egui_image_qoi_blend_persistent_cached_rows(const egui_image_qoi_info_t *info, egui_dim_t y, egui_dim_t screen_x_start, egui_dim_t img_col_start,
                                                        egui_dim_t count, egui_dim_t img_y_start, egui_dim_t img_y_end, egui_color_int_t *fast_dst_row,
                                                        egui_dim_t fast_dst_stride, egui_canvas_t *masked_canvas, egui_color_int_t *masked_dst_row,
                                                        egui_dim_t masked_dst_stride, int use_fast_copy, int use_fast_alpha8, int use_masked_opaque,
                                                        int use_masked_alpha8)
{
    int has_alpha = (info->channels == 4);
    uint8_t alpha_type = has_alpha ? EGUI_IMAGE_ALPHA_TYPE_8 : EGUI_IMAGE_ALPHA_TYPE_1;
    uint8_t pixel_size = (info->data_type == EGUI_IMAGE_DATA_TYPE_RGB565) ? 2 : 4;
    egui_dim_t row_count = img_y_end - img_y_start;
    egui_dim_t screen_y = y + img_y_start;
    uint32_t pixel_row_bytes = (uint32_t)info->width * pixel_size;
    uint32_t alpha_row_bytes = has_alpha ? info->width : 0;
    const uint8_t *pixel_buf = egui_image_decode_persistent_cache_pixel_row((uint16_t)img_y_start);
    const uint8_t *alpha_buf = has_alpha ? egui_image_decode_persistent_cache_alpha_row_bytes((uint16_t)img_y_start) : NULL;

    if (fast_dst_row != NULL && use_fast_copy)
    {
        const uint16_t *src_pixels = (const uint16_t *)pixel_buf + img_col_start;

        for (egui_dim_t row = 0; row < row_count; row++)
        {
            egui_api_memcpy(fast_dst_row, src_pixels, (size_t)count * sizeof(uint16_t));
            fast_dst_row += fast_dst_stride;
            src_pixels += info->width;
        }
        return;
    }

    if (fast_dst_row != NULL && use_fast_alpha8)
    {
        const uint16_t *src_pixels = (const uint16_t *)pixel_buf + img_col_start;
        const uint8_t *src_alpha = alpha_buf + img_col_start;

        for (egui_dim_t row = 0; row < row_count; row++)
        {
            egui_image_decode_blend_rgb565_alpha8_row_fast_path(fast_dst_row, src_pixels, src_alpha, count);
            fast_dst_row += fast_dst_stride;
            src_pixels += info->width;
            src_alpha += alpha_row_bytes;
        }
        return;
    }

    if (masked_canvas != NULL && masked_dst_row != NULL && use_masked_opaque)
    {
        const uint16_t *src_pixels = (const uint16_t *)pixel_buf + img_col_start;
        const uint8_t *opaque_alpha_row = NULL;
        egui_alpha_t canvas_alpha = masked_canvas->alpha;
        int use_image_mask = (masked_canvas->mask != NULL && masked_canvas->mask->api->kind == EGUI_MASK_KIND_IMAGE);

        if (use_image_mask)
        {
            if (egui_mask_image_blend_rgb565_row_block(masked_canvas->mask, masked_dst_row, masked_dst_stride, src_pixels, info->width, row_count, count,
                                                       screen_x_start, screen_y, canvas_alpha))
            {
                return;
            }

            for (egui_dim_t row = 0; row < row_count; row++)
            {
                egui_image_qoi_blend_masked_rgb565_image_row(masked_canvas, masked_dst_row, src_pixels, NULL, count, screen_x_start, screen_y, canvas_alpha,
                                                             &opaque_alpha_row);
                masked_dst_row += masked_dst_stride;
                src_pixels += info->width;
                screen_y++;
            }
        }
        else
        {
            for (egui_dim_t row = 0; row < row_count; row++)
            {
                if (!egui_image_std_blend_rgb565_masked_row(masked_canvas, masked_dst_row, src_pixels, count, screen_x_start, screen_y, canvas_alpha))
                {
                    if (opaque_alpha_row == NULL)
                    {
                        opaque_alpha_row = egui_image_decode_get_opaque_alpha_row(count);
                        if (opaque_alpha_row == NULL)
                        {
                            return;
                        }
                    }
                    egui_image_std_blend_rgb565_alpha8_masked_row(masked_canvas, masked_dst_row, src_pixels, opaque_alpha_row, count, screen_x_start, screen_y,
                                                                  canvas_alpha);
                }
                masked_dst_row += masked_dst_stride;
                src_pixels += info->width;
                screen_y++;
            }
        }
        return;
    }

    if (masked_canvas != NULL && masked_dst_row != NULL && use_masked_alpha8)
    {
        const uint16_t *src_pixels = (const uint16_t *)pixel_buf + img_col_start;
        const uint8_t *src_alpha = alpha_buf + img_col_start;
        egui_alpha_t canvas_alpha = masked_canvas->alpha;
        int use_image_mask = (masked_canvas->mask != NULL && masked_canvas->mask->api->kind == EGUI_MASK_KIND_IMAGE);

        if (use_image_mask)
        {
            if (egui_mask_image_blend_rgb565_alpha8_row_block(masked_canvas->mask, masked_dst_row, masked_dst_stride, src_pixels, info->width, src_alpha,
                                                              alpha_row_bytes, row_count, count, screen_x_start, screen_y, canvas_alpha))
            {
                return;
            }

            for (egui_dim_t row = 0; row < row_count; row++)
            {
                egui_image_qoi_blend_masked_rgb565_image_row(masked_canvas, masked_dst_row, src_pixels, src_alpha, count, screen_x_start, screen_y,
                                                             canvas_alpha, NULL);
                masked_dst_row += masked_dst_stride;
                src_pixels += info->width;
                src_alpha += alpha_row_bytes;
                screen_y++;
            }
        }
        else
        {
            for (egui_dim_t row = 0; row < row_count; row++)
            {
                egui_image_std_blend_rgb565_alpha8_masked_row(masked_canvas, masked_dst_row, src_pixels, src_alpha, count, screen_x_start, screen_y,
                                                              canvas_alpha);
                masked_dst_row += masked_dst_stride;
                src_pixels += info->width;
                src_alpha += alpha_row_bytes;
                screen_y++;
            }
        }
        return;
    }

    for (egui_dim_t row = 0; row < row_count; row++)
    {
        egui_image_decode_blend_row_clipped(screen_x_start, screen_y, img_col_start, count, info->data_type, alpha_type, has_alpha, pixel_buf, alpha_buf);

        pixel_buf += pixel_row_bytes;
        if (alpha_buf != NULL)
        {
            alpha_buf += alpha_row_bytes;
        }
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

    if (!egui_image_qoi_prepare_state())
    {
        return;
    }

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
    int use_fast_copy = (draw_info->data_type == EGUI_IMAGE_DATA_TYPE_RGB565 && !has_alpha);
    int use_fast_alpha8 = (draw_info->data_type == EGUI_IMAGE_DATA_TYPE_RGB565 && has_alpha && alpha_type == EGUI_IMAGE_ALPHA_TYPE_8);
    egui_color_int_t *fast_dst_row = NULL;
    egui_dim_t fast_dst_stride = 0;
    egui_canvas_t *masked_canvas = NULL;
    egui_color_int_t *masked_dst_row = NULL;
    egui_dim_t masked_dst_stride = 0;
    int use_masked_opaque = 0;
    int use_masked_alpha8 = 0;
    int use_direct_fast_copy = 0;
    int use_direct_fast_alpha8 = 0;
    int use_direct_masked_alpha8 = 0;
#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
    int use_row_cache = 0;
    int use_tail_row_cache = 0;
    uint16_t cache_col_start = 0;
    uint16_t cache_col_count = 0;
    uint8_t *row_pixel_scratch = NULL;
    uint8_t *row_alpha_scratch = NULL;
#endif

    if (!egui_image_decode_get_horizontal_clip(x, draw_info->width, &screen_x_start, &img_col_start, &count))
    {
        return;
    }

    if ((use_fast_copy || use_fast_alpha8) && !egui_image_decode_get_fast_rgb565_dst(screen_x_start, y + img_y_start, &fast_dst_row, &fast_dst_stride))
    {
        fast_dst_row = NULL;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
    {
        egui_canvas_t *canvas = egui_canvas_get_canvas();

        if (fast_dst_row == NULL && canvas != NULL && canvas->mask != NULL && draw_info->data_type == EGUI_IMAGE_DATA_TYPE_RGB565)
        {
            use_masked_opaque = !has_alpha;
            use_masked_alpha8 = has_alpha && alpha_type == EGUI_IMAGE_ALPHA_TYPE_8;

            if ((use_masked_opaque || use_masked_alpha8) &&
                egui_image_decode_get_rgb565_dst(screen_x_start, y + img_y_start, &masked_dst_row, &masked_dst_stride))
            {
                masked_canvas = canvas;
            }
            else
            {
                use_masked_opaque = 0;
                use_masked_alpha8 = 0;
            }
        }
    }
#endif

#if EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES > 0
    if (egui_image_decode_persistent_cache_is_hit((const void *)info))
    {
        egui_image_qoi_blend_persistent_cached_rows(draw_info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end, fast_dst_row, fast_dst_stride,
                                                    masked_canvas, masked_dst_row, masked_dst_stride, use_fast_copy, use_fast_alpha8, use_masked_opaque,
                                                    use_masked_alpha8);
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
        egui_image_qoi_blend_persistent_cached_rows(draw_info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end, fast_dst_row, fast_dst_stride,
                                                    masked_canvas, masked_dst_row, masked_dst_stride, use_fast_copy, use_fast_alpha8, use_masked_opaque,
                                                    use_masked_alpha8);
        return;
    }
#endif

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
    if (egui_image_decode_cache_is_full_image_hit((const void *)info))
    {
        egui_image_qoi_blend_cached_rows(info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end, 0, fast_dst_row, fast_dst_stride, masked_canvas,
                                         masked_dst_row, masked_dst_stride, use_fast_copy, use_fast_alpha8, use_masked_opaque, use_masked_alpha8);
        return;
    }

    if (egui_image_decode_cache_can_hold_full_image(draw_info->width, draw_info->height, pixel_size, has_alpha ? draw_info->width : 0) &&
        egui_image_decode_cache_prepare_rows(draw_info->width, draw_info->height, pixel_size, has_alpha ? draw_info->width : 0))
    {
        egui_image_qoi_reset_state(info);

        for (egui_dim_t row = 0; row < draw_info->height; row++)
        {
            uint8_t *pixel_buf = egui_image_decode_cache_pixel_row((uint16_t)row, draw_info->width, pixel_size);
            uint8_t *alpha_buf = has_alpha ? egui_image_decode_cache_alpha_row((uint16_t)row, draw_info->width) : NULL;

            egui_image_qoi_decode_row(draw_info, pixel_buf, alpha_buf);
            qoi_state.current_row++;
        }

        egui_image_decode_cache_set_full_image((const void *)info, draw_info->height, draw_info->width);
        egui_image_qoi_blend_cached_rows(info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end, 0, fast_dst_row, fast_dst_stride, masked_canvas,
                                         masked_dst_row, masked_dst_stride, use_fast_copy, use_fast_alpha8, use_masked_opaque, use_masked_alpha8);
        return;
    }

    /* Row-band cache: check if this row band is already cached */
    if (egui_image_decode_cache_is_row_band_hit((const void *)info, (uint16_t)img_y_start, (uint16_t)img_col_start, (uint16_t)count))
    {
        /* Cache hit — blend directly from cached rows without decoding */
        egui_image_qoi_blend_cached_rows(draw_info, y, screen_x_start, img_col_start, count, img_y_start, img_y_end, img_y_start, fast_dst_row, fast_dst_stride,
                                         masked_canvas, masked_dst_row, masked_dst_stride, use_fast_copy, use_fast_alpha8, use_masked_opaque,
                                         use_masked_alpha8);
        return;
    }
    if (egui_image_qoi_can_use_tail_row_cache(draw_info, img_col_start, count))
    {
        cache_col_start = (uint16_t)(img_col_start + count);
        cache_col_count = egui_image_decode_limit_tail_cache_cols((uint16_t)(draw_info->width - cache_col_start));

        if (cache_col_count != 0 &&
            egui_image_decode_cache_prepare_rows(cache_col_count, (uint16_t)(img_y_end - img_y_start), pixel_size, has_alpha ? cache_col_count : 0))
        {
            if (fast_dst_row != NULL && use_fast_copy)
            {
                use_row_cache = 1;
                use_tail_row_cache = 1;
                use_direct_fast_copy = 1;
            }
            else if (fast_dst_row != NULL && use_fast_alpha8)
            {
                use_row_cache = 1;
                use_tail_row_cache = 1;
                use_direct_fast_alpha8 = 1;
            }
            else if (masked_canvas != NULL && masked_dst_row != NULL && use_masked_alpha8)
            {
                if (cache_col_count >= (uint16_t)count)
                {
                    use_row_cache = 1;
                    use_tail_row_cache = 1;
                    use_direct_masked_alpha8 = 1;
                }
                else
                {
                    /* The masked visible-span scratch still follows the clipped tile width,
                     * so allocate it on heap only when the tail-row heap buffer is too small to borrow. */
                    row_pixel_scratch = (uint8_t *)egui_malloc((int)((uint32_t)count * pixel_size));
                    row_alpha_scratch = (uint8_t *)egui_malloc((int)count);

                    if (row_pixel_scratch != NULL && row_alpha_scratch != NULL)
                    {
                        use_row_cache = 1;
                        use_tail_row_cache = 1;
                        use_direct_masked_alpha8 = 1;
                    }
                    else
                    {
                        if (row_alpha_scratch != NULL)
                        {
                            egui_free(row_alpha_scratch);
                            row_alpha_scratch = NULL;
                        }
                        if (row_pixel_scratch != NULL)
                        {
                            egui_free(row_pixel_scratch);
                            row_pixel_scratch = NULL;
                        }
                        cache_col_start = 0;
                        cache_col_count = 0;
                    }
                }
            }
            else
            {
                row_pixel_scratch = (uint8_t *)egui_malloc((int)((uint32_t)count * pixel_size));
                if (has_alpha)
                {
                    row_alpha_scratch = (uint8_t *)egui_malloc((int)count);
                }

                if (row_pixel_scratch != NULL && (!has_alpha || row_alpha_scratch != NULL))
                {
                    use_row_cache = 1;
                    use_tail_row_cache = 1;
                }
                else
                {
                    if (row_alpha_scratch != NULL)
                    {
                        egui_free(row_alpha_scratch);
                        row_alpha_scratch = NULL;
                    }
                    if (row_pixel_scratch != NULL)
                    {
                        egui_free(row_pixel_scratch);
                        row_pixel_scratch = NULL;
                    }
                    cache_col_start = 0;
                    cache_col_count = 0;
                }
            }
        }
        else
        {
            cache_col_start = 0;
            cache_col_count = 0;
        }
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
    const uint8_t *opaque_alpha_row = NULL;
    for (egui_dim_t row = img_y_start; row < img_y_end; row++)
    {
        egui_dim_t blend_img_col_start = img_col_start;
#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
        uint8_t *pixel_buf;
        uint8_t *alpha_buf;

        if (use_row_cache && use_tail_row_cache && !use_direct_fast_copy && !use_direct_fast_alpha8 && !use_direct_masked_alpha8)
        {
            pixel_buf = row_pixel_scratch;
            alpha_buf = has_alpha ? row_alpha_scratch : NULL;
        }
        else if (use_row_cache)
        {
            uint16_t row_in_band = (uint16_t)row - (uint16_t)img_y_start;
            pixel_buf = egui_image_decode_cache_pixel_row(row_in_band, cache_col_count, pixel_size);
            alpha_buf = has_alpha ? egui_image_decode_cache_alpha_row(row_in_band, cache_col_count) : NULL;
        }
        else
        {
            pixel_buf = egui_image_decode_get_row_pixel_buf(pixel_size);
            alpha_buf = has_alpha ? egui_image_decode_get_row_alpha_scratch(alpha_row_bytes) : NULL;
        }
#else
        uint8_t *pixel_buf = egui_image_decode_get_row_pixel_buf(pixel_size);
        uint8_t *alpha_buf = has_alpha ? egui_image_decode_get_row_alpha_scratch(alpha_row_bytes) : NULL;
#endif
        if (!use_direct_fast_copy && !use_direct_fast_alpha8 && !use_direct_masked_alpha8 && (pixel_buf == NULL || (has_alpha && alpha_buf == NULL)))
        {
            goto cleanup;
        }
#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
        if (use_row_cache && use_tail_row_cache)
        {
            uint16_t row_in_band = (uint16_t)row - (uint16_t)img_y_start;
            uint8_t *cache_pixel_row = egui_image_decode_cache_pixel_row(row_in_band, cache_col_count, pixel_size);
            uint8_t *cache_alpha_row = has_alpha ? egui_image_decode_cache_alpha_row(row_in_band, cache_col_count) : NULL;

            if (use_direct_fast_copy)
            {
                egui_image_qoi_decode_row_rgb565_split(draw_info, (uint8_t *)fast_dst_row, NULL, (uint16_t)img_col_start, (uint16_t)count, cache_pixel_row,
                                                       NULL, cache_col_start, cache_col_count);
            }
            else if (use_direct_fast_alpha8)
            {
                egui_image_qoi_decode_row_rgb565_alpha8_blend_split(draw_info, fast_dst_row, (uint16_t)img_col_start, (uint16_t)count, cache_pixel_row,
                                                                    cache_alpha_row, cache_col_start, cache_col_count);
            }
            else if (use_direct_masked_alpha8)
            {
                uint8_t *visible_pixel_row = cache_pixel_row;
                uint8_t *visible_alpha_row = cache_alpha_row;

                if (row_pixel_scratch != NULL && row_alpha_scratch != NULL)
                {
                    visible_pixel_row = row_pixel_scratch;
                    visible_alpha_row = row_alpha_scratch;
                }

                egui_image_qoi_decode_row_rgb565_alpha8_masked_split(draw_info, masked_canvas, masked_dst_row, screen_x_start, screen_y, masked_canvas->alpha,
                                                                     (uint16_t)img_col_start, (uint16_t)count, visible_pixel_row, visible_alpha_row,
                                                                     cache_pixel_row, cache_alpha_row, cache_col_start, cache_col_count);
            }
            else
            {
                egui_image_qoi_decode_row_rgb565_split(draw_info, pixel_buf, alpha_buf, (uint16_t)img_col_start, (uint16_t)count, cache_pixel_row,
                                                       cache_alpha_row, cache_col_start, cache_col_count);
                blend_img_col_start = 0;
            }
        }
        else
#endif
        {
            egui_image_qoi_decode_row(draw_info, pixel_buf, alpha_buf);
        }

        qoi_state.current_row++;

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
        if (use_direct_fast_copy || use_direct_fast_alpha8)
        {
            fast_dst_row += fast_dst_stride;
            screen_y++;
            continue;
        }
        if (use_direct_masked_alpha8)
        {
            masked_dst_row += masked_dst_stride;
            screen_y++;
            continue;
        }
#endif

        if (fast_dst_row != NULL)
        {
            const uint16_t *src_pixels = (const uint16_t *)pixel_buf + blend_img_col_start;

            if (use_fast_copy)
            {
                egui_api_memcpy(fast_dst_row, src_pixels, (size_t)count * sizeof(uint16_t));
            }
            else if (use_fast_alpha8)
            {
                egui_image_decode_blend_rgb565_alpha8_row_fast_path(fast_dst_row, src_pixels, alpha_buf + blend_img_col_start, count);
            }
            else
            {
                egui_image_decode_blend_row_clipped(screen_x_start, screen_y, blend_img_col_start, count, draw_info->data_type, alpha_type, has_alpha,
                                                    pixel_buf, alpha_buf);
            }

            fast_dst_row += fast_dst_stride;
        }
        else if (masked_canvas != NULL && masked_dst_row != NULL && use_masked_opaque)
        {
            const uint16_t *src_pixels = (const uint16_t *)pixel_buf + blend_img_col_start;
            int use_image_mask = (masked_canvas->mask != NULL && masked_canvas->mask->api->kind == EGUI_MASK_KIND_IMAGE);

            if (use_image_mask)
            {
                egui_image_qoi_blend_masked_rgb565_image_row(masked_canvas, masked_dst_row, src_pixels, NULL, count, screen_x_start, screen_y,
                                                             masked_canvas->alpha, &opaque_alpha_row);
            }
            else if (!egui_image_std_blend_rgb565_masked_row(masked_canvas, masked_dst_row, src_pixels, count, screen_x_start, screen_y, masked_canvas->alpha))
            {
                if (opaque_alpha_row == NULL)
                {
                    opaque_alpha_row = egui_image_decode_get_opaque_alpha_row(count);
                    if (opaque_alpha_row == NULL)
                    {
                        goto cleanup;
                    }
                }
                egui_image_std_blend_rgb565_alpha8_masked_row(masked_canvas, masked_dst_row, src_pixels, opaque_alpha_row, count, screen_x_start, screen_y,
                                                              masked_canvas->alpha);
            }
            masked_dst_row += masked_dst_stride;
        }
        else if (masked_canvas != NULL && masked_dst_row != NULL && use_masked_alpha8)
        {
            const uint16_t *src_pixels = (const uint16_t *)pixel_buf + blend_img_col_start;
            int use_image_mask = (masked_canvas->mask != NULL && masked_canvas->mask->api->kind == EGUI_MASK_KIND_IMAGE);

            if (use_image_mask)
            {
                egui_image_qoi_blend_masked_rgb565_image_row(masked_canvas, masked_dst_row, src_pixels, alpha_buf + blend_img_col_start, count, screen_x_start,
                                                             screen_y, masked_canvas->alpha, NULL);
            }
            else
            {
                egui_image_std_blend_rgb565_alpha8_masked_row(masked_canvas, masked_dst_row, src_pixels, alpha_buf + blend_img_col_start, count, screen_x_start,
                                                              screen_y, masked_canvas->alpha);
            }
            masked_dst_row += masked_dst_stride;
        }
        else
        {
            egui_image_decode_blend_row_clipped(screen_x_start, screen_y, blend_img_col_start, count, draw_info->data_type, alpha_type, has_alpha, pixel_buf,
                                                alpha_buf);
        }
        screen_y++;
    }

#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
    /* Mark this row band as cached for subsequent horizontal tiles */
    if (use_row_cache)
    {
        egui_image_decode_cache_set_row_band((const void *)info, (uint16_t)img_y_start, (uint16_t)(img_y_end - img_y_start), cache_col_start, cache_col_count);
    }
#endif

cleanup:
#if EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
    if (row_alpha_scratch != NULL)
    {
        egui_free(row_alpha_scratch);
    }
    if (row_pixel_scratch != NULL)
    {
        egui_free(row_pixel_scratch);
    }
#endif
}

static void egui_image_qoi_draw_image_resize(const egui_image_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
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

void egui_image_qoi_release_frame_cache(void)
{
    if (qoi_state_ptr != NULL)
    {
        egui_free(qoi_state_ptr);
        qoi_state_ptr = NULL;
    }
}

#endif /* EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE */
