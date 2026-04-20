#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_display_driver.h"
#include "egui_rotation.h"

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

__EGUI_WEAK__ egui_dim_t egui_core_get_logical_pfb_target_width_hint(egui_core_t *core)
{
    return 0;
}

static egui_dim_t egui_core_get_logical_pfb_probe_width(egui_core_t *core, uint32_t pfb_total_pixel_count)
{
    egui_dim_t target_width = 0;
    int32_t candidate_width;
    egui_dim_t hint_width = egui_core_get_logical_pfb_target_width_hint(core);

#if EGUI_CONFIG_CORE_LOGICAL_PFB_PROBE_ENABLE
    target_width = EGUI_CONFIG_CORE_LOGICAL_PFB_PROBE_TARGET_WIDTH;
#endif
    if (hint_width > target_width)
    {
        target_width = hint_width;
    }

    if (target_width <= core->pfb_width)
    {
        return 0;
    }

    if ((uint32_t)target_width > pfb_total_pixel_count)
    {
        target_width = (egui_dim_t)pfb_total_pixel_count;
    }

    if (target_width > core->screen_width)
    {
        target_width = core->screen_width;
    }

    for (candidate_width = target_width; candidate_width > core->pfb_width; candidate_width--)
    {
        uint32_t candidate_height;

        if ((pfb_total_pixel_count % (uint32_t)candidate_width) != 0U)
        {
            continue;
        }

        candidate_height = pfb_total_pixel_count / (uint32_t)candidate_width;
        if (candidate_height == 0U || candidate_height > (uint32_t)core->pfb_height)
        {
            continue;
        }

        return (egui_dim_t)candidate_width;
    }

    return 0;
}

void egui_core_apply_logical_pfb_probe_shape(egui_core_t *core, egui_dim_t *pfb_width, egui_dim_t *pfb_height, uint32_t pfb_total_pixel_count)
{
    egui_dim_t logical_width = egui_core_get_logical_pfb_probe_width(core, pfb_total_pixel_count);

    if (logical_width <= 0)
    {
        return;
    }

    *pfb_width = logical_width;
    *pfb_height = (egui_dim_t)(pfb_total_pixel_count / (uint32_t)logical_width);
}

void egui_core_set_pfb_scan_direction(egui_core_t *core, uint8_t reverse_x, uint8_t reverse_y)
{
    core->pfb_scan_reverse_x = reverse_x ? 1U : 0U;
    core->pfb_scan_reverse_y = reverse_y ? 1U : 0U;
}

void egui_core_reset_pfb_scan_direction(egui_core_t *core)
{
    egui_core_set_pfb_scan_direction(core, 0U, 0U);
}

uint8_t egui_core_get_pfb_scan_reverse_x(egui_core_t *core)
{
    return core->pfb_scan_reverse_x;
}

uint8_t egui_core_get_pfb_scan_reverse_y(egui_core_t *core)
{
    return core->pfb_scan_reverse_y;
}

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
