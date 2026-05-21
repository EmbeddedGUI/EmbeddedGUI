#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_roller_get_state.h"

static egui_view_roller_t s_roller;
static const char *s_items[] = {"Alpha", "Beta", "Gamma"};
static uint8_t s_selected_index;

static void on_selected(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    s_selected_index = index;
}

static void setup(void)
{
    memset(&s_roller, 0, sizeof(s_roller));
    s_selected_index = 0xFFu;
    egui_view_roller_init(EGUI_VIEW_OF(&s_roller), uicode_get_core());
}

static void layout_roller(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 80;
    region.size.height = 90;
    egui_view_layout(EGUI_VIEW_OF(&s_roller), &region);
    egui_region_copy(&EGUI_VIEW_OF(&s_roller)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&s_roller)->api->on_touch_event(EGUI_VIEW_OF(&s_roller), &event);
}

static void test_roller_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_roller_get_on_selected_listener(EGUI_VIEW_OF(&s_roller)));
    EGUI_TEST_ASSERT_NULL(egui_view_roller_get_items(EGUI_VIEW_OF(&s_roller)));
}

static void test_roller_get_state_after_setters(void)
{
    setup();
    egui_view_roller_set_items(EGUI_VIEW_OF(&s_roller), s_items, 3);
    egui_view_roller_set_on_selected_listener(EGUI_VIEW_OF(&s_roller), on_selected);

    EGUI_TEST_ASSERT_TRUE(egui_view_roller_get_items(EGUI_VIEW_OF(&s_roller)) == s_items);
    EGUI_TEST_ASSERT_TRUE(egui_view_roller_get_on_selected_listener(EGUI_VIEW_OF(&s_roller)) == on_selected);
    EGUI_TEST_ASSERT_EQUAL_INT(0xFF, (int)s_selected_index);
}

static void test_roller_get_state_clear(void)
{
    setup();
    egui_view_roller_set_items(EGUI_VIEW_OF(&s_roller), s_items, 3);
    egui_view_roller_set_on_selected_listener(EGUI_VIEW_OF(&s_roller), on_selected);

    egui_view_roller_set_items(EGUI_VIEW_OF(&s_roller), NULL, 0);
    egui_view_roller_set_on_selected_listener(EGUI_VIEW_OF(&s_roller), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_roller_get_items(EGUI_VIEW_OF(&s_roller)));
    EGUI_TEST_ASSERT_NULL(egui_view_roller_get_on_selected_listener(EGUI_VIEW_OF(&s_roller)));
}

static void test_roller_get_state_listener_fires_on_release(void)
{
    setup();
    egui_view_roller_set_items(EGUI_VIEW_OF(&s_roller), s_items, 3);
    egui_view_roller_set_on_selected_listener(EGUI_VIEW_OF(&s_roller), on_selected);
    layout_roller();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 40, 50));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, 40, 50));

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_selected_index);
    EGUI_TEST_ASSERT_TRUE(egui_view_roller_get_on_selected_listener(EGUI_VIEW_OF(&s_roller)) == on_selected);
}

static void test_roller_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_roller_get_on_selected_listener(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_roller_get_items(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_roller_get_current_index(NULL));
}

void test_roller_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(roller_get_state);

    EGUI_TEST_RUN(test_roller_get_state_defaults);
    EGUI_TEST_RUN(test_roller_get_state_after_setters);
    EGUI_TEST_RUN(test_roller_get_state_clear);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_roller_get_state_listener_fires_on_release);
#endif
    EGUI_TEST_RUN(test_roller_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
