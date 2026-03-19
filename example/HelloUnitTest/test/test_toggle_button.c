#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_toggle_button.h"

#include "../../HelloCustomWidgets/input/toggle_button/egui_view_toggle_button.h"
#include "../../HelloCustomWidgets/input/toggle_button/egui_view_toggle_button.c"

static egui_view_toggle_button_t test_button;
static int toggled_count;
static uint8_t last_toggled;

static void on_toggled(egui_view_t *self, uint8_t is_toggled)
{
    EGUI_UNUSED(self);
    toggled_count++;
    last_toggled = is_toggled;
}

static void setup_button(uint8_t is_toggled)
{
    egui_view_toggle_button_init(EGUI_VIEW_OF(&test_button));
    egui_view_set_size(EGUI_VIEW_OF(&test_button), 116, 44);
    egui_view_toggle_button_set_text(EGUI_VIEW_OF(&test_button), "Alerts");
    egui_view_toggle_button_set_icon(EGUI_VIEW_OF(&test_button), EGUI_ICON_MS_NOTIFICATIONS);
    egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&test_button), EGUI_FONT_ICON_MS_16);
    hcw_toggle_button_apply_standard_style(EGUI_VIEW_OF(&test_button));
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&test_button), on_toggled);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(EGUI_VIEW_OF(&test_button), 1);
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    static egui_view_api_t test_button_key_api;
    egui_view_override_api_on_key(EGUI_VIEW_OF(&test_button), &test_button_key_api, hcw_toggle_button_on_key_event);
#endif
    egui_view_toggle_button_set_toggled(EGUI_VIEW_OF(&test_button), is_toggled);
    toggled_count = 0;
    last_toggled = is_toggled;
}

static void layout_button(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 116;
    region.size.height = 44;
    egui_view_layout(EGUI_VIEW_OF(&test_button), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_button)->region_screen, &region);
}

static int send_touch(uint8_t type)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = 40;
    event.location.y = 36;
    return EGUI_VIEW_OF(&test_button)->api->on_touch_event(EGUI_VIEW_OF(&test_button), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_button)->api->dispatch_key_event(EGUI_VIEW_OF(&test_button), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_button)->api->dispatch_key_event(EGUI_VIEW_OF(&test_button), &event);
    return handled;
}

static void test_toggle_button_touch_toggles_state(void)
{
    setup_button(0);
    layout_button();
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_TRUE(egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, toggled_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, last_toggled);
    EGUI_TEST_ASSERT_FALSE(egui_view_get_pressed(EGUI_VIEW_OF(&test_button)));
}

static void test_toggle_button_enter_and_space_toggle_state(void)
{
    setup_button(0);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_TRUE(egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, toggled_count);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_FALSE(egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, toggled_count);
}

static void test_toggle_button_disabled_ignores_input(void)
{
    setup_button(1);
    egui_view_set_enable(EGUI_VIEW_OF(&test_button), 0);
    layout_button();
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_TRUE(egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, toggled_count);
}

static void test_toggle_button_style_helpers_update_palette(void)
{
    setup_button(0);
    EGUI_TEST_ASSERT_EQUAL_INT(10, test_button.corner_radius);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x2563EB).full, test_button.on_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xEAF1FB).full, test_button.off_color.full);

    hcw_toggle_button_apply_compact_style(EGUI_VIEW_OF(&test_button));
    EGUI_TEST_ASSERT_EQUAL_INT(7, test_button.corner_radius);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x0C7C73).full, test_button.on_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xDBEAE5).full, test_button.off_color.full);

    hcw_toggle_button_apply_read_only_style(EGUI_VIEW_OF(&test_button));
    EGUI_TEST_ASSERT_EQUAL_INT(7, test_button.corner_radius);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xAFB8C3).full, test_button.on_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xF3F6F8).full, test_button.off_color.full);
}

static void test_toggle_button_unhandled_key_does_not_toggle(void)
{
    setup_button(0);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_FALSE(egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&test_button)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, toggled_count);
}

void test_toggle_button_run(void)
{
    EGUI_TEST_SUITE_BEGIN(toggle_button);
    EGUI_TEST_RUN(test_toggle_button_touch_toggles_state);
    EGUI_TEST_RUN(test_toggle_button_enter_and_space_toggle_state);
    EGUI_TEST_RUN(test_toggle_button_disabled_ignores_input);
    EGUI_TEST_RUN(test_toggle_button_style_helpers_update_palette);
    EGUI_TEST_RUN(test_toggle_button_unhandled_key_does_not_toggle);
    EGUI_TEST_SUITE_END();
}
