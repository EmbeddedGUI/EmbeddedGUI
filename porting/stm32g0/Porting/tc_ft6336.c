#include "tc_ft6336.h"

static void ft6336_reset(void)
{
    FT6336_RST_L;
    HAL_Delay(20);
    FT6336_RST_H;
    HAL_Delay(300);
}

uint8_t ft6336_write_reg(uint16_t reg_addr, uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Write(&FT6336_SPI_PORT, FT6336_ADDR, reg_addr, I2C_MEMADD_SIZE_8BIT, data, size, 1000);
    return status == HAL_OK;
}

uint8_t ft6336_read_reg(uint16_t reg_addr, uint8_t *data, uint16_t size)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Read(&FT6336_SPI_PORT, FT6336_ADDR, reg_addr, I2C_MEMADD_SIZE_8BIT, data, size, 1000);

    return status == HAL_OK;
}

int ft6336_init(void)
{
    uint8_t id;

    ft6336_reset();
    // read chip id
    ft6336_read_reg(FT6336_ID_G_FOCALTECH_ID, &id, 1);
    if (id != FT6336_PANNEL_ID)
    {
        return 0;
    }
    return 1;
}

int ft6336_get_touch_point(uint16_t *x, uint16_t *y)
{
    uint8_t buf[4];
    uint8_t num = 0;
    // read number of touch points
    ft6336_read_reg(FT6336_REG_NUM_FINGER, &num, sizeof(num));

    if (num)
    {
        // only get one point.
        ft6336_read_reg(FT6336_TP1_REG, buf, 4);
        *x = ((uint16_t)(buf[0] & 0X0F) << 8) + buf[1];
        *y = ((uint16_t)(buf[2] & 0X0F) << 8) + buf[3];
    }

    return num;
}
