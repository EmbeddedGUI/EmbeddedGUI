#include "port_main.h"
#include <stdio.h>
#include "iwdg.h"
#include "egui.h"
#include "uicode_disp0.h"

/**
 * @brief  Main program
 * @param  None
 * @retval None
 */
extern void plaftorm_api_init(void);
EGUI_CONFIG_PFB_BUFFER_DECLARE(egui_pfb);
static egui_core_t core;

extern void egui_port_init(egui_core_t *core);


void init_all(void)
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
    egui_polling_refresh_display(&core);
    egui_display_set_brightness(&core, 255);
}

void loop_run(void)
{
    while (1)
    {
        egui_polling_work(&core);
        HAL_IWDG_Refresh(&hiwdg);
    }
}

