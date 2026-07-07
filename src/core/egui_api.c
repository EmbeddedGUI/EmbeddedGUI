#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>

#include "egui_api.h"
#include "egui_core.h"
#include "canvas/egui_canvas.h"
#include "egui_display_driver.h"
#include "egui_platform.h"

/**
 * @file egui_api.c
 * @brief Bridge layer that routes generic runtime services to either libc or the registered platform callbacks.
 */

/**
 * Bridge layer: implements egui_api_* functions.
 *
 * When the corresponding EGUI_CONFIG_PLATFORM_CUSTOM_xxx macro is 0 (default),
 * the function calls the standard C library directly no callback, no
 * function-pointer dereference, shortest possible path.
 *
 * When the macro is 1, the function dispatches through the registered
 * platform ops callback so that the porting layer can provide a
 * hardware-accelerated or otherwise specialised implementation.
 */

/** Allocate memory through the active platform hook when enabled, otherwise fall back to `malloc`. */
static void *egui_api_platform_malloc(egui_core_t *core, int size)
{
    EGUI_UNUSED(core);

#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops != NULL && plat->ops->malloc != NULL)
    {
        return plat->ops->malloc(size);
    }

#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC_LIBC_FALLBACK
    return malloc((size_t)size);
#else
    return NULL;
#endif
#else
    return malloc((size_t)size);
#endif
}

/** Free memory through the active platform hook when enabled, otherwise fall back to `free`. */
static void egui_api_platform_free(egui_core_t *core, void *ptr)
{
    EGUI_UNUSED(core);

#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops != NULL && plat->ops->free != NULL)
    {
        plat->ops->free(ptr);
        return;
    }

#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC_LIBC_FALLBACK
    free(ptr);
#else
    EGUI_UNUSED(ptr);
#endif
#else
    free(ptr);
#endif
}

#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC || EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
typedef union egui_api_alloc_header
{
    struct
    {
        egui_core_t *alloc_core;
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
        size_t tracked_size;
        egui_core_t *monitor_core;
#endif
    } meta;
    uintptr_t align_uintptr;
    long double align_long_double;
} egui_api_alloc_header_t;

#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
/** Read a tracked memory counter used by the debug memory monitor. */
static size_t egui_api_mem_counter_load(const size_t *value)
{
    return *value;
}

/** Increase a tracked memory counter and return the updated value. */
static size_t egui_api_mem_counter_add(size_t *value, size_t delta)
{
    *value += delta;
    return *value;
}

/** Decrease a tracked memory counter without allowing it to underflow below zero. */
static size_t egui_api_mem_counter_sub_clamp_zero(size_t *value, size_t delta)
{
    *value = (*value >= delta) ? (*value - delta) : 0U;
    return *value;
}

/** Update the peak memory watermark when the latest usage exceeds it. */
static void egui_api_mem_counter_update_peak(size_t *peak_value, size_t current_used)
{
    if (current_used > *peak_value)
    {
        *peak_value = current_used;
    }
}

/** Attribute one allocation to the owning core's memory monitor. */
static void egui_api_mem_counter_add_to_core(egui_core_t *core, size_t tracked_size)
{
    size_t current_used;

    if (core == NULL)
    {
        return;
    }

    current_used = egui_api_mem_counter_add(&core->debug.mem_monitor_used_size, tracked_size);
    egui_api_mem_counter_update_peak(&core->debug.mem_monitor_peak_size, current_used);
}

/** Remove one allocation from the owning core's memory monitor. */
static void egui_api_mem_counter_sub_from_core(egui_core_t *core, size_t tracked_size)
{
    if (core == NULL)
    {
        return;
    }

    egui_api_mem_counter_sub_clamp_zero(&core->debug.mem_monitor_used_size, tracked_size);
}
#endif

/** Validate and compute the raw allocation size once the tracking header is included. */
static int egui_api_calc_alloc_size(size_t payload_size, size_t *alloc_size)
{
    if (alloc_size == NULL)
    {
        return 0;
    }

    if (payload_size > (size_t)INT_MAX - sizeof(egui_api_alloc_header_t))
    {
        return 0;
    }

    *alloc_size = sizeof(egui_api_alloc_header_t) + payload_size;
    return 1;
}

/** Raw allocator entry point used after the wrapped size has already been computed. */
static void *egui_api_malloc_raw(egui_core_t *core, int size)
{
    return egui_api_platform_malloc(core, size);
}

/** Raw free entry point paired with `egui_api_malloc_raw()`. */
static void egui_api_free_raw(egui_core_t *core, void *ptr)
{
    egui_api_platform_free(core, ptr);
}
#endif

/** Emit a consistent allocation-failure log including both payload and tracked sizes. */
static void egui_api_log_alloc_fail(const char *reason, size_t payload_size, size_t tracked_size)
{
    EGUI_UNUSED(payload_size);
    EGUI_UNUSED(reason);
    EGUI_UNUSED(tracked_size);
    EGUI_LOG_ERR("egui malloc %s: payload=%lu, tracked=%lu\r\n", reason, (unsigned long)payload_size, (unsigned long)tracked_size);
}

#if (EGUI_LOG_LEVEL > EGUI_LOG_IMPL_LEVEL_NONE) || EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE || EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS ||                             \
        EGUI_CONFIG_DEBUG_DIRTY_REGION_DETAIL
#if EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF
void egui_api_log(const char *format, ...)
{
    va_list args;
    egui_platform_t *plat = egui_platform_get();

    va_start(args, format);

    if (plat != NULL && plat->ops != NULL && plat->ops->vlog != NULL)
    {
        plat->ops->vlog(format, args);
    }

    va_end(args);
}
#else
void egui_api_log(const char *format, ...)
{
    va_list args;

    va_start(args, format);

    vprintf(format, args);
    fflush(stdout);
    va_end(args);
}
#endif // EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF
#else
void egui_api_log(const char *format, ...)
{
    EGUI_UNUSED(format);
}
#endif

/** Trap forever after an assertion failure so embedded targets stop at the failing point. */
void egui_api_assert(const char *file, int line)
{
    EGUI_UNUSED(file);
    EGUI_UNUSED(line);
    while (1)
        ;
}

/** Free one block allocated by `egui_api_malloc()`, removing tracking metadata when present. */
void egui_api_free(egui_core_t *core, void *ptr)
{
#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC || EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_api_alloc_header_t *header;

    if (ptr == NULL)
    {
        return;
    }

    header = ((egui_api_alloc_header_t *)ptr) - 1;
    EGUI_ASSERT(core == NULL || header->meta.alloc_core == NULL || header->meta.alloc_core == core);
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    {
        size_t tracked_size = header->meta.tracked_size;

        egui_api_mem_counter_sub_from_core(header->meta.monitor_core, tracked_size);
    }
#endif
    egui_api_free_raw(header->meta.alloc_core, (void *)header);
#else
    egui_api_platform_free(core, ptr);
#endif
}

/** Allocate one block of memory, optionally prepending tracking metadata for debug and custom free paths. */
void *egui_api_malloc(egui_core_t *core, int size)
{
    if (size <= 0)
    {
        return NULL;
    }

#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC || EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_api_alloc_header_t *header;
    size_t payload_size;
    size_t alloc_size;
#endif

#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC || EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    payload_size = (size_t)size;
    if (!egui_api_calc_alloc_size(payload_size, &alloc_size))
    {
        egui_api_log_alloc_fail("size_overflow", payload_size, 0U);
        return NULL;
    }

    header = (egui_api_alloc_header_t *)egui_api_malloc_raw(core, (int)alloc_size);
    if (header == NULL)
    {
        egui_api_log_alloc_fail("failed", payload_size, alloc_size);
        return NULL;
    }

    header->meta.alloc_core = core;
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    header->meta.tracked_size = alloc_size;
    header->meta.monitor_core = core;
    egui_api_mem_counter_add_to_core(core, alloc_size);
#endif
    return (void *)(header + 1);
#else
    void *ptr;

    ptr = egui_api_platform_malloc(core, size);
    if (ptr == NULL)
    {
        egui_api_log_alloc_fail("failed", (size_t)size, (size_t)size);
    }
    return ptr;
#endif
}

/** Snapshot the current memory monitor values for the given core. */
int egui_api_get_mem_monitor(egui_core_t *core, egui_mem_monitor_t *monitor)
{
    EGUI_UNUSED(core);
    if (monitor == NULL)
    {
        return 0;
    }

    memset(monitor, 0, sizeof(*monitor));

#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    if (core == NULL)
    {
        return 0;
    }

    monitor->used_size = egui_api_mem_counter_load(&core->debug.mem_monitor_used_size);
    monitor->max_used = egui_api_mem_counter_load(&core->debug.mem_monitor_peak_size);
#endif

    return 1;
}

void egui_api_draw_data(egui_core_t *core, int16_t x, int16_t y, int16_t width, int16_t height, const egui_color_int_t *data)
{
    egui_display_driver_t *drv = egui_display_driver_get(core);

    if (drv != NULL && drv->ops->draw_area != NULL)
    {
        drv->ops->draw_area(core, x, y, width, height, data);
        // wait_draw_complete == NULL means draw_area is synchronous, no wait needed
        if (drv->ops->wait_draw_complete != NULL)
        {
            drv->ops->wait_draw_complete(core);
        }
    }
}

/** Ask the display driver to flush any deferred work it manages internally. */
void egui_api_refresh_display(egui_core_t *core)
{
    egui_display_driver_t *drv = egui_display_driver_get(core);

    if (drv != NULL && drv->ops->flush != NULL)
    {
        drv->ops->flush(core);
    }
}

/** Start or reprogram the process-global platform timer for the target core. */
void egui_api_timer_start(egui_core_t *core, uint32_t ms)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops != NULL && plat->ops->timer_start != NULL)
    {
        plat->ops->timer_start(core, ms);
    }
}

/** Stop the process-global platform timer for the target core. */
void egui_api_timer_stop(egui_core_t *core)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops != NULL && plat->ops->timer_stop != NULL)
    {
        plat->ops->timer_stop(core);
    }
}

/** Read the current platform tick counter through the process-global platform. */
uint32_t egui_api_timer_get_current_core(egui_core_t *core)
{
    EGUI_UNUSED(core);

    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops != NULL && plat->ops->get_tick_ms != NULL)
    {
        return plat->ops->get_tick_ms();
    }
    return 0;
}

/** Read the current global platform tick counter without an explicit core context. */
uint32_t egui_api_timer_get_current(void)
{
    return egui_api_timer_get_current_core(NULL);
}

/** Delay for `ms` milliseconds through the process-global platform hook when available. */
void egui_api_delay_core(egui_core_t *core, uint32_t ms)
{
    EGUI_UNUSED(core);

    egui_platform_t *plat = egui_platform_get();
    if (ms == 0)
    {
        return;
    }
    if (plat != NULL && plat->ops != NULL && plat->ops->delay != NULL)
    {
        plat->ops->delay(ms);
    }
}

/** Delay for `ms` milliseconds without an explicit core context. */
void egui_api_delay(uint32_t ms)
{
    egui_api_delay_core(NULL, ms);
}

#if EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP
/** Clear one PFB-sized memory block using the configured memory backend. */
void egui_api_pfb_clear(void *s, int n)
{
    egui_api_memset(s, 0, n);
}

/** Set one memory range using the configured memory backend. */
void egui_api_memset(void *s, int c, int n)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops != NULL && plat->ops->memset_fast != NULL)
    {
        plat->ops->memset_fast(s, c, n);
        return;
    }

    memset(s, c, n);
}

/** Copy one memory range using the configured memory backend. */
void egui_api_memcpy(void *dst, const void *src, int n)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops != NULL && plat->ops->memcpy_fast != NULL)
    {
        plat->ops->memcpy_fast(dst, src, n);
        return;
    }

    memcpy(dst, src, n);
}
#else
/** Clear one PFB-sized memory block using the default libc backend. */
void egui_api_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}

/** Set one memory range using the default libc backend. */
void egui_api_memset(void *s, int c, int n)
{
    memset(s, c, n);
}

/** Copy one memory range using the default libc backend. */
void egui_api_memcpy(void *dst, const void *src, int n)
{
    memcpy(dst, src, n);
}
#endif

/**
 * Load one external resource block through the platform hook.
 * If external resources share the display bus, lock the PFB bus around the read.
 * Boards with independent resource/display buses can disable that lock in config.
 */
void egui_api_load_external_resource_core(egui_core_t *core, void *dest, egui_uintptr_t res_id, uint32_t start_offset, uint32_t size)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops != NULL && plat->ops->load_external_resource != NULL)
    {
#if EGUI_CONFIG_EXTERNAL_RESOURCE_SHARED_DISPLAY_BUS
        if (core != NULL)
        {
            // Acquire SPI bus: wait for pending DMA, prevent new DMA starts.
            // Handles both interrupt and polling modes transparently.
            egui_pfb_bus_acquire(core);
        }
#endif
        plat->ops->load_external_resource(core, dest, res_id, start_offset, size);
#if EGUI_CONFIG_EXTERNAL_RESOURCE_SHARED_DISPLAY_BUS
        if (core != NULL)
        {
            egui_pfb_bus_release(core);
        }
#endif
    }
}

void egui_api_load_external_resource(const egui_canvas_t *canvas, void *dest, egui_uintptr_t res_id, uint32_t start_offset, uint32_t size)
{
    egui_api_load_external_resource_core(canvas != NULL ? egui_canvas_get_core(canvas) : NULL, dest, res_id, start_offset, size);
}

/** Disable interrupts without an explicit core context. */
egui_base_t egui_hw_interrupt_disable(void)
{
    egui_platform_t *plat = egui_platform_get();

    if (plat != NULL && plat->ops != NULL && plat->ops->interrupt_disable != NULL)
    {
        return plat->ops->interrupt_disable();
    }

    return 0;
}

/** Restore interrupts without an explicit core context. */
void egui_hw_interrupt_enable(egui_base_t level)
{
    egui_platform_t *plat = egui_platform_get();

    if (plat != NULL && plat->ops != NULL && plat->ops->interrupt_enable != NULL)
    {
        plat->ops->interrupt_enable(level);
    }
}
