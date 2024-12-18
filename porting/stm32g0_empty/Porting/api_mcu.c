#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>

#include "uicode.h"

#include "port_main.h"


void egui_api_log(const char *format, ...)
{
}

void egui_api_assert(const char *file, int line)
{
    while(1);
}

void egui_api_free(void *ptr)
{
}

void *egui_api_malloc(int size)
{
    return 0;
}

void egui_api_sprintf(char *str, const char *format, ...)
{
}

void egui_api_draw_data(int16_t x, int16_t y, int16_t width, int16_t height, const egui_color_int_t *data)
{
}

void egui_api_refresh_display(void)
{
}

void egui_api_timer_start(uint32_t ms)
{
}

void egui_api_timer_stop(void)
{
}

uint32_t egui_api_timer_get_current(void)
{
    return 0;
}

void egui_api_delay(uint32_t ms)
{
}


void egui_api_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}
