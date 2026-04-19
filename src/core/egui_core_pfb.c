#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_display_driver.h"
#include "egui_rotation.h"

void egui_core_draw_data(egui_core_t *core, egui_region_t *p_region)
{
    int16_t x = p_region->location.x;
    int16_t y = p_region->location.y;
    int16_t w = p_region->size.width;
    int16_t h = p_region->size.height;
    const egui_color_int_t *data = core->pfb;

    // Runtime software rotation (replaces compile-time EGUI_CONFIG_SOFTWARE_ROTATION)
    if (core->render.software_rotation)
    {
        egui_display_driver_t *drv = egui_display_driver_get(core);
        // Apply software rotation if hardware doesn't support it
        if (drv != NULL && drv->rotation != EGUI_DISPLAY_ROTATION_0 && drv->ops->set_rotation == NULL)
        {
            egui_color_int_t *scratch = core->render.rotation_scratch;
            if (scratch != NULL)
            {
                int scratch_size = core->pfb_width * core->pfb_height;
                egui_rotation_transform_pfb(drv->rotation, drv->physical_width, drv->physical_height, &x, &y, &w, &h, data, scratch, scratch_size);
                egui_memcpy((void *)data, scratch, w * h * sizeof(egui_color_int_t));
            }
        }
    }

    // Submit to PFB ring buffer manager.
    // Single buffer: synchronous draw.
    // Multi-buffer: async DMA with ring queue.
    egui_pfb_manager_submit(&core->render.pfb_mgr, x, y, w, h, data);
}
