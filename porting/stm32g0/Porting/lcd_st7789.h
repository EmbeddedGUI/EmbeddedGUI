#ifndef _LCD_ST7789_H_
#define _LCD_ST7789_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#include "port_main.h"

/* choose a Hardware SPI port to use. */
#define ST7789_SPI_PORT APP_EGUI_LCD_SPI
extern SPI_HandleTypeDef ST7789_SPI_PORT;

/* Pin connection*/
#define ST7789_RST_PORT APP_EGUI_LCD_RST_PORT
#define ST7789_RST_PIN  APP_EGUI_LCD_RST_PIN
#define ST7789_DC_PORT  APP_EGUI_LCD_DC_PORT
#define ST7789_DC_PIN   APP_EGUI_LCD_DC_PIN

/*
 * Comment one to use another.
 * 3 parameters can be choosed
 * 240x240 & 240x320
 * ST7789_X_SHIFT & ST7789_Y_SHIFT are used to adapt different display's resolution
 */

/* Choose a type you are using */
//  #define ST7789_SCREEN_240_240
#define ST7789_SCREEN_240_320

/* Choose a display rotation you want to use: (0-3) */
// #define ST7789_ROTATION 0
// #define ST7789_ROTATION 1
#define ST7789_ROTATION 2 //  use Normally on 240x240
// #define ST7789_ROTATION 3

#ifdef ST7789_SCREEN_240_240

#define ST7789_WIDTH  240
#define ST7789_HEIGHT 240

#if ST7789_ROTATION == 0
#define ST7789_X_SHIFT 0
#define ST7789_Y_SHIFT 80
#elif ST7789_ROTATION == 1
#define ST7789_X_SHIFT 80
#define ST7789_Y_SHIFT 0
#elif ST7789_ROTATION == 2
#define ST7789_X_SHIFT 0
#define ST7789_Y_SHIFT 0
#elif ST7789_ROTATION == 3
#define ST7789_X_SHIFT 0
#define ST7789_Y_SHIFT 0
#endif

#endif

#ifdef ST7789_SCREEN_240_320

#if ST7789_ROTATION == 0
#define ST7789_WIDTH   240
#define ST7789_HEIGHT  320
#define ST7789_X_SHIFT 0
#define ST7789_Y_SHIFT 0
#endif

#if ST7789_ROTATION == 1
#define ST7789_WIDTH   320
#define ST7789_HEIGHT  240
#define ST7789_X_SHIFT 0
#define ST7789_Y_SHIFT 0
#endif

#if ST7789_ROTATION == 2
#define ST7789_WIDTH   240
#define ST7789_HEIGHT  320
#define ST7789_X_SHIFT 0
#define ST7789_Y_SHIFT 0
#endif

#if ST7789_ROTATION == 3
#define ST7789_WIDTH   320
#define ST7789_HEIGHT  240
#define ST7789_X_SHIFT 0
#define ST7789_Y_SHIFT 0
#endif

#endif

/* Control Registers and constant codes */
#define ST7789_NOP     0x00
#define ST7789_SWRESET 0x01
#define ST7789_RDDID   0x04
#define ST7789_RDDST   0x09

#define ST7789_SLPIN  0x10
#define ST7789_SLPOUT 0x11
#define ST7789_PTLON  0x12
#define ST7789_NORON  0x13

#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E

#define ST7789_PTLAR  0x30
#define ST7789_COLMOD 0x3A
#define ST7789_MADCTL 0x36

/**
 * Memory Data Access Control Register (0x36H)
 * MAP:     D7  D6  D5  D4  D3  D2  D1  D0
 * param:   MY  MX  MV  ML  RGB MH  -   -
 *
 */

/* Page Address Order ('0': Top to Bottom, '1': the opposite) */
#define ST7789_MADCTL_MY  0x80
/* Column Address Order ('0': Left to Right, '1': the opposite) */
#define ST7789_MADCTL_MX  0x40
/* Page/Column Order ('0' = Normal Mode, '1' = Reverse Mode) */
#define ST7789_MADCTL_MV  0x20
/* Line Address Order ('0' = LCD Refresh Top to Bottom, '1' = the opposite) */
#define ST7789_MADCTL_ML  0x10
/* RGB/BGR Order ('0' = RGB, '1' = BGR) */
#define ST7789_MADCTL_RGB 0x00

#define ST7789_RDID1 0xDA
#define ST7789_RDID2 0xDB
#define ST7789_RDID3 0xDC
#define ST7789_RDID4 0xDD

/* Advanced options */
#define ST7789_COLOR_MODE_16bit 0x55 //  RGB565 (16bit)
#define ST7789_COLOR_MODE_18bit 0x66 //  RGB666 (18bit)

/* Basic operations */
#define ST7789_RST_CLR() HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_RESET)
#define ST7789_RST_SET() HAL_GPIO_WritePin(ST7789_RST_PORT, ST7789_RST_PIN, GPIO_PIN_SET)

#define ST7789_DC_CLR() HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_RESET)
#define ST7789_DC_SET() HAL_GPIO_WritePin(ST7789_DC_PORT, ST7789_DC_PIN, GPIO_PIN_SET)

void st7789_draw_image_dma_cache(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void st7789_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void st7789_init(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _LCD_ST7789_H_ */
