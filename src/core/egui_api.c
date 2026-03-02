#include <string.h>
#include <stdarg.h>

#include "egui_api.h"
#include "egui_core.h"
#include "egui_display_driver.h"
#include "egui_platform.h"

/**
 * Bridge layer: implements egui_api_* functions by dispatching through
 * registered display driver and platform driver.
 *
 * This replaces both the old weak defaults and per-port direct implementations
 * (api_pc.c, api_mcu.c). Ports now register drivers instead of implementing
 * these functions directly.
 */

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
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->free != NULL)
    {
        plat->ops->free(ptr);
    }
}

void *egui_api_malloc(int size)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->malloc != NULL)
    {
        return plat->ops->malloc(size);
    }
    return NULL;
}

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

void egui_api_draw_data(int16_t x, int16_t y, int16_t width, int16_t height, const egui_color_int_t *data)
{
    egui_display_driver_t *drv = egui_display_driver_get();
    if (drv != NULL && drv->ops->draw_area != NULL)
    {
        drv->ops->draw_area(x, y, width, height, data);
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
    if (plat != NULL && plat->ops->delay != NULL)
    {
        plat->ops->delay(ms);
    }
}

void egui_api_pfb_clear(void *s, int n)
{
    egui_platform_t *plat = egui_platform_get();
    if (plat != NULL && plat->ops->pfb_clear != NULL)
    {
        plat->ops->pfb_clear(s, n);
    }
    else
    {
        memset(s, 0, n);
    }
}

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
