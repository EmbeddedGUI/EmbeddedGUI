/**
 * @file egui_hal_stm32g0.c
 * @brief STM32G0 HAL Bus IO implementations for EGUI
 */

#include "egui_hal_stm32g0.h"
#include "port_main.h"
#include <string.h>

/* External HAL handles */
extern SPI_HandleTypeDef APP_EGUI_LCD_SPI;
extern I2C_HandleTypeDef APP_EGUI_TOUCH_I2C;

/* ============================================================
 * LCD SPI Operations
 * ============================================================ */

static void lcd_spi_init(void)
{
    /* SPI is initialized by CubeMX in main.c */
}

static void lcd_spi_deinit(void)
{
    /* Not needed - managed by HAL */
}

static int lcd_spi_write(const uint8_t *data, uint32_t len)
{
    HAL_StatusTypeDef status;
    status = HAL_SPI_Transmit_DMA(&APP_EGUI_LCD_SPI, (uint8_t *)data, len);
    return (status == HAL_OK) ? 0 : -1;
}

static void lcd_spi_wait_complete(void)
{
    while (APP_EGUI_LCD_SPI.hdmatx->State != HAL_DMA_STATE_READY)
    {
        continue;
    }
}

static const egui_bus_spi_ops_t s_lcd_spi_ops = {
        .init = lcd_spi_init,
        .deinit = lcd_spi_deinit,
        .write = lcd_spi_write,
        .wait_complete = lcd_spi_wait_complete,
        .read = NULL, /* LCD doesn't need read */
};

const egui_bus_spi_ops_t *egui_hal_stm32g0_get_lcd_spi_ops(void)
{
    return &s_lcd_spi_ops;
}

/* ============================================================
 * LCD GPIO Operations
 * ============================================================ */

static void lcd_gpio_init(void)
{
    /* GPIO is initialized by CubeMX in main.c */
}

static void lcd_gpio_deinit(void)
{
    /* Not needed */
}

static void lcd_gpio_set_rst(uint8_t level)
{
    HAL_GPIO_WritePin(APP_EGUI_LCD_RST_PORT, APP_EGUI_LCD_RST_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void lcd_gpio_set_dc(uint8_t level)
{
    HAL_GPIO_WritePin(APP_EGUI_LCD_DC_PORT, APP_EGUI_LCD_DC_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void lcd_gpio_set_cs(uint8_t level)
{
    /* CS is managed by hardware SPI on STM32G0 */
    (void)level;
}

static const egui_lcd_gpio_ops_t s_lcd_gpio_ops = {
        .init = lcd_gpio_init,
        .deinit = lcd_gpio_deinit,
        .set_rst = lcd_gpio_set_rst,
        .set_dc = lcd_gpio_set_dc,
        .set_cs = lcd_gpio_set_cs,
};

const egui_lcd_gpio_ops_t *egui_hal_stm32g0_get_lcd_gpio_ops(void)
{
    return &s_lcd_gpio_ops;
}

/* ============================================================
 * LCD Backlight Control
 * ============================================================ */

void egui_hal_stm32g0_set_backlight(egui_hal_lcd_driver_t *self, uint8_t level)
{
    (void)self;
    HAL_GPIO_WritePin(APP_EGUI_LCD_LED_PORT, APP_EGUI_LCD_LED_PIN, level > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ============================================================
 * Touch I2C Operations
 * ============================================================ */

static void touch_i2c_init(void)
{
    /* I2C is initialized by CubeMX in main.c */
}

static void touch_i2c_deinit(void)
{
    /* Not needed - managed by HAL */
}

static uint16_t touch_i2c_mem_addr_size(uint16_t reg)
{
    return (reg > 0xFFU) ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT;
}

static int touch_i2c_write_reg(uint8_t addr, uint16_t reg, const uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Write(&APP_EGUI_TOUCH_I2C, addr, reg, touch_i2c_mem_addr_size(reg), (uint8_t *)data, len, 1000);
    return (status == HAL_OK) ? 0 : -1;
}

static int touch_i2c_read_reg(uint8_t addr, uint16_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;
    status = HAL_I2C_Mem_Read(&APP_EGUI_TOUCH_I2C, addr, reg, touch_i2c_mem_addr_size(reg), data, len, 1000);
    return (status == HAL_OK) ? 0 : -1;
}

static int touch_i2c_write_raw(uint8_t addr, const uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;
    status = HAL_I2C_Master_Transmit(&APP_EGUI_TOUCH_I2C, addr, (uint8_t *)data, len, 1000);
    return (status == HAL_OK) ? 0 : -1;
}

static int touch_i2c_read_raw(uint8_t addr, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef status;
    status = HAL_I2C_Master_Receive(&APP_EGUI_TOUCH_I2C, addr, data, len, 1000);
    return (status == HAL_OK) ? 0 : -1;
}

static const egui_bus_i2c_ops_t s_touch_i2c_ops = {
        .init = touch_i2c_init,
        .deinit = touch_i2c_deinit,
        .write_reg = touch_i2c_write_reg,
        .read_reg = touch_i2c_read_reg,
        .write_raw = touch_i2c_write_raw,
        .read_raw = touch_i2c_read_raw,
        .write_read = NULL,
};

const egui_bus_i2c_ops_t *egui_hal_stm32g0_get_touch_i2c_ops(void)
{
    return &s_touch_i2c_ops;
}

/* ============================================================
 * Touch GPIO Operations
 * ============================================================ */

static void touch_gpio_init(void)
{
    /* GPIO is initialized by CubeMX in main.c */
}

static void touch_gpio_deinit(void)
{
    /* Not needed */
}

static void touch_gpio_set_rst(uint8_t level)
{
    HAL_GPIO_WritePin(APP_EGUI_TOUCH_RST_PORT, APP_EGUI_TOUCH_RST_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static uint8_t touch_gpio_get_int(void)
{
    /* PB2 is configured as pull-up + falling-edge EXTI in CubeMX, so
     * low level means interrupt pending. The GPIO HAL should expose the
     * logical pending state expected by the bridge, not raw pin polarity. */
    return HAL_GPIO_ReadPin(APP_EGUI_TOUCH_INT_PORT, APP_EGUI_TOUCH_INT_PIN) == GPIO_PIN_RESET ? 1 : 0;
}

static const egui_touch_gpio_ops_t s_touch_gpio_ops = {
        .init = touch_gpio_init,
        .deinit = touch_gpio_deinit,
        .set_rst = touch_gpio_set_rst,
        .set_int = NULL, /* INT is input only, no set needed */
        .get_int = touch_gpio_get_int,
};

const egui_touch_gpio_ops_t *egui_hal_stm32g0_get_touch_gpio_ops(void)
{
    return &s_touch_gpio_ops;
}
