#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "egui.h"
#include "port_main.h"
#include "app_lcd.h"
#include "lcd_st7789.h"

/**
 * STM32G0 port: display and platform driver registration.
 * Replaces the old api_mcu.c which directly implemented egui_api_* functions.
 */

// ============================================================================
// Display driver
// ============================================================================

static void mcu_display_init(void)
{
    // LCD init is handled in app_lcd_init() called from port_main
}

static void mcu_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    app_lcd_draw_data(x, y, w, h, (void *)data);
}

static void mcu_display_draw_area_async(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    st7789_draw_image_dma_async(x, y, w, h, (const uint16_t *)data);
}

static void mcu_display_wait_draw_complete(void)
{
    st7789_wait_dma_complete();
}

static void mcu_display_flush(void)
{
    // No-op for STM32 port
}

static const egui_display_driver_ops_t mcu_display_ops = {
        .init = mcu_display_init,
        .draw_area = mcu_display_draw_area,
        .draw_area_async = mcu_display_draw_area_async,
        .wait_draw_complete = mcu_display_wait_draw_complete,
        .flush = mcu_display_flush,
        .set_brightness = NULL,
        .set_power = NULL,
        .set_rotation = NULL,
        .fill_rect = NULL,
        .blit = NULL,
        .blend = NULL,
        .wait_vsync = NULL,
};

static egui_display_driver_t mcu_display_driver = {
        .ops = &mcu_display_ops,
        .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_0,
        .brightness = 255,
        .power_on = 1,
};

// ============================================================================
// Platform driver
// ============================================================================

static void mcu_vlog(const char *format, va_list args)
{
    vprintf(format, args);
}

static void mcu_assert_handler(const char *file, int line)
{
#if EGUI_CONFIG_DEBUG_LOG_LEVEL >= EGUI_LOG_IMPL_LEVEL_DBG
    char s_buf[0x200];
    memset(s_buf, 0, sizeof(s_buf));
    sprintf(s_buf, "vvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\nAssert@ file = %s, line = %d\n\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", file, line);
    printf("%s", s_buf);
#endif
    while (1)
        ;
}

static void *mcu_malloc(int size)
{
    return malloc(size);
}

static void mcu_free(void *ptr)
{
    free(ptr);
}

static void mcu_vsprintf(char *str, const char *format, va_list args)
{
    vsprintf(str, format, args);
}

static uint32_t mcu_get_tick_ms(void)
{
    return HAL_GetTick();
}

static void mcu_delay(uint32_t ms)
{
    HAL_Delay(ms);
}

#if APP_EGUI_CONFIG_USE_DMA_TO_RESET_PFB_BUFFER
extern DMA_HandleTypeDef hdma_memtomem_dma2_channel5;
const egui_color_int_t fixed_0_buffer[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT] = {0};

static void mcu_pfb_clear(void *s, int n)
{
    HAL_DMA_Start(&hdma_memtomem_dma2_channel5, (uint32_t)fixed_0_buffer, (uint32_t)s, n >> 2);
    HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_channel5, HAL_DMA_FULL_TRANSFER, 1000);
}
#else
static void mcu_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}
#endif

static egui_base_t mcu_interrupt_disable(void)
{
    egui_base_t level = __get_PRIMASK();
    __disable_irq();
    return level;
}

static void mcu_interrupt_enable(egui_base_t level)
{
    __set_PRIMASK(level);
}

static const egui_platform_ops_t mcu_platform_ops = {
        .malloc = mcu_malloc,
        .free = mcu_free,
        .vlog = mcu_vlog,
        .assert_handler = mcu_assert_handler,
        .vsprintf = mcu_vsprintf,
        .delay = mcu_delay,
        .get_tick_ms = mcu_get_tick_ms,
        .pfb_clear = mcu_pfb_clear,
        .interrupt_disable = mcu_interrupt_disable,
        .interrupt_enable = mcu_interrupt_enable,
        .load_external_resource = NULL,
        .mutex_create = NULL,
        .mutex_lock = NULL,
        .mutex_unlock = NULL,
        .mutex_destroy = NULL,
        .timer_start = NULL,
        .timer_stop = NULL,
        .memcpy_fast = NULL,
        .watchdog_feed = NULL,
};

static egui_platform_t mcu_platform = {
        .ops = &mcu_platform_ops,
};

// ============================================================================
// Port initialization
// ============================================================================

void egui_port_init(void)
{
    egui_display_driver_register(&mcu_display_driver);
    egui_platform_register(&mcu_platform);
}
