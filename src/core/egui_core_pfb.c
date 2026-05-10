#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_display_driver.h"
#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
#include "egui_rotation.h"
#endif

/**
 * @file egui_core_pfb.c
 * @brief Core-facing wrappers around the PFB buffer, scan policy, and flush submission path.
 */

/** Return the raw PFB pointer currently bound to the core. */
egui_color_int_t *egui_core_get_pfb_buffer_ptr(egui_core_t *core)
{
    return core->pfb;
}

/** Forward a display flush-complete notification into the PFB manager. */
void egui_pfb_notify_flush_complete(egui_core_t *core)
{
    egui_pfb_manager_notify_flush_complete(&core->render.pfb_mgr);
}

/** Acquire the shared PFB/display bus through the PFB manager. */
void egui_pfb_bus_acquire(egui_core_t *core)
{
    egui_pfb_manager_bus_acquire(&core->render.pfb_mgr);
}

/** Release the shared PFB/display bus through the PFB manager. */
void egui_pfb_bus_release(egui_core_t *core)
{
    egui_pfb_manager_bus_release(&core->render.pfb_mgr);
}

/** Bind the primary PFB buffer and precompute how many tiles cover the current screen. */
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

/** Weak extension hook that lets ports or applications suggest a preferred logical probe width. */
#if defined(_MSC_VER)
egui_dim_t egui_core_get_logical_pfb_target_width_hint_default(egui_core_t *core)
#else
__EGUI_WEAK__ egui_dim_t egui_core_get_logical_pfb_target_width_hint(egui_core_t *core)
#endif
{
    EGUI_UNUSED(core);
    return 0;
}
__EGUI_MSVC_ALTERNATE_NAME(egui_core_get_logical_pfb_target_width_hint, egui_core_get_logical_pfb_target_width_hint_default)

/** Pick a wider logical probe width when the same pixel budget can form a useful alternative tile shape. */
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

        // Only exact factorizations are accepted so the logical probe still covers the same total pixel count.
        if ((pfb_total_pixel_count % (uint32_t)candidate_width) != 0U)
        {
            continue;
        }

        candidate_height = pfb_total_pixel_count / (uint32_t)candidate_width;
        // Keep the logical probe inside the real PFB height budget.
        if (candidate_height == 0U || candidate_height > (uint32_t)core->pfb_height)
        {
            continue;
        }

        return (egui_dim_t)candidate_width;
    }

    return 0;
}

/** Apply the optional logical probe shape chosen by `egui_core_get_logical_pfb_probe_width()`. */
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

/** Store whether the PFB scan order should be reversed on each axis. */
void egui_core_set_pfb_scan_direction(egui_core_t *core, uint8_t reverse_x, uint8_t reverse_y)
{
    core->pfb_scan_reverse_x = reverse_x ? 1U : 0U;
    core->pfb_scan_reverse_y = reverse_y ? 1U : 0U;
}

/** Restore the default left-to-right, top-to-bottom PFB scan direction. */
void egui_core_reset_pfb_scan_direction(egui_core_t *core)
{
    egui_core_set_pfb_scan_direction(core, 0U, 0U);
}

/** Return non-zero when the current PFB scan direction is reversed on X. */
uint8_t egui_core_get_pfb_scan_reverse_x(egui_core_t *core)
{
    return core->pfb_scan_reverse_x;
}

/** Return non-zero when the current PFB scan direction is reversed on Y. */
uint8_t egui_core_get_pfb_scan_reverse_y(egui_core_t *core)
{
    return core->pfb_scan_reverse_y;
}

/**
 * Submit the just-rendered PFB tile to the display path.
 * When software rotation owns the transform, the tile is rotated into the
 * scratch buffer first and then handed to the PFB manager.
 */
void egui_core_draw_data(egui_core_t *core, egui_region_t *p_region)
{
    int16_t x = p_region->location.x;
    int16_t y = p_region->location.y;
    int16_t w = p_region->size.width;
    int16_t h = p_region->size.height;
    const egui_color_int_t *data = core->pfb;

#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
    // Runtime software rotation (replaces compile-time EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE)
    if (core->render.software_rotation)
    {
        egui_display_driver_t *drv = egui_display_driver_get(core);
        // Only rotate in software when the display driver does not expose a hardware rotation callback.
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
#endif

    // The PFB manager hides whether this becomes a synchronous flush or an async queued transfer.
    egui_pfb_manager_submit(&core->render.pfb_mgr, x, y, w, h, data);
}
