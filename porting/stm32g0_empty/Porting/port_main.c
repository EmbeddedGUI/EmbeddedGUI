
#include <stdio.h>
#include "egui.h"
#include "uicode.h"
#include "port_main.h"

// Use GPIOD0-6 for test code use time.
void app_set_gpio(uint8_t pin, uint8_t state)
{
    if (state)
    {
        GPIOD->BSRR = 1 << pin;
    }
    else
    {
        GPIOD->BRR = 1 << pin;
    }
}

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
extern void plaftorm_api_init(void);
static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT] APP_EGUI_CONFIG_PFB_SECTION;
void port_main(void)
{
    egui_init(egui_pfb);

    EGUI_LOG_DBG("Hello EGUI\r\n");

    uicode_create_ui();

    while (1)
    {
        egui_polling_work();
    }
}
