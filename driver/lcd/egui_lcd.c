/**
 * @file egui_lcd.c
 * @brief LCD driver common helper functions
 */

#include <string.h>
#include "egui_lcd.h"
#include "core/egui_api.h"
#include "core/egui_display_driver.h"

int egui_lcd_send_vendor_init_cmds(egui_panel_io_handle_t io, const egui_lcd_vendor_init_cmd_t *cmds, uint16_t size)
{
    if (!io || !cmds)
    {
        return -1;
    }

    for (uint16_t i = 0; i < size; i++)
    {
        int ret = io->tx_param(io, cmds[i].cmd, cmds[i].data, cmds[i].data_bytes);
        if (ret != 0)
        {
            return ret;
        }
        if (cmds[i].delay_ms > 0)
        {
            egui_api_delay(cmds[i].delay_ms);
        }
    }
    return 0;
}

void egui_lcd_hw_reset(egui_hal_lcd_driver_t *self, int delay_ms)
{
    if (self->set_rst)
    {
        self->set_rst(0);
        egui_api_delay(delay_ms);
        self->set_rst(1);
        egui_api_delay(delay_ms);
    }
}

/* ============================================================
 * LCD → Core Display Driver Registration
 * ============================================================ */

static egui_hal_lcd_driver_t *s_hal_lcd = NULL;

static void lcd_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    if (s_hal_lcd && s_hal_lcd->draw_area)
    {
        s_hal_lcd->draw_area(s_hal_lcd, x, y, w, h, data, w * h * sizeof(egui_color_int_t));
    }
}

static void lcd_display_wait_draw_complete(void)
{
    if (s_hal_lcd && s_hal_lcd->io && s_hal_lcd->io->wait_tx_done)
    {
        s_hal_lcd->io->wait_tx_done(s_hal_lcd->io);
    }
}

static void lcd_display_set_power(uint8_t on)
{
    if (s_hal_lcd && s_hal_lcd->set_power)
    {
        s_hal_lcd->set_power(s_hal_lcd, on);
    }
}

void egui_hal_lcd_register(egui_display_driver_t *driver, egui_hal_lcd_driver_t *lcd, const egui_hal_lcd_config_t *config)
{
    s_hal_lcd = lcd;

    /* Reset and initialize the HAL LCD driver */
    if (lcd->reset)
    {
        lcd->reset(lcd);
    }
    if (lcd->init)
    {
        lcd->init(lcd, config);
    }

    /* Fill in HAL-backed ops (cast away const - porting layer owns the mutable storage) */
    egui_display_driver_ops_t *ops = (egui_display_driver_ops_t *)driver->ops;
    ops->draw_area = lcd_display_draw_area;
    ops->wait_draw_complete = (lcd->io && lcd->io->wait_tx_done) ? lcd_display_wait_draw_complete : NULL;
    ops->set_power = lcd->set_power ? lcd_display_set_power : NULL;

    egui_display_driver_register(driver);
}

egui_hal_lcd_driver_t *egui_hal_lcd_get(void)
{
    return s_hal_lcd;
}
