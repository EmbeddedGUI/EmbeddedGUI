/**
 * @file egui_hal_stm32g0.c
 * @brief STM32G0 HAL Bus IO implementations for EGUI
 *
 * Provides SPI/I2C bus operations and individual GPIO functions
 * for use with the unified Panel IO interface.
 */

#include "egui_hal_stm32.h"
#include "port_main.h"
#include "SoftIIC.h"
#include <string.h>

#define EGUI_LCD_SPI_TIMEOUT_MS 1000U
#define EGUI_LCD_SPI_CHUNK_SIZE 65535U
#define EGUI_TOUCH_IIC_DELAY 20U
#define APP_EGUI_LCD_SPI_USE_DMA 1U
#define APP_EGUI_LCD_SPI_DMA_THRESHOLD 64U

static customIIC s_touch_iic;
static const uint8_t *s_lcd_spi_dma_ptr;
static uint32_t s_lcd_spi_dma_remaining;
static uint8_t s_lcd_spi_dma_active;

/* ============================================================
 * LCD SPI Operations
 * ============================================================ */

/**
 * @brief Initialize the LCD SPI bus wrapper.
 */
static void lcd_spi_init(void)
{
    /* SPI is initialized by CubeMX. Keep CS inactive by default. */
    egui_hal_stm32g0_lcd_set_cs(1);
    s_lcd_spi_dma_ptr = NULL;
    s_lcd_spi_dma_remaining = 0U;
    s_lcd_spi_dma_active = 0U;
}

/**
 * @brief Deinitialize the LCD SPI bus wrapper.
 */
static void lcd_spi_deinit(void)
{
    /* Not needed - managed by HAL */
}

/**
 * @brief Transmit LCD SPI data in blocking mode.
 * @param data Transmit buffer.
 * @param len Number of bytes to transmit.
 * @return 0 on success, -1 on failure.
 *
 * Keep this direct SPI path as a stable fallback. When debugging timing
 * issues or DMA-related problems, switching the top-level write path back
 * to this helper is the quickest way to isolate the bus behavior.
 */
static int lcd_spi_write_blocking(const uint8_t *data, uint32_t len)
{
    uint32_t offset;
    uint16_t chunk;

    if ((data == NULL) && (len > 0U))
    {
        return -1;
    }
    if (len == 0U)
    {
        return 0;
    }

    while (APP_EGUI_LCD_SPI.State != HAL_SPI_STATE_READY)
    {
        continue;
    }

    for (offset = 0U; offset < len; offset += chunk)
    {
        chunk = (uint16_t)(((len - offset) > EGUI_LCD_SPI_CHUNK_SIZE) ? EGUI_LCD_SPI_CHUNK_SIZE : (len - offset));
        if (HAL_SPI_Transmit(&APP_EGUI_LCD_SPI, (uint8_t *)(data + offset), chunk, EGUI_LCD_SPI_TIMEOUT_MS) != HAL_OK)
        {
            return -1;
        }
    }

    return 0;
}

/**
 * @brief Start one LCD SPI DMA chunk.
 * @return 0 on success, -1 on failure.
 */
static int lcd_spi_start_next_dma(void)
{
    uint16_t chunk;

    if (s_lcd_spi_dma_remaining == 0U)
    {
        s_lcd_spi_dma_active = 0U;
        return 0;
    }

    chunk = (s_lcd_spi_dma_remaining > EGUI_LCD_SPI_CHUNK_SIZE) ? (uint16_t)EGUI_LCD_SPI_CHUNK_SIZE : (uint16_t)s_lcd_spi_dma_remaining;
    if (HAL_SPI_Transmit_DMA(&APP_EGUI_LCD_SPI, (uint8_t *)s_lcd_spi_dma_ptr, chunk) != HAL_OK)
    {
        s_lcd_spi_dma_active = 0U;
        return -1;
    }

    s_lcd_spi_dma_ptr += chunk;
    s_lcd_spi_dma_remaining -= chunk;
    s_lcd_spi_dma_active = 1U;
    return 0;
}

/**
 * @brief Transmit LCD SPI data.
 * @param data Transmit buffer.
 * @param len Number of bytes to transmit.
 * @return 0 on success, -1 on failure.
 *
 * Small command/parameter packets are sent directly to avoid DMA startup
 * overhead. Large pixel payloads automatically use DMA for better throughput.
 */
static int lcd_spi_write(const uint8_t *data, uint32_t len)
{
#if APP_EGUI_LCD_SPI_USE_DMA
    if ((data == NULL) && (len > 0U))
    {
        return -1;
    }
    if (len == 0U)
    {
        return 0;
    }

    if (len < APP_EGUI_LCD_SPI_DMA_THRESHOLD)
    {
        return lcd_spi_write_blocking(data, len);
    }

    while (APP_EGUI_LCD_SPI.State != HAL_SPI_STATE_READY)
    {
        continue;
    }

    s_lcd_spi_dma_ptr = data;
    s_lcd_spi_dma_remaining = len;
    s_lcd_spi_dma_active = 0U;
    return lcd_spi_start_next_dma();
#else
    /* Fallback path: preserve the original direct SPI implementation. */
    return lcd_spi_write_blocking(data, len);
#endif
}

/**
 * @brief Wait until the current LCD SPI transfer completes.
 */
static void lcd_spi_wait_complete(void)
{
#if APP_EGUI_LCD_SPI_USE_DMA
    while (s_lcd_spi_dma_active != 0U)
    {
        while ((APP_EGUI_LCD_SPI.State != HAL_SPI_STATE_READY) ||
               (APP_EGUI_LCD_SPI.hdmatx != NULL && APP_EGUI_LCD_SPI.hdmatx->State != HAL_DMA_STATE_READY))
        {
            continue;
        }

        if (s_lcd_spi_dma_remaining == 0U)
        {
            s_lcd_spi_dma_active = 0U;
        }
        else if (lcd_spi_start_next_dma() != 0)
        {
            Error_Handler();
        }
    }
#else
    while (APP_EGUI_LCD_SPI.State != HAL_SPI_STATE_READY)
    {
        continue;
    }
#endif
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
 * LCD GPIO Functions
 * ============================================================ */

/**
 * @brief Control the LCD reset pin.
 * @param level 0 = low, non-zero = high.
 */
void egui_hal_stm32g0_lcd_set_rst(uint8_t level)
{
    HAL_GPIO_WritePin(APP_EGUI_LCD_RST_PORT, APP_EGUI_LCD_RST_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Control the LCD DC pin.
 * @param level 0 = command, non-zero = data.
 */
void egui_hal_stm32g0_lcd_set_dc(uint8_t level)
{
    HAL_GPIO_WritePin(APP_EGUI_LCD_DC_PORT, APP_EGUI_LCD_DC_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Control the LCD chip select pin.
 * @param level 0 = selected, non-zero = released.
 */
void egui_hal_stm32g0_lcd_set_cs(uint8_t level)
{
    HAL_GPIO_WritePin(APP_EGUI_LCD_CS_PORT, APP_EGUI_LCD_CS_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ============================================================
 * LCD Backlight Control
 * ============================================================ */

/**
 * @brief Set the LCD backlight level.
 * @param level 0 = off, non-zero = on.
 */
void egui_hal_stm32g0_set_backlight_level(uint8_t level)
{
    HAL_GPIO_WritePin(APP_EGUI_LCD_LED_PORT, APP_EGUI_LCD_LED_PIN, level > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ============================================================
 * Touch I2C Operations
 * ============================================================ */

/**
 * @brief Initialize the software I2C pins used by the touch controller.
 */
static void touch_i2c_init(void)
{
    custom_iic_init(&s_touch_iic, APP_EGUI_TOUCH_SCL_PORT, APP_EGUI_TOUCH_SCL_PIN, APP_EGUI_TOUCH_SDA_PORT, APP_EGUI_TOUCH_SDA_PIN, EGUI_TOUCH_IIC_DELAY);
}

/**
 * @brief Deinitialize the touch I2C wrapper.
 */
static void touch_i2c_deinit(void)
{
    /* Not needed - managed by HAL */
}

/**
 * @brief Write data into a touch controller register.
 * @param addr 8-bit I2C address with write bit cleared.
 * @param reg Register address.
 * @param data Data buffer.
 * @param len Number of bytes to write.
 * @return 0 on success, -1 on failure.
 */
static int touch_i2c_write_reg(uint8_t addr, uint16_t reg, const uint8_t *data, uint16_t len)
{
    uint8_t reg_len = (reg > 0xFFU) ? 2U : 1U;
    return custom_iic_write_reg(s_touch_iic, addr, reg, reg_len, data, len) ? 0 : -1;
}

/**
 * @brief Read data from a touch controller register.
 * @param addr 8-bit I2C address with write bit cleared.
 * @param reg Register address.
 * @param data Receive buffer.
 * @param len Number of bytes to read.
 * @return 0 on success, -1 on failure.
 */
static int touch_i2c_read_reg(uint8_t addr, uint16_t reg, uint8_t *data, uint16_t len)
{
    uint8_t reg_len = (reg > 0xFFU) ? 2U : 1U;
    return custom_iic_read_reg(s_touch_iic, addr, reg, reg_len, data, len) ? 0 : -1;
}

/**
 * @brief Write a raw byte stream to the touch controller.
 * @param addr 8-bit I2C address with write bit cleared.
 * @param data Data buffer.
 * @param len Number of bytes to write.
 * @return 0 on success, -1 on failure.
 */
static int touch_i2c_write_raw(uint8_t addr, const uint8_t *data, uint16_t len)
{
    return custom_iic_write(s_touch_iic, addr, data, len) ? 0 : -1;
}

/**
 * @brief Read a raw byte stream from the touch controller.
 * @param addr 8-bit I2C address with write bit cleared.
 * @param data Receive buffer.
 * @param len Number of bytes to read.
 * @return 0 on success, -1 on failure.
 */
static int touch_i2c_read_raw(uint8_t addr, uint8_t *data, uint16_t len)
{
    return custom_iic_read(s_touch_iic, addr, data, len) ? 0 : -1;
}

/**
 * @brief Perform a repeated-start write then read transaction.
 * @param addr 8-bit I2C address with write bit cleared.
 * @param tx Write buffer.
 * @param tx_len Number of bytes to write first.
 * @param rx Read buffer.
 * @param rx_len Number of bytes to read after the repeated start.
 * @return 0 on success, -1 on failure.
 */
static int touch_i2c_write_read(uint8_t addr, const uint8_t *tx, uint16_t tx_len, uint8_t *rx, uint16_t rx_len)
{
    return custom_iic_write_read(s_touch_iic, addr, tx, tx_len, rx, rx_len) ? 0 : -1;
}

static const egui_bus_i2c_ops_t s_touch_i2c_ops = {
        .init = touch_i2c_init,
        .deinit = touch_i2c_deinit,
        .write_reg = touch_i2c_write_reg,
        .read_reg = touch_i2c_read_reg,
        .write_raw = touch_i2c_write_raw,
        .read_raw = touch_i2c_read_raw,
        .write_read = touch_i2c_write_read,
};

const egui_bus_i2c_ops_t *egui_hal_stm32g0_get_touch_i2c_ops(void)
{
    return &s_touch_i2c_ops;
}

/* ============================================================
 * Touch GPIO Functions
 * ============================================================ */

/**
 * @brief Control the touch reset pin.
 * @param level 0 = low, non-zero = high.
 */
void egui_hal_stm32g0_touch_set_rst(uint8_t level)
{
    HAL_GPIO_WritePin(APP_EGUI_TOUCH_RST_PORT, APP_EGUI_TOUCH_RST_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Read the touch interrupt pin.
 * @return 1 when interrupt is pending, otherwise 0.
 */
uint8_t egui_hal_stm32g0_touch_get_int(void)
{
    /* PA4 is configured as pull-up + falling-edge EXTI in CubeMX, so
     * low level means interrupt pending. The GPIO HAL should expose the
     * logical pending state expected by the bridge, not raw pin polarity. */
    return HAL_GPIO_ReadPin(APP_EGUI_TOUCH_INT_PORT, APP_EGUI_TOUCH_INT_PIN) == GPIO_PIN_RESET ? 1 : 0;
}
