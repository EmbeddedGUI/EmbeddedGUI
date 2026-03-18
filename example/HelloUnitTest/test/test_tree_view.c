#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_tree_view.h"

#include "../../HelloCustomWidgets/navigation/tree_view/egui_view_tree_view.h"
#include "../../HelloCustomWidgets/navigation/tree_view/egui_view_tree_view.c"

static egui_view_tree_view_t test_tree_view;
static uint8_t selection_change_count;
static uint8_t last_selection_index;

static const egui_view_tree_view_item_t tree_items_primary[] = {
        {"Workspace", "3", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Controls", "12", 1, EGUI_VIEW_TREE_VIEW_TONE_ACCENT, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Buttons", "", 2, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
        {"Tree View", "Draft", 2, EGUI_VIEW_TREE_VIEW_TONE_ACCENT, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
};

static const egui_view_tree_view_item_t tree_items_secondary[] = {
        {"Workspace", "3", 0, EGUI_VIEW_TREE_VIEW_TONE_NEUTRAL, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"Docs", "7", 1, EGUI_VIEW_TREE_VIEW_TONE_ACCENT, EGUI_VIEW_TREE_VIEW_KIND_FOLDER, 1, 1},
        {"API", "New", 2, EGUI_VIEW_TREE_VIEW_TONE_SUCCESS, EGUI_VIEW_TREE_VIEW_KIND_LEAF, 0, 0},
};

static const egui_view_tree_view_snapshot_t tree_snapshots[] = {
        {"Solution", "4 rows", "Controls open", tree_items_primary, 4, 1},
        {"Docs", "3 rows", "API highlighted", tree_items_secondary, 3, 2},
};

static void on_selection_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    selection_change_count++;
    last_selection_index = index;
}

static void reset_listener_state(void)
{
    selection_change_count = 0;
    last_selection_index = EGUI_VIEW_TREE_VIEW_INDEX_NONE;
}

static void setup_tree_view(void)
{
    egui_view_tree_view_init(EGUI_VIEW_OF(&test_tree_view));
    egui_view_set_size(EGUI_VIEW_OF(&test_tree_view), 144, 120);
    egui_view_tree_view_set_snapshots(EGUI_VIEW_OF(&test_tree_view), tree_snapshots, (uint8_t)(sizeof(tree_snapshots) / sizeof(tree_snapshots[0])));
    egui_view_tree_view_set_on_selection_changed_listener(EGUI_VIEW_OF(&test_tree_view), on_selection_changed);
    reset_listener_state();
}

static void layout_tree_view(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_tree_view), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_tree_view)->region_screen, &region);
}

static uint8_t get_item_center(uint8_t index, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_tree_view_metrics_t metrics;
    const egui_view_tree_view_snapshot_t *snapshot = egui_view_tree_view_get_snapshot(&test_tree_view);
    uint8_t item_count = snapshot == NULL ? 0 : egui_view_tree_view_clamp_item_count(snapshot->item_count);

    if (snapshot == NULL)
    {
        return 0;
    }

    egui_view_tree_view_get_metrics(&test_tree_view, EGUI_VIEW_OF(&test_tree_view), item_count, &metrics);
    if (index >= metrics.visible_item_count)
    {
        return 0;
    }

    *x = metrics.item_regions[index].location.x + metrics.item_regions[index].size.width / 2;
    *y = metrics.item_regions[index].location.y + metrics.item_regions[index].size.height / 2;
    return 1;
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_tree_view)->api->on_touch_event(EGUI_VIEW_OF(&test_tree_view), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_tree_view)->api->on_key_event(EGUI_VIEW_OF(&test_tree_view), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_tree_view)->api->on_key_event(EGUI_VIEW_OF(&test_tree_view), &event);
    return handled;
}

static void test_tree_view_snapshot_switch_resets_focus_index(void)
{
    setup_tree_view();
    egui_view_tree_view_set_current_index(EGUI_VIEW_OF(&test_tree_view), 3);
    reset_listener_state();

    egui_view_tree_view_set_current_snapshot(EGUI_VIEW_OF(&test_tree_view), 1);

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_tree_view_get_current_snapshot(EGUI_VIEW_OF(&test_tree_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_tree_view_get_current_index(EGUI_VIEW_OF(&test_tree_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, selection_change_count);
}

static void test_tree_view_set_current_index_clamps_to_last_item(void)
{
    setup_tree_view();

    egui_view_tree_view_set_current_index(EGUI_VIEW_OF(&test_tree_view), 99);

    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_tree_view_get_current_index(EGUI_VIEW_OF(&test_tree_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, selection_change_count);
    EGUI_TEST_ASSERT_EQUAL_INT(3, last_selection_index);
}

static void test_tree_view_touch_selects_item(void)
{
    egui_dim_t x;
    egui_dim_t y;

    setup_tree_view();
    layout_tree_view(10, 20, 144, 120);
    EGUI_TEST_ASSERT_TRUE(get_item_center(2, &x, &y));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_tree_view_get_current_index(EGUI_VIEW_OF(&test_tree_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, selection_change_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, last_selection_index);
}

static void test_tree_view_locked_mode_ignores_input(void)
{
    egui_dim_t x;
    egui_dim_t y;

    setup_tree_view();
    egui_view_tree_view_set_locked_mode(EGUI_VIEW_OF(&test_tree_view), 1);
    layout_tree_view(10, 20, 144, 120);
    EGUI_TEST_ASSERT_TRUE(get_item_center(3, &x, &y));

    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_tree_view_get_current_index(EGUI_VIEW_OF(&test_tree_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, selection_change_count);
}

static void test_tree_view_compact_mode_keeps_selection_and_allows_input(void)
{
    egui_dim_t x;
    egui_dim_t y;

    setup_tree_view();
    egui_view_tree_view_set_current_index(EGUI_VIEW_OF(&test_tree_view), 2);
    reset_listener_state();

    egui_view_tree_view_set_compact_mode(EGUI_VIEW_OF(&test_tree_view), 1);
    layout_tree_view(10, 20, 144, 120);
    EGUI_TEST_ASSERT_TRUE(get_item_center(0, &x, &y));

    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_tree_view_get_current_index(EGUI_VIEW_OF(&test_tree_view)));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_tree_view_get_current_index(EGUI_VIEW_OF(&test_tree_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, selection_change_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, last_selection_index);
}

static void test_tree_view_keyboard_navigation(void)
{
    setup_tree_view();

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_DOWN));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_tree_view_get_current_index(EGUI_VIEW_OF(&test_tree_view)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_UP));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_tree_view_get_current_index(EGUI_VIEW_OF(&test_tree_view)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_tree_view_get_current_index(EGUI_VIEW_OF(&test_tree_view)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_tree_view_get_current_index(EGUI_VIEW_OF(&test_tree_view)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, selection_change_count);
    EGUI_TEST_ASSERT_EQUAL_INT(3, last_selection_index);
}

void test_tree_view_run(void)
{
    EGUI_TEST_SUITE_BEGIN(tree_view);
    EGUI_TEST_RUN(test_tree_view_snapshot_switch_resets_focus_index);
    EGUI_TEST_RUN(test_tree_view_set_current_index_clamps_to_last_item);
    EGUI_TEST_RUN(test_tree_view_touch_selects_item);
    EGUI_TEST_RUN(test_tree_view_locked_mode_ignores_input);
    EGUI_TEST_RUN(test_tree_view_compact_mode_keeps_selection_and_allows_input);
    EGUI_TEST_RUN(test_tree_view_keyboard_navigation);
    EGUI_TEST_SUITE_END();
}
