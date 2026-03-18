/**
 * @file egui_bus_i2c.h
 * @brief I2C Bus IO interface
 *
 * Users implement this interface for their platform's I2C peripheral.
 *
 * Return value convention: 0 = success, non-zero = failure
 *
 * This interface uses register-based read/write which is common for
 * touch controllers and OLED displays.
 */

#ifndef _EGUI_BUS_I2C_H_
#define _EGUI_BUS_I2C_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * I2C Bus operations interface
 */
typedef struct egui_bus_i2c_ops
{
    /**
     * Initialize I2C peripheral.
     * Called once during driver init. May be NULL if already initialized.
     */
    void (*init)(void);

    /**
     * Deinitialize I2C peripheral.
     * Called during driver deinit. May be NULL if not needed.
     */
    void (*deinit)(void);

    /**
     * Write data to a device register.
     * @param addr  7-bit I2C device address (shifted left by 1 for R/W bit)
     * @param reg   Register address (8 or 16 bit depending on device)
     * @param data  Data buffer to write
     * @param len   Number of bytes to write
     * @return 0 on success, non-zero on failure
     */
    int (*write_reg)(uint8_t addr, uint16_t reg, const uint8_t *data, uint16_t len);

    /**
     * Read data from a device register.
     * @param addr  7-bit I2C device address (shifted left by 1 for R/W bit)
     * @param reg   Register address (8 or 16 bit depending on device)
     * @param data  Buffer to receive data
     * @param len   Number of bytes to read
     * @return 0 on success, non-zero on failure
     */
    int (*read_reg)(uint8_t addr, uint16_t reg, uint8_t *data, uint16_t len);

    /**
     * Write raw bytes without a register-address phase.
     * Useful for devices that expect command streams or payload-only writes.
     * May be NULL when not needed by the current platform.
     */
    int (*write_raw)(uint8_t addr, const uint8_t *data, uint16_t len);

    /**
     * Read raw bytes without a register-address phase.
     * Useful for report-style devices that return payload directly on read.
     * May be NULL when not needed by the current platform.
     */
    int (*read_raw)(uint8_t addr, uint8_t *data, uint16_t len);

    /**
     * Perform a raw write followed by a raw read, typically using a repeated-start.
     * May be NULL when the platform cannot provide this transaction form.
     */
    int (*write_read)(uint8_t addr, const uint8_t *tx, uint16_t tx_len, uint8_t *rx, uint16_t rx_len);
} egui_bus_i2c_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BUS_I2C_H_ */
