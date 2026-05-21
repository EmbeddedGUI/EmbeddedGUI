#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_pattern_lock_get_state.h"

#define PATTERN_LOCK_NODE_NONE 0xFF

static egui_view_pattern_lock_t s_lock;
static uint8_t s_complete_count;
static uint8_t s_complete_node_count;
static uint8_t s_finish_count;
static uint8_t s_finish_node_count;
static uint8_t s_finish_valid;
static uint8_t s_finish_nodes[EGUI_VIEW_PATTERN_LOCK_MAX_NODES];

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_lock, 0, sizeof(s_lock));
    memset(s_finish_nodes, PATTERN_LOCK_NODE_NONE, sizeof(s_finish_nodes));
    s_complete_count = 0;
    s_complete_node_count = 0;
    s_finish_count = 0;
    s_finish_node_count = 0;
    s_finish_valid = 0;
    egui_view_pattern_lock_init(EGUI_VIEW_OF(&s_lock), core);
}

static void layout_lock(void)
{
    egui_region_t region;

    egui_region_init(&region, 10, 20, 180, 180);
    egui_view_layout(EGUI_VIEW_OF(&s_lock), &region);
    egui_region_copy(&EGUI_VIEW_OF(&s_lock)->region_screen, &region);
}

static void get_node_center(uint8_t node, egui_dim_t *x, egui_dim_t *y)
{
    egui_dim_t step_x = EGUI_VIEW_OF(&s_lock)->region.size.width / EGUI_VIEW_PATTERN_LOCK_GRID_SIZE;
    egui_dim_t step_y = EGUI_VIEW_OF(&s_lock)->region.size.height / EGUI_VIEW_PATTERN_LOCK_GRID_SIZE;
    uint8_t col = node % EGUI_VIEW_PATTERN_LOCK_GRID_SIZE;
    uint8_t row = node / EGUI_VIEW_PATTERN_LOCK_GRID_SIZE;

    *x = EGUI_VIEW_OF(&s_lock)->region_screen.location.x + step_x / 2 + col * step_x;
    *y = EGUI_VIEW_OF(&s_lock)->region_screen.location.y + step_y / 2 + row * step_y;
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&s_lock)->api->on_touch_event(EGUI_VIEW_OF(&s_lock), &event);
}

static void on_pattern_complete(egui_view_t *self, uint8_t node_count)
{
    EGUI_UNUSED(self);
    s_complete_count++;
    s_complete_node_count = node_count;
}

static void on_pattern_finish(egui_view_t *self, const uint8_t *nodes, uint8_t node_count, uint8_t valid)
{
    uint8_t i;

    EGUI_UNUSED(self);
    s_finish_count++;
    s_finish_node_count = node_count;
    s_finish_valid = valid;
    for (i = 0; i < EGUI_VIEW_PATTERN_LOCK_MAX_NODES; i++)
    {
        s_finish_nodes[i] = (i < node_count) ? nodes[i] : PATTERN_LOCK_NODE_NONE;
    }
}

static void test_pattern_lock_get_state_defaults(void)
{
    uint8_t i;

    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_pattern_lock_get_min_nodes(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_node_count(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_TRUE(egui_view_pattern_lock_get_nodes(EGUI_VIEW_OF(&s_lock)) == s_lock.nodes);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_is_tracking(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_is_completed(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_show_error(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_pattern_lock_get_touch_expand(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_SURFACE_VARIANT.full, (int)egui_view_pattern_lock_get_bg_color(EGUI_VIEW_OF(&s_lock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_BORDER.full, (int)egui_view_pattern_lock_get_border_color(EGUI_VIEW_OF(&s_lock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT_SECONDARY.full, (int)egui_view_pattern_lock_get_node_color(EGUI_VIEW_OF(&s_lock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full, (int)egui_view_pattern_lock_get_active_node_color(EGUI_VIEW_OF(&s_lock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY.full, (int)egui_view_pattern_lock_get_line_color(EGUI_VIEW_OF(&s_lock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_DANGER.full, (int)egui_view_pattern_lock_get_error_color(EGUI_VIEW_OF(&s_lock)).full);
    EGUI_TEST_ASSERT_NULL(egui_view_pattern_lock_get_on_pattern_complete_listener(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_NULL(egui_view_pattern_lock_get_on_pattern_finish_listener(EGUI_VIEW_OF(&s_lock)));
    for (i = 0; i < EGUI_VIEW_PATTERN_LOCK_MAX_NODES; i++)
    {
        EGUI_TEST_ASSERT_EQUAL_INT(PATTERN_LOCK_NODE_NONE, (int)egui_view_pattern_lock_get_node(EGUI_VIEW_OF(&s_lock), i));
        EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_selected(EGUI_VIEW_OF(&s_lock), i));
    }
}

static void test_pattern_lock_get_state_after_setters(void)
{
    egui_color_t bg_color = {.full = 0x1234};
    egui_color_t border_color = {.full = 0x2345};
    egui_color_t node_color = {.full = 0x3456};
    egui_color_t active_node_color = {.full = 0x4567};
    egui_color_t line_color = {.full = 0x5678};
    egui_color_t error_color = {.full = 0x6789};

    setup();
    egui_view_pattern_lock_set_min_nodes(EGUI_VIEW_OF(&s_lock), 0);
    egui_view_pattern_lock_set_touch_expand(EGUI_VIEW_OF(&s_lock), 99);
    egui_view_pattern_lock_set_on_pattern_complete_listener(EGUI_VIEW_OF(&s_lock), on_pattern_complete);
    egui_view_pattern_lock_set_on_pattern_finish_listener(EGUI_VIEW_OF(&s_lock), on_pattern_finish);
    egui_view_pattern_lock_set_bg_color(EGUI_VIEW_OF(&s_lock), bg_color);
    egui_view_pattern_lock_set_border_color(EGUI_VIEW_OF(&s_lock), border_color);
    egui_view_pattern_lock_set_node_color(EGUI_VIEW_OF(&s_lock), node_color);
    egui_view_pattern_lock_set_active_node_color(EGUI_VIEW_OF(&s_lock), active_node_color);
    egui_view_pattern_lock_set_line_color(EGUI_VIEW_OF(&s_lock), line_color);
    egui_view_pattern_lock_set_error_color(EGUI_VIEW_OF(&s_lock), error_color);

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_pattern_lock_get_min_nodes(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(24, (int)egui_view_pattern_lock_get_touch_expand(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_TRUE(egui_view_pattern_lock_get_on_pattern_complete_listener(EGUI_VIEW_OF(&s_lock)) == on_pattern_complete);
    EGUI_TEST_ASSERT_TRUE(egui_view_pattern_lock_get_on_pattern_finish_listener(EGUI_VIEW_OF(&s_lock)) == on_pattern_finish);
    EGUI_TEST_ASSERT_EQUAL_INT((int)bg_color.full, (int)egui_view_pattern_lock_get_bg_color(EGUI_VIEW_OF(&s_lock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)border_color.full, (int)egui_view_pattern_lock_get_border_color(EGUI_VIEW_OF(&s_lock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)node_color.full, (int)egui_view_pattern_lock_get_node_color(EGUI_VIEW_OF(&s_lock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)active_node_color.full, (int)egui_view_pattern_lock_get_active_node_color(EGUI_VIEW_OF(&s_lock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)line_color.full, (int)egui_view_pattern_lock_get_line_color(EGUI_VIEW_OF(&s_lock)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)error_color.full, (int)egui_view_pattern_lock_get_error_color(EGUI_VIEW_OF(&s_lock)).full);

    egui_view_pattern_lock_set_min_nodes(EGUI_VIEW_OF(&s_lock), 99);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_PATTERN_LOCK_MAX_NODES, (int)egui_view_pattern_lock_get_min_nodes(EGUI_VIEW_OF(&s_lock)));
}

static void test_pattern_lock_get_state_apply_params(void)
{
    static const egui_view_pattern_lock_params_t params = {
            .region = {{3, 4}, {120, 130}},
            .min_nodes = 0,
            .touch_expand = 99,
    };

    setup();
    egui_view_pattern_lock_apply_params(EGUI_VIEW_OF(&s_lock), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(120, (int)egui_view_get_width(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(130, (int)egui_view_get_height(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_pattern_lock_get_min_nodes(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(24, (int)egui_view_pattern_lock_get_touch_expand(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_node_count(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_is_completed(EGUI_VIEW_OF(&s_lock)));
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void test_pattern_lock_get_state_valid_gesture_tracks_nodes(void)
{
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x2;
    egui_dim_t y2;
    egui_dim_t x8;
    egui_dim_t y8;

    setup();
    layout_lock();
    egui_view_pattern_lock_set_min_nodes(EGUI_VIEW_OF(&s_lock), 4);
    egui_view_pattern_lock_set_on_pattern_complete_listener(EGUI_VIEW_OF(&s_lock), on_pattern_complete);
    egui_view_pattern_lock_set_on_pattern_finish_listener(EGUI_VIEW_OF(&s_lock), on_pattern_finish);
    get_node_center(0, &x0, &y0);
    get_node_center(2, &x2, &y2);
    get_node_center(8, &x8, &y8);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x0, y0));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_pattern_lock_get_is_tracking(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_pattern_lock_get_node_count(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_node(EGUI_VIEW_OF(&s_lock), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_pattern_lock_get_selected(EGUI_VIEW_OF(&s_lock), 0));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x2, y2));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_pattern_lock_get_node_count(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_node(EGUI_VIEW_OF(&s_lock), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_pattern_lock_get_node(EGUI_VIEW_OF(&s_lock), 1));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_pattern_lock_get_node(EGUI_VIEW_OF(&s_lock), 2));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_pattern_lock_get_selected(EGUI_VIEW_OF(&s_lock), 1));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x8, y8));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_is_tracking(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_pattern_lock_get_is_completed(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_show_error(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_pattern_lock_get_node_count(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_node(EGUI_VIEW_OF(&s_lock), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_pattern_lock_get_node(EGUI_VIEW_OF(&s_lock), 1));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_pattern_lock_get_node(EGUI_VIEW_OF(&s_lock), 2));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_pattern_lock_get_node(EGUI_VIEW_OF(&s_lock), 3));
    EGUI_TEST_ASSERT_EQUAL_INT(8, (int)egui_view_pattern_lock_get_node(EGUI_VIEW_OF(&s_lock), 4));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_finish_count);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)s_finish_node_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_finish_valid);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_complete_count);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)s_complete_node_count);
    EGUI_TEST_ASSERT_EQUAL_INT(8, (int)s_finish_nodes[4]);

    egui_view_pattern_lock_clear_pattern(EGUI_VIEW_OF(&s_lock));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_node_count(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_is_completed(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_selected(EGUI_VIEW_OF(&s_lock), 8));
    EGUI_TEST_ASSERT_EQUAL_INT(PATTERN_LOCK_NODE_NONE, (int)egui_view_pattern_lock_get_node(EGUI_VIEW_OF(&s_lock), 0));
}

static void test_pattern_lock_get_state_invalid_gesture_sets_error(void)
{
    egui_dim_t x0;
    egui_dim_t y0;

    setup();
    layout_lock();
    egui_view_pattern_lock_set_min_nodes(EGUI_VIEW_OF(&s_lock), 4);
    egui_view_pattern_lock_set_on_pattern_complete_listener(EGUI_VIEW_OF(&s_lock), on_pattern_complete);
    egui_view_pattern_lock_set_on_pattern_finish_listener(EGUI_VIEW_OF(&s_lock), on_pattern_finish);
    get_node_center(0, &x0, &y0);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x0, y0));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x0, y0));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_pattern_lock_get_is_completed(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_pattern_lock_get_show_error(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_pattern_lock_get_node_count(EGUI_VIEW_OF(&s_lock)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_finish_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_finish_node_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_finish_valid);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_complete_count);
}
#endif

static void test_pattern_lock_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_min_nodes(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_node_count(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_pattern_lock_get_nodes(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(PATTERN_LOCK_NODE_NONE, (int)egui_view_pattern_lock_get_node(NULL, 0));
    EGUI_TEST_ASSERT_EQUAL_INT(PATTERN_LOCK_NODE_NONE, (int)egui_view_pattern_lock_get_node(EGUI_VIEW_OF(&s_lock), EGUI_VIEW_PATTERN_LOCK_MAX_NODES));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_selected(NULL, 0));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_selected(EGUI_VIEW_OF(&s_lock), EGUI_VIEW_PATTERN_LOCK_MAX_NODES));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_is_tracking(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_is_completed(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_show_error(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_touch_expand(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_pattern_lock_get_on_pattern_complete_listener(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_pattern_lock_get_on_pattern_finish_listener(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_bg_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_border_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_node_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_active_node_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_line_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_pattern_lock_get_error_color(NULL).full);
}

void test_pattern_lock_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(pattern_lock_get_state);

    EGUI_TEST_RUN(test_pattern_lock_get_state_defaults);
    EGUI_TEST_RUN(test_pattern_lock_get_state_after_setters);
    EGUI_TEST_RUN(test_pattern_lock_get_state_apply_params);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_pattern_lock_get_state_valid_gesture_tracks_nodes);
    EGUI_TEST_RUN(test_pattern_lock_get_state_invalid_gesture_sets_error);
#endif
    EGUI_TEST_RUN(test_pattern_lock_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
