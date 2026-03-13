/**
 * @file egui_bus_gpio.h
 * @brief GPIO control interface
 *
 * Users implement this interface for their platform's GPIO pins.
 *
 * LCD and Touch drivers each have their own GPIO instance - RST pins are NOT shared.
 * All function pointers may be NULL if the pin is hardware-controlled or not needed.
 *
 * Pin usage by device type:
 * - LCD: RST, DC, CS
 * - Touch: RST, CS (SPI only), INT
 *
 * Note: Backlight control is handled by LCD driver's set_brightness(), not GPIO.
 */

#ifndef _EGUI_BUS_GPIO_H_
#define _EGUI_BUS_GPIO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * LCD GPIO operations interface
 */
typedef struct egui_lcd_gpio_ops {
    /**
     * Initialize GPIO pins.
     * Called once during driver init. May be NULL if already initialized.
     */
    void (*init)(void);

    /**
     * Deinitialize GPIO pins.
     * Called during driver deinit. May be NULL if not needed.
     */
    void (*deinit)(void);

    /**
     * Set Reset pin level.
     * @param level  0 = low (reset active), 1 = high (reset inactive)
     * May be NULL if RST is hardware-controlled or directly tied.
     */
    void (*set_rst)(uint8_t level);

    /**
     * Set Data/Command pin level.
     * @param level  0 = command mode, 1 = data mode
     * May be NULL if using 8080 interface with separate CMD/DATA lines.
     */
    void (*set_dc)(uint8_t level);

    /**
     * Set Chip Select pin level.
     * @param level  0 = selected (active), 1 = deselected
     * May be NULL if CS is hardware-controlled by SPI peripheral.
     */
    void (*set_cs)(uint8_t level);
} egui_lcd_gpio_ops_t;

/**
 * Touch GPIO operations interface
 */
typedef struct egui_touch_gpio_ops {
    /**
     * Initialize GPIO pins.
     * Called once during driver init. May be NULL if already initialized.
     */
    void (*init)(void);

    /**
     * Deinitialize GPIO pins.
     * Called during driver deinit. May be NULL if not needed.
     */
    void (*deinit)(void);

    /**
     * Set Reset pin level.
     * @param level  0 = low (reset active), 1 = high (reset inactive)
     * May be NULL if RST is hardware-controlled or directly tied.
     */
    void (*set_rst)(uint8_t level);

    /**
     * Set Chip Select pin level (SPI touch only).
     * @param level  0 = selected (active), 1 = deselected
     * May be NULL if CS is hardware-controlled by SPI peripheral.
     */
    void (*set_cs)(uint8_t level);

    /**
     * Set Interrupt pin level.
     * @param level  0 = low, 1 = high
     * Some touch chips (e.g., GT911) need INT pin control during init sequence.
     * May be NULL if not needed.
     */
    void (*set_int)(uint8_t level);

    /**
     * Get Interrupt pin level.
     * @return  0 = low (no interrupt), 1 = high (interrupt pending)
     * Used to check if touch data is ready before reading.
     * May be NULL if interrupt pin is not used or polling is preferred.
     */
    uint8_t (*get_int)(void);
} egui_touch_gpio_ops_t;

/**
 * Unified GPIO operations (for backward compatibility or generic use)
 */
typedef union egui_bus_gpio_ops {
    egui_lcd_gpio_ops_t lcd;
    egui_touch_gpio_ops_t touch;
} egui_bus_gpio_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BUS_GPIO_H_ */
