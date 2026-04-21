#include "egui.h"
#include "background/egui_background_color.h"
#include "svg_spec_cases_gen.h"
#include "uicode_disp0.h"

static egui_view_group_t root;
static egui_view_image_t image_view;
static egui_image_svg_t svg_image;
static int current_case_index;

#ifndef HELLO_SVG_SPEC_VIEWPORT_X
#define HELLO_SVG_SPEC_VIEWPORT_X 0
#endif

#ifndef HELLO_SVG_SPEC_VIEWPORT_Y
#define HELLO_SVG_SPEC_VIEWPORT_Y 0
#endif

#ifndef HELLO_SVG_SPEC_VIEWPORT_WIDTH
#define HELLO_SVG_SPEC_VIEWPORT_WIDTH EGUI_CONFIG_SCEEN_WIDTH
#endif

#ifndef HELLO_SVG_SPEC_VIEWPORT_HEIGHT
#define HELLO_SVG_SPEC_VIEWPORT_HEIGHT EGUI_CONFIG_SCEEN_HEIGHT
#endif

EGUI_VIEW_GROUP_PARAMS_INIT(root_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
EGUI_VIEW_IMAGE_PARAMS_INIT(image_view_params, HELLO_SVG_SPEC_VIEWPORT_X, HELLO_SVG_SPEC_VIEWPORT_Y, HELLO_SVG_SPEC_VIEWPORT_WIDTH,
                            HELLO_SVG_SPEC_VIEWPORT_HEIGHT, NULL);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(root_bg_param, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(root_bg_params, &root_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(root_bg, &root_bg_params);

static void hello_svg_spec_apply_case(int case_index)
{
    if (case_index >= HELLO_SVG_SPEC_CASE_COUNT)
    {
        return;
    }

    current_case_index = case_index;
    if (!egui_image_svg_load_memory(&svg_image, hello_svg_spec_cases[case_index].svg_text))
    {
        EGUI_LOG_ERR("failed to load svg case: %s\n", hello_svg_spec_cases[case_index].id);
        return;
    }
    egui_view_invalidate(EGUI_VIEW_OF(&image_view));
}

static void uicode_disp0_init_ui(egui_core_t *core)
{
    egui_view_group_init_with_params(EGUI_VIEW_OF(&root), core, &root_params);
    egui_view_set_background(EGUI_VIEW_OF(&root), EGUI_BG_OF(&root_bg));

    egui_image_svg_init(&svg_image);

    egui_view_image_init_with_params(EGUI_VIEW_OF(&image_view), core, &image_view_params);
    egui_view_image_set_image(EGUI_VIEW_OF(&image_view), (egui_image_t *)&svg_image);
    egui_view_image_set_image_type(EGUI_VIEW_OF(&image_view), EGUI_VIEW_IMAGE_TYPE_RESIZE);

    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&image_view));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&root));

    hello_svg_spec_apply_case(0);
}

void uicode_disp0_init(egui_core_t *core)
{
    uicode_disp0_init_ui(core);
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
#if EGUI_PORT == EGUI_PORT_TYPE_PC
const char *egui_port_get_recording_frame_label(void)
{
    if (current_case_index >= HELLO_SVG_SPEC_CASE_COUNT)
    {
        return NULL;
    }
    return hello_svg_spec_cases[current_case_index].id;
}
#endif

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);

    last_action = action_index;
    if (action_index < 0 || action_index >= HELLO_SVG_SPEC_CASE_COUNT)
    {
        return false;
    }

    if (first_call)
    {
        hello_svg_spec_apply_case(action_index);
        recording_request_snapshot();
    }

    EGUI_SIM_SET_WAIT(p_action, 180);
    return true;
}
#endif
