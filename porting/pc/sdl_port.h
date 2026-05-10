#ifndef _VIRTUAL_TFT_PORT_H_
#define _VIRTUAL_TFT_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include "egui.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define VT_WIDTH       EGUI_CONFIG_SCREEN_WIDTH
#define VT_HEIGHT      EGUI_CONFIG_SCREEN_HEIGHT
#define VT_COLOR_DEPTH EGUI_CONFIG_COLOR_DEPTH

/**
 * SDL display options (PC simulator only).
 * When enabled and color depth is 16, SDL uses native RGB565 texture
 * to accurately simulate embedded display color quality.
 * When 0 (default), always converts to ARGB8888 for display.
 */
#ifndef EGUI_CONFIG_SDL_NATIVE_COLOR
#define EGUI_CONFIG_SDL_NATIVE_COLOR 0
#endif

#if EGUI_CONFIG_SDL_NATIVE_COLOR && VT_COLOR_DEPTH == 16
#define VT_SDL_NATIVE_RGB565 1
#else
#define VT_SDL_NATIVE_RGB565 0
#endif

#define VT_VIRTUAL_MACHINE 0 /*Different rendering should be used if running in a Virtual machine*/

#ifndef EGUI_PC_LOG_TO_DEBUG_OUTPUT
#define EGUI_PC_LOG_TO_DEBUG_OUTPUT 0
#endif

extern void VT_init(void);
extern bool VT_is_request_quit(void);
extern void VT_begin_shutdown(void);
extern void VT_deinit(void);
extern void VT_sdl_flush(int32_t nMS);
extern void VT_sdl_flush_core(egui_core_t *core, int32_t nMS);
extern void VT_sdl_refresh_task(void);
extern void sdl_port_request_refresh(void);

extern uint32_t sdl_get_system_timestamp_ms(void);
extern void VT_Fill_Multiple_Colors(int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t *color_p);
extern void VT_Fill_Multiple_Colors_Core(egui_core_t *core, int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t *color_p);
extern void sdl_port_sleep(uint32_t nMS);
extern void egui_pc_log_init(void);
extern void egui_pc_log(const char *format, ...);
extern void egui_pc_vlog(const char *format, va_list args);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
extern int sdl_port_touch_read(egui_core_t *core, egui_touch_driver_data_t *data);
#endif
extern void egui_port_init(void);
extern egui_display_driver_t *egui_port_get_display_driver(void);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
extern void egui_port_register_touch_driver(egui_core_t *core);
#endif
extern void egui_port_register_core(egui_core_t *core);
extern egui_core_t *egui_port_get_core_by_display_id(int display_id);
typedef void (*egui_port_core_task_func_t)(egui_core_t *core, uintptr_t user_data);
extern int egui_port_post_core_task(egui_core_t *core, egui_port_core_task_func_t task_func, uintptr_t user_data);
extern int egui_port_post_core_task_named(egui_core_t *core, egui_port_core_task_func_t task_func, uintptr_t user_data, const char *context);
extern int egui_port_post_core_task_sync(egui_core_t *core, egui_port_core_task_func_t task_func, uintptr_t user_data, uint32_t timeout_ms);
extern int egui_port_post_core_task_sync_named(egui_core_t *core, egui_port_core_task_func_t task_func, uintptr_t user_data, uint32_t timeout_ms,
                                               const char *context);

typedef struct egui_port_display_runtime_info
{
    int16_t physical_width;
    int16_t physical_height;
    egui_display_rotation_t rotation;
    uint8_t software_rotation;
    uint8_t has_hardware_rotation;
} egui_port_display_runtime_info_t;

extern int egui_port_get_display_runtime_info(egui_core_t *core, egui_port_display_runtime_info_t *info);
extern int egui_port_get_view_center(egui_core_t *core, egui_view_t *view, int *x, int *y);

typedef struct egui_port_core_task_queue_metrics
{
    uint16_t queue_capacity;
    uint16_t pending_count;
    uint16_t inflight_count;
    uint16_t peak_count;
    const char *pending_context;
    const char *inflight_context;
    uint16_t last_reject_pending_count;
    uint16_t last_reject_inflight_count;
    const char *last_reject_context;
    const char *last_reject_pending_context;
    const char *last_reject_inflight_context;
    uint32_t post_success_count;
    uint32_t post_retry_count;
    uint32_t post_max_retry_burst;
    uint32_t post_reject_count;
    uint32_t wait_timeout_count;
    uint32_t max_queue_wait_ms;
    const char *max_queue_wait_context;
    uint32_t max_exec_time_ms;
    const char *max_exec_time_context;
} egui_port_core_task_queue_metrics_t;

extern int egui_port_get_core_task_queue_metrics(egui_core_t *core, egui_port_core_task_queue_metrics_t *metrics);

#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
typedef struct egui_port_extra_display_descriptor
{
    int screen_width;
    int screen_height;
    int pfb_width;
    int pfb_height;
    egui_color_int_t **pfb_buffers;
    int pfb_buffer_count;
    const egui_core_render_config_t *render_config;
    egui_touch_register_func_t touch_register;
    egui_uicode_init_func_t uicode_init;
} egui_port_extra_display_descriptor_t;

/**
 * App hook for providing additional display descriptors on PC.
 * Descriptors are mapped to display ids 1..N in array order.
 */
extern int egui_port_get_additional_display_descriptors(egui_port_extra_display_descriptor_t *descriptors, int max_count);
#endif
// extern
// bool VT_mouse_get_location(arm_2d_location_t *ptLocation);

/*
 * Recording/session APIs are process-global for a single PC simulator runtime.
 * They are not core-scoped and should not be used to drive multiple independent
 * simulator instances within the same process.
 */
extern void recording_init(const char *output_dir, int fps, int duration_sec);
extern void recording_set_speed(int speed);
extern void recording_set_clock_scale(int scale);
extern void recording_set_snapshot_settle_ms(int settle_ms);
extern void recording_set_snapshot_stability(int stable_cycles, int max_wait_ms);
extern bool recording_is_enabled(void);
extern bool recording_is_finished(void);
extern void recording_request_snapshot(void);
extern void sdl_port_set_headless(bool headless);

#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
extern void sdl_port_add_display(int display_id, int16_t w, int16_t h);

/**
 * Create a sub-display driver for multi-display PC simulation.
 * Allocates a new display driver and creates an SDL window inside the
 * current simulator process-global SDL session.
 *
 * @param display_id  Display ID (1, 2, ...)
 * @param w           Screen width
 * @param h           Screen height
 * @return            Display driver pointer, or NULL on failure
 */
extern egui_display_driver_t *egui_port_create_sub_display(egui_core_t *core, int display_id, int16_t w, int16_t h);

#endif

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
#include "core/egui_input_simulator.h"

/**
 * Weak function for apps to define custom actions during recording.
 * Override this function in your app to customize simulation behavior.
 * Supports click, drag, swipe, and wait actions.
 * @param action_index The current action index (0, 1, 2, ...)
 * @param p_action Pointer to store action details
 * @return true if action is valid, false to stop simulation
 */
extern bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //_VIRTUAL_TFT_PORT_H_
