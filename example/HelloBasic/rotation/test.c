#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"

// Labels to show rotation state
static egui_view_label_t label_title;
static egui_view_label_t label_angle;
static egui_view_label_t label_top_left;
static egui_view_label_t label_bottom_right;

// Button to switch rotation
static egui_view_button_t btn_rotate;

// Layout container
static egui_view_gridlayout_t grid;

// Current rotation index (0-3)
static int rotation_index = 0;

// Text buffer for angle display
static char angle_text[32];

// Rotation values
static const egui_display_rotation_t rotations[] = {
        EGUI_DISPLAY_ROTATION_0,
        EGUI_DISPLAY_ROTATION_90,
        EGUI_DISPLAY_ROTATION_180,
        EGUI_DISPLAY_ROTATION_270,
};

static const char *rotation_names[] = {
        "0 deg",
        "90 deg",
        "180 deg",
        "270 deg",
};

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 200, 120, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

EGUI_VIEW_LABEL_PARAMS_INIT(label_title_params, 0, 0, 200, 30, "Rotation Test", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_angle_params, 0, 0, 200, 40, "0 deg", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_PRIMARY, EGUI_ALPHA_100);

// Corner labels
EGUI_VIEW_LABEL_PARAMS_INIT(label_tl_params, 0, 0, 60, 20, "TL", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_br_params, 0, 0, 60, 20, "BR", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);

// Button params - placed at bottom-left area
EGUI_VIEW_BUTTON_PARAMS_INIT(btn_rotate_params, 0, 0, 100, 36, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);

static void apply_rotation(int index)
{
    rotation_index = index % 4;
    egui_display_set_rotation(rotations[rotation_index]);

    // Update angle label text
    snprintf(angle_text, sizeof(angle_text), "%s", rotation_names[rotation_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&label_angle), angle_text);

    // Reposition views based on current logical screen size
    int16_t sw = egui_display_get_width();
    int16_t sh = egui_display_get_height();

    // Corner labels
    egui_view_set_position(EGUI_VIEW_OF(&label_top_left), 2, 2);
    egui_view_set_position(EGUI_VIEW_OF(&label_bottom_right), sw - 62, sh - 22);

    // Center grid
    egui_view_set_position(EGUI_VIEW_OF(&grid), (sw - 200) / 2, (sh - 120) / 2 - 20);

    // Button at bottom-left
    egui_view_set_position(EGUI_VIEW_OF(&btn_rotate), 10, sh - 46);
}

static void btn_rotate_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("Rotate button clicked, switching rotation\n");
    apply_rotation(rotation_index + 1);
}

void test_init_ui(void)
{
    // Init grid container
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init center labels
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_title), &label_title_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_title), EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_set_margin_all(EGUI_VIEW_OF(&label_title), 6);

    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_angle), &label_angle_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_angle), EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
    egui_view_set_margin_all(EGUI_VIEW_OF(&label_angle), 6);

    // Add to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&label_title));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&label_angle));

    // Layout grid
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));

    // Init corner labels
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_top_left), &label_tl_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_top_left), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);

    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_bottom_right), &label_br_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_bottom_right), EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER);

    // Init rotate button at bottom-left
    egui_view_button_init_with_params(EGUI_VIEW_OF(&btn_rotate), &btn_rotate_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&btn_rotate), "Rotate");
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&btn_rotate), btn_rotate_click_cb);

    // Add all to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_top_left));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_bottom_right));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&btn_rotate));

    // Apply initial rotation (0 deg)
    apply_rotation(0);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    // Pattern: CLICK then WAIT to let egui thread process rotation and re-render
    switch (action_index)
    {
    case 0:
        // Capture initial 0 deg state
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 1:
        // Click button to rotate to 90 deg
        EGUI_SIM_SET_CLICK_VIEW(p_action, &btn_rotate, 500);
        return true;
    case 2:
        // Wait for 90 deg render to complete
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 3:
        // Click button to rotate to 180 deg
        EGUI_SIM_SET_CLICK_VIEW(p_action, &btn_rotate, 500);
        return true;
    case 4:
        // Wait for 180 deg render to complete
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 5:
        // Click button to rotate to 270 deg
        EGUI_SIM_SET_CLICK_VIEW(p_action, &btn_rotate, 500);
        return true;
    case 6:
        // Wait for 270 deg render to complete
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    default:
        return false;
    }
}
#endif
