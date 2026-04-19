#include "egui_core_touch.h"

#include "egui_canvas.h"
#include "egui_core.h"

void egui_core_touch_draw_trace(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    egui_canvas_t *canvas;
    uint16_t j;
    egui_region_t *pfb_region;
    EGUI_REGION_DEFINE(region_screen, 0, 0, core->screen_width, core->screen_height);

    if (core == NULL)
    {
        return;
    }

    canvas = &core->canvas;
    pfb_region = egui_canvas_get_pfb_region(canvas);

    if (core->touch.trace_record.point_count == 0)
    {
        return;
    }

    egui_canvas_calc_work_region(canvas, &region_screen);

    if (pfb_region != NULL && !egui_region_is_intersect(&core->touch.trace_record.bounds, pfb_region))
    {
        return;
    }

    if (core->touch.trace_record.point_count == 1)
    {
        egui_canvas_draw_point(canvas, core->touch.trace_record.points[0].x, core->touch.trace_record.points[0].y, EGUI_COLOR_RED, EGUI_ALPHA_100);
        return;
    }

    for (j = 1; j < core->touch.trace_record.point_count; j++)
    {
        egui_canvas_draw_line(canvas, core->touch.trace_record.points[j - 1U].x, core->touch.trace_record.points[j - 1U].y,
                              core->touch.trace_record.points[j].x, core->touch.trace_record.points[j].y, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);
    }
#else
    EGUI_UNUSED(core);
#endif
}
