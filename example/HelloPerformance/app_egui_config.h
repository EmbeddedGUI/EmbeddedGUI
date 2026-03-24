#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_SCEEN_WIDTH  240
#define EGUI_CONFIG_SCEEN_HEIGHT 240

#define EGUI_CONFIG_MAX_FPS 1

#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 300

#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_INF

#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW 1
#define EGUI_CONFIG_FUNCTION_SUPPORT_MASK   1

#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_1 1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_2 1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8 1
#define EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8  1

// Enable large image tests (480px/240px direct-draw, resize, rotate)
// Set to 0 to save flash on constrained devices; 40x40 tiled tests still run.
#define EGUI_TEST_CONFIG_IMAGE_LARGE 1

// Enable external resource support.
// QEMU uses semihosting file I/O for resource loading.
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 1

// Double-size (480x480) image tests - always enabled (was 960x960 before, now small enough for QEMU too)
#define EGUI_TEST_CONFIG_IMAGE_DOUBLE 1

// Enable image compression codecs for performance testing
#define EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE 1
#define EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE 1

// Enable row-band decode cache: first PFB tile decodes to cache,
// horizontal tile neighbors blend from cache without re-decoding.
// RAM cost: PFB_HEIGHT * SCREEN_WIDTH * 5 = 30*240*5 = 36KB
#define EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE 1

// Keep one decoded compressed image across repeated refreshes.
// Covers the largest HelloPerformance compressed case: 240x240 RGB565 + alpha8.
#define EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES (EGUI_CONFIG_SCEEN_WIDTH * EGUI_CONFIG_SCEEN_HEIGHT * 3)

// Keep one external raw image across repeated refreshes.
// Covers the largest HelloPerformance external RGB565 + alpha8 case: 240x240.
#define EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES (EGUI_CONFIG_SCEEN_WIDTH * EGUI_CONFIG_SCEEN_HEIGHT * 3)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
