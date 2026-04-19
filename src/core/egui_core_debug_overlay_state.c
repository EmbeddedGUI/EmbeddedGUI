#include <string.h>

#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

#if EGUI_DEBUG_MONITOR_SHOW
void egui_debug_overlay_init(egui_debug_overlay_t *overlay, uint8_t align_type, egui_dim_t offset_x, egui_dim_t offset_y)
{
    if (overlay == NULL)
    {
        return;
    }

    overlay->text[0] = '\0';
    egui_region_init_empty(&overlay->region_last);
    overlay->region_last_valid = 0;
    overlay->align_type = align_type;
    overlay->offset_x = offset_x;
    overlay->offset_y = offset_y;
}

uint8_t egui_debug_overlay_set_text(egui_debug_overlay_t *overlay, const char *next_text)
{
    if (overlay == NULL || next_text == NULL)
    {
        return 0;
    }

    if (strcmp(overlay->text, next_text) == 0)
    {
        return 0;
    }

    strcpy(overlay->text, next_text);
    return 1;
}

#endif
