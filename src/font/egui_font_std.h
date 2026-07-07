#ifndef _EGUI_FONT_STD_H_
#define _EGUI_FONT_STD_H_

#include "egui_font.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// avoid size mismatch warning in some compilers
#define EGUI_FONT_STD_EXT_CODE_DESC_ITEM_SIZE 4
#define EGUI_FONT_STD_EXT_CHAR_DESC_ITEM_SIZE 12

/*
 * By default egui_font_std_prepare_access() keeps external pixel data external-backed to avoid copying the whole font bitmap onto the heap. Define this to 1
 * only if your caller explicitly needs a fully local pixel buffer.
 */
#ifndef EGUI_FONT_STD_PREPARE_ACCESS_COPY_PIXEL_BUFFER
#define EGUI_FONT_STD_PREPARE_ACCESS_COPY_PIXEL_BUFFER 0
#endif

#define EGUI_FONT_STD_GET_FONT_HEIGHT(_font) (((egui_font_std_info_t *)(_font)->res)->height)

#define EGUI_FONT_STD_BITMAP_CODEC_RAW      0
#define EGUI_FONT_STD_BITMAP_CODEC_RLE4     1
#define EGUI_FONT_STD_BITMAP_CODEC_RLE4_XOR 2

#define EGUI_FONT_STD_ASCII_CACHE_SIZE 128
#define EGUI_FONT_STD_DRAW_PREFIX_CACHE_ENABLED                                                                                                                \
    (EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW && (EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS > 0) && (EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS > 0))
#define EGUI_FONT_STD_DRAW_PREFIX_CACHE_GLYPH_STORAGE_MAX                                                                                                      \
    ((EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS > 0) ? EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_MAX_GLYPHS : 1)
#define EGUI_FONT_STD_BITMAP_CODEC_RLE4_ENABLED     (EGUI_CONFIG_FUNCTION_FONT_FORMAT_4 && EGUI_CONFIG_FUNCTION_FONT_STD_BITMAP_CODEC_RLE4)
#define EGUI_FONT_STD_BITMAP_CODEC_RLE4_XOR_ENABLED (EGUI_CONFIG_FUNCTION_FONT_FORMAT_4 && EGUI_CONFIG_FUNCTION_FONT_STD_BITMAP_CODEC_RLE4_XOR)
#define EGUI_FONT_STD_BITMAP_COMPRESSED_ENABLED     (EGUI_FONT_STD_BITMAP_CODEC_RLE4_ENABLED || EGUI_FONT_STD_BITMAP_CODEC_RLE4_XOR_ENABLED)
#ifndef EGUI_FONT_STD_LINE_CACHE_MAX_LINES
#define EGUI_FONT_STD_LINE_CACHE_MAX_LINES 16
#endif
#ifndef EGUI_FONT_STD_LINE_CACHE_SLOTS
#define EGUI_FONT_STD_LINE_CACHE_SLOTS 2
#endif
#define EGUI_FONT_STD_COMPRESSED_GLYPH_CACHE_ENABLED                                                                                                           \
    (EGUI_FONT_STD_BITMAP_COMPRESSED_ENABLED && (EGUI_CONFIG_FONT_STD_COMPRESSED_GLYPH_CACHE_MAX_BYTES > 0) &&                                                 \
     (EGUI_CONFIG_FONT_STD_COMPRESSED_GLYPH_CACHE_SLOTS > 0))

typedef struct
{
    uint32_t code; // only support 32bit utf8 code
} egui_font_std_code_descriptor_t;

typedef struct
{
    uint32_t idx;  // the buffer size is 32bit
    uint16_t size; // the buffer size
    uint8_t box_w; // we never use big font, so we don't need to support big font
    uint8_t box_h; // we never use big font, so we don't need to support big font
    uint8_t adv;   // the distance from the origin to the next character
    int8_t off_x;  // the x offset of the character
    int8_t off_y;  // the y offset of the character
} egui_font_std_char_descriptor_t;

typedef struct
{
    uint8_t font_size;     // font size
    uint8_t font_bit_mode; // font bit size 1, 2, 4, 8
    uint8_t height;        // font height
    uint8_t res_type;      // EGUI_RESOURCE_TYPE_INTERNAL, EGUI_RESOURCE_TYPE_EXTERNAL
    uint8_t code_res_type; // EGUI_RESOURCE_TYPE_INTERNAL, EGUI_RESOURCE_TYPE_EXTERNAL
    uint8_t bitmap_codec;  // EGUI_FONT_STD_BITMAP_CODEC_*
    uint16_t count;        // total char count, only support 65535
    const egui_font_std_code_descriptor_t *code_array;
    const egui_font_std_char_descriptor_t *char_array;
    const uint8_t *pixel_buffer;
} egui_font_std_info_t;

typedef struct
{
    egui_font_std_info_t info;
    egui_core_t *core;
    egui_font_std_code_descriptor_t *owned_code_array;
    egui_font_std_char_descriptor_t *owned_char_array;
    uint8_t *owned_pixel_buffer;
} egui_font_std_access_t;

typedef struct
{
    egui_core_t *core;
    uint8_t *pixel_buf;
    uint32_t capacity;
    uint8_t *compressed_buf;
    uint32_t compressed_capacity;
} egui_font_std_external_draw_scratch_t;

typedef uint16_t egui_font_std_code_lookup_count_t;
typedef uint32_t egui_font_std_code_lookup_code_t;
typedef int egui_font_std_code_lookup_index_t;

typedef struct
{
    const egui_font_std_code_descriptor_t *code_array;
    egui_font_std_code_lookup_count_t count;
    uint8_t code_res_type;
    egui_font_std_code_lookup_code_t last_code;
    egui_font_std_code_lookup_index_t last_index;
    egui_font_std_code_lookup_code_t block_start_code;
    egui_font_std_code_lookup_code_t block_end_code;
    egui_font_std_code_lookup_index_t block_start_index;
    egui_font_std_code_lookup_index_t block_end_index;
} egui_font_std_code_lookup_cache_t;

typedef uint16_t egui_font_std_ascii_index_t;

typedef struct
{
    uint32_t idx;
    uint16_t size;
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
    uint8_t code_res_type;
    egui_font_std_ascii_index_t ascii_index[EGUI_FONT_STD_ASCII_CACHE_SIZE];
    uint8_t is_ready;
} egui_font_std_ascii_lookup_cache_t;

typedef struct
{
    const egui_font_std_info_t *font;
    uint32_t glyph_idx;
    uint16_t encoded_size;
    uint16_t pixel_size;
    uint8_t box_w;
    uint8_t box_h;
    uint8_t bitmap_codec;
    uint8_t is_ready;
    uint32_t stamp;
    uint8_t *pixel_buf;
    uint32_t capacity;
} egui_font_std_compressed_glyph_cache_t;

struct egui_font_std
{
    egui_font_t base;
};

extern const egui_font_api_t egui_font_std_t_api_table;

const egui_font_std_char_descriptor_t *egui_font_std_get_desc_fast_api(egui_canvas_t *canvas, const egui_font_std_info_t *font, uint32_t utf8_code,
                                                                       egui_font_std_char_descriptor_t *external_desc_scratch);
int egui_font_std_get_str_size_with_core(const egui_font_t *self, egui_core_t *core, const void *string, uint8_t is_multi_line, egui_dim_t line_space,
                                         egui_dim_t *width, egui_dim_t *height);
int egui_font_std_get_str_size_with_canvas(const egui_font_t *self, egui_canvas_t *canvas, const void *string, uint8_t is_multi_line, egui_dim_t line_space,
                                           egui_dim_t *width, egui_dim_t *height);
int egui_font_std_prepare_desc_access(egui_canvas_t *canvas, const egui_font_std_info_t *font, egui_font_std_access_t *access);
int egui_font_std_prepare_access(egui_canvas_t *canvas, const egui_font_std_info_t *font, egui_font_std_access_t *access);
void egui_font_std_release_access(egui_font_std_access_t *access);
void egui_font_std_release_frame_cache(egui_core_t *core);
int egui_font_std_try_get_line_height(const egui_font_t *self, egui_dim_t *line_height);
int egui_font_std_try_draw_string_in_rect_fast(const egui_font_t *self, egui_canvas_t *canvas, const void *string, egui_region_t *rect, uint8_t align_type,
                                               egui_dim_t line_space, egui_color_t color, egui_alpha_t alpha);
void egui_font_std_init(egui_font_t *self, const void *res);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_FONT_STD_H_ */
