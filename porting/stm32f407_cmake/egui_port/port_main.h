/**
 * @file port_main.h
 * @brief Board-level pin and peripheral mapping for the EGUI STM32 port.
 */

#ifndef _APP_EGUI_PORT_MAIN_H_
#define _APP_EGUI_PORT_MAIN_H_

#include "main.h"
#include "spi.h"

#define APP_EGUI_LCD_SPI hspi1

#define APP_EGUI_LCD_CS_PORT LCD_CS_GPIO_Port
#define APP_EGUI_LCD_CS_PIN LCD_CS_Pin
#define APP_EGUI_LCD_RST_PORT LCD_RST_GPIO_Port
#define APP_EGUI_LCD_RST_PIN LCD_RST_Pin
#define APP_EGUI_LCD_DC_PORT LCD_RS_GPIO_Port
#define APP_EGUI_LCD_DC_PIN LCD_RS_Pin
#define APP_EGUI_LCD_LED_PORT LCD_BL_GPIO_Port
#define APP_EGUI_LCD_LED_PIN LCD_BL_Pin

#define APP_EGUI_TOUCH_RST_PORT TP_RST_GPIO_Port
#define APP_EGUI_TOUCH_RST_PIN TP_RST_Pin
#define APP_EGUI_TOUCH_INT_PORT TP_INT_GPIO_Port
#define APP_EGUI_TOUCH_INT_PIN TP_INT_Pin
#define APP_EGUI_TOUCH_SCL_PORT TOUCH_SCL_GPIO_Port
#define APP_EGUI_TOUCH_SCL_PIN TOUCH_SCL_Pin
#define APP_EGUI_TOUCH_SDA_PORT TOUCH_SDA_GPIO_Port
#define APP_EGUI_TOUCH_SDA_PIN TOUCH_SDA_Pin

#endif /* _APP_EGUI_PORT_MAIN_H_ */
