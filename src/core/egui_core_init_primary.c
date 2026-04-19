#include <string.h>

#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

void egui_init(egui_core_t *core, egui_color_int_t pfb[][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT])
{
    egui_color_int_t *pfb_bufs[EGUI_CONFIG_PFB_BUFFER_COUNT];
    int i;

    for (i = 0; i < EGUI_CONFIG_PFB_BUFFER_COUNT; i++)
    {
        pfb_bufs[i] = pfb[i];
    }

    egui_init_display(core, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, pfb_bufs, EGUI_CONFIG_PFB_BUFFER_COUNT, EGUI_CONFIG_PFB_WIDTH,
                      EGUI_CONFIG_PFB_HEIGHT);
}
