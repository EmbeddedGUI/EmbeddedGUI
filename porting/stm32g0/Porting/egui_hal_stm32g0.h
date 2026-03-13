/**
 * @file egui_hal_stm32g0.h
 * @brief STM32G0 HAL Bus IO implementations for EGUI
 *
 * Provides SPI, I2C, and GPIO operations for LCD and Touch drivers.
 */

#ifndef _EGUI_HAL_STM32G0_H_
#define _EGUI_HAL_STM32G0_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "egui_bus_spi.h"
#include "egui_bus_i2c.h"
#include "egui_bus_gpio.h"
#include "egui_lcd.h"

/**
 * Get SPI bus operations for LCD.
 * Uses DMA for async transfers.
 */
const egui_bus_spi_ops_t *egui_hal_stm32g0_get_lcd_spi_ops(void);

/**
 * Get GPIO operations for LCD (RST, DC pins).
 */
const egui_lcd_gpio_ops_t *egui_hal_stm32g0_get_lcd_gpio_ops(void);

/**
 * Set LCD backlight brightness.
 * Use this as driver->set_brightness callback.
 * @param self   LCD driver instance (unused)
 * @param level  0 = off, >0 = on
 */
void egui_hal_stm32g0_set_backlight(egui_hal_lcd_driver_t *self, uint8_t level);

/**
 * Get I2C bus operations for Touch.
 */
const egui_bus_i2c_ops_t *egui_hal_stm32g0_get_touch_i2c_ops(void);

/**
 * Get GPIO operations for Touch (RST, INT pins).
 */
const egui_touch_gpio_ops_t *egui_hal_stm32g0_get_touch_gpio_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_HAL_STM32G0_H_ */
