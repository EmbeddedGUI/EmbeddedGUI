/**
 * @file egui_bus_spi.h
 * @brief SPI Bus IO interface
 *
 * Users implement this interface for their platform's SPI peripheral.
 *
 * Return value convention: 0 = success, non-zero = failure
 *
 * Sync/Async mode:
 * - If wait_complete is NULL, write() must block until transfer completes (sync mode)
 * - If wait_complete is non-NULL, write() may return immediately (async/DMA mode)
 *   and wait_complete() must be called before reusing the data buffer
 *
 * DMA note: In async mode, the data buffer must remain valid until wait_complete() returns.
 */

#ifndef _EGUI_BUS_SPI_H_
#define _EGUI_BUS_SPI_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SPI Bus operations interface
 */
typedef struct egui_bus_spi_ops {
    /**
     * Initialize SPI peripheral.
     * Called once during driver init. May be NULL if already initialized.
     */
    void (*init)(void);

    /**
     * Deinitialize SPI peripheral.
     * Called during driver deinit. May be NULL if not needed.
     */
    void (*deinit)(void);

    /**
     * Write data to SPI bus.
     * @param data  Data buffer to transmit
     * @param len   Number of bytes to transmit
     * @return 0 on success, non-zero on failure
     */
    int (*write)(const uint8_t *data, uint32_t len);

    /**
     * Wait for async write to complete.
     * NULL = sync mode (write blocks until complete)
     * Non-NULL = async mode (call this before reusing buffer)
     */
    void (*wait_complete)(void);

    /**
     * Read data from SPI bus.
     * @param data  Buffer to receive data
     * @param len   Number of bytes to read
     * @return 0 on success, non-zero on failure
     * May be NULL if read not needed (most LCD drivers are write-only).
     */
    int (*read)(uint8_t *data, uint32_t len);
} egui_bus_spi_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BUS_SPI_H_ */
