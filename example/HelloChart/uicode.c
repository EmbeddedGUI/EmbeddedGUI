#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// ============== Layout Constants ==============
#define SCREEN_W         EGUI_CONFIG_SCEEN_WIDTH
#define SCREEN_H         EGUI_CONFIG_SCEEN_HEIGHT
#define PAGE_COUNT       4
#define INDICATOR_HEIGHT 20
#define TOOLBAR_HEIGHT   20
#define PAGE_HEIGHT      (SCREEN_H - TOOLBAR_HEIGHT - INDICATOR_HEIGHT)
#define PAGE_WIDTH       SCREEN_W
#define BTN_WIDTH        40
#define BTN_HEIGHT       TOOLBAR_HEIGHT
#define YBTN_WIDTH       30
#define YBTN_HEIGHT      18
#define YBTN_V_MARGIN    2
#define YBTN_GROUP_H     ((YBTN_HEIGHT + (YBTN_V_MARGIN * 2)) * 3)
#define YBTN_X           2
#define YBTN_Y           ((PAGE_HEIGHT - YBTN_GROUP_H) / 2)

// ============== ViewPage ==============
static egui_view_viewpage_t viewpage;
EGUI_VIEW_VIEWPAGE_PARAMS_INIT(viewpage_params, 0, 0, PAGE_WIDTH, PAGE_HEIGHT);

// ============== Line Chart (Page 1) ==============
static egui_view_chart_line_t chart_line;
EGUI_VIEW_CHART_LINE_PARAMS_INIT(chart_line_params, 0, 0, PAGE_WIDTH, PAGE_HEIGHT);

static const egui_chart_point_t line_pts1[] = {
        {0, 10}, {15, 45}, {30, 30}, {45, 70}, {60, 55}, {75, 85}, {90, 40}, {100, 65},
};
static const egui_chart_point_t line_pts2[] = {
        {0, 50}, {15, 35}, {30, 60}, {45, 25}, {60, 80}, {75, 45}, {90, 70}, {100, 30},
};
static const egui_chart_series_t line_series[] = {
        {.points = line_pts1, .point_count = 8, .color = EGUI_THEME_PRIMARY, .name = "Temp"},
        {.points = line_pts2, .point_count = 8, .color = EGUI_THEME_SECONDARY, .name = "Humi"},
};

// ============== Scatter Chart (Page 2) ==============
static egui_view_chart_scatter_t chart_scatter;
EGUI_VIEW_CHART_SCATTER_PARAMS_INIT(chart_scatter_params, 0, 0, PAGE_WIDTH, PAGE_HEIGHT);

static const egui_chart_point_t scatter_pts1[] = {
        {5, 20}, {12, 55}, {22, 35}, {35, 80}, {48, 45}, {58, 90}, {72, 60}, {82, 75}, {92, 50}, {98, 30},
};
static const egui_chart_point_t scatter_pts2[] = {
        {8, 40}, {18, 15}, {32, 65}, {42, 30}, {55, 70}, {65, 50}, {78, 85}, {88, 40}, {95, 60}, {100, 25},
};
static const egui_chart_series_t scatter_series[] = {
        {.points = scatter_pts1, .point_count = 10, .color = EGUI_THEME_WARNING, .name = "Set1"},
        {.points = scatter_pts2, .point_count = 10, .color = EGUI_THEME_DANGER, .name = "Set2"},
};

// ============== Bar Chart (Page 3) ==============
static egui_view_chart_bar_t chart_bar;
EGUI_VIEW_CHART_BAR_PARAMS_INIT(chart_bar_params, 0, 0, PAGE_WIDTH, PAGE_HEIGHT);

static const egui_chart_point_t bar_pts1[] = {
        {0, 30}, {1, 60}, {2, 45}, {3, 80}, {4, 55},
};
static const egui_chart_point_t bar_pts2[] = {
        {0, 50}, {1, 40}, {2, 70}, {3, 35}, {4, 65},
};
static const egui_chart_series_t bar_series[] = {
        {.points = bar_pts1, .point_count = 5, .color = EGUI_THEME_PRIMARY_DARK, .name = "2024"},
        {.points = bar_pts2, .point_count = 5, .color = EGUI_THEME_SUCCESS, .name = "2025"},
};

// ============== Pie Chart (Page 4) ==============
static egui_view_chart_pie_t chart_pie;
EGUI_VIEW_CHART_PIE_PARAMS_INIT(chart_pie_params, 0, 0, PAGE_WIDTH, PAGE_HEIGHT);

static const egui_chart_pie_slice_t pie_data[] = {
        {.value = 35, .color = EGUI_THEME_PRIMARY, .name = "CPU"},
        {.value = 25, .color = EGUI_THEME_SECONDARY, .name = "RAM"},
        {.value = 20, .color = EGUI_THEME_SUCCESS, .name = "IO"},
        {.value = 20, .color = EGUI_THEME_WARNING, .name = "Net"},
};

// ============== Toolbar (zoom buttons) ==============
static egui_view_linearlayout_t toolbar;
static egui_view_button_t btn_zoom_out;
static egui_view_button_t btn_zoom_reset;
static egui_view_button_t btn_zoom_in;

// ============== Left Toolbar (Y-axis zoom buttons) ==============
static egui_view_linearlayout_t toolbar_y;
static egui_view_button_t btn_zoom_y_out;
static egui_view_button_t btn_zoom_y_reset;
static egui_view_button_t btn_zoom_y_in;

EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT_H(toolbar_params, 0, 0, SCREEN_W, TOOLBAR_HEIGHT, EGUI_ALIGN_CENTER);
EGUI_VIEW_BUTTON_PARAMS_INIT(btn_zoom_out_params, 0, 0, BTN_WIDTH, BTN_HEIGHT, "X-", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_VIEW_BUTTON_PARAMS_INIT(btn_zoom_reset_params, 0, 0, BTN_WIDTH, BTN_HEIGHT, "XR", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_VIEW_BUTTON_PARAMS_INIT(btn_zoom_in_params, 0, 0, BTN_WIDTH, BTN_HEIGHT, "X+", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT(toolbar_y_params, YBTN_X, YBTN_Y, YBTN_WIDTH, YBTN_GROUP_H, EGUI_ALIGN_CENTER);
EGUI_VIEW_BUTTON_PARAMS_INIT(btn_zoom_y_out_params, 0, 0, YBTN_WIDTH, YBTN_HEIGHT, "Y-", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_VIEW_BUTTON_PARAMS_INIT(btn_zoom_y_reset_params, 0, 0, YBTN_WIDTH, YBTN_HEIGHT, "YR", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_VIEW_BUTTON_PARAMS_INIT(btn_zoom_y_in_params, 0, 0, YBTN_WIDTH, YBTN_HEIGHT, "Y+", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

// ============== Page Indicator ==============
static egui_view_page_indicator_t indicator;
EGUI_VIEW_PAGE_INDICATOR_PARAMS_INIT(indicator_params, 0, 0, SCREEN_W, INDICATOR_HEIGHT, PAGE_COUNT, 0);

// ============== Timer ==============
static egui_timer_t sync_timer;

// ============== Zoom Callbacks ==============

static void btn_zoom_out_cb(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    egui_view_viewpage_t *vp = (egui_view_viewpage_t *)&viewpage;
    switch (vp->current_page_index)
    {
    case 0:
        egui_view_chart_line_zoom_out_x(EGUI_VIEW_OF(&chart_line));
        break;
    case 1:
        egui_view_chart_scatter_zoom_out_x(EGUI_VIEW_OF(&chart_scatter));
        break;
    case 2:
        egui_view_chart_bar_zoom_out_x(EGUI_VIEW_OF(&chart_bar));
        break;
    default:
        break;
    }
#endif
}

static void btn_zoom_reset_cb(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    egui_view_viewpage_t *vp = (egui_view_viewpage_t *)&viewpage;
    switch (vp->current_page_index)
    {
    case 0:
        egui_view_chart_line_zoom_reset_x(EGUI_VIEW_OF(&chart_line));
        break;
    case 1:
        egui_view_chart_scatter_zoom_reset_x(EGUI_VIEW_OF(&chart_scatter));
        break;
    case 2:
        egui_view_chart_bar_zoom_reset_x(EGUI_VIEW_OF(&chart_bar));
        break;
    default:
        break;
    }
#endif
}

static void btn_zoom_in_cb(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    egui_view_viewpage_t *vp = (egui_view_viewpage_t *)&viewpage;
    switch (vp->current_page_index)
    {
    case 0:
        egui_view_chart_line_zoom_in_x(EGUI_VIEW_OF(&chart_line));
        break;
    case 1:
        egui_view_chart_scatter_zoom_in_x(EGUI_VIEW_OF(&chart_scatter));
        break;
    case 2:
        egui_view_chart_bar_zoom_in_x(EGUI_VIEW_OF(&chart_bar));
        break;
    default:
        break;
    }
#endif
}

static void btn_zoom_y_out_cb(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    egui_view_viewpage_t *vp = (egui_view_viewpage_t *)&viewpage;
    switch (vp->current_page_index)
    {
    case 0:
        egui_view_chart_line_zoom_out_y(EGUI_VIEW_OF(&chart_line));
        break;
    case 1:
        egui_view_chart_scatter_zoom_out_y(EGUI_VIEW_OF(&chart_scatter));
        break;
    case 2:
        egui_view_chart_bar_zoom_out_y(EGUI_VIEW_OF(&chart_bar));
        break;
    default:
        break;
    }
#endif
}

static void btn_zoom_y_reset_cb(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    egui_view_viewpage_t *vp = (egui_view_viewpage_t *)&viewpage;
    switch (vp->current_page_index)
    {
    case 0:
        egui_view_chart_line_zoom_reset_y(EGUI_VIEW_OF(&chart_line));
        break;
    case 1:
        egui_view_chart_scatter_zoom_reset_y(EGUI_VIEW_OF(&chart_scatter));
        break;
    case 2:
        egui_view_chart_bar_zoom_reset_y(EGUI_VIEW_OF(&chart_bar));
        break;
    default:
        break;
    }
#endif
}

static void btn_zoom_y_in_cb(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    egui_view_viewpage_t *vp = (egui_view_viewpage_t *)&viewpage;
    switch (vp->current_page_index)
    {
    case 0:
        egui_view_chart_line_zoom_in_y(EGUI_VIEW_OF(&chart_line));
        break;
    case 1:
        egui_view_chart_scatter_zoom_in_y(EGUI_VIEW_OF(&chart_scatter));
        break;
    case 2:
        egui_view_chart_bar_zoom_in_y(EGUI_VIEW_OF(&chart_bar));
        break;
    default:
        break;
    }
#endif
}

static void sync_timer_callback(egui_timer_t *timer)
{
    egui_view_viewpage_t *vp = (egui_view_viewpage_t *)&viewpage;
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&indicator), vp->current_page_index);
}

void uicode_init_ui(void)
{
    // ---- ViewPage ----
    egui_view_viewpage_init_with_params(EGUI_VIEW_OF(&viewpage), &viewpage_params);

    // ---- Line Chart ----
    egui_view_chart_line_init_with_params(EGUI_VIEW_OF(&chart_line), &chart_line_params);
    egui_view_set_padding(EGUI_VIEW_OF(&chart_line), 2, 10, 6, 6);
    egui_view_chart_line_set_series(EGUI_VIEW_OF(&chart_line), line_series, 2);
    egui_view_chart_line_set_axis_x(EGUI_VIEW_OF(&chart_line), 0, 100, 20);
    egui_view_chart_line_set_axis_y(EGUI_VIEW_OF(&chart_line), 0, 100, 25);
    egui_view_chart_line_set_legend_pos(EGUI_VIEW_OF(&chart_line), EGUI_CHART_LEGEND_BOTTOM);
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    egui_view_chart_line_set_zoom_enabled(EGUI_VIEW_OF(&chart_line), 1);
#endif

    // ---- Scatter Chart ----
    egui_view_chart_scatter_init_with_params(EGUI_VIEW_OF(&chart_scatter), &chart_scatter_params);
    egui_view_set_padding(EGUI_VIEW_OF(&chart_scatter), 2, 10, 6, 6);
    egui_view_chart_scatter_set_series(EGUI_VIEW_OF(&chart_scatter), scatter_series, 2);
    egui_view_chart_scatter_set_axis_x(EGUI_VIEW_OF(&chart_scatter), 0, 100, 25);
    egui_view_chart_scatter_set_axis_y(EGUI_VIEW_OF(&chart_scatter), 0, 100, 25);
    egui_view_chart_scatter_set_legend_pos(EGUI_VIEW_OF(&chart_scatter), EGUI_CHART_LEGEND_BOTTOM);
    egui_view_chart_scatter_set_point_radius(EGUI_VIEW_OF(&chart_scatter), 3);
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    egui_view_chart_scatter_set_zoom_enabled(EGUI_VIEW_OF(&chart_scatter), 1);
#endif

    // ---- Bar Chart ----
    egui_view_chart_bar_init_with_params(EGUI_VIEW_OF(&chart_bar), &chart_bar_params);
    egui_view_set_padding(EGUI_VIEW_OF(&chart_bar), 2, 10, 6, 6);
    egui_view_chart_bar_set_series(EGUI_VIEW_OF(&chart_bar), bar_series, 2);
    egui_view_chart_bar_set_axis_x(EGUI_VIEW_OF(&chart_bar), 0, 4, 1);
    egui_view_chart_bar_set_axis_y(EGUI_VIEW_OF(&chart_bar), 0, 100, 25);
    egui_view_chart_bar_set_legend_pos(EGUI_VIEW_OF(&chart_bar), EGUI_CHART_LEGEND_BOTTOM);
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    egui_view_chart_bar_set_zoom_enabled(EGUI_VIEW_OF(&chart_bar), 1);
#endif

    // ---- Pie Chart (no zoom) ----
    egui_view_chart_pie_init_with_params(EGUI_VIEW_OF(&chart_pie), &chart_pie_params);
    egui_view_set_padding(EGUI_VIEW_OF(&chart_pie), 2, 10, 6, 6);
    egui_view_chart_pie_set_slices(EGUI_VIEW_OF(&chart_pie), pie_data, 4);
    egui_view_chart_pie_set_legend_pos(EGUI_VIEW_OF(&chart_pie), EGUI_CHART_LEGEND_RIGHT);

    // ---- Add charts to ViewPage ----
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&chart_line));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&chart_scatter));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&chart_bar));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&chart_pie));
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&viewpage));

    // ---- Toolbar ----
    egui_view_linearlayout_init_with_params(EGUI_VIEW_OF(&toolbar), &toolbar_params);

    egui_view_button_init_with_params(EGUI_VIEW_OF(&btn_zoom_out), &btn_zoom_out_params);
    egui_view_set_margin(EGUI_VIEW_OF(&btn_zoom_out), 4, 4, 0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&btn_zoom_out), btn_zoom_out_cb);

    egui_view_button_init_with_params(EGUI_VIEW_OF(&btn_zoom_reset), &btn_zoom_reset_params);
    egui_view_set_margin(EGUI_VIEW_OF(&btn_zoom_reset), 4, 4, 0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&btn_zoom_reset), btn_zoom_reset_cb);

    egui_view_button_init_with_params(EGUI_VIEW_OF(&btn_zoom_in), &btn_zoom_in_params);
    egui_view_set_margin(EGUI_VIEW_OF(&btn_zoom_in), 4, 4, 0, 0);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&btn_zoom_in), btn_zoom_in_cb);

    egui_view_group_add_child(EGUI_VIEW_OF(&toolbar), EGUI_VIEW_OF(&btn_zoom_out));
    egui_view_group_add_child(EGUI_VIEW_OF(&toolbar), EGUI_VIEW_OF(&btn_zoom_reset));
    egui_view_group_add_child(EGUI_VIEW_OF(&toolbar), EGUI_VIEW_OF(&btn_zoom_in));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&toolbar));

    // ---- Left Y-axis Toolbar ----
    egui_view_linearlayout_init_with_params(EGUI_VIEW_OF(&toolbar_y), &toolbar_y_params);

    egui_view_button_init_with_params(EGUI_VIEW_OF(&btn_zoom_y_out), &btn_zoom_y_out_params);
    egui_view_set_margin(EGUI_VIEW_OF(&btn_zoom_y_out), 0, 0, YBTN_V_MARGIN, YBTN_V_MARGIN);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&btn_zoom_y_out), btn_zoom_y_out_cb);

    egui_view_button_init_with_params(EGUI_VIEW_OF(&btn_zoom_y_reset), &btn_zoom_y_reset_params);
    egui_view_set_margin(EGUI_VIEW_OF(&btn_zoom_y_reset), 0, 0, YBTN_V_MARGIN, YBTN_V_MARGIN);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&btn_zoom_y_reset), btn_zoom_y_reset_cb);

    egui_view_button_init_with_params(EGUI_VIEW_OF(&btn_zoom_y_in), &btn_zoom_y_in_params);
    egui_view_set_margin(EGUI_VIEW_OF(&btn_zoom_y_in), 0, 0, YBTN_V_MARGIN, YBTN_V_MARGIN);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&btn_zoom_y_in), btn_zoom_y_in_cb);

    egui_view_group_add_child(EGUI_VIEW_OF(&toolbar_y), EGUI_VIEW_OF(&btn_zoom_y_out));
    egui_view_group_add_child(EGUI_VIEW_OF(&toolbar_y), EGUI_VIEW_OF(&btn_zoom_y_reset));
    egui_view_group_add_child(EGUI_VIEW_OF(&toolbar_y), EGUI_VIEW_OF(&btn_zoom_y_in));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&toolbar_y));

    // ---- Page Indicator ----
    egui_view_page_indicator_init_with_params(EGUI_VIEW_OF(&indicator), &indicator_params);

    // ---- Add to root ----
    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar_y));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toolbar));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&indicator));

    // Use explicit positions (do not auto-layout root views), otherwise
    // toolbar_y / toolbar / indicator can be re-arranged and overlap.
    egui_view_set_position(EGUI_VIEW_OF(&viewpage), 0, 0);
    egui_view_set_position(EGUI_VIEW_OF(&toolbar_y), YBTN_X, YBTN_Y);
    egui_view_set_position(EGUI_VIEW_OF(&indicator), 0, PAGE_HEIGHT);
    egui_view_set_position(EGUI_VIEW_OF(&toolbar), 0, PAGE_HEIGHT + INDICATOR_HEIGHT);

    // ---- Sync timer ----
    egui_timer_init_timer(&sync_timer, NULL, sync_timer_callback);
    egui_timer_start_timer(&sync_timer, 100, 100);
}

void uicode_create_ui(void)
{
    uicode_init_ui();
}

#if EGUI_CONFIG_RECORDING_TEST
static void recording_switch_page(int page)
{
    egui_view_viewpage_set_current_page(EGUI_VIEW_OF(&viewpage), page);
}

static void recording_zoom_in_x(void)
{
    egui_view_chart_line_zoom_in_x(EGUI_VIEW_OF(&chart_line));
}

static void recording_zoom_reset_x(void)
{
    egui_view_chart_line_zoom_reset_x(EGUI_VIEW_OF(&chart_line));
}

static void recording_zoom_in_y(void)
{
    egui_view_chart_line_zoom_in_y(EGUI_VIEW_OF(&chart_line));
}

static void recording_zoom_reset_y(void)
{
    egui_view_chart_line_zoom_reset_y(EGUI_VIEW_OF(&chart_line));
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);
    last_action = action_index;

    switch (action_index)
    {
    case 0: // show line chart
        if (first_call)
            recording_request_snapshot();
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    case 1: // zoom in X
        if (first_call)
        {
            recording_zoom_in_x();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    case 2: // zoom in X again
        if (first_call)
        {
            recording_zoom_in_x();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    case 3: // zoom in Y
        if (first_call)
        {
            recording_zoom_in_y();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    case 4: // zoom in Y again
        if (first_call)
        {
            recording_zoom_in_y();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    case 5: // zoom reset all
        if (first_call)
        {
            recording_zoom_reset_x();
            recording_zoom_reset_y();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    case 6: // switch to scatter chart (page 1)
        if (first_call)
        {
            recording_switch_page(1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    case 7: // switch to bar chart (page 2)
        if (first_call)
        {
            recording_switch_page(2);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    case 8: // switch to pie chart (page 3)
        if (first_call)
        {
            recording_switch_page(3);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    default:
        return false;
    }
}
#endif
