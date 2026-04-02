#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"
#include "size_analysis_probe_config.h"

#if defined(__GNUC__)
#define EGUI_SIZE_PROBE_FUNC static void __attribute__((unused))
#else
#define EGUI_SIZE_PROBE_FUNC static void
#endif

static egui_view_label_t label_1;
static egui_view_button_t button_1;
static egui_view_linearlayout_t layout_1;

#define BUTTON_WIDTH  150
#define BUTTON_HEIGHT 50

#define LABEL_WIDTH  150
#define LABEL_HEIGHT 50

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

#if defined(EGUI_SIZE_PROBE_LINK_LINE_HQ) || defined(EGUI_SIZE_PROBE_LINK_CIRCLE_HQ) || defined(EGUI_SIZE_PROBE_LINK_ARC_HQ) || \
    defined(EGUI_SIZE_PROBE_LINK_ARC_ROUND_CAP_HQ)
static volatile int g_uicode_size_probe_runtime_enable = 0;

EGUI_SIZE_PROBE_FUNC uicode_size_probe_link_hq_paths(void)
{
    static const egui_dim_t probe_polyline[] = {
            0, 0, 8, 8, 16, 0,
    };

    if (!g_uicode_size_probe_runtime_enable)
    {
        return;
    }

#if defined(EGUI_SIZE_PROBE_LINK_LINE_HQ)
    egui_canvas_draw_line_hq(0, 0, 16, 16, 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_line_round_cap_hq(0, 0, 16, 16, 4, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_polyline_hq(probe_polyline, 3, 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_polyline_round_cap_hq(probe_polyline, 3, 4, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
#endif

#if defined(EGUI_SIZE_PROBE_LINK_CIRCLE_HQ)
    egui_canvas_draw_circle_hq(16, 16, 8, 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill_hq(16, 16, 8, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
#endif

#if defined(EGUI_SIZE_PROBE_LINK_ARC_HQ)
    egui_canvas_draw_arc_hq(16, 16, 10, 0, 90, 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_canvas_draw_arc_fill_hq(16, 16, 10, 0, 90, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
#endif

#if defined(EGUI_SIZE_PROBE_LINK_ARC_ROUND_CAP_HQ)
    egui_canvas_draw_arc_round_cap_hq(16, 16, 12, 0, 120, 4, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
#endif
}
#endif

void uicode_init_ui(void)
{
#if defined(EGUI_SIZE_PROBE_LINK_LINE_HQ) || defined(EGUI_SIZE_PROBE_LINK_CIRCLE_HQ) || defined(EGUI_SIZE_PROBE_LINK_ARC_HQ) || \
    defined(EGUI_SIZE_PROBE_LINK_ARC_ROUND_CAP_HQ)
    uicode_size_probe_link_hq_paths();
#endif

    egui_view_linearlayout_init_with_params(EGUI_VIEW_OF(&layout_1), &layout_1_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), &label_1_params);
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_1), &button_1_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&button_1), button_str);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_1), button_click_cb);

    egui_view_group_add_child(EGUI_VIEW_OF(&layout_1), EGUI_VIEW_OF(&label_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&layout_1), EGUI_VIEW_OF(&button_1));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&layout_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&layout_1));
}

void uicode_create_ui(void)
{
    uicode_init_ui();
}

#if EGUI_CONFIG_RECORDING_TEST
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
