#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 cards: XS / S / M / L, each with 2 labels inside
static egui_view_card_t card_xs;
static egui_view_card_t card_s;
static egui_view_card_t card_m;
static egui_view_card_t card_l;

EGUI_SHADOW_PARAM_INIT_ROUND(card_shadow, 8, 0, 2, EGUI_COLOR_BLACK, EGUI_ALPHA_20, 8);

// Labels for card XS
static egui_view_label_t label_title_xs;
static egui_view_label_t label_desc_xs;

// Labels for card S
static egui_view_label_t label_title_s;
static egui_view_label_t label_desc_s;

// Labels for card M
static egui_view_label_t label_title_m;
static egui_view_label_t label_desc_m;

// Labels for card L
static egui_view_label_t label_title_l;
static egui_view_label_t label_desc_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

// Size XS (90x36)
EGUI_VIEW_CARD_PARAMS_INIT(card_xs_params, 0, 0, 90, 36, 6);
// Size S (100x48)
EGUI_VIEW_CARD_PARAMS_INIT(card_s_params, 0, 0, 100, 48, 6);
// Size M (100x60)
EGUI_VIEW_CARD_PARAMS_INIT(card_m_params, 0, 0, 100, 60, 8);
// Size L (100x76)
EGUI_VIEW_CARD_PARAMS_INIT(card_l_params, 0, 0, 100, 76, 8);

// Label params for each card
EGUI_VIEW_LABEL_PARAMS_INIT(label_title_xs_params, 0, 0, 70, 12, "XS Card", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_desc_xs_params, 0, 0, 70, 12, "Surface", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);

EGUI_VIEW_LABEL_PARAMS_INIT(label_title_s_params, 0, 0, 80, 15, "S Card", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_desc_s_params, 0, 0, 80, 15, "Surface", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);

EGUI_VIEW_LABEL_PARAMS_INIT(label_title_m_params, 0, 0, 80, 18, "M Card", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_desc_m_params, 0, 0, 80, 18, "Surface", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);

EGUI_VIEW_LABEL_PARAMS_INIT(label_title_l_params, 0, 0, 80, 22, "L Card", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_desc_l_params, 0, 0, 80, 22, "Surface", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init card XS
    egui_view_card_init_with_params(EGUI_VIEW_OF(&card_xs), &card_xs_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_title_xs), &label_title_xs_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_desc_xs), &label_desc_xs_params);
    egui_view_set_margin(EGUI_VIEW_OF(&label_title_xs), 5, 3, 5, 1);
    egui_view_set_margin(EGUI_VIEW_OF(&label_desc_xs), 5, 1, 5, 3);
    egui_view_card_add_child(EGUI_VIEW_OF(&card_xs), EGUI_VIEW_OF(&label_title_xs));
    egui_view_card_add_child(EGUI_VIEW_OF(&card_xs), EGUI_VIEW_OF(&label_desc_xs));
    egui_view_card_layout_childs(EGUI_VIEW_OF(&card_xs), 0, EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP);
    egui_view_set_shadow(EGUI_VIEW_OF(&card_xs), &card_shadow);

    // Init card S
    egui_view_card_init_with_params(EGUI_VIEW_OF(&card_s), &card_s_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_title_s), &label_title_s_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_desc_s), &label_desc_s_params);
    egui_view_set_margin(EGUI_VIEW_OF(&label_title_s), 5, 5, 5, 2);
    egui_view_set_margin(EGUI_VIEW_OF(&label_desc_s), 5, 2, 5, 5);
    egui_view_card_add_child(EGUI_VIEW_OF(&card_s), EGUI_VIEW_OF(&label_title_s));
    egui_view_card_add_child(EGUI_VIEW_OF(&card_s), EGUI_VIEW_OF(&label_desc_s));
    egui_view_card_layout_childs(EGUI_VIEW_OF(&card_s), 0, EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP);
    egui_view_set_shadow(EGUI_VIEW_OF(&card_s), &card_shadow);

    // Init card M
    egui_view_card_init_with_params(EGUI_VIEW_OF(&card_m), &card_m_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_title_m), &label_title_m_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_desc_m), &label_desc_m_params);
    egui_view_set_margin(EGUI_VIEW_OF(&label_title_m), 5, 5, 5, 2);
    egui_view_set_margin(EGUI_VIEW_OF(&label_desc_m), 5, 2, 5, 5);
    egui_view_card_add_child(EGUI_VIEW_OF(&card_m), EGUI_VIEW_OF(&label_title_m));
    egui_view_card_add_child(EGUI_VIEW_OF(&card_m), EGUI_VIEW_OF(&label_desc_m));
    egui_view_card_layout_childs(EGUI_VIEW_OF(&card_m), 0, EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP);
    egui_view_set_shadow(EGUI_VIEW_OF(&card_m), &card_shadow);

    // Init card L
    egui_view_card_init_with_params(EGUI_VIEW_OF(&card_l), &card_l_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_title_l), &label_title_l_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_desc_l), &label_desc_l_params);
    egui_view_set_margin(EGUI_VIEW_OF(&label_title_l), 5, 5, 5, 2);
    egui_view_set_margin(EGUI_VIEW_OF(&label_desc_l), 5, 2, 5, 5);
    egui_view_card_add_child(EGUI_VIEW_OF(&card_l), EGUI_VIEW_OF(&label_title_l));
    egui_view_card_add_child(EGUI_VIEW_OF(&card_l), EGUI_VIEW_OF(&label_desc_l));
    egui_view_card_layout_childs(EGUI_VIEW_OF(&card_l), 0, EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP);
    egui_view_set_shadow(EGUI_VIEW_OF(&card_l), &card_shadow);

    // Set card margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&card_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&card_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&card_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&card_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&card_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&card_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&card_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&card_l));

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
    if (action_index >= 1)
    {
        return false;
    }
    EGUI_SIM_SET_WAIT(p_action, 1500);
    return true;
}
#endif
