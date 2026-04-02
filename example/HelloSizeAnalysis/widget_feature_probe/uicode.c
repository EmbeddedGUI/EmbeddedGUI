#include "egui.h"
#include "uicode.h"

#include "widget/egui_view_activity_ring.h"
#include "widget/egui_view_analog_clock.h"
#include "widget/egui_view_chart_line.h"
#include "widget/egui_view_circular_progress_bar.h"
#include "widget/egui_view_gauge.h"
#include "widget/egui_view_line.h"
#include "widget/egui_view_page_indicator.h"
#include "widget/egui_view_slider.h"
#include "widget/egui_view_stepper.h"
#include "widget/egui_view_switch.h"
#include "size_analysis_probe_config.h"

#if defined(__GNUC__)
#define EGUI_SIZE_PROBE_FUNC static void __attribute__((unused))
#else
#define EGUI_SIZE_PROBE_FUNC static void
#endif

static egui_view_slider_t probe_slider;
static egui_view_switch_t probe_switch;
static egui_view_page_indicator_t probe_page_indicator;
static egui_view_stepper_t probe_stepper;
static egui_view_circular_progress_bar_t probe_cpb;
static egui_view_gauge_t probe_gauge;
static egui_view_activity_ring_t probe_activity_ring;
static egui_view_analog_clock_t probe_analog_clock;
static egui_view_line_t probe_line;
static egui_view_chart_line_t probe_chart_line;

EGUI_VIEW_SLIDER_PARAMS_INIT(probe_slider_params, 0, 0, 120, 24, 45);
EGUI_VIEW_SWITCH_PARAMS_INIT(probe_switch_params, 0, 0, 48, 28, 1);
EGUI_VIEW_PAGE_INDICATOR_PARAMS_INIT(probe_page_indicator_params, 0, 0, 120, 20, 5, 2);
EGUI_VIEW_STEPPER_PARAMS_INIT(probe_stepper_params, 0, 0, 120, 20, 5, 2);
EGUI_VIEW_CIRCULAR_PROGRESS_BAR_PARAMS_INIT(probe_cpb_params, 0, 0, 80, 80, 65);
EGUI_VIEW_GAUGE_PARAMS_INIT(probe_gauge_params, 0, 0, 100, 100, 72);
EGUI_VIEW_ACTIVITY_RING_PARAMS_INIT(probe_activity_ring_params, 0, 0, 120, 120);
EGUI_VIEW_ANALOG_CLOCK_PARAMS_INIT(probe_analog_clock_params, 0, 0, 100, 100, 10, 8, 24);
EGUI_VIEW_LINE_PARAMS_INIT(probe_line_params, 0, 0, 100, 40, 6, EGUI_COLOR_GREEN);
EGUI_VIEW_CHART_LINE_PARAMS_INIT(probe_chart_line_params, 0, 0, 120, 80);

static const egui_view_line_point_t probe_line_points[] = {
        {0, 20},
        {24, 4},
        {52, 30},
        {88, 10},
};

static const egui_chart_point_t probe_chart_points[] = {
        {0, 10}, {10, 35}, {20, 18}, {30, 42}, {40, 24},
};

static const egui_chart_series_t probe_chart_series[] = {
        {
                .points = probe_chart_points,
                .point_count = EGUI_ARRAY_SIZE(probe_chart_points),
                .color = EGUI_COLOR_CYAN,
                .name = "A",
        },
};

volatile int g_widget_feature_probe_runtime_enable = 0;

EGUI_SIZE_PROBE_FUNC widget_probe_slider(void)
{
    egui_view_slider_init_with_params(EGUI_VIEW_OF(&probe_slider), &probe_slider_params);
    egui_view_slider_set_value(EGUI_VIEW_OF(&probe_slider), 60);
}

EGUI_SIZE_PROBE_FUNC widget_probe_switch(void)
{
    egui_view_switch_init_with_params(EGUI_VIEW_OF(&probe_switch), &probe_switch_params);
    egui_view_switch_set_checked(EGUI_VIEW_OF(&probe_switch), 1);
}

EGUI_SIZE_PROBE_FUNC widget_probe_page_indicator(void)
{
    egui_view_page_indicator_init_with_params(EGUI_VIEW_OF(&probe_page_indicator), &probe_page_indicator_params);
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&probe_page_indicator), 3);
}

EGUI_SIZE_PROBE_FUNC widget_probe_stepper(void)
{
    egui_view_stepper_init_with_params(EGUI_VIEW_OF(&probe_stepper), &probe_stepper_params);
    egui_view_stepper_set_current_step(EGUI_VIEW_OF(&probe_stepper), 3);
}

EGUI_SIZE_PROBE_FUNC widget_probe_circular_progress_bar(void)
{
    egui_view_circular_progress_bar_init_with_params(EGUI_VIEW_OF(&probe_cpb), &probe_cpb_params);
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&probe_cpb), 70);
}

EGUI_SIZE_PROBE_FUNC widget_probe_gauge(void)
{
    egui_view_gauge_init_with_params(EGUI_VIEW_OF(&probe_gauge), &probe_gauge_params);
    egui_view_gauge_set_value(EGUI_VIEW_OF(&probe_gauge), 75);
}

EGUI_SIZE_PROBE_FUNC widget_probe_activity_ring(void)
{
    egui_view_activity_ring_init_with_params(EGUI_VIEW_OF(&probe_activity_ring), &probe_activity_ring_params);
    egui_view_activity_ring_set_ring_count(EGUI_VIEW_OF(&probe_activity_ring), 3);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&probe_activity_ring), 0, 70);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&probe_activity_ring), 1, 55);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&probe_activity_ring), 2, 80);
    egui_view_activity_ring_set_show_round_cap(EGUI_VIEW_OF(&probe_activity_ring), 1);
}

EGUI_SIZE_PROBE_FUNC widget_probe_analog_clock(void)
{
    egui_view_analog_clock_init_with_params(EGUI_VIEW_OF(&probe_analog_clock), &probe_analog_clock_params);
    egui_view_analog_clock_show_second(EGUI_VIEW_OF(&probe_analog_clock), 1);
    egui_view_analog_clock_show_ticks(EGUI_VIEW_OF(&probe_analog_clock), 1);
}

EGUI_SIZE_PROBE_FUNC widget_probe_line(void)
{
    egui_view_line_init_with_params(EGUI_VIEW_OF(&probe_line), &probe_line_params);
    egui_view_line_set_points(EGUI_VIEW_OF(&probe_line), probe_line_points, EGUI_ARRAY_SIZE(probe_line_points));
    egui_view_line_set_use_round_cap(EGUI_VIEW_OF(&probe_line), 1);
}

EGUI_SIZE_PROBE_FUNC widget_probe_chart_line(void)
{
    egui_view_chart_line_init_with_params(EGUI_VIEW_OF(&probe_chart_line), &probe_chart_line_params);
    egui_view_chart_line_set_series(EGUI_VIEW_OF(&probe_chart_line), probe_chart_series, EGUI_ARRAY_SIZE(probe_chart_series));
    egui_view_chart_line_set_axis_x(EGUI_VIEW_OF(&probe_chart_line), 0, 40, 10);
    egui_view_chart_line_set_axis_y(EGUI_VIEW_OF(&probe_chart_line), 0, 50, 10);
}

static void widget_probe_link_enabled_features(void)
{
#if defined(EGUI_SIZE_PROBE_WIDGET_SLIDER)
    if (g_widget_feature_probe_runtime_enable)
    {
        widget_probe_slider();
    }
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_SWITCH)
    if (g_widget_feature_probe_runtime_enable)
    {
        widget_probe_switch();
    }
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR)
    if (g_widget_feature_probe_runtime_enable)
    {
        widget_probe_page_indicator();
    }
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_STEPPER)
    if (g_widget_feature_probe_runtime_enable)
    {
        widget_probe_stepper();
    }
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_CPB)
    if (g_widget_feature_probe_runtime_enable)
    {
        widget_probe_circular_progress_bar();
    }
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_GAUGE)
    if (g_widget_feature_probe_runtime_enable)
    {
        widget_probe_gauge();
    }
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING)
    if (g_widget_feature_probe_runtime_enable)
    {
        widget_probe_activity_ring();
    }
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK)
    if (g_widget_feature_probe_runtime_enable)
    {
        widget_probe_analog_clock();
    }
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_LINE)
    if (g_widget_feature_probe_runtime_enable)
    {
        widget_probe_line();
    }
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_CHART_LINE)
    if (g_widget_feature_probe_runtime_enable)
    {
        widget_probe_chart_line();
    }
#endif
}

void uicode_create_ui(void)
{
    widget_probe_link_enabled_features();
}
