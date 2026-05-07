#include "egui.h"
#include "uicode_disp0.h"

#include "egui_view_multitouch_canvas.h"

static egui_view_multitouch_canvas_t touch_canvas;

void uicode_disp0_init(egui_core_t *core)
{
    egui_view_multitouch_canvas_init(EGUI_VIEW_OF(&touch_canvas), core);
    egui_view_set_position(EGUI_VIEW_OF(&touch_canvas), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&touch_canvas), EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&touch_canvas));
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const int multi_start_x[] = {54, 102, 160, 218, 266};
    static const int multi_start_y[] = {78, 150, 100, 174, 82};
    static const int multi_end_x[] = {82, 128, 160, 194, 238};
    static const int multi_end_y[] = {180, 82, 172, 92, 182};
    static const int zoom_out_start_x[] = {110, 210};
    static const int zoom_out_start_y[] = {132, 132};
    static const int zoom_out_end_x[] = {58, 264};
    static const int zoom_out_end_y[] = {92, 176};
    static const int zoom_in_start_x[] = {60, 270};
    static const int zoom_in_start_y[] = {178, 78};
    static const int zoom_in_end_x[] = {138, 184};
    static const int zoom_in_end_y[] = {130, 128};
    static int last_action = -1;
    int first_call = (action_index != last_action);
    last_action = action_index;

    if (first_call)
    {
        recording_request_snapshot();
    }

    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    case 1:
        p_action->type = EGUI_SIM_ACTION_DRAG;
        p_action->x1 = 44;
        p_action->y1 = 82;
        p_action->x2 = 176;
        p_action->y2 = 154;
        p_action->steps = 12;
        p_action->interval_ms = 300;
        p_action->display_id = 0;
        return true;
    case 2:
        EGUI_SIM_SET_MULTI_DRAG(p_action, 5, multi_start_x, multi_start_y, multi_end_x, multi_end_y, 14, 300);
        return true;
    case 3:
        EGUI_SIM_SET_MULTI_DRAG(p_action, 2, zoom_out_start_x, zoom_out_start_y, zoom_out_end_x, zoom_out_end_y, 14, 300);
        return true;
    case 4:
        EGUI_SIM_SET_MULTI_DRAG(p_action, 2, zoom_in_start_x, zoom_in_start_y, zoom_in_end_x, zoom_in_end_y, 12, 300);
        return true;
    default:
        return false;
    }
}
#endif
