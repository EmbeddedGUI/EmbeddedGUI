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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //_VIRTUAL_TFT_PORT_H_
