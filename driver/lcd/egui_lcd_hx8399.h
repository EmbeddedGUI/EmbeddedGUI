/**
 * @file egui_lcd_hx8399.h
 * @brief HX8399 LCD driver
 *
 * HX8399 is a MIPI DSI interface LCD controller from Himax.
 * Supports RGB565, RGB666, RGB888 color modes.
 * The real panel path requires MIPI DSI-style pixel output rather than SPI RAMWR flushes.
 * Current EGUI implementation should therefore be treated as command-side bring-up code only,
 * not a production-verified DSI driver.
 */

#ifndef _EGUI_LCD_HX8399_H_
#define _EGUI_LCD_HX8399_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize HX8399 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param spi      SPI bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if all pins hardware-controlled)
 *
 * Use this in environments without malloc.
 */
void egui_lcd_hx8399_init(egui_hal_lcd_driver_t *storage,
                          const egui_bus_spi_ops_t *spi,
                          const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_HX8399_H_ */
