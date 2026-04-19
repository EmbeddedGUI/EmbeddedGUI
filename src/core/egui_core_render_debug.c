#include "egui_api.h"
#include "egui_canvas.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "egui_core_touch.h"

void egui_core_render_tile_debug_decorate(egui_core_t *core, const egui_region_t *p_region_dirty, int is_debug_mode)
{
#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
    if (is_debug_mode)
    {
        egui_canvas_t *canvas = &core->canvas;
        egui_region_t *p_region;
        // change to screen coordinate
        EGUI_REGION_DEFINE(region_screen, 0, 0, core->screen_width, core->screen_height);
        egui_canvas_calc_work_region(canvas, &region_screen);

#if EGUI_CONFIG_DEBUG_PFB_REFRESH
        p_region = egui_canvas_get_pfb_region(canvas);
        egui_canvas_draw_rectangle(canvas, p_region->location.x, p_region->location.y, p_region->size.width, p_region->size.height, 1, EGUI_COLOR_RED,
                                   EGUI_ALPHA_100);
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
        p_region = (egui_region_t *)p_region_dirty;
        egui_canvas_draw_rectangle(canvas, p_region->location.x, p_region->location.y, p_region->size.width, p_region->size.height, 2, EGUI_COLOR_BLUE,
                                   EGUI_ALPHA_100);
#endif // EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
    }
#else
    EGUI_UNUSED(core);
    EGUI_UNUSED(p_region_dirty);
    EGUI_UNUSED(is_debug_mode);
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH

#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    egui_core_touch_draw_trace(core);
#endif

#if EGUI_DEBUG_MONITOR_SHOW
    egui_debug_draw_overlays_for_current_pfb(core);
#endif
}

void egui_core_render_tile_present(egui_core_t *core, egui_region_t *region, int is_debug_mode)
{
    egui_core_draw_data(core, region);

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
    if (is_debug_mode)
    {
        egui_api_delay_core(core, EGUI_CONFIG_DEBUG_REFRESH_DELAY);
        egui_api_refresh_display(core);
    }
#else
    EGUI_UNUSED(is_debug_mode);
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
}
