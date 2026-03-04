#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"

#define DB_FONT_TITLE EGUI_FONT_OF(&egui_res_font_montserrat_20_4)
#define DB_FONT_KPI   EGUI_FONT_OF(&egui_res_font_montserrat_22_4)
#define DB_FONT_LABEL EGUI_FONT_OF(&egui_res_font_montserrat_12_4)
#define DB_FONT_ICON  EGUI_FONT_OF(&egui_res_font_materialsymbolsoutlined_regular_14_4)

// Theme icon UTF-8: E518=light_mode, E51C=dark_mode
#define ICON_LIGHT_MODE "\xEE\x94\x98"
#define ICON_DARK_MODE  "\xEE\x94\x9C"

// Title
static egui_view_label_t db_title;
static egui_view_button_t db_theme_btn;

// KPI cards
static egui_view_card_t db_kpi1;
static egui_view_label_t db_kpi1_val;
static egui_view_label_t db_kpi1_name;

static egui_view_card_t db_kpi2;
static egui_view_label_t db_kpi2_val;
static egui_view_label_t db_kpi2_name;

static egui_view_card_t db_kpi3;
static egui_view_label_t db_kpi3_val;
static egui_view_label_t db_kpi3_name;

// Charts
static egui_view_chart_line_t db_line_chart;
static egui_view_chart_bar_t db_bar_chart;
static egui_view_chart_pie_t db_pie_chart;

// Target chart data (final values)
static const int16_t db_line_target_y[] = {20, 45, 30, 60, 40, 75, 55};
static const int16_t db_bar_target_y[] = {30, 50, 40, 70, 60};
static const uint16_t db_pie_target_vals[] = {40, 30, 20, 10};

// Animated chart data (mutable, interpolated from 0 to target)
static egui_chart_point_t db_line_pts_anim[] = {
        {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0},
};
static egui_chart_series_t db_line_series_anim[] = {
        {.points = db_line_pts_anim, .point_count = 7, .color = EGUI_COLOR_MAKE(0x25, 0x63, 0xEB), .name = "CPU"},
};

static egui_chart_point_t db_bar_pts_anim[] = {
        {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0},
};
static egui_chart_series_t db_bar_series_anim[] = {
        {.points = db_bar_pts_anim, .point_count = 5, .color = EGUI_COLOR_MAKE(0x25, 0x63, 0xEB), .name = "Sales"},
};

static egui_chart_pie_slice_t db_pie_slices_anim[] = {
        {.value = 1, .color = EGUI_COLOR_MAKE(0x25, 0x63, 0xEB), .name = "A"},
        {.value = 0, .color = EGUI_COLOR_MAKE(0x10, 0xB9, 0x81), .name = "B"},
        {.value = 0, .color = EGUI_COLOR_MAKE(0xF5, 0x9E, 0x0B), .name = "C"},
        {.value = 0, .color = EGUI_COLOR_MAKE(0xEF, 0x44, 0x44), .name = "D"},
};

// Growth animation state
static egui_timer_t db_growth_timer;
static int db_growth_frame = 0;
#define DB_GROWTH_FRAMES   20
#define DB_GROWTH_INTERVAL 40

// KPI text buffers
static char db_kpi1_buf[8];
static char db_kpi2_buf[8];
static char db_kpi3_buf[8];

static void db_update_kpi_display(int decel)
{
    int val1 = (85 * decel) / 100;
    int val2 = (127 * decel) / 100;
    int val3_x10 = (42 * decel) / 100;

    sprintf(db_kpi1_buf, "%d%%", val1);
    sprintf(db_kpi2_buf, "%d", val2);
    sprintf(db_kpi3_buf, "%d.%d", val3_x10 / 10, val3_x10 % 10);

    egui_view_label_set_text(EGUI_VIEW_OF(&db_kpi1_val), db_kpi1_buf);
    egui_view_label_set_text(EGUI_VIEW_OF(&db_kpi2_val), db_kpi2_buf);
    egui_view_label_set_text(EGUI_VIEW_OF(&db_kpi3_val), db_kpi3_buf);
}

static void db_growth_timer_callback(egui_timer_t *timer)
{
    (void)timer;
    db_growth_frame++;
    if (db_growth_frame > DB_GROWTH_FRAMES)
    {
        egui_timer_stop_timer(&db_growth_timer);
        return;
    }

    int progress = (db_growth_frame * 100) / DB_GROWTH_FRAMES;
    // decelerate easing: t' = 1 - (1-t)^2
    int t_inv = 100 - progress;
    int decel = 100 - (t_inv * t_inv) / 100;

    // Update line chart points
    for (int i = 0; i < 7; i++)
    {
        db_line_pts_anim[i].y = (int16_t)((db_line_target_y[i] * decel) / 100);
    }
    egui_view_chart_line_set_series(EGUI_VIEW_OF(&db_line_chart), db_line_series_anim, 1);

    // Update bar chart points
    for (int i = 0; i < 5; i++)
    {
        db_bar_pts_anim[i].y = (int16_t)((db_bar_target_y[i] * decel) / 100);
    }
    egui_view_chart_bar_set_series(EGUI_VIEW_OF(&db_bar_chart), db_bar_series_anim, 1);

    // Update pie chart slices
    for (int i = 0; i < 4; i++)
    {
        db_pie_slices_anim[i].value = (uint16_t)((db_pie_target_vals[i] * decel) / 100);
    }
    if (db_pie_slices_anim[0].value == 0 && db_pie_slices_anim[1].value == 0 && db_pie_slices_anim[2].value == 0 && db_pie_slices_anim[3].value == 0)
    {
        db_pie_slices_anim[0].value = 1;
    }
    egui_view_chart_pie_set_slices(EGUI_VIEW_OF(&db_pie_chart), db_pie_slices_anim, 4);

    // Update KPI numbers
    db_update_kpi_display(decel);
}

static void db_theme_btn_click(egui_view_t *self)
{
    (void)self;
    uicode_toggle_theme();
}

void uicode_init_page_dashboard(egui_view_t *parent)
{
    // Title "Dashboard"
    egui_view_label_init(EGUI_VIEW_OF(&db_title));
    egui_view_set_position(EGUI_VIEW_OF(&db_title), 10, 5);
    egui_view_set_size(EGUI_VIEW_OF(&db_title), 150, 30);
    egui_view_label_set_text(EGUI_VIEW_OF(&db_title), "Dashboard");
    egui_view_label_set_font(EGUI_VIEW_OF(&db_title), DB_FONT_TITLE);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&db_title), EGUI_COLOR_MAKE(0x1E, 0x29, 0x3B), EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&db_title), EGUI_ALIGN_LEFT);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&db_title));

    // Theme toggle (icon button)
    egui_view_button_init(EGUI_VIEW_OF(&db_theme_btn));
    egui_view_set_position(EGUI_VIEW_OF(&db_theme_btn), 200, 5);
    egui_view_set_size(EGUI_VIEW_OF(&db_theme_btn), 32, 28);
    egui_view_label_set_text(EGUI_VIEW_OF(&db_theme_btn), ICON_LIGHT_MODE);
    egui_view_label_set_font(EGUI_VIEW_OF(&db_theme_btn), DB_FONT_ICON);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&db_theme_btn), db_theme_btn_click);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&db_theme_btn));

    // KPI 1: CPU
    egui_view_card_init(EGUI_VIEW_OF(&db_kpi1));
    egui_view_set_position(EGUI_VIEW_OF(&db_kpi1), 5, 40);
    egui_view_set_size(EGUI_VIEW_OF(&db_kpi1), 73, 55);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&db_kpi1), 6);

    egui_view_label_init(EGUI_VIEW_OF(&db_kpi1_val));
    egui_view_set_position(EGUI_VIEW_OF(&db_kpi1_val), 2, 5);
    egui_view_set_size(EGUI_VIEW_OF(&db_kpi1_val), 69, 24);
    egui_view_label_set_text(EGUI_VIEW_OF(&db_kpi1_val), "0%");
    egui_view_label_set_font(EGUI_VIEW_OF(&db_kpi1_val), DB_FONT_KPI);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&db_kpi1_val), EGUI_COLOR_MAKE(0x25, 0x63, 0xEB), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&db_kpi1), EGUI_VIEW_OF(&db_kpi1_val));

    egui_view_label_init(EGUI_VIEW_OF(&db_kpi1_name));
    egui_view_set_position(EGUI_VIEW_OF(&db_kpi1_name), 2, 30);
    egui_view_set_size(EGUI_VIEW_OF(&db_kpi1_name), 69, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&db_kpi1_name), "CPU");
    egui_view_label_set_font(EGUI_VIEW_OF(&db_kpi1_name), DB_FONT_LABEL);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&db_kpi1_name), EGUI_COLOR_MAKE(0x64, 0x74, 0x8B), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&db_kpi1), EGUI_VIEW_OF(&db_kpi1_name));

    egui_view_group_add_child(parent, EGUI_VIEW_OF(&db_kpi1));

    // KPI 2: Users
    egui_view_card_init(EGUI_VIEW_OF(&db_kpi2));
    egui_view_set_position(EGUI_VIEW_OF(&db_kpi2), 83, 40);
    egui_view_set_size(EGUI_VIEW_OF(&db_kpi2), 73, 55);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&db_kpi2), 6);

    egui_view_label_init(EGUI_VIEW_OF(&db_kpi2_val));
    egui_view_set_position(EGUI_VIEW_OF(&db_kpi2_val), 2, 5);
    egui_view_set_size(EGUI_VIEW_OF(&db_kpi2_val), 69, 24);
    egui_view_label_set_text(EGUI_VIEW_OF(&db_kpi2_val), "0");
    egui_view_label_set_font(EGUI_VIEW_OF(&db_kpi2_val), DB_FONT_KPI);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&db_kpi2_val), EGUI_COLOR_MAKE(0x10, 0xB9, 0x81), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&db_kpi2), EGUI_VIEW_OF(&db_kpi2_val));

    egui_view_label_init(EGUI_VIEW_OF(&db_kpi2_name));
    egui_view_set_position(EGUI_VIEW_OF(&db_kpi2_name), 2, 30);
    egui_view_set_size(EGUI_VIEW_OF(&db_kpi2_name), 69, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&db_kpi2_name), "Users");
    egui_view_label_set_font(EGUI_VIEW_OF(&db_kpi2_name), DB_FONT_LABEL);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&db_kpi2_name), EGUI_COLOR_MAKE(0x64, 0x74, 0x8B), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&db_kpi2), EGUI_VIEW_OF(&db_kpi2_name));

    egui_view_group_add_child(parent, EGUI_VIEW_OF(&db_kpi2));

    // KPI 3: Rate
    egui_view_card_init(EGUI_VIEW_OF(&db_kpi3));
    egui_view_set_position(EGUI_VIEW_OF(&db_kpi3), 161, 40);
    egui_view_set_size(EGUI_VIEW_OF(&db_kpi3), 73, 55);
    egui_view_card_set_corner_radius(EGUI_VIEW_OF(&db_kpi3), 6);

    egui_view_label_init(EGUI_VIEW_OF(&db_kpi3_val));
    egui_view_set_position(EGUI_VIEW_OF(&db_kpi3_val), 2, 5);
    egui_view_set_size(EGUI_VIEW_OF(&db_kpi3_val), 69, 24);
    egui_view_label_set_text(EGUI_VIEW_OF(&db_kpi3_val), "0.0");
    egui_view_label_set_font(EGUI_VIEW_OF(&db_kpi3_val), DB_FONT_KPI);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&db_kpi3_val), EGUI_COLOR_MAKE(0xF5, 0x9E, 0x0B), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&db_kpi3), EGUI_VIEW_OF(&db_kpi3_val));

    egui_view_label_init(EGUI_VIEW_OF(&db_kpi3_name));
    egui_view_set_position(EGUI_VIEW_OF(&db_kpi3_name), 2, 30);
    egui_view_set_size(EGUI_VIEW_OF(&db_kpi3_name), 69, 18);
    egui_view_label_set_text(EGUI_VIEW_OF(&db_kpi3_name), "Rate");
    egui_view_label_set_font(EGUI_VIEW_OF(&db_kpi3_name), DB_FONT_LABEL);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&db_kpi3_name), EGUI_COLOR_MAKE(0x64, 0x74, 0x8B), EGUI_ALPHA_100);
    egui_view_card_add_child(EGUI_VIEW_OF(&db_kpi3), EGUI_VIEW_OF(&db_kpi3_name));

    egui_view_group_add_child(parent, EGUI_VIEW_OF(&db_kpi3));

    // Line chart (start with zero data)
    egui_view_chart_line_init(EGUI_VIEW_OF(&db_line_chart));
    egui_view_set_position(EGUI_VIEW_OF(&db_line_chart), 5, 102);
    egui_view_set_size(EGUI_VIEW_OF(&db_line_chart), 230, 90);
    egui_view_chart_line_set_series(EGUI_VIEW_OF(&db_line_chart), db_line_series_anim, 1);
    egui_view_chart_line_set_axis_x(EGUI_VIEW_OF(&db_line_chart), 0, 6, 1);
    egui_view_chart_line_set_axis_y(EGUI_VIEW_OF(&db_line_chart), 0, 80, 20);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&db_line_chart));

    // Bar chart (start with zero data)
    egui_view_chart_bar_init(EGUI_VIEW_OF(&db_bar_chart));
    egui_view_set_position(EGUI_VIEW_OF(&db_bar_chart), 5, 200);
    egui_view_set_size(EGUI_VIEW_OF(&db_bar_chart), 112, 110);
    egui_view_chart_bar_set_series(EGUI_VIEW_OF(&db_bar_chart), db_bar_series_anim, 1);
    egui_view_chart_bar_set_axis_x(EGUI_VIEW_OF(&db_bar_chart), 0, 4, 1);
    egui_view_chart_bar_set_axis_y(EGUI_VIEW_OF(&db_bar_chart), 0, 80, 20);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&db_bar_chart));

    // Pie chart (start with minimal data)
    egui_view_chart_pie_init(EGUI_VIEW_OF(&db_pie_chart));
    egui_view_set_position(EGUI_VIEW_OF(&db_pie_chart), 122, 200);
    egui_view_set_size(EGUI_VIEW_OF(&db_pie_chart), 112, 110);
    egui_view_chart_pie_set_slices(EGUI_VIEW_OF(&db_pie_chart), db_pie_slices_anim, 4);
    egui_view_group_add_child(parent, EGUI_VIEW_OF(&db_pie_chart));

    // Init growth timer
    egui_timer_init_timer(&db_growth_timer, NULL, db_growth_timer_callback);
}

void uicode_page_dashboard_update_theme_icon(void)
{
    const char *icon = uicode_is_dark_theme() ? ICON_DARK_MODE : ICON_LIGHT_MODE;
    egui_view_label_set_text(EGUI_VIEW_OF(&db_theme_btn), icon);
}

void uicode_page_dashboard_update_theme_labels(void)
{
    if (uicode_is_dark_theme())
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&db_title), EGUI_COLOR_MAKE(0xE2, 0xE8, 0xF0), EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&db_kpi1_name), EGUI_COLOR_MAKE(0x94, 0xA3, 0xB8), EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&db_kpi2_name), EGUI_COLOR_MAKE(0x94, 0xA3, 0xB8), EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&db_kpi3_name), EGUI_COLOR_MAKE(0x94, 0xA3, 0xB8), EGUI_ALPHA_100);
    }
    else
    {
        egui_view_label_set_font_color(EGUI_VIEW_OF(&db_title), EGUI_COLOR_MAKE(0x1E, 0x29, 0x3B), EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&db_kpi1_name), EGUI_COLOR_MAKE(0x64, 0x74, 0x8B), EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&db_kpi2_name), EGUI_COLOR_MAKE(0x64, 0x74, 0x8B), EGUI_ALPHA_100);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&db_kpi3_name), EGUI_COLOR_MAKE(0x64, 0x74, 0x8B), EGUI_ALPHA_100);
    }
}

void uicode_page_dashboard_on_enter(void)
{
    // Reset animation state
    db_growth_frame = 0;
    for (int i = 0; i < 7; i++)
    {
        db_line_pts_anim[i].y = 0;
    }
    for (int i = 0; i < 5; i++)
    {
        db_bar_pts_anim[i].y = 0;
    }
    for (int i = 0; i < 4; i++)
    {
        db_pie_slices_anim[i].value = 0;
    }
    db_pie_slices_anim[0].value = 1;
    db_update_kpi_display(0);

    // Start growth animation
    egui_timer_start_timer(&db_growth_timer, DB_GROWTH_INTERVAL, DB_GROWTH_INTERVAL);
}
