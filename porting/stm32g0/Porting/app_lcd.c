
#include <stdio.h>
#include "app_lcd.h"
#include "egui.h"
#include "lcd_st7789.h"
#include "tc_ft6336.h"

#define APP_LCD_INVALID_POINT 0xFFFF

static int is_touch_in_process;
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    //   printf("FT6336 INT, GPIO_Pin: 0x%x\r\n", GPIO_Pin);
    if (GPIO_Pin == APP_EGUI_TOUCH_INT_PIN)
    {
        is_touch_in_process = 1;
    }
}

static int app_lcd_get_touch_point(uint16_t *x, uint16_t *y)
{
    return ft6336_get_touch_point(x, y);
}

static uint16_t last_touch_x, last_touch_y;
void app_lcd_polling_work(void)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    if (is_touch_in_process)
    {
        uint16_t x, y;
        if (app_lcd_get_touch_point(&x, &y))
        {
            if (x != last_touch_x || y != last_touch_y)
            {
                if (last_touch_x == APP_LCD_INVALID_POINT || last_touch_y == APP_LCD_INVALID_POINT)
                {
                    egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_DOWN, x, y);
                }
                else
                {
                    egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_MOVE, x, y);
                }
                last_touch_x = x;
                last_touch_y = y;
            }
        }
        else
        {
            if (last_touch_x != APP_LCD_INVALID_POINT && last_touch_y != APP_LCD_INVALID_POINT)
            {
                egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_UP, last_touch_x, last_touch_y);
                last_touch_x = APP_LCD_INVALID_POINT;
                last_touch_y = APP_LCD_INVALID_POINT;
            }
            is_touch_in_process = 0;
        }
    }
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
}

#if APP_EGUI_CONFIG_LCD_ENABLE_BACKUP_PFB
static egui_color_int_t egui_pfb_backup[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT] APP_EGUI_CONFIG_PFB_SECTION;
static void *egui_normal_pfb_buffer;
#endif
void app_lcd_draw_data(int16_t x, int16_t y, int16_t width, int16_t height, void *data)
{
#if APP_EGUI_CONFIG_LCD_ENABLE_BACKUP_PFB
    if (data == egui_normal_pfb_buffer)
    {
        egui_core_set_pfb_buffer_ptr(egui_pfb_backup);
    }
    else
    {
        egui_core_set_pfb_buffer_ptr(egui_normal_pfb_buffer);
    }
    st7789_draw_image_dma_cache(x, y, width, height, (uint16_t *)data);
#else
    st7789_draw_image(x, y, width, height, (uint16_t *)data);
#endif
}

void app_lcd_power_on(void)
{
    // Enable LCD backlight.
    HAL_GPIO_WritePin(APP_EGUI_LCD_LED_PORT, APP_EGUI_LCD_LED_PIN, GPIO_PIN_SET);
}

void app_lcd_power_off(void)
{
    // Diable LCD backlight.
    HAL_GPIO_WritePin(APP_EGUI_LCD_LED_PORT, APP_EGUI_LCD_LED_PIN, GPIO_PIN_RESET);
}

void app_lcd_init(void)
{
    last_touch_x = APP_LCD_INVALID_POINT;
    last_touch_y = APP_LCD_INVALID_POINT;

    app_lcd_power_on();

    st7789_init();
    ft6336_init();

#if APP_EGUI_CONFIG_LCD_ENABLE_BACKUP_PFB
    egui_normal_pfb_buffer = egui_core_get_pfb_buffer_ptr();
#endif
}
