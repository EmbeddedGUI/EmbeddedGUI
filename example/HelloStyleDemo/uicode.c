#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "style/egui_theme.h"

// ViewPage for 4 pages
static egui_view_viewpage_t viewpage;
EGUI_VIEW_VIEWPAGE_PARAMS_INIT(viewpage_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

// 4 page containers (groups)
static egui_view_group_t page_smarthome;
static egui_view_group_t page_music;
static egui_view_group_t page_dashboard;
static egui_view_group_t page_watch;

EGUI_VIEW_GROUP_PARAMS_INIT(page_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

// Background colors for pages
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_smarthome_param, EGUI_COLOR_MAKE(0xF0, 0xF4, 0xF8), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_smarthome_params, &bg_smarthome_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_smarthome, &bg_smarthome_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_music_param, EGUI_COLOR_MAKE(0x1A, 0x1A, 0x2E), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_music_params, &bg_music_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_music, &bg_music_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_dashboard_param, EGUI_COLOR_MAKE(0xF0, 0xF4, 0xF8), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_dashboard_params, &bg_dashboard_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_dashboard, &bg_dashboard_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_watch_param, EGUI_COLOR_MAKE(0x0A, 0x0A, 0x1A), EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_watch_params, &bg_watch_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_watch, &bg_watch_params);

// Page change callback
static void viewpage_on_page_changed(egui_view_t *self, int page_index)
{
    (void)self;
    switch (page_index)
    {
    case 0:
        uicode_page_smarthome_on_enter();
        break;
    case 1:
        uicode_page_music_on_enter();
        break;
    case 2:
        uicode_page_dashboard_on_enter();
        break;
    case 3:
        uicode_page_watch_on_enter();
        break;
    default:
        break;
    }
}

// Theme state
static uint8_t is_dark_theme = 0;

uint8_t uicode_is_dark_theme(void)
{
    return is_dark_theme;
}

void uicode_toggle_theme(void)
{
    if (is_dark_theme)
    {
        egui_theme_set(&egui_theme_light);
        is_dark_theme = 0;
    }
    else
    {
        egui_theme_set(&egui_theme_dark);
        is_dark_theme = 1;
    }
    uicode_update_theme_icons();
}

void uicode_update_theme_icons(void)
{
    uicode_page_smarthome_update_theme_icon();
    uicode_page_dashboard_update_theme_icon();
    uicode_page_watch_update_theme_icon();
}

void uicode_create_ui(void)
{
    // Init ViewPage
    egui_view_viewpage_init_with_params(EGUI_VIEW_OF(&viewpage), &viewpage_params);

    // Init 4 page groups
    egui_view_group_init_with_params(EGUI_VIEW_OF(&page_smarthome), &page_params);
    egui_view_group_init_with_params(EGUI_VIEW_OF(&page_music), &page_params);
    egui_view_group_init_with_params(EGUI_VIEW_OF(&page_dashboard), &page_params);
    egui_view_group_init_with_params(EGUI_VIEW_OF(&page_watch), &page_params);

    // Set backgrounds
    egui_view_set_background(EGUI_VIEW_OF(&page_smarthome), EGUI_BG_OF(&bg_smarthome));
    egui_view_set_background(EGUI_VIEW_OF(&page_music), EGUI_BG_OF(&bg_music));
    egui_view_set_background(EGUI_VIEW_OF(&page_dashboard), EGUI_BG_OF(&bg_dashboard));
    egui_view_set_background(EGUI_VIEW_OF(&page_watch), EGUI_BG_OF(&bg_watch));

    // Init page contents
    uicode_init_page_smarthome(EGUI_VIEW_OF(&page_smarthome));
    uicode_init_page_music(EGUI_VIEW_OF(&page_music));
    uicode_init_page_dashboard(EGUI_VIEW_OF(&page_dashboard));
    uicode_init_page_watch(EGUI_VIEW_OF(&page_watch));

    // Add pages to ViewPage
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page_smarthome));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page_music));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page_dashboard));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&page_watch));

    // Layout and add to root
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&viewpage));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage));

    // Register page change callback and trigger first page entry animation
    egui_view_viewpage_set_on_page_changed(EGUI_VIEW_OF(&viewpage), viewpage_on_page_changed);
    uicode_page_smarthome_on_enter();
}

#if EGUI_CONFIG_RECORDING_TEST
static int recording_page_index = 0;

static void recording_switch_to_next_page(void)
{
    recording_page_index++;
    if (recording_page_index < 4)
    {
        egui_view_viewpage_set_current_page(EGUI_VIEW_OF(&viewpage), recording_page_index);
    }
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);
    last_action = action_index;

    switch (action_index)
    {
    case 0: // Smart Home - card stagger entrance ~600ms
        if (first_call)
            recording_request_snapshot();
        EGUI_SIM_SET_WAIT(p_action, 1000);
        return true;
    case 1: // Music - fade in + auto playback ~500ms
        if (first_call)
        {
            recording_switch_to_next_page();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 800);
        return true;
    case 2: // Dashboard - chart growth animation ~800ms
        if (first_call)
        {
            recording_switch_to_next_page();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 1200);
        return true;
    case 3: // Watch - ring growth ~1000ms + weather fade 500ms
        if (first_call)
        {
            recording_switch_to_next_page();
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 1500);
        return true;
    default:
        return false;
    }
}
#endif
