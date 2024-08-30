#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>

#include "uicode.h"
#include "sdl_port.h"


void egui_api_log(const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);
}

void egui_api_assert(const char *file, int line)
{
#if EGUI_CONFIG_DEBUG_LOG_LEVEL >= EGUI_LOG_IMPL_LEVEL_DBG
    char s_buf[0x200];
    memset(s_buf, 0, sizeof(s_buf));
    sprintf(s_buf, "vvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\nAssert@ file = %s, line = %d\n\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", file, line);
    printf("%s", s_buf);
#endif

    while(1);
}

void egui_api_free(void *ptr)
{
    free(ptr);
}

void *egui_api_malloc(int size)
{
    return malloc(size);
}

void egui_api_sprintf(char *str, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    vsprintf(str, format, argptr);
    va_end(argptr);
}

void egui_api_draw_data(int16_t x, int16_t y, int16_t width, int16_t height, const egui_color_int_t *data)
{
    // printf("api_draw_data, x: %d, y: %d, width: %d, height: %d\n", x, y, width, height);

    VT_Fill_Multiple_Colors(x, y, x + width - 1, y + height - 1, (egui_color_int_t *)data);
}

void egui_api_refresh_display(void)
{
    // printf("api_refresh_display\n");

    VT_sdl_flush(1);
}

void egui_api_timer_start(uint32_t ms)
{
}

void egui_api_timer_stop(void)
{
}

uint32_t egui_api_timer_get_current(void)
{

    return sdl_get_system_timestamp_ms();
}

void egui_api_delay(uint32_t ms)
{
    sdl_port_sleep(ms);
}


void egui_api_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}

