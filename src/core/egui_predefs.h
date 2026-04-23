#ifndef _EGUI_PREDEFS_H_
#define _EGUI_PREDEFS_H_

/**
 * @file egui_predefs.h
 * @brief Small dependency-free definitions shared by low-level core headers.
 *
 * Keep this header lightweight so it can be included early without pulling in configuration or type headers and accidentally creating include cycles.
 */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/** Build-time identifier for the desktop/PC simulation port family. */
#define EGUI_PORT_TYPE_PC   1
/** Build-time identifier for embedded MCU ports that talk to real hardware. */
#define EGUI_PORT_TYPE_MCU  2
/** Build-time identifier for the QEMU-based validation port family. */
#define EGUI_PORT_TYPE_QEMU 3

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PREDEFS_H_ */
