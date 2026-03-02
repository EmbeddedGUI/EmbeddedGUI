#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

static egui_view_spangroup_t spangroup;

EGUI_VIEW_SPANGROUP_PARAMS_INIT(spangroup_params, 10, 10, 220, 140);

void test_init_ui(void)
{
    egui_view_spangroup_init_with_params(EGUI_VIEW_OF(&spangroup), &spangroup_params);
    egui_view_spangroup_set_line_spacing(EGUI_VIEW_OF(&spangroup), 6);

    egui_view_spangroup_add_span(EGUI_VIEW_OF(&spangroup), "EmbeddedGUI ", (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT);
    egui_view_spangroup_add_span(EGUI_VIEW_OF(&spangroup), "SpanGroup", (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_PRIMARY);
    egui_view_spangroup_add_span(EGUI_VIEW_OF(&spangroup), "  Demo", (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY);
    egui_view_spangroup_add_span(EGUI_VIEW_OF(&spangroup), "  Readable", (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_SUCCESS);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&spangroup));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    if (action_index >= 1)
    {
        return false;
    }
    EGUI_SIM_SET_WAIT(p_action, 1500);
    return true;
}
#endif
