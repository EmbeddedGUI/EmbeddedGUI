
#ifndef __SOFTIIC_H
#define __SOFTIIC_H

#include "main.h"
#include <stdint.h>

typedef struct {
    GPIO_TypeDef *SCL_GPIO_PORT;
    uint16_t SCL_GPIO_PIN;

    GPIO_TypeDef *SDA_GPIO_PORT;
    uint16_t SDA_GPIO_PIN;
    uint8_t delay;
} customIIC;

void custom_iic_start(customIIC iic);
void custom_iic_stop(customIIC iic);
uint8_t custom_iic_wait_ack(customIIC iic);
void custom_iic_ack(customIIC iic);
void custom_iic_nack(customIIC iic);
void custom_iic_send_byte(customIIC iic, uint8_t data);
uint8_t custom_iic_read_byte(customIIC iic, uint8_t ack);

/**
 * @brief Initialize a software I2C descriptor and release the bus lines.
 * @param iic Software I2C descriptor to initialize.
 * @param scl_port GPIO port of SCL.
 * @param scl_pin GPIO pin of SCL.
 * @param sda_port GPIO port of SDA.
 * @param sda_pin GPIO pin of SDA.
 * @param delay Delay count used by the bit-bang timing.
 */
void custom_iic_init(customIIC *iic, GPIO_TypeDef *scl_port, uint16_t scl_pin, GPIO_TypeDef *sda_port, uint16_t sda_pin, uint8_t delay);

/**
 * @brief Write a raw byte stream to an I2C device.
 * @param iic Software I2C descriptor.
 * @param addr 8-bit device address with write bit cleared.
 * @param data Data buffer.
 * @param len Number of bytes to write.
 * @return 1 on success, 0 on failure.
 */
uint8_t custom_iic_write(customIIC iic, uint8_t addr, const uint8_t *data, uint16_t len);

/**
 * @brief Read a raw byte stream from an I2C device.
 * @param iic Software I2C descriptor.
 * @param addr 8-bit device address with write bit cleared.
 * @param data Receive buffer.
 * @param len Number of bytes to read.
 * @return 1 on success, 0 on failure.
 */
uint8_t custom_iic_read(customIIC iic, uint8_t addr, uint8_t *data, uint16_t len);

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
uint8_t custom_iic_write_read(customIIC iic, uint8_t addr, const uint8_t *tx, uint16_t tx_len, uint8_t *rx, uint16_t rx_len);

/**
 * @brief Write device register data.
 * @param iic Software I2C descriptor.
 * @param addr 8-bit device address with write bit cleared.
 * @param reg Register address.
 * @param reg_len Register address width in bytes, usually 1 or 2.
 * @param data Data buffer.
 * @param len Number of bytes to write.
 * @return 1 on success, 0 on failure.
 */
uint8_t custom_iic_write_reg(customIIC iic, uint8_t addr, uint16_t reg, uint8_t reg_len, const uint8_t *data, uint16_t len);

/**
 * @brief Read device register data.
 * @param iic Software I2C descriptor.
 * @param addr 8-bit device address with write bit cleared.
 * @param reg Register address.
 * @param reg_len Register address width in bytes, usually 1 or 2.
 * @param data Receive buffer.
 * @param len Number of bytes to read.
 * @return 1 on success, 0 on failure.
 */
uint8_t custom_iic_read_reg(customIIC iic, uint8_t addr, uint16_t reg, uint8_t reg_len, uint8_t *data, uint16_t len);
/* IIC所有操作函数 */
#endif
