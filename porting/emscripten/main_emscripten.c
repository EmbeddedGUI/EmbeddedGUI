#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <emscripten.h>

#include "egui.h"
#include "uicode.h"
#include "sdl_port.h"

static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_BUFFER_COUNT][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

void app_set_gpio(uint8_t pin, uint8_t state)
{
}

#define MAX_PATH 0x1000
char input_file_path[MAX_PATH];

char *pc_get_input_file_path(void)
{
    return input_file_path;
}

static void main_loop_iteration(void)
{
    egui_polling_work();
    VT_sdl_refresh_task();

    if (VT_is_request_quit())
    {
        emscripten_cancel_main_loop();
    }
}

int main(int argc, const char *argv[])
{
    printf("Hello, egui! (WebAssembly)\n");

    // Resource file is preloaded into Emscripten virtual filesystem
    strcpy(input_file_path, "app_egui_resource_merge.bin");

    VT_init();

    extern void egui_port_init(void);
    egui_port_init();

    egui_init(egui_pfb);
    uicode_create_ui();
    egui_screen_on();

    // 0 = requestAnimationFrame (~60fps), 1 = simulate infinite loop
    emscripten_set_main_loop(main_loop_iteration, 0, 1);

    VT_deinit();
    return 0;
}
