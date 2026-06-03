
#include "SoftIIC.h"
#define USE_NULL_DELAY 1

#if !USE_NULL_DELAY
#include "delay.h"
#endif

#define I2C_SDA_H(iic) \
    HAL_GPIO_WritePin(iic.SDA_GPIO_PORT, iic.SDA_GPIO_PIN, GPIO_PIN_SET)
#define I2C_SDA_L(iic) \
    HAL_GPIO_WritePin(iic.SDA_GPIO_PORT, iic.SDA_GPIO_PIN, GPIO_PIN_RESET)


#define I2C_SCL_H(iic) \
    HAL_GPIO_WritePin(iic.SCL_GPIO_PORT, iic.SCL_GPIO_PIN, GPIO_PIN_SET)

#define I2C_SCL_L(iic) \
    HAL_GPIO_WritePin(iic.SCL_GPIO_PORT, iic.SCL_GPIO_PIN, GPIO_PIN_RESET)

#define I2C_SDA_Read(iic) \
    HAL_GPIO_ReadPin(iic.SDA_GPIO_PORT, iic.SDA_GPIO_PIN)

#define I2C_SCL_Read(iic) \
    HAL_GPIO_ReadPin(iic.SCL_GPIO_PORT, iic.SCL_GPIO_PIN)

static void Delay(__IO uint32_t nCount)
{

#if USE_NULL_DELAY
    // nCount *= 5.154;
    // for(; nCount != 0; nCount--);
    unsigned char x;
    while (nCount--)
        for (x = 0; x < 8; x++);
#else
    delay_us(nCount);
#endif
}

/**
 * @brief Send an address byte and check whether the slave acknowledges it.
 * @param iic Software I2C descriptor.
 * @param addr 8-bit address byte including the R/W bit.
 * @return 1 on success, 0 on failure.
 */
static uint8_t custom_iic_send_addr(customIIC iic, uint8_t addr)
{
    custom_iic_send_byte(iic, addr);
    return custom_iic_wait_ack(iic);
}

/**
 * @brief Send a multi-byte register address.
 * @param iic Software I2C descriptor.
 * @param reg Register address value.
 * @param reg_len Register address width in bytes.
 * @return 1 on success, 0 on failure.
 */
static uint8_t custom_iic_send_reg(customIIC iic, uint16_t reg, uint8_t reg_len)
{
    if (reg_len == 2U) {
        custom_iic_send_byte(iic, (uint8_t)(reg >> 8));
        if (!custom_iic_wait_ack(iic)) {
            return 0;
        }
    }

    custom_iic_send_byte(iic, (uint8_t)reg);
    return custom_iic_wait_ack(iic);
}

void custom_iic_start(customIIC iic)
{
    I2C_SDA_H(iic);
    I2C_SCL_H(iic);
    Delay(iic.delay);
    I2C_SDA_L(iic);
    Delay(iic.delay);
    I2C_SCL_L(iic);
    Delay(iic.delay);
}

void custom_iic_stop(customIIC iic)
{
    I2C_SCL_L(iic);
    I2C_SDA_L(iic);
    Delay(iic.delay);
    I2C_SCL_H(iic);
    Delay(iic.delay);
    I2C_SDA_H(iic);
    Delay(iic.delay);
}

uint8_t custom_iic_wait_ack(customIIC iic)
{
    uint8_t waittime = 0;
    uint8_t rack     = 1;

    I2C_SDA_H(iic);
    Delay(iic.delay);
    I2C_SCL_H(iic);
    Delay(iic.delay);

    while (I2C_SDA_Read(iic)) {
        waittime++;

        if (waittime > 250) {
            custom_iic_stop(iic);
            rack = 0;
            break;
        }
    }

    I2C_SCL_L(iic);
    Delay(iic.delay);
    return rack;
}

void custom_iic_ack(customIIC iic)
{
    I2C_SDA_L(iic);
    Delay(iic.delay);
    I2C_SCL_H(iic);
    Delay(iic.delay);
    I2C_SCL_L(iic);
    Delay(iic.delay);
    I2C_SDA_H(iic);
    Delay(iic.delay);
}

void custom_iic_nack(customIIC iic)
{
    I2C_SDA_H(iic);
    Delay(iic.delay);
    I2C_SCL_H(iic);
    Delay(iic.delay);
    I2C_SCL_L(iic);
    Delay(iic.delay);
}

void custom_iic_send_byte(customIIC iic, uint8_t data)
{
    uint8_t t;

    for (t = 0; t < 8; t++) {
        HAL_GPIO_WritePin(iic.SDA_GPIO_PORT, iic.SDA_GPIO_PIN, (data & 0x80) >> 7);
        Delay(iic.delay);
        I2C_SCL_H(iic);
        Delay(iic.delay);
        I2C_SCL_L(iic);
        data <<= 1; /* 左移1位,用于下一次发送 */
    }
    I2C_SDA_H(iic);
}

uint8_t custom_iic_read_byte(customIIC iic, uint8_t ack)
{
    uint8_t i, receive = 0;

    for (i = 0; i < 8; i++) /* 接收1个字节数据 */
    {
        receive <<= 1; /* 高位先输出,所以先收到的数据位要左移 */
        I2C_SCL_H(iic);
        Delay(iic.delay);
        if (I2C_SDA_Read(iic)) {
            receive++;
        }
        I2C_SCL_L(iic);
        Delay(iic.delay);
    }

    if (!ack) {
        custom_iic_nack(iic); /* 发送nACK */
    } else {
        custom_iic_ack(iic); /* 发送ACK */
    }

    return receive;
}

/**
 * @brief Initialize a software I2C descriptor and release the bus lines.
 * @param iic Software I2C descriptor to initialize.
 * @param scl_port GPIO port of SCL.
 * @param scl_pin GPIO pin of SCL.
 * @param sda_port GPIO port of SDA.
 * @param sda_pin GPIO pin of SDA.
 * @param delay Delay count used by the bit-bang timing.
 */
void custom_iic_init(customIIC *iic, GPIO_TypeDef *scl_port, uint16_t scl_pin, GPIO_TypeDef *sda_port, uint16_t sda_pin, uint8_t delay)
{
    if (iic == NULL) {
        return;
    }

    iic->SCL_GPIO_PORT = scl_port;
    iic->SCL_GPIO_PIN = scl_pin;
    iic->SDA_GPIO_PORT = sda_port;
    iic->SDA_GPIO_PIN = sda_pin;
    iic->delay = delay;

    HAL_GPIO_WritePin(iic->SCL_GPIO_PORT, iic->SCL_GPIO_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(iic->SDA_GPIO_PORT, iic->SDA_GPIO_PIN, GPIO_PIN_SET);
}

/**
 * @brief Write a raw byte stream to an I2C device.
 * @param iic Software I2C descriptor.
 * @param addr 8-bit device address with write bit cleared.
 * @param data Data buffer.
 * @param len Number of bytes to write.
 * @return 1 on success, 0 on failure.
 */
uint8_t custom_iic_write(customIIC iic, uint8_t addr, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    custom_iic_start(iic);
    if (!custom_iic_send_addr(iic, addr & 0xFEU)) {
        custom_iic_stop(iic);
        return 0;
    }

    for (i = 0U; i < len; i++) {
        custom_iic_send_byte(iic, data[i]);
        if (!custom_iic_wait_ack(iic)) {
            custom_iic_stop(iic);
            return 0;
        }
    }

    custom_iic_stop(iic);
    return 1;
}

/**
 * @brief Read a raw byte stream from an I2C device.
 * @param iic Software I2C descriptor.
 * @param addr 8-bit device address with write bit cleared.
 * @param data Receive buffer.
 * @param len Number of bytes to read.
 * @return 1 on success, 0 on failure.
 */
uint8_t custom_iic_read(customIIC iic, uint8_t addr, uint8_t *data, uint16_t len)
{
    uint16_t i;

    if ((data == NULL) && (len > 0U)) {
        return 0;
    }

    custom_iic_start(iic);
    if (!custom_iic_send_addr(iic, addr | 0x01U)) {
        custom_iic_stop(iic);
        return 0;
    }

    for (i = 0U; i < len; i++) {
        data[i] = custom_iic_read_byte(iic, (i + 1U) < len);
    }

    custom_iic_stop(iic);
    return 1;
}

/**
 * @brief Perform a write then read transaction with a repeated start.
 * @param iic Software I2C descriptor.
 * @param addr 8-bit device address with write bit cleared.
 * @param tx Write buffer.
 * @param tx_len Number of bytes to write first.
 * @param rx Read buffer.
 * @param rx_len Number of bytes to read after the repeated start.
 * @return 1 on success, 0 on failure.
 */
uint8_t custom_iic_write_read(customIIC iic, uint8_t addr, const uint8_t *tx, uint16_t tx_len, uint8_t *rx, uint16_t rx_len)
{
    uint16_t i;

    custom_iic_start(iic);
    if (!custom_iic_send_addr(iic, addr & 0xFEU)) {
        custom_iic_stop(iic);
        return 0;
    }

    for (i = 0U; i < tx_len; i++) {
        custom_iic_send_byte(iic, tx[i]);
        if (!custom_iic_wait_ack(iic)) {
            custom_iic_stop(iic);
            return 0;
        }
    }

    if ((rx != NULL) && (rx_len > 0U)) {
        custom_iic_start(iic);
        if (!custom_iic_send_addr(iic, addr | 0x01U)) {
            custom_iic_stop(iic);
            return 0;
        }

        for (i = 0U; i < rx_len; i++) {
            rx[i] = custom_iic_read_byte(iic, (i + 1U) < rx_len);
        }
    }

    custom_iic_stop(iic);
    return 1;
}

/**
 * @brief Write device register data.
 * @param iic Software I2C descriptor.
 * @param addr 8-bit device address with write bit cleared.
 * @param reg Register address.
 * @param reg_len Register address width in bytes.
 * @param data Data buffer.
 * @param len Number of bytes to write.
 * @return 1 on success, 0 on failure.
 */
uint8_t custom_iic_write_reg(customIIC iic, uint8_t addr, uint16_t reg, uint8_t reg_len, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    custom_iic_start(iic);
    if (!custom_iic_send_addr(iic, addr & 0xFEU)) {
        custom_iic_stop(iic);
        return 0;
    }
    if (!custom_iic_send_reg(iic, reg, reg_len)) {
        custom_iic_stop(iic);
        return 0;
    }

    for (i = 0U; i < len; i++) {
        custom_iic_send_byte(iic, data[i]);
        if (!custom_iic_wait_ack(iic)) {
            custom_iic_stop(iic);
            return 0;
        }
    }

    custom_iic_stop(iic);
    return 1;
}

/**
 * @brief Read device register data.
 * @param iic Software I2C descriptor.
 * @param addr 8-bit device address with write bit cleared.
 * @param reg Register address.
 * @param reg_len Register address width in bytes.
 * @param data Receive buffer.
 * @param len Number of bytes to read.
 * @return 1 on success, 0 on failure.
 */
uint8_t custom_iic_read_reg(customIIC iic, uint8_t addr, uint16_t reg, uint8_t reg_len, uint8_t *data, uint16_t len)
{
    if ((data == NULL) && (len > 0U)) {
        return 0;
    }

    custom_iic_start(iic);
    if (!custom_iic_send_addr(iic, addr & 0xFEU)) {
        custom_iic_stop(iic);
        return 0;
    }
    if (!custom_iic_send_reg(iic, reg, reg_len)) {
        custom_iic_stop(iic);
        return 0;
    }

    custom_iic_start(iic);
    if (!custom_iic_send_addr(iic, addr | 0x01U)) {
        custom_iic_stop(iic);
        return 0;
    }

    for (uint16_t i = 0U; i < len; i++) {
        data[i] = custom_iic_read_byte(iic, (i + 1U) < len);
    }

    custom_iic_stop(iic);
    return 1;
}
