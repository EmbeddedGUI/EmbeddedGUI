/**
 * @file egui_bus_8080.h
 * @brief 8080 Parallel Bus IO interface
 *
 * Users implement this interface for their platform's 8080 parallel interface.
 * This is commonly used for larger LCD panels (ILI9488, NT35510, etc.)
 *
 * Return value convention: 0 = success, non-zero = failure
 *
 * Sync/Async mode:
 * - If wait_complete is NULL, write_data() must block until transfer completes
 * - If wait_complete is non-NULL, write_data() may return immediately
 */

#ifndef _EGUI_BUS_8080_H_
#define _EGUI_BUS_8080_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 8080 Parallel Bus operations interface
 */
typedef struct egui_bus_8080_ops
{
    /**
     * Initialize 8080 interface.
     * Called once during driver init. May be NULL if already initialized.
     */
    void (*init)(void);

    /**
     * Deinitialize 8080 interface.
     * Called during driver deinit. May be NULL if not needed.
     */
    void (*deinit)(void);

    /**
     * Write command to LCD controller.
     * @param cmd  Command value (8 or 16 bit depending on bus width)
     * @return 0 on success, non-zero on failure
     */
    int (*write_cmd)(uint16_t cmd);

    /**
     * Write data to LCD controller.
     * @param data  Data buffer to transmit
     * @param len   Number of bytes to transmit
     * @return 0 on success, non-zero on failure
     */
    int (*write_data)(const uint8_t *data, uint32_t len);

    /**
     * Wait for async write_data to complete.
     * NULL = sync mode (write_data blocks until complete)
     * Non-NULL = async mode (call this before reusing buffer)
     */
    void (*wait_complete)(void);

    /**
     * Read data from LCD controller.
     * @param data  Buffer to receive data
     * @param len   Number of bytes to read
     * @return 0 on success, non-zero on failure
     * May be NULL if read not needed.
     */
    int (*read_data)(uint8_t *data, uint32_t len);
} egui_bus_8080_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BUS_8080_H_ */
