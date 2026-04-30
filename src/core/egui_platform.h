#ifndef _EGUI_PLATFORM_H_
#define _EGUI_PLATFORM_H_

/**
 * @file egui_platform.h
 * @brief Platform-service callbacks that supply time, locking, memory, and OS glue.
 */

#include <stdarg.h>
#include <stdint.h>

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Platform operations (vtable).
 * This layer provides OS-like services and utility hooks, while display and touch hardware live in their own dedicated driver interfaces.
 *
 * Fields guarded by EGUI_CONFIG_PLATFORM_CUSTOM_xxx macros only exist when the corresponding macro is set to 1.  When the macro is 0, the framework calls the
 * standard C library directly and the porting layer does NOT need to register any callback for that feature.
 */
struct egui_platform_ops
{
#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
    /** Custom memory allocator, for example a DMA-safe pool or instrumented heap. */
    void *(*malloc)(int size);
    /** Release memory previously returned by the custom `malloc` hook. */
    void (*free)(void *ptr);
#endif

#if EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF
    /** Logging hook in `va_list` form, for example semihosting or UART output. */
    void (*vlog)(const char *format, va_list args);
#endif

    /** Assertion handler used by the framework's internal assert macros. */
    void (*assert_handler)(const char *file, int line);

    /** Busy-wait or sleep for the requested number of milliseconds. */
    void (*delay)(uint32_t ms);

    /** Return a monotonic tick count in milliseconds. */
    uint32_t (*get_tick_ms)(void);

#if EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP
    /** Fast memory set hook. Leave `NULL` to fall back to the C library. */
    void (*memset_fast)(void *s, int c, int n);
    /** Fast memory copy hook. Leave `NULL` to fall back to the C library. */
    void (*memcpy_fast)(void *dst, const void *src, int n);
#endif

    /** Disable interrupts and return a token that later restores the previous state. */
    egui_base_t (*interrupt_disable)(void);
    /** Restore the interrupt state token returned by `interrupt_disable`. */
    void (*interrupt_enable)(egui_base_t level);

    /** Load part of one external resource into RAM for a specific core. Optional. */
    void (*load_external_resource)(egui_core_t *core, void *dest, egui_uintptr_t res_id, uint32_t start_offset, uint32_t size);

    /* ---- Timer callback ---- */

    /**
     * Schedule a platform timer wakeup for a specific core at an absolute millisecond deadline.
     * When the deadline arrives, the platform should call egui_timer_polling_work(core).
     * Leave this `NULL` when the port drives timers by polling instead.
     */
    void (*timer_start)(egui_core_t *core, uint32_t expiry_time_ms);

    /** Cancel the scheduled timer wakeup for a specific core. Optional when `timer_start` is unused. */
    void (*timer_stop)(egui_core_t *core);
};

/** One process-global platform-service instance. */
struct egui_platform
{
    const egui_platform_ops_t *ops; // callback table implemented by the port or OS adapter
};

/** Bind the process-global platform callback table during startup. Re-registering the same instance is allowed. */
void egui_platform_register(egui_platform_t *platform);
/** Return the process-global platform currently registered by the active port. */
egui_platform_t *egui_platform_get(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PLATFORM_H_ */
