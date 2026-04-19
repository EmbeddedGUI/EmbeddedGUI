#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
#define egui_dirty_region_stats (core->debug.dirty_region_stats)
#endif

void egui_core_draw_view_group(egui_core_t *core, egui_region_t *p_region_dirty, int is_debug_mode)
{
    egui_view_group_t *view_group = (egui_view_group_t *)&core->scene.root_view_group;
    egui_dim_t x, y, x_pos, y_pos;
    egui_dim_t x_pos_base = p_region_dirty->location.x;
    egui_dim_t y_pos_base = p_region_dirty->location.y;
    egui_dim_t width_dirty = p_region_dirty->size.width;
    egui_dim_t height_dirty = p_region_dirty->size.height;
    egui_dim_t pfb_width = core->pfb_width;
    egui_dim_t pfb_height = core->pfb_height;
    egui_dim_t tmp_pfb_width;
    egui_dim_t tmp_pfb_height;
    egui_dim_t pfb_width_count, pfb_height_count;
    uint32_t pfb_total_pixel_count = (uint32_t)core->pfb_total_buffer_size / sizeof(egui_color_int_t);
    uint8_t reverse_x = core->pfb_scan_reverse_x;
    uint8_t reverse_y = core->pfb_scan_reverse_y;

    EGUI_LOG_DBG("region_dirty, x: %d, y: %d, width: %d, height: %d\n", p_region_dirty->location.x, p_region_dirty->location.y, p_region_dirty->size.width,
                 p_region_dirty->size.height);

    egui_core_apply_logical_pfb_probe_shape(core, &pfb_width, &pfb_height, pfb_total_pixel_count);

    // change pfb size to fit the dirty region
    if (pfb_width > width_dirty)
    {
        pfb_width = width_dirty;
        pfb_height = (egui_dim_t)(pfb_total_pixel_count / (uint32_t)pfb_width);
    }
    else if (pfb_height > height_dirty)
    {
        pfb_height = height_dirty;
        pfb_width = (egui_dim_t)(pfb_total_pixel_count / (uint32_t)pfb_height);
    }

    pfb_width_count = (width_dirty + pfb_width - 1) / pfb_width;
    pfb_height_count = (height_dirty + pfb_height - 1) / pfb_height;

    EGUI_LOG_DBG("pfb_update, pfb_width_count: %d, pfb_height_count: %d, pfb_width: %d, pfb_height: %d\n", pfb_width_count, pfb_height_count, pfb_width,
                 pfb_height);

    // start draw
    for (y = 0; y < pfb_height_count; y++)
    {
        egui_dim_t tile_y = reverse_y ? (pfb_height_count - 1 - y) : y;
        y_pos = y_pos_base + tile_y * pfb_height;

        for (x = 0; x < pfb_width_count; x++)
        {
            egui_dim_t tile_x = reverse_x ? (pfb_width_count - 1 - x) : x;

            x_pos = x_pos_base + tile_x * pfb_width;
            tmp_pfb_width = EGUI_MIN(pfb_width, x_pos_base + width_dirty - x_pos);
            tmp_pfb_height = EGUI_MIN(pfb_height, y_pos_base + height_dirty - y_pos);
            EGUI_LOG_DBG("pfb_region, x_pos: %d, y_pos: %d, pfb_width: %d, pfb_height: %d\n", x_pos, y_pos, tmp_pfb_width, tmp_pfb_height);

            EGUI_REGION_DEFINE(region, x_pos, y_pos, tmp_pfb_width, tmp_pfb_height);

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
            egui_core_dirty_region_stats_count_tile(core, &region);
#endif

            core->pfb = egui_pfb_manager_get_render_buffer(&core->render.pfb_mgr);

            egui_canvas_init(&core->canvas, core, core->pfb, &region);

            int pfb_total_buffer_size = region.size.width * region.size.height * sizeof(egui_color_int_t);
            egui_api_pfb_clear(core->pfb, pfb_total_buffer_size);

            view_group->base.api->draw((egui_view_t *)view_group);
            egui_core_render_tile_debug_decorate(core, p_region_dirty, is_debug_mode);
            egui_core_render_tile_present(core, &region, is_debug_mode);
        }
    }
}
