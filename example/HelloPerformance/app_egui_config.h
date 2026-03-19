#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_CONFIG_SCEEN_WIDTH  480
#define EGUI_CONFIG_SCEEN_HEIGHT 480
#define EGUI_CONFIG_PFB_WIDTH    60
#define EGUI_CONFIG_PFB_HEIGHT   60

#define EGUI_CONFIG_MAX_FPS 1

#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 300

#define EGUI_CONFIG_DEBUG_LOG_LEVEL EGUI_LOG_IMPL_LEVEL_INF

#define EGUI_CONFIG_FUNCTION_CANVAS_DRAW_GRADIENT 1
#define EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW       1

#ifndef EGUI_PORT_TYPE_PC
#define EGUI_PORT_TYPE_PC 1
#endif

#ifndef EGUI_PORT_TYPE_QEMU
#define EGUI_PORT_TYPE_QEMU 3
#endif

// Enable large image tests (480px/240px direct-draw, resize, rotate)
// Set to 0 to save flash on constrained devices; 40x40 tiled tests still run.
#define EGUI_TEST_CONFIG_IMAGE_LARGE 0

// Enable external resource support.
// QEMU uses semihosting file I/O for resource loading.
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 1

// 480px alpha variant direct-draw tests (IMAGE_565_1/2/4) — disabled on QEMU to save flash.
// test_perf images have alpha=255, so these are identical to IMAGE_565_0 in performance.
#if EGUI_PORT == EGUI_PORT_TYPE_QEMU
#define EGUI_TEST_CONFIG_IMAGE_480_ALPHA 0
#else
#define EGUI_TEST_CONFIG_IMAGE_480_ALPHA 1
#endif

// Double-size (960x960) image tests - disabled on QEMU due to flash size limits
#if EGUI_PORT == EGUI_PORT_TYPE_QEMU
#define EGUI_TEST_CONFIG_IMAGE_DOUBLE 0
#else
#define EGUI_TEST_CONFIG_IMAGE_DOUBLE 1
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_EGUI_CONFIG_H_ */
