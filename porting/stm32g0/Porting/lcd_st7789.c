
#include <stdio.h>
#include "lcd_st7789.h"

/**
 * @brief Write command to ST7789 controller
 * @param cmd -> command to write
 * @return none
 */
static void st7789_write_command(uint8_t cmd)
{
    ST7789_DC_CLR();
    HAL_SPI_Transmit(&ST7789_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}

/**
 * @brief Write data to ST7789 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
static void st7789_write_data(uint8_t *buff, size_t buff_size)
{
    ST7789_DC_SET();

    // should not over 64K at once
    HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, buff, buff_size);
    while (ST7789_SPI_PORT.hdmatx->State != HAL_DMA_STATE_READY)
        continue;
}

static void st7789_write_data_dma_without_wait(uint8_t *buff, size_t buff_size)
{
    ST7789_DC_SET();
    // should not over 64K at once
    HAL_SPI_Transmit_DMA(&ST7789_SPI_PORT, buff, buff_size);
}
/**
 * @brief Write data to ST7789 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */
static void st7789_write_small_data(uint8_t data)
{
    ST7789_DC_SET();
    HAL_SPI_Transmit(&ST7789_SPI_PORT, &data, sizeof(data), HAL_MAX_DELAY);
}

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in st7789.h)
 * @return none
 */
void st7789_set_rotation(uint8_t m)
{
    st7789_write_command(ST7789_MADCTL); // MADCTL
    switch (m)
    {
    case 0:
        st7789_write_small_data(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
        break;
    case 1:
        st7789_write_small_data(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
        break;
    case 2:
        st7789_write_small_data(ST7789_MADCTL_RGB);
        break;
    case 3:
        st7789_write_small_data(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
        break;
    default:
        break;
    }
}

/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void st7789_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint16_t x_start = x0 + ST7789_X_SHIFT, x_end = x1 + ST7789_X_SHIFT;
    uint16_t y_start = y0 + ST7789_Y_SHIFT, y_end = y1 + ST7789_Y_SHIFT;

    /* Column Address set */
    st7789_write_command(ST7789_CASET);
    {
        uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
        st7789_write_data(data, sizeof(data));
    }

    /* Row Address set */
    st7789_write_command(ST7789_RASET);
    {
        uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
        st7789_write_data(data, sizeof(data));
    }
    /* Write to RAM */
    st7789_write_command(ST7789_RAMWR);
}

/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void st7789_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    st7789_set_address_window(x, y, x + w - 1, y + h - 1);
    st7789_write_data((uint8_t *)data, sizeof(uint16_t) * w * h);
}

void st7789_draw_image_dma_cache(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    // wait last DMA transfer finished
    while (ST7789_SPI_PORT.hdmatx->State != HAL_DMA_STATE_READY)
    {
        continue;
    }
    st7789_set_address_window(x, y, x + w - 1, y + h - 1);
    st7789_write_data_dma_without_wait((uint8_t *)data, sizeof(uint16_t) * w * h);
}

/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x < 0) || (x >= ST7789_WIDTH) || (y < 0) || (y >= ST7789_HEIGHT))
        return;

    st7789_set_address_window(x, y, x, y);
    uint8_t data[] = {color >> 8, color & 0xFF};
    st7789_write_data(data, sizeof(data));
}

/**
 * @brief Initialize ST7789 controller
 * @param none
 * @return none
 */
void st7789_init(void)
{
    HAL_Delay(10);
    ST7789_RST_CLR();
    HAL_Delay(10);
    ST7789_RST_SET();
    HAL_Delay(20);

    // Change to little enddian.
    st7789_write_command(0xB0); //	RAMCTRL
    {
        uint8_t data[] = {0x00, 0xF8}; // Little Enddian
        st7789_write_data(data, sizeof(data));
    }
    st7789_write_command(ST7789_COLMOD); //	Set color mode
    st7789_write_small_data(ST7789_COLOR_MODE_16bit);
    st7789_write_command(0xB2); //	Porch control
    {
        uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
        st7789_write_data(data, sizeof(data));
    }
    st7789_set_rotation(ST7789_ROTATION); //	MADCTL (Display Rotation)

    /* Internal LCD Voltage generator settings */
    st7789_write_command(0XB7);    //	Gate Control
    st7789_write_small_data(0x35); //	Default value
    st7789_write_command(0xBB);    //	VCOM setting
    st7789_write_small_data(0x19); //	0.725v (default 0.75v for 0x20)
    st7789_write_command(0xC0);    //	LCMCTRL
    st7789_write_small_data(0x2C); //	Default value
    st7789_write_command(0xC2);    //	VDV and VRH command Enable
    st7789_write_small_data(0x01); //	Default value
    st7789_write_command(0xC3);    //	VRH set
    st7789_write_small_data(0x12); //	+-4.45v (defalut +-4.1v for 0x0B)
    st7789_write_command(0xC4);    //	VDV set
    st7789_write_small_data(0x20); //	Default value
    st7789_write_command(0xC6);    //	Frame rate control in normal mode
    st7789_write_small_data(0x0F); //	Default value (60HZ)
    st7789_write_command(0xD0);    //	Power control
    st7789_write_small_data(0xA4); //	Default value
    st7789_write_small_data(0xA1); //	Default value
    /**************** Division line ****************/

    st7789_write_command(0xE0);
    {
        uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
        st7789_write_data(data, sizeof(data));
    }

    st7789_write_command(0xE1);
    {
        uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
        st7789_write_data(data, sizeof(data));
    }
    st7789_write_command(ST7789_INVON);  //	Inversion ON
    st7789_write_command(ST7789_SLPOUT); //	Out of sleep mode
    st7789_write_command(ST7789_NORON);  //	Normal Display on
    st7789_write_command(ST7789_DISPON); //	Main screen turned on
}
