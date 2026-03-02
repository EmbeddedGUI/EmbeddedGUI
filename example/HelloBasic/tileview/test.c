#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// TileView
static egui_view_tileview_t tileview_1;

// Tiles (each is a label filling the full screen)
static egui_view_label_t tile_label_00;
static egui_view_label_t tile_label_10;
static egui_view_label_t tile_label_01;
static egui_view_label_t tile_label_11;

// Backgrounds for each tile
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_00_param, EGUI_THEME_SURFACE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_00_params, &bg_00_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_00, &bg_00_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_10_param, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_10_params, &bg_10_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_10, &bg_10_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_01_param, EGUI_THEME_TRACK_BG, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_01_params, &bg_01_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_01, &bg_01_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_11_param, EGUI_THEME_PRIMARY_DARK, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_11_params, &bg_11_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_11, &bg_11_params);

#define TILE_WIDTH  EGUI_CONFIG_SCEEN_WIDTH
#define TILE_HEIGHT EGUI_CONFIG_SCEEN_HEIGHT

// View params
EGUI_VIEW_TILEVIEW_PARAMS_INIT(tileview_1_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
EGUI_VIEW_LABEL_PARAMS_INIT(tile_label_00_params, 0, 0, TILE_WIDTH, TILE_HEIGHT, "Tile 0,0", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(tile_label_10_params, 0, 0, TILE_WIDTH, TILE_HEIGHT, "Tile 1,0", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(tile_label_01_params, 0, 0, TILE_WIDTH, TILE_HEIGHT, "Tile 0,1", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(tile_label_11_params, 0, 0, TILE_WIDTH, TILE_HEIGHT, "Tile 1,1", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

void test_init_ui(void)
{
    // Init tileview
    egui_view_tileview_init_with_params(EGUI_VIEW_OF(&tileview_1), &tileview_1_params);

    // Init tile labels
    egui_view_label_init_with_params(EGUI_VIEW_OF(&tile_label_00), &tile_label_00_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&tile_label_10), &tile_label_10_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&tile_label_01), &tile_label_01_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&tile_label_11), &tile_label_11_params);

    // Set backgrounds
    egui_view_set_background(EGUI_VIEW_OF(&tile_label_00), EGUI_BG_OF(&bg_00));
    egui_view_set_background(EGUI_VIEW_OF(&tile_label_10), EGUI_BG_OF(&bg_10));
    egui_view_set_background(EGUI_VIEW_OF(&tile_label_01), EGUI_BG_OF(&bg_01));
    egui_view_set_background(EGUI_VIEW_OF(&tile_label_11), EGUI_BG_OF(&bg_11));

    // Add tiles at grid positions: (col, row)
    egui_view_tileview_add_tile(EGUI_VIEW_OF(&tileview_1), EGUI_VIEW_OF(&tile_label_00), 0, 0);
    egui_view_tileview_add_tile(EGUI_VIEW_OF(&tileview_1), EGUI_VIEW_OF(&tile_label_10), 1, 0);
    egui_view_tileview_add_tile(EGUI_VIEW_OF(&tileview_1), EGUI_VIEW_OF(&tile_label_01), 0, 1);
    egui_view_tileview_add_tile(EGUI_VIEW_OF(&tileview_1), EGUI_VIEW_OF(&tile_label_11), 1, 1);

    // Add to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&tileview_1));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0: // swipe left -> Tile 1,0
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 5;
        p_action->interval_ms = 800;
        return true;
    case 1: // swipe down -> Tile 1,1
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 4;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 4;
        p_action->steps = 5;
        p_action->interval_ms = 800;
        return true;
    case 2: // swipe right -> Tile 0,1
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 5;
        p_action->interval_ms = 800;
        return true;
    case 3: // swipe up -> Tile 0,0
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 4;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 4;
        p_action->steps = 5;
        p_action->interval_ms = 800;
        return true;
    default:
        return false;
    }
}
#endif
