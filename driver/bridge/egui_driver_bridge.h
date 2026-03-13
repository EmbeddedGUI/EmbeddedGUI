/**
 * @file egui_driver_bridge.h
 * @brief Bridge layer connecting new HAL drivers to Core interfaces
 *
 * This layer adapts the new egui_hal_lcd_driver_t and egui_hal_touch_driver_t
 * to the existing egui_display_driver_t and touch input system.
 *
 * Multi-touch handling:
 * - New drivers support up to 5 touch points
 * - Bridge converts multi-touch to single-point events for Core
 * - First touch point is the primary point
 */

#ifndef _EGUI_DRIVER_BRIDGE_H_
#define _EGUI_DRIVER_BRIDGE_H_

#include "egui_lcd.h"
#include "egui_touch.h"
#include "core/egui_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create display driver from LCD driver.
 *
 * Wraps an egui_hal_lcd_driver_t to create an egui_display_driver_t
 * that can be registered with the Core layer.
 *
 * @param lcd  LCD driver instance (must be initialized)
 * @return Display driver instance for Core registration
 *
 * Note: The returned pointer points to static storage - only one
 * display driver can be active at a time.
 */
egui_display_driver_t *egui_display_driver_from_lcd(egui_hal_lcd_driver_t *lcd);

/**
 * Register touch driver with bridge and Core.
 *
 * Creates a Core-compatible touch driver wrapper and registers it
 * with egui_touch_driver_register(). The Core's egui_input_polling_work()
 * will automatically poll the touch driver.
 *
 * @param touch  Touch driver instance (must be initialized)
 */
void egui_touch_driver_bridge_register(egui_hal_touch_driver_t *touch);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_DRIVER_BRIDGE_H_ */
