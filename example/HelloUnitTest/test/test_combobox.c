#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_combobox.h"

static egui_view_group_t test_root;
static egui_view_group_t test_parent;
static egui_view_combobox_t test_box;
static egui_view_combobox_t test_box_overlap;
static uint8_t g_selected_count;
static uint8_t g_last_selected;
static egui_touch_driver_t g_fake_touch_driver;
static egui_touch_driver_ops_t g_fake_touch_ops;
static uint8_t g_fake_touch_pressed[4];
static uint8_t g_fake_touch_has_position[4];
static egui_dim_t g_fake_touch_x[4];
static egui_dim_t g_fake_touch_y[4];
static uint8_t g_fake_touch_count;
static uint8_t g_fake_touch_index;

static const char *g_items[] = {"One", "Two", "Three"};

static void on_selected(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    g_selected_count++;
    g_last_selected = index;
}

static void reset_listener_state(void)
{
    g_selected_count = 0;
    g_last_selected = 0xFF;
}

static void fake_touch_read_sample(uint8_t *pressed, int16_t *x, int16_t *y, uint8_t *has_position)
{
    uint8_t index = g_fake_touch_index;

    if (g_fake_touch_count == 0)
    {
        *pressed = 0;
        *has_position = 0;
        *x = 0;
        *y = 0;
        return;
    }

    if (index >= g_fake_touch_count)
    {
        index = (uint8_t)(g_fake_touch_count - 1);
    }
    else
    {
        g_fake_touch_index++;
    }

    *pressed = g_fake_touch_pressed[index];
    *has_position = g_fake_touch_has_position[index];
    *x = g_fake_touch_x[index];
    *y = g_fake_touch_y[index];
}

static void fake_touch_read(uint8_t *pressed, int16_t *x, int16_t *y)
{
    uint8_t has_position = 0;

    fake_touch_read_sample(pressed, x, y, &has_position);
}

static void fake_touch_read_ex(uint8_t *pressed, int16_t *x, int16_t *y, uint8_t *has_position)
{
    fake_touch_read_sample(pressed, x, y, has_position);
}

static void fake_touch_set_sequence_with_position(uint8_t count, const uint8_t *pressed, const uint8_t *has_position, const egui_dim_t *x, const egui_dim_t *y)
{
    for (uint8_t i = 0; i < count; i++)
    {
        g_fake_touch_pressed[i] = pressed[i];
        g_fake_touch_has_position[i] = has_position[i];
        g_fake_touch_x[i] = x[i];
        g_fake_touch_y[i] = y[i];
    }

    g_fake_touch_count = count;
    g_fake_touch_index = 0;
}

static void fake_touch_set_sequence(uint8_t count, const uint8_t *pressed, const egui_dim_t *x, const egui_dim_t *y)
{
    uint8_t has_position[4] = {0};

    for (uint8_t i = 0; i < count; i++)
    {
        has_position[i] = pressed[i];
    }
    fake_touch_set_sequence_with_position(count, pressed, has_position, x, y);
}

static void sync_layout(void)
{
    EGUI_VIEW_OF(&test_root)->api->calculate_layout(EGUI_VIEW_OF(&test_root));
}

static void setup_combobox_in_parent(void)
{
    egui_region_t root_region = {{0, 0}, {240, 200}};

    egui_view_group_init(EGUI_VIEW_OF(&test_root));
    egui_view_group_init(EGUI_VIEW_OF(&test_parent));
    egui_view_combobox_init(EGUI_VIEW_OF(&test_box));

    egui_view_layout(EGUI_VIEW_OF(&test_root), &root_region);
    egui_view_set_position(EGUI_VIEW_OF(&test_parent), 10, 10);
    egui_view_set_size(EGUI_VIEW_OF(&test_parent), 200, 140);

    egui_view_set_position(EGUI_VIEW_OF(&test_box), 20, 60);
    egui_view_set_size(EGUI_VIEW_OF(&test_box), 120, 30);
    egui_view_combobox_set_items(EGUI_VIEW_OF(&test_box), g_items, 3);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&test_box), 3);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&test_box), on_selected);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), EGUI_VIEW_OF(&test_parent));
    egui_view_group_add_child(EGUI_VIEW_OF(&test_parent), EGUI_VIEW_OF(&test_box));

    sync_layout();
    reset_listener_state();
}

static void setup_overlapping_comboboxes(void)
{
    egui_region_t root_region = {{0, 0}, {240, 200}};

    egui_view_group_init(EGUI_VIEW_OF(&test_root));
    egui_view_group_init(EGUI_VIEW_OF(&test_parent));
    egui_view_combobox_init(EGUI_VIEW_OF(&test_box));
    egui_view_combobox_init(EGUI_VIEW_OF(&test_box_overlap));

    egui_view_layout(EGUI_VIEW_OF(&test_root), &root_region);
    egui_view_set_position(EGUI_VIEW_OF(&test_parent), 10, 10);
    egui_view_set_size(EGUI_VIEW_OF(&test_parent), 200, 140);

    egui_view_set_position(EGUI_VIEW_OF(&test_box), 20, 10);
    egui_view_set_size(EGUI_VIEW_OF(&test_box), 120, 30);
    egui_view_combobox_set_items(EGUI_VIEW_OF(&test_box), g_items, 3);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&test_box), 3);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&test_box), on_selected);

    egui_view_set_position(EGUI_VIEW_OF(&test_box_overlap), 10, 50);
    egui_view_set_size(EGUI_VIEW_OF(&test_box_overlap), 140, 32);
    egui_view_combobox_set_items(EGUI_VIEW_OF(&test_box_overlap), g_items, 3);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&test_box_overlap), 3);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_root), EGUI_VIEW_OF(&test_parent));
    egui_view_group_add_child(EGUI_VIEW_OF(&test_parent), EGUI_VIEW_OF(&test_box));
    egui_view_group_add_child(EGUI_VIEW_OF(&test_parent), EGUI_VIEW_OF(&test_box_overlap));

    sync_layout();
    reset_listener_state();
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_root)->api->dispatch_touch_event(EGUI_VIEW_OF(&test_root), &event);
}

static int send_touch_via_core_root(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;
    egui_view_group_t *root = egui_core_get_root_view();

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(root)->api->dispatch_touch_event(EGUI_VIEW_OF(root), &event);
}

static void get_header_center(egui_dim_t *x, egui_dim_t *y)
{
    egui_view_combobox_t *local = (egui_view_combobox_t *)EGUI_VIEW_OF(&test_box);

    *x = EGUI_VIEW_OF(&test_box)->region_screen.location.x + EGUI_VIEW_OF(&test_box)->region_screen.size.width / 2;
    *y = EGUI_VIEW_OF(&test_box)->region_screen.location.y + local->collapsed_height / 2;
}

static void get_item_center(uint8_t item_index, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_combobox_t *local = (egui_view_combobox_t *)EGUI_VIEW_OF(&test_box);

    *x = EGUI_VIEW_OF(&test_box)->region_screen.location.x + EGUI_VIEW_OF(&test_box)->region_screen.size.width / 2;
    *y = EGUI_VIEW_OF(&test_box)->region_screen.location.y + local->collapsed_height + item_index * local->item_height + local->item_height / 2;
}

static void sync_core_layout(void)
{
    egui_view_group_t *root = egui_core_get_root_view();

    EGUI_VIEW_OF(root)->api->calculate_layout(EGUI_VIEW_OF(root));
}

static void setup_core_overlapping_comboboxes(void)
{
    egui_view_group_t *user_root = egui_core_get_user_root_view();

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
    egui_view_group_init(EGUI_VIEW_OF(&test_parent));
    egui_view_combobox_init(EGUI_VIEW_OF(&test_box));
    egui_view_combobox_init(EGUI_VIEW_OF(&test_box_overlap));

    egui_view_set_position(EGUI_VIEW_OF(&test_parent), 10, 10);
    egui_view_set_size(EGUI_VIEW_OF(&test_parent), 200, 140);

    egui_view_set_position(EGUI_VIEW_OF(&test_box), 20, 10);
    egui_view_set_size(EGUI_VIEW_OF(&test_box), 120, 30);
    egui_view_combobox_set_items(EGUI_VIEW_OF(&test_box), g_items, 3);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&test_box), 3);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&test_box), on_selected);

    egui_view_set_position(EGUI_VIEW_OF(&test_box_overlap), 10, 50);
    egui_view_set_size(EGUI_VIEW_OF(&test_box_overlap), 140, 32);
    egui_view_combobox_set_items(EGUI_VIEW_OF(&test_box_overlap), g_items, 3);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&test_box_overlap), 3);

    egui_view_group_add_child(EGUI_VIEW_OF(&test_parent), EGUI_VIEW_OF(&test_box));
    egui_view_group_add_child(EGUI_VIEW_OF(&test_parent), EGUI_VIEW_OF(&test_box_overlap));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_parent));

    sync_core_layout();
    reset_listener_state();
}

static void test_combobox_expand_clamps_to_parent_height_and_keeps_selection_clickable(void)
{
    egui_view_combobox_t *local = (egui_view_combobox_t *)EGUI_VIEW_OF(&test_box);
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_dim_t expected_height;

    setup_combobox_in_parent();
    get_header_center(&x, &y);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    sync_layout();

    expected_height = local->collapsed_height + 2 * local->item_height;
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(expected_height, EGUI_VIEW_OF(&test_box)->region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(expected_height, EGUI_VIEW_OF(&test_box)->region_screen.size.height);

    get_item_center(1, &x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    sync_layout();

    EGUI_TEST_ASSERT_FALSE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_combobox_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selected_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_selected);
}

static void test_combobox_expanded_dropdown_stays_clickable_over_sibling(void)
{
    egui_dim_t x = 0;
    egui_dim_t y = 0;

    setup_overlapping_comboboxes();
    get_header_center(&x, &y);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    sync_layout();

    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));

    get_item_center(1, &x, &y);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    sync_layout();

    EGUI_TEST_ASSERT_FALSE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_combobox_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selected_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_selected);
}

static void test_combobox_release_outside_does_not_expand_or_commit_selection(void)
{
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_dim_t outside_x = 0;
    egui_dim_t outside_y = 0;

    setup_combobox_in_parent();
    get_header_center(&x, &y);
    outside_x = EGUI_VIEW_OF(&test_box)->region_screen.location.x + EGUI_VIEW_OF(&test_box)->region_screen.size.width + 40;
    outside_y = y;

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, outside_x, outside_y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, outside_x, outside_y));
    sync_layout();

    EGUI_TEST_ASSERT_FALSE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_combobox_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_selected_count);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    sync_layout();
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));

    get_item_center(1, &x, &y);
    outside_x = EGUI_VIEW_OF(&test_box)->region_screen.location.x + EGUI_VIEW_OF(&test_box)->region_screen.size.width + 40;
    outside_y = EGUI_VIEW_OF(&test_box)->region_screen.location.y + EGUI_VIEW_OF(&test_box)->region_screen.size.height + 20;

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, outside_x, outside_y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, outside_x, outside_y));
    sync_layout();

    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_combobox_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_selected_count);
}

static void test_combobox_release_on_different_item_does_not_commit_selection(void)
{
    egui_dim_t x1 = 0;
    egui_dim_t y1 = 0;
    egui_dim_t x2 = 0;
    egui_dim_t y2 = 0;

    setup_combobox_in_parent();
    get_header_center(&x1, &y1);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));
    sync_layout();
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));

    get_item_center(1, &x1, &y1);
    get_item_center(2, &x2, &y2);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x2, y2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x2, y2));
    sync_layout();

    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_combobox_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_selected_count);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x1, y1));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x2, y2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x1, y1));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x1, y1));
    sync_layout();

    EGUI_TEST_ASSERT_FALSE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_combobox_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selected_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_selected);
}

static void test_combobox_input_polling_reuses_last_pressed_coords_for_release(void)
{
    static const uint8_t touch_pressed[] = {1, 0};
    egui_dim_t touch_x[2];
    egui_dim_t touch_y[2];
    egui_touch_driver_t *saved_touch_driver = egui_touch_driver_get();
    egui_view_group_t *user_root = egui_core_get_user_root_view();
    egui_dim_t x = 0;
    egui_dim_t y = 0;

    setup_core_overlapping_comboboxes();
    get_header_center(&x, &y);

    EGUI_TEST_ASSERT_TRUE(send_touch_via_core_root(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch_via_core_root(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    sync_core_layout();

    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));

    get_item_center(1, &x, &y);
    touch_x[0] = x;
    touch_y[0] = y;
    touch_x[1] = 0;
    touch_y[1] = 0;

    g_fake_touch_ops.init = NULL;
    g_fake_touch_ops.read = fake_touch_read;
    g_fake_touch_driver.ops = &g_fake_touch_ops;
    fake_touch_set_sequence(2, touch_pressed, touch_x, touch_y);

    egui_touch_driver_register(&g_fake_touch_driver);
    egui_input_init();
    egui_input_polling_work();
    egui_input_polling_work();
    sync_core_layout();

    EGUI_TEST_ASSERT_FALSE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_combobox_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selected_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_last_selected);

    egui_touch_driver_register(saved_touch_driver);
    egui_input_init();
    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
}

static void test_combobox_input_polling_prefers_reported_release_coords(void)
{
    static const uint8_t touch_pressed[] = {1, 0};
    static const uint8_t touch_has_position[] = {1, 1};
    egui_dim_t touch_x[2];
    egui_dim_t touch_y[2];
    egui_touch_driver_t *saved_touch_driver = egui_touch_driver_get();
    egui_view_group_t *user_root = egui_core_get_user_root_view();
    egui_dim_t x = 0;
    egui_dim_t y = 0;
    egui_dim_t outside_x = 0;
    egui_dim_t outside_y = 0;

    setup_core_overlapping_comboboxes();
    get_header_center(&x, &y);

    EGUI_TEST_ASSERT_TRUE(send_touch_via_core_root(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_TRUE(send_touch_via_core_root(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    sync_core_layout();
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));

    get_item_center(1, &x, &y);
    outside_x = EGUI_VIEW_OF(&test_box)->region_screen.location.x + EGUI_VIEW_OF(&test_box)->region_screen.size.width + 40;
    outside_y = y;

    touch_x[0] = x;
    touch_y[0] = y;
    touch_x[1] = outside_x;
    touch_y[1] = outside_y;

    g_fake_touch_ops.init = NULL;
    g_fake_touch_ops.read = fake_touch_read;
    g_fake_touch_ops.read_ex = fake_touch_read_ex;
    g_fake_touch_driver.ops = &g_fake_touch_ops;
    fake_touch_set_sequence_with_position(2, touch_pressed, touch_has_position, touch_x, touch_y);

    egui_touch_driver_register(&g_fake_touch_driver);
    egui_input_init();
    egui_input_polling_work();
    egui_input_polling_work();
    sync_core_layout();

    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_is_expanded(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_combobox_get_current_index(EGUI_VIEW_OF(&test_box)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_selected_count);

    egui_touch_driver_register(saved_touch_driver);
    egui_input_init();
    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
}

void test_combobox_run(void)
{
    EGUI_TEST_SUITE_BEGIN(combobox);
    EGUI_TEST_RUN(test_combobox_expand_clamps_to_parent_height_and_keeps_selection_clickable);
    EGUI_TEST_RUN(test_combobox_expanded_dropdown_stays_clickable_over_sibling);
    EGUI_TEST_RUN(test_combobox_release_outside_does_not_expand_or_commit_selection);
    EGUI_TEST_RUN(test_combobox_release_on_different_item_does_not_commit_selection);
    EGUI_TEST_RUN(test_combobox_input_polling_reuses_last_pressed_coords_for_release);
    EGUI_TEST_RUN(test_combobox_input_polling_prefers_reported_release_coords);
    EGUI_TEST_SUITE_END();
}
