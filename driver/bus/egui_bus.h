/**
 * @file egui_bus.h
 * @brief Common Bus IO definitions
 *
 * This file defines the bus type enumeration used by LCD and Touch drivers
 * to identify which bus interface they use.
 */

#ifndef _EGUI_BUS_H_
#define _EGUI_BUS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Bus type enumeration
 */
typedef enum egui_bus_type
{
    EGUI_BUS_TYPE_SPI,
    EGUI_BUS_TYPE_I2C,
    EGUI_BUS_TYPE_8080,
} egui_bus_type_t;

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_BUS_H_ */
