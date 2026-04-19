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

static egui_hal_lcd_driver_t *egui_hal_lcd_from_core(egui_core_t *core)
{
    egui_display_driver_t *driver;

    if (core == NULL)
    {
        return NULL;
    }

    driver = egui_display_driver_get(core);
    if (driver == NULL)
    {
        return NULL;
    }

    return (egui_hal_lcd_driver_t *)driver->user_data;
}

static void lcd_display_draw_area(egui_core_t *core, int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    egui_hal_lcd_driver_t *lcd = egui_hal_lcd_from_core(core);

    if (lcd != NULL && lcd->draw_area != NULL)
    {
        lcd->bridge_core = core;
        lcd->draw_area(lcd, x, y, w, h, data, w * h * sizeof(egui_color_int_t));
    }
}

static void lcd_display_wait_draw_complete(egui_core_t *core)
{
    egui_hal_lcd_driver_t *lcd = egui_hal_lcd_from_core(core);

    if (lcd != NULL && lcd->io != NULL && lcd->io->wait_tx_done != NULL)
    {
        lcd->bridge_core = core;
        lcd->io->wait_tx_done(lcd->io);
    }
}

static void lcd_display_set_power(egui_core_t *core, uint8_t on)
{
    egui_hal_lcd_driver_t *lcd = egui_hal_lcd_from_core(core);

    if (lcd != NULL && lcd->set_power != NULL)
    {
        lcd->bridge_core = core;
        lcd->set_power(lcd, on);
    }
}

void egui_hal_lcd_register(egui_display_driver_t *driver, egui_hal_lcd_driver_t *lcd, const egui_hal_lcd_config_t *config)
{
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
    driver->user_data = lcd;

    /* Driver registration into core is done by the caller after egui_init() */
}

egui_hal_lcd_driver_t *egui_hal_lcd_get(egui_core_t *core)
{
    return egui_hal_lcd_from_core(core);
}
