#include "egui.h"
#include "test/egui_test.h"
#include "test_token_input.h"

#include <string.h>

#include "../../HelloCustomWidgets/input/token_input/egui_view_token_input.h"
#include "../../HelloCustomWidgets/input/token_input/egui_view_token_input.c"

static egui_view_token_input_t test_token_input;
static uint8_t g_changed_token_count = 0xFF;
static uint8_t g_changed_part = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
static uint8_t g_changed_count = 0;

static void reset_changed_state(void)
{
    g_changed_token_count = 0xFF;
    g_changed_part = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    g_changed_count = 0;
}

static void on_token_input_changed(egui_view_t *self, uint8_t token_count, uint8_t part)
{
    EGUI_UNUSED(self);
    g_changed_token_count = token_count;
    g_changed_part = part;
    g_changed_count++;
}

static void setup_token_input(const char **tokens, uint8_t count)
{
    egui_view_token_input_init(EGUI_VIEW_OF(&test_token_input));
    egui_view_token_input_set_on_changed_listener(EGUI_VIEW_OF(&test_token_input), on_token_input_changed);
    egui_view_token_input_set_placeholder(EGUI_VIEW_OF(&test_token_input), "Add token");
    egui_view_token_input_set_tokens(EGUI_VIEW_OF(&test_token_input), tokens, count);
    reset_changed_state();
}

static void layout_token_input(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_token_input), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_token_input)->region_screen, &region);
}

static int send_touch_event(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_token_input)->api->on_touch_event(EGUI_VIEW_OF(&test_token_input), &event);
}

static int send_key_event(uint8_t type, uint8_t key_code)
{
    egui_key_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key_code = key_code;
    return EGUI_VIEW_OF(&test_token_input)->api->on_key_event(EGUI_VIEW_OF(&test_token_input), &event);
}

static void assert_changed_state(uint8_t count, uint8_t token_count, uint8_t part)
{
    EGUI_TEST_ASSERT_EQUAL_INT(count, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(token_count, g_changed_token_count);
    EGUI_TEST_ASSERT_EQUAL_INT(part, g_changed_part);
}

static void test_token_input_enter_commits_draft_token(void)
{
    static const char *tokens[] = {"alpha", "beta", "gamma"};

    setup_token_input(tokens, 3);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_N));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_N));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_E));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_E));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_T));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_T));
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 3)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    assert_changed_state(1, 4, EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
}

static void test_token_input_comma_and_space_commit_tokens(void)
{
    setup_token_input(NULL, 0);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_A));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_A));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_COMMA));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_COMMA));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("a", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 0)) == 0);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_B));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_B));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_SPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("b", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 1)) == 0);
    assert_changed_state(2, 2, EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
}

static void test_token_input_backspace_and_delete_remove_expected_targets(void)
{
    static const char *tokens[] = {"alpha", "beta"};

    setup_token_input(tokens, 2);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_C));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_C));
    EGUI_TEST_ASSERT_TRUE(strcmp("c", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(strcmp("", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("alpha", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 0)) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    assert_changed_state(1, 1, EGUI_VIEW_TOKEN_INPUT_PART_INPUT);

    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), 0);
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_DELETE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DELETE));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_NULL(egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    assert_changed_state(2, 0, 0);
}

static void test_token_input_delete_last_focused_token_keeps_previous_focus(void)
{
    static const char *tokens[] = {"alpha", "beta", "gamma"};

    setup_token_input(tokens, 3);

    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), 2);
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_DELETE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_DELETE));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("beta", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 1)) == 0);
    assert_changed_state(1, 2, 2);
}

static void test_token_input_backspace_prefers_focused_token_over_draft(void)
{
    static const char *tokens[] = {"alpha", "beta"};

    setup_token_input(tokens, 2);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_C));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_C));
    EGUI_TEST_ASSERT_TRUE(strcmp("c", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("alpha", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 0)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("c", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    assert_changed_state(1, 1, 1);
}

static void test_token_input_touch_remove_icon_deletes_token(void)
{
    static const char *tokens[] = {"alpha", "beta", "gamma"};
    egui_region_t remove_region;
    egui_dim_t x;
    egui_dim_t y;

    setup_token_input(tokens, 3);
    layout_token_input(12, 18, 196, 92);

    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_remove_region(EGUI_VIEW_OF(&test_token_input), 1, &remove_region));
    x = remove_region.location.x + remove_region.size.width / 2;
    y = remove_region.location.y + remove_region.size.height / 2;

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.pressed_remove);

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("gamma", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 1)) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_NONE, test_token_input.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_token_input.pressed_remove);
    assert_changed_state(1, 2, 1);
}

static void test_token_input_navigation_cycles_between_tokens_and_input(void)
{
    static const char *tokens[] = {"alpha", "beta", "gamma"};

    setup_token_input(tokens, 3);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_token_input_remove_token_before_focus_shifts_index_left(void)
{
    static const char *tokens[] = {"alpha", "beta", "gamma"};

    setup_token_input(tokens, 3);

    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), 2);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_remove_token(EGUI_VIEW_OF(&test_token_input), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("gamma", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 1)) == 0);
    assert_changed_state(1, 2, 0);
}

static void test_token_input_compact_overflow_hides_tail_tokens(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys", "net"};
    egui_region_t token_region;
    egui_region_t input_region;
    egui_region_t remove_region;

    setup_token_input(tokens, 5);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    layout_token_input(12, 18, 106, 48);

    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), 0, &token_region));
    EGUI_TEST_ASSERT_TRUE(token_region.size.width > 0);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), 3, &token_region));
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), 4, &token_region));
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_remove_region(EGUI_VIEW_OF(&test_token_input), 3, &remove_region));
}

static void test_token_input_compact_overflow_blank_area_does_not_focus_hidden_input(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys", "net"};
    egui_region_t work_region;
    egui_dim_t x;
    egui_dim_t y;

    setup_token_input(tokens, 5);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    layout_token_input(12, 18, 106, 48);
    egui_view_get_work_region(EGUI_VIEW_OF(&test_token_input), &work_region);
    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), 0);

    x = EGUI_VIEW_OF(&test_token_input)->region_screen.location.x + work_region.location.x + work_region.size.width - 4;
    y = EGUI_VIEW_OF(&test_token_input)->region_screen.location.y + work_region.location.y + work_region.size.height - 4;

    EGUI_TEST_ASSERT_FALSE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_NONE, test_token_input.pressed_part);
}

static void test_token_input_compact_overflow_navigation_skips_hidden_parts(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys", "net"};
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(tokens, 5);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    layout_token_input(12, 18, 106, 48);

    for (index = 0; index < 5; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }
    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_TRUE(last_visible < 4);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), (uint8_t)(last_visible + 1), &token_region));

    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_LEFT));
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
}

static void test_token_input_read_only_overflow_hides_tail_tokens(void)
{
    static const char *tokens[] = {"audit", "factory", "pinned", "shift", "fleet"};
    egui_region_t token_region;
    egui_region_t input_region;
    egui_region_t remove_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(tokens, 5);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    egui_view_token_input_set_read_only_mode(EGUI_VIEW_OF(&test_token_input), 1);
    layout_token_input(12, 18, 106, 48);

    for (index = 0; index < 5; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_TRUE(last_visible < 4);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), (uint8_t)(last_visible + 1), &token_region));
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_remove_region(EGUI_VIEW_OF(&test_token_input), 0, &remove_region));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), 4);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
}

static void test_token_input_relayout_normalizes_hidden_input_focus(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys"};
    egui_region_t input_region;
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(tokens, 4);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    layout_token_input(12, 18, 260, 48);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    layout_token_input(12, 18, 106, 48);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    for (index = 0; index < 4; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
}

static void test_token_input_set_tokens_normalizes_internal_focus_for_overflow_layout(void)
{
    static const char *start_tokens[] = {"ui", "qa"};
    static const char *overflow_tokens[] = {"ui", "qa", "ops", "sys", "net"};
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(start_tokens, 2);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    layout_token_input(12, 18, 106, 48);
    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, test_token_input.current_part);

    egui_view_token_input_set_tokens(EGUI_VIEW_OF(&test_token_input), overflow_tokens, 5);
    for (index = 0; index < 5; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, test_token_input.current_part);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
}

static void test_token_input_add_token_normalizes_internal_focus_for_overflow_layout(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys"};
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(tokens, 4);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    layout_token_input(12, 18, 106, 48);
    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, test_token_input.current_part);

    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), "net"));
    for (index = 0; index < 5; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, test_token_input.current_part);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
}

static void test_token_input_set_tokens_clears_hidden_pressed_state_for_overflow_layout(void)
{
    static const char *start_tokens[] = {"ui", "qa"};
    static const char *overflow_tokens[] = {"ui", "qa", "ops", "sys", "net"};
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(start_tokens, 2);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    layout_token_input(12, 18, 106, 48);
    test_token_input.current_part = EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
    test_token_input.pressed_part = EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
    test_token_input.pressed_remove = 1;

    egui_view_token_input_set_tokens(EGUI_VIEW_OF(&test_token_input), overflow_tokens, 5);
    for (index = 0; index < 5; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, test_token_input.current_part);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_NONE, test_token_input.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_token_input.pressed_remove);
}

static void test_token_input_set_read_only_mode_clears_input_pressed_state_after_layout(void)
{
    static const char *tokens[] = {"alpha", "beta"};

    setup_token_input(tokens, 2);
    layout_token_input(12, 18, 196, 92);
    test_token_input.current_part = EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
    test_token_input.pressed_part = EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
    test_token_input.pressed_remove = 1;

    egui_view_token_input_set_read_only_mode(EGUI_VIEW_OF(&test_token_input), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_token_input.current_part);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_NONE, test_token_input.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_token_input.pressed_remove);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
}

static void test_token_input_compact_hides_input_when_wrapped_outside_view(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys"};
    egui_region_t input_region;
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(tokens, 4);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    layout_token_input(12, 18, 106, 48);

    for (index = 0; index < 4; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
}

static void test_token_input_hidden_input_ignores_printable_keys(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys"};
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(tokens, 4);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    layout_token_input(12, 18, 106, 48);

    for (index = 0; index < 4; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_C));
    EGUI_TEST_ASSERT_FALSE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_C));
    EGUI_TEST_ASSERT_TRUE(strcmp("", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_token_input_hidden_input_keeps_existing_draft_without_commit(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys"};
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(tokens, 4);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    test_token_input.draft_len = 3;
    memcpy(test_token_input.draft_text, "net", 4);
    layout_token_input(12, 18, 106, 48);

    for (index = 0; index < 4; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_FALSE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_token_input_hidden_draft_commits_after_input_returns(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys"};
    egui_region_t input_region;
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(tokens, 4);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    test_token_input.draft_len = 3;
    memcpy(test_token_input.draft_text, "net", 4);

    layout_token_input(12, 18, 106, 48);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);

    layout_token_input(12, 18, 188, 48);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 4)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);

    for (index = 0; index < 5; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    assert_changed_state(1, 5, EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
}

static void test_token_input_hidden_draft_restores_input_focus_after_layout_returns(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys"};
    egui_region_t input_region;
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(tokens, 4);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    test_token_input.draft_len = 3;
    memcpy(test_token_input.draft_text, "net", 4);

    layout_token_input(12, 18, 106, 48);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));

    for (index = 0; index < 4; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    layout_token_input(12, 18, 260, 48);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("ne", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_token_input_compact_toggle_restores_draft_input_focus(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys"};
    egui_region_t input_region;
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(tokens, 4);
    layout_token_input(12, 18, 196, 92);
    test_token_input.draft_len = 3;
    memcpy(test_token_input.draft_text, "net", 4);
    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT);

    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    layout_token_input(12, 18, 106, 48);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));

    for (index = 0; index < 4; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.restore_input_focus);

    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 0);
    layout_token_input(12, 18, 196, 92);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("ne", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_token_input_compact_toggle_respects_explicit_token_focus(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys"};
    egui_region_t input_region;

    setup_token_input(tokens, 4);
    layout_token_input(12, 18, 196, 92);
    test_token_input.draft_len = 3;
    memcpy(test_token_input.draft_text, "net", 4);
    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT);

    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    layout_token_input(12, 18, 106, 48);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)) != EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.restore_input_focus);

    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_token_input.restore_input_focus);

    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 0);
    layout_token_input(12, 18, 196, 92);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("qa", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 0)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
}

static void test_token_input_part_region_hides_input_when_read_only(void)
{
    static const char *tokens[] = {"alpha", "beta"};
    egui_region_t token_region;
    egui_region_t input_region;
    egui_region_t remove_region;

    setup_token_input(tokens, 2);
    layout_token_input(12, 18, 196, 92);

    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), 0, &token_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_remove_region(EGUI_VIEW_OF(&test_token_input), 0, &remove_region));
    EGUI_TEST_ASSERT_TRUE(token_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(input_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(remove_region.size.width > 0);
    EGUI_TEST_ASSERT_TRUE(input_region.location.y >= token_region.location.y);

    egui_view_token_input_set_read_only_mode(EGUI_VIEW_OF(&test_token_input), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), 0, &token_region));
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_remove_region(EGUI_VIEW_OF(&test_token_input), 0, &remove_region));

    egui_view_token_input_set_tokens(EGUI_VIEW_OF(&test_token_input), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_NONE, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), 0, &token_region));
}

static void test_token_input_touch_selects_token_and_input_parts(void)
{
    static const char *tokens[] = {"alpha", "beta"};
    egui_region_t token_region;
    egui_region_t input_region;
    egui_dim_t x;
    egui_dim_t y;

    setup_token_input(tokens, 2);
    layout_token_input(12, 18, 196, 92);

    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), 1, &token_region));
    x = token_region.location.x + token_region.size.width / 2;
    y = token_region.location.y + token_region.size.height / 2;
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.pressed_part);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_token_input)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_NONE, test_token_input.pressed_part);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_token_input)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    x = input_region.location.x + input_region.size.width / 2;
    y = input_region.location.y + input_region.size.height / 2;
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, test_token_input.pressed_part);
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_NONE, test_token_input.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_token_input_touch_blank_area_focuses_input(void)
{
    static const char *tokens[] = {"alpha", "beta"};
    egui_region_t work_region;
    egui_region_t input_region;
    egui_dim_t x;
    egui_dim_t y;

    setup_token_input(tokens, 2);
    layout_token_input(12, 18, 196, 92);
    egui_view_get_work_region(EGUI_VIEW_OF(&test_token_input), &work_region);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));

    x = EGUI_VIEW_OF(&test_token_input)->region_screen.location.x + work_region.location.x + work_region.size.width / 2;
    y = EGUI_VIEW_OF(&test_token_input)->region_screen.location.y + work_region.location.y + work_region.size.height - 8;
    EGUI_TEST_ASSERT_FALSE(egui_region_pt_in_rect(&input_region, x, y));

    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, test_token_input.pressed_part);
    EGUI_TEST_ASSERT_TRUE(send_touch_event(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_NONE, test_token_input.pressed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_token_input_read_only_and_disabled_key_guard(void)
{
    static const char *tokens[] = {"alpha", "beta"};

    setup_token_input(tokens, 2);

    egui_view_token_input_set_read_only_mode(EGUI_VIEW_OF(&test_token_input), 1);
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_FALSE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);

    egui_view_token_input_set_read_only_mode(EGUI_VIEW_OF(&test_token_input), 0);
    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    egui_view_set_enable(EGUI_VIEW_OF(&test_token_input), 0);
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_FALSE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_token_input_read_only_toggle_restores_draft_input_focus(void)
{
    static const char *tokens[] = {"alpha", "beta"};
    egui_region_t input_region;

    setup_token_input(tokens, 2);
    layout_token_input(12, 18, 196, 92);
    test_token_input.draft_len = 3;
    memcpy(test_token_input.draft_text, "net", 4);
    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT);

    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    egui_view_token_input_set_read_only_mode(EGUI_VIEW_OF(&test_token_input), 1);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    egui_view_token_input_set_read_only_mode(EGUI_VIEW_OF(&test_token_input), 0);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("ne", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_token_input_clear_draft_clears_hidden_restore_flag(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys"};
    egui_region_t input_region;
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(tokens, 4);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    test_token_input.draft_len = 3;
    memcpy(test_token_input.draft_text, "net", 4);

    layout_token_input(12, 18, 106, 48);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));

    for (index = 0; index < 4; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.restore_input_focus);
    egui_view_token_input_clear_draft(EGUI_VIEW_OF(&test_token_input));
    EGUI_TEST_ASSERT_TRUE(strcmp("", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_token_input.restore_input_focus);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    layout_token_input(12, 18, 260, 48);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_token_input_hidden_navigation_cancels_pending_input_restore(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys", "net2"};
    egui_region_t input_region;
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;
    uint8_t focused_part;

    setup_token_input(tokens, 5);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    test_token_input.draft_len = 3;
    memcpy(test_token_input.draft_text, "net", 4);

    layout_token_input(12, 18, 106, 48);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));

    for (index = 0; index < 5; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.restore_input_focus);

    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_TAB));
    focused_part = egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input));
    EGUI_TEST_ASSERT_EQUAL_INT(0, focused_part);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_token_input.restore_input_focus);
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);

    layout_token_input(12, 18, 188, 48);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_EQUAL_INT(focused_part, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(focused_part, g_changed_part);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
}

static void test_token_input_set_tokens_clears_hidden_restore_flag(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys"};
    static const char *next_tokens[] = {"audit", "ops", "mail", "sync", "ux"};
    egui_region_t input_region;
    egui_region_t token_region;
    uint8_t last_visible = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    uint8_t index;

    setup_token_input(tokens, 4);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    test_token_input.draft_len = 3;
    memcpy(test_token_input.draft_text, "net", 4);

    layout_token_input(12, 18, 106, 48);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)) != EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.restore_input_focus);

    egui_view_token_input_set_tokens(EGUI_VIEW_OF(&test_token_input), next_tokens, 5);
    EGUI_TEST_ASSERT_TRUE(strcmp("", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_token_input.restore_input_focus);

    for (index = 0; index < 5; index++)
    {
        if (egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), index, &token_region))
        {
            last_visible = index;
        }
    }

    EGUI_TEST_ASSERT_TRUE(last_visible != EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    layout_token_input(12, 18, 260, 48);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_EQUAL_INT(last_visible, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_token_input_remove_token_preserves_hidden_draft_restore(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys", "mail"};
    egui_region_t input_region;

    setup_token_input(tokens, 5);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    test_token_input.draft_len = 3;
    memcpy(test_token_input.draft_text, "net", 4);

    layout_token_input(12, 18, 106, 48);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)) != EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.restore_input_focus);

    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_remove_token(EGUI_VIEW_OF(&test_token_input), 0));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("qa", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 0)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.restore_input_focus);
    assert_changed_state(1, 4, 0);

    layout_token_input(12, 18, 196, 48);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(strcmp("ne", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
}

static void test_token_input_add_token_preserves_hidden_draft_restore(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys", "mail"};
    egui_region_t input_region;

    setup_token_input(tokens, 5);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    test_token_input.draft_len = 3;
    memcpy(test_token_input.draft_text, "net", 4);

    layout_token_input(12, 18, 106, 48);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)) != EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.restore_input_focus);

    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), "sync"));
    EGUI_TEST_ASSERT_EQUAL_INT(6, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("sync", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 5)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.restore_input_focus);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)) != EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    assert_changed_state(1, 6, EGUI_VIEW_TOKEN_INPUT_PART_INPUT);

    layout_token_input(12, 18, 260, 48);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(strcmp("ne", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(6, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_changed_count);
}

static void test_token_input_add_token_hidden_restore_respects_explicit_token_focus(void)
{
    static const char *tokens[] = {"ui", "qa", "ops", "sys", "mail"};
    egui_region_t input_region;

    setup_token_input(tokens, 5);
    egui_view_token_input_set_compact_mode(EGUI_VIEW_OF(&test_token_input), 1);
    test_token_input.draft_len = 3;
    memcpy(test_token_input.draft_text, "net", 4);

    layout_token_input(12, 18, 106, 48);
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)) != EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.restore_input_focus);

    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), "sync"));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_token_input.restore_input_focus);
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);

    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_token_input.restore_input_focus);

    layout_token_input(12, 18, 260, 48);
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_get_part_region(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT, &input_region));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_TRUE(send_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_BACKSPACE));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("qa", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 0)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("net", egui_view_token_input_get_draft_text(EGUI_VIEW_OF(&test_token_input))) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_changed_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_part);
}

static void test_token_input_set_current_part_ignores_hidden_targets(void)
{
    static const char *tokens[] = {"alpha", "beta"};

    setup_token_input(tokens, 2);

    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    egui_view_token_input_set_read_only_mode(EGUI_VIEW_OF(&test_token_input), 1);
    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    egui_view_token_input_set_tokens(EGUI_VIEW_OF(&test_token_input), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_NONE, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    egui_view_token_input_set_current_part(EGUI_VIEW_OF(&test_token_input), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_NONE, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));

    egui_view_token_input_set_read_only_mode(EGUI_VIEW_OF(&test_token_input), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

static void test_token_input_add_token_clamps_at_max_capacity(void)
{
    static const char *tokens[] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7"};

    setup_token_input(NULL, 0);

    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), tokens[0]));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), tokens[1]));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), tokens[2]));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), tokens[3]));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), tokens[4]));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), tokens[5]));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), tokens[6]));
    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), tokens[7]));
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), "overflow"));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("t7", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 7)) == 0);
    assert_changed_state(8, 8, EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
}

static void test_token_input_add_token_trims_surrounding_whitespace(void)
{
    setup_token_input(NULL, 0);

    EGUI_TEST_ASSERT_TRUE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), "  alpha \t"));
    EGUI_TEST_ASSERT_FALSE(egui_view_token_input_add_token(EGUI_VIEW_OF(&test_token_input), "   \t  "));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("alpha", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 0)) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    assert_changed_state(1, 1, EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
}

static void test_token_input_set_tokens_skips_empty_and_whitespace_entries(void)
{
    static const char *tokens[] = {" alpha ", "", "  \t", NULL, "beta "};

    setup_token_input(NULL, 0);
    egui_view_token_input_set_tokens(EGUI_VIEW_OF(&test_token_input), tokens, 5);

    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_token_input_get_token_count(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_TRUE(strcmp("alpha", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 0)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("beta", egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 1)) == 0);
    EGUI_TEST_ASSERT_NULL(egui_view_token_input_get_token(EGUI_VIEW_OF(&test_token_input), 2));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TOKEN_INPUT_PART_INPUT, egui_view_token_input_get_current_part(EGUI_VIEW_OF(&test_token_input)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_changed_count);
}

void test_token_input_run(void)
{
    EGUI_TEST_SUITE_BEGIN(token_input);

    EGUI_TEST_RUN(test_token_input_enter_commits_draft_token);
    EGUI_TEST_RUN(test_token_input_comma_and_space_commit_tokens);
    EGUI_TEST_RUN(test_token_input_backspace_and_delete_remove_expected_targets);
    EGUI_TEST_RUN(test_token_input_delete_last_focused_token_keeps_previous_focus);
    EGUI_TEST_RUN(test_token_input_backspace_prefers_focused_token_over_draft);
    EGUI_TEST_RUN(test_token_input_touch_remove_icon_deletes_token);
    EGUI_TEST_RUN(test_token_input_navigation_cycles_between_tokens_and_input);
    EGUI_TEST_RUN(test_token_input_remove_token_before_focus_shifts_index_left);
    EGUI_TEST_RUN(test_token_input_compact_overflow_hides_tail_tokens);
    EGUI_TEST_RUN(test_token_input_compact_overflow_blank_area_does_not_focus_hidden_input);
    EGUI_TEST_RUN(test_token_input_compact_overflow_navigation_skips_hidden_parts);
    EGUI_TEST_RUN(test_token_input_read_only_overflow_hides_tail_tokens);
    EGUI_TEST_RUN(test_token_input_relayout_normalizes_hidden_input_focus);
    EGUI_TEST_RUN(test_token_input_set_tokens_normalizes_internal_focus_for_overflow_layout);
    EGUI_TEST_RUN(test_token_input_add_token_normalizes_internal_focus_for_overflow_layout);
    EGUI_TEST_RUN(test_token_input_set_tokens_clears_hidden_pressed_state_for_overflow_layout);
    EGUI_TEST_RUN(test_token_input_set_read_only_mode_clears_input_pressed_state_after_layout);
    EGUI_TEST_RUN(test_token_input_compact_hides_input_when_wrapped_outside_view);
    EGUI_TEST_RUN(test_token_input_hidden_input_ignores_printable_keys);
    EGUI_TEST_RUN(test_token_input_hidden_input_keeps_existing_draft_without_commit);
    EGUI_TEST_RUN(test_token_input_hidden_draft_commits_after_input_returns);
    EGUI_TEST_RUN(test_token_input_hidden_draft_restores_input_focus_after_layout_returns);
    EGUI_TEST_RUN(test_token_input_compact_toggle_restores_draft_input_focus);
    EGUI_TEST_RUN(test_token_input_compact_toggle_respects_explicit_token_focus);
    EGUI_TEST_RUN(test_token_input_part_region_hides_input_when_read_only);
    EGUI_TEST_RUN(test_token_input_touch_selects_token_and_input_parts);
    EGUI_TEST_RUN(test_token_input_touch_blank_area_focuses_input);
    EGUI_TEST_RUN(test_token_input_read_only_and_disabled_key_guard);
    EGUI_TEST_RUN(test_token_input_read_only_toggle_restores_draft_input_focus);
    EGUI_TEST_RUN(test_token_input_clear_draft_clears_hidden_restore_flag);
    EGUI_TEST_RUN(test_token_input_hidden_navigation_cancels_pending_input_restore);
    EGUI_TEST_RUN(test_token_input_set_tokens_clears_hidden_restore_flag);
    EGUI_TEST_RUN(test_token_input_remove_token_preserves_hidden_draft_restore);
    EGUI_TEST_RUN(test_token_input_add_token_preserves_hidden_draft_restore);
    EGUI_TEST_RUN(test_token_input_add_token_hidden_restore_respects_explicit_token_focus);
    EGUI_TEST_RUN(test_token_input_set_current_part_ignores_hidden_targets);
    EGUI_TEST_RUN(test_token_input_add_token_clamps_at_max_capacity);
    EGUI_TEST_RUN(test_token_input_add_token_trims_surrounding_whitespace);
    EGUI_TEST_RUN(test_token_input_set_tokens_skips_empty_and_whitespace_entries);

    EGUI_TEST_SUITE_END();
}
