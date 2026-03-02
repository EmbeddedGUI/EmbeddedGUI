
#include <stdio.h>
#include "egui.h"
#include "uicode.h"
#include "app_lcd.h"

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

#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 2
static egui_color_int_t egui_pfb_2[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT] APP_EGUI_CONFIG_PFB_SECTION;
#endif
#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 3
static egui_color_int_t egui_pfb_3[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT] APP_EGUI_CONFIG_PFB_SECTION;
#endif
#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 4
static egui_color_int_t egui_pfb_4[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT] APP_EGUI_CONFIG_PFB_SECTION;
#endif

void port_main(void)
{
    extern void egui_port_init(void);
    egui_port_init();

    egui_init_config_t init_config = {
            .pfb = egui_pfb,
            .pfb_backup = NULL,
    };
    egui_init(&init_config);

    // Add extra PFB buffers for multi-buffer ring queue
#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 2
    egui_pfb_add_buffer(egui_pfb_2);
#endif
#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 3
    egui_pfb_add_buffer(egui_pfb_3);
#endif
#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 4
    egui_pfb_add_buffer(egui_pfb_4);
#endif

    app_lcd_init();

    EGUI_LOG_DBG("Hello EGUI\r\n");

    uicode_create_ui();

    egui_screen_on();

    while (1)
    {
        egui_polling_work();

        app_lcd_polling_work();
    }
}
