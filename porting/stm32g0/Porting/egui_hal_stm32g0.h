/**
 * @file egui_hal_stm32g0.h
 * @brief STM32G0 HAL Bus IO implementations for EGUI
 *
 * Provides SPI, I2C bus operations and individual GPIO functions
 * for LCD and Touch drivers using the unified Panel IO interface.
 */

#ifndef _EGUI_HAL_STM32G0_H_
#define _EGUI_HAL_STM32G0_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "egui_bus_spi.h"
#include "egui_bus_i2c.h"

/**
 * Get SPI bus operations for LCD.
 * Uses DMA for async transfers.
 */
const egui_bus_spi_ops_t *egui_hal_stm32g0_get_lcd_spi_ops(void);

/**
 * Get I2C bus operations for Touch.
 */
const egui_bus_i2c_ops_t *egui_hal_stm32g0_get_touch_i2c_ops(void);

/**
 * LCD GPIO control functions.
 * These are used individually with Panel IO and LCD driver init.
 */
void egui_hal_stm32g0_lcd_set_rst(uint8_t level);
void egui_hal_stm32g0_lcd_set_dc(uint8_t level);
void egui_hal_stm32g0_lcd_set_cs(uint8_t level);

/**
 * Touch GPIO control functions.
 * Used individually with Panel IO and Touch driver init.
 */
void egui_hal_stm32g0_touch_set_rst(uint8_t level);
uint8_t egui_hal_stm32g0_touch_get_int(void);

/**
 * LCD backlight brightness control.
 * @param level  0 = off, >0 = on
 */
void egui_hal_stm32g0_set_backlight_level(uint8_t level);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_HAL_STM32G0_H_ */
