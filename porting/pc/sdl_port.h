#ifndef _VIRTUAL_TFT_PORT_H_
#define _VIRTUAL_TFT_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "egui.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define VT_WIDTH       EGUI_CONFIG_SCEEN_WIDTH
#define VT_HEIGHT      EGUI_CONFIG_SCEEN_HEIGHT
#define VT_COLOR_DEPTH EGUI_CONFIG_COLOR_DEPTH

#if EGUI_CONFIG_SDL_NATIVE_COLOR && VT_COLOR_DEPTH == 16
#define VT_SDL_NATIVE_RGB565 1
#else
#define VT_SDL_NATIVE_RGB565 0
#endif

#define VT_VIRTUAL_MACHINE 0 /*Different rendering should be used if running in a Virtual machine*/

extern void VT_init(void);
extern bool VT_is_request_quit(void);
extern void VT_deinit(void);
extern void VT_sdl_flush(int32_t nMS);
extern void VT_sdl_refresh_task(void);

extern uint32_t sdl_get_system_timestamp_ms(void);
extern void VT_sdl_flush(int32_t nMS);
extern void VT_Fill_Multiple_Colors(int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t *color_p);
extern void sdl_port_sleep(uint32_t nMS);
// extern
// bool VT_mouse_get_location(arm_2d_location_t *ptLocation);

// Recording functions for GIF generation
extern void recording_init(const char *output_dir, int fps, int duration_sec);
extern void recording_set_speed(int speed);
extern void recording_set_clock_scale(int scale);
extern void recording_set_snapshot_settle_ms(int settle_ms);
extern void recording_set_snapshot_stability(int stable_cycles, int max_wait_ms);
extern bool recording_is_enabled(void);
extern bool recording_is_finished(void);
extern void recording_request_snapshot(void);
extern void sdl_port_set_headless(bool headless);

#if EGUI_CONFIG_RECORDING_TEST
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
