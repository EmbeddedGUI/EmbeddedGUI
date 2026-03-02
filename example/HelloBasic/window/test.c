#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

static egui_view_window_t window_1;
static egui_view_label_t content_label_1;
static egui_view_label_t content_label_2;

EGUI_VIEW_WINDOW_PARAMS_INIT(window_1_params, 10, 10, 220, 200, 30, "My Window");
EGUI_VIEW_LABEL_PARAMS_INIT(content_label_1_params, 10, 10, 200, 30, "Primary content line", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(content_label_2_params, 10, 50, 200, 30, "Secondary content line", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY,
                            EGUI_ALPHA_100);

void test_init_ui(void)
{
    // Init window
    egui_view_window_init_with_params(EGUI_VIEW_OF(&window_1), &window_1_params);

    // Init content labels
    egui_view_label_init_with_params(EGUI_VIEW_OF(&content_label_1), &content_label_1_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&content_label_2), &content_label_2_params);

    // Add content to window
    egui_view_window_add_content(EGUI_VIEW_OF(&window_1), EGUI_VIEW_OF(&content_label_1));
    egui_view_window_add_content(EGUI_VIEW_OF(&window_1), EGUI_VIEW_OF(&content_label_2));

    // Add window to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&window_1));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    // No special actions needed, just display the window
    return false;
}
#endif
