#if EGUI_DRIVER_PANEL_IO_SPI_ENABLE

/**
 * @file egui_panel_io_spi.c
 * @brief SPI Panel IO implementation
 */

#include "egui_panel_io_spi.h"
#include <string.h>

/* Send command byte (DC=0) + optional parameter bytes (DC=1), blocking */
static int spi_tx_param(egui_panel_io_t *io, int cmd, const void *param, size_t param_size)
{
    egui_panel_io_spi_t *self = (egui_panel_io_spi_t *)io;
    uint8_t cmd_byte = (uint8_t)cmd;

    /* Send command byte with DC=0 */
    if (self->set_dc)
    {
        self->set_dc(0);
    }
    /* CS active */
    if (self->set_cs)
    {
        self->set_cs(0);
    }
    self->spi->write(&cmd_byte, 1);
    if (self->spi->wait_complete)
    {
        self->spi->wait_complete();
    }
    /* CS inactive */
    if (self->set_cs)
    {
        self->set_cs(1);
    }

    /* Send parameter data with DC=1 */
    if (param && param_size > 0)
    {
        if (self->set_dc)
        {
            self->set_dc(1);
        }
        /* CS active */
        if (self->set_cs)
        {
            self->set_cs(0);
        }
        self->spi->write((const uint8_t *)param, param_size);
        if (self->spi->wait_complete)
        {
            self->spi->wait_complete();
        }
        /* CS inactive */
        if (self->set_cs)
        {
            self->set_cs(1);
        }
    }

    return 0;
}

/* Read parameter data: send command (DC=0), then read (DC=1) */
static int spi_rx_param(egui_panel_io_t *io, int cmd, void *param, size_t param_size)
{
    egui_panel_io_spi_t *self = (egui_panel_io_spi_t *)io;

    if (!self->spi->read)
    {
        return -1;
    }

    uint8_t cmd_byte = (uint8_t)cmd;

    /* Send command */
    if (self->set_dc)
    {
        self->set_dc(0);
    }

    if (self->set_cs)
    {
        self->set_cs(0);
    }
    self->spi->write(&cmd_byte, 1);
    if (self->spi->wait_complete)
    {
        self->spi->wait_complete();
    }
    if (self->set_cs)
    {
        self->set_cs(1);
    }

    /* Read response */
    if (self->set_dc)
    {
        self->set_dc(1);
    }
    if (self->set_cs)
    {
        self->set_cs(0);
    }
    self->spi->read((uint8_t *)param, param_size);
    if (self->set_cs)
    {
        self->set_cs(1);
    }

    return 0;
}

/* Send color data: set DC=1, CS=0, start transfer (potentially async) */
static int spi_tx_color(egui_panel_io_t *io, int cmd, const void *color, size_t color_size)
{
    egui_panel_io_spi_t *self = (egui_panel_io_spi_t *)io;

    /* Send command byte if specified (cmd >= 0) */
    if (cmd >= 0)
    {
        uint8_t cmd_byte = (uint8_t)cmd;
        if (self->set_cs)
        {
            self->set_cs(0);
        }
        if (self->set_dc)
        {
            self->set_dc(0);
        }
        self->spi->write(&cmd_byte, 1);
        if (self->spi->wait_complete)
        {
            self->spi->wait_complete();
        }
    }
    else
    {
        if (self->set_cs)
        {
            self->set_cs(0);
        }
    }

    /* Send color data with DC=1 (may be async/DMA) */
    if (self->set_dc)
    {
        self->set_dc(1);
    }
    self->spi->write((const uint8_t *)color, color_size);

    /* If synchronous mode, deselect CS now */
    if (!self->spi->wait_complete)
    {
        if (self->set_cs)
        {
            self->set_cs(1);
        }
    }
    /* If async mode, CS will be deselected in wait_tx_done */

    return 0;
}

/* Wait for async transfer to complete, then deselect CS */
static void spi_wait_tx_done(egui_panel_io_t *io)
{
    egui_panel_io_spi_t *self = (egui_panel_io_spi_t *)io;

    if (self->spi->wait_complete)
    {
        self->spi->wait_complete();
    }
    if (self->set_cs)
    {
        self->set_cs(1);
    }
}

/* Initialize SPI bus */
static void spi_io_init(egui_panel_io_t *io)
{
    egui_panel_io_spi_t *self = (egui_panel_io_spi_t *)io;
    if (self->spi->init)
    {
        self->spi->init();
    }
}

/* Deinitialize SPI bus */
static void spi_io_deinit(egui_panel_io_t *io)
{
    egui_panel_io_spi_t *self = (egui_panel_io_spi_t *)io;
    if (self->spi->deinit)
    {
        self->spi->deinit();
    }
}

void egui_panel_io_spi_init(egui_panel_io_spi_t *io, const egui_bus_spi_ops_t *spi, void (*set_dc)(uint8_t level), void (*set_cs)(uint8_t level))
{
    if (!io || !spi || !spi->write)
    {
        return;
    }

    memset(io, 0, sizeof(egui_panel_io_spi_t));

    io->base.tx_param = spi_tx_param;
    io->base.rx_param = spi->read ? spi_rx_param : NULL;
    io->base.tx_color = spi_tx_color;
    io->base.wait_tx_done = spi->wait_complete ? spi_wait_tx_done : NULL;
    io->base.init = spi_io_init;
    io->base.deinit = spi_io_deinit;

    io->spi = spi;
    io->set_dc = set_dc;
    io->set_cs = set_cs;
}

#endif /* EGUI_DRIVER_PANEL_IO_SPI_ENABLE */
