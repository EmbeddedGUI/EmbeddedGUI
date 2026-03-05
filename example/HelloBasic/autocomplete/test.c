#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"

static egui_view_gridlayout_t grid;
static egui_view_autocomplete_t autocomplete_view;
static egui_view_label_t status_label;
static char status_text[40];
static uint8_t clip_fail_reported = 0;

static const char *suggestions[] = {
        "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta",
};

EGUI_VIEW_AUTOCOMPLETE_PARAMS_INIT(autocomplete_params, 0, 0, 210, 34, suggestions, 6, 0);
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 228, 200, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
EGUI_VIEW_LABEL_PARAMS_INIT(status_label_params, 0, 0, 210, 24, "", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);

static void update_status(uint8_t index)
{
    const char *text = "";
    if (index < sizeof(suggestions) / sizeof(suggestions[0]))
    {
        text = suggestions[index];
    }
    snprintf(status_text, sizeof(status_text), "Pick #%u: %s", (unsigned int)(index + 1U), text);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
}

static void on_selected(egui_view_t *self, uint8_t index)
{
    (void)self;
    EGUI_LOG_INF("autocomplete: selected index=%d\r\n", index);
    update_status(index);
}

void test_init_ui(void)
{
    clip_fail_reported = 0;
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    egui_view_autocomplete_init_with_params(EGUI_VIEW_OF(&autocomplete_view), &autocomplete_params);
    egui_view_autocomplete_set_on_selected_listener(EGUI_VIEW_OF(&autocomplete_view), on_selected);
    egui_view_autocomplete_set_max_visible_items(EGUI_VIEW_OF(&autocomplete_view), 4);
    egui_view_set_margin_all(EGUI_VIEW_OF(&autocomplete_view), 6);

    egui_view_label_init_with_params(EGUI_VIEW_OF(&status_label), &status_label_params);
    egui_view_set_margin_all(EGUI_VIEW_OF(&status_label), 6);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    update_status(0);

    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&autocomplete_view));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&status_label));
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
static void report_if_clipped(const char *name, egui_view_t *view)
{
    int clipped_w = (view->region_screen.size.width < view->region.size.width);
    int clipped_h = (view->region_screen.size.height < view->region.size.height);
    if (!clipped_w && !clipped_h)
    {
        return;
    }

    clip_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s clipped: screen(%d,%d) logic(%d,%d)\n",
           name,
           (int)view->region_screen.size.width,
           (int)view->region_screen.size.height,
           (int)view->region.size.width,
           (int)view->region.size.height);
}

static void report_if_layout_clipped(void)
{
    if (clip_fail_reported)
    {
        return;
    }
    report_if_clipped("autocomplete", EGUI_VIEW_OF(&autocomplete_view));
    report_if_clipped("autocomplete_status", EGUI_VIEW_OF(&status_label));
}

static void report_if_expanded_clipped(void)
{
    if (clip_fail_reported)
    {
        return;
    }

    egui_view_t *view = EGUI_VIEW_OF(&autocomplete_view);
    egui_view_combobox_t *local = (egui_view_combobox_t *)view;
    if (local->is_expanded && view->region_screen.size.height < view->region.size.height)
    {
        clip_fail_reported = 1;
        printf("[RUNTIME_CHECK_FAIL] autocomplete expanded clipped: screen_h=%d logic_h=%d\n",
               (int)view->region_screen.size.height,
               (int)view->region.size.height);
    }
}

static void get_item_center(uint8_t item_index, int *x, int *y)
{
    egui_view_t *view = EGUI_VIEW_OF(&autocomplete_view);
    egui_view_combobox_t *local = (egui_view_combobox_t *)view;
    report_if_expanded_clipped();
    int left = view->region_screen.location.x;
    int top = view->region_screen.location.y;
    int width = view->region_screen.size.width;
    int header_height = local->collapsed_height;
    int item_height = local->item_height;

    *x = left + width / 2;
    *y = top + header_height + item_index * item_height + item_height / 2;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    int x = 0;
    int y = 0;
    report_if_layout_clipped();

    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &autocomplete_view, 600);
        return true;
    case 1:
        get_item_center(2, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 700;
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &autocomplete_view, 600);
        return true;
    case 3:
        get_item_center(0, &x, &y);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x;
        p_action->y1 = y;
        p_action->interval_ms = 700;
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    default:
        return false;
    }
}
#endif
