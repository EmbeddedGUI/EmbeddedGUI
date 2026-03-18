/**
 * @file egui_panel_io_8080.c
 * @brief 8080 Parallel Bus Panel IO implementation
 */

#include "egui_panel_io_8080.h"
#include <string.h>

static int bus8080_tx_param(egui_panel_io_t *io, int cmd, const void *param, size_t param_size)
{
    egui_panel_io_8080_t *self = (egui_panel_io_8080_t *)io;

    self->bus_8080->write_cmd((uint16_t)cmd);

    if (param && param_size > 0)
    {
        self->bus_8080->write_data((const uint8_t *)param, param_size);
        if (self->bus_8080->wait_complete)
        {
            self->bus_8080->wait_complete();
        }
    }
    return 0;
}

static int bus8080_rx_param(egui_panel_io_t *io, int cmd, void *param, size_t param_size)
{
    egui_panel_io_8080_t *self = (egui_panel_io_8080_t *)io;

    if (!self->bus_8080->read_data)
    {
        return -1;
    }

    self->bus_8080->write_cmd((uint16_t)cmd);
    return self->bus_8080->read_data((uint8_t *)param, param_size);
}

static int bus8080_tx_color(egui_panel_io_t *io, int cmd, const void *color, size_t color_size)
{
    egui_panel_io_8080_t *self = (egui_panel_io_8080_t *)io;

    if (cmd >= 0)
    {
        self->bus_8080->write_cmd((uint16_t)cmd);
    }
    self->bus_8080->write_data((const uint8_t *)color, color_size);
    /* Don't wait here - caller uses wait_tx_done if async */
    return 0;
}

static void bus8080_wait_tx_done(egui_panel_io_t *io)
{
    egui_panel_io_8080_t *self = (egui_panel_io_8080_t *)io;
    if (self->bus_8080->wait_complete)
    {
        self->bus_8080->wait_complete();
    }
}

static void bus8080_io_init(egui_panel_io_t *io)
{
    egui_panel_io_8080_t *self = (egui_panel_io_8080_t *)io;
    if (self->bus_8080->init)
    {
        self->bus_8080->init();
    }
}

static void bus8080_io_deinit(egui_panel_io_t *io)
{
    egui_panel_io_8080_t *self = (egui_panel_io_8080_t *)io;
    if (self->bus_8080->deinit)
    {
        self->bus_8080->deinit();
    }
}

void egui_panel_io_8080_init(egui_panel_io_8080_t *io, const egui_bus_8080_ops_t *bus_8080)
{
    if (!io || !bus_8080)
    {
        return;
    }

    memset(io, 0, sizeof(egui_panel_io_8080_t));

    io->base.tx_param = bus8080_tx_param;
    io->base.rx_param = bus_8080->read_data ? bus8080_rx_param : NULL;
    io->base.tx_color = bus8080_tx_color;
    io->base.wait_tx_done = bus_8080->wait_complete ? bus8080_wait_tx_done : NULL;
    io->base.init = bus8080_io_init;
    io->base.deinit = bus8080_io_deinit;

    io->bus_8080 = bus_8080;
}
