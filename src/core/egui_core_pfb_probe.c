#include "egui_core.h"
#include "egui_core_internal.h"

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
