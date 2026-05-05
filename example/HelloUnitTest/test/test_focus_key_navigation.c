#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_focus_key_navigation.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

static egui_view_group_t test_root;
static egui_view_t view_center;
static egui_view_t view_up;
static egui_view_t view_down;
static egui_view_t view_left;
static egui_view_t view_right;
static egui_view_t view_down_right;
static egui_view_t view_hidden;
static egui_view_t view_disabled;
static egui_view_t view_gone;
static egui_view_group_t state_parent;
static egui_view_t state_child;
static egui_view_t click_view;
static egui_view_t click_space_view;
static egui_view_t click_blur_view;
static egui_view_t click_blur_target_view;
static egui_view_progress_bar_t progress_bar;
static egui_view_textinput_t keyboard_textinput;
static egui_view_keyboard_t keyboard_view;
static int g_click_count;
static int g_keyboard_submit_count;

static egui_core_t *test_focus_key_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void test_focus_key_click_cb(egui_view_t *self)
{
    EGUI_UNUSED(self);
    g_click_count++;
}

static void test_focus_key_keyboard_submit_cb(egui_view_t *self, const char *text)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(text);
    g_keyboard_submit_count++;
}

static void test_focus_key_layout_view(egui_view_t *view, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    egui_region_t region;

    egui_region_init(&region, x, y, w, h);
    egui_view_layout(view, &region);
    egui_region_copy(&view->region_screen, &region);
}

static void test_focus_key_cleanup_root(void)
{
    egui_core_t *core = test_focus_key_get_core();
    egui_view_t *root_view = EGUI_VIEW_OF(&test_root);
    egui_view_t *keyboard_view_base = EGUI_VIEW_OF(&keyboard_view);
    egui_view_t *user_root_view = EGUI_VIEW_OF(egui_core_get_user_root_view(core));

    egui_focus_manager_clear_focus(core);
    if (EGUI_VIEW_PARENT(root_view) == user_root_view)
    {
        egui_view_remove_from_user_root(root_view);
    }
    if (EGUI_VIEW_PARENT(keyboard_view_base) == user_root_view)
    {
        egui_view_remove_from_user_root(keyboard_view_base);
    }
}

static void test_focus_key_setup_root(void)
{
    egui_core_t *core = test_focus_key_get_core();
    egui_region_t root_region;

    test_focus_key_cleanup_root();

    egui_view_group_init(EGUI_VIEW_OF(&test_root), core);
    egui_region_init(&root_region, 0, 0, 240, 180);
    egui_view_layout(EGUI_VIEW_OF(&test_root), &root_region);
    egui_region_copy(&EGUI_VIEW_OF(&test_root)->region_screen, &root_region);

    egui_view_init(&view_center, core);
    egui_view_init(&view_up, core);
    egui_view_init(&view_down, core);
    egui_view_init(&view_left, core);
    egui_view_init(&view_right, core);
    egui_view_init(&view_down_right, core);
    egui_view_init(&view_hidden, core);
    egui_view_init(&view_disabled, core);
    egui_view_init(&view_gone, core);

    test_focus_key_layout_view(&view_center, 90, 70, 30, 30);
    test_focus_key_layout_view(&view_up, 90, 20, 30, 30);
    test_focus_key_layout_view(&view_down, 90, 130, 30, 30);
    test_focus_key_layout_view(&view_left, 20, 70, 30, 30);
    test_focus_key_layout_view(&view_right, 170, 70, 30, 30);
    test_focus_key_layout_view(&view_down_right, 105, 130, 30, 30);
    test_focus_key_layout_view(&view_hidden, 170, 20, 30, 30);
    test_focus_key_layout_view(&view_disabled, 170, 130, 30, 30);
    test_focus_key_layout_view(&view_gone, 20, 20, 30, 30);

    egui_view_set_focusable(&view_center, true);
    egui_view_set_focusable(&view_up, true);
    egui_view_set_focusable(&view_down, true);
    egui_view_set_focusable(&view_left, true);
    egui_view_set_focusable(&view_right, true);
    egui_view_set_focusable(&view_down_right, true);
    egui_view_set_focusable(&view_hidden, true);
    egui_view_set_focusable(&view_disabled, true);
    egui_view_set_focusable(&view_gone, true);
    egui_view_set_visible(&view_hidden, false);
    egui_view_set_enable(&view_disabled, false);
    egui_view_set_gone(&view_gone, true);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), &view_center);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), &view_up);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), &view_down);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), &view_left);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), &view_right);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), &view_down_right);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), &view_hidden);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), &view_disabled);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), &view_gone);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_root));
    EGUI_VIEW_OF(egui_core_get_user_root_view(core))->api->calculate_layout(EGUI_VIEW_OF(egui_core_get_user_root_view(core)));
}

static void test_focus_key_teardown_root(void)
{
    test_focus_key_cleanup_root();
}

static void test_focus_key_send_key(uint8_t type, uint8_t key_code)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key_code = key_code;
    egui_core_process_input_key(test_focus_key_get_core(), &event);
}

static void test_focus_direction_selects_spatial_neighbor(void)
{
    test_focus_key_setup_root();
    egui_focus_manager_set_focus(test_focus_key_get_core(), &view_center);

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(test_focus_key_get_core()) == &view_right);

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_LEFT);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(test_focus_key_get_core()) == &view_center);

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_UP);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(test_focus_key_get_core()) == &view_up);

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DOWN);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(test_focus_key_get_core()) == &view_center);

    test_focus_key_teardown_root();
}

static void test_focus_direction_prefers_beam_candidate(void)
{
    test_focus_key_setup_root();
    egui_focus_manager_set_focus(test_focus_key_get_core(), &view_center);

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(test_focus_key_get_core()) == &view_right);
    EGUI_TEST_ASSERT_FALSE(egui_focus_manager_get_focused_view(test_focus_key_get_core()) == &view_down_right);

    test_focus_key_teardown_root();
}

static void test_focus_direction_enters_first_focusable_when_empty(void)
{
    test_focus_key_setup_root();

    EGUI_TEST_ASSERT_NULL(egui_focus_manager_get_focused_view(test_focus_key_get_core()));

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DOWN);

    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(test_focus_key_get_core()) == &view_center);

    test_focus_key_teardown_root();
}

static void test_focus_direction_skips_unavailable_candidates(void)
{
    test_focus_key_setup_root();
    egui_focus_manager_set_focus(test_focus_key_get_core(), &view_center);

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    EGUI_TEST_ASSERT_FALSE(egui_focus_manager_get_focused_view(test_focus_key_get_core()) == &view_hidden);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(test_focus_key_get_core()) == &view_right);

    egui_focus_manager_set_focus(test_focus_key_get_core(), &view_right);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DOWN);
    EGUI_TEST_ASSERT_FALSE(egui_focus_manager_get_focused_view(test_focus_key_get_core()) == &view_disabled);

    egui_focus_manager_set_focus(test_focus_key_get_core(), &view_center);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_LEFT);
    EGUI_TEST_ASSERT_FALSE(egui_focus_manager_get_focused_view(test_focus_key_get_core()) == &view_gone);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(test_focus_key_get_core()) == &view_left);

    test_focus_key_teardown_root();
}

static void test_focus_rejects_direct_unavailable_targets(void)
{
    test_focus_key_setup_root();

    egui_focus_manager_set_focus(test_focus_key_get_core(), &view_hidden);
    EGUI_TEST_ASSERT_NULL(egui_focus_manager_get_focused_view(test_focus_key_get_core()));

    egui_focus_manager_set_focus(test_focus_key_get_core(), &view_disabled);
    EGUI_TEST_ASSERT_NULL(egui_focus_manager_get_focused_view(test_focus_key_get_core()));

    egui_focus_manager_set_focus(test_focus_key_get_core(), &view_gone);
    EGUI_TEST_ASSERT_NULL(egui_focus_manager_get_focused_view(test_focus_key_get_core()));

    test_focus_key_teardown_root();
}

static void test_focus_clears_when_focused_view_becomes_unavailable(void)
{
    test_focus_key_setup_root();

    egui_focus_manager_set_focus(test_focus_key_get_core(), &view_center);
    EGUI_TEST_ASSERT_TRUE(view_center.is_focused);
    egui_view_set_enable(&view_center, false);
    EGUI_TEST_ASSERT_NULL(egui_focus_manager_get_focused_view(test_focus_key_get_core()));
    EGUI_TEST_ASSERT_FALSE(view_center.is_focused);
    egui_view_set_enable(&view_center, true);

    egui_focus_manager_set_focus(test_focus_key_get_core(), &view_center);
    EGUI_TEST_ASSERT_TRUE(view_center.is_focused);
    egui_view_set_visible(&view_center, false);
    EGUI_TEST_ASSERT_NULL(egui_focus_manager_get_focused_view(test_focus_key_get_core()));
    EGUI_TEST_ASSERT_FALSE(view_center.is_focused);
    egui_view_set_visible(&view_center, true);

    egui_focus_manager_set_focus(test_focus_key_get_core(), &view_center);
    EGUI_TEST_ASSERT_TRUE(view_center.is_focused);
    egui_view_set_gone(&view_center, true);
    EGUI_TEST_ASSERT_NULL(egui_focus_manager_get_focused_view(test_focus_key_get_core()));
    EGUI_TEST_ASSERT_FALSE(view_center.is_focused);

    test_focus_key_teardown_root();
}

static void test_focus_clears_when_focused_descendant_parent_becomes_unavailable(void)
{
    egui_core_t *core = test_focus_key_get_core();
    egui_region_t root_region;

    test_focus_key_cleanup_root();

    egui_view_group_init(EGUI_VIEW_OF(&test_root), core);
    egui_region_init(&root_region, 0, 0, 240, 180);
    egui_view_layout(EGUI_VIEW_OF(&test_root), &root_region);
    egui_region_copy(&EGUI_VIEW_OF(&test_root)->region_screen, &root_region);

    egui_view_group_init(EGUI_VIEW_OF(&state_parent), core);
    test_focus_key_layout_view(EGUI_VIEW_OF(&state_parent), 40, 40, 120, 80);
    egui_view_init(&state_child, core);
    egui_view_set_focusable(&state_child, true);
    test_focus_key_layout_view(&state_child, 10, 10, 60, 30);

    egui_view_group_add_child(EGUI_VIEW_OF(&state_parent), &state_child);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), EGUI_VIEW_OF(&state_parent));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_root));
    EGUI_VIEW_OF(egui_core_get_user_root_view(core))->api->calculate_layout(EGUI_VIEW_OF(egui_core_get_user_root_view(core)));

    egui_focus_manager_set_focus(core, &state_child);
    EGUI_TEST_ASSERT_TRUE(state_child.is_focused);
    egui_view_set_enable(EGUI_VIEW_OF(&state_parent), false);
    EGUI_TEST_ASSERT_NULL(egui_focus_manager_get_focused_view(core));
    EGUI_TEST_ASSERT_FALSE(state_child.is_focused);
    egui_view_set_enable(EGUI_VIEW_OF(&state_parent), true);

    egui_focus_manager_set_focus(core, &state_child);
    EGUI_TEST_ASSERT_TRUE(state_child.is_focused);
    egui_view_set_visible(EGUI_VIEW_OF(&state_parent), false);
    EGUI_TEST_ASSERT_NULL(egui_focus_manager_get_focused_view(core));
    EGUI_TEST_ASSERT_FALSE(state_child.is_focused);
    egui_view_set_visible(EGUI_VIEW_OF(&state_parent), true);

    egui_focus_manager_set_focus(core, &state_child);
    EGUI_TEST_ASSERT_TRUE(state_child.is_focused);
    egui_view_set_gone(EGUI_VIEW_OF(&state_parent), true);
    EGUI_TEST_ASSERT_NULL(egui_focus_manager_get_focused_view(core));
    EGUI_TEST_ASSERT_FALSE(state_child.is_focused);

    test_focus_key_teardown_root();
}

static void test_enter_invokes_click_without_touch_path(void)
{
    egui_core_t *core = test_focus_key_get_core();

    egui_focus_manager_clear_focus(core);
    egui_view_init(&click_view, core);
    egui_view_set_focusable(&click_view, true);
    egui_view_set_on_click_listener(&click_view, test_focus_key_click_cb);
    g_click_count = 0;

    egui_focus_manager_set_focus(core, &click_view);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_ENTER);
    EGUI_TEST_ASSERT_TRUE(click_view.is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_click_count);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ENTER);

    EGUI_TEST_ASSERT_EQUAL_INT(1, g_click_count);
    EGUI_TEST_ASSERT_FALSE(click_view.is_pressed);
    EGUI_TEST_ASSERT_TRUE(click_view.is_focusable);
}

static void test_space_invokes_click_with_pressed_lifecycle(void)
{
    egui_core_t *core = test_focus_key_get_core();

    egui_focus_manager_clear_focus(core);
    egui_view_init(&click_space_view, core);
    egui_view_set_focusable(&click_space_view, true);
    egui_view_set_on_click_listener(&click_space_view, test_focus_key_click_cb);
    g_click_count = 0;

    egui_focus_manager_set_focus(core, &click_space_view);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_SPACE);
    EGUI_TEST_ASSERT_TRUE(click_space_view.is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_click_count);

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_REPEAT, EGUI_KEY_CODE_SPACE);
    EGUI_TEST_ASSERT_TRUE(click_space_view.is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_click_count);

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_SPACE);
    EGUI_TEST_ASSERT_FALSE(click_space_view.is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_click_count);

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_SPACE);
    EGUI_TEST_ASSERT_FALSE(click_space_view.is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_click_count);
}

static void test_progress_bar_key_press_repeat_and_long_press(void)
{
    egui_core_t *core = test_focus_key_get_core();

    egui_focus_manager_clear_focus(core);
    egui_view_progress_bar_init(EGUI_VIEW_OF(&progress_bar), core);
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&progress_bar), 50);
    egui_focus_manager_set_focus(core, EGUI_VIEW_OF(&progress_bar));
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == EGUI_VIEW_OF(&progress_bar));

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_RIGHT);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&progress_bar)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(51, progress_bar.process);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_REPEAT, EGUI_KEY_CODE_RIGHT);
    EGUI_TEST_ASSERT_EQUAL_INT(52, progress_bar.process);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_LONG_PRESS, EGUI_KEY_CODE_RIGHT);
    EGUI_TEST_ASSERT_EQUAL_INT(53, progress_bar.process);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&progress_bar)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(53, progress_bar.process);

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_LEFT);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&progress_bar)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(52, progress_bar.process);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_REPEAT, EGUI_KEY_CODE_LEFT);
    EGUI_TEST_ASSERT_EQUAL_INT(51, progress_bar.process);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_LEFT);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&progress_bar)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(51, progress_bar.process);
}

static void test_pressed_state_clears_on_focus_loss(void)
{
    egui_core_t *core = test_focus_key_get_core();

    egui_focus_manager_clear_focus(core);
    egui_view_init(&click_blur_view, core);
    egui_view_init(&click_blur_target_view, core);
    egui_view_set_focusable(&click_blur_view, true);
    egui_view_set_focusable(&click_blur_target_view, true);
    egui_view_set_on_click_listener(&click_blur_view, test_focus_key_click_cb);
    g_click_count = 0;

    egui_focus_manager_set_focus(core, &click_blur_view);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_ENTER);
    EGUI_TEST_ASSERT_TRUE(click_blur_view.is_pressed);

    egui_focus_manager_set_focus(core, &click_blur_target_view);
    EGUI_TEST_ASSERT_FALSE(click_blur_view.is_pressed);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == &click_blur_target_view);

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ENTER);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_click_count);
}

static void test_core_widget_key_handlers(void)
{
    egui_core_t *core = test_focus_key_get_core();
    egui_view_slider_t slider;
    egui_view_number_picker_t picker;
    egui_view_tab_bar_t tab_bar;
    egui_view_button_matrix_t matrix;
    const char *tabs[] = {"A", "B", "C"};
    const char *buttons[] = {"1", "2", "3", "4"};

    egui_view_slider_init(EGUI_VIEW_OF(&slider), core);
    egui_view_slider_set_value(EGUI_VIEW_OF(&slider), 10);
    egui_focus_manager_set_focus(core, EGUI_VIEW_OF(&slider));
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    EGUI_TEST_ASSERT_EQUAL_INT(11, egui_view_slider_get_value(EGUI_VIEW_OF(&slider)));
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_LEFT);
    EGUI_TEST_ASSERT_EQUAL_INT(10, egui_view_slider_get_value(EGUI_VIEW_OF(&slider)));

    egui_view_number_picker_init(EGUI_VIEW_OF(&picker), core);
    egui_view_number_picker_set_range(EGUI_VIEW_OF(&picker), 0, 20);
    egui_view_number_picker_set_value(EGUI_VIEW_OF(&picker), 10);
    egui_view_number_picker_set_step(EGUI_VIEW_OF(&picker), 2);
    egui_focus_manager_set_focus(core, EGUI_VIEW_OF(&picker));
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_UP);
    EGUI_TEST_ASSERT_EQUAL_INT(12, egui_view_number_picker_get_value(EGUI_VIEW_OF(&picker)));
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DOWN);
    EGUI_TEST_ASSERT_EQUAL_INT(10, egui_view_number_picker_get_value(EGUI_VIEW_OF(&picker)));

    egui_view_tab_bar_init(EGUI_VIEW_OF(&tab_bar), core);
    egui_view_tab_bar_set_tabs(EGUI_VIEW_OF(&tab_bar), tabs, 3);
    egui_focus_manager_set_focus(core, EGUI_VIEW_OF(&tab_bar));
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    EGUI_TEST_ASSERT_EQUAL_INT(1, tab_bar.current_index);

    egui_view_button_matrix_init(EGUI_VIEW_OF(&matrix), core);
    egui_view_button_matrix_set_labels(EGUI_VIEW_OF(&matrix), buttons, 4, 2);
    egui_view_button_matrix_set_selection_enabled(EGUI_VIEW_OF(&matrix), 1);
    egui_focus_manager_set_focus(core, EGUI_VIEW_OF(&matrix));
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_button_matrix_get_selected_index(EGUI_VIEW_OF(&matrix)));
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DOWN);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_button_matrix_get_selected_index(EGUI_VIEW_OF(&matrix)));

    egui_focus_manager_clear_focus(core);
}

static void test_combobox_closed_direction_moves_focus(void)
{
    egui_core_t *core = test_focus_key_get_core();
    egui_region_t root_region;
    egui_view_combobox_t combo;
    egui_view_t target;
    static const char *items[] = {"A", "B", "C"};

    test_focus_key_cleanup_root();

    egui_view_group_init(EGUI_VIEW_OF(&test_root), core);
    egui_region_init(&root_region, 0, 0, 160, 120);
    egui_view_layout(EGUI_VIEW_OF(&test_root), &root_region);
    egui_region_copy(&EGUI_VIEW_OF(&test_root)->region_screen, &root_region);

    egui_view_combobox_init(EGUI_VIEW_OF(&combo), core);
    egui_view_combobox_set_items(EGUI_VIEW_OF(&combo), items, 3);
    test_focus_key_layout_view(EGUI_VIEW_OF(&combo), 20, 20, 80, 30);

    egui_view_init(&target, core);
    egui_view_set_focusable(&target, true);
    test_focus_key_layout_view(&target, 20, 70, 80, 30);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), EGUI_VIEW_OF(&combo));
    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), &target);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_root));
    EGUI_VIEW_OF(egui_core_get_user_root_view(core))->api->calculate_layout(EGUI_VIEW_OF(egui_core_get_user_root_view(core)));

    egui_focus_manager_set_focus(core, EGUI_VIEW_OF(&combo));
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DOWN);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == &target);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_combobox_is_expanded(EGUI_VIEW_OF(&combo)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_combobox_get_current_index(EGUI_VIEW_OF(&combo)));

    egui_focus_manager_set_focus(core, EGUI_VIEW_OF(&combo));
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_ENTER);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ENTER);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == EGUI_VIEW_OF(&combo));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_combobox_is_expanded(EGUI_VIEW_OF(&combo)));

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DOWN);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == EGUI_VIEW_OF(&combo));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_combobox_get_current_index(EGUI_VIEW_OF(&combo)));

    test_focus_key_teardown_root();
}

static void test_keyboard_keys_support_focus_navigation_and_hide(void)
{
    egui_core_t *core = test_focus_key_get_core();
    egui_region_t root_region;

    test_focus_key_cleanup_root();

    egui_view_group_init(EGUI_VIEW_OF(&test_root), core);
    egui_region_init(&root_region, 0, 0, 240, 320);
    egui_view_layout(EGUI_VIEW_OF(&test_root), &root_region);
    egui_region_copy(&EGUI_VIEW_OF(&test_root)->region_screen, &root_region);

    egui_view_textinput_init(EGUI_VIEW_OF(&keyboard_textinput), core);
    egui_view_set_position(EGUI_VIEW_OF(&keyboard_textinput), 20, 20);
    egui_view_set_size(EGUI_VIEW_OF(&keyboard_textinput), 120, 28);
    egui_view_textinput_set_font(EGUI_VIEW_OF(&keyboard_textinput), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_textinput_set_on_submit(EGUI_VIEW_OF(&keyboard_textinput), test_focus_key_keyboard_submit_cb);
    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), EGUI_VIEW_OF(&keyboard_textinput));

    egui_view_keyboard_init(EGUI_VIEW_OF(&keyboard_view), core);
    egui_view_set_position(EGUI_VIEW_OF(&keyboard_view), 0, 192);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_root));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&keyboard_view));
    EGUI_VIEW_OF(egui_core_get_user_root_view(core))->api->calculate_layout(EGUI_VIEW_OF(egui_core_get_user_root_view(core)));

    g_keyboard_submit_count = 0;
    egui_view_keyboard_show(EGUI_VIEW_OF(&keyboard_view), EGUI_VIEW_OF(&keyboard_textinput));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&keyboard_view)->is_visible);
    EGUI_TEST_ASSERT_TRUE(keyboard_view.target == EGUI_VIEW_OF(&keyboard_textinput));
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == EGUI_VIEW_OF(&keyboard_view.keys[0]));

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DOWN);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == EGUI_VIEW_OF(&keyboard_view.keys[10]));
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == EGUI_VIEW_OF(&keyboard_view.keys[15]));

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_ENTER);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ENTER);
    EGUI_TEST_ASSERT_TRUE(strcmp(egui_view_textinput_get_text(EGUI_VIEW_OF(&keyboard_textinput)), "h") == 0);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&keyboard_view)->is_visible);

    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DOWN);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DOWN);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == EGUI_VIEW_OF(&keyboard_view.keys[30]));
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_ENTER);
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ENTER);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_keyboard_submit_count);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&keyboard_view)->is_visible);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&keyboard_view)->is_gone);
    EGUI_TEST_ASSERT_NULL(keyboard_view.target);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == EGUI_VIEW_OF(&keyboard_textinput));

    egui_view_keyboard_show(EGUI_VIEW_OF(&keyboard_view), EGUI_VIEW_OF(&keyboard_textinput));
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == EGUI_VIEW_OF(&keyboard_view.keys[0]));
    test_focus_key_send_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ESCAPE);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&keyboard_view)->is_visible);
    EGUI_TEST_ASSERT_NULL(keyboard_view.target);
    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == EGUI_VIEW_OF(&keyboard_textinput));

    test_focus_key_teardown_root();
}

#endif

void test_focus_key_navigation_run(void)
{
    EGUI_TEST_SUITE_BEGIN(focus_key_navigation);
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    EGUI_TEST_RUN(test_focus_direction_selects_spatial_neighbor);
    EGUI_TEST_RUN(test_focus_direction_prefers_beam_candidate);
    EGUI_TEST_RUN(test_focus_direction_enters_first_focusable_when_empty);
    EGUI_TEST_RUN(test_focus_direction_skips_unavailable_candidates);
    EGUI_TEST_RUN(test_focus_rejects_direct_unavailable_targets);
    EGUI_TEST_RUN(test_focus_clears_when_focused_view_becomes_unavailable);
    EGUI_TEST_RUN(test_focus_clears_when_focused_descendant_parent_becomes_unavailable);
    EGUI_TEST_RUN(test_enter_invokes_click_without_touch_path);
    EGUI_TEST_RUN(test_space_invokes_click_with_pressed_lifecycle);
    EGUI_TEST_RUN(test_progress_bar_key_press_repeat_and_long_press);
    EGUI_TEST_RUN(test_pressed_state_clears_on_focus_loss);
    EGUI_TEST_RUN(test_core_widget_key_handlers);
    EGUI_TEST_RUN(test_combobox_closed_direction_moves_focus);
    EGUI_TEST_RUN(test_keyboard_keys_support_focus_navigation_and_hide);
#endif
    EGUI_TEST_SUITE_END();
}
