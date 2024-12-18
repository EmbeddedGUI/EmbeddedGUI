#ifndef _PORT_MAIN_H_
#define _PORT_MAIN_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// FUNCTION Select
#define APP_EGUI_LCD_SPI   hspi1
#define APP_EGUI_TOUCH_I2C hi2c1

#define APP_EGUI_FUNCTION_IO_0_PORT GPIOA
#define APP_EGUI_FUNCTION_IO_0_PIN  GPIO_PIN_3

#define APP_EGUI_FUNCTION_IO_1_PORT GPIOA
#define APP_EGUI_FUNCTION_IO_1_PIN  GPIO_PIN_5

#define APP_EGUI_FUNCTION_IO_2_PORT GPIOB
#define APP_EGUI_FUNCTION_IO_2_PIN  GPIO_PIN_0

#define APP_EGUI_FUNCTION_IO_3_PORT GPIOB
#define APP_EGUI_FUNCTION_IO_3_PIN  GPIO_PIN_1

#define APP_EGUI_FUNCTION_IO_4_PORT GPIOB
#define APP_EGUI_FUNCTION_IO_4_PIN  GPIO_PIN_2

#define APP_EGUI_LCD_RST_PORT APP_EGUI_FUNCTION_IO_0_PORT
#define APP_EGUI_LCD_RST_PIN  APP_EGUI_FUNCTION_IO_0_PIN

#define APP_EGUI_LCD_DC_PORT APP_EGUI_FUNCTION_IO_1_PORT
#define APP_EGUI_LCD_DC_PIN  APP_EGUI_FUNCTION_IO_1_PIN

#define APP_EGUI_LCD_LED_PORT APP_EGUI_FUNCTION_IO_2_PORT
#define APP_EGUI_LCD_LED_PIN  APP_EGUI_FUNCTION_IO_2_PIN

#define APP_EGUI_TOUCH_RST_PORT APP_EGUI_FUNCTION_IO_3_PORT
#define APP_EGUI_TOUCH_RST_PIN  APP_EGUI_FUNCTION_IO_3_PIN

#define APP_EGUI_TOUCH_INT_PORT APP_EGUI_FUNCTION_IO_4_PORT
#define APP_EGUI_TOUCH_INT_PIN  APP_EGUI_FUNCTION_IO_4_PIN



#define APP_EGUI_CONFIG_PFB_SECTION  __attribute__((section(".bss.pfb_area")))
#define APP_EGUI_CONFIG_LCD_ENABLE_BACKUP_PFB 1
#define APP_EGUI_CONFIG_USE_DMA_TO_RESET_PFB_BUFFER 1

void app_set_gpio(uint8_t pin, uint8_t state);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _PORT_MAIN_H_ */
