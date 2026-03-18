/**
 * @file egui_panel_io_i2c.c
 * @brief I2C Panel IO implementation
 */

#include "egui_panel_io_i2c.h"
#include <string.h>

/* Write register: cmd = register address, param = data */
static int i2c_tx_param(egui_panel_io_t *io, int cmd, const void *param, size_t param_size)
{
    egui_panel_io_i2c_t *self = (egui_panel_io_i2c_t *)io;
    return self->i2c->write_reg(self->dev_addr, (uint16_t)cmd, param, (uint16_t)param_size);
}

/* Read register: cmd = register address, param = output buffer */
static int i2c_rx_param(egui_panel_io_t *io, int cmd, void *param, size_t param_size)
{
    egui_panel_io_i2c_t *self = (egui_panel_io_i2c_t *)io;
    return self->i2c->read_reg(self->dev_addr, (uint16_t)cmd, param, (uint16_t)param_size);
}

/* Send data: for I2C OLEDs, cmd = data control byte (e.g., 0x40) */
static int i2c_tx_color(egui_panel_io_t *io, int cmd, const void *color, size_t color_size)
{
    egui_panel_io_i2c_t *self = (egui_panel_io_i2c_t *)io;
    const uint8_t *data = (const uint8_t *)color;
    uint16_t reg = (cmd >= 0) ? (uint16_t)cmd : 0;

    /* Write in chunks to avoid large stack buffers */
    while (color_size > 0)
    {
        uint16_t chunk = (color_size > 128) ? 128 : (uint16_t)color_size;
        int ret = self->i2c->write_reg(self->dev_addr, reg, data, chunk);
        if (ret != 0)
        {
            return ret;
        }
        data += chunk;
        color_size -= chunk;
    }
    return 0;
}

/* I2C init */
static void i2c_io_init(egui_panel_io_t *io)
{
    egui_panel_io_i2c_t *self = (egui_panel_io_i2c_t *)io;
    if (self->i2c->init)
    {
        self->i2c->init();
    }
}

/* I2C deinit */
static void i2c_io_deinit(egui_panel_io_t *io)
{
    egui_panel_io_i2c_t *self = (egui_panel_io_i2c_t *)io;
    if (self->i2c->deinit)
    {
        self->i2c->deinit();
    }
}

void egui_panel_io_i2c_init(egui_panel_io_i2c_t *io, const egui_bus_i2c_ops_t *i2c, uint8_t dev_addr)
{
    if (!io || !i2c)
    {
        return;
    }

    memset(io, 0, sizeof(egui_panel_io_i2c_t));

    io->base.tx_param = i2c_tx_param;
    io->base.rx_param = (i2c->read_reg) ? i2c_rx_param : NULL;
    io->base.tx_color = i2c_tx_color;
    io->base.wait_tx_done = NULL; /* I2C is synchronous */
    io->base.init = i2c_io_init;
    io->base.deinit = i2c_io_deinit;

    io->i2c = i2c;
    io->dev_addr = dev_addr;
}

void egui_panel_io_i2c_set_addr(egui_panel_io_i2c_t *io, uint8_t dev_addr)
{
    if (io)
    {
        io->dev_addr = dev_addr;
    }
}
