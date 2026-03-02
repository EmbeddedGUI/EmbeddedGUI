#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// ViewPage with 3 colored pages + PageIndicator at bottom

static egui_view_viewpage_t viewpage;
static egui_view_label_t page_1;
static egui_view_label_t page_2;
static egui_view_label_t page_3;
static egui_view_page_indicator_t indicator;

// Page backgrounds
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_1_param, EGUI_THEME_SURFACE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_1_params, &bg_1_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_1, &bg_1_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_2_param, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_2_params, &bg_2_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_2, &bg_2_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_3_param, EGUI_THEME_TRACK_BG, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_3_params, &bg_3_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_3, &bg_3_params);

#define PAGE_COUNT       3
#define INDICATOR_HEIGHT 20
#define PAGE_WIDTH       EGUI_CONFIG_SCEEN_WIDTH
#define PAGE_HEIGHT      (EGUI_CONFIG_SCEEN_HEIGHT - INDICATOR_HEIGHT)

// View params
EGUI_VIEW_VIEWPAGE_PARAMS_INIT(viewpage_params, 0, 0, PAGE_WIDTH, PAGE_HEIGHT);
EGUI_VIEW_LABEL_PARAMS_INIT(page_1_params, 0, 0, PAGE_WIDTH, PAGE_HEIGHT, "Page 1", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(page_2_params, 0, 0, PAGE_WIDTH, PAGE_HEIGHT, "Page 2", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(page_3_params, 0, 0, PAGE_WIDTH, PAGE_HEIGHT, "Page 3", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_PAGE_INDICATOR_PARAMS_INIT(indicator_params, 0, 0, PAGE_WIDTH, INDICATOR_HEIGHT, PAGE_COUNT, 0);

// Timer to sync indicator with viewpage
static egui_timer_t sync_timer;

static void sync_timer_callback(egui_timer_t *timer)
{
    egui_view_viewpage_t *vp = (egui_view_viewpage_t *)&viewpage;
    egui_view_page_indicator_set_current_index(EGUI_VIEW_OF(&indicator), vp->current_page_index);
}

void test_init_ui(void)
{
    // Init viewpage
    egui_view_viewpage_init_with_params(EGUI_VIEW_OF(&viewpage), &viewpage_params);

    // Init page labels
    egui_view_label_init_with_params(EGUI_VIEW_OF(&page_1), &page_1_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&page_2), &page_2_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&page_3), &page_3_params);

    // Set backgrounds
    egui_view_set_background(EGUI_VIEW_OF(&page_1), EGUI_BG_OF(&bg_1));
    egui_view_set_background(EGUI_VIEW_OF(&page_2), EGUI_BG_OF(&bg_2));
    egui_view_set_background(EGUI_VIEW_OF(&page_3), EGUI_BG_OF(&bg_3));

    // Add pages to viewpage
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page_1));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page_2));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page_3));
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&viewpage));

    // Init indicator
    egui_view_page_indicator_init_with_params(EGUI_VIEW_OF(&indicator), &indicator_params);

    // Add to root: viewpage on top, indicator at bottom
    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&indicator));

    // Layout root children vertically
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP);

    // Start sync timer (100ms interval)
    egui_timer_init_timer(&sync_timer, NULL, sync_timer_callback);
    egui_timer_start_timer(&sync_timer, 100, 100);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0: // swipe left -> page 2
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 5;
        p_action->interval_ms = 1500;
        return true;
    case 1: // swipe left -> page 3
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 5;
        p_action->interval_ms = 1500;
        return true;
    case 2: // swipe right -> page 2
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 5;
        p_action->interval_ms = 1500;
        return true;
    case 3: // swipe right -> page 1
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 5;
        p_action->interval_ms = 1500;
        return true;
    default:
        return false;
    }
}
#endif
