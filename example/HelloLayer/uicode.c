#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// ============== Overlapping color views ==============
// Three overlapping labels with colored backgrounds to demonstrate layer ordering.
// The view drawn last (highest layer) appears on top.

static egui_view_group_t overlap_group;
static egui_view_label_t label_red;
static egui_view_label_t label_green;
static egui_view_label_t label_blue;

// Background for each colored label (normal state only, no press/disabled needed)
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_red_param, EGUI_COLOR_RED, EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(bg_red_params, &bg_red_param, &bg_red_param, &bg_red_param);
static egui_background_color_t bg_red;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_green_param, EGUI_COLOR_GREEN, EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(bg_green_params, &bg_green_param, &bg_green_param, &bg_green_param);
static egui_background_color_t bg_green;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_blue_param, EGUI_COLOR_BLUE, EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_PARAM_INIT(bg_blue_params, &bg_blue_param, &bg_blue_param, &bg_blue_param);
static egui_background_color_t bg_blue;

// ============== Title and status ==============
static egui_view_label_t label_title;
static egui_view_label_t label_status;
static char status_str[40] = "Front: Green";

// ============== Control buttons ==============
static egui_view_linearlayout_t btn_layout;
static egui_view_button_t btn_red_top;
static egui_view_button_t btn_green_top;
static egui_view_button_t btn_blue_top;

// ============== Root layout ==============
static egui_view_group_t root_group;

// Background for root
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_root_param, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_root_params, &bg_root_param, &bg_root_param, &bg_root_param);
static egui_background_color_t bg_root;

// ============== Layer dimensions ==============
#define OVERLAP_LABEL_W 120
#define OVERLAP_LABEL_H 100

#define RED_X   20
#define RED_Y   10
#define GREEN_X 60
#define GREEN_Y 50
#define BLUE_X  100
#define BLUE_Y  90

#define BTN_W 76
#define BTN_H 34

// ============== Helper: update status label ==============
static void update_status(const char *front_name)
{
    egui_api_sprintf(status_str, "Front: %s", front_name);
    egui_view_label_set_text(EGUI_VIEW_OF(&label_status), status_str);
}

// ============== Button callbacks ==============
static void btn_red_top_cb(egui_view_t *self)
{
    EGUI_LOG_INF("btn_red_top_cb\n");
    egui_view_set_layer(EGUI_VIEW_OF(&label_red), EGUI_VIEW_LAYER_TOP);
    egui_view_set_layer(EGUI_VIEW_OF(&label_green), EGUI_VIEW_LAYER_DEFAULT);
    egui_view_set_layer(EGUI_VIEW_OF(&label_blue), EGUI_VIEW_LAYER_DEFAULT);
    update_status("Red");
}

static void btn_green_top_cb(egui_view_t *self)
{
    EGUI_LOG_INF("btn_green_top_cb\n");
    egui_view_set_layer(EGUI_VIEW_OF(&label_red), EGUI_VIEW_LAYER_DEFAULT);
    egui_view_set_layer(EGUI_VIEW_OF(&label_green), EGUI_VIEW_LAYER_TOP);
    egui_view_set_layer(EGUI_VIEW_OF(&label_blue), EGUI_VIEW_LAYER_DEFAULT);
    update_status("Green");
}

static void btn_blue_top_cb(egui_view_t *self)
{
    EGUI_LOG_INF("btn_blue_top_cb\n");
    egui_view_set_layer(EGUI_VIEW_OF(&label_red), EGUI_VIEW_LAYER_DEFAULT);
    egui_view_set_layer(EGUI_VIEW_OF(&label_green), EGUI_VIEW_LAYER_DEFAULT);
    egui_view_set_layer(EGUI_VIEW_OF(&label_blue), EGUI_VIEW_LAYER_TOP);
    update_status("Blue");
}

// ============== UI init ==============
void uicode_init_ui(void)
{
    // Root group - full screen
    egui_view_group_init(EGUI_VIEW_OF(&root_group));
    egui_view_set_position(EGUI_VIEW_OF(&root_group), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&root_group), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_background_color_init_with_params(EGUI_BG_OF(&bg_root), &bg_root_params);
    egui_view_set_background(EGUI_VIEW_OF(&root_group), EGUI_BG_OF(&bg_root));

    // Title label
    egui_view_label_init(EGUI_VIEW_OF(&label_title));
    egui_view_set_position(EGUI_VIEW_OF(&label_title), 0, 5);
    egui_view_set_size(EGUI_VIEW_OF(&label_title), EGUI_CONFIG_SCEEN_WIDTH, 30);
    egui_view_label_set_text(EGUI_VIEW_OF(&label_title), "Layer Demo");
    egui_view_label_set_font(EGUI_VIEW_OF(&label_title), (const egui_font_t *)&egui_res_font_montserrat_18_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&label_title), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_title), EGUI_ALIGN_CENTER);

    // Overlap container group
    egui_view_group_init(EGUI_VIEW_OF(&overlap_group));
    egui_view_set_position(EGUI_VIEW_OF(&overlap_group), 0, 35);
    egui_view_set_size(EGUI_VIEW_OF(&overlap_group), EGUI_CONFIG_SCEEN_WIDTH, 200);

    // Red label
    egui_view_label_init(EGUI_VIEW_OF(&label_red));
    egui_view_set_position(EGUI_VIEW_OF(&label_red), RED_X, RED_Y);
    egui_view_set_size(EGUI_VIEW_OF(&label_red), OVERLAP_LABEL_W, OVERLAP_LABEL_H);
    egui_view_label_set_text(EGUI_VIEW_OF(&label_red), "Red");
    egui_view_label_set_font(EGUI_VIEW_OF(&label_red), (const egui_font_t *)&egui_res_font_montserrat_20_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&label_red), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_red), EGUI_ALIGN_CENTER);
    egui_background_color_init_with_params(EGUI_BG_OF(&bg_red), &bg_red_params);
    egui_view_set_background(EGUI_VIEW_OF(&label_red), EGUI_BG_OF(&bg_red));

    // Green label
    egui_view_label_init(EGUI_VIEW_OF(&label_green));
    egui_view_set_position(EGUI_VIEW_OF(&label_green), GREEN_X, GREEN_Y);
    egui_view_set_size(EGUI_VIEW_OF(&label_green), OVERLAP_LABEL_W, OVERLAP_LABEL_H);
    egui_view_label_set_text(EGUI_VIEW_OF(&label_green), "Green");
    egui_view_label_set_font(EGUI_VIEW_OF(&label_green), (const egui_font_t *)&egui_res_font_montserrat_20_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&label_green), EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_green), EGUI_ALIGN_CENTER);
    egui_background_color_init_with_params(EGUI_BG_OF(&bg_green), &bg_green_params);
    egui_view_set_background(EGUI_VIEW_OF(&label_green), EGUI_BG_OF(&bg_green));

    // Blue label
    egui_view_label_init(EGUI_VIEW_OF(&label_blue));
    egui_view_set_position(EGUI_VIEW_OF(&label_blue), BLUE_X, BLUE_Y);
    egui_view_set_size(EGUI_VIEW_OF(&label_blue), OVERLAP_LABEL_W, OVERLAP_LABEL_H);
    egui_view_label_set_text(EGUI_VIEW_OF(&label_blue), "Blue");
    egui_view_label_set_font(EGUI_VIEW_OF(&label_blue), (const egui_font_t *)&egui_res_font_montserrat_20_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&label_blue), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_blue), EGUI_ALIGN_CENTER);
    egui_background_color_init_with_params(EGUI_BG_OF(&bg_blue), &bg_blue_params);
    egui_view_set_background(EGUI_VIEW_OF(&label_blue), EGUI_BG_OF(&bg_blue));

    // Add colored labels to overlap group (default layer = 0, last added draws on top)
    egui_view_group_add_child(EGUI_VIEW_OF(&overlap_group), EGUI_VIEW_OF(&label_red));
    egui_view_group_add_child(EGUI_VIEW_OF(&overlap_group), EGUI_VIEW_OF(&label_green));
    egui_view_group_add_child(EGUI_VIEW_OF(&overlap_group), EGUI_VIEW_OF(&label_blue));

    // Set green on top for initial state
    egui_view_set_layer(EGUI_VIEW_OF(&label_green), EGUI_VIEW_LAYER_TOP);
    egui_view_set_layer(EGUI_VIEW_OF(&label_red), EGUI_VIEW_LAYER_DEFAULT);
    egui_view_set_layer(EGUI_VIEW_OF(&label_blue), EGUI_VIEW_LAYER_DEFAULT);

    // Status label - shows which view is currently on top
    egui_view_label_init(EGUI_VIEW_OF(&label_status));
    egui_view_set_position(EGUI_VIEW_OF(&label_status), 0, 240);
    egui_view_set_size(EGUI_VIEW_OF(&label_status), EGUI_CONFIG_SCEEN_WIDTH, 30);
    egui_view_label_set_text(EGUI_VIEW_OF(&label_status), status_str);
    egui_view_label_set_font(EGUI_VIEW_OF(&label_status), (const egui_font_t *)&egui_res_font_montserrat_16_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&label_status), EGUI_THEME_TEXT, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_status), EGUI_ALIGN_CENTER);

    // Button layout - horizontal, centered at bottom
    egui_view_linearlayout_init(EGUI_VIEW_OF(&btn_layout));
    egui_view_set_position(EGUI_VIEW_OF(&btn_layout), 0, 272);
    egui_view_set_size(EGUI_VIEW_OF(&btn_layout), EGUI_CONFIG_SCEEN_WIDTH, BTN_H + 10);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&btn_layout), 1); // horizontal
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&btn_layout), EGUI_ALIGN_CENTER);

    // Red Top button
    egui_view_button_init(EGUI_VIEW_OF(&btn_red_top));
    egui_view_set_size(EGUI_VIEW_OF(&btn_red_top), BTN_W, BTN_H);
    egui_view_set_margin_all(EGUI_VIEW_OF(&btn_red_top), 4);
    egui_view_label_set_text(EGUI_VIEW_OF(&btn_red_top), "Red Top");
    egui_view_label_set_font(EGUI_VIEW_OF(&btn_red_top), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&btn_red_top), EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&btn_red_top), EGUI_ALIGN_CENTER);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&btn_red_top), btn_red_top_cb);

    // Green Top button
    egui_view_button_init(EGUI_VIEW_OF(&btn_green_top));
    egui_view_set_size(EGUI_VIEW_OF(&btn_green_top), BTN_W, BTN_H);
    egui_view_set_margin_all(EGUI_VIEW_OF(&btn_green_top), 4);
    egui_view_label_set_text(EGUI_VIEW_OF(&btn_green_top), "Grn Top");
    egui_view_label_set_font(EGUI_VIEW_OF(&btn_green_top), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&btn_green_top), EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&btn_green_top), EGUI_ALIGN_CENTER);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&btn_green_top), btn_green_top_cb);

    // Blue Top button
    egui_view_button_init(EGUI_VIEW_OF(&btn_blue_top));
    egui_view_set_size(EGUI_VIEW_OF(&btn_blue_top), BTN_W, BTN_H);
    egui_view_set_margin_all(EGUI_VIEW_OF(&btn_blue_top), 4);
    egui_view_label_set_text(EGUI_VIEW_OF(&btn_blue_top), "Blu Top");
    egui_view_label_set_font(EGUI_VIEW_OF(&btn_blue_top), (const egui_font_t *)&egui_res_font_montserrat_12_4);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&btn_blue_top), EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&btn_blue_top), EGUI_ALIGN_CENTER);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&btn_blue_top), btn_blue_top_cb);

    // Add buttons to button layout
    egui_view_group_add_child(EGUI_VIEW_OF(&btn_layout), EGUI_VIEW_OF(&btn_red_top));
    egui_view_group_add_child(EGUI_VIEW_OF(&btn_layout), EGUI_VIEW_OF(&btn_green_top));
    egui_view_group_add_child(EGUI_VIEW_OF(&btn_layout), EGUI_VIEW_OF(&btn_blue_top));
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&btn_layout));

    // Add all to root group
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&label_title));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&overlap_group));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&label_status));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_group), EGUI_VIEW_OF(&btn_layout));

    // Add root to screen
    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_group));
}

void uicode_create_ui(void)
{
    uicode_init_ui();
}

#if EGUI_CONFIG_RECORDING_TEST
// Recording actions: cycle through all 3 layer states
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &btn_red_top, 500);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &btn_green_top, 500);
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &btn_blue_top, 500);
        return true;
    default:
        return false;
    }
}
#endif
