/**
 * @file egui_panel_io.h
 * @brief Unified Panel IO interface for LCD and Touch drivers
 *
 * Inspired by ESP-IDF's esp_lcd_panel_io_handle_t design.
 * All LCD and Touch drivers communicate through this abstract interface,
 * decoupling them from the underlying bus type (SPI, I2C, 8080).
 *
 * Key methods:
 * - tx_param: Send command with parameter data (blocking)
 * - rx_param: Read data from a register/command (blocking)
 * - tx_color: Send pixel/color data (may be async with DMA)
 * - wait_tx_done: Wait for async tx_color to complete
 */

#ifndef _EGUI_PANEL_IO_H_
#define _EGUI_PANEL_IO_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Panel IO instance (forward declaration)
 */
typedef struct egui_panel_io egui_panel_io_t;

/**
 * Panel IO handle (pointer to Panel IO instance)
 */
typedef egui_panel_io_t *egui_panel_io_handle_t;

/**
 * Unified Panel IO interface
 *
 * Implementations:
 * - egui_panel_io_spi_t  (SPI with DC/CS GPIO)
 * - egui_panel_io_i2c_t  (I2C with device address)
 * - egui_panel_io_8080_t (8080 parallel bus)
 */
struct egui_panel_io {
    /**
     * Send command with parameter data (blocking).
     *
     * For SPI LCD: sets DC=0 for cmd byte, DC=1 for param bytes.
     * For I2C: writes param data to register address 'cmd'.
     * For 8080: sends command then data.
     *
     * @param io         Panel IO handle
     * @param cmd        Command or register address
     * @param param      Parameter data buffer (NULL if no params)
     * @param param_size Parameter data size in bytes (0 if no params)
     * @return 0 on success, non-zero on failure
     */
    int (*tx_param)(egui_panel_io_t *io, int cmd, const void *param, size_t param_size);

    /**
     * Read data from a register/command (blocking).
     *
     * For I2C: reads data from register address 'cmd'.
     * For SPI: sends cmd then reads response (if supported).
     * May be NULL if read is not supported.
     *
     * @param io         Panel IO handle
     * @param cmd        Command or register address
     * @param param      Buffer to receive data
     * @param param_size Number of bytes to read
     * @return 0 on success, non-zero on failure
     */
    int (*rx_param)(egui_panel_io_t *io, int cmd, void *param, size_t param_size);

    /**
     * Send pixel/color data (may be async with DMA).
     *
     * For SPI: sets DC=1, CS=0, starts transfer (may return before done).
     * For I2C: sends data to register address 'cmd' in chunks.
     * For 8080: writes data to bus (may return before done).
     *
     * After calling tx_color, caller must call wait_tx_done() before
     * reusing the data buffer (if wait_tx_done is non-NULL).
     *
     * @param io         Panel IO handle
     * @param cmd        Command (e.g., RAMWR for LCD). -1 if no command needed.
     * @param color      Pixel data buffer
     * @param color_size Data size in bytes
     * @return 0 on success, non-zero on failure
     */
    int (*tx_color)(egui_panel_io_t *io, int cmd, const void *color, size_t color_size);

    /**
     * Wait for async tx_color to complete.
     * NULL = tx_color is synchronous (no wait needed).
     *
     * @param io  Panel IO handle
     */
    void (*wait_tx_done)(egui_panel_io_t *io);

    /**
     * Initialize the IO interface.
     * Called once during driver init. May be NULL if already initialized.
     *
     * @param io  Panel IO handle
     */
    void (*init)(egui_panel_io_t *io);

    /**
     * Deinitialize the IO interface.
     * Called during driver deinit. May be NULL if not needed.
     *
     * @param io  Panel IO handle
     */
    void (*deinit)(egui_panel_io_t *io);
};

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PANEL_IO_H_ */
