#include <stdio.h>
#include <string.h>

#include "egui.h"
#include "uicode.h"

#include "sdl_port.h"

int egui_main_thread(void *argument)
{
    while (1)
    {
        egui_polling_work();

        // avoid cpu high usage
        // Sleep(1);
    }
    return -1;
}

void app_set_gpio(uint8_t pin, uint8_t state)
{
}

static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

int main(int argc, char *argv[])
{
    printf("Hello, egui!\n");

    VT_init();

    egui_init(egui_pfb);
    uicode_create_ui();

    SDL_CreateThread(egui_main_thread, "egui thread", NULL);

    while (1)
    {
        VT_sdl_refresh_task();
        if (VT_is_request_quit())
        {
            break;
        }
    }

    VT_deinit();
    return 0;
}