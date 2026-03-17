/**
 * @file egui_lcd.h
 * @brief LCD Driver common interface
 *
 * This file defines the common interface for all LCD drivers.
 * Each LCD controller (ST7789, ILI9341, etc.) implements this interface.
 *
 * Uses unified Panel IO handle for all bus communication.
 * Device-level GPIO (RST) is kept separate from IO protocol.
 *
 * Note: Uses egui_hal_lcd_* prefix to avoid conflict with Core layer types.
 */

#ifndef _EGUI_HAL_LCD_H_
#define _EGUI_HAL_LCD_H_

#include <stdint.h>
#include "egui_panel_io.h"
#include "core/egui_display_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Vendor Init Command
 * ============================================================ */

/**
 * LCD vendor initialization command entry.
 *
 * Used to describe chip-specific register writes during initialization.
 * The driver iterates through an array of these and sends each via IO handle.
 */
typedef struct egui_lcd_vendor_init_cmd {
    int cmd;                   /**< Command/register address */
    const uint8_t *data;       /**< Parameter data (NULL if none) */
    size_t data_bytes;         /**< Size of data in bytes */
    unsigned int delay_ms;     /**< Delay in ms after sending this command */
} egui_lcd_vendor_init_cmd_t;

/**
 * Convenience macros for defining vendor init commands.
 *
 * Usage:
 *   static const egui_lcd_vendor_init_cmd_t my_init_cmds[] = {
 *       EGUI_LCD_CMD_WITH_PARAM(0, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02),
 *       EGUI_LCD_CMD_WITH_PARAM(0, 0xCF, 0x00, 0xC1, 0x30),
 *       EGUI_LCD_CMD_NO_PARAM(120, 0x11),
 *   };
 */
#define EGUI_LCD_CMD_WITH_PARAM(delay, command, ...) \
    {(command), (const uint8_t[]){__VA_ARGS__}, sizeof((const uint8_t[]){__VA_ARGS__}), (delay)}

#define EGUI_LCD_CMD_NO_PARAM(delay, command) \
    {(command), NULL, 0, (delay)}

/* ============================================================
 * LCD Configuration
 * ============================================================ */

/* Forward declaration */
typedef struct egui_hal_lcd_config egui_hal_lcd_config_t;

/**
 * Custom initialization callback type.
 *
 * If provided in config, this callback replaces the driver's built-in
 * initialization sequence (after hardware reset).
 * The callback has full control: it should send all commands needed
 * to bring the LCD into operational state (SLPOUT, COLMOD, MADCTL,
 * vendor-specific registers, DISPON, etc.).
 *
 * @param io      Panel IO handle for sending commands
 * @param config  LCD configuration
 * @return 0 on success, non-zero on failure
 */
typedef int (*egui_lcd_custom_init_fn)(egui_panel_io_handle_t io, const egui_hal_lcd_config_t *config);

/**
 * LCD configuration structure
 */
struct egui_hal_lcd_config {
    uint16_t width;           /**< Screen width in pixels */
    uint16_t height;          /**< Screen height in pixels */
    uint8_t color_depth;      /**< Color depth: 1, 8, 16, or 24 bits */
    uint8_t color_swap;       /**< Byte order swap (for RGB565) */
    int16_t x_offset;         /**< X offset for partial displays */
    int16_t y_offset;         /**< Y offset for partial displays */
    uint8_t invert_color;     /**< Color inversion (0 = normal, 1 = inverted) */
    uint8_t mirror_x;         /**< X axis mirror */
    uint8_t mirror_y;         /**< Y axis mirror */
    uint8_t swap_xy;          /**< Swap X and Y axes */

    /**
     * Custom init callback.
     * If non-NULL, replaces the entire driver init sequence after hardware reset.
     * The callback has full control over the initialization.
     */
    egui_lcd_custom_init_fn custom_init;
};

/* ============================================================
 * LCD Driver
 * ============================================================ */

/**
 * LCD driver instance (forward declaration)
 */
typedef struct egui_hal_lcd_driver egui_hal_lcd_driver_t;

/**
 * LCD driver structure
 *
 * Lifecycle: factory_init → reset → init → draw_area/... → del
 *
 * Uses a unified Panel IO handle for all bus communication.
 * Device-level GPIO (RST) is managed internally by reset/del.
 */
struct egui_hal_lcd_driver {
    const char *name;                    /**< Driver name (e.g., "ST7789") */

    /* Lifecycle: reset → init → ... → del */
    int (*reset)(egui_hal_lcd_driver_t *self);   /**< Hardware reset (RST low→high) */
    int (*init)(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config);
    void (*del)(egui_hal_lcd_driver_t *self);    /**< Pull RST low + clear struct */

    /* Core drawing */
    void (*draw_area)(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
                      int16_t w, int16_t h, const void *data, uint32_t len);

    /* Optional features */
    void (*mirror)(egui_hal_lcd_driver_t *self, uint8_t mirror_x, uint8_t mirror_y);
    void (*swap_xy)(egui_hal_lcd_driver_t *self, uint8_t swap);
    void (*set_power)(egui_hal_lcd_driver_t *self, uint8_t on);
    void (*set_invert)(egui_hal_lcd_driver_t *self, uint8_t invert);

    /* IO and GPIO */
    egui_panel_io_handle_t io;           /**< Unified IO handle for commands/data */
    void (*set_rst)(uint8_t level);      /**< Device RST pin. Used by reset/del internally */

    /* State */
    egui_hal_lcd_config_t config;        /**< Current configuration */
    void *priv;                          /**< Driver-specific private data */
};

/* ============================================================
 * Helper Functions
 * ============================================================ */

/**
 * Send vendor init commands via IO handle.
 *
 * @param io    Panel IO handle
 * @param cmds  Array of vendor init commands
 * @param size  Number of commands in the array
 * @return 0 on success, non-zero on failure
 */
int egui_lcd_send_vendor_init_cmds(egui_panel_io_handle_t io,
                                    const egui_lcd_vendor_init_cmd_t *cmds,
                                    uint16_t size);

/**
 * Hardware reset sequence.
 * Pulls RST low, delays, pulls RST high, delays.
 * No-op if set_rst is NULL.
 *
 * @param self      LCD driver instance
 * @param delay_ms  Delay in ms for each phase
 */
void egui_lcd_hw_reset(egui_hal_lcd_driver_t *self, int delay_ms);

/**
 * Register HAL LCD driver with Core display system.
 *
 * Calls reset(), init(), then creates and registers a Core display driver.
 * The porting layer provides optional callbacks for board-specific features
 * (brightness, power, rotation) that are not part of the LCD driver itself.
 *
 * @param lcd     HAL LCD driver instance (factory_init already called)
 * @param config  LCD configuration
 */
void egui_hal_lcd_register(egui_display_driver_t *driver, egui_hal_lcd_driver_t *lcd,
                            const egui_hal_lcd_config_t *config);

/**
 * Get the registered HAL LCD driver instance.
 * @return HAL LCD driver pointer, or NULL if not registered
 */
egui_hal_lcd_driver_t *egui_hal_lcd_get(void);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_HAL_LCD_H_ */
