#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_font_std.h"
#include "core/egui_api.h"

#define FONT_ERROR_FONT_SIZE(_height) (_height >> 1)

extern const egui_font_api_t egui_font_std_t_api_table;

typedef struct
{
    uint8_t *pixel_buf;
    uint32_t capacity;
    uint8_t *compressed_buf;
    uint32_t compressed_capacity;
} egui_font_std_external_draw_scratch_t;

typedef uint16_t egui_font_std_code_lookup_count_t;
typedef uint32_t egui_font_std_code_lookup_code_t;
typedef int egui_font_std_code_lookup_index_t;
#define EGUI_FONT_STD_CODE_LOOKUP_INDEX_INVALID (-1)

typedef struct
{
    const egui_font_std_code_descriptor_t *code_array;
    egui_font_std_code_lookup_count_t count;
    egui_font_std_code_lookup_code_t last_code;
    egui_font_std_code_lookup_index_t last_index;
    egui_font_std_code_lookup_code_t block_start_code;
    egui_font_std_code_lookup_code_t block_end_code;
    egui_font_std_code_lookup_index_t block_start_index;
    egui_font_std_code_lookup_index_t block_end_index;
} egui_font_std_code_lookup_cache_t;

static egui_font_std_code_lookup_cache_t g_font_std_code_lookup_cache = {
        .code_array = NULL,
        .count = 0,
        .last_index = EGUI_FONT_STD_CODE_LOOKUP_INDEX_INVALID,
        .block_start_index = EGUI_FONT_STD_CODE_LOOKUP_INDEX_INVALID,
        .block_end_index = EGUI_FONT_STD_CODE_LOOKUP_INDEX_INVALID,
};

#define EGUI_FONT_STD_ASCII_CACHE_SIZE 128
#ifndef EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE
#define EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE 1
#endif
#ifndef EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS
#define EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS 64
#endif
#ifndef EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS
#define EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS 2
#endif
#ifndef EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
#define EGUI_CONFIG_FUNCTION_FONT_FORMAT_1 0
#endif
#ifndef EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
#define EGUI_CONFIG_FUNCTION_FONT_FORMAT_2 0
#endif
#ifndef EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
#define EGUI_CONFIG_FUNCTION_FONT_FORMAT_4 1
#endif
#ifndef EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
#define EGUI_CONFIG_FUNCTION_FONT_FORMAT_8 0
#endif
#define EGUI_FONT_STD_DRAW_PREFIX_CACHE_ENABLED ((EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS > 0) && (EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS > 0))
#define EGUI_FONT_STD_DRAW_PREFIX_CACHE_GLYPH_STORAGE_MAX                                                                                                      \
    ((EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS > 0) ? EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS : 1)
#ifndef EGUI_FONT_STD_LINE_CACHE_MAX_LINES
#define EGUI_FONT_STD_LINE_CACHE_MAX_LINES 16
#endif
#ifndef EGUI_FONT_STD_LINE_CACHE_SLOTS
#define EGUI_FONT_STD_LINE_CACHE_SLOTS 2
#endif

typedef uint16_t egui_font_std_ascii_index_t;
#define EGUI_FONT_STD_ASCII_INDEX_INVALID UINT16_MAX

typedef struct
{
    uint32_t idx;
    int16_t x;
    int16_t box_x0;
    int16_t box_x1;
    uint8_t box_w;
    uint8_t box_h;
    uint8_t adv;
    uint8_t char_bytes;
    uint8_t has_desc;
    int8_t off_x;
    int8_t off_y;
} egui_font_std_draw_prefix_glyph_t;

typedef struct
{
    const void *font_key;
    const void *string;
    uint16_t glyph_count;
    uint16_t cached_bytes;
    uint16_t line_bytes;
    int16_t cached_advance;
    uint32_t content_hash;
    uint8_t is_complete_line;
    uint8_t is_ready;
    uint32_t stamp;
    egui_font_std_draw_prefix_glyph_t glyphs[EGUI_FONT_STD_DRAW_PREFIX_CACHE_GLYPH_STORAGE_MAX];
} egui_font_std_draw_prefix_cache_t;

typedef struct
{
    const egui_font_std_code_descriptor_t *code_array;
    const egui_font_std_char_descriptor_t *char_array;
    uint16_t count;
    egui_font_std_ascii_index_t ascii_index[EGUI_FONT_STD_ASCII_CACHE_SIZE];
    uint8_t is_ready;
} egui_font_std_ascii_lookup_cache_t;

typedef struct
{
    const char *string;
    uint16_t line_count;
    uint32_t content_hash;
    uint8_t is_ready;
    uint8_t is_complete;
    uint32_t stamp;
    const char *lines[EGUI_FONT_STD_LINE_CACHE_MAX_LINES];
} egui_font_std_line_cache_t;

typedef struct
{
    uint32_t stamp;
    egui_font_std_line_cache_t entries[EGUI_FONT_STD_LINE_CACHE_SLOTS];
} egui_font_std_line_cache_storage_t;

#if EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE
static egui_font_std_ascii_lookup_cache_t *g_font_std_ascii_lookup_cache = NULL;
#endif

static egui_font_std_external_draw_scratch_t g_font_std_compressed_draw_scratch = {
        0,
};

#if EGUI_FONT_STD_DRAW_PREFIX_CACHE_ENABLED
static egui_font_std_draw_prefix_cache_t g_font_std_draw_prefix_cache[EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS];
static uint32_t g_font_std_draw_prefix_cache_stamp = 0;
#endif

__EGUI_STATIC_INLINE__ const egui_font_std_char_descriptor_t *egui_font_std_get_desc_draw_fast(const egui_font_std_info_t *font, uint32_t utf8_code,
                                                                                               egui_font_std_char_descriptor_t *external_desc_scratch);
static void egui_font_std_external_draw_scratch_release(egui_font_std_external_draw_scratch_t *scratch);

__EGUI_STATIC_INLINE__ int egui_font_std_bitmap_codec_is_raw(const egui_font_std_info_t *font)
{
    return font == NULL || font->bitmap_codec == EGUI_FONT_STD_BITMAP_CODEC_RAW;
}

__EGUI_STATIC_INLINE__ int egui_font_std_bitmap_codec_is_compressed(const egui_font_std_info_t *font)
{
    return !egui_font_std_bitmap_codec_is_raw(font);
}

__EGUI_STATIC_INLINE__ egui_font_std_code_lookup_count_t egui_font_std_code_lookup_cache_store_count(uint16_t count)
{
    return (egui_font_std_code_lookup_count_t)count;
}

__EGUI_STATIC_INLINE__ egui_font_std_code_lookup_code_t egui_font_std_code_lookup_cache_store_code(uint32_t code)
{
    return (egui_font_std_code_lookup_code_t)code;
}

__EGUI_STATIC_INLINE__ egui_font_std_code_lookup_index_t egui_font_std_code_lookup_cache_store_index(int index)
{
    return (egui_font_std_code_lookup_index_t)index;
}

#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) && EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
static uint32_t egui_font_std_hash_bytes(const char *s, uint16_t *byte_count, int stop_at_newline)
{
    uint32_t hash = 2166136261u;
    uint16_t bytes = 0;

    if (s == NULL)
    {
        if (byte_count != NULL)
        {
            *byte_count = 0;
        }
        return 0;
    }

    while (s[bytes] != '\0')
    {
        uint8_t ch = (uint8_t)s[bytes];

        hash ^= ch;
        hash *= 16777619u;
        bytes++;

        if (stop_at_newline && ch == '\n')
        {
            break;
        }
    }

    if (byte_count != NULL)
    {
        *byte_count = bytes;
    }

    return hash;
}
#endif

static uint16_t egui_font_std_get_packed_pixel_row_stride(const egui_font_std_info_t *font, egui_dim_t width)
{
    if (font == NULL || width <= 0)
    {
        return 0;
    }

    switch (font->font_bit_mode)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
        return (uint16_t)((width + 7) >> 3);
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
        return (uint16_t)((width + 3) >> 2);
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
        return (uint16_t)((width + 1) >> 1);
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
        return (uint16_t)width;
#endif
    default:
        return 0;
    }
}

static uint32_t egui_font_std_get_packed_pixel_size(const egui_font_std_info_t *font, egui_dim_t width, egui_dim_t height)
{
    uint16_t row_stride = egui_font_std_get_packed_pixel_row_stride(font, width);

    if (row_stride == 0 || height <= 0)
    {
        return 0;
    }

    return (uint32_t)row_stride * (uint32_t)height;
}

static uint8_t egui_font_std_read_packed_nibble_4(const uint8_t *buf, uint32_t nibble_index)
{
    uint8_t packed = buf[nibble_index >> 1];
    return (nibble_index & 0x01) ? (uint8_t)((packed >> 4) & 0x0F) : (uint8_t)(packed & 0x0F);
}

static void egui_font_std_write_packed_nibble_4(uint8_t *buf, uint32_t nibble_index, uint8_t value)
{
    uint8_t *packed = &buf[nibble_index >> 1];

    if ((nibble_index & 0x01) == 0)
    {
        *packed = (uint8_t)((*packed & 0xF0) | (value & 0x0F));
    }
    else
    {
        *packed = (uint8_t)((*packed & 0x0F) | ((value & 0x0F) << 4));
    }
}

static int egui_font_std_is_glyph_outside_region(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const egui_region_t *region)
{
    egui_dim_t region_x1;
    egui_dim_t region_y1;
    egui_dim_t glyph_x1;
    egui_dim_t glyph_y1;

    if (region == NULL || width <= 0 || height <= 0)
    {
        return 0;
    }

    region_x1 = region->location.x + region->size.width;
    region_y1 = region->location.y + region->size.height;
    glyph_x1 = x + width;
    glyph_y1 = y + height;

    return x >= region_x1 || y >= region_y1 || glyph_x1 <= region->location.x || glyph_y1 <= region->location.y;
}

static int egui_font_std_decode_rle4_bitmap(const uint8_t *src, uint32_t src_size, uint8_t *dst, uint32_t dst_size, egui_dim_t width, egui_dim_t height,
                                            int use_row_xor)
{
    uint32_t nibble_total;
    uint32_t src_offset = 0;
    uint32_t nibble_offset = 0;
    uint32_t row_pos = 0;
    uint8_t row_prev = 0;

    if (src == NULL || dst == NULL || width <= 0 || height <= 0)
    {
        return 0;
    }

    nibble_total = (uint32_t)width * (uint32_t)height;
    if (dst_size != ((nibble_total + 1u) >> 1))
    {
        return 0;
    }

    egui_api_memset(dst, 0, dst_size);

    while (nibble_offset < nibble_total)
    {
        uint8_t ctrl;
        uint32_t run;
        uint8_t repeat_value = 0;
        uint32_t literal_bytes = 0;
        const uint8_t *literal_src = NULL;

        if (src_offset >= src_size)
        {
            return 0;
        }

        ctrl = src[src_offset++];
        run = (uint32_t)(ctrl & 0x7F) + 1u;
        if (run > (nibble_total - nibble_offset))
        {
            return 0;
        }

        if ((ctrl & 0x80) != 0)
        {
            if (src_offset >= src_size)
            {
                return 0;
            }

            repeat_value = (uint8_t)(src[src_offset++] & 0x0F);
        }
        else
        {
            literal_bytes = (run + 1u) >> 1;
            if ((src_offset + literal_bytes) > src_size)
            {
                return 0;
            }

            literal_src = &src[src_offset];
            src_offset += literal_bytes;
        }

        for (uint32_t i = 0; i < run; i++)
        {
            uint8_t value = repeat_value;

            if (literal_src != NULL)
            {
                value = egui_font_std_read_packed_nibble_4(literal_src, i);
            }

            if (use_row_xor)
            {
                value = (uint8_t)((value ^ row_prev) & 0x0F);
                row_prev = value;
            }

            egui_font_std_write_packed_nibble_4(dst, nibble_offset++, value);
            row_pos++;
            if (row_pos >= (uint32_t)width)
            {
                row_pos = 0;
                row_prev = 0;
            }
        }
    }

    return src_offset == src_size;
}

static void egui_font_std_release_ascii_lookup_cache(void)
{
#if EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE
    if (g_font_std_ascii_lookup_cache != NULL)
    {
        egui_free(g_font_std_ascii_lookup_cache);
        g_font_std_ascii_lookup_cache = NULL;
    }
#endif
}

static egui_font_std_ascii_lookup_cache_t *egui_font_std_get_ascii_lookup_cache(void)
{
#if !EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE
    return NULL;
#else
    if (g_font_std_ascii_lookup_cache == NULL)
    {
        g_font_std_ascii_lookup_cache = (egui_font_std_ascii_lookup_cache_t *)egui_malloc(sizeof(egui_font_std_ascii_lookup_cache_t));
        if (g_font_std_ascii_lookup_cache == NULL)
        {
            return NULL;
        }

        egui_api_memset(g_font_std_ascii_lookup_cache, 0, sizeof(egui_font_std_ascii_lookup_cache_t));
    }

    return g_font_std_ascii_lookup_cache;
#endif
}

__EGUI_STATIC_INLINE__ void egui_font_std_reset_ascii_lookup_cache(const egui_font_std_info_t *font, egui_font_std_ascii_lookup_cache_t *cache)
{
    if (font == NULL || cache == NULL)
    {
        return;
    }

    if (cache->code_array != font->code_array || cache->char_array != font->char_array || cache->count != font->count)
    {
        cache->code_array = font->code_array;
        cache->char_array = font->char_array;
        cache->count = font->count;
        cache->is_ready = 0;
    }
}

__EGUI_STATIC_INLINE__ int egui_font_std_ascii_lookup_cache_supports_font(const egui_font_std_info_t *font)
{
    if (font == NULL)
    {
        return 0;
    }

    return font->count <= EGUI_FONT_STD_ASCII_INDEX_INVALID;
}

__EGUI_STATIC_INLINE__ int egui_font_std_get_ascii_lookup_index(const egui_font_std_ascii_lookup_cache_t *cache, uint8_t ascii_code)
{
    uint16_t index;

    if (cache == NULL)
    {
        return -1;
    }

    index = cache->ascii_index[ascii_code];
    if (index == EGUI_FONT_STD_ASCII_INDEX_INVALID)
    {
        return -1;
    }

    return (int)index;
}

static const egui_font_std_ascii_lookup_cache_t *egui_font_std_prepare_ascii_lookup_cache(const egui_font_std_info_t *font)
{
    int i;
    egui_font_std_ascii_lookup_cache_t *cache = egui_font_std_get_ascii_lookup_cache();

    if (cache == NULL || font == NULL)
    {
        return NULL;
    }

    if (!egui_font_std_ascii_lookup_cache_supports_font(font))
    {
        return NULL;
    }

    egui_font_std_reset_ascii_lookup_cache(font, cache);
    if (cache->is_ready)
    {
        return cache;
    }

    for (i = 0; i < EGUI_FONT_STD_ASCII_CACHE_SIZE; i++)
    {
        cache->ascii_index[i] = EGUI_FONT_STD_ASCII_INDEX_INVALID;
    }

    if (font->code_array != NULL)
    {
        for (i = 0; i < font->count; i++)
        {
            uint32_t code = font->code_array[i].code;
            if (code < EGUI_FONT_STD_ASCII_CACHE_SIZE)
            {
                cache->ascii_index[code] = (uint16_t)i;
            }
        }
    }

    cache->is_ready = 1;
    return cache;
}

#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) && EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
static const egui_font_std_draw_prefix_cache_t *egui_font_std_prepare_draw_prefix_cache(const void *font_key, const egui_font_std_info_t *font, const char *s)
{
#if !EGUI_FONT_STD_DRAW_PREFIX_CACHE_ENABLED
    (void)font_key;
    (void)font;
    (void)s;
    return NULL;
#else
    int cursor_x = 0;
    int cached_bytes = 0;
    int glyph_count = 0;
    egui_font_std_char_descriptor_t external_desc_scratch;
    int advance_limit;
    const char *cursor = s;
    egui_font_std_draw_prefix_cache_t *cache = &g_font_std_draw_prefix_cache[0];
    uint16_t line_bytes = 0;
    uint32_t content_hash = egui_font_std_hash_bytes(s, &line_bytes, 1);
    const egui_font_std_ascii_lookup_cache_t *ascii_cache = NULL;

    if (font_key == NULL || font == NULL || s == NULL)
    {
        return NULL;
    }

    if (!egui_font_std_bitmap_codec_is_raw(font))
    {
        return NULL;
    }

    for (int i = 0; i < EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS; i++)
    {
        egui_font_std_draw_prefix_cache_t *entry = &g_font_std_draw_prefix_cache[i];

        if (entry->is_ready && entry->font_key == font_key && entry->string == s && entry->line_bytes == line_bytes && entry->content_hash == content_hash)
        {
            entry->stamp = ++g_font_std_draw_prefix_cache_stamp;
            return entry;
        }

        if (!entry->is_ready)
        {
            cache = entry;
            break;
        }

        if (cache->is_ready && entry->stamp < cache->stamp)
        {
            cache = entry;
        }
    }

    cache->font_key = font_key;
    cache->string = s;
    cache->glyph_count = 0;
    cache->cached_bytes = 0;
    cache->line_bytes = 0;
    cache->cached_advance = 0;
    cache->content_hash = content_hash;
    cache->is_complete_line = 0;
    cache->is_ready = 1;
    cache->stamp = ++g_font_std_draw_prefix_cache_stamp;
    cache->line_bytes = line_bytes;

    advance_limit = EGUI_CONFIG_SCEEN_WIDTH * 2 + font->height;

    if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL && font->code_array != NULL)
    {
        ascii_cache = egui_font_std_prepare_ascii_lookup_cache(font);
    }

    while (*cursor != '\0')
    {
        int char_bytes;
        int adv;
        uint32_t utf8_code;
        const egui_font_std_char_descriptor_t *p_char_desc;

        if (*cursor == '\r')
        {
            cursor++;
            cached_bytes++;
            continue;
        }

        if (*cursor == '\n')
        {
            cache->is_complete_line = 1;
            cache->cached_bytes = (uint16_t)(cached_bytes + 1);
            cache->cached_advance = (int16_t)cursor_x;
            cache->glyph_count = (uint16_t)glyph_count;
            return cache;
        }

        if (glyph_count >= EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS || cursor_x > advance_limit)
        {
            break;
        }

        if (ascii_cache != NULL && (((uint8_t)cursor[0]) & 0x80) == 0)
        {
            uint8_t ascii_code = (uint8_t)cursor[0];
            int ascii_index;
            char_bytes = 1;
            ascii_index = egui_font_std_get_ascii_lookup_index(ascii_cache, ascii_code);
            p_char_desc = ascii_index >= 0 ? &font->char_array[ascii_index] : NULL;
            adv = p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
        }
        else
        {
            char_bytes = egui_font_get_utf8_code_fast(cursor, &utf8_code);
            p_char_desc = egui_font_std_get_desc_draw_fast(font, utf8_code, &external_desc_scratch);
            adv = p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
        }

        if (p_char_desc != NULL)
        {
            egui_font_std_draw_prefix_glyph_t *glyph = &cache->glyphs[glyph_count];

            glyph->idx = p_char_desc->idx;
            glyph->x = (int16_t)cursor_x;
            glyph->box_x0 = (int16_t)(cursor_x + p_char_desc->off_x);
            glyph->box_x1 = (int16_t)(glyph->box_x0 + p_char_desc->box_w);
            glyph->box_w = p_char_desc->box_w;
            glyph->box_h = p_char_desc->box_h;
            glyph->adv = (uint8_t)adv;
            glyph->char_bytes = (uint8_t)char_bytes;
            glyph->has_desc = 1;
            glyph->off_x = p_char_desc->off_x;
            glyph->off_y = p_char_desc->off_y;
            glyph_count++;
        }
        else
        {
            egui_font_std_draw_prefix_glyph_t *glyph = &cache->glyphs[glyph_count];
            int16_t error_w = FONT_ERROR_FONT_SIZE(font->height);

            glyph->idx = 0;
            glyph->x = (int16_t)cursor_x;
            glyph->box_x0 = (int16_t)cursor_x;
            glyph->box_x1 = (int16_t)(cursor_x + error_w);
            glyph->box_w = 0;
            glyph->box_h = 0;
            glyph->adv = (uint8_t)adv;
            glyph->char_bytes = (uint8_t)char_bytes;
            glyph->has_desc = 0;
            glyph->off_x = 0;
            glyph->off_y = 0;
            glyph_count++;
        }

        cursor += char_bytes;
        cached_bytes += char_bytes;
        cursor_x += adv;
    }

    if (*cursor == '\0')
    {
        cache->is_complete_line = 1;
    }

    cache->cached_bytes = (uint16_t)cached_bytes;
    cache->cached_advance = (int16_t)cursor_x;
    cache->glyph_count = (uint16_t)glyph_count;
    return cache;
#endif
}

static int egui_font_std_find_prefix_first_visible(const egui_font_std_draw_prefix_cache_t *cache, int16_t local_x0)
{
    int left = 0;
    int right = cache->glyph_count;

    while (left < right)
    {
        int mid = left + ((right - left) >> 1);

        if (cache->glyphs[mid].box_x1 <= local_x0)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }

    return left;
}
#endif

static void egui_font_std_release_line_cache(void)
{
}

#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) && EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
static egui_font_std_line_cache_storage_t *egui_font_std_get_line_cache_storage(void)
{
    return NULL;
}

static const egui_font_std_line_cache_t *egui_font_std_prepare_line_cache(const char *s)
{
    const char *cursor;
    egui_font_std_line_cache_storage_t *line_cache_storage;
    egui_font_std_line_cache_t *cache;
    uint32_t content_hash;

    if (s == NULL)
    {
        return NULL;
    }

    line_cache_storage = egui_font_std_get_line_cache_storage();
    if (line_cache_storage == NULL)
    {
        return NULL;
    }

    cache = &line_cache_storage->entries[0];
    content_hash = egui_font_std_hash_bytes(s, NULL, 0);

    for (int i = 0; i < EGUI_FONT_STD_LINE_CACHE_SLOTS; i++)
    {
        egui_font_std_line_cache_t *entry = &line_cache_storage->entries[i];

        if (entry->is_ready && entry->string == s && entry->content_hash == content_hash)
        {
            entry->stamp = ++line_cache_storage->stamp;
            return entry;
        }

        if (!entry->is_ready)
        {
            cache = entry;
            break;
        }

        if (cache->is_ready && entry->stamp < cache->stamp)
        {
            cache = entry;
        }
    }

    cache->string = s;
    cache->line_count = 0;
    cache->content_hash = content_hash;
    cache->is_ready = 1;
    cache->is_complete = 0;
    cache->stamp = ++line_cache_storage->stamp;

    cursor = s;
    if (cache->line_count < EGUI_FONT_STD_LINE_CACHE_MAX_LINES)
    {
        cache->lines[cache->line_count++] = cursor;
    }

    while (*cursor != '\0')
    {
        if (*cursor == '\n')
        {
            if (cache->line_count >= EGUI_FONT_STD_LINE_CACHE_MAX_LINES)
            {
                return cache;
            }

            cache->lines[cache->line_count++] = cursor + 1;
        }
        cursor++;
    }

    cache->is_complete = 1;
    return cache;
}
#endif

static void egui_font_std_reset_code_lookup_cache(const egui_font_std_info_t *font)
{
    if (g_font_std_code_lookup_cache.code_array != font->code_array || g_font_std_code_lookup_cache.count != font->count)
    {
        g_font_std_code_lookup_cache.code_array = font->code_array;
        g_font_std_code_lookup_cache.count = egui_font_std_code_lookup_cache_store_count(font->count);
        g_font_std_code_lookup_cache.last_index = EGUI_FONT_STD_CODE_LOOKUP_INDEX_INVALID;
        g_font_std_code_lookup_cache.block_start_index = EGUI_FONT_STD_CODE_LOOKUP_INDEX_INVALID;
        g_font_std_code_lookup_cache.block_end_index = EGUI_FONT_STD_CODE_LOOKUP_INDEX_INVALID;
    }
}

static void egui_font_std_update_code_lookup_cache(const egui_font_std_info_t *font, int code_index)
{
    const egui_font_std_code_descriptor_t *code_array = font->code_array;
    int block_start = code_index;
    int block_end = code_index;

    while (block_start > 0)
    {
        if ((code_array[block_start].code - code_array[block_start - 1].code) != 1)
        {
            break;
        }
        block_start--;
    }

    while (block_end + 1 < font->count)
    {
        if ((code_array[block_end + 1].code - code_array[block_end].code) != 1)
        {
            break;
        }
        block_end++;
    }

    g_font_std_code_lookup_cache.last_code = egui_font_std_code_lookup_cache_store_code(code_array[code_index].code);
    g_font_std_code_lookup_cache.last_index = egui_font_std_code_lookup_cache_store_index(code_index);
    g_font_std_code_lookup_cache.block_start_code = egui_font_std_code_lookup_cache_store_code(code_array[block_start].code);
    g_font_std_code_lookup_cache.block_end_code = egui_font_std_code_lookup_cache_store_code(code_array[block_end].code);
    g_font_std_code_lookup_cache.block_start_index = egui_font_std_code_lookup_cache_store_index(block_start);
    g_font_std_code_lookup_cache.block_end_index = egui_font_std_code_lookup_cache_store_index(block_end);
}

int egui_font_std_find_code_index(const egui_font_std_info_t *font, uint32_t utf8_code)
{
    if (font == NULL || font->count == 0 || font->code_array == NULL)
    {
        return -1;
    }

    egui_font_std_reset_code_lookup_cache(font);

    if (utf8_code < EGUI_FONT_STD_ASCII_CACHE_SIZE)
    {
        const egui_font_std_ascii_lookup_cache_t *ascii_cache = egui_font_std_prepare_ascii_lookup_cache(font);
        if (ascii_cache != NULL)
        {
            return egui_font_std_get_ascii_lookup_index(ascii_cache, (uint8_t)utf8_code);
        }
    }

    if (g_font_std_code_lookup_cache.last_index >= 0)
    {
        if (g_font_std_code_lookup_cache.last_code == utf8_code)
        {
            return g_font_std_code_lookup_cache.last_index;
        }

        if (utf8_code >= g_font_std_code_lookup_cache.block_start_code && utf8_code <= g_font_std_code_lookup_cache.block_end_code)
        {
            int cached_index = g_font_std_code_lookup_cache.block_start_index + (int)(utf8_code - g_font_std_code_lookup_cache.block_start_code);
            if (cached_index <= g_font_std_code_lookup_cache.block_end_index && font->code_array[cached_index].code == utf8_code)
            {
                g_font_std_code_lookup_cache.last_code = utf8_code;
                g_font_std_code_lookup_cache.last_index = cached_index;
                return cached_index;
            }
        }

        if (utf8_code > g_font_std_code_lookup_cache.last_code)
        {
            int next_index = g_font_std_code_lookup_cache.last_index + 1;
            if ((utf8_code - g_font_std_code_lookup_cache.last_code) == 1 && next_index < font->count && font->code_array[next_index].code == utf8_code)
            {
                g_font_std_code_lookup_cache.last_code = utf8_code;
                g_font_std_code_lookup_cache.last_index = next_index;
                return next_index;
            }
        }
        else if (utf8_code < g_font_std_code_lookup_cache.last_code)
        {
            int prev_index = g_font_std_code_lookup_cache.last_index - 1;
            if ((g_font_std_code_lookup_cache.last_code - utf8_code) == 1 && prev_index >= 0 && font->code_array[prev_index].code == utf8_code)
            {
                g_font_std_code_lookup_cache.last_code = utf8_code;
                g_font_std_code_lookup_cache.last_index = prev_index;
                return prev_index;
            }
        }
    }

    int first = 0;
    int last = font->count - 1;

    while (first <= last)
    {
        int middle = (first + last) / 2;
        if (font->code_array[middle].code < utf8_code)
        {
            first = middle + 1;
        }
        else if (font->code_array[middle].code == utf8_code)
        {
            egui_font_std_update_code_lookup_cache(font, middle);
            return middle;
        }
        else
        {
            last = middle - 1;
        }
    }
    return -1;
}

__EGUI_STATIC_INLINE__ const egui_font_std_char_descriptor_t *egui_font_std_get_desc_by_index(const egui_font_std_info_t *font, int code_index,
                                                                                              egui_font_std_char_descriptor_t *external_desc_scratch)
{
    if (font == NULL || code_index < 0 || code_index >= font->count)
    {
        return NULL;
    }

    if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
    {
        return &font->char_array[code_index];
    }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    {
        if (external_desc_scratch == NULL)
        {
            return NULL;
        }

        egui_api_load_external_resource(external_desc_scratch, (egui_uintptr_t)font->char_array, code_index * sizeof(egui_font_std_char_descriptor_t),
                                        sizeof(egui_font_std_char_descriptor_t));
        if (external_desc_scratch->size == 0)
        {
            return NULL;
        }

        return external_desc_scratch;
    }
#else
    {
        (void)external_desc_scratch;
        return NULL;
    }
#endif
}

static const egui_font_std_char_descriptor_t *egui_font_std_get_desc(const egui_font_std_info_t *font, uint32_t utf8_code,
                                                                     egui_font_std_char_descriptor_t *external_desc_scratch)
{
    int code_index = egui_font_std_find_code_index(font, utf8_code);
    if (code_index < 0)
    {
        return NULL;
    }

    return egui_font_std_get_desc_by_index(font, code_index, external_desc_scratch);
}

__EGUI_STATIC_INLINE__ const egui_font_std_char_descriptor_t *egui_font_std_get_desc_fast(const egui_font_std_info_t *font, uint32_t utf8_code,
                                                                                          egui_font_std_char_descriptor_t *external_desc_scratch)
{
    if (font == NULL)
    {
        return NULL;
    }

    if (utf8_code < EGUI_FONT_STD_ASCII_CACHE_SIZE)
    {
        int code_index;
        const egui_font_std_ascii_lookup_cache_t *ascii_cache = egui_font_std_prepare_ascii_lookup_cache(font);

        if (ascii_cache == NULL)
        {
            return egui_font_std_get_desc(font, utf8_code, external_desc_scratch);
        }

        code_index = egui_font_std_get_ascii_lookup_index(ascii_cache, (uint8_t)utf8_code);
        if (code_index < 0)
        {
            return NULL;
        }

        return egui_font_std_get_desc_by_index(font, code_index, external_desc_scratch);
    }

    return egui_font_std_get_desc(font, utf8_code, external_desc_scratch);
}

const egui_font_std_char_descriptor_t *egui_font_std_get_desc_fast_api(const egui_font_std_info_t *font, uint32_t utf8_code,
                                                                       egui_font_std_char_descriptor_t *external_desc_scratch)
{
    return egui_font_std_get_desc_fast(font, utf8_code, external_desc_scratch);
}

__EGUI_STATIC_INLINE__ const egui_font_std_char_descriptor_t *egui_font_std_get_desc_draw_fast(const egui_font_std_info_t *font, uint32_t utf8_code,
                                                                                               egui_font_std_char_descriptor_t *external_desc_scratch)
{
    if (font == NULL)
    {
        return NULL;
    }

    if (utf8_code < EGUI_FONT_STD_ASCII_CACHE_SIZE && font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
    {
        const egui_font_std_ascii_lookup_cache_t *ascii_cache = egui_font_std_prepare_ascii_lookup_cache(font);
        if (ascii_cache != NULL)
        {
            int code_index = egui_font_std_get_ascii_lookup_index(ascii_cache, (uint8_t)utf8_code);
            if (code_index >= 0)
            {
                return &font->char_array[code_index];
            }

            return NULL;
        }
    }

    return egui_font_std_get_desc_fast(font, utf8_code, external_desc_scratch);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE && EGUI_FONT_STD_PREPARE_ACCESS_COPY_PIXEL_BUFFER
static uint32_t egui_font_std_get_external_pixel_total_size(const egui_font_std_info_t *font)
{
    uint32_t max_extent = 0;

    if (font == NULL || font->count == 0)
    {
        return 0;
    }

    for (int i = 0; i < font->count; i++)
    {
        egui_font_std_char_descriptor_t tmp_desc;
        egui_api_load_external_resource(&tmp_desc, (egui_uintptr_t)font->char_array, i * sizeof(egui_font_std_char_descriptor_t),
                                        sizeof(egui_font_std_char_descriptor_t));
        if (tmp_desc.size == 0)
        {
            continue;
        }

        {
            uint32_t extent = tmp_desc.idx + tmp_desc.size;
            if (extent > max_extent)
            {
                max_extent = extent;
            }
        }
    }

    return max_extent;
}
#endif

void egui_font_std_release_frame_cache(void)
{
    egui_font_std_release_ascii_lookup_cache();
    egui_font_std_release_line_cache();
    egui_font_std_external_draw_scratch_release(&g_font_std_compressed_draw_scratch);
}

void egui_font_std_release_access(egui_font_std_access_t *access)
{
    if (access == NULL)
    {
        return;
    }

    if (access->owned_char_array != NULL)
    {
        egui_free(access->owned_char_array);
    }

    if (access->owned_pixel_buffer != NULL)
    {
        egui_free(access->owned_pixel_buffer);
    }

    egui_api_memset(access, 0, sizeof(*access));
}

int egui_font_std_prepare_desc_access(const egui_font_std_info_t *font, egui_font_std_access_t *access)
{
    if (font == NULL || access == NULL)
    {
        return -1;
    }

    egui_api_memset(access, 0, sizeof(*access));
    access->info = *font;

    if (font->res_type != EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        return 0;
    }

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    {
        uint32_t char_array_size = font->count * sizeof(egui_font_std_char_descriptor_t);
        const egui_font_std_char_descriptor_t *char_array;

        access->owned_char_array = (egui_font_std_char_descriptor_t *)egui_malloc(char_array_size);
        if (access->owned_char_array == NULL)
        {
            egui_font_std_release_access(access);
            return -1;
        }

        egui_api_load_external_resource(access->owned_char_array, (egui_uintptr_t)font->char_array, 0, char_array_size);
        char_array = access->owned_char_array;
        access->info.char_array = char_array;
        return 0;
    }
#else
    {
        egui_font_std_release_access(access);
        return -1;
    }
#endif
}

int egui_font_std_prepare_access(const egui_font_std_info_t *font, egui_font_std_access_t *access)
{
    if (egui_font_std_prepare_desc_access(font, access) != 0)
    {
        return -1;
    }

    if (font == NULL || access == NULL)
    {
        return -1;
    }

    if (font->res_type != EGUI_RESOURCE_TYPE_EXTERNAL)
    {
        return 0;
    }

#if !EGUI_FONT_STD_PREPARE_ACCESS_COPY_PIXEL_BUFFER
    return 0;
#else
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    {
        uint32_t pixel_size = egui_font_std_get_external_pixel_total_size(font);
        if (pixel_size == 0)
        {
            egui_font_std_release_access(access);
            return -1;
        }

        access->owned_pixel_buffer = (uint8_t *)egui_malloc(pixel_size);
        if (access->owned_pixel_buffer == NULL)
        {
            egui_font_std_release_access(access);
            return -1;
        }

        egui_api_load_external_resource(access->owned_pixel_buffer, (egui_uintptr_t)font->pixel_buffer, 0, pixel_size);
        access->info.pixel_buffer = access->owned_pixel_buffer;
        access->info.res_type = EGUI_RESOURCE_TYPE_INTERNAL;
        return 0;
    }
#else
    {
        egui_font_std_release_access(access);
        return -1;
    }
#endif
#endif
}

__EGUI_STATIC_INLINE__ void egui_font_std_blend_pixel(egui_color_int_t *dst, egui_color_t color, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        *dst = color.full;
    }
    else
    {
        egui_rgb_mix_ptr((egui_color_t *)dst, &color, (egui_color_t *)dst, alpha);
    }
}

typedef struct
{
    egui_color_t color;
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    uint32_t color_rb_g;
#endif
} egui_font_std_blend_ctx_t;

__EGUI_STATIC_INLINE__ void egui_font_std_blend_ctx_init(egui_font_std_blend_ctx_t *ctx, egui_color_t color)
{
    ctx->color = color;
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    {
        uint16_t fg = color.full;
        ctx->color_rb_g = (fg | ((uint32_t)fg << 16)) & 0x07E0F81FUL;
    }
#endif
}

__EGUI_STATIC_INLINE__ void egui_font_std_blend_pixel_ctx(egui_color_int_t *dst, const egui_font_std_blend_ctx_t *ctx, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    if (alpha > 251)
    {
        *dst = ctx->color.full;
        return;
    }

    if (alpha < 4)
    {
        return;
    }

    {
        uint16_t bg = *dst;
        uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
        uint32_t result = (bg_rb_g + ((ctx->color_rb_g - bg_rb_g) * ((uint32_t)alpha >> 3) >> 5)) & 0x07E0F81FUL;
        *dst = (uint16_t)(result | (result >> 16));
    }
#else
    egui_font_std_blend_pixel(dst, ctx->color, alpha);
#endif
}

__EGUI_STATIC_INLINE__ void egui_font_std_blend_pixel_ctx_partial(egui_color_int_t *dst, const egui_font_std_blend_ctx_t *ctx, egui_alpha_t alpha)
{
#if (EGUI_CONFIG_COLOR_DEPTH == 16)
    uint16_t bg = *dst;
    uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
    uint32_t result = (bg_rb_g + ((ctx->color_rb_g - bg_rb_g) * ((uint32_t)alpha >> 3) >> 5)) & 0x07E0F81FUL;
    *dst = (uint16_t)(result | (result >> 16));
#else
    egui_font_std_blend_pixel(dst, ctx->color, alpha);
#endif
}

#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) && EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
static int egui_font_std_clip_to_region(const egui_region_t *region, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height,
                                        egui_region_t *visible_rect)
{
    egui_dim_t vis_x0 = x;
    egui_dim_t vis_y0 = y;
    egui_dim_t vis_x1 = x + width;
    egui_dim_t vis_y1 = y + height;

    if (width <= 0 || height <= 0)
    {
        return 0;
    }

    if (vis_x0 < region->location.x)
    {
        vis_x0 = region->location.x;
    }
    if (vis_y0 < region->location.y)
    {
        vis_y0 = region->location.y;
    }
    if (vis_x1 > region->location.x + region->size.width)
    {
        vis_x1 = region->location.x + region->size.width;
    }
    if (vis_y1 > region->location.y + region->size.height)
    {
        vis_y1 = region->location.y + region->size.height;
    }

    if (vis_x0 >= vis_x1 || vis_y0 >= vis_y1)
    {
        return 0;
    }

    visible_rect->location.x = vis_x0;
    visible_rect->location.y = vis_y0;
    visible_rect->size.width = vis_x1 - vis_x0;
    visible_rect->size.height = vis_y1 - vis_y0;
    return 1;
}

static int egui_font_std_clip_to_work_region(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_region_t *visible_rect)
{
    return egui_font_std_clip_to_region(egui_canvas_get_base_view_work_region(), x, y, width, height, visible_rect);
}

static int egui_font_std_can_fast_draw(const egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t visible_rect;

    if (canvas->mask != NULL)
    {
        return 0;
    }

    if (!egui_font_std_clip_to_work_region(x, y, width, height, &visible_rect))
    {
        return 0;
    }

    if (visible_rect.location.x != x || visible_rect.location.y != y || visible_rect.size.width != width || visible_rect.size.height != height)
    {
        return 0;
    }

    // Extra safety: verify the PFB index is within bounds
    egui_dim_t pfb_x = x - canvas->pfb_location_in_base_view.x;
    egui_dim_t pfb_y = y - canvas->pfb_location_in_base_view.y;
    egui_dim_t pfb_w = canvas->pfb_region.size.width;
    egui_dim_t pfb_h = canvas->pfb_region.size.height;
    if (pfb_x < 0 || pfb_y < 0 || pfb_x + width > pfb_w || pfb_y + height > pfb_h)
    {
        return 0;
    }

    return 1;
}

#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
static void egui_font_std_draw_fast_1(const egui_canvas_t *canvas, egui_dim_t pfb_x, egui_dim_t pfb_y, egui_dim_t src_width, egui_dim_t src_col0,
                                      egui_dim_t src_row0, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    uint16_t row_stride = (src_width + 7) >> 3;

    for (egui_dim_t row = 0; row < height; row++)
    {
        const uint8_t *src = p_data + (src_row0 + row) * row_stride;
        egui_color_int_t *dst = &canvas->pfb[(pfb_y + row) * pfb_width + pfb_x];

        for (egui_dim_t col = 0; col < width; col++)
        {
            egui_dim_t src_col = src_col0 + col;
            if ((src[src_col >> 3] >> (src_col & 0x07)) & 0x01)
            {
                egui_font_std_blend_pixel(&dst[col], color, alpha);
            }
        }
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
static void egui_font_std_draw_fast_2(const egui_canvas_t *canvas, egui_dim_t pfb_x, egui_dim_t pfb_y, egui_dim_t src_width, egui_dim_t src_col0,
                                      egui_dim_t src_row0, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    uint16_t row_stride = (src_width + 3) >> 2;

    for (egui_dim_t row = 0; row < height; row++)
    {
        const uint8_t *src = p_data + (src_row0 + row) * row_stride;
        egui_color_int_t *dst = &canvas->pfb[(pfb_y + row) * pfb_width + pfb_x];

        for (egui_dim_t col = 0; col < width; col++)
        {
            egui_dim_t src_col = src_col0 + col;
            uint8_t sel_value = (src[src_col >> 2] >> ((src_col & 0x03) << 1)) & 0x03;
            egui_alpha_t pixel_alpha = egui_alpha_change_table_2[sel_value];

            if (pixel_alpha != 0)
            {
                egui_font_std_blend_pixel(&dst[col], color, egui_color_alpha_mix(alpha, pixel_alpha));
            }
        }
    }
}
#endif

static void egui_font_std_draw_fast_4_ctx(const egui_canvas_t *canvas, egui_dim_t pfb_x, egui_dim_t pfb_y, egui_dim_t src_width, egui_dim_t src_col0,
                                          egui_dim_t src_row0, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_alpha_t alpha,
                                          const egui_font_std_blend_ctx_t *blend_ctx)
{
    egui_dim_t pfb_width = canvas->pfb_region.size.width;
    uint16_t row_stride = (src_width + 1) >> 1;
    egui_alpha_t mixed_alpha_table[16];

    if (alpha != EGUI_ALPHA_100)
    {
        for (uint8_t i = 0; i < 16; i++)
        {
            mixed_alpha_table[i] = egui_color_alpha_mix(alpha, egui_alpha_change_table_4[i]);
        }
    }

    if (src_col0 == 0 && width == src_width)
    {
        for (egui_dim_t row = 0; row < height; row++)
        {
            const uint8_t *src = p_data + (src_row0 + row) * row_stride;
            egui_color_int_t *dst = &canvas->pfb[(pfb_y + row) * pfb_width + pfb_x];
            egui_dim_t col = 0;

            if (alpha == EGUI_ALPHA_100)
            {
                while (col + 1 < width)
                {
                    uint8_t packed = *src++;

                    if (packed == 0)
                    {
                        col += 2;
                        continue;
                    }

                    if (packed == 0xFF)
                    {
                        dst[col] = blend_ctx->color.full;
                        dst[col + 1] = blend_ctx->color.full;
                        col += 2;
                        continue;
                    }

                    {
                        uint8_t alpha0 = packed & 0x0F;
                        uint8_t alpha1 = (packed >> 4) & 0x0F;

                        if (alpha0 == 0x0F)
                        {
                            dst[col] = blend_ctx->color.full;
                        }
                        else if (alpha0 != 0)
                        {
                            egui_font_std_blend_pixel_ctx_partial(&dst[col], blend_ctx, egui_alpha_change_table_4[alpha0]);
                        }

                        if (alpha1 == 0x0F)
                        {
                            dst[col + 1] = blend_ctx->color.full;
                        }
                        else if (alpha1 != 0)
                        {
                            egui_font_std_blend_pixel_ctx_partial(&dst[col + 1], blend_ctx, egui_alpha_change_table_4[alpha1]);
                        }
                    }

                    col += 2;
                }

                if (col < width)
                {
                    uint8_t packed = *src & 0x0F;

                    if (packed == 0x0F)
                    {
                        dst[col] = blend_ctx->color.full;
                    }
                    else if (packed != 0)
                    {
                        egui_font_std_blend_pixel_ctx_partial(&dst[col], blend_ctx, egui_alpha_change_table_4[packed]);
                    }
                }
            }
            else
            {
                while (col + 1 < width)
                {
                    uint8_t packed = *src++;

                    if (packed == 0)
                    {
                        col += 2;
                        continue;
                    }

                    egui_alpha_t alpha0 = mixed_alpha_table[packed & 0x0F];
                    egui_alpha_t alpha1 = mixed_alpha_table[(packed >> 4) & 0x0F];

                    if (alpha0 != 0)
                    {
                        egui_font_std_blend_pixel_ctx(&dst[col], blend_ctx, alpha0);
                    }
                    if (alpha1 != 0)
                    {
                        egui_font_std_blend_pixel_ctx(&dst[col + 1], blend_ctx, alpha1);
                    }
                    col += 2;
                }

                if (col < width)
                {
                    egui_alpha_t alpha0 = mixed_alpha_table[*src & 0x0F];
                    if (alpha0 != 0)
                    {
                        egui_font_std_blend_pixel_ctx(&dst[col], blend_ctx, alpha0);
                    }
                }
            }
        }
        return;
    }

    for (egui_dim_t row = 0; row < height; row++)
    {
        const uint8_t *src = p_data + (src_row0 + row) * row_stride;
        egui_color_int_t *dst = &canvas->pfb[(pfb_y + row) * pfb_width + pfb_x];
        egui_dim_t col = 0;
        egui_dim_t src_col = src_col0;

        if (alpha == EGUI_ALPHA_100)
        {
            if ((src_col & 0x01) != 0 && col < width)
            {
                uint8_t sel_value = (src[src_col >> 1] >> 4) & 0x0F;

                if (sel_value == 0x0F)
                {
                    dst[col] = blend_ctx->color.full;
                }
                else if (sel_value != 0)
                {
                    egui_font_std_blend_pixel_ctx_partial(&dst[col], blend_ctx, egui_alpha_change_table_4[sel_value]);
                }

                col++;
                src_col++;
            }

            {
                const uint8_t *src_byte = src + (src_col >> 1);

                while (col + 1 < width)
                {
                    uint8_t packed = *src_byte++;

                    if (packed == 0)
                    {
                        col += 2;
                        continue;
                    }

                    if (packed == 0xFF)
                    {
                        dst[col] = blend_ctx->color.full;
                        dst[col + 1] = blend_ctx->color.full;
                        col += 2;
                        continue;
                    }

                    {
                        uint8_t alpha0 = packed & 0x0F;
                        uint8_t alpha1 = (packed >> 4) & 0x0F;

                        if (alpha0 == 0x0F)
                        {
                            dst[col] = blend_ctx->color.full;
                        }
                        else if (alpha0 != 0)
                        {
                            egui_font_std_blend_pixel_ctx_partial(&dst[col], blend_ctx, egui_alpha_change_table_4[alpha0]);
                        }

                        if (alpha1 == 0x0F)
                        {
                            dst[col + 1] = blend_ctx->color.full;
                        }
                        else if (alpha1 != 0)
                        {
                            egui_font_std_blend_pixel_ctx_partial(&dst[col + 1], blend_ctx, egui_alpha_change_table_4[alpha1]);
                        }
                    }

                    col += 2;
                }

                if (col < width)
                {
                    uint8_t sel_value = *src_byte & 0x0F;

                    if (sel_value == 0x0F)
                    {
                        dst[col] = blend_ctx->color.full;
                    }
                    else if (sel_value != 0)
                    {
                        egui_font_std_blend_pixel_ctx_partial(&dst[col], blend_ctx, egui_alpha_change_table_4[sel_value]);
                    }
                }
            }
        }
        else
        {
            if ((src_col & 0x01) != 0 && col < width)
            {
                egui_alpha_t pixel_alpha = mixed_alpha_table[(src[src_col >> 1] >> 4) & 0x0F];

                if (pixel_alpha != 0)
                {
                    egui_font_std_blend_pixel_ctx(&dst[col], blend_ctx, pixel_alpha);
                }

                col++;
                src_col++;
            }

            {
                const uint8_t *src_byte = src + (src_col >> 1);

                while (col + 1 < width)
                {
                    uint8_t packed = *src_byte++;

                    if (packed == 0)
                    {
                        col += 2;
                        continue;
                    }

                    {
                        egui_alpha_t alpha0 = mixed_alpha_table[packed & 0x0F];
                        egui_alpha_t alpha1 = mixed_alpha_table[(packed >> 4) & 0x0F];

                        if (alpha0 != 0)
                        {
                            egui_font_std_blend_pixel_ctx(&dst[col], blend_ctx, alpha0);
                        }
                        if (alpha1 != 0)
                        {
                            egui_font_std_blend_pixel_ctx(&dst[col + 1], blend_ctx, alpha1);
                        }
                    }

                    col += 2;
                }

                if (col < width)
                {
                    egui_alpha_t pixel_alpha = mixed_alpha_table[*src_byte & 0x0F];

                    if (pixel_alpha != 0)
                    {
                        egui_font_std_blend_pixel_ctx(&dst[col], blend_ctx, pixel_alpha);
                    }
                }
            }
        }
    }
}

static void egui_font_std_draw_fast_4(const egui_canvas_t *canvas, egui_dim_t pfb_x, egui_dim_t pfb_y, egui_dim_t src_width, egui_dim_t src_col0,
                                      egui_dim_t src_row0, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    egui_font_std_blend_ctx_t blend_ctx;

    egui_font_std_blend_ctx_init(&blend_ctx, color);
    egui_font_std_draw_fast_4_ctx(canvas, pfb_x, pfb_y, src_width, src_col0, src_row0, width, height, p_data, alpha, &blend_ctx);
}

#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
static void egui_font_std_draw_fast_8(const egui_canvas_t *canvas, egui_dim_t pfb_x, egui_dim_t pfb_y, egui_dim_t src_width, egui_dim_t src_col0,
                                      egui_dim_t src_row0, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t pfb_width = canvas->pfb_region.size.width;

    for (egui_dim_t row = 0; row < height; row++)
    {
        const uint8_t *src = p_data + (src_row0 + row) * src_width;
        egui_color_int_t *dst = &canvas->pfb[(pfb_y + row) * pfb_width + pfb_x];

        for (egui_dim_t col = 0; col < width; col++)
        {
            if (src[src_col0 + col] != 0)
            {
                egui_font_std_blend_pixel(&dst[col], color, egui_color_alpha_mix(alpha, src[src_col0 + col]));
            }
        }
    }
}
#endif

static int egui_font_std_draw_fast_4_prepared(const egui_canvas_t *canvas, const egui_region_t *work_region, egui_dim_t x, egui_dim_t y, egui_dim_t width,
                                              egui_dim_t height, const uint8_t *p_data, egui_alpha_t draw_alpha, const egui_font_std_blend_ctx_t *blend_ctx)
{
    egui_dim_t work_x0;
    egui_dim_t work_y0;
    egui_dim_t work_x1;
    egui_dim_t work_y1;
    egui_dim_t vis_x0;
    egui_dim_t vis_y0;
    egui_dim_t vis_x1;
    egui_dim_t vis_y1;
    egui_dim_t pfb_x;
    egui_dim_t pfb_y;
    egui_dim_t col0;
    egui_dim_t row0;
    egui_dim_t draw_w;
    egui_dim_t draw_h;

    if (draw_alpha == 0 || width <= 0 || height <= 0)
    {
        return 1;
    }

    work_x0 = work_region->location.x;
    work_y0 = work_region->location.y;
    work_x1 = work_x0 + work_region->size.width;
    work_y1 = work_y0 + work_region->size.height;

    vis_x0 = (x > work_x0) ? x : work_x0;
    vis_y0 = (y > work_y0) ? y : work_y0;
    vis_x1 = ((x + width) < work_x1) ? (x + width) : work_x1;
    vis_y1 = ((y + height) < work_y1) ? (y + height) : work_y1;

    if (vis_x0 >= vis_x1 || vis_y0 >= vis_y1)
    {
        return 1;
    }

    draw_w = vis_x1 - vis_x0;
    draw_h = vis_y1 - vis_y0;

    pfb_x = vis_x0 - canvas->pfb_location_in_base_view.x;
    pfb_y = vis_y0 - canvas->pfb_location_in_base_view.y;

    if (pfb_x < 0 || pfb_y < 0 || pfb_x + draw_w > canvas->pfb_region.size.width || pfb_y + draw_h > canvas->pfb_region.size.height)
    {
        return 0;
    }

    col0 = vis_x0 - x;
    row0 = vis_y0 - y;

    egui_font_std_draw_fast_4_ctx(canvas, pfb_x, pfb_y, width, col0, row0, draw_w, draw_h, p_data, draw_alpha, blend_ctx);
    return 1;
}

static int egui_font_std_draw_fast(const egui_font_std_info_t *font, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data,
                                   egui_color_t color, egui_alpha_t alpha)
{
    const egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_alpha_t draw_alpha = egui_color_alpha_mix(canvas->alpha, alpha);
    egui_dim_t pfb_x = x - canvas->pfb_location_in_base_view.x;
    egui_dim_t pfb_y = y - canvas->pfb_location_in_base_view.y;

    if (!egui_font_std_can_fast_draw(canvas, x, y, width, height) || draw_alpha == 0)
    {
        return 0;
    }

    switch (font->font_bit_mode)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
        egui_font_std_draw_fast_1(canvas, pfb_x, pfb_y, width, 0, 0, width, height, p_data, color, draw_alpha);
        return 1;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
        egui_font_std_draw_fast_2(canvas, pfb_x, pfb_y, width, 0, 0, width, height, p_data, color, draw_alpha);
        return 1;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
        egui_font_std_draw_fast_4(canvas, pfb_x, pfb_y, width, 0, 0, width, height, p_data, color, draw_alpha);
        return 1;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
        egui_font_std_draw_fast_8(canvas, pfb_x, pfb_y, width, 0, 0, width, height, p_data, color, draw_alpha);
        return 1;
#endif
    default:
        break;
    }

    return 0;
}

static int egui_font_std_draw_fast_clipped(const egui_font_std_info_t *font, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height,
                                           const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    const egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_region_t visible_rect;
    egui_alpha_t draw_alpha = egui_color_alpha_mix(canvas->alpha, alpha);

    if (canvas->mask != NULL)
    {
        return 0;
    }

    if (draw_alpha == 0 || width <= 0 || height <= 0)
    {
        return 1;
    }

    if (!egui_font_std_clip_to_work_region(x, y, width, height, &visible_rect))
    {
        return 1;
    }

    egui_dim_t col0 = visible_rect.location.x - x;
    egui_dim_t row0 = visible_rect.location.y - y;
    egui_dim_t pfb_x = visible_rect.location.x - canvas->pfb_location_in_base_view.x;
    egui_dim_t pfb_y = visible_rect.location.y - canvas->pfb_location_in_base_view.y;

    if (pfb_x < 0 || pfb_y < 0 || pfb_x + visible_rect.size.width > canvas->pfb_region.size.width ||
        pfb_y + visible_rect.size.height > canvas->pfb_region.size.height)
    {
        return 0;
    }

    switch (font->font_bit_mode)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
        egui_font_std_draw_fast_1(canvas, pfb_x, pfb_y, width, col0, row0, visible_rect.size.width, visible_rect.size.height, p_data, color, draw_alpha);
        return 1;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
        egui_font_std_draw_fast_2(canvas, pfb_x, pfb_y, width, col0, row0, visible_rect.size.width, visible_rect.size.height, p_data, color, draw_alpha);
        return 1;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
        egui_font_std_draw_fast_4(canvas, pfb_x, pfb_y, width, col0, row0, visible_rect.size.width, visible_rect.size.height, p_data, color, draw_alpha);
        return 1;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
        egui_font_std_draw_fast_8(canvas, pfb_x, pfb_y, width, col0, row0, visible_rect.size.width, visible_rect.size.height, p_data, color, draw_alpha);
        return 1;
#endif
    default:
        return 0;
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK && (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE)
// Fast path for font rendering with masks that support row-level color blend
// (e.g., LINEAR_VERTICAL gradient masks). Avoids per-pixel mask_point vtable dispatch.
// Handles clipping to PFB work region, so partially-visible glyphs are covered too.
static int egui_font_std_draw_fast_mask(const egui_font_std_info_t *font, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height,
                                        const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    const egui_canvas_t *canvas = egui_canvas_get_canvas();
    egui_mask_t *mask = canvas->mask;
    egui_region_t visible_rect;

    if (mask == NULL || mask->api->mask_blend_row_color == NULL)
    {
        return 0;
    }

    egui_alpha_t draw_alpha = egui_color_alpha_mix(canvas->alpha, alpha);
    if (draw_alpha == 0)
    {
        return 0;
    }

    if (width <= 0 || height <= 0)
    {
        return 1;
    }

    if (!egui_font_std_clip_to_work_region(x, y, width, height, &visible_rect))
    {
        return 1; // fully clipped, nothing to draw
    }

    // Glyph data offsets for the visible portion
    egui_dim_t col0 = visible_rect.location.x - x;
    egui_dim_t row0 = visible_rect.location.y - y;
    egui_dim_t col1 = col0 + visible_rect.size.width;
    egui_dim_t row1 = row0 + visible_rect.size.height;

    // PFB coordinates for the visible top-left
    egui_dim_t pfb_x0 = visible_rect.location.x - canvas->pfb_location_in_base_view.x;
    egui_dim_t pfb_y0 = visible_rect.location.y - canvas->pfb_location_in_base_view.y;
    egui_dim_t pfb_w = canvas->pfb_region.size.width;

    if (pfb_x0 < 0 || pfb_y0 < 0 || pfb_x0 + visible_rect.size.width > canvas->pfb_region.size.width ||
        pfb_y0 + visible_rect.size.height > canvas->pfb_region.size.height)
    {
        return 0;
    }

    // Probe: check if mask supports row-level blend for this configuration
    egui_color_t probe = color;
    if (!mask->api->mask_blend_row_color(mask, visible_rect.location.y, &probe))
    {
        return 0;
    }

    switch (font->font_bit_mode)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
    {
        uint16_t row_stride = (width + 7) >> 3;
        for (egui_dim_t row = row0; row < row1; row++)
        {
            egui_color_t row_color = color;
            mask->api->mask_blend_row_color(mask, y + row, &row_color);
            const uint8_t *src = p_data + row * row_stride;
            egui_color_int_t *dst = &canvas->pfb[(pfb_y0 + row - row0) * pfb_w + pfb_x0];
            for (egui_dim_t col = col0; col < col1; col++)
            {
                if (src[col >> 3] & (1 << (col & 7)))
                {
                    egui_font_std_blend_pixel(&dst[col - col0], row_color, draw_alpha);
                }
            }
        }
        return 1;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
    {
        uint16_t row_stride = (width + 3) >> 2;
        for (egui_dim_t row = row0; row < row1; row++)
        {
            egui_color_t row_color = color;
            mask->api->mask_blend_row_color(mask, y + row, &row_color);
            const uint8_t *src = p_data + row * row_stride;
            egui_color_int_t *dst = &canvas->pfb[(pfb_y0 + row - row0) * pfb_w + pfb_x0];
            for (egui_dim_t col = col0; col < col1; col++)
            {
                uint8_t sel_value = (src[col >> 2] >> ((col & 0x03) << 1)) & 0x03;
                egui_alpha_t pixel_alpha = egui_alpha_change_table_2[sel_value];
                if (pixel_alpha != 0)
                {
                    egui_font_std_blend_pixel(&dst[col - col0], row_color, egui_color_alpha_mix(draw_alpha, pixel_alpha));
                }
            }
        }
        return 1;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
    {
        uint16_t row_stride = (width + 1) >> 1;
        egui_alpha_t mixed_alpha_table[16];

        if (draw_alpha != EGUI_ALPHA_100)
        {
            for (uint8_t i = 0; i < 16; i++)
            {
                mixed_alpha_table[i] = egui_color_alpha_mix(draw_alpha, egui_alpha_change_table_4[i]);
            }
        }

        for (egui_dim_t row = row0; row < row1; row++)
        {
            egui_color_t row_color = color;
            egui_font_std_blend_ctx_t blend_ctx;
            mask->api->mask_blend_row_color(mask, y + row, &row_color);
            const uint8_t *src = p_data + row * row_stride;
            egui_color_int_t *dst = &canvas->pfb[(pfb_y0 + row - row0) * pfb_w + pfb_x0];
            egui_font_std_blend_ctx_init(&blend_ctx, row_color);

            if (col0 == 0 && col1 == width)
            {
                egui_dim_t col = 0;

                if (draw_alpha == EGUI_ALPHA_100)
                {
                    while (col + 1 < width)
                    {
                        uint8_t packed = *src++;

                        if (packed == 0)
                        {
                            col += 2;
                            continue;
                        }

                        if (packed == 0xFF)
                        {
                            dst[col] = blend_ctx.color.full;
                            dst[col + 1] = blend_ctx.color.full;
                            col += 2;
                            continue;
                        }

                        egui_alpha_t alpha0 = egui_alpha_change_table_4[packed & 0x0F];
                        egui_alpha_t alpha1 = egui_alpha_change_table_4[(packed >> 4) & 0x0F];

                        if (alpha0 != 0)
                        {
                            egui_font_std_blend_pixel_ctx_partial(&dst[col], &blend_ctx, alpha0);
                        }
                        if (alpha1 != 0)
                        {
                            egui_font_std_blend_pixel_ctx_partial(&dst[col + 1], &blend_ctx, alpha1);
                        }
                        col += 2;
                    }

                    if (col < width)
                    {
                        egui_alpha_t alpha0 = egui_alpha_change_table_4[*src & 0x0F];
                        if (alpha0 != 0)
                        {
                            egui_font_std_blend_pixel_ctx_partial(&dst[col], &blend_ctx, alpha0);
                        }
                    }
                }
                else
                {
                    while (col + 1 < width)
                    {
                        uint8_t packed = *src++;

                        if (packed == 0)
                        {
                            col += 2;
                            continue;
                        }

                        egui_alpha_t alpha0 = mixed_alpha_table[packed & 0x0F];
                        egui_alpha_t alpha1 = mixed_alpha_table[(packed >> 4) & 0x0F];

                        if (alpha0 != 0)
                        {
                            egui_font_std_blend_pixel_ctx(&dst[col], &blend_ctx, alpha0);
                        }
                        if (alpha1 != 0)
                        {
                            egui_font_std_blend_pixel_ctx(&dst[col + 1], &blend_ctx, alpha1);
                        }
                        col += 2;
                    }

                    if (col < width)
                    {
                        egui_alpha_t alpha0 = mixed_alpha_table[*src & 0x0F];
                        if (alpha0 != 0)
                        {
                            egui_font_std_blend_pixel_ctx(&dst[col], &blend_ctx, alpha0);
                        }
                    }
                }
            }
            else
            {
                egui_dim_t draw_w = col1 - col0;
                egui_dim_t dst_col = 0;
                egui_dim_t src_col = col0;

                if (draw_alpha == EGUI_ALPHA_100)
                {
                    if ((src_col & 0x01) != 0 && dst_col < draw_w)
                    {
                        uint8_t sel_value = (src[src_col >> 1] >> 4) & 0x0F;

                        if (sel_value == 0x0F)
                        {
                            dst[dst_col] = blend_ctx.color.full;
                        }
                        else if (sel_value != 0)
                        {
                            egui_font_std_blend_pixel_ctx_partial(&dst[dst_col], &blend_ctx, egui_alpha_change_table_4[sel_value]);
                        }

                        dst_col++;
                        src_col++;
                    }

                    {
                        const uint8_t *src_byte = src + (src_col >> 1);

                        while (dst_col + 1 < draw_w)
                        {
                            uint8_t packed = *src_byte++;

                            if (packed == 0)
                            {
                                dst_col += 2;
                                continue;
                            }

                            if (packed == 0xFF)
                            {
                                dst[dst_col] = blend_ctx.color.full;
                                dst[dst_col + 1] = blend_ctx.color.full;
                                dst_col += 2;
                                continue;
                            }

                            {
                                egui_alpha_t alpha0 = egui_alpha_change_table_4[packed & 0x0F];
                                egui_alpha_t alpha1 = egui_alpha_change_table_4[(packed >> 4) & 0x0F];

                                if (alpha0 != 0)
                                {
                                    egui_font_std_blend_pixel_ctx_partial(&dst[dst_col], &blend_ctx, alpha0);
                                }
                                if (alpha1 != 0)
                                {
                                    egui_font_std_blend_pixel_ctx_partial(&dst[dst_col + 1], &blend_ctx, alpha1);
                                }
                            }

                            dst_col += 2;
                        }

                        if (dst_col < draw_w)
                        {
                            egui_alpha_t alpha0 = egui_alpha_change_table_4[*src_byte & 0x0F];
                            if (alpha0 != 0)
                            {
                                egui_font_std_blend_pixel_ctx_partial(&dst[dst_col], &blend_ctx, alpha0);
                            }
                        }
                    }
                }
                else
                {
                    if ((src_col & 0x01) != 0 && dst_col < draw_w)
                    {
                        egui_alpha_t pixel_alpha = mixed_alpha_table[(src[src_col >> 1] >> 4) & 0x0F];

                        if (pixel_alpha != 0)
                        {
                            egui_font_std_blend_pixel_ctx(&dst[dst_col], &blend_ctx, pixel_alpha);
                        }

                        dst_col++;
                        src_col++;
                    }

                    {
                        const uint8_t *src_byte = src + (src_col >> 1);

                        while (dst_col + 1 < draw_w)
                        {
                            uint8_t packed = *src_byte++;

                            if (packed == 0)
                            {
                                dst_col += 2;
                                continue;
                            }

                            {
                                egui_alpha_t alpha0 = mixed_alpha_table[packed & 0x0F];
                                egui_alpha_t alpha1 = mixed_alpha_table[(packed >> 4) & 0x0F];

                                if (alpha0 != 0)
                                {
                                    egui_font_std_blend_pixel_ctx(&dst[dst_col], &blend_ctx, alpha0);
                                }
                                if (alpha1 != 0)
                                {
                                    egui_font_std_blend_pixel_ctx(&dst[dst_col + 1], &blend_ctx, alpha1);
                                }
                            }

                            dst_col += 2;
                        }

                        if (dst_col < draw_w)
                        {
                            egui_alpha_t pixel_alpha = mixed_alpha_table[*src_byte & 0x0F];
                            if (pixel_alpha != 0)
                            {
                                egui_font_std_blend_pixel_ctx(&dst[dst_col], &blend_ctx, pixel_alpha);
                            }
                        }
                    }
                }
            }
        }
        return 1;
    }
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
    {
        for (egui_dim_t row = row0; row < row1; row++)
        {
            egui_color_t row_color = color;
            mask->api->mask_blend_row_color(mask, y + row, &row_color);
            const uint8_t *src = p_data + row * width;
            egui_color_int_t *dst = &canvas->pfb[(pfb_y0 + row - row0) * pfb_w + pfb_x0];
            for (egui_dim_t col = col0; col < col1; col++)
            {
                if (src[col] != 0)
                {
                    egui_font_std_blend_pixel(&dst[col - col0], row_color, egui_color_alpha_mix(draw_alpha, src[col]));
                }
            }
        }
        return 1;
    }
#endif
    default:
        return 0;
    }
}
#endif
#endif

#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
static void egui_font_std_draw_1(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    uint8_t bit_pos;
    uint8_t sel_data;
    for (egui_dim_t y_ = 0; y_ < height; y_++)
    {
        bit_pos = 0;
        for (egui_dim_t x_ = 0; x_ < width; x_++)
        {
            // update bytes position.
            if (bit_pos == 0)
            {
                sel_data = *p_data++;
            }

            if (sel_data & (1 << bit_pos))
            {
                egui_canvas_draw_point((x + x_), (y + y_), color, alpha);
            }

            // update bit position.
            bit_pos++;
            if (bit_pos == 8)
            {
                bit_pos = 0;
            }
        }
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
static void egui_font_std_draw_2(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    uint8_t bit_pos;
    uint8_t sel_data;
    uint8_t sel_value;
    for (egui_dim_t y_ = 0; y_ < height; y_++)
    {
        bit_pos = 0;
        for (egui_dim_t x_ = 0; x_ < width; x_++)
        {
            // update bytes position.
            if (bit_pos == 0)
            {
                sel_data = *p_data++;
            }

            sel_value = (sel_data >> bit_pos) & 0x03;
            if (sel_value)
            {
                if (alpha == EGUI_ALPHA_100)
                {
                    egui_canvas_draw_point((x + x_), (y + y_), color, egui_alpha_change_table_2[sel_value]);
                }
                else
                {
                    egui_canvas_draw_point((x + x_), (y + y_), color, egui_alpha_change_table_2[sel_value] * alpha / EGUI_ALPHA_100);
                }
            }

            // update bit position.
            bit_pos += 2;
            if (bit_pos == 8)
            {
                bit_pos = 0;
            }
        }
    }
}
#endif

static void egui_font_std_draw_4(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    uint8_t bit_pos;
    uint8_t sel_data;
    uint8_t sel_value;
    for (egui_dim_t y_ = 0; y_ < height; y_++)
    {
        bit_pos = 0;
        for (egui_dim_t x_ = 0; x_ < width; x_++)
        {
            // update bytes position.
            if (bit_pos == 0)
            {
                sel_data = *p_data++;
            }

            sel_value = (sel_data >> bit_pos) & 0x0F;
            if (sel_value)
            {
                if (alpha == EGUI_ALPHA_100)
                {
                    egui_canvas_draw_point((x + x_), (y + y_), color, egui_alpha_change_table_4[sel_value]);
                }
                else
                {
                    egui_canvas_draw_point((x + x_), (y + y_), color, egui_alpha_change_table_4[sel_value] * alpha / EGUI_ALPHA_100);
                }
            }

            // update bit position.
            bit_pos += 4;
            if (bit_pos == 8)
            {
                bit_pos = 0;
            }
        }
    }
}

#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
static void egui_font_std_draw_8(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const uint8_t *p_data, egui_color_t color, egui_alpha_t alpha)
{
    uint16_t sel_value;
    for (egui_dim_t y_ = 0; y_ < height; y_++)
    {
        for (egui_dim_t x_ = 0; x_ < width; x_++)
        {
            sel_value = *p_data++;

            if (sel_value)
            {
                if (alpha == EGUI_ALPHA_100)
                {
                    egui_canvas_draw_point((x + x_), (y + y_), color, sel_value);
                }
                else
                {
                    egui_canvas_draw_point((x + x_), (y + y_), color, sel_value * alpha / EGUI_ALPHA_100);
                }
            }
        }
    }
}
#endif

static int egui_font_std_draw_scratch_ensure_buffer(uint8_t **buf, uint32_t *capacity, uint32_t needed, uint8_t **out_buf)
{
    uint8_t *new_buf;

    if (buf == NULL || capacity == NULL || out_buf == NULL || needed == 0)
    {
        return 0;
    }

    if (*buf != NULL && needed <= *capacity)
    {
        *out_buf = *buf;
        return 1;
    }

    new_buf = (uint8_t *)egui_malloc((int)needed);
    if (new_buf == NULL)
    {
        return 0;
    }

    if (*buf != NULL)
    {
        egui_free(*buf);
    }

    *buf = new_buf;
    *capacity = needed;
    *out_buf = new_buf;
    return 1;
}

static int egui_font_std_external_draw_scratch_ensure(egui_font_std_external_draw_scratch_t *scratch, uint32_t needed, uint8_t **pixel_buf)
{
    if (scratch == NULL)
    {
        uint8_t *temp_buf = NULL;
        uint32_t temp_capacity = 0;
        return egui_font_std_draw_scratch_ensure_buffer(&temp_buf, &temp_capacity, needed, pixel_buf);
    }

    return egui_font_std_draw_scratch_ensure_buffer(&scratch->pixel_buf, &scratch->capacity, needed, pixel_buf);
}

#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
static int egui_font_std_external_draw_scratch_ensure_compressed(egui_font_std_external_draw_scratch_t *scratch, uint32_t needed, uint8_t **compressed_buf)
{
    if (scratch == NULL)
    {
        uint8_t *temp_buf = NULL;
        uint32_t temp_capacity = 0;
        return egui_font_std_draw_scratch_ensure_buffer(&temp_buf, &temp_capacity, needed, compressed_buf);
    }

    return egui_font_std_draw_scratch_ensure_buffer(&scratch->compressed_buf, &scratch->compressed_capacity, needed, compressed_buf);
}
#endif

static void egui_font_std_external_draw_scratch_release(egui_font_std_external_draw_scratch_t *scratch)
{
    if (scratch == NULL)
    {
        return;
    }

    if (scratch->pixel_buf != NULL)
    {
        egui_free(scratch->pixel_buf);
        scratch->pixel_buf = NULL;
        scratch->capacity = 0;
    }

    if (scratch->compressed_buf != NULL)
    {
        egui_free(scratch->compressed_buf);
        scratch->compressed_buf = NULL;
        scratch->compressed_capacity = 0;
    }
}
static int egui_font_std_draw_loaded_glyph(const egui_font_std_info_t *font, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height,
                                           const uint8_t *pixel_buf, egui_color_t color, egui_alpha_t alpha, const egui_canvas_t *canvas,
                                           const egui_region_t *work_region, egui_alpha_t draw_alpha, const egui_font_std_blend_ctx_t *blend_ctx)
{
    if (font == NULL || pixel_buf == NULL || width <= 0 || height <= 0)
    {
        return 0;
    }

#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) && EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    if (font->font_bit_mode == 4 && canvas != NULL && work_region != NULL && canvas->mask == NULL && blend_ctx != NULL)
    {
        if (egui_font_std_draw_fast_4_prepared(canvas, work_region, x, y, width, height, pixel_buf, draw_alpha, blend_ctx))
        {
            return 1;
        }
    }

    if (egui_font_std_draw_fast(font, x, y, width, height, pixel_buf, color, alpha))
    {
        return 1;
    }

    if (egui_font_std_draw_fast_clipped(font, x, y, width, height, pixel_buf, color, alpha))
    {
        return 1;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK && (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE)
    if (egui_font_std_draw_fast_mask(font, x, y, width, height, pixel_buf, color, alpha))
    {
        return 1;
    }
#endif
#else
    (void)canvas;
    (void)work_region;
    (void)draw_alpha;
    (void)blend_ctx;
#endif

    switch (font->font_bit_mode)
    {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
    case 1:
        egui_font_std_draw_1(x, y, width, height, pixel_buf, color, alpha);
        return 1;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
    case 2:
        egui_font_std_draw_2(x, y, width, height, pixel_buf, color, alpha);
        return 1;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    case 4:
        egui_font_std_draw_4(x, y, width, height, pixel_buf, color, alpha);
        return 1;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
    case 8:
        egui_font_std_draw_8(x, y, width, height, pixel_buf, color, alpha);
        return 1;
#endif
    default:
        EGUI_ASSERT(0);
        return 0;
    }
}

static int egui_font_std_draw_single_char_desc_external_stream(const egui_font_std_info_t *font, const egui_font_std_char_descriptor_t *p_char_desc,
                                                               egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha, const egui_canvas_t *canvas,
                                                               const egui_region_t *work_region, egui_alpha_t draw_alpha,
                                                               const egui_font_std_blend_ctx_t *blend_ctx, egui_font_std_external_draw_scratch_t *scratch)
{
    uint32_t pixel_size;
    uint32_t encoded_size;
    uint8_t *pixel_buf = NULL;
    uint8_t *compressed_buf = NULL;
    const uint8_t *src_buf = NULL;
    int has_temp_pixel_buf = 0;
    int has_temp_compressed_buf = 0;
    int result = 0;
    egui_dim_t draw_x;
    egui_dim_t draw_y;

    if (font == NULL || p_char_desc == NULL)
    {
        return 0;
    }

    if (p_char_desc->box_w == 0 || p_char_desc->box_h == 0)
    {
        return p_char_desc->adv;
    }

    pixel_size = egui_font_std_get_packed_pixel_size(font, p_char_desc->box_w, p_char_desc->box_h);
    if (pixel_size == 0)
    {
        EGUI_ASSERT(0);
        return 0;
    }

    draw_x = x + p_char_desc->off_x;
    draw_y = y + p_char_desc->off_y;
    if (alpha == 0 || draw_alpha == 0 || egui_font_std_is_glyph_outside_region(draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, work_region))
    {
        return p_char_desc->adv;
    }

    if (egui_font_std_bitmap_codec_is_raw(font) && font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
    {
        src_buf = font->pixel_buffer + p_char_desc->idx;
    }
    else
    {
        if (!egui_font_std_external_draw_scratch_ensure(scratch, pixel_size, &pixel_buf))
        {
            EGUI_ASSERT(0);
            return 0;
        }

        has_temp_pixel_buf = (scratch == NULL);
    }

    if (egui_font_std_bitmap_codec_is_raw(font))
    {
        if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
        {
            src_buf = font->pixel_buffer + p_char_desc->idx;
        }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        else
        {
            egui_api_load_external_resource(pixel_buf, (egui_uintptr_t)font->pixel_buffer, p_char_desc->idx, pixel_size);
            src_buf = pixel_buf;
        }
#else
        else
        {
            EGUI_ASSERT(0);
            goto cleanup;
        }
#endif
    }
    else
    {
        encoded_size = p_char_desc->size;
        if (font->font_bit_mode != 4 || encoded_size == 0)
        {
            EGUI_ASSERT(0);
            goto cleanup;
        }

        if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
        {
            src_buf = font->pixel_buffer + p_char_desc->idx;
        }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        else
        {
            if (!egui_font_std_external_draw_scratch_ensure_compressed(scratch, encoded_size, &compressed_buf))
            {
                EGUI_ASSERT(0);
                goto cleanup;
            }

            has_temp_compressed_buf = (scratch == NULL);
            egui_api_load_external_resource(compressed_buf, (egui_uintptr_t)font->pixel_buffer, p_char_desc->idx, encoded_size);
            src_buf = compressed_buf;
        }
#else
        else
        {
            EGUI_ASSERT(0);
            goto cleanup;
        }
#endif

        if (font->bitmap_codec == EGUI_FONT_STD_BITMAP_CODEC_RLE4)
        {
            if (!egui_font_std_decode_rle4_bitmap(src_buf, encoded_size, pixel_buf, pixel_size, p_char_desc->box_w, p_char_desc->box_h, 0))
            {
                EGUI_ASSERT(0);
                goto cleanup;
            }
        }
        else if (font->bitmap_codec == EGUI_FONT_STD_BITMAP_CODEC_RLE4_XOR)
        {
            if (!egui_font_std_decode_rle4_bitmap(src_buf, encoded_size, pixel_buf, pixel_size, p_char_desc->box_w, p_char_desc->box_h, 1))
            {
                EGUI_ASSERT(0);
                goto cleanup;
            }
        }
        else
        {
            EGUI_ASSERT(0);
            goto cleanup;
        }

        src_buf = pixel_buf;
    }

    if (egui_font_std_draw_loaded_glyph(font, draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, src_buf, color, alpha, canvas, work_region, draw_alpha,
                                        blend_ctx))
    {
        result = p_char_desc->adv;
    }

cleanup:
    if (has_temp_pixel_buf && pixel_buf != NULL)
    {
        egui_free(pixel_buf);
    }

    if (has_temp_compressed_buf && compressed_buf != NULL)
    {
        egui_free(compressed_buf);
    }

    return result;
}

static int egui_font_std_draw_single_char_desc(const egui_font_std_info_t *font, const egui_font_std_char_descriptor_t *p_char_desc, egui_dim_t x, egui_dim_t y,
                                               egui_color_t color, egui_alpha_t alpha, const egui_canvas_t *canvas, const egui_region_t *work_region,
                                               egui_alpha_t draw_alpha, const egui_font_std_blend_ctx_t *blend_ctx)
{
    if (font == NULL)
    {
        return 0;
    }

    if (p_char_desc)
    {
        const uint8_t *p_pixer_buffer = NULL;
        int use_direct_raw = 0;

        if (egui_font_std_bitmap_codec_is_raw(font) && font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
        {
            p_pixer_buffer = font->pixel_buffer + p_char_desc->idx;
            use_direct_raw = 1;
        }

        if (!use_direct_raw)
        {
            egui_font_std_external_draw_scratch_t *scratch = NULL;

            if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL && egui_font_std_bitmap_codec_is_compressed(font))
            {
                scratch = &g_font_std_compressed_draw_scratch;
            }

            return egui_font_std_draw_single_char_desc_external_stream(font, p_char_desc, x, y, color, alpha, canvas, work_region, draw_alpha, blend_ctx, scratch);
        }

        egui_dim_t draw_x = x + p_char_desc->off_x;
        egui_dim_t draw_y = y + p_char_desc->off_y;

#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) && EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
        if (font->font_bit_mode == 4 && font->res_type == EGUI_RESOURCE_TYPE_INTERNAL && egui_font_std_bitmap_codec_is_raw(font) && canvas != NULL &&
            work_region != NULL && canvas->mask == NULL &&
            blend_ctx != NULL)
        {
            if (egui_font_std_draw_fast_4_prepared(canvas, work_region, draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, draw_alpha,
                                                   blend_ctx))
            {
                return p_char_desc->adv;
            }
        }

        if (!egui_font_std_draw_fast(font, draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha))
        {
            if (!egui_font_std_draw_fast_clipped(font, draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha))
            {
                {
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK && (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE)
                    if (!egui_font_std_draw_fast_mask(font, draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha))
#endif
                    {
                        switch (font->font_bit_mode)
                        {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
                        case 1:
                            egui_font_std_draw_1(draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha);
                            break;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
                        case 2:
                            egui_font_std_draw_2(draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha);
                            break;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
                        case 4:
                            egui_font_std_draw_4(draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha);
                            break;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
                        case 8:
                            egui_font_std_draw_8(draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha);
                            break;
#endif
                        default:
                            EGUI_ASSERT(0);
                            break;
                        }
                    }
                }
            }
        }
#else
        (void)canvas;
        (void)work_region;
        (void)draw_alpha;
        (void)blend_ctx;

        switch (font->font_bit_mode)
        {
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_1
        case 1:
            egui_font_std_draw_1(draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha);
            break;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_2
        case 2:
            egui_font_std_draw_2(draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha);
            break;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
        case 4:
            egui_font_std_draw_4(draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha);
            break;
#endif
#if EGUI_CONFIG_FUNCTION_FONT_FORMAT_8
        case 8:
            egui_font_std_draw_8(draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, p_pixer_buffer, color, alpha);
            break;
#endif
        default:
            EGUI_ASSERT(0);
            break;
        }
#endif
        return p_char_desc->adv;
    }

    egui_canvas_draw_rectangle(x, y, FONT_ERROR_FONT_SIZE(font->height) - 2, font->height, 1, color, alpha);
    return FONT_ERROR_FONT_SIZE(font->height);
}

#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) && EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
static int egui_font_std_draw_string_fast_4(const void *font_key, const egui_font_std_info_t *font, const char *s, egui_dim_t x, egui_dim_t y,
                                            egui_color_t color, egui_alpha_t alpha, const egui_canvas_t *canvas, const egui_region_t *work_region,
                                            egui_alpha_t draw_alpha, const egui_font_std_blend_ctx_t *blend_ctx)
{
    int result = 0;
    int str_cnt = 0;
    egui_font_std_char_descriptor_t external_desc_scratch;
    egui_dim_t work_x0 = work_region->location.x;
    egui_dim_t work_y0 = work_region->location.y;
    egui_dim_t work_x1 = work_x0 + work_region->size.width;
    egui_dim_t work_y1 = work_y0 + work_region->size.height;
    egui_dim_t pfb_base_x = canvas->pfb_location_in_base_view.x;
    egui_dim_t pfb_base_y = canvas->pfb_location_in_base_view.y;
    const uint8_t *pixel_buffer = font->pixel_buffer;
    const egui_font_std_draw_prefix_cache_t *prefix_cache = egui_font_std_prepare_draw_prefix_cache(font_key, font, s);
    const egui_font_std_ascii_lookup_cache_t *ascii_cache = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_font_std_external_draw_scratch_t external_draw_scratch = {
            0,
    };
#endif

    if (font == NULL || !egui_font_std_bitmap_codec_is_raw(font))
    {
        return 0;
    }

    if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL && font->code_array != NULL)
    {
        ascii_cache = egui_font_std_prepare_ascii_lookup_cache(font);
    }

    if (prefix_cache != NULL)
    {
        int16_t local_x0 = (int16_t)(work_x0 - x);
        int16_t local_x1 = (int16_t)(work_x1 - x);
        uint16_t start = (uint16_t)egui_font_std_find_prefix_first_visible(prefix_cache, local_x0);

        for (uint16_t i = start; i < prefix_cache->glyph_count; i++)
        {
            const egui_font_std_draw_prefix_glyph_t *glyph = &prefix_cache->glyphs[i];
            egui_dim_t offset = x + glyph->x;

            if (glyph->box_x0 >= local_x1)
            {
                result = prefix_cache->line_bytes;
                goto cleanup;
            }

            if (glyph->has_desc)
            {
                egui_dim_t draw_x = offset + glyph->off_x;
                egui_dim_t draw_y = y + glyph->off_y;
                egui_dim_t vis_x0 = (draw_x > work_x0) ? draw_x : work_x0;
                egui_dim_t vis_y0 = (draw_y > work_y0) ? draw_y : work_y0;
                egui_dim_t vis_x1 = ((draw_x + glyph->box_w) < work_x1) ? (draw_x + glyph->box_w) : work_x1;
                egui_dim_t vis_y1 = ((draw_y + glyph->box_h) < work_y1) ? (draw_y + glyph->box_h) : work_y1;

                if (vis_x0 < vis_x1 && vis_y0 < vis_y1)
                {
                    if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
                    {
                        egui_dim_t pfb_x = vis_x0 - pfb_base_x;
                        egui_dim_t pfb_y = vis_y0 - pfb_base_y;
                        const uint8_t *p_pixer_buffer = pixel_buffer + glyph->idx;

                        egui_font_std_draw_fast_4_ctx(canvas, pfb_x, pfb_y, glyph->box_w, vis_x0 - draw_x, vis_y0 - draw_y, vis_x1 - vis_x0, vis_y1 - vis_y0,
                                                      p_pixer_buffer, draw_alpha, blend_ctx);
                    }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                    else
                    {
                        egui_font_std_char_descriptor_t glyph_desc = {
                                .idx = glyph->idx,
                                .size = 0,
                                .box_w = glyph->box_w,
                                .box_h = glyph->box_h,
                                .adv = glyph->adv,
                                .off_x = glyph->off_x,
                                .off_y = glyph->off_y,
                        };

                        if (egui_font_std_draw_single_char_desc_external_stream(font, &glyph_desc, offset, y, color, alpha, canvas, work_region, draw_alpha,
                                                                                blend_ctx, &external_draw_scratch) == 0)
                        {
                            result = 0;
                            goto cleanup;
                        }
                    }
#endif
                }
            }
            else
            {
                egui_font_std_draw_single_char_desc(font, NULL, offset, y, color, alpha, canvas, work_region, draw_alpha, blend_ctx);
            }
        }

        if (prefix_cache->is_complete_line)
        {
            result = prefix_cache->line_bytes;
            goto cleanup;
        }

        s += prefix_cache->cached_bytes;
        str_cnt = prefix_cache->cached_bytes;
    }

    int offset = x + ((prefix_cache != NULL) ? prefix_cache->cached_advance : 0);

    while (*s != '\0')
    {
        int char_bytes;
        uint32_t utf8_code;
        const egui_font_std_char_descriptor_t *p_char_desc;
        int adv;

        if (*s == '\r')
        {
            s++;
            str_cnt++;
            continue;
        }
        else if (*s == '\n')
        {
            str_cnt++;
            break;
        }

        if (offset >= work_x1)
        {
            if (prefix_cache != NULL)
            {
                result = prefix_cache->line_bytes;
                goto cleanup;
            }

            const char *next_line = strchr(s, '\n');
            if (next_line != NULL)
            {
                str_cnt += (int)(next_line - s) + 1;
            }
            else
            {
                str_cnt += (int)strlen(s);
            }
            break;
        }

        if (ascii_cache != NULL && (((uint8_t)s[0]) & 0x80) == 0)
        {
            uint8_t ascii_code = (uint8_t)s[0];
            int ascii_index;
            char_bytes = 1;
            ascii_index = egui_font_std_get_ascii_lookup_index(ascii_cache, ascii_code);
            p_char_desc = ascii_index >= 0 ? &font->char_array[ascii_index] : NULL;
            adv = p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
        }
        else
        {
            char_bytes = egui_font_get_utf8_code_fast(s, &utf8_code);
            p_char_desc = egui_font_std_get_desc_draw_fast(font, utf8_code, &external_desc_scratch);
            adv = p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
        }

        s += char_bytes;
        str_cnt += char_bytes;

        if (draw_alpha == 0)
        {
            offset += adv;
            continue;
        }

        if (p_char_desc == NULL)
        {
            offset += egui_font_std_draw_single_char_desc(font, p_char_desc, offset, y, color, alpha, canvas, work_region, draw_alpha, blend_ctx);
            continue;
        }

        if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
        {
            egui_dim_t draw_x = offset + p_char_desc->off_x;
            egui_dim_t draw_y = y + p_char_desc->off_y;
            egui_dim_t vis_x0 = (draw_x > work_x0) ? draw_x : work_x0;
            egui_dim_t vis_y0 = (draw_y > work_y0) ? draw_y : work_y0;
            egui_dim_t vis_x1 = ((draw_x + p_char_desc->box_w) < work_x1) ? (draw_x + p_char_desc->box_w) : work_x1;
            egui_dim_t vis_y1 = ((draw_y + p_char_desc->box_h) < work_y1) ? (draw_y + p_char_desc->box_h) : work_y1;

            if (vis_x0 < vis_x1 && vis_y0 < vis_y1)
            {
                egui_dim_t pfb_x = vis_x0 - pfb_base_x;
                egui_dim_t pfb_y = vis_y0 - pfb_base_y;
                const uint8_t *p_pixer_buffer = pixel_buffer + p_char_desc->idx;

                egui_font_std_draw_fast_4_ctx(canvas, pfb_x, pfb_y, p_char_desc->box_w, vis_x0 - draw_x, vis_y0 - draw_y, vis_x1 - vis_x0, vis_y1 - vis_y0,
                                              p_pixer_buffer, draw_alpha, blend_ctx);
            }

            offset += adv;
        }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
        else
        {
            int draw_adv = egui_font_std_draw_single_char_desc_external_stream(font, p_char_desc, offset, y, color, alpha, canvas, work_region, draw_alpha,
                                                                               blend_ctx, &external_draw_scratch);

            if (draw_adv == 0)
            {
                result = 0;
                goto cleanup;
            }

            offset += draw_adv;
        }
#endif
    }

    result = str_cnt;

cleanup:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_font_std_external_draw_scratch_release(&external_draw_scratch);
#endif
    return result;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK && (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE)
static int egui_font_std_draw_string_fast_4_mask(const void *font_key, const egui_font_std_info_t *font, const char *s, egui_dim_t x, egui_dim_t y,
                                                 egui_color_t color, egui_alpha_t alpha, const egui_region_t *work_region)
{
    int result = 0;
    int str_cnt = 0;
    egui_font_std_char_descriptor_t external_desc_scratch;
    egui_dim_t work_x0 = work_region->location.x;
    egui_dim_t work_y0 = work_region->location.y;
    egui_dim_t work_x1 = work_x0 + work_region->size.width;
    egui_dim_t work_y1 = work_y0 + work_region->size.height;
#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE)
    const uint8_t *pixel_buffer = font->pixel_buffer;
#endif
    const egui_font_std_draw_prefix_cache_t *prefix_cache = egui_font_std_prepare_draw_prefix_cache(font_key, font, s);
    const egui_font_std_ascii_lookup_cache_t *ascii_cache = NULL;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_font_std_external_draw_scratch_t external_draw_scratch = {
            0,
    };
#endif

    if (font == NULL || !egui_font_std_bitmap_codec_is_raw(font))
    {
        return 0;
    }

    if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL && font->code_array != NULL)
    {
        ascii_cache = egui_font_std_prepare_ascii_lookup_cache(font);
    }

    if (prefix_cache != NULL)
    {
        int16_t local_x0 = (int16_t)(work_x0 - x);
        int16_t local_x1 = (int16_t)(work_x1 - x);
        uint16_t start = (uint16_t)egui_font_std_find_prefix_first_visible(prefix_cache, local_x0);

        for (uint16_t i = start; i < prefix_cache->glyph_count; i++)
        {
            const egui_font_std_draw_prefix_glyph_t *glyph = &prefix_cache->glyphs[i];
            egui_dim_t offset = x + glyph->x;

            if (glyph->box_x0 >= local_x1)
            {
                result = prefix_cache->line_bytes;
                goto cleanup;
            }

            if (glyph->has_desc)
            {
                egui_dim_t draw_y = y + glyph->off_y;

                if (draw_y < work_y1 && (draw_y + glyph->box_h) > work_y0)
                {
                    if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
                    {
#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE)
                        egui_dim_t draw_x = offset + glyph->off_x;
                        egui_font_std_draw_fast_mask(font, draw_x, draw_y, glyph->box_w, glyph->box_h, pixel_buffer + glyph->idx, color, alpha);
#else
                        egui_font_std_char_descriptor_t internal_desc = {
                                .idx = glyph->idx,
                                .size = 0,
                                .box_w = glyph->box_w,
                                .box_h = glyph->box_h,
                                .adv = glyph->adv,
                                .off_x = glyph->off_x,
                                .off_y = glyph->off_y,
                        };

                        egui_font_std_draw_single_char_desc(font, &internal_desc, offset, y, color, alpha, NULL, NULL, 0, NULL);
#endif
                    }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                    else
                    {
                        egui_font_std_char_descriptor_t glyph_desc = {
                                .idx = glyph->idx,
                                .size = 0,
                                .box_w = glyph->box_w,
                                .box_h = glyph->box_h,
                                .adv = glyph->adv,
                                .off_x = glyph->off_x,
                                .off_y = glyph->off_y,
                        };

                        if (egui_font_std_draw_single_char_desc_external_stream(font, &glyph_desc, offset, y, color, alpha, NULL, work_region, 0, NULL,
                                                                                &external_draw_scratch) == 0)
                        {
                            result = 0;
                            goto cleanup;
                        }
                    }
#endif
                }
            }
            else
            {
                egui_font_std_draw_single_char_desc(font, NULL, offset, y, color, alpha, NULL, NULL, 0, NULL);
            }
        }

        if (prefix_cache->is_complete_line)
        {
            result = prefix_cache->line_bytes;
            goto cleanup;
        }

        s += prefix_cache->cached_bytes;
        str_cnt = prefix_cache->cached_bytes;
    }

    {
        int offset = x + ((prefix_cache != NULL) ? prefix_cache->cached_advance : 0);

        while (*s != '\0')
        {
            int char_bytes;
            uint32_t utf8_code;
            const egui_font_std_char_descriptor_t *p_char_desc;
            int adv;

            if (*s == '\r')
            {
                s++;
                str_cnt++;
                continue;
            }
            else if (*s == '\n')
            {
                str_cnt++;
                break;
            }

            if (offset >= work_x1)
            {
                if (prefix_cache != NULL)
                {
                    result = prefix_cache->line_bytes;
                    goto cleanup;
                }

                {
                    const char *next_line = strchr(s, '\n');
                    if (next_line != NULL)
                    {
                        str_cnt += (int)(next_line - s) + 1;
                    }
                    else
                    {
                        str_cnt += (int)strlen(s);
                    }
                }
                break;
            }

            if (ascii_cache != NULL && (((uint8_t)s[0]) & 0x80) == 0)
            {
                uint8_t ascii_code = (uint8_t)s[0];
                int ascii_index;
                char_bytes = 1;
                ascii_index = egui_font_std_get_ascii_lookup_index(ascii_cache, ascii_code);
                p_char_desc = ascii_index >= 0 ? &font->char_array[ascii_index] : NULL;
                adv = p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
            }
            else
            {
                char_bytes = egui_font_get_utf8_code_fast(s, &utf8_code);
                p_char_desc = egui_font_std_get_desc_draw_fast(font, utf8_code, &external_desc_scratch);
                adv = p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
            }

            s += char_bytes;
            str_cnt += char_bytes;

            if (p_char_desc == NULL)
            {
                offset += egui_font_std_draw_single_char_desc(font, p_char_desc, offset, y, color, alpha, NULL, NULL, 0, NULL);
                continue;
            }

            if ((offset + font->height) > work_x0)
            {
                egui_dim_t draw_y = y + p_char_desc->off_y;

                if (draw_y < work_y1 && (draw_y + p_char_desc->box_h) > work_y0)
                {
                    if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL)
                    {
#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE)
                        egui_dim_t draw_x = offset + p_char_desc->off_x;
                        egui_font_std_draw_fast_mask(font, draw_x, draw_y, p_char_desc->box_w, p_char_desc->box_h, pixel_buffer + p_char_desc->idx, color,
                                                     alpha);
#else
                        egui_font_std_draw_single_char_desc(font, p_char_desc, offset, y, color, alpha, NULL, NULL, 0, NULL);
#endif
                    }
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
                    else
                    {
                        int draw_adv = egui_font_std_draw_single_char_desc_external_stream(font, p_char_desc, offset, y, color, alpha, NULL, work_region, 0,
                                                                                           NULL, &external_draw_scratch);

                        if (draw_adv == 0)
                        {
                            result = 0;
                            goto cleanup;
                        }

                        offset += draw_adv;
                        continue;
                    }
#endif
                }
            }

            offset += adv;
        }
    }

    result = str_cnt;

cleanup:
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_font_std_external_draw_scratch_release(&external_draw_scratch);
#endif
    return result;
}
#endif
#endif

int egui_font_std_try_draw_string_in_rect_fast(const egui_font_t *self, const void *string, egui_region_t *rect, uint8_t align_type, egui_dim_t line_space,
                                               egui_color_t color, egui_alpha_t alpha)
{
#if defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) && !EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE || !EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    (void)self;
    (void)string;
    (void)rect;
    (void)align_type;
    (void)line_space;
    (void)color;
    (void)alpha;
    return 0;
#else
    const char *s = (const char *)string;
    const egui_font_std_line_cache_t *line_cache = NULL;
    egui_font_std_info_t *font;
    const egui_canvas_t *canvas;
    const egui_region_t *work_region;
    egui_alpha_t draw_alpha;
    egui_font_std_blend_ctx_t blend_ctx;
    int use_fast_no_mask = 0;
    int use_fast_mask = 0;
    egui_dim_t y_size;
    egui_dim_t draw_y;
    egui_dim_t work_y0;
    egui_dim_t work_y1;
    egui_dim_t line_step;
    int str_bytes;
    uint8_t h_align = align_type & EGUI_ALIGN_HMASK;
    uint8_t v_align = align_type & EGUI_ALIGN_VMASK;

    if (self == NULL || rect == NULL || s == NULL || self->api != &egui_font_std_t_api_table)
    {
        return 0;
    }

    if (!((h_align == 0 || h_align == EGUI_ALIGN_LEFT) && (v_align == 0 || v_align == EGUI_ALIGN_TOP)))
    {
        return 0;
    }

    font = (egui_font_std_info_t *)self->res;
    if (!egui_font_std_bitmap_codec_is_raw(font))
    {
        return 0;
    }

    canvas = egui_canvas_get_canvas();
    work_region = egui_canvas_get_base_view_work_region();
    draw_alpha = egui_color_alpha_mix(canvas->alpha, alpha);

    if (draw_alpha != 0 && font->font_bit_mode == 4)
    {
        if (canvas->mask == NULL)
        {
            use_fast_no_mask = 1;
        }
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK && (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE)
        else if (canvas->mask->api->mask_blend_row_color != NULL)
        {
            use_fast_mask = 1;
        }
#endif
    }

    if (!use_fast_no_mask && !use_fast_mask)
    {
        return 0;
    }

    if (use_fast_no_mask)
    {
        egui_font_std_blend_ctx_init(&blend_ctx, color);
    }
    y_size = font->height;
    draw_y = rect->location.y;
    work_y0 = work_region->location.y;
    work_y1 = work_y0 + work_region->size.height;
    line_step = y_size + line_space;
    line_cache = egui_font_std_prepare_line_cache(s);

    if (line_cache != NULL && line_cache->is_complete)
    {
        uint16_t line_index = 0;

        if (line_step > 0)
        {
            while (line_index < line_cache->line_count && draw_y + y_size <= work_y0)
            {
                line_index++;
                draw_y += line_step;
            }
        }

        for (; line_index < line_cache->line_count; line_index++)
        {
            const char *line_s = line_cache->lines[line_index];

            if (line_step > 0 && draw_y >= work_y1)
            {
                break;
            }

            if (use_fast_no_mask)
            {
                str_bytes = egui_font_std_draw_string_fast_4(self->res, font, line_s, rect->location.x, draw_y, color, alpha, canvas, work_region, draw_alpha,
                                                             &blend_ctx);
            }
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK && (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE)
            else
            {
                str_bytes = egui_font_std_draw_string_fast_4_mask(self->res, font, line_s, rect->location.x, draw_y, color, alpha, work_region);
            }
#endif
            if (str_bytes <= 0)
            {
                break;
            }

            draw_y += line_step;
        }

        return 1;
    }

    if (line_step > 0)
    {
        while (*s && draw_y + y_size <= work_y0)
        {
            const char *next_line = strchr(s, '\n');

            if (next_line == NULL)
            {
                return 1;
            }

            s = next_line + 1;
            draw_y += line_step;
        }
    }

    while (*s)
    {
        if (line_step > 0 && draw_y >= work_y1)
        {
            break;
        }

        if (use_fast_no_mask)
        {
            str_bytes =
                    egui_font_std_draw_string_fast_4(self->res, font, s, rect->location.x, draw_y, color, alpha, canvas, work_region, draw_alpha, &blend_ctx);
        }
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK && (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE)
        else
        {
            str_bytes = egui_font_std_draw_string_fast_4_mask(self->res, font, s, rect->location.x, draw_y, color, alpha, work_region);
        }
#endif
        if (str_bytes <= 0)
        {
            break;
        }

        s += str_bytes;
        draw_y += line_step;
    }
    return 1;
#endif
}

int egui_font_std_draw_string(const egui_font_t *self, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    const char *s = (const char *)string;
    const char *next_line;
    int str_cnt = 0;
    egui_font_std_char_descriptor_t external_desc_scratch;
    int offset = x;
    int char_bytes;
    uint32_t utf8_code;
    egui_font_std_info_t *font = (egui_font_std_info_t *)self->res;
    const egui_canvas_t *canvas = egui_canvas_get_canvas();
    const egui_region_t *base_region = egui_canvas_get_base_view_work_region();
    egui_alpha_t draw_alpha = egui_color_alpha_mix(canvas->alpha, alpha);
    const egui_font_std_blend_ctx_t *p_blend_ctx = NULL;
    const egui_font_std_ascii_lookup_cache_t *ascii_cache = NULL;
#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) && EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    egui_font_std_blend_ctx_t blend_ctx;
#endif

    if (0 == s)
    {
        return 0;
    }

#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) && EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    if (draw_alpha != 0 && font->font_bit_mode == 4 && canvas->mask == NULL)
    {
        egui_font_std_blend_ctx_init(&blend_ctx, color);
        p_blend_ctx = &blend_ctx;
    }
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK && (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE)
    else
    {
        int use_fast_mask = (draw_alpha != 0 && font->font_bit_mode == 4 && font->res_type == EGUI_RESOURCE_TYPE_INTERNAL &&
                             egui_font_std_bitmap_codec_is_raw(font) && canvas->mask != NULL && canvas->mask->api->mask_blend_row_color != NULL);
        if (use_fast_mask)
        {
            str_cnt = egui_font_std_draw_string_fast_4_mask(self->res, font, s, x, y, color, alpha, base_region);
            return str_cnt;
        }
    }
#endif
#endif

    if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL && font->code_array != NULL)
    {
        ascii_cache = egui_font_std_prepare_ascii_lookup_cache(font);
    }

    // check if the string is in the canvas region.
    if (y > (base_region->location.y + base_region->size.height) || (y + font->height) < base_region->location.y)
    {
        next_line = strchr(s, '\n');
        if (next_line)
        {
            str_cnt += (next_line - s) + 1;
        }
        else
        {
            str_cnt += strlen(s);
        }
        return str_cnt;
    }

#if (!defined(EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) || EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE) && EGUI_CONFIG_FUNCTION_FONT_FORMAT_4
    if (p_blend_ctx != NULL && egui_font_std_bitmap_codec_is_raw(font))
    {
        str_cnt = egui_font_std_draw_string_fast_4(self->res, font, s, x, y, color, alpha, canvas, base_region, draw_alpha, p_blend_ctx);
        return str_cnt;
    }
#endif

    while ((*s != '\0'))
    {
        if (*s == '\r')
        {
            s++;
            str_cnt++;
            continue;
        }
        else if (*s == '\n')
        {
            str_cnt++;
            break;
        }
        // only check x-axis intersection, since y-axis is fixed.
        if (offset >= base_region->location.x + base_region->size.width)
        {
            next_line = strchr(s, '\n');
            if (next_line)
            {
                str_cnt += (next_line - s) + 1;
            }
            else
            {
                str_cnt += strlen(s);
            }
            break;
        }
        else
        {
            const egui_font_std_char_descriptor_t *p_char_desc;
            int adv;

            if (ascii_cache != NULL && (((uint8_t)s[0]) & 0x80) == 0)
            {
                uint8_t ascii_code = (uint8_t)s[0];
                int ascii_index;
                char_bytes = 1;
                ascii_index = egui_font_std_get_ascii_lookup_index(ascii_cache, ascii_code);
                p_char_desc = ascii_index >= 0 ? &font->char_array[ascii_index] : NULL;
                adv = p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
            }
            else
            {
                char_bytes = egui_font_get_utf8_code_fast(s, &utf8_code);
                p_char_desc = egui_font_std_get_desc_draw_fast(font, utf8_code, &external_desc_scratch);
                adv = p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
            }

            s += char_bytes;
            str_cnt += char_bytes;

            if ((offset + font->height) <= base_region->location.x) // max font width is font->height
            {
                offset += adv;
            }
            else if (draw_alpha == 0)
            {
                offset += adv;
            }
            else
            {
                offset += egui_font_std_draw_single_char_desc(font, p_char_desc, offset, y, color, alpha, canvas, base_region, draw_alpha, p_blend_ctx);
            }
        }
    }
    return str_cnt;
}

int egui_font_std_get_str_size(const egui_font_t *self, const void *string, uint8_t is_multi_line, egui_dim_t line_space, egui_dim_t *width, egui_dim_t *height)
{
    const char *s = (const char *)string;
    egui_font_std_char_descriptor_t external_desc_scratch;
    egui_font_std_info_t *font = (egui_font_std_info_t *)self->res;

    if (NULL == s || NULL == font)
    {
        *width = *height = 0;
        return -1;
    }

    int font_width = 0;
    int font_height = font->height;
    int font_max_width = 0;
    uint32_t utf8_code;
    int utf8_bytes;
    int adv;
    const egui_font_std_char_descriptor_t *p_char_desc;
    const egui_font_std_ascii_lookup_cache_t *ascii_cache = NULL;

    if (font->res_type == EGUI_RESOURCE_TYPE_INTERNAL && font->code_array != NULL)
    {
        ascii_cache = egui_font_std_prepare_ascii_lookup_cache(font);
    }

    int is_line_full = 0;
    while ((*s != '\0'))
    {
        if (*s == '\r')
        {
            s++;
            continue;
        }
        else if (*s == '\n')
        {
            if (is_multi_line == 0)
            {
                break;
            }
            // update max width.
            if (font_max_width < font_width)
            {
                font_max_width = font_width;
            }
            font_height += font->height + line_space;
            font_width = 0;

            is_line_full = 0;
        }

        if (ascii_cache != NULL && (((uint8_t)s[0]) & 0x80) == 0)
        {
            uint8_t ascii_code = (uint8_t)s[0];
            int ascii_index;
            utf8_bytes = 1;
            ascii_index = egui_font_std_get_ascii_lookup_index(ascii_cache, ascii_code);
            p_char_desc = ascii_index >= 0 ? &font->char_array[ascii_index] : NULL;
            adv = p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
        }
        else
        {
            utf8_bytes = egui_font_get_utf8_code_fast(s, &utf8_code);
            p_char_desc = egui_font_std_get_desc_draw_fast(font, utf8_code, &external_desc_scratch);
            adv = p_char_desc ? p_char_desc->adv : FONT_ERROR_FONT_SIZE(font->height);
        }
        s += utf8_bytes;

        // caculate font width.
        if (!is_line_full)
        {
            font_width += adv;

            if (*width != 0)
            {
                if (font_width > *width)
                {
                    font_width = *width;
                    is_line_full = 1;
                    continue;
                }
            }
        }
    }

    // update max width.
    if (font_max_width < font_width)
    {
        font_max_width = font_width;
    }

    *width = font_max_width;
    *height = font_height;

    return 0;
}

int egui_font_std_try_get_line_height(const egui_font_t *self, egui_dim_t *line_height)
{
    const egui_font_std_info_t *font;

    if (self == NULL || line_height == NULL || self->api != &egui_font_std_t_api_table || self->res == NULL)
    {
        return 0;
    }

    font = (const egui_font_std_info_t *)self->res;
    *line_height = font->height;
    return 1;
}

const egui_font_api_t egui_font_std_t_api_table = {
        .draw_string = egui_font_std_draw_string,
        .get_str_size = egui_font_std_get_str_size,
};

void egui_font_std_init(egui_font_t *self, const void *res)
{
    EGUI_LOCAL_INIT(egui_font_std_t);
    // call super init.
    egui_font_init(self, res);

    // update api.
    self->api = &egui_font_std_t_api_table;
}
