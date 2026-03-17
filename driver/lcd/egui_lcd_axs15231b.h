/**
 * @file egui_lcd_axs15231b.h
 * @brief AXS15231B LCD driver
 *
 * AXS15231B is a SPI/QSPI interface LCD controller.
 * Supports RGB565, RGB666, RGB888 color modes.
 */

#ifndef _EGUI_LCD_AXS15231B_H_
#define _EGUI_LCD_AXS15231B_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize AXS15231B driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param spi      SPI bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if all pins hardware-controlled)
 *
 * Use this in environments without malloc.
 *
 * Note: set_brightness is NULL by default. Porting layer should set
 * driver->set_brightness after init if backlight control is needed.
 */
void egui_lcd_axs15231b_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_AXS15231B_H_ */
