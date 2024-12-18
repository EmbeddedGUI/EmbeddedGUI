#ifndef _APP_LCD_H_
#define _APP_LCD_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#include "port_main.h"

void app_lcd_polling_work(void);
void app_lcd_draw_data(int16_t x, int16_t y, int16_t width, int16_t height, void *data);
void app_lcd_power_on(void);
void app_lcd_power_off(void);
void app_lcd_init(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _APP_LCD_H_ */
