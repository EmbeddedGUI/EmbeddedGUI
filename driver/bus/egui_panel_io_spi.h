/**
 * @file egui_panel_io_spi.h
 * @brief SPI Panel IO implementation
 *
 * Wraps egui_bus_spi_ops_t and DC/CS GPIO into a unified egui_panel_io_t.
 * Handles DC pin toggling for command vs data mode internally.
 */

#ifndef _EGUI_PANEL_IO_SPI_H_
#define _EGUI_PANEL_IO_SPI_H_

#include "egui_panel_io.h"
#include "egui_bus_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SPI Panel IO instance
 */
typedef struct egui_panel_io_spi {
    egui_panel_io_t base;              /**< Base interface (MUST be first member) */
    const egui_bus_spi_ops_t *spi;     /**< SPI bus operations */
    void (*set_dc)(uint8_t level);     /**< DC pin control. NULL if not needed */
    void (*set_cs)(uint8_t level);     /**< CS pin control. NULL if hardware-controlled */
} egui_panel_io_spi_t;

/**
 * Initialize SPI Panel IO.
 *
 * @param io      User-provided storage
 * @param spi     SPI bus operations (must not be NULL, spi->write must not be NULL)
 * @param set_dc  DC pin control function (NULL if not needed)
 * @param set_cs  CS pin control function (NULL if hardware-controlled)
 */
void egui_panel_io_spi_init(egui_panel_io_spi_t *io,
                             const egui_bus_spi_ops_t *spi,
                             void (*set_dc)(uint8_t level),
                             void (*set_cs)(uint8_t level));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PANEL_IO_SPI_H_ */
