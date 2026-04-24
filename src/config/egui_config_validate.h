#ifndef _EGUI_CONFIG_VALIDATE_H_
#define _EGUI_CONFIG_VALIDATE_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if (EGUI_CONFIG_COLOR_DEPTH != 8) && (EGUI_CONFIG_COLOR_DEPTH != 16) && (EGUI_CONFIG_COLOR_DEPTH != 32)
#error "EGUI_CONFIG_COLOR_DEPTH must be 8, 16, or 32."
#endif

#if (EGUI_CONFIG_SCEEN_WIDTH <= 0) || (EGUI_CONFIG_SCEEN_HEIGHT <= 0)
#error "EGUI_CONFIG_SCEEN_WIDTH and EGUI_CONFIG_SCEEN_HEIGHT must be > 0."
#endif

#if (EGUI_CONFIG_SCEEN_WIDTH > 32767) || (EGUI_CONFIG_SCEEN_HEIGHT > 32767)
#error "EGUI_CONFIG_SCEEN_WIDTH and EGUI_CONFIG_SCEEN_HEIGHT must fit in int16_t."
#endif

#if (EGUI_CONFIG_PFB_WIDTH <= 0) || (EGUI_CONFIG_PFB_HEIGHT <= 0)
#error "EGUI_CONFIG_PFB_WIDTH and EGUI_CONFIG_PFB_HEIGHT must be > 0."
#endif

#if (EGUI_CONFIG_PFB_WIDTH > 32767) || (EGUI_CONFIG_PFB_HEIGHT > 32767)
#error "EGUI_CONFIG_PFB_WIDTH and EGUI_CONFIG_PFB_HEIGHT must fit in int16_t."
#endif

#if (EGUI_CONFIG_PFB_BUFFER_COUNT <= 0) || (EGUI_CONFIG_PFB_BUFFER_COUNT > 4)
#error "EGUI_CONFIG_PFB_BUFFER_COUNT must be in range 1..4."
#endif

#if (EGUI_CONFIG_DIRTY_AREA_COUNT <= 0)
#error "EGUI_CONFIG_DIRTY_AREA_COUNT must be > 0."
#endif

#if (EGUI_CONFIG_MAX_FPS <= 0)
#error "EGUI_CONFIG_MAX_FPS must be > 0."
#endif

#if (EGUI_CONFIG_MAX_DISPLAY_COUNT <= 0)
#error "EGUI_CONFIG_MAX_DISPLAY_COUNT must be > 0."
#endif

#if (EGUI_CONFIG_MAX_DISPLAY_COUNT > 255)
#error "EGUI_CONFIG_MAX_DISPLAY_COUNT must fit in uint8_t display_id."
#endif

#if (EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT <= 0)
#error "EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT must be > 0."
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && (EGUI_CONFIG_INPUT_KEY_CACHE_COUNT <= 0)
#error "EGUI_CONFIG_INPUT_KEY_CACHE_COUNT must be > 0 when key support is enabled."
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && (EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX <= 0)
#error "EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX must be > 0 when touch support is enabled."
#endif

#if (EGUI_CONFIG_COLOR_16_SWAP != 0) && (EGUI_CONFIG_COLOR_16_SWAP != 1)
#error "EGUI_CONFIG_COLOR_16_SWAP must be 0 or 1."
#endif

#if (EGUI_CONFIG_SOFTWARE_ROTATION != 0) && (EGUI_CONFIG_SOFTWARE_ROTATION != 1)
#error "EGUI_CONFIG_SOFTWARE_ROTATION must be 0 or 1."
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_VALIDATE_H_ */
