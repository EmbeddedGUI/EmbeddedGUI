#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_SCEEN_HEIGHT 240

#define EGUI_CONFIG_MAX_FPS 1

// HelloPerformance is dominated by full-screen benchmark scenes, and a 48x16
// tile still divides the 240x240 canvas cleanly while cutting the PFB-height-
// proportional row caches much harder than the default 30x30 tile.
#ifndef EGUI_CONFIG_PFB_WIDTH
#define EGUI_CONFIG_PFB_WIDTH 48
#endif

#ifndef EGUI_CONFIG_PFB_HEIGHT
#define EGUI_CONFIG_PFB_HEIGHT 16
#endif

// HelloPerformance runs on PC/QEMU with synchronous tile flush, so extra PFB
// buffers do not create useful overlap and only increase static RAM.
#ifndef EGUI_CONFIG_PFB_BUFFER_COUNT
#define EGUI_CONFIG_PFB_BUFFER_COUNT 1
#endif

// HelloPerformance's largest built-in round/circle benchmark radius is 240,
// and the canvas lookup uses a strict `< range` check, so 241 is the minimum
// value that still preserves current coverage while trimming circle LUT/cache RAM.
#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 241

#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_INF

#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW 1
#define EGUI_CONFIG_FUNCTION_SUPPORT_MASK   1

#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1 1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2 1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8 1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8  1

// Enable external resource support.
// QEMU uses semihosting file I/O for resource loading.
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 1

// Enable image compression codecs for performance testing
#define EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE 1
#define EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE 1

// Enable row-band decode cache: first PFB tile decodes to cache,
// horizontal tile neighbors blend from cache without re-decoding.
#ifndef EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE 1
#endif

// HelloPerformance advances scenes through its timer/recording pipeline and
// does not rely on touch interaction, so the touch dispatcher and motion queue
// only consume persistent RAM in this app.
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH 0

/*
 * Optional low-RAM codec/decode policy overrides.
 *
 * Keep this block disabled by default so HelloPerformance uses the framework
 * defaults, which favor maximum rendering throughput over RAM savings.
 *
 * Turn this block on only when you want to measure the old low-RAM
 * HelloPerformance profile for SRAM-sensitive benchmarks.
 */
#if 0
// Compressed image resources are RGB565-only, so decode scratch/cache buffers
// only need 2 bytes per pixel instead of the default 4.
#define EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE 2

// Masked opaque fallback rows can borrow the codec row-cache alpha backing
// store instead of reserving a dedicated 240B decode-row alpha buffer in BSS.
#define EGUI_CONFIG_IMAGE_DECODE_OPAQUE_ALPHA_ROW_USE_ROW_CACHE 1

// Disable QOI decoder checkpoints to remove their persistent heap allocation.
#define EGUI_CONFIG_IMAGE_QOI_CHECKPOINT_COUNT 0

// Low-RAM QOI row-cache mode: only cache the horizontal tail instead of the
// full row-band. Saves about 1.5KB peak heap on 240px alpha-QOI scenes.
#define EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE 1

// External RLE read window: 64B instead of default 128B, saves 64B static RAM.
#define EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE 64

// Disable persistent "source alpha is fully opaque" metadata cache slots.
#define EGUI_CONFIG_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS 0

// Text-transform layout: 16-bit indices shrink transient heap for the current
// benchmark strings and font offsets.
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_PIXEL_INDEX_16BIT 1
#define EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_LINE_INDEX_16BIT  1

// Rotated-text scratch: keep layout/tile scratch on transient heap and cap the
// visible alpha8 fast-path cache at the smaller low-RAM ceiling.
#define EGUI_CONFIG_TEXT_TRANSFORM_SCRATCH_HEAP_ENABLE 1
#define EGUI_CONFIG_TEXT_TRANSFORM_VISIBLE_ALPHA8_MAX_BYTES 2560
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
