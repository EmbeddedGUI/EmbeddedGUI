#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

void egui_core_clear_screen(egui_core_t *core)
{
    int16_t screen_w = egui_display_get_width(core);
    int16_t screen_h = egui_display_get_height(core);
    int16_t pfb_w = core->pfb_width;
    int16_t pfb_h = core->pfb_height;

    // Clear PFB buffer to black
    egui_api_pfb_clear(core->pfb, core->pfb_total_buffer_size);

    // Send black tiles to cover entire screen
    for (int16_t y = 0; y < screen_h; y += pfb_h)
    {
        for (int16_t x = 0; x < screen_w; x += pfb_w)
        {
            int16_t w = (x + pfb_w > screen_w) ? (screen_w - x) : pfb_w;
            int16_t h = (y + pfb_h > screen_h) ? (screen_h - y) : pfb_h;
            EGUI_REGION_DEFINE(region, x, y, w, h);

            if (core->render.pfb_mgr.buffer_count > 1)
            {
                core->pfb = egui_pfb_manager_get_render_buffer(&core->render.pfb_mgr);
                // Clear PFB buffer to black
                egui_api_pfb_clear(core->pfb, core->pfb_total_buffer_size);
            }

            egui_core_draw_data(core, &region);
        }
    }

    egui_pfb_manager_wait_all_complete(&core->render.pfb_mgr);
}
