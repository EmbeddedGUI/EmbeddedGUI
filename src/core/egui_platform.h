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
 */
typedef struct egui_platform_ops egui_platform_ops_t;
struct egui_platform_ops
{
    /** Memory allocation (can be NULL if malloc not available) */
    void *(*malloc)(int size);
    void (*free)(void *ptr);

    /** Logging (va_list version) */
    void (*vlog)(const char *format, va_list args);

    /** Assert handler */
    void (*assert_handler)(const char *file, int line);

    /** String formatting (va_list version) */
    void (*vsprintf)(char *str, const char *format, va_list args);

    /** Delay in milliseconds */
    void (*delay)(uint32_t ms);

    /** Get current time in milliseconds (monotonic) */
    uint32_t (*get_tick_ms)(void);

    /** Fast memory clear for PFB (can be DMA-accelerated) */
    void (*pfb_clear)(void *s, int n);

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

    /** Fast memory copy (can be DMA-accelerated). NULL = use memcpy. */
    void (*memcpy_fast)(void *dst, const void *src, int n);

    /** Feed the watchdog during long operations. NULL = no watchdog. */
    void (*watchdog_feed)(void);
};

/**
 * Platform driver instance.
 */
typedef struct egui_platform egui_platform_t;
struct egui_platform
{
    const egui_platform_ops_t *ops;
};

/** Register platform driver. Must be called before egui_init(). */
void egui_platform_register(egui_platform_t *platform);

/** Get registered platform. Returns NULL if not registered. */
egui_platform_t *egui_platform_get(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PLATFORM_H_ */
