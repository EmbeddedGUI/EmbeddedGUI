#include "egui_api.h"
#include "egui_canvas.h"
#include "egui_core.h"
#include "egui_core_internal.h"

#if EGUI_DEBUG_MONITOR_SHOW
#define EGUI_DEBUG_MONITOR_OVERLAY_PADDING  3
#define EGUI_DEBUG_MONITOR_OVERLAY_BG_ALPHA 128
#define EGUI_DEBUG_MONITOR_FONT()           ((const egui_font_t *)EGUI_CONFIG_DEBUG_MONITOR_FONT)

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
#define debug_perf_overlay (core->debug.debug_perf_overlay)
#endif

#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
#define debug_mem_overlay (core->debug.debug_mem_overlay)
#endif

static void egui_debug_draw_overlay_for_current_pfb(egui_core_t *core, const egui_debug_overlay_t *overlay)
{
    egui_canvas_t *canvas = &core->canvas;
    egui_region_t overlay_region;
    egui_region_t text_region;
    egui_region_t *pfb_region = egui_canvas_get_pfb_region(canvas);
    EGUI_REGION_DEFINE(region_screen, 0, 0, core->screen_width, core->screen_height);

    if (!egui_debug_get_overlay_region(core, overlay, &overlay_region))
    {
        return;
    }

    if (pfb_region != NULL && !egui_region_is_intersect(&overlay_region, pfb_region))
    {
        return;
    }

    egui_canvas_calc_work_region(canvas, &region_screen);
    egui_canvas_draw_rectangle_fill(canvas, overlay_region.location.x, overlay_region.location.y, overlay_region.size.width, overlay_region.size.height,
                                    EGUI_COLOR_BLACK, EGUI_DEBUG_MONITOR_OVERLAY_BG_ALPHA);

    egui_region_copy(&text_region, &overlay_region);
    if (text_region.size.width <= (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1) || text_region.size.height <= (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1))
    {
        return;
    }

    text_region.location.x += EGUI_DEBUG_MONITOR_OVERLAY_PADDING;
    text_region.location.y += EGUI_DEBUG_MONITOR_OVERLAY_PADDING;
    text_region.size.width -= (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1);
    text_region.size.height -= (EGUI_DEBUG_MONITOR_OVERLAY_PADDING << 1);
    egui_canvas_draw_text_in_rect(canvas, EGUI_DEBUG_MONITOR_FONT(), overlay->text, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_TOP, EGUI_COLOR_WHITE,
                                  EGUI_ALPHA_100);
}

static void egui_debug_commit_overlay_region(egui_core_t *core, egui_debug_overlay_t *overlay)
{
    egui_region_t overlay_region;

    if (overlay == NULL || !egui_debug_get_overlay_region(core, overlay, &overlay_region))
    {
        return;
    }

    egui_region_copy(&overlay->region_last, &overlay_region);
    overlay->region_last_valid = 1;
}

void egui_debug_draw_overlays_for_current_pfb(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_draw_overlay_for_current_pfb(core, &debug_perf_overlay);
#endif
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_debug_draw_overlay_for_current_pfb(core, &debug_mem_overlay);
#endif
}

void egui_debug_commit_overlay_regions(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_commit_overlay_region(core, &debug_perf_overlay);
#endif
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_debug_commit_overlay_region(core, &debug_mem_overlay);
#endif
}
#endif
