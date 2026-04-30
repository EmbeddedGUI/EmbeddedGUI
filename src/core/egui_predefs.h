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

/*
 * TC32 GCC 4.5.1-tc32-1.3 does not define __tc32__ in this toolchain, even
 * though its bundled system headers test for it.  Detect the target from the
 * compiler-provided tc32 specs macro plus the architecture macros it emits.
 * EGUI_FORCE_TARGET_TC32 is reserved for host-side checks that intentionally
 * compile the TC32-only paths with another compiler.
 */
#ifndef EGUI_TARGET_TC32
#if defined(__tc32__) || defined(EGUI_FORCE_TARGET_TC32) ||                                                                                                    \
        (defined(__USES_INITFINI__) && defined(__arm__) && defined(__thumb__) && defined(__ARM_ARCH_3M__) && defined(__GNUC__) && defined(__GNUC_MINOR__) &&   \
         (__GNUC__ == 4) && (__GNUC_MINOR__ == 5))
#define EGUI_TARGET_TC32 1
#else
#define EGUI_TARGET_TC32 0
#endif
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PREDEFS_H_ */
