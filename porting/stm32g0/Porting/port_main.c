#include "port_main.h"

#include <stdio.h>

#include "egui.h"
#include "uicode.h"

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
EGUI_CONFIG_PFB_BUFFER_DECLARE(egui_pfb);

extern void egui_port_init(void);

void port_main(void)
{
    egui_port_init();

    egui_init(egui_pfb);

    EGUI_LOG_DBG("Hello EGUI\r\n");

    uicode_create_ui();

    egui_screen_on();

    while (1)
    {
        egui_polling_work();
    }
}
