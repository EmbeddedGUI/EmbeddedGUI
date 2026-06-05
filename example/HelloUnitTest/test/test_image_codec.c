#include <stdint.h>
#include <string.h>

#include "egui.h"
#include "image/egui_image_decode_utils.h"
#include "image/egui_image_qoi.h"
#include "image/egui_image_rle.h"
#include "test/egui_test.h"
#include "test_image_codec.h"
#include "uicode_disp0.h"

#if EGUI_CONFIG_FUNCTION_IMAGE_CODEC_QOI && EGUI_CONFIG_FUNCTION_IMAGE_CODEC_RLE

#define TEST_IMAGE_CODEC_SCENE_COUNT     128
#define TEST_IMAGE_CODEC_PFB_SCENE_COUNT 32
#define TEST_IMAGE_CODEC_PFB_SCENE_BASE  (TEST_IMAGE_CODEC_SCENE_COUNT - TEST_IMAGE_CODEC_PFB_SCENE_COUNT)
#define TEST_IMAGE_CODEC_MAX_PASSES      2
#define TEST_IMAGE_CODEC_GUARD_WORDS     4096
#define TEST_IMAGE_CODEC_GUARD_VALUE     0x5a5aU

#define TEST_QOI_OP_INDEX 0x00
#define TEST_QOI_OP_DIFF  0x40
#define TEST_QOI_OP_LUMA  0x80
#define TEST_QOI_OP_RUN   0xC0
#define TEST_QOI_OP_RGB   0xFE
#define TEST_QOI_OP_RGBA  0xFF

typedef struct test_image_codec_pixel
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    uint16_t rgb565;
} test_image_codec_pixel_t;

typedef struct test_image_codec_stream
{
    uint8_t *data;
    uint32_t size;
    uint32_t capacity;
} test_image_codec_stream_t;

typedef struct test_image_codec_pass
{
    egui_region_t clip;
} test_image_codec_pass_t;

typedef struct test_image_codec_scene
{
    uint16_t index;
    uint16_t width;
    uint16_t height;
    uint16_t canvas_width;
    uint16_t canvas_height;
    int16_t image_x;
    int16_t image_y;
    uint8_t channels;
    uint8_t alpha_type;
    uint8_t pattern;
    uint8_t pass_count;
    test_image_codec_pass_t passes[TEST_IMAGE_CODEC_MAX_PASSES];
} test_image_codec_scene_t;

static egui_core_t *test_image_codec_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static uint8_t test_image_codec_qoi_hash(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (uint8_t)(((uint16_t)r * 3U + (uint16_t)g * 5U + (uint16_t)b * 7U + (uint16_t)a * 11U) & 63U);
}

static int test_image_codec_same_pixel(const test_image_codec_pixel_t *a, const test_image_codec_pixel_t *b, uint8_t channels)
{
    if (a->r != b->r || a->g != b->g || a->b != b->b)
    {
        return 0;
    }
    return channels == 3 || a->a == b->a;
}

static uint16_t test_image_codec_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    egui_color_t color = EGUI_COLOR_MAKE(r, g, b);

    return color.full;
}

static int test_image_codec_stream_init(egui_core_t *core, test_image_codec_stream_t *stream, uint32_t capacity)
{
    stream->data = (uint8_t *)egui_malloc(core, (int)capacity);
    stream->size = 0;
    stream->capacity = capacity;
    return stream->data != NULL;
}

static void test_image_codec_stream_deinit(egui_core_t *core, test_image_codec_stream_t *stream)
{
    if (stream->data != NULL)
    {
        egui_free(core, stream->data);
    }
    stream->data = NULL;
    stream->size = 0;
    stream->capacity = 0;
}

static int test_image_codec_stream_put_u8(test_image_codec_stream_t *stream, uint8_t value)
{
    if (stream->size >= stream->capacity)
    {
        return 0;
    }
    stream->data[stream->size++] = value;
    return 1;
}

static int test_image_codec_stream_put_bytes(test_image_codec_stream_t *stream, const uint8_t *data, uint32_t size)
{
    if (size > stream->capacity || stream->size > stream->capacity - size)
    {
        return 0;
    }
    memcpy(&stream->data[stream->size], data, size);
    stream->size += size;
    return 1;
}

static int test_image_codec_qoi_flush_run(test_image_codec_stream_t *stream, uint8_t *run)
{
    while (*run > 0)
    {
        uint8_t chunk = *run;

        if (chunk > 62U)
        {
            chunk = 62U;
        }
        if (!test_image_codec_stream_put_u8(stream, (uint8_t)(TEST_QOI_OP_RUN | (chunk - 1U))))
        {
            return 0;
        }
        *run = (uint8_t)(*run - chunk);
    }
    return 1;
}

static int test_image_codec_qoi_encode(egui_core_t *core, const test_image_codec_pixel_t *pixels, uint32_t pixel_count, uint8_t channels,
                                       test_image_codec_stream_t *stream)
{
    test_image_codec_pixel_t index[64];
    test_image_codec_pixel_t prev = {.r = 0, .g = 0, .b = 0, .a = 255, .rgb565 = 0};
    uint8_t run = 0;

    memset(index, 0, sizeof(index));
    if (!test_image_codec_stream_init(core, stream, pixel_count * 5U + 16U))
    {
        return 0;
    }

    for (uint32_t i = 0; i < pixel_count; i++)
    {
        test_image_codec_pixel_t px = pixels[i];
        uint8_t hash;

        if (channels == 3)
        {
            px.a = 255;
        }

        if (test_image_codec_same_pixel(&px, &prev, channels))
        {
            run++;
            if (run == 62U && !test_image_codec_qoi_flush_run(stream, &run))
            {
                return 0;
            }
            continue;
        }

        if (!test_image_codec_qoi_flush_run(stream, &run))
        {
            return 0;
        }

        hash = test_image_codec_qoi_hash(px.r, px.g, px.b, px.a);
        if (test_image_codec_same_pixel(&index[hash], &px, channels))
        {
            if (!test_image_codec_stream_put_u8(stream, hash))
            {
                return 0;
            }
        }
        else
        {
            int dr = (int)px.r - (int)prev.r;
            int dg = (int)px.g - (int)prev.g;
            int db = (int)px.b - (int)prev.b;

            index[hash] = px;
            if (px.a == prev.a && dr >= -2 && dr <= 1 && dg >= -2 && dg <= 1 && db >= -2 && db <= 1)
            {
                uint8_t op = (uint8_t)(TEST_QOI_OP_DIFF | ((dr + 2) << 4) | ((dg + 2) << 2) | (db + 2));

                if (!test_image_codec_stream_put_u8(stream, op))
                {
                    return 0;
                }
            }
            else if (px.a == prev.a && dg >= -32 && dg <= 31 && (dr - dg) >= -8 && (dr - dg) <= 7 && (db - dg) >= -8 && (db - dg) <= 7)
            {
                if (!test_image_codec_stream_put_u8(stream, (uint8_t)(TEST_QOI_OP_LUMA | (dg + 32))) ||
                    !test_image_codec_stream_put_u8(stream, (uint8_t)(((dr - dg + 8) << 4) | (db - dg + 8))))
                {
                    return 0;
                }
            }
            else if (channels == 4 && px.a != prev.a)
            {
                if (!test_image_codec_stream_put_u8(stream, TEST_QOI_OP_RGBA) || !test_image_codec_stream_put_u8(stream, px.r) ||
                    !test_image_codec_stream_put_u8(stream, px.g) || !test_image_codec_stream_put_u8(stream, px.b) ||
                    !test_image_codec_stream_put_u8(stream, px.a))
                {
                    return 0;
                }
            }
            else
            {
                if (!test_image_codec_stream_put_u8(stream, TEST_QOI_OP_RGB) || !test_image_codec_stream_put_u8(stream, px.r) ||
                    !test_image_codec_stream_put_u8(stream, px.g) || !test_image_codec_stream_put_u8(stream, px.b))
                {
                    return 0;
                }
            }
        }
        prev = px;
    }

    return test_image_codec_qoi_flush_run(stream, &run);
}

static int test_image_codec_units_equal(const uint8_t *a, const uint8_t *b, uint8_t unit_size)
{
    for (uint8_t i = 0; i < unit_size; i++)
    {
        if (a[i] != b[i])
        {
            return 0;
        }
    }
    return 1;
}

static uint16_t test_image_codec_count_repeat(const uint8_t *row, uint16_t row_units, uint16_t start, uint8_t unit_size)
{
    uint16_t count = 1;
    const uint8_t *unit = &row[(uint32_t)start * unit_size];

    while ((uint16_t)(start + count) < row_units && count < 127U && test_image_codec_units_equal(unit, &row[(uint32_t)(start + count) * unit_size], unit_size))
    {
        count++;
    }
    return count;
}

static int test_image_codec_rle_encode_row(test_image_codec_stream_t *stream, const uint8_t *row, uint16_t row_units, uint8_t unit_size)
{
    uint16_t col = 0;

    while (col < row_units)
    {
        uint16_t repeat = test_image_codec_count_repeat(row, row_units, col, unit_size);

        if (repeat >= 2U)
        {
            if (!test_image_codec_stream_put_u8(stream, (uint8_t)repeat) ||
                !test_image_codec_stream_put_bytes(stream, &row[(uint32_t)col * unit_size], unit_size))
            {
                return 0;
            }
            col = (uint16_t)(col + repeat);
        }
        else
        {
            uint16_t literal_start = col;
            uint16_t literal_count = 0;

            while (col < row_units && literal_count < 127U)
            {
                repeat = test_image_codec_count_repeat(row, row_units, col, unit_size);
                if (repeat >= 2U && literal_count > 0U)
                {
                    break;
                }
                col++;
                literal_count++;
                if (repeat >= 2U)
                {
                    break;
                }
            }

            if (!test_image_codec_stream_put_u8(stream, (uint8_t)(0x80U | literal_count)) ||
                !test_image_codec_stream_put_bytes(stream, &row[(uint32_t)literal_start * unit_size], (uint32_t)literal_count * unit_size))
            {
                return 0;
            }
        }
    }

    return 1;
}

static int test_image_codec_rle_encode(egui_core_t *core, const uint8_t *units, uint16_t row_units, uint16_t rows, uint8_t unit_size,
                                       test_image_codec_stream_t *stream)
{
    uint32_t unit_count = (uint32_t)row_units * rows;
    uint32_t capacity = unit_count * ((uint32_t)unit_size + 1U) + (uint32_t)rows + 16U;

    if (!test_image_codec_stream_init(core, stream, capacity))
    {
        return 0;
    }

    for (uint16_t row = 0; row < rows; row++)
    {
        const uint8_t *row_data = &units[(uint32_t)row * row_units * unit_size];

        if (!test_image_codec_rle_encode_row(stream, row_data, row_units, unit_size))
        {
            return 0;
        }
    }
    return 1;
}

static uint16_t test_image_codec_alpha_row_bytes(uint8_t alpha_type, uint16_t width)
{
    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_8:
        return width;
    case EGUI_IMAGE_ALPHA_TYPE_4:
        return (uint16_t)((width + 1U) >> 1);
    case EGUI_IMAGE_ALPHA_TYPE_2:
        return (uint16_t)((width + 3U) >> 2);
    case EGUI_IMAGE_ALPHA_TYPE_1:
    default:
        return (uint16_t)((width + 7U) >> 3);
    }
}

static uint8_t test_image_codec_alpha_from_level(uint8_t alpha_type, uint16_t level)
{
    static const uint8_t alpha8_values[] = {0, 1, 3, 4, 17, 64, 85, 127, 170, 200, 251, 252, 255};

    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_1:
        return (level & 1U) ? EGUI_ALPHA_100 : EGUI_ALPHA_0;
    case EGUI_IMAGE_ALPHA_TYPE_2:
        return egui_alpha_change_table_2[level & 0x03U];
    case EGUI_IMAGE_ALPHA_TYPE_4:
        return egui_alpha_change_table_4[level & 0x0FU];
    case EGUI_IMAGE_ALPHA_TYPE_8:
    default:
        return alpha8_values[level % EGUI_ARRAY_SIZE(alpha8_values)];
    }
}

static uint8_t test_image_codec_alpha_unit(uint8_t alpha_type, uint8_t alpha)
{
    switch (alpha_type)
    {
    case EGUI_IMAGE_ALPHA_TYPE_1:
        return alpha >= 128U ? 1U : 0U;
    case EGUI_IMAGE_ALPHA_TYPE_2:
        for (uint8_t i = 0; i < 4U; i++)
        {
            if (egui_alpha_change_table_2[i] == alpha)
            {
                return i;
            }
        }
        return 0;
    case EGUI_IMAGE_ALPHA_TYPE_4:
        for (uint8_t i = 0; i < 16U; i++)
        {
            if (egui_alpha_change_table_4[i] == alpha)
            {
                return i;
            }
        }
        return 0;
    case EGUI_IMAGE_ALPHA_TYPE_8:
    default:
        return alpha;
    }
}

static uint8_t test_image_codec_clamp_u8(uint16_t value)
{
    return (uint8_t)(value & 0xFFU);
}

static void test_image_codec_make_pixel(const test_image_codec_scene_t *scene, uint16_t x, uint16_t y, test_image_codec_pixel_t *pixel)
{
    uint16_t linear = (uint16_t)(y * scene->width + x);
    uint16_t seed = (uint16_t)(scene->index * 29U + 17U);
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    switch (scene->pattern)
    {
    case 0:
        r = test_image_codec_clamp_u8(seed + 31U);
        g = test_image_codec_clamp_u8(seed + 77U);
        b = test_image_codec_clamp_u8(seed + 141U);
        break;
    case 1:
        r = (uint8_t)(((x / 5U) & 1U) ? 224U : 24U);
        g = (uint8_t)(((x / 5U) & 1U) ? 32U : 180U);
        b = test_image_codec_clamp_u8(seed + ((x / 5U) * 19U));
        break;
    case 2:
        r = (uint8_t)(((y / 3U) & 1U) ? 40U : 210U);
        g = test_image_codec_clamp_u8(seed + ((y / 3U) * 23U));
        b = (uint8_t)(((y / 3U) & 1U) ? 210U : 40U);
        break;
    case 3:
        r = (uint8_t)(((x ^ y) & 1U) ? 0xF8U : 0x18U);
        g = (uint8_t)(((x ^ y) & 1U) ? 0x20U : 0xE0U);
        b = (uint8_t)(((x ^ y) & 1U) ? 0x10U : 0xF0U);
        break;
    case 4:
        r = test_image_codec_clamp_u8(37U + linear);
        g = test_image_codec_clamp_u8(91U + linear);
        b = test_image_codec_clamp_u8(143U + linear);
        break;
    case 5:
        r = test_image_codec_clamp_u8(19U + linear * 6U);
        g = test_image_codec_clamp_u8(61U + linear * 5U);
        b = test_image_codec_clamp_u8(113U + linear * 4U);
        break;
    case 6:
    {
        static const uint8_t palette[8][3] = {
                {0, 0, 0}, {255, 255, 255}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0}, {0, 255, 255}, {255, 0, 255},
        };
        uint8_t index = (uint8_t)((x * 3U + y * 5U + scene->index) & 7U);

        r = palette[index][0];
        g = palette[index][1];
        b = palette[index][2];
        break;
    }
    case 7:
    {
        uint32_t value = (uint32_t)seed * 1103515245UL + (uint32_t)linear * 2654435761UL + 12345UL;

        r = (uint8_t)(value >> 3);
        g = (uint8_t)(value >> 11);
        b = (uint8_t)(value >> 19);
        break;
    }
    case 8:
        r = test_image_codec_clamp_u8(40U + x * 9U);
        g = test_image_codec_clamp_u8(70U + y * 13U);
        b = test_image_codec_clamp_u8(120U + linear * 3U);
        break;
    case 9:
        r = (uint8_t)((x < scene->width / 2U) ? 0x08U : 0xE8U);
        g = (uint8_t)((y < scene->height / 2U) ? 0x18U : 0xD8U);
        b = (uint8_t)(((x + y) & 3U) == 0U ? 0xF0U : 0x20U);
        break;
    case 10:
        r = (uint8_t)((x == 0U || y == 0U || x + 1U == scene->width || y + 1U == scene->height) ? 0xFFU : 0x20U);
        g = test_image_codec_clamp_u8(20U + x * 11U + y * 7U);
        b = (uint8_t)((x == y || x + y + 1U == scene->width) ? 0xFFU : 0x30U);
        break;
    case 11:
    default:
        if (linear < (uint16_t)(scene->width * scene->height / 2U))
        {
            r = test_image_codec_clamp_u8(seed + 5U);
            g = test_image_codec_clamp_u8(seed + 35U);
            b = test_image_codec_clamp_u8(seed + 65U);
        }
        else
        {
            r = test_image_codec_clamp_u8(seed + 95U);
            g = test_image_codec_clamp_u8(seed + 125U);
            b = test_image_codec_clamp_u8(seed + 155U);
        }
        break;
    }

    pixel->r = r;
    pixel->g = g;
    pixel->b = b;
    pixel->a = EGUI_ALPHA_100;
    if (scene->channels == 4U)
    {
        uint16_t level = (uint16_t)(linear + scene->index + x * 3U + y * 5U);

        if (scene->pattern == 0U || scene->pattern == 11U)
        {
            level = (uint16_t)(scene->index + y);
        }
        else if (scene->pattern == 9U)
        {
            level = (uint16_t)((x / 3U) + (y / 2U) * 3U + scene->index);
        }
        pixel->a = test_image_codec_alpha_from_level(scene->alpha_type, level);
    }
    pixel->rgb565 = test_image_codec_rgb565(pixel->r, pixel->g, pixel->b);
}

static void test_image_codec_make_pixels(const test_image_codec_scene_t *scene, test_image_codec_pixel_t *pixels)
{
    for (uint16_t y = 0; y < scene->height; y++)
    {
        for (uint16_t x = 0; x < scene->width; x++)
        {
            test_image_codec_make_pixel(scene, x, y, &pixels[(uint32_t)y * scene->width + x]);
        }
    }
}

static void test_image_codec_make_scene(uint16_t index, test_image_codec_scene_t *scene)
{
    static const uint16_t widths[] = {1, 2, 3, 4, 5, 7, 8, 13, 16, 23, 31, 40, 63, 79, 127, 181};
    static const uint16_t heights[] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 23, 31, 43, 6, 11};
    static const uint16_t pfb_widths[] = {17, 31, 63, 79, 127, 181, 95, 151};
    static const uint16_t pfb_heights[] = {9, 13, 17, 23, 31, 43, 73, 89};
    static const int16_t offsets_x[] = {-5, -1, 0, 2, 6, 11};
    static const int16_t offsets_y[] = {-4, -1, 0, 3, 7};
    static const uint8_t alpha_types[] = {EGUI_IMAGE_ALPHA_TYPE_8, EGUI_IMAGE_ALPHA_TYPE_4, EGUI_IMAGE_ALPHA_TYPE_2, EGUI_IMAGE_ALPHA_TYPE_1};
    uint8_t pass_mode;
    int16_t strip_x;
    int16_t strip_y;
    int16_t center_x;
    int16_t center_y;
    int16_t center_w;
    int16_t center_h;

    memset(scene, 0, sizeof(*scene));
    scene->index = index;
    scene->width = widths[index % EGUI_ARRAY_SIZE(widths)];
    scene->height = heights[(index / EGUI_ARRAY_SIZE(widths)) % EGUI_ARRAY_SIZE(heights)];
    scene->canvas_width = (uint16_t)(scene->width + 24U);
    scene->canvas_height = (uint16_t)(scene->height + 20U);
    scene->image_x = offsets_x[(index / 7U) % EGUI_ARRAY_SIZE(offsets_x)];
    scene->image_y = offsets_y[(index / 11U) % EGUI_ARRAY_SIZE(offsets_y)];
    scene->channels = (index % 4U == 0U) ? 3U : 4U;
    scene->alpha_type = alpha_types[(index / 4U) % EGUI_ARRAY_SIZE(alpha_types)];
    scene->pattern = (uint8_t)(index % 12U);
    pass_mode = (uint8_t)((index / 12U) % 8U);

    if (index >= TEST_IMAGE_CODEC_PFB_SCENE_BASE)
    {
        uint16_t pfb_index = (uint16_t)(index - TEST_IMAGE_CODEC_PFB_SCENE_BASE);

        scene->width = pfb_widths[pfb_index % EGUI_ARRAY_SIZE(pfb_widths)];
        scene->height = pfb_heights[(pfb_index / 4U) % EGUI_ARRAY_SIZE(pfb_heights)];
        scene->canvas_width = (uint16_t)(scene->width + 29U);
        scene->canvas_height = (uint16_t)(scene->height + 23U);
        scene->image_x = offsets_x[(pfb_index / 3U) % EGUI_ARRAY_SIZE(offsets_x)];
        scene->image_y = offsets_y[(pfb_index / 5U) % EGUI_ARRAY_SIZE(offsets_y)];
        scene->channels = (pfb_index % 3U == 0U) ? 3U : 4U;
        scene->alpha_type = alpha_types[pfb_index % EGUI_ARRAY_SIZE(alpha_types)];
        scene->pattern = (uint8_t)((pfb_index * 5U + 3U) % 12U);
        pass_mode = (uint8_t)((pfb_index / 4U) % 8U);
    }

    if (pass_mode == 7U)
    {
        scene->image_x = (int16_t)(scene->canvas_width + 3U);
        scene->image_y = (int16_t)(scene->canvas_height + 3U);
    }

    strip_x = scene->image_x + (int16_t)(scene->width / 2U);
    if (strip_x < 0)
    {
        strip_x = 0;
    }
    if (strip_x >= (int16_t)scene->canvas_width)
    {
        strip_x = (int16_t)(scene->canvas_width - 1U);
    }

    strip_y = scene->image_y + (int16_t)(scene->height / 2U);
    if (strip_y < 0)
    {
        strip_y = 0;
    }
    if (strip_y >= (int16_t)scene->canvas_height)
    {
        strip_y = (int16_t)(scene->canvas_height - 1U);
    }

    center_x = scene->image_x + (int16_t)(scene->width / 3U);
    center_y = scene->image_y + (int16_t)(scene->height / 3U);
    center_w = (int16_t)EGUI_MAX(1, scene->width / 2U);
    center_h = (int16_t)EGUI_MAX(1, scene->height / 2U);
    if (center_x < 0)
    {
        center_x = 0;
    }
    if (center_y < 0)
    {
        center_y = 0;
    }
    if (center_x + center_w > (int16_t)scene->canvas_width)
    {
        center_w = (int16_t)scene->canvas_width - center_x;
    }
    if (center_y + center_h > (int16_t)scene->canvas_height)
    {
        center_h = (int16_t)scene->canvas_height - center_y;
    }

    switch (pass_mode)
    {
    case 1:
        scene->pass_count = 1;
        egui_region_init(&scene->passes[0].clip, strip_x, 0, 1, scene->canvas_height);
        break;
    case 2:
        scene->pass_count = 1;
        egui_region_init(&scene->passes[0].clip, 0, strip_y, scene->canvas_width, 1);
        break;
    case 3:
        scene->pass_count = 2;
        egui_region_init(&scene->passes[0].clip, 0, 0, scene->canvas_width / 2, scene->canvas_height);
        egui_region_init(&scene->passes[1].clip, scene->canvas_width / 2, 0, scene->canvas_width - scene->canvas_width / 2, scene->canvas_height);
        break;
    case 4:
        scene->pass_count = 2;
        egui_region_init(&scene->passes[0].clip, 0, scene->canvas_height / 2, scene->canvas_width, scene->canvas_height - scene->canvas_height / 2);
        egui_region_init(&scene->passes[1].clip, 0, 0, scene->canvas_width, scene->canvas_height / 2);
        break;
    case 5:
        scene->pass_count = 1;
        egui_region_init(&scene->passes[0].clip, center_x, center_y, center_w, center_h);
        break;
    case 6:
        scene->pass_count = 2;
        egui_region_init(&scene->passes[0].clip, center_x, center_y, center_w, center_h);
        egui_region_init(&scene->passes[1].clip, 0, 0, scene->canvas_width, scene->canvas_height);
        break;
    case 7:
        scene->pass_count = 1;
        egui_region_init(&scene->passes[0].clip, 0, 0, EGUI_MIN(3, scene->canvas_width), EGUI_MIN(3, scene->canvas_height));
        break;
    case 0:
    default:
        scene->pass_count = 1;
        egui_region_init(&scene->passes[0].clip, 0, 0, scene->canvas_width, scene->canvas_height);
        break;
    }
}

static uint16_t test_image_codec_background_at(uint16_t scene_index, uint16_t x, uint16_t y)
{
    uint8_t r = test_image_codec_clamp_u8(13U + scene_index * 3U + x * 5U);
    uint8_t g = test_image_codec_clamp_u8(41U + scene_index * 7U + y * 9U);
    uint8_t b = test_image_codec_clamp_u8(83U + scene_index * 11U + x * 3U + y * 2U);

    return test_image_codec_rgb565(r, g, b);
}

static void test_image_codec_fill_background(const test_image_codec_scene_t *scene, egui_color_int_t *buffer)
{
    for (uint16_t y = 0; y < scene->canvas_height; y++)
    {
        for (uint16_t x = 0; x < scene->canvas_width; x++)
        {
            buffer[(uint32_t)y * scene->canvas_width + x] = test_image_codec_background_at(scene->index, x, y);
        }
    }
}

static void test_image_codec_apply_expected(const test_image_codec_scene_t *scene, const test_image_codec_pixel_t *pixels, egui_color_int_t *expected)
{
    for (uint8_t pass_index = 0; pass_index < scene->pass_count; pass_index++)
    {
        const egui_region_t *clip = &scene->passes[pass_index].clip;
        int16_t y_end = (int16_t)(clip->location.y + clip->size.height);
        int16_t x_end = (int16_t)(clip->location.x + clip->size.width);

        for (int16_t screen_y = clip->location.y; screen_y < y_end; screen_y++)
        {
            for (int16_t screen_x = clip->location.x; screen_x < x_end; screen_x++)
            {
                int16_t image_x = (int16_t)(screen_x - scene->image_x);
                int16_t image_y = (int16_t)(screen_y - scene->image_y);

                if (screen_x < 0 || screen_y < 0 || screen_x >= (int16_t)scene->canvas_width || screen_y >= (int16_t)scene->canvas_height)
                {
                    continue;
                }
                if (image_x >= 0 && image_y >= 0 && image_x < (int16_t)scene->width && image_y < (int16_t)scene->height)
                {
                    const test_image_codec_pixel_t *src = &pixels[(uint32_t)image_y * scene->width + image_x];
                    uint32_t offset = (uint32_t)screen_y * scene->canvas_width + screen_x;
                    egui_color_t bg;
                    egui_color_t fg;
                    egui_color_t out;

                    bg.full = expected[offset];
                    fg.full = src->rgb565;
                    out = egui_rgb_mix(bg, fg, scene->channels == 4U ? src->a : EGUI_ALPHA_100);
                    expected[offset] = out.full;
                }
            }
        }
    }
}

static int test_image_codec_make_rle_pixel_units(egui_core_t *core, const test_image_codec_scene_t *scene, const test_image_codec_pixel_t *pixels,
                                                 uint8_t **out_units)
{
    uint32_t pixel_count = (uint32_t)scene->width * scene->height;
    uint8_t *units = (uint8_t *)egui_malloc(core, (int)(pixel_count * 2U));

    if (units == NULL)
    {
        return 0;
    }

    for (uint32_t i = 0; i < pixel_count; i++)
    {
        units[i * 2U + 0U] = (uint8_t)(pixels[i].rgb565 & 0xFFU);
        units[i * 2U + 1U] = (uint8_t)(pixels[i].rgb565 >> 8);
    }
    *out_units = units;
    return 1;
}

static int test_image_codec_make_rle_alpha_units(egui_core_t *core, const test_image_codec_scene_t *scene, const test_image_codec_pixel_t *pixels,
                                                 uint16_t alpha_row_bytes, uint8_t **out_units)
{
    uint8_t *units = (uint8_t *)egui_malloc(core, (int)((uint32_t)alpha_row_bytes * scene->height));

    if (units == NULL)
    {
        return 0;
    }
    memset(units, 0, (size_t)alpha_row_bytes * scene->height);

    for (uint16_t y = 0; y < scene->height; y++)
    {
        uint8_t *row = &units[(uint32_t)y * alpha_row_bytes];

        for (uint16_t x = 0; x < scene->width; x++)
        {
            const test_image_codec_pixel_t *pixel = &pixels[(uint32_t)y * scene->width + x];
            uint8_t unit = test_image_codec_alpha_unit(scene->alpha_type, pixel->a);

            switch (scene->alpha_type)
            {
            case EGUI_IMAGE_ALPHA_TYPE_8:
                row[x] = unit;
                break;
            case EGUI_IMAGE_ALPHA_TYPE_4:
                row[x >> 1] |= (uint8_t)(unit << ((x & 1U) << 2));
                break;
            case EGUI_IMAGE_ALPHA_TYPE_2:
                row[x >> 2] |= (uint8_t)(unit << ((x & 3U) << 1));
                break;
            case EGUI_IMAGE_ALPHA_TYPE_1:
            default:
                row[x >> 3] |= (uint8_t)(unit << (x & 7U));
                break;
            }
        }
    }

    *out_units = units;
    return 1;
}

static void test_image_codec_setup_canvas(const test_image_codec_scene_t *scene, egui_canvas_t *canvas, egui_color_int_t *pfb)
{
    egui_region_t pfb_region;
    egui_region_t base_region;

    egui_region_init(&pfb_region, 0, 0, scene->canvas_width, scene->canvas_height);
    egui_region_init(&base_region, 0, 0, scene->canvas_width, scene->canvas_height);
    egui_canvas_init(canvas, test_image_codec_get_core(), pfb, &pfb_region);
    egui_canvas_clear_mask(canvas);
    egui_canvas_calc_work_region(canvas, &base_region);
}

static void test_image_codec_draw_passes(const test_image_codec_scene_t *scene, const egui_image_t *image, egui_color_int_t *pfb)
{
    egui_canvas_t canvas;

    test_image_codec_setup_canvas(scene, &canvas, pfb);
    for (uint8_t pass_index = 0; pass_index < scene->pass_count; pass_index++)
    {
        canvas.base_view_work_region = scene->passes[pass_index].clip;
        egui_canvas_draw_image(&canvas, image, scene->image_x, scene->image_y);
    }
}

static void test_image_codec_fill_guard(egui_color_int_t *raw, uint32_t pixel_count);
static int test_image_codec_check_guard(const test_image_codec_scene_t *scene, const char *codec_name, const egui_color_int_t *raw, uint32_t pixel_count);

static void test_image_codec_get_pfb_tile_size(const test_image_codec_scene_t *scene, uint16_t *tile_width, uint16_t *tile_height)
{
    static const uint16_t tile_widths[] = {1, 2, 3, 5, 7, 8, 11, 16, 29, 37};
    static const uint16_t tile_heights[] = {1, 2, 3, 4, 5, 7, 9, 13, 17};

    *tile_width = tile_widths[scene->index % EGUI_ARRAY_SIZE(tile_widths)];
    *tile_height = tile_heights[(scene->index / 5U) % EGUI_ARRAY_SIZE(tile_heights)];
    if (*tile_width > scene->canvas_width)
    {
        *tile_width = scene->canvas_width;
    }
    if (*tile_height > scene->canvas_height)
    {
        *tile_height = scene->canvas_height;
    }
}

static void test_image_codec_copy_canvas_to_pfb_tile(const egui_region_t *tile, const egui_color_int_t *canvas_buffer, uint16_t canvas_width,
                                                     egui_color_int_t *pfb)
{
    for (uint16_t y = 0; y < (uint16_t)tile->size.height; y++)
    {
        for (uint16_t x = 0; x < (uint16_t)tile->size.width; x++)
        {
            uint32_t src_offset = (uint32_t)(tile->location.y + y) * canvas_width + (uint16_t)(tile->location.x + x);

            pfb[(uint32_t)y * tile->size.width + x] = canvas_buffer[src_offset];
        }
    }
}

static void test_image_codec_copy_pfb_tile_to_canvas(const egui_region_t *tile, const egui_color_int_t *pfb, egui_color_int_t *canvas_buffer,
                                                     uint16_t canvas_width)
{
    for (uint16_t y = 0; y < (uint16_t)tile->size.height; y++)
    {
        for (uint16_t x = 0; x < (uint16_t)tile->size.width; x++)
        {
            uint32_t src_offset = (uint32_t)y * tile->size.width + x;
            uint32_t dst_offset = (uint32_t)(tile->location.y + y) * canvas_width + (uint16_t)(tile->location.x + x);

            canvas_buffer[dst_offset] = pfb[src_offset];
        }
    }
}

static void test_image_codec_draw_one_pfb_tile(const test_image_codec_scene_t *scene, const egui_image_t *image, const egui_region_t *base_region,
                                               const egui_region_t *tile, egui_color_int_t *pfb, egui_color_int_t *canvas_buffer)
{
    egui_canvas_t canvas;
    egui_region_t full_base_region;
    egui_region_t clipped_work_region;

    test_image_codec_copy_canvas_to_pfb_tile(tile, canvas_buffer, scene->canvas_width, pfb);
    egui_canvas_init(&canvas, test_image_codec_get_core(), pfb, (egui_region_t *)tile);
    egui_canvas_clear_mask(&canvas);
    egui_region_init(&full_base_region, 0, 0, scene->canvas_width, scene->canvas_height);
    egui_canvas_calc_work_region(&canvas, &full_base_region);
    egui_region_intersect(&canvas.base_view_work_region, base_region, &clipped_work_region);
    if (!egui_region_is_empty(&clipped_work_region))
    {
        canvas.base_view_work_region = clipped_work_region;
        egui_canvas_draw_image(&canvas, image, scene->image_x, scene->image_y);
    }
    test_image_codec_copy_pfb_tile_to_canvas(tile, pfb, canvas_buffer, scene->canvas_width);
}

static int test_image_codec_draw_pfb_tiles(const test_image_codec_scene_t *scene, const char *codec_name, const egui_image_t *image,
                                           egui_color_int_t *canvas_buffer)
{
    egui_core_t *core = test_image_codec_get_core();
    egui_color_int_t *pfb_raw = NULL;
    egui_color_int_t *pfb = NULL;
    uint16_t tile_width;
    uint16_t tile_height;
    uint32_t tile_pixel_count;
    uint16_t x_tiles;
    uint16_t y_tiles;
    uint8_t reverse_x;
    uint8_t reverse_y;
    int ok = 0;

    test_image_codec_get_pfb_tile_size(scene, &tile_width, &tile_height);
    tile_pixel_count = (uint32_t)tile_width * tile_height;
    pfb_raw = (egui_color_int_t *)egui_malloc(core, (int)((tile_pixel_count + TEST_IMAGE_CODEC_GUARD_WORDS * 2U) * sizeof(*pfb_raw)));
    if (pfb_raw == NULL)
    {
        egui_test_fail(__FILE__, __LINE__, "pfb_allocation", "expected non-NULL");
        return 0;
    }
    pfb = pfb_raw + TEST_IMAGE_CODEC_GUARD_WORDS;

    x_tiles = (uint16_t)((scene->canvas_width + tile_width - 1U) / tile_width);
    y_tiles = (uint16_t)((scene->canvas_height + tile_height - 1U) / tile_height);
    reverse_x = (uint8_t)((scene->index >> 1) & 1U);
    reverse_y = (uint8_t)((scene->index >> 2) & 1U);

    for (uint8_t pass_index = 0; pass_index < scene->pass_count; pass_index++)
    {
        const egui_region_t *base_region = &scene->passes[pass_index].clip;

        for (uint16_t y_order = 0; y_order < y_tiles; y_order++)
        {
            uint16_t tile_y_index = reverse_y ? (uint16_t)(y_tiles - 1U - y_order) : y_order;
            uint16_t tile_y = (uint16_t)(tile_y_index * tile_height);
            uint16_t current_tile_height = (uint16_t)EGUI_MIN(tile_height, scene->canvas_height - tile_y);

            for (uint16_t x_order = 0; x_order < x_tiles; x_order++)
            {
                uint16_t tile_x_index = reverse_x ? (uint16_t)(x_tiles - 1U - x_order) : x_order;
                uint16_t tile_x = (uint16_t)(tile_x_index * tile_width);
                uint16_t current_tile_width = (uint16_t)EGUI_MIN(tile_width, scene->canvas_width - tile_x);
                uint32_t current_tile_pixel_count = (uint32_t)current_tile_width * current_tile_height;
                egui_region_t tile;

                egui_region_init(&tile, tile_x, tile_y, current_tile_width, current_tile_height);
                if (egui_region_is_intersect((egui_region_t *)base_region, &tile))
                {
                    test_image_codec_fill_guard(pfb_raw, current_tile_pixel_count);
                    test_image_codec_draw_one_pfb_tile(scene, image, base_region, &tile, pfb, canvas_buffer);
                    if (!test_image_codec_check_guard(scene, codec_name, pfb_raw, current_tile_pixel_count))
                    {
                        goto cleanup;
                    }
                }
            }
        }
    }

    ok = 1;

cleanup:
    egui_free(core, pfb_raw);
    return ok;
}

static int test_image_codec_compare_buffer(const test_image_codec_scene_t *scene, const char *codec_name, const egui_color_int_t *actual,
                                           const egui_color_int_t *expected)
{
    uint32_t pixel_count = (uint32_t)scene->canvas_width * scene->canvas_height;

    for (uint32_t i = 0; i < pixel_count; i++)
    {
        if (actual[i] != expected[i])
        {
            uint16_t x = (uint16_t)(i % scene->canvas_width);
            uint16_t y = (uint16_t)(i / scene->canvas_width);

            egui_api_log("  image_codec mismatch scene=%u codec=%s at (%u,%u), expected=0x%04x actual=0x%04x\n", scene->index, codec_name, x, y, expected[i],
                         actual[i]);
            egui_test_fail_int(__FILE__, __LINE__, "pixel", expected[i], actual[i]);
            return 0;
        }
    }
    return 1;
}

static void test_image_codec_fill_guard(egui_color_int_t *raw, uint32_t pixel_count)
{
    for (uint32_t i = 0; i < TEST_IMAGE_CODEC_GUARD_WORDS; i++)
    {
        raw[i] = TEST_IMAGE_CODEC_GUARD_VALUE;
        raw[TEST_IMAGE_CODEC_GUARD_WORDS + pixel_count + i] = TEST_IMAGE_CODEC_GUARD_VALUE;
    }
}

static int test_image_codec_check_guard(const test_image_codec_scene_t *scene, const char *codec_name, const egui_color_int_t *raw, uint32_t pixel_count)
{
    for (uint32_t i = 0; i < TEST_IMAGE_CODEC_GUARD_WORDS; i++)
    {
        if (raw[i] != TEST_IMAGE_CODEC_GUARD_VALUE)
        {
            egui_api_log("  image_codec guard underrun scene=%u codec=%s guard=%u\n", scene->index, codec_name, i);
            egui_test_fail_int(__FILE__, __LINE__, "guard_before", TEST_IMAGE_CODEC_GUARD_VALUE, raw[i]);
            return 0;
        }
        if (raw[TEST_IMAGE_CODEC_GUARD_WORDS + pixel_count + i] != TEST_IMAGE_CODEC_GUARD_VALUE)
        {
            egui_api_log("  image_codec guard overrun scene=%u codec=%s guard=%u\n", scene->index, codec_name, i);
            egui_test_fail_int(__FILE__, __LINE__, "guard_after", TEST_IMAGE_CODEC_GUARD_VALUE, raw[TEST_IMAGE_CODEC_GUARD_WORDS + pixel_count + i]);
            return 0;
        }
    }
    return 1;
}

static void test_image_codec_release_codec_caches(egui_core_t *core)
{
    egui_image_decode_release_frame_cache(core);
    egui_image_qoi_release_frame_cache(core);
    egui_image_rle_release_frame_cache(core);
}

static int test_image_codec_run_scene_internal(uint16_t scene_index, int run_pfb_tiles)
{
    egui_core_t *core = test_image_codec_get_core();
    test_image_codec_scene_t scene;
    test_image_codec_pixel_t *pixels = NULL;
    uint8_t *rle_pixel_units = NULL;
    uint8_t *rle_alpha_units = NULL;
    egui_color_int_t *actual_raw = NULL;
    egui_color_int_t *actual = NULL;
    egui_color_int_t *expected = NULL;
    test_image_codec_stream_t qoi_stream = {0};
    test_image_codec_stream_t rle_pixel_stream = {0};
    test_image_codec_stream_t rle_alpha_stream = {0};
    uint32_t pixel_count;
    uint32_t canvas_pixel_count;
    uint16_t alpha_row_bytes = 0;
    egui_image_qoi_info_t qoi_info;
    egui_image_rle_info_t rle_info;
    egui_image_qoi_t qoi_image;
    egui_image_rle_t rle_image;
    egui_dim_t width = 0;
    egui_dim_t height = 0;
    int ok = 0;

    test_image_codec_make_scene(scene_index, &scene);
    pixel_count = (uint32_t)scene.width * scene.height;
    canvas_pixel_count = (uint32_t)scene.canvas_width * scene.canvas_height;

    pixels = (test_image_codec_pixel_t *)egui_malloc(core, (int)(pixel_count * sizeof(*pixels)));
    actual_raw = (egui_color_int_t *)egui_malloc(core, (int)((canvas_pixel_count + TEST_IMAGE_CODEC_GUARD_WORDS * 2U) * sizeof(*actual_raw)));
    expected = (egui_color_int_t *)egui_malloc(core, (int)(canvas_pixel_count * sizeof(*expected)));
    if (pixels == NULL || actual_raw == NULL || expected == NULL)
    {
        egui_test_fail(__FILE__, __LINE__, "allocation", "expected non-NULL");
        goto cleanup;
    }
    actual = actual_raw + TEST_IMAGE_CODEC_GUARD_WORDS;

    test_image_codec_make_pixels(&scene, pixels);
    if (!test_image_codec_qoi_encode(core, pixels, pixel_count, scene.channels, &qoi_stream))
    {
        egui_test_fail(__FILE__, __LINE__, "qoi_encode", "expected success");
        goto cleanup;
    }

    if (!test_image_codec_make_rle_pixel_units(core, &scene, pixels, &rle_pixel_units) ||
        !test_image_codec_rle_encode(core, rle_pixel_units, scene.width, scene.height, 2, &rle_pixel_stream))
    {
        egui_test_fail(__FILE__, __LINE__, "rle_pixel_encode", "expected success");
        goto cleanup;
    }

    if (scene.channels == 4U)
    {
        alpha_row_bytes = test_image_codec_alpha_row_bytes(scene.alpha_type, scene.width);
        if (!test_image_codec_make_rle_alpha_units(core, &scene, pixels, alpha_row_bytes, &rle_alpha_units) ||
            !test_image_codec_rle_encode(core, rle_alpha_units, alpha_row_bytes, scene.height, 1, &rle_alpha_stream))
        {
            egui_test_fail(__FILE__, __LINE__, "rle_alpha_encode", "expected success");
            goto cleanup;
        }
    }

    qoi_info.data_buf = qoi_stream.data;
    qoi_info.alpha_buf = NULL;
    qoi_info.data_type = EGUI_IMAGE_DATA_TYPE_RGB565;
    qoi_info.alpha_type = scene.channels == 4U ? EGUI_IMAGE_ALPHA_TYPE_8 : EGUI_IMAGE_ALPHA_TYPE_1;
    qoi_info.res_type = EGUI_RESOURCE_TYPE_INTERNAL;
    qoi_info.channels = scene.channels;
    qoi_info.width = scene.width;
    qoi_info.height = scene.height;
    qoi_info.data_size = qoi_stream.size;
    qoi_info.decompressed_size = pixel_count * 2U;
    egui_image_qoi_init((egui_image_t *)&qoi_image, &qoi_info);

    rle_info.data_buf = rle_pixel_stream.data;
    rle_info.alpha_buf = scene.channels == 4U ? rle_alpha_stream.data : NULL;
    rle_info.data_type = EGUI_IMAGE_DATA_TYPE_RGB565;
    rle_info.alpha_type = scene.alpha_type;
    rle_info.res_type = EGUI_RESOURCE_TYPE_INTERNAL;
    rle_info.width = scene.width;
    rle_info.height = scene.height;
    rle_info.data_size = rle_pixel_stream.size;
    rle_info.alpha_size = scene.channels == 4U ? rle_alpha_stream.size : 0;
    rle_info.decompressed_size = pixel_count * 2U;
    egui_image_rle_init((egui_image_t *)&rle_image, &rle_info);

    if (!egui_image_get_size((const egui_image_t *)&qoi_image, &width, &height) || width != (egui_dim_t)scene.width || height != (egui_dim_t)scene.height)
    {
        egui_test_fail(__FILE__, __LINE__, "qoi_size", "unexpected size");
        goto cleanup;
    }
    if (!egui_image_get_size((const egui_image_t *)&rle_image, &width, &height) || width != (egui_dim_t)scene.width || height != (egui_dim_t)scene.height)
    {
        egui_test_fail(__FILE__, __LINE__, "rle_size", "unexpected size");
        goto cleanup;
    }

    test_image_codec_fill_background(&scene, expected);
    test_image_codec_apply_expected(&scene, pixels, expected);

    test_image_codec_fill_background(&scene, actual);
    test_image_codec_fill_guard(actual_raw, canvas_pixel_count);
    test_image_codec_release_codec_caches(core);
    test_image_codec_draw_passes(&scene, (const egui_image_t *)&qoi_image, actual);
    if (!test_image_codec_check_guard(&scene, "qoi", actual_raw, canvas_pixel_count) || !test_image_codec_compare_buffer(&scene, "qoi", actual, expected))
    {
        goto cleanup;
    }

    test_image_codec_fill_background(&scene, actual);
    test_image_codec_fill_guard(actual_raw, canvas_pixel_count);
    test_image_codec_release_codec_caches(core);
    test_image_codec_draw_passes(&scene, (const egui_image_t *)&rle_image, actual);
    if (!test_image_codec_check_guard(&scene, "rle", actual_raw, canvas_pixel_count) || !test_image_codec_compare_buffer(&scene, "rle", actual, expected))
    {
        goto cleanup;
    }

    if (run_pfb_tiles)
    {
        test_image_codec_fill_background(&scene, actual);
        test_image_codec_release_codec_caches(core);
        if (!test_image_codec_draw_pfb_tiles(&scene, "qoi_pfb", (const egui_image_t *)&qoi_image, actual) ||
            !test_image_codec_compare_buffer(&scene, "qoi_pfb", actual, expected))
        {
            goto cleanup;
        }

        test_image_codec_fill_background(&scene, actual);
        test_image_codec_release_codec_caches(core);
        if (!test_image_codec_draw_pfb_tiles(&scene, "rle_pfb", (const egui_image_t *)&rle_image, actual) ||
            !test_image_codec_compare_buffer(&scene, "rle_pfb", actual, expected))
        {
            goto cleanup;
        }
    }

    ok = 1;

cleanup:
    test_image_codec_release_codec_caches(core);
    if (rle_alpha_units != NULL)
    {
        egui_free(core, rle_alpha_units);
    }
    if (rle_pixel_units != NULL)
    {
        egui_free(core, rle_pixel_units);
    }
    if (expected != NULL)
    {
        egui_free(core, expected);
    }
    if (actual_raw != NULL)
    {
        egui_free(core, actual_raw);
    }
    if (pixels != NULL)
    {
        egui_free(core, pixels);
    }
    test_image_codec_stream_deinit(core, &rle_alpha_stream);
    test_image_codec_stream_deinit(core, &rle_pixel_stream);
    test_image_codec_stream_deinit(core, &qoi_stream);
    return ok;
}

static void test_image_codec_run_scene(uint16_t scene_index)
{
    if (!test_image_codec_run_scene_internal(scene_index, 0))
    {
        return;
    }
}

static void test_image_codec_run_pfb_scene(uint16_t scene_index)
{
    if (!test_image_codec_run_scene_internal(scene_index, 1))
    {
        return;
    }
}

static void test_image_codec_api_contracts(void)
{
    static const uint8_t qoi_data[] = {TEST_QOI_OP_RGB, 0x20, 0x40, 0x60};
    static const uint8_t rle_data[] = {1, 0x44, 0x4a};
    const egui_image_qoi_info_t qoi_info = {
            .data_buf = qoi_data,
            .alpha_buf = NULL,
            .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
            .alpha_type = EGUI_IMAGE_ALPHA_TYPE_1,
            .res_type = EGUI_RESOURCE_TYPE_INTERNAL,
            .channels = 3,
            .width = 1,
            .height = 1,
            .data_size = sizeof(qoi_data),
            .decompressed_size = 2,
    };
    const egui_image_rle_info_t rle_info = {
            .data_buf = rle_data,
            .alpha_buf = NULL,
            .data_type = EGUI_IMAGE_DATA_TYPE_RGB565,
            .alpha_type = EGUI_IMAGE_ALPHA_TYPE_1,
            .res_type = EGUI_RESOURCE_TYPE_INTERNAL,
            .width = 1,
            .height = 1,
            .data_size = sizeof(rle_data),
            .alpha_size = 0,
            .decompressed_size = 2,
    };
    egui_image_qoi_t qoi_image;
    egui_image_rle_t rle_image;
    egui_dim_t width = 0;
    egui_dim_t height = 0;
    egui_color_t color = EGUI_COLOR_WHITE;
    egui_alpha_t alpha = EGUI_ALPHA_100;
    egui_color_int_t pfb = 0x1357;
    egui_canvas_t canvas;
    egui_region_t region;

    egui_image_qoi_init((egui_image_t *)&qoi_image, &qoi_info);
    egui_image_rle_init((egui_image_t *)&rle_image, &rle_info);

    EGUI_TEST_ASSERT_TRUE(egui_image_get_size((const egui_image_t *)&qoi_image, &width, &height));
    EGUI_TEST_ASSERT_EQUAL_INT(1, width);
    EGUI_TEST_ASSERT_EQUAL_INT(1, height);
    EGUI_TEST_ASSERT_TRUE(egui_image_get_size((const egui_image_t *)&rle_image, &width, &height));
    EGUI_TEST_ASSERT_EQUAL_INT(1, width);
    EGUI_TEST_ASSERT_EQUAL_INT(1, height);

    EGUI_TEST_ASSERT_FALSE(((const egui_image_t *)&qoi_image)->api->get_point((const egui_image_t *)&qoi_image, 0, 0, &color, &alpha));
    EGUI_TEST_ASSERT_EQUAL_INT(0, color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, alpha);
    color = EGUI_COLOR_WHITE;
    alpha = EGUI_ALPHA_100;
    EGUI_TEST_ASSERT_FALSE(((const egui_image_t *)&rle_image)->api->get_point((const egui_image_t *)&rle_image, 0, 0, &color, &alpha));
    EGUI_TEST_ASSERT_EQUAL_INT(0, color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, alpha);

    egui_region_init(&region, 0, 0, 1, 1);
    egui_canvas_init(&canvas, test_image_codec_get_core(), &pfb, &region);
    egui_canvas_clear_mask(&canvas);
    egui_canvas_calc_work_region(&canvas, &region);
    egui_canvas_draw_image_resize(&canvas, (const egui_image_t *)&qoi_image, 0, 0, 2, 2);
    EGUI_TEST_ASSERT_EQUAL_INT(0x1357, pfb);
    egui_canvas_draw_image_resize(&canvas, (const egui_image_t *)&rle_image, 0, 0, 2, 2);
    EGUI_TEST_ASSERT_EQUAL_INT(0x1357, pfb);
}

#define TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(_index)                                                                                                             \
    static void test_image_codec_scene_##_index(void)                                                                                                          \
    {                                                                                                                                                          \
        test_image_codec_run_scene(_index);                                                                                                                    \
    }

TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(0)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(1)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(2)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(3)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(4)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(5)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(6)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(7)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(8)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(9)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(10)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(11)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(12)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(13)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(14)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(15)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(16)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(17)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(18)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(19)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(20)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(21)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(22)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(23)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(24)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(25)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(26)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(27)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(28)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(29)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(30)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(31)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(32)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(33)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(34)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(35)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(36)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(37)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(38)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(39)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(40)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(41)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(42)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(43)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(44)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(45)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(46)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(47)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(48)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(49)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(50)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(51)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(52)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(53)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(54)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(55)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(56)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(57)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(58)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(59)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(60)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(61)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(62)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(63)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(64)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(65)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(66)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(67)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(68)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(69)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(70)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(71)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(72)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(73)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(74)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(75)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(76)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(77)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(78)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(79)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(80)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(81)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(82)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(83)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(84)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(85)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(86)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(87)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(88)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(89)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(90)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(91)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(92)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(93)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(94)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(95)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(96)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(97)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(98)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(99)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(100)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(101)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(102)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(103)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(104)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(105)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(106)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(107)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(108)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(109)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(110)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(111)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(112)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(113)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(114)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(115)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(116)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(117)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(118)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(119)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(120)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(121)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(122)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(123)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(124)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(125)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(126)
TEST_IMAGE_CODEC_DEFINE_SCENE_CASE(127)

#define TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(_index)                                                                                                         \
    static void test_image_codec_pfb_scene_##_index(void)                                                                                                      \
    {                                                                                                                                                          \
        test_image_codec_run_pfb_scene(_index);                                                                                                                \
    }

TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(96)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(97)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(98)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(99)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(100)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(101)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(102)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(103)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(104)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(105)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(106)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(107)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(108)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(109)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(110)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(111)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(112)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(113)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(114)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(115)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(116)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(117)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(118)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(119)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(120)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(121)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(122)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(123)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(124)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(125)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(126)
TEST_IMAGE_CODEC_DEFINE_PFB_SCENE_CASE(127)

#define TEST_IMAGE_CODEC_RUN_SCENE_CASE(_index)     EGUI_TEST_RUN(test_image_codec_scene_##_index)
#define TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(_index) EGUI_TEST_RUN(test_image_codec_pfb_scene_##_index)

void test_image_codec_run(void)
{
    EGUI_TEST_SUITE_BEGIN(image_codec);

    EGUI_TEST_RUN(test_image_codec_api_contracts);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(0);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(1);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(2);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(3);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(4);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(5);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(6);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(7);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(8);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(9);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(10);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(11);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(12);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(13);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(14);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(15);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(16);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(17);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(18);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(19);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(20);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(21);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(22);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(23);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(24);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(25);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(26);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(27);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(28);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(29);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(30);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(31);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(32);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(33);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(34);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(35);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(36);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(37);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(38);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(39);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(40);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(41);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(42);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(43);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(44);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(45);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(46);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(47);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(48);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(49);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(50);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(51);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(52);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(53);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(54);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(55);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(56);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(57);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(58);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(59);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(60);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(61);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(62);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(63);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(64);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(65);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(66);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(67);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(68);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(69);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(70);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(71);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(72);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(73);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(74);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(75);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(76);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(77);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(78);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(79);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(80);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(81);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(82);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(83);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(84);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(85);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(86);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(87);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(88);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(89);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(90);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(91);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(92);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(93);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(94);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(95);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(96);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(97);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(98);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(99);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(100);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(101);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(102);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(103);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(104);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(105);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(106);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(107);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(108);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(109);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(110);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(111);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(112);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(113);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(114);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(115);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(116);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(117);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(118);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(119);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(120);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(121);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(122);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(123);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(124);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(125);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(126);
    TEST_IMAGE_CODEC_RUN_SCENE_CASE(127);

    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(96);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(97);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(98);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(99);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(100);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(101);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(102);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(103);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(104);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(105);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(106);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(107);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(108);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(109);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(110);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(111);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(112);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(113);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(114);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(115);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(116);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(117);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(118);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(119);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(120);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(121);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(122);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(123);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(124);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(125);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(126);
    TEST_IMAGE_CODEC_RUN_PFB_SCENE_CASE(127);

    EGUI_TEST_SUITE_END();
}

#else

void test_image_codec_run(void)
{
}

#endif
