#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

#if EGUI_DEBUG_MONITOR_SHOW
#define EGUI_DEBUG_MONITOR_OVERLAY_PADDING 3
#define EGUI_DEBUG_MONITOR_FONT()          ((const egui_font_t *)EGUI_CONFIG_DEBUG_MONITOR_FONT)

int egui_debug_get_overlay_region(egui_core_t *core, const egui_debug_overlay_t *overlay, egui_region_t *region)
{
    egui_dim_t text_width = 0;
    egui_dim_t text_height = 0;
    egui_dim_t overlay_width;
    egui_dim_t overlay_height;
    egui_dim_t overlay_x;
    egui_dim_t overlay_y;
    const egui_font_t *font = EGUI_DEBUG_MONITOR_FONT();

    if (overlay == NULL || region == NULL || core->screen_width <= 0 || core->screen_height <= 0 || overlay->text[0] == '\0')
    {
        return 0;
    }

    font->api->get_str_size(font, overlay->text, 1, 0, &text_width, &text_height);

    overlay_width = text_width + (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1);
    overlay_height = text_height + (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1);
    overlay_width = EGUI_MIN(overlay_width, (egui_dim_t)core->screen_width);
    overlay_height = EGUI_MIN(overlay_height, (egui_dim_t)core->screen_height);

    if (overlay_width <= 0 || overlay_height <= 0)
    {
        return 0;
    }

    egui_common_align_get_x_y(core->screen_width, core->screen_height, overlay_width, overlay_height, overlay->align_type, &overlay_x, &overlay_y);
    overlay_x += overlay->offset_x;
    overlay_y += overlay->offset_y;

    egui_region_init(region, overlay_x, overlay_y, overlay_width, overlay_height);
    return 1;
}

void egui_debug_mark_overlay_dirty(egui_core_t *core, egui_debug_overlay_t *overlay)
{
    egui_region_t overlay_region;
    egui_region_t refresh_region;
    int has_overlay_region = egui_debug_get_overlay_region(core, overlay, &overlay_region);

    if (overlay == NULL)
    {
        return;
    }

    if (!has_overlay_region && !overlay->region_last_valid)
    {
        return;
    }

    if (overlay->region_last_valid && has_overlay_region)
    {
        egui_region_union(&overlay->region_last, &overlay_region, &refresh_region);
    }
    else if (has_overlay_region)
    {
        egui_region_copy(&refresh_region, &overlay_region);
    }
    else
    {
        egui_region_copy(&refresh_region, &overlay->region_last);
    }

    if (!egui_region_is_empty(&refresh_region))
    {
        egui_core_update_region_dirty(core, &refresh_region);
    }
}
#endif
