#include "egui.h"
#include "uicode.h"

#include "widget/egui_view_activity_ring.h"
#include "widget/egui_view_analog_clock.h"
#include "widget/egui_view_button.h"
#include "widget/egui_view_chart_line.h"
#include "widget/egui_view_chart_pie.h"
#include "widget/egui_view_checkbox.h"
#include "widget/egui_view_circular_progress_bar.h"
#include "widget/egui_view_gauge.h"
#include "widget/egui_view_image_button.h"
#include "widget/egui_view_line.h"
#include "widget/egui_view_notification_badge.h"
#include "widget/egui_view_page_indicator.h"
#include "widget/egui_view_progress_bar.h"
#include "widget/egui_view_radio_button.h"
#include "widget/egui_view_slider.h"
#include "widget/egui_view_stepper.h"
#include "widget/egui_view_switch.h"
#include "widget/egui_view_toggle_button.h"
#include "size_analysis_probe_config.h"

#if defined(__GNUC__)
#define EGUI_SIZE_PROBE_FUNC static void __attribute__((noinline, used))
#else
#define EGUI_SIZE_PROBE_FUNC static void
#endif

static egui_view_slider_t probe_slider;
static egui_view_switch_t probe_switch;
static egui_view_page_indicator_t probe_page_indicator;
static egui_view_stepper_t probe_stepper;
static egui_view_checkbox_t probe_checkbox;
static egui_view_radio_button_t probe_radio_button;
static egui_view_progress_bar_t probe_progress_bar;
static egui_view_toggle_button_t probe_toggle_button;
static egui_view_notification_badge_t probe_notification_badge;
static egui_view_button_t probe_button;
static egui_view_image_button_t probe_image_button;
static egui_view_circular_progress_bar_t probe_cpb;
static egui_view_gauge_t probe_gauge;
static egui_view_activity_ring_t probe_activity_ring;
static egui_view_analog_clock_t probe_analog_clock;
static egui_view_line_t probe_line;
static egui_view_chart_line_t probe_chart_line;
static egui_view_chart_pie_t probe_chart_pie;

EGUI_VIEW_SLIDER_PARAMS_INIT(probe_slider_params, 0, 0, 120, 24, 45);
EGUI_VIEW_SWITCH_PARAMS_INIT(probe_switch_params, 0, 0, 48, 28, 1);
EGUI_VIEW_PAGE_INDICATOR_PARAMS_INIT(probe_page_indicator_params, 0, 0, 120, 20, 5, 2);
EGUI_VIEW_STEPPER_PARAMS_INIT(probe_stepper_params, 0, 0, 120, 20, 5, 2);
EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT(probe_progress_bar_params, 0, 0, 120, 20, 60);
EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(probe_toggle_button_params, 0, 0, 120, 28, "Mode", 1);
EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT(probe_notification_badge_params, 0, 0, 24, 24, 8);
EGUI_VIEW_BUTTON_PARAMS_INIT_SIMPLE(probe_button_params, 0, 0, 96, 28, "Run");
EGUI_VIEW_IMAGE_BUTTON_PARAMS_INIT(probe_image_button_params, 0, 0, 72, 72, NULL);
EGUI_VIEW_CIRCULAR_PROGRESS_BAR_PARAMS_INIT(probe_cpb_params, 0, 0, 80, 80, 65);
EGUI_VIEW_GAUGE_PARAMS_INIT(probe_gauge_params, 0, 0, 100, 100, 72);
EGUI_VIEW_ACTIVITY_RING_PARAMS_INIT(probe_activity_ring_params, 0, 0, 120, 120);
EGUI_VIEW_ANALOG_CLOCK_PARAMS_INIT(probe_analog_clock_params, 0, 0, 100, 100, 10, 8, 24);
EGUI_VIEW_LINE_PARAMS_INIT(probe_line_params, 0, 0, 100, 40, 6, EGUI_COLOR_GREEN);
EGUI_VIEW_CHART_LINE_PARAMS_INIT(probe_chart_line_params, 0, 0, 120, 80);
EGUI_VIEW_CHART_PIE_PARAMS_INIT(probe_chart_pie_params, 0, 0, 120, 80);

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

static const egui_chart_pie_slice_t probe_chart_pie_slices[] = {
        {.value = 40, .color = EGUI_COLOR_BLUE, .name = "CPU"},
        {.value = 25, .color = EGUI_COLOR_GREEN, .name = "RAM"},
        {.value = 20, .color = EGUI_COLOR_ORANGE, .name = "IO"},
        {.value = 15, .color = EGUI_COLOR_RED, .name = "NET"},
};

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

EGUI_SIZE_PROBE_FUNC widget_probe_checkbox(void)
{
    egui_view_checkbox_init(EGUI_VIEW_OF(&probe_checkbox));
    egui_view_checkbox_set_checked(EGUI_VIEW_OF(&probe_checkbox), 1);
}

EGUI_SIZE_PROBE_FUNC widget_probe_radio_button(void)
{
    egui_view_radio_button_init(EGUI_VIEW_OF(&probe_radio_button));
    egui_view_radio_button_set_checked(EGUI_VIEW_OF(&probe_radio_button), 1);
}

EGUI_SIZE_PROBE_FUNC widget_probe_progress_bar(void)
{
    egui_view_progress_bar_init_with_params(EGUI_VIEW_OF(&probe_progress_bar), &probe_progress_bar_params);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&probe_progress_bar), 75);
}

EGUI_SIZE_PROBE_FUNC widget_probe_toggle_button(void)
{
    egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&probe_toggle_button), &probe_toggle_button_params);
    egui_view_toggle_button_set_toggled(EGUI_VIEW_OF(&probe_toggle_button), 1);
}

EGUI_SIZE_PROBE_FUNC widget_probe_notification_badge(void)
{
    egui_view_notification_badge_init_with_params(EGUI_VIEW_OF(&probe_notification_badge), &probe_notification_badge_params);
    egui_view_notification_badge_set_count(EGUI_VIEW_OF(&probe_notification_badge), 9);
}

EGUI_SIZE_PROBE_FUNC widget_probe_button(void)
{
    egui_view_button_init_with_params(EGUI_VIEW_OF(&probe_button), &probe_button_params);
}

EGUI_SIZE_PROBE_FUNC widget_probe_image_button(void)
{
    egui_view_image_button_init_with_params(EGUI_VIEW_OF(&probe_image_button), &probe_image_button_params);
    egui_view_image_button_set_text(EGUI_VIEW_OF(&probe_image_button), "Play");
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

EGUI_SIZE_PROBE_FUNC widget_probe_chart_pie(void)
{
    egui_view_chart_pie_init_with_params(EGUI_VIEW_OF(&probe_chart_pie), &probe_chart_pie_params);
    egui_view_chart_pie_set_slices(EGUI_VIEW_OF(&probe_chart_pie), probe_chart_pie_slices, EGUI_ARRAY_SIZE(probe_chart_pie_slices));
    egui_view_chart_pie_set_legend_pos(EGUI_VIEW_OF(&probe_chart_pie), EGUI_CHART_LEGEND_RIGHT);
}

static void widget_probe_link_enabled_features(void)
{
#if defined(EGUI_SIZE_PROBE_WIDGET_SLIDER)
    widget_probe_slider();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_SWITCH)
    widget_probe_switch();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_PAGE_INDICATOR)
    widget_probe_page_indicator();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_STEPPER)
    widget_probe_stepper();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_CHECKBOX)
    widget_probe_checkbox();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_RADIO_BUTTON)
    widget_probe_radio_button();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_PROGRESS_BAR)
    widget_probe_progress_bar();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_TOGGLE_BUTTON)
    widget_probe_toggle_button();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_NOTIFICATION_BADGE)
    widget_probe_notification_badge();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_BUTTON)
    widget_probe_button();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_IMAGE_BUTTON)
    widget_probe_image_button();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_CPB)
    widget_probe_circular_progress_bar();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_GAUGE)
    widget_probe_gauge();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_ACTIVITY_RING)
    widget_probe_activity_ring();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_ANALOG_CLOCK)
    widget_probe_analog_clock();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_LINE)
    widget_probe_line();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_CHART_LINE)
    widget_probe_chart_line();
#endif

#if defined(EGUI_SIZE_PROBE_WIDGET_CHART_PIE)
    widget_probe_chart_pie();
#endif
}

void uicode_create_ui(void)
{
    widget_probe_link_enabled_features();
}
