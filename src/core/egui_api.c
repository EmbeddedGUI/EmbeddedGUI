#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "egui_api.h"
#include "egui_core.h"
#include "egui_display_driver.h"
#include "egui_platform.h"

/**
 * Bridge layer: implements egui_api_* functions.
 *
 * When the corresponding EGUI_CONFIG_PLATFORM_CUSTOM_xxx macro is 0 (default),
 * the function calls the standard C library directly — no callback, no
 * function-pointer dereference, shortest possible path.
 *
 * When the macro is 1, the function dispatches through the registered
 * platform ops callback so that the porting layer can provide a
 * hardware-accelerated or otherwise specialised implementation.
 */

static void *egui_api_platform_malloc(int size)
{
#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->malloc != NULL)
    {
        return plat->ops->malloc(size);
    }
    return NULL;
#else
    return malloc((size_t)size);
#endif
}

static void egui_api_platform_free(void *ptr)
{
#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->free != NULL)
    {
        plat->ops->free(ptr);
    }
#else
    free(ptr);
#endif
}

#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
typedef union egui_api_alloc_header
{
    size_t payload_size;
    uintptr_t align_uintptr;
    long double align_long_double;
} egui_api_alloc_header_t;

static size_t s_egui_api_mem_used_size;
static size_t s_egui_api_mem_peak_size;

static void *egui_api_malloc_raw(int size)
{
    return egui_api_platform_malloc(size);
}

static void egui_api_free_raw(void *ptr)
{
    egui_api_platform_free(ptr);
}
#endif

#if EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF
void egui_api_log(const char *format, ...)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->vlog != NULL)
    {
        va_list args;
        va_start(args, format);
        plat->ops->vlog(format, args);
        va_end(args);
    }
}
#else
void egui_api_log(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
#endif

void egui_api_assert(const char *file, int line)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->assert_handler != NULL)
    {
        plat->ops->assert_handler(file, line);
    }
    while (1)
        ;
}

void egui_api_free(void *ptr)
{
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_api_alloc_header_t *header;

    if (ptr == NULL)
    {
        return;
    }

    header = ((egui_api_alloc_header_t *)ptr) - 1;
    if (s_egui_api_mem_used_size >= header->payload_size)
    {
        s_egui_api_mem_used_size -= header->payload_size;
    }
    else
    {
        s_egui_api_mem_used_size = 0U;
    }
    egui_api_free_raw((void *)header);
#else
    egui_api_platform_free(ptr);
#endif
}

void *egui_api_malloc(int size)
{
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_api_alloc_header_t *header;
    size_t payload_size;
    size_t raw_size;
#endif

    if (size <= 0)
    {
        return NULL;
    }

#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    payload_size = (size_t)size;
    if (payload_size > (size_t)INT_MAX - sizeof(egui_api_alloc_header_t))
    {
        return NULL;
    }

    raw_size = sizeof(egui_api_alloc_header_t) + payload_size;
    header = (egui_api_alloc_header_t *)egui_api_malloc_raw((int)raw_size);
    if (header == NULL)
    {
        return NULL;
    }

    header->payload_size = payload_size;
    s_egui_api_mem_used_size += payload_size;
    if (s_egui_api_mem_used_size > s_egui_api_mem_peak_size)
    {
        s_egui_api_mem_peak_size = s_egui_api_mem_used_size;
    }
    return (void *)(header + 1);
#else
    return egui_api_platform_malloc(size);
#endif
}

int egui_api_get_mem_monitor(egui_mem_monitor_t *monitor)
{
    if (monitor == NULL)
    {
        return 0;
    }

    memset(monitor, 0, sizeof(*monitor));

#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    monitor->used_size = s_egui_api_mem_used_size;
    monitor->max_used = s_egui_api_mem_peak_size;
#endif

    return 1;
}

#if EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF
void egui_api_sprintf(char *str, const char *format, ...)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->vsprintf != NULL)
    {
        va_list args;
        va_start(args, format);
        plat->ops->vsprintf(str, format, args);
        va_end(args);
    }
}
#else
void egui_api_sprintf(char *str, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsprintf(str, format, args);
    va_end(args);
}
#endif

void egui_api_draw_data(int16_t x, int16_t y, int16_t width, int16_t height, const egui_color_int_t *data)
{
    egui_display_driver_t *drv = egui_display_driver_get();
    if (drv != NULL && drv->ops->draw_area != NULL)
    {
        drv->ops->draw_area(x, y, width, height, data);
        // wait_draw_complete == NULL means draw_area is synchronous, no wait needed
        if (drv->ops->wait_draw_complete != NULL)
        {
            drv->ops->wait_draw_complete();
        }
    }
}

void egui_api_refresh_display(void)
{
    egui_display_driver_t *drv = egui_display_driver_get();
    if (drv != NULL && drv->ops->flush != NULL)
    {
        drv->ops->flush();
    }
}

void egui_api_timer_start(uint32_t ms)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->timer_start != NULL)
    {
        plat->ops->timer_start(ms);
    }
}

void egui_api_timer_stop(void)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->timer_stop != NULL)
    {
        plat->ops->timer_stop();
    }
}

uint32_t egui_api_timer_get_current(void)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->get_tick_ms != NULL)
    {
        return plat->ops->get_tick_ms();
    }
    return 0;
}

void egui_api_delay(uint32_t ms)
{
    egui_platform_t *plat = egui_platform_get();
    if (ms == 0)
    {
        return;
    }
    if (plat != NULL && plat->ops->delay != NULL)
    {
        plat->ops->delay(ms);
    }
}

#if EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP
void egui_api_pfb_clear(void *s, int n)
{
    egui_api_memset(s, 0, n);
}

void egui_api_memset(void *s, int c, int n)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->memset_fast != NULL)
    {
        plat->ops->memset_fast(s, c, n);
    }
    else
    {
        memset(s, c, n);
    }
}

void egui_api_memcpy(void *dst, const void *src, int n)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->memcpy_fast != NULL)
    {
        plat->ops->memcpy_fast(dst, src, n);
    }
    else
    {
        memcpy(dst, src, n);
    }
}
#else
void egui_api_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}

void egui_api_memset(void *s, int c, int n)
{
    memset(s, c, n);
}

void egui_api_memcpy(void *dst, const void *src, int n)
{
    memcpy(dst, src, n);
}
#endif

void egui_api_load_external_resource(void *dest, const uint32_t res_id, uint32_t start_offset, uint32_t size)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->load_external_resource != NULL)
    {
        // Acquire SPI bus: wait for pending DMA, prevent new DMA starts.
        // Handles both interrupt and polling modes transparently.
        egui_pfb_bus_acquire();
        plat->ops->load_external_resource(dest, res_id, start_offset, size);
        egui_pfb_bus_release();
    }
}

egui_base_t egui_hw_interrupt_disable(void)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->interrupt_disable != NULL)
    {
        return plat->ops->interrupt_disable();
    }
    return 0;
}

void egui_hw_interrupt_enable(egui_base_t level)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->interrupt_enable != NULL)
    {
        plat->ops->interrupt_enable(level);
    }
}
