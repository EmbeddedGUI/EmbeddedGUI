/**
 * @file egui_panel_io_i2c.h
 * @brief I2C Panel IO implementation
 *
 * Wraps egui_bus_i2c_ops_t and device address into a unified egui_panel_io_t.
 * Register addresses are passed as 'cmd' parameter in tx_param/rx_param.
 */

#ifndef _EGUI_PANEL_IO_I2C_H_
#define _EGUI_PANEL_IO_I2C_H_

#include "egui_panel_io.h"
#include "egui_bus_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * I2C Panel IO instance
 */
typedef struct egui_panel_io_i2c
{
    egui_panel_io_t base;          /**< Base interface (MUST be first member) */
    const egui_bus_i2c_ops_t *i2c; /**< I2C bus operations */
    uint8_t dev_addr;              /**< 7-bit I2C device address (shifted) */
} egui_panel_io_i2c_t;

/**
 * Initialize I2C Panel IO.
 *
 * @param io        User-provided storage
 * @param i2c       I2C bus operations (must not be NULL)
 * @param dev_addr  I2C device address (7-bit, shifted left by 1)
 */
void egui_panel_io_i2c_init(egui_panel_io_i2c_t *io, const egui_bus_i2c_ops_t *i2c, uint8_t dev_addr);

/**
 * Change device address at runtime.
 *
 * Used by drivers that support address fallback (e.g., GT911 tries
 * 0xBA then 0x28). The new address takes effect on the next IO call.
 *
 * @param io        I2C Panel IO instance
 * @param dev_addr  New I2C device address
 */
void egui_panel_io_i2c_set_addr(egui_panel_io_i2c_t *io, uint8_t dev_addr);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PANEL_IO_I2C_H_ */
