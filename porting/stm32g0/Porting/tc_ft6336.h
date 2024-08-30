#ifndef _TC_FT6336_H_
#define _TC_FT6336_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#include "port_main.h"

/* choose a Hardware SPI port to use. */
#define FT6336_SPI_PORT APP_EGUI_TOUCH_I2C
extern I2C_HandleTypeDef FT6336_SPI_PORT;

/* Pin connection*/
#define FT6336_RST_PORT APP_EGUI_TOUCH_RST_PORT
#define FT6336_RST_PIN  APP_EGUI_TOUCH_RST_PIN

#define FT6336_RST_L HAL_GPIO_WritePin(FT6336_RST_PORT, FT6336_RST_PIN, GPIO_PIN_RESET)
#define FT6336_RST_H HAL_GPIO_WritePin(FT6336_RST_PORT, FT6336_RST_PIN, GPIO_PIN_SET)

#define FT6336_ADDR 0X71

// Register definitions
#define FT6336_DEVIDE_MODE    0x00 // FT6336 mode control register
#define FT6336_REG_NUM_FINGER 0x02 // Touch status register

#define FT6336_TP1_REG 0X03 // First touch point data address
#define FT6336_TP2_REG 0X09 // Second touch point data address

#define FT6336_ID_G_CIPHER_MID   0x9F // Chip code (middle byte) Default value 0x26
#define FT6336_ID_G_CIPHER_LOW   0xA0 // Chip code (low byte) 0x01: Ft6336G  0x02: Ft6336U
#define FT6336_ID_G_LIB_VERSION  0xA1 // Version
#define FT6336_ID_G_CIPHER_HIGH  0xA3 // Chip code (high byte) Default 0x64
#define FT6336_ID_G_MODE         0xA4 // FT6636 interrupt mode control register
#define FT6336_ID_G_FOCALTECH_ID 0xA8 // VENDOR ID Default value is 0x11
#define FT6336_ID_G_THGROUP      0x80 // Touch valid value setting register
#define FT6336_ID_G_PERIODACTIVE 0x88 // Active state period setting register

// MATCH VALUE LIST
#define FT6336_PANNEL_ID 0x11

int ft6336_get_touch_point(uint16_t *x, uint16_t *y);
int ft6336_init(void);
uint8_t ft6336_read_reg(uint16_t reg_addr, uint8_t *pData, uint16_t Size);
uint8_t ft6336_write_reg(uint16_t reg_addr, uint8_t *pData, uint16_t Size);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _TC_FT6336_H_ */
