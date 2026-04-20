#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <emscripten.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "sdl_port.h"

egui_core_t core;
static egui_core_t *g_registered_core = NULL;

EGUI_CONFIG_PFB_BUFFER_DECLARE(egui_pfb);

void app_set_gpio(uint8_t pin, uint8_t state)
{
}

#define MAX_PATH 0x1000
char input_file_path[MAX_PATH];

char *pc_get_input_file_path(void)
{
    return input_file_path;
}

void egui_port_register_core(egui_core_t *core_inst)
{
    EGUI_ASSERT(core_inst != NULL);
    EGUI_ASSERT(core_inst == &core);
    g_registered_core = core_inst;
}

egui_core_t *egui_port_get_core_by_display_id(int display_id)
{
    if (display_id != 0)
    {
        return NULL;
    }
    return g_registered_core;
}

int egui_port_post_core_task(egui_core_t *core_inst, egui_port_core_task_func_t task_func, uintptr_t user_data)
{
    return egui_port_post_core_task_named(core_inst, task_func, user_data, "wasm_post");
}

int egui_port_post_core_task_named(egui_core_t *core_inst, egui_port_core_task_func_t task_func, uintptr_t user_data, const char *context)
{
    EGUI_UNUSED(context);

    if (core_inst == NULL || task_func == NULL)
    {
        return 0;
    }

    task_func(core_inst, user_data);
    return 1;
}

int egui_port_post_core_task_sync(egui_core_t *core_inst, egui_port_core_task_func_t task_func, uintptr_t user_data, uint32_t timeout_ms)
{
    return egui_port_post_core_task_sync_named(core_inst, task_func, user_data, timeout_ms, "wasm_post_sync");
}

int egui_port_post_core_task_sync_named(egui_core_t *core_inst, egui_port_core_task_func_t task_func, uintptr_t user_data, uint32_t timeout_ms,
                                        const char *context)
{
    EGUI_UNUSED(timeout_ms);
    return egui_port_post_core_task_named(core_inst, task_func, user_data, context);
}

static void main_loop_iteration(void)
{
    egui_polling_work(&core);
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

    egui_init(&core, egui_pfb);
    egui_port_register_core(&core);
    egui_port_init();
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    extern void egui_port_register_touch_driver(egui_core_t * core);
    egui_port_register_touch_driver(&core);
#endif
    extern egui_display_driver_t *egui_port_get_display_driver(void);
    extern egui_platform_t *egui_port_get_platform(void);
    egui_platform_register(&core, egui_port_get_platform());
    egui_display_driver_register(&core, egui_port_get_display_driver());
    uicode_disp0_init(&core);
    egui_screen_on(&core);

    // 0 = requestAnimationFrame (~60fps), 1 = simulate infinite loop
    emscripten_set_main_loop(main_loop_iteration, 0, 1);

    VT_deinit();
    return 0;
}
