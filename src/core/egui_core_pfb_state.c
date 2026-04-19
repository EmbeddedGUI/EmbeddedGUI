#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

egui_color_int_t *egui_core_get_pfb_buffer_ptr(egui_core_t *core)
{
    return core->pfb;
}

void egui_pfb_notify_flush_complete(egui_core_t *core)
{
    egui_pfb_manager_notify_flush_complete(&core->render.pfb_mgr);
}

void egui_pfb_bus_acquire(egui_core_t *core)
{
    egui_pfb_manager_bus_acquire(&core->render.pfb_mgr);
}

void egui_pfb_bus_release(egui_core_t *core)
{
    egui_pfb_manager_bus_release(&core->render.pfb_mgr);
}

void egui_core_pfb_set_buffer(egui_core_t *core, egui_color_int_t *pfb, uint16_t width, uint16_t height)
{
    core->pfb = pfb;
    core->pfb_width = width;
    core->pfb_height = height;
    core->pfb_total_buffer_size = width * height * core->color_bytes;

    // Calculate the number of pixels in each row and column of the PFB
    core->pfb_width_count = (core->screen_width + width - 1) / width;
    core->pfb_height_count = (core->screen_height + height - 1) / height;
}
