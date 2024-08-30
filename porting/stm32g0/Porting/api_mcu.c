#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>

#include "uicode.h"

#include "port_main.h"


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
    app_lcd_draw_data(x, y, width, height, data);
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
    return HAL_GetTick();
}

void egui_api_delay(uint32_t ms)
{
    HAL_Delay(ms);
}


#if APP_EGUI_CONFIG_USE_DMA_TO_RESET_PFB_BUFFER
// Use DMA is faster than memset
extern DMA_HandleTypeDef hdma_memtomem_dma2_channel5;
const egui_color_int_t fixed_0_buffer[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT] = {0};

void egui_api_pfb_clear(void *s, int n)
{
    HAL_DMA_Start(&hdma_memtomem_dma2_channel5, (uint32_t)fixed_0_buffer, (uint32_t)s, n >> 2);
    HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_channel5, HAL_DMA_FULL_TRANSFER, 1000);
}
#else
void egui_api_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}
#endif

