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

// Enable image tests (set to 0 to disable all image tests and save flash)
#define EGUI_TEST_CONFIG_IMAGE_565 1

// app_egui_config.h is included before egui_common.h, so define local port ids here.
// Enable external resource support for non-QEMU builds (QEMU lacks file I/O).
#if !defined(EGUI_PORT) || (EGUI_PORT != EGUI_PORT_TYPE_QEMU)
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 1
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
