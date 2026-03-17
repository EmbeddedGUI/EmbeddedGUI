#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 image buttons: XS / S / M / L
static egui_view_image_button_t imgbtn_xs;
static egui_view_image_button_t imgbtn_s;
static egui_view_image_button_t imgbtn_m;
static egui_view_image_button_t imgbtn_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 2, EGUI_ALIGN_CENTER);

// Size XS (54x54)
EGUI_VIEW_IMAGE_BUTTON_PARAMS_INIT(imgbtn_xs_params, 0, 0, 60, 60, NULL);
// Size S (68x68)
EGUI_VIEW_IMAGE_BUTTON_PARAMS_INIT(imgbtn_s_params, 0, 0, 72, 72, NULL);
// Size M (82x82)
EGUI_VIEW_IMAGE_BUTTON_PARAMS_INIT(imgbtn_m_params, 0, 0, 84, 84, NULL);
// Size L (96x96)
EGUI_VIEW_IMAGE_BUTTON_PARAMS_INIT(imgbtn_l_params, 0, 0, 96, 96, NULL);

static void image_button_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("Image Button Clicked\n");
}

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all image buttons
    egui_view_image_button_init_with_params(EGUI_VIEW_OF(&imgbtn_xs), &imgbtn_xs_params);
    egui_view_image_button_set_icon(EGUI_VIEW_OF(&imgbtn_xs), EGUI_ICON_MS_PLAY_ARROW);
    egui_view_image_button_set_icon_font(EGUI_VIEW_OF(&imgbtn_xs), EGUI_FONT_ICON_MS_16);
    egui_view_image_button_set_text(EGUI_VIEW_OF(&imgbtn_xs), "Play");
    egui_view_image_button_set_icon_text_gap(EGUI_VIEW_OF(&imgbtn_xs), 1);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&imgbtn_xs), image_button_click_cb);

    egui_view_image_button_init_with_params(EGUI_VIEW_OF(&imgbtn_s), &imgbtn_s_params);
    egui_view_image_button_set_icon(EGUI_VIEW_OF(&imgbtn_s), EGUI_ICON_MS_SEARCH);
    egui_view_image_button_set_icon_font(EGUI_VIEW_OF(&imgbtn_s), EGUI_FONT_ICON_MS_20);
    egui_view_image_button_set_text(EGUI_VIEW_OF(&imgbtn_s), "Find");
    egui_view_image_button_set_icon_text_gap(EGUI_VIEW_OF(&imgbtn_s), 2);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&imgbtn_s), image_button_click_cb);

    egui_view_image_button_init_with_params(EGUI_VIEW_OF(&imgbtn_m), &imgbtn_m_params);
    egui_view_image_button_set_icon(EGUI_VIEW_OF(&imgbtn_m), EGUI_ICON_MS_SYNC);
    egui_view_image_button_set_icon_font(EGUI_VIEW_OF(&imgbtn_m), EGUI_FONT_ICON_MS_20);
    egui_view_image_button_set_text(EGUI_VIEW_OF(&imgbtn_m), "Sync");
    egui_view_image_button_set_icon_text_gap(EGUI_VIEW_OF(&imgbtn_m), 2);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&imgbtn_m), image_button_click_cb);

    egui_view_image_button_init_with_params(EGUI_VIEW_OF(&imgbtn_l), &imgbtn_l_params);
    egui_view_image_button_set_icon(EGUI_VIEW_OF(&imgbtn_l), EGUI_ICON_MS_HEART);
    egui_view_image_button_set_icon_font(EGUI_VIEW_OF(&imgbtn_l), EGUI_FONT_ICON_MS_24);
    egui_view_image_button_set_text(EGUI_VIEW_OF(&imgbtn_l), "Like");
    egui_view_image_button_set_icon_text_gap(EGUI_VIEW_OF(&imgbtn_l), 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&imgbtn_l), image_button_click_cb);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&imgbtn_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&imgbtn_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&imgbtn_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&imgbtn_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&imgbtn_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&imgbtn_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&imgbtn_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&imgbtn_l));

    // Layout grid children
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));

    // Add grid to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));

    // Center grid on screen
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &imgbtn_xs, 1000);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &imgbtn_s, 1000);
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &imgbtn_m, 1000);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &imgbtn_l, 1000);
        return true;
    default:
        return false;
    }
}
#endif
