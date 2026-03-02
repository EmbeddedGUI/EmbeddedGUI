#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "egui_view_page_test.h"

// views in root
static egui_view_viewpage_cache_t viewpage_1;

// View params
EGUI_VIEW_VIEWPAGE_CACHE_PARAMS_INIT(viewpage_1_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

void *on_page_load_listener(egui_view_t *self, int current_page_index)
{
    EGUI_LOG_INF("on_page_load_listener, current_page_index: %d\r\n", current_page_index);
    egui_view_page_test_t *p_page = (egui_view_page_test_t *)egui_api_malloc(sizeof(egui_view_page_test_t));
    egui_view_page_test_init(EGUI_VIEW_OF(p_page));
    egui_view_page_test_set_index(EGUI_VIEW_OF(p_page), current_page_index);
    return p_page;
}

void on_page_free_listener(egui_view_t *self, int current_page_index, egui_view_t *page)
{
    EGUI_LOG_INF("on_page_free_listener, current_page_index: %d\r\n", current_page_index);
    egui_api_free(page);
}

void test_init_ui(void)
{
    // Init all views
    // viewpage_1
    egui_view_viewpage_cache_init_with_params(EGUI_VIEW_OF(&viewpage_1), &viewpage_1_params);
    egui_view_viewpage_cache_set_child_total_cnt(EGUI_VIEW_OF(&viewpage_1), 10);
    egui_view_viewpage_cache_set_on_page_load_listener(EGUI_VIEW_OF(&viewpage_1), on_page_load_listener);
    egui_view_viewpage_cache_set_on_page_free_listener(EGUI_VIEW_OF(&viewpage_1), on_page_free_listener);

    egui_view_viewpage_cache_set_current_page(EGUI_VIEW_OF(&viewpage_1), 0);

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage_1));
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
        p_action->interval_ms = 800;
        return true;
    case 1: // swipe left -> page 3
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 5;
        p_action->interval_ms = 800;
        return true;
    case 2: // swipe right -> page 2
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 5;
        p_action->interval_ms = 800;
        return true;
    case 3: // swipe right -> page 1
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 5;
        p_action->interval_ms = 800;
        return true;
    default:
        return false;
    }
}
#endif
