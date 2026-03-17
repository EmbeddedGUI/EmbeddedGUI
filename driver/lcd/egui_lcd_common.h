/**
 * @file egui_lcd_common.h
 * @brief Common MIPI DCS commands and helper macros for LCD drivers
 *
 * Based on MIPI DCS v1.02.00 specification.
 * Reference: LVGL lv_lcd_generic_mipi.h, ESP-IDF esp_lcd_panel_commands.h
 */

#ifndef _EGUI_LCD_COMMON_H_
#define _EGUI_LCD_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

/* MIPI DCS Commands - Basic */
#define LCD_CMD_NOP          0x00
#define LCD_CMD_SWRESET      0x01  /* Software Reset */
#define LCD_CMD_RDDID        0x04  /* Read Display ID */
#define LCD_CMD_RDDST        0x09  /* Read Display Status */
#define LCD_CMD_RDDPM        0x0A  /* Read Display Power Mode */
#define LCD_CMD_RDDMADCTL    0x0B  /* Read Display MADCTL */
#define LCD_CMD_RDDCOLMOD    0x0C  /* Read Display Pixel Format */
#define LCD_CMD_RDDIM        0x0D  /* Read Display Image Mode */
#define LCD_CMD_RDDSM        0x0E  /* Read Display Signal Mode */
#define LCD_CMD_RDDSDR       0x0F  /* Read Display Self-Diagnostic Result */

/* MIPI DCS Commands - Sleep */
#define LCD_CMD_SLPIN        0x10  /* Sleep In */
#define LCD_CMD_SLPOUT       0x11  /* Sleep Out */
#define LCD_CMD_PTLON        0x12  /* Partial Mode On */
#define LCD_CMD_NORON        0x13  /* Normal Display Mode On */

/* MIPI DCS Commands - Display */
#define LCD_CMD_INVOFF       0x20  /* Display Inversion Off */
#define LCD_CMD_INVON        0x21  /* Display Inversion On */
#define LCD_CMD_GAMSET       0x26  /* Gamma Set */
#define LCD_CMD_DISPOFF      0x28  /* Display Off */
#define LCD_CMD_DISPON       0x29  /* Display On */

/* MIPI DCS Commands - Column/Row Address */
#define LCD_CMD_CASET        0x2A  /* Column Address Set */
#define LCD_CMD_RASET        0x2B  /* Row Address Set (Page Address Set) */
#define LCD_CMD_RAMWR        0x2C  /* Memory Write */
#define LCD_CMD_RAMRD        0x2E  /* Memory Read */

/* MIPI DCS Commands - Partial Area */
#define LCD_CMD_PTLAR        0x30  /* Partial Area */
#define LCD_CMD_VSCRDEF      0x33  /* Vertical Scrolling Definition */
#define LCD_CMD_TEOFF        0x34  /* Tearing Effect Line Off */
#define LCD_CMD_TEON         0x35  /* Tearing Effect Line On */
#define LCD_CMD_MADCTL       0x36  /* Memory Access Control */
#define LCD_CMD_VSCSAD       0x37  /* Vertical Scroll Start Address */
#define LCD_CMD_IDMOFF       0x38  /* Idle Mode Off */
#define LCD_CMD_IDMON        0x39  /* Idle Mode On */
#define LCD_CMD_COLMOD       0x3A  /* Interface Pixel Format */
#define LCD_CMD_RAMWRC       0x3C  /* Memory Write Continue */
#define LCD_CMD_RAMRDC       0x3E  /* Memory Read Continue */

/* MIPI DCS Commands - Brightness */
#define LCD_CMD_WRDISBV      0x51  /* Write Display Brightness */
#define LCD_CMD_RDDISBV      0x52  /* Read Display Brightness */
#define LCD_CMD_WRCTRLD      0x53  /* Write CTRL Display */
#define LCD_CMD_RDCTRLD      0x54  /* Read CTRL Display */

/* MADCTL bits */
#define LCD_MADCTL_MY        0x80  /* Row Address Order (0=Top to Bottom) */
#define LCD_MADCTL_MX        0x40  /* Column Address Order (0=Left to Right) */
#define LCD_MADCTL_MV        0x20  /* Row/Column Exchange */
#define LCD_MADCTL_ML        0x10  /* Vertical Refresh Order */
#define LCD_MADCTL_BGR       0x08  /* BGR Order (0=RGB, 1=BGR) */
#define LCD_MADCTL_MH        0x04  /* Horizontal Refresh Order */
#define LCD_MADCTL_RGB       0x00  /* RGB Order */

/* Color modes (COLMOD) */
#define LCD_COLMOD_12BIT     0x33  /* 12-bit/pixel */
#define LCD_COLMOD_16BIT     0x55  /* 16-bit/pixel (RGB565) */
#define LCD_COLMOD_18BIT     0x66  /* 18-bit/pixel (RGB666) */
#define LCD_COLMOD_24BIT     0x77  /* 24-bit/pixel (RGB888) */

/* SSD1306 specific commands */
#define SSD1306_CMD_SET_CONTRAST         0x81
#define SSD1306_CMD_DISPLAY_ALL_ON_RESUME 0xA4
#define SSD1306_CMD_DISPLAY_ALL_ON       0xA5
#define SSD1306_CMD_NORMAL_DISPLAY       0xA6
#define SSD1306_CMD_INVERT_DISPLAY       0xA7
#define SSD1306_CMD_DISPLAY_OFF          0xAE
#define SSD1306_CMD_DISPLAY_ON           0xAF
#define SSD1306_CMD_SET_DISPLAY_OFFSET   0xD3
#define SSD1306_CMD_SET_COM_PINS         0xDA
#define SSD1306_CMD_SET_VCOM_DETECT      0xDB
#define SSD1306_CMD_SET_DISPLAY_CLOCK_DIV 0xD5
#define SSD1306_CMD_SET_PRECHARGE        0xD9
#define SSD1306_CMD_SET_MULTIPLEX        0xA8
#define SSD1306_CMD_SET_LOW_COLUMN       0x00
#define SSD1306_CMD_SET_HIGH_COLUMN      0x10
#define SSD1306_CMD_SET_START_LINE       0x40
#define SSD1306_CMD_MEMORY_MODE          0x20
#define SSD1306_CMD_SET_PAGE_ADDR        0xB0
#define SSD1306_CMD_COM_SCAN_INC         0xC0
#define SSD1306_CMD_COM_SCAN_DEC         0xC8
#define SSD1306_CMD_SEG_REMAP            0xA0
#define SSD1306_CMD_CHARGE_PUMP          0x8D

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_COMMON_H_ */
