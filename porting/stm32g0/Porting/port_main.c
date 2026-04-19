#include "port_main.h"

#include <stdio.h>

#include "egui.h"
#include "uicode_disp0.h"

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
static egui_core_t core;

extern void egui_port_init(egui_core_t *core);

void port_main(void)
{
    egui_init(&core, egui_pfb);
    egui_port_init(&core);
    extern egui_display_driver_t *egui_port_get_display_driver(void);
    egui_display_driver_register(&core, egui_port_get_display_driver());
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    extern void egui_port_register_touch_driver(egui_core_t * core);
    egui_port_register_touch_driver(&core);
#endif

    EGUI_LOG_DBG("Hello EGUI\r\n");

    uicode_disp0_init(&core);

    egui_screen_on(&core);

    while (1)
    {
        egui_polling_work(&core);
    }
}
