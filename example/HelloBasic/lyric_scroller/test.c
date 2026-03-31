#include "egui.h"

#include "uicode.h"

static egui_view_label_t title_label;
static egui_view_label_t hint_label;
static egui_view_lyric_scroller_t lyric_static;
static egui_view_lyric_scroller_t lyric_dynamic;
static egui_timer_t lyric_swap_timer;

static const char *s_dynamic_lines[] = {
        "We are glowing in the midnight signal line.",
        "Every chorus reset should start the lyric from the left edge.",
        "Small screens still need the song title to stay readable.",
};

static uint8_t s_dynamic_line_index = 0;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_title_param, EGUI_COLOR_MAKE(18, 28, 44), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_title_params, &bg_title_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_title, &bg_title_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_scroller_1_param, EGUI_COLOR_MAKE(28, 45, 68), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_scroller_1_params, &bg_scroller_1_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_scroller_1, &bg_scroller_1_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_scroller_2_param, EGUI_COLOR_MAKE(46, 30, 62), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_scroller_2_params, &bg_scroller_2_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_scroller_2, &bg_scroller_2_params);

EGUI_VIEW_LABEL_PARAMS_INIT(title_label_params, 0, 12, EGUI_CONFIG_SCEEN_WIDTH, 24, "Lyric Scroller", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE,
                            EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(hint_label_params, 12, 54, EGUI_CONFIG_SCEEN_WIDTH - 24, 24, "Bottom line updates every 1.8s and restarts scrolling.",
                            EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);
EGUI_VIEW_LYRIC_SCROLLER_PARAMS_INIT(lyric_static_params, 12, 102, EGUI_CONFIG_SCEEN_WIDTH - 24, 28,
                                     "Scrolling lyrics can bounce left and right while keeping the text clipped inside its own viewport.",
                                     EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_VIEW_LYRIC_SCROLLER_PARAMS_INIT(lyric_dynamic_params, 12, 152, EGUI_CONFIG_SCEEN_WIDTH - 24, 28, "We are glowing in the midnight signal line.",
                                     EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

static void lyric_dynamic_swap_callback(egui_timer_t *timer)
{
    EGUI_UNUSED(timer);

    s_dynamic_line_index = (uint8_t)((s_dynamic_line_index + 1) % EGUI_ARRAY_SIZE(s_dynamic_lines));
    egui_view_lyric_scroller_set_text(EGUI_VIEW_OF(&lyric_dynamic), s_dynamic_lines[s_dynamic_line_index]);
}

void test_init_ui(void)
{
    egui_view_label_init_with_params(EGUI_VIEW_OF(&title_label), &title_label_params);
    egui_view_set_background(EGUI_VIEW_OF(&title_label), EGUI_BG_OF(&bg_title));

    egui_view_label_init_with_params(EGUI_VIEW_OF(&hint_label), &hint_label_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&hint_label), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);

    egui_view_lyric_scroller_init_with_params(EGUI_VIEW_OF(&lyric_static), &lyric_static_params);
    egui_view_set_padding(EGUI_VIEW_OF(&lyric_static), 8, 8, 0, 0);
    egui_view_set_background(EGUI_VIEW_OF(&lyric_static), EGUI_BG_OF(&bg_scroller_1));
    egui_view_lyric_scroller_set_interval_ms(EGUI_VIEW_OF(&lyric_static), 50);
    egui_view_lyric_scroller_set_pause_duration_ms(EGUI_VIEW_OF(&lyric_static), 360);

    egui_view_lyric_scroller_init_with_params(EGUI_VIEW_OF(&lyric_dynamic), &lyric_dynamic_params);
    egui_view_set_padding(EGUI_VIEW_OF(&lyric_dynamic), 8, 8, 0, 0);
    egui_view_set_background(EGUI_VIEW_OF(&lyric_dynamic), EGUI_BG_OF(&bg_scroller_2));
    egui_view_lyric_scroller_set_interval_ms(EGUI_VIEW_OF(&lyric_dynamic), 50);
    egui_view_lyric_scroller_set_pause_duration_ms(EGUI_VIEW_OF(&lyric_dynamic), 280);

    egui_timer_init_timer(&lyric_swap_timer, NULL, lyric_dynamic_swap_callback);
    egui_timer_start_timer(&lyric_swap_timer, 1800, 1800);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&title_label));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&hint_label));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&lyric_static));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&lyric_dynamic));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 1:
        EGUI_SIM_SET_WAIT(p_action, 1600);
        return true;
    case 2:
        EGUI_SIM_SET_WAIT(p_action, 2000);
        return true;
    default:
        return false;
    }
}
#endif
