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

#define MAX_PATH 0x1000
char input_file_path[MAX_PATH];

char* pc_get_input_file_path(void)
{
    return input_file_path;
}

static void pasre_input_params(int argc, const char *argv[])
{
    if (argc > 1)
    {
        // printf("input params: %s\n", argv[1]);
        strcpy(input_file_path, argv[1]);
    }
    else
    {
        strcpy(input_file_path, "app_egui_resource_merge.bin");
    }
}

int main(int argc, const char *argv[])
{
    printf("Hello, egui!\n");
    
    pasre_input_params(argc, argv);

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
