#include "egui.h"
#include "uicode_disp0.h"

static egui_view_label_t label_1;
static egui_view_button_t button_1;

#define BUTTON_WIDTH  150
#define BUTTON_HEIGHT 50
#define LABEL_WIDTH   150
#define LABEL_HEIGHT  50
#define STACK_SPACING 0

#define STACK_HEIGHT (LABEL_HEIGHT + BUTTON_HEIGHT + STACK_SPACING)
#define LABEL_X      ((EGUI_CONFIG_SCREEN_WIDTH - LABEL_WIDTH) / 2)
#define LABEL_Y      ((EGUI_CONFIG_SCREEN_HEIGHT - STACK_HEIGHT) / 2)
#define BUTTON_X     ((EGUI_CONFIG_SCREEN_WIDTH - BUTTON_WIDTH) / 2)
#define BUTTON_Y     (LABEL_Y + LABEL_HEIGHT + STACK_SPACING)

EGUI_VIEW_LABEL_PARAMS_INIT(label_1_params, LABEL_X, LABEL_Y, LABEL_WIDTH, LABEL_HEIGHT, "Hello World!", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE,
                            EGUI_ALPHA_100);
EGUI_VIEW_BUTTON_PARAMS_INIT(button_1_params, BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "Click me!", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE,
                             EGUI_ALPHA_100);

static const char *button_texts[] = {
        "Click me!",
        "Done 1",
        "Done 2",
        "Done 3",
};

static void button_click_cb(egui_view_t *self)
{
    static uint8_t click_index;
    uint8_t last_index = (uint8_t)(sizeof(button_texts) / sizeof(button_texts[0])) - 1;

    if (click_index < last_index)
    {
        click_index++;
    }

    egui_view_label_set_text(self, button_texts[click_index]);
}

void uicode_disp0_init(egui_core_t *core)
{
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), core, &label_1_params);
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_1), core, &button_1_params);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_1), button_click_cb);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&button_1));
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
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
