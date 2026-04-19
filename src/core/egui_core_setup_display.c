#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

void egui_setup_display(egui_core_t *core, const egui_display_setup_t *setup)
{
    EGUI_ASSERT(core != NULL);
    EGUI_ASSERT(setup != NULL);
    EGUI_ASSERT(setup->pfb_buffers != NULL);
    EGUI_ASSERT(setup->pfb_buffer_count > 0);
    EGUI_ASSERT(setup->display_driver != NULL);
    EGUI_ASSERT(setup->platform != NULL);

    egui_init_display(core, (int16_t)setup->screen_width, (int16_t)setup->screen_height, setup->pfb_buffers, setup->pfb_buffer_count, setup->pfb_width,
                      setup->pfb_height);
    core->id = setup->display_id;
    egui_core_setup_display_start(core, setup);
}
