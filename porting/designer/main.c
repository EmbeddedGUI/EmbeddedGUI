#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "designer_ipc.h"

extern void egui_port_init(void);
extern void designer_fb_to_rgb888(void);
extern uint8_t *designer_get_rgb888(void);
extern uint32_t designer_get_rgb888_size(void);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
extern void designer_touch_set_state(uint8_t pressed, int16_t x, int16_t y);
#endif

static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_BUFFER_COUNT][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

#define MAX_PATH_LEN 0x1000
char input_file_path[MAX_PATH_LEN];

char *pc_get_input_file_path(void)
{
    return input_file_path;
}

void app_set_gpio(uint8_t pin, uint8_t state)
{
    EGUI_UNUSED(pin);
    EGUI_UNUSED(state);
}

int main(int argc, const char *argv[])
{
    /* Parse resource path */
    if (argc > 1)
    {
        strcpy(input_file_path, argv[1]);
    }
    else
    {
        strcpy(input_file_path, "app_egui_resource_merge.bin");
    }

    /* Init IPC (binary mode, unbuffered stdout) */
    designer_ipc_init();

    /* Init EGUI */
    egui_port_init();
    egui_init(egui_pfb);
    uicode_create_ui();
    egui_screen_on();

    /* Do initial render */
    egui_polling_work();

    /* Signal ready to Python */
    designer_ipc_send_ready();

    /* Main command loop */
    uint8_t payload[16];
    int payload_len;
    while (1)
    {
        uint8_t cmd = designer_ipc_read_cmd(payload, &payload_len);
        if (cmd == 0 || cmd == DESIGNER_CMD_QUIT)
        {
            break;
        }

        switch (cmd)
        {
        case DESIGNER_CMD_RENDER:
            egui_polling_work();
            designer_fb_to_rgb888();
            designer_ipc_send_frame(designer_get_rgb888(), designer_get_rgb888_size());
            break;

        case DESIGNER_CMD_TOUCH:
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
            uint8_t action = payload[0];
            uint16_t x = payload[1] | (payload[2] << 8);
            uint16_t y = payload[3] | (payload[4] << 8);
            uint8_t pressed = (action != DESIGNER_TOUCH_UP);
            designer_touch_set_state(pressed, (int16_t)x, (int16_t)y);
#endif
            break;
        }

        case DESIGNER_CMD_KEY:
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
            uint8_t type = payload[0];
            uint16_t code = payload[1] | (payload[2] << 8);
            egui_input_add_key(type, code, 0, 0);
#endif
            break;
        }
        }
    }

    return 0;
}
