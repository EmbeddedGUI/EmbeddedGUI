#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"

void egui_core_set_pfb_scan_direction(egui_core_t *core, uint8_t reverse_x, uint8_t reverse_y)
{
    core->pfb_scan_reverse_x = reverse_x ? 1U : 0U;
    core->pfb_scan_reverse_y = reverse_y ? 1U : 0U;
}

void egui_core_reset_pfb_scan_direction(egui_core_t *core)
{
    egui_core_set_pfb_scan_direction(core, 0U, 0U);
}

uint8_t egui_core_get_pfb_scan_reverse_x(egui_core_t *core)
{
    return core->pfb_scan_reverse_x;
}

uint8_t egui_core_get_pfb_scan_reverse_y(egui_core_t *core)
{
    return core->pfb_scan_reverse_y;
}
