#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "egui_api.h"

__EGUI_WEAK__ void egui_api_log(const char *format, ...)
{
}

__EGUI_WEAK__ void egui_api_assert(const char *file, int line)
{
    while(1);
}

__EGUI_WEAK__ void egui_api_free(void *ptr)
{
}

__EGUI_WEAK__ void *egui_api_malloc(int size)
{
    return NULL;
}

__EGUI_WEAK__ void egui_api_sprintf(char *str, const char *format, ...)
{
}

__EGUI_WEAK__ void egui_api_draw_data(int16_t x, int16_t y, int16_t width, int16_t height, const egui_color_int_t *data)
{
}

__EGUI_WEAK__ void egui_api_refresh_display(void)
{
}

__EGUI_WEAK__ void egui_api_timer_start(uint32_t ms)
{
}

__EGUI_WEAK__ void egui_api_timer_stop(void)
{
}

__EGUI_WEAK__ uint32_t egui_api_timer_get_current(void)
{

    return 0;
}

__EGUI_WEAK__ void egui_api_delay(uint32_t ms)
{
}


__EGUI_WEAK__ void egui_api_pfb_clear(void *s, int n)
{
}


__EGUI_WEAK__ void egui_api_load_external_resource(void *dest, const uint32_t res_id, uint32_t start_offset, uint32_t size)
{

}

__EGUI_WEAK__ egui_base_t egui_hw_interrupt_disable(void)
{
    return 0;
}

__EGUI_WEAK__ void egui_hw_interrupt_enable(egui_base_t level)
{
}

