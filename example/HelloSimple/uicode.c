#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"

// views in root
static egui_view_label_t label_1;
static egui_view_button_t button_1;
static egui_view_linearlayout_t layout_1;

#define BUTTON_WIDTH  150
#define BUTTON_HEIGHT 50

#define LABEL_WIDTH  150
#define LABEL_HEIGHT 50

// View params
EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT(layout_1_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, EGUI_ALIGN_CENTER);
EGUI_VIEW_LABEL_PARAMS_INIT(label_1_params, 0, 0, LABEL_WIDTH, LABEL_HEIGHT, "Hello World!", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(button_1_params, 0, 0, BUTTON_WIDTH, BUTTON_HEIGHT, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

static char button_str[20] = "Click me!";
static void button_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("Clicked\n");

    static uint32_t cnt = 1;
    egui_api_sprintf(button_str, "Done %d", cnt);
    EGUI_LOG_INF("button_str: %s\n", button_str);

    egui_view_label_set_text((egui_view_t *)self, button_str);
    cnt++;
}

void uicode_init_ui(void)
{
    // Init all views
    // layout_1
    egui_view_linearlayout_init_with_params(EGUI_VIEW_OF(&layout_1), &layout_1_params);

    // label_1
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), &label_1_params);

    // button_1
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_1), &button_1_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&button_1), button_str);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_1), button_click_cb);

    // Add childs to layout_1
    egui_view_group_add_child(EGUI_VIEW_OF(&layout_1), EGUI_VIEW_OF(&label_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&layout_1), EGUI_VIEW_OF(&button_1));

    // Re-layout childs
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&layout_1));

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&layout_1));
}

void uicode_create_ui(void)
{
    uicode_init_ui();
}

#if EGUI_CONFIG_RECORDING_TEST
// Custom actions for GIF recording - click button 3 times
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    if (action_index >= 3)
    {
        return false;
    }

    EGUI_SIM_SET_CLICK_VIEW(p_action, &button_1, 1000);
    return true;
}
#endif
