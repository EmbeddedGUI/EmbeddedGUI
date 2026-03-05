#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

char *pc_get_input_file_path(void)
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

static void parse_recording_params(int argc, const char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--headless") == 0)
        {
            sdl_port_set_headless(true);
        }
        if (strcmp(argv[i], "--record") == 0 && i + 3 < argc)
        {
            const char *output_dir = argv[i + 1];
            int fps = atoi(argv[i + 2]);
            int duration = atoi(argv[i + 3]);
            recording_init(output_dir, fps, duration);
        }
        else if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc)
        {
            int speed = atoi(argv[i + 1]);
            recording_set_speed(speed);
        }
        else if (strcmp(argv[i], "--clock-scale") == 0 && i + 1 < argc)
        {
            int clock_scale = atoi(argv[i + 1]);
            recording_set_clock_scale(clock_scale);
        }
        else if (strcmp(argv[i], "--snapshot-settle-ms") == 0 && i + 1 < argc)
        {
            int settle_ms = atoi(argv[i + 1]);
            recording_set_snapshot_settle_ms(settle_ms);
        }
        else if (strcmp(argv[i], "--snapshot-stable-cycles") == 0 && i + 1 < argc)
        {
            int stable_cycles = atoi(argv[i + 1]);
            recording_set_snapshot_stability(stable_cycles, -1);
        }
        else if (strcmp(argv[i], "--snapshot-max-wait-ms") == 0 && i + 1 < argc)
        {
            int max_wait_ms = atoi(argv[i + 1]);
            recording_set_snapshot_stability(-1, max_wait_ms);
        }
    }
}

int main(int argc, const char *argv[])
{
    printf("Hello, egui!\n");

    pasre_input_params(argc, argv);
    parse_recording_params(argc, argv);

    VT_init();

    // Register display and platform drivers before egui_init
    extern void egui_port_init(void);
    egui_port_init();

    egui_init_config_t init_config = {
            .pfb = egui_pfb,
            .pfb_backup = NULL,
    };
    egui_init(&init_config);
    uicode_create_ui();
    egui_screen_on();

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
