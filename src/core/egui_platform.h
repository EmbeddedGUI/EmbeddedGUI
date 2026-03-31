#ifndef _EGUI_PLATFORM_H_
#define _EGUI_PLATFORM_H_

#include <stdarg.h>
#include <stdint.h>

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Platform operations (vtable).
 * Separates OS/platform services from hardware drivers.
 *
 * Fields guarded by EGUI_CONFIG_PLATFORM_CUSTOM_xxx macros only exist when
 * the corresponding macro is set to 1.  When the macro is 0, the framework
 * calls the standard C library directly and the porting layer does NOT need
 * to register any callback for that feature.
 */
struct egui_platform_ops
{
#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
    /** Custom memory allocator (e.g. DMA-safe pool, instrumented heap). */
    void *(*malloc)(int size);
    void (*free)(void *ptr);
#endif

#if EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF
    /** Logging (va_list version, e.g. semihosting, UART). */
    void (*vlog)(const char *format, va_list args);
    /** String formatting (va_list version, e.g. minimal printf). */
    void (*vsprintf)(char *str, const char *format, va_list args);
#endif

    /** Assert handler */
    void (*assert_handler)(const char *file, int line);

    /** Delay in milliseconds */
    void (*delay)(uint32_t ms);

    /** Get current time in milliseconds (monotonic) */
    uint32_t (*get_tick_ms)(void);

#if EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP
    /** Fast memory set (can be DMA-accelerated). NULL = use memset. */
    void (*memset_fast)(void *s, int c, int n);
    /** Fast memory copy (can be DMA-accelerated). NULL = use memcpy. */
    void (*memcpy_fast)(void *dst, const void *src, int n);
#endif

    /** Interrupt disable/enable */
    egui_base_t (*interrupt_disable)(void);
    void (*interrupt_enable)(egui_base_t level);

    /** Load external resource (optional, can be NULL) */
    void (*load_external_resource)(void *dest, uint32_t res_id, uint32_t start_offset, uint32_t size);

    /* ---- RTOS mutex (all optional, NULL = no mutex support) ---- */

    /** Create a mutex. Returns opaque handle. NULL = not supported. */
    void *(*mutex_create)(void);

    /** Lock a mutex (blocking). */
    void (*mutex_lock)(void *mutex);

    /** Unlock a mutex. */
    void (*mutex_unlock)(void *mutex);

    /** Destroy a mutex. */
    void (*mutex_destroy)(void *mutex);

    /* ---- Timer callback ---- */

    /**
     * Schedule a platform timer wakeup at absolute time (ms).
     * When the time arrives, platform should call egui_timer_polling_work().
     * NULL = timer wakeup not supported (core uses polling).
     */
    void (*timer_start)(uint32_t expiry_time_ms);

    /** Cancel the scheduled timer wakeup. NULL = not supported. */
    void (*timer_stop)(void);

    /* ---- Misc ---- */

    /** Feed the watchdog during long operations. NULL = no watchdog. */
    void (*watchdog_feed)(void);
};

/**
 * Platform driver instance.
 */
struct egui_platform
{
    const egui_platform_ops_t *ops;
};

extern egui_platform_t *g_egui_platform;

__EGUI_STATIC_INLINE__ void egui_platform_register(egui_platform_t *platform)
{
    g_egui_platform = platform;
}

__EGUI_STATIC_INLINE__ egui_platform_t *egui_platform_get(void)
{
    return g_egui_platform;
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PLATFORM_H_ */
