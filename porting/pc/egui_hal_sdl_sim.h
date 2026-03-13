#ifndef _EGUI_HAL_SDL_SIM_H_
#define _EGUI_HAL_SDL_SIM_H_

#include "egui_lcd.h"
#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

void egui_hal_sdl_lcd_setup(egui_hal_lcd_driver_t *storage);
void egui_hal_sdl_touch_setup(egui_hal_touch_driver_t *storage);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_HAL_SDL_SIM_H_ */
