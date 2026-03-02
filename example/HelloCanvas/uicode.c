#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

#include "egui_view_canvas.h"

// ViewPage for swiping between canvas pages
static egui_view_viewpage_t viewpage;

// 8 canvas views, one per drawing page
static egui_view_canvas_t canvas_views[EGUI_VIEW_CANVAS_PAGE_COUNT];

// View params
EGUI_VIEW_VIEWPAGE_PARAMS_INIT(viewpage_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

void uicode_init_ui(void)
{
    // Init viewpage
    egui_view_viewpage_init_with_params(EGUI_VIEW_OF(&viewpage), &viewpage_params);

    // Init each canvas view with its page index and add to viewpage
    for (uint8_t i = 0; i < EGUI_VIEW_CANVAS_PAGE_COUNT; i++)
    {
        egui_view_canvas_init_with_page(EGUI_VIEW_OF(&canvas_views[i]), i);
        egui_view_set_size(EGUI_VIEW_OF(&canvas_views[i]), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
        egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&canvas_views[i]));
    }

    // Layout children horizontally
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&viewpage));

    // Add to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage));
}

void uicode_create_ui(void)
{
    uicode_init_ui();
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    // Canvas rendering is CPU-intensive, so swipe events get merged before
    // the UI thread can process them. Use direct page switching instead.
    static int last_action = -1;
    int first_call = (action_index != last_action);
    last_action = action_index;

    if (action_index >= EGUI_VIEW_CANVAS_PAGE_COUNT - 1)
    {
        return false;
    }

    if (first_call)
    {
        egui_view_viewpage_set_current_page(EGUI_VIEW_OF(&viewpage), action_index + 1);
    }

    if (first_call)
        recording_request_snapshot();
    EGUI_SIM_SET_WAIT(p_action, 200);
    return true;
}
#endif
