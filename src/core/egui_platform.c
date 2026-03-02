#include "egui_platform.h"

static egui_platform_t *g_platform = NULL;

void egui_platform_register(egui_platform_t *platform)
{
    g_platform = platform;
}

egui_platform_t *egui_platform_get(void)
{
    return g_platform;
}
