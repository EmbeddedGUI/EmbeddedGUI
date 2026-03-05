#include "egui.h"
#include <stdio.h>
#include <string.h>
#include "uicode.h"

static egui_view_gridlayout_t root_grid;
static egui_view_pattern_lock_t pattern_lock_view;
static egui_view_label_t status_label;
static egui_view_button_t clear_button;
static char status_text[72];
#if EGUI_CONFIG_RECORDING_TEST
static uint8_t clip_fail_reported = 0;
#endif

EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(root_grid_params, 0, 0, 228, 320, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
EGUI_VIEW_PATTERN_LOCK_PARAMS_INIT(pattern_lock_params, 0, 0, 216, 216, 3, 7);
EGUI_VIEW_LABEL_PARAMS_INIT(status_label_params, 0, 0, 216, 28, "", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_BUTTON_PARAMS_INIT(clear_button_params, 0, 0, 140, 34, "Clear Pattern", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);

static void set_status_text(const char *text, egui_color_t color)
{
    snprintf(status_text, sizeof(status_text), "%s", text);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), color, EGUI_ALPHA_100);
}

static void build_pattern_text(char *out, size_t out_size, const uint8_t *nodes, uint8_t count)
{
    size_t used = 0;
    if (out_size == 0)
    {
        return;
    }

    used += (size_t)snprintf(out + used, out_size - used, "Pattern:");
    uint8_t i;
    for (i = 0; i < count; i++)
    {
        if (used >= out_size)
        {
            break;
        }
        used += (size_t)snprintf(out + used, out_size - used, "%s%u", (i == 0) ? " " : "-", (unsigned int)nodes[i]);
    }
}

static void on_pattern_complete(egui_view_t *self, uint8_t node_count)
{
    (void)self;
    EGUI_LOG_INF("pattern_lock completed, node_count=%d\r\n", node_count);
}

static void on_pattern_finish(egui_view_t *self, const uint8_t *nodes, uint8_t node_count, uint8_t valid)
{
    (void)self;
    if (!valid)
    {
        snprintf(status_text, sizeof(status_text), "Need >= %u nodes (now %u)",
                 (unsigned int)egui_view_pattern_lock_get_min_nodes(EGUI_VIEW_OF(&pattern_lock_view)), (unsigned int)node_count);
        egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_THEME_DANGER, EGUI_ALPHA_100);
        EGUI_LOG_INF("pattern_lock invalid, node_count=%d\r\n", node_count);
        return;
    }

    build_pattern_text(status_text, sizeof(status_text), nodes, node_count);
    egui_view_label_set_text(EGUI_VIEW_OF(&status_label), status_text);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&status_label), EGUI_THEME_SUCCESS, EGUI_ALPHA_100);
    EGUI_LOG_INF("pattern_lock finish valid, node_count=%d\r\n", node_count);
}

static void on_clear_click(egui_view_t *self)
{
    (void)self;
    egui_view_pattern_lock_clear_pattern(EGUI_VIEW_OF(&pattern_lock_view));
    set_status_text("Pattern cleared", EGUI_THEME_TEXT_SECONDARY);
}

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    clip_fail_reported = 0;
#endif
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&root_grid), &root_grid_params);

    egui_view_pattern_lock_init_with_params(EGUI_VIEW_OF(&pattern_lock_view), &pattern_lock_params);
    egui_view_pattern_lock_set_on_pattern_complete_listener(EGUI_VIEW_OF(&pattern_lock_view), on_pattern_complete);
    egui_view_pattern_lock_set_on_pattern_finish_listener(EGUI_VIEW_OF(&pattern_lock_view), on_pattern_finish);
    egui_view_pattern_lock_set_bg_color(EGUI_VIEW_OF(&pattern_lock_view), EGUI_THEME_SURFACE);
    egui_view_pattern_lock_set_border_color(EGUI_VIEW_OF(&pattern_lock_view), EGUI_THEME_TRACK_OFF);
    egui_view_pattern_lock_set_node_color(EGUI_VIEW_OF(&pattern_lock_view), EGUI_THEME_TRACK_OFF);
    egui_view_pattern_lock_set_active_node_color(EGUI_VIEW_OF(&pattern_lock_view), EGUI_THEME_PRIMARY_LIGHT);
    egui_view_pattern_lock_set_line_color(EGUI_VIEW_OF(&pattern_lock_view), EGUI_THEME_SECONDARY);
    egui_view_pattern_lock_set_error_color(EGUI_VIEW_OF(&pattern_lock_view), EGUI_THEME_DANGER);
    egui_view_set_margin_all(EGUI_VIEW_OF(&pattern_lock_view), 4);

    egui_view_label_init_with_params(EGUI_VIEW_OF(&status_label), &status_label_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&status_label), EGUI_ALIGN_CENTER | EGUI_ALIGN_VCENTER);
    egui_view_set_margin_all(EGUI_VIEW_OF(&status_label), 6);
    set_status_text("Draw at least 3 nodes", EGUI_THEME_TEXT_SECONDARY);

    egui_view_button_init_with_params(EGUI_VIEW_OF(&clear_button), &clear_button_params);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&clear_button), on_clear_click);
    egui_view_set_margin_all(EGUI_VIEW_OF(&clear_button), 6);

    egui_view_group_add_child(EGUI_VIEW_OF(&root_grid), EGUI_VIEW_OF(&pattern_lock_view));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_grid), EGUI_VIEW_OF(&status_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&root_grid), EGUI_VIEW_OF(&clear_button));
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&root_grid));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root_grid));
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
    printf("[RUNTIME_CHECK_FAIL] %s clipped: screen(%d,%d) logic(%d,%d)\n", name, (int)view->region_screen.size.width, (int)view->region_screen.size.height,
           (int)view->region.size.width, (int)view->region.size.height);
}

static void report_if_layout_clipped(void)
{
    if (clip_fail_reported)
    {
        return;
    }
    report_if_clipped("pattern_lock", EGUI_VIEW_OF(&pattern_lock_view));
    report_if_clipped("status_label", EGUI_VIEW_OF(&status_label));
    report_if_clipped("clear_button", EGUI_VIEW_OF(&clear_button));
}

static void get_node_center(uint8_t node_index, int *x, int *y)
{
    egui_view_t *view = EGUI_VIEW_OF(&pattern_lock_view);
    int step_x = view->region_screen.size.width / 3;
    int step_y = view->region_screen.size.height / 3;
    int col = node_index % 3;
    int row = node_index / 3;
    *x = view->region_screen.location.x + step_x / 2 + col * step_x;
    *y = view->region_screen.location.y + step_y / 2 + row * step_y;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    int x1, y1, x2, y2;
    report_if_layout_clipped();
    switch (action_index)
    {
    case 0:
        get_node_center(0, &x1, &y1);
        get_node_center(2, &x2, &y2);
        p_action->type = EGUI_SIM_ACTION_DRAG;
        p_action->x1 = x1;
        p_action->y1 = y1;
        p_action->x2 = x2;
        p_action->y2 = y2;
        p_action->steps = 10;
        p_action->interval_ms = 500;
        return true;
    case 1:
        get_node_center(0, &x1, &y1);
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = x1;
        p_action->y1 = y1;
        p_action->interval_ms = 500;
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &clear_button, 450);
        return true;
    case 3:
        get_node_center(0, &x1, &y1);
        get_node_center(8, &x2, &y2);
        p_action->type = EGUI_SIM_ACTION_DRAG;
        p_action->x1 = x1;
        p_action->y1 = y1;
        p_action->x2 = x2;
        p_action->y2 = y2;
        p_action->steps = 12;
        p_action->interval_ms = 600;
        return true;
    case 4:
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    default:
        return false;
    }
}
#endif
