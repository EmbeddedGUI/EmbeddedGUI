/**
 * @file egui_panel_io_8080.h
 * @brief 8080 Parallel Bus Panel IO implementation
 */

#ifndef _EGUI_PANEL_IO_8080_H_
#define _EGUI_PANEL_IO_8080_H_

#include "egui_panel_io.h"
#include "egui_bus_8080.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 8080 Panel IO instance
 */
typedef struct egui_panel_io_8080
{
    egui_panel_io_t base;                /**< Base interface (MUST be first member) */
    const egui_bus_8080_ops_t *bus_8080; /**< 8080 bus operations */
} egui_panel_io_8080_t;

/**
 * Initialize 8080 Panel IO.
 *
 * @param io        User-provided storage
 * @param bus_8080  8080 bus operations (must not be NULL)
 */
void egui_panel_io_8080_init(egui_panel_io_8080_t *io, const egui_bus_8080_ops_t *bus_8080);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PANEL_IO_8080_H_ */
