#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_chapter_strip.h"

#include "../../HelloCustomWidgets/media/chapter_strip/egui_view_chapter_strip.h"
#include "../../HelloCustomWidgets/media/chapter_strip/egui_view_chapter_strip.c"

static egui_view_chapter_strip_t test_strip;

static const egui_view_chapter_strip_item_t unit_items[] = {
        {"Intro", "Cold open", "00:12", EGUI_COLOR_HEX(0x2563EB)},
        {"Host", "Scene setup", "02:18", EGUI_COLOR_HEX(0x2563EB)},
        {"Guest", "Ad break", "07:40", EGUI_COLOR_HEX(0x2563EB)},
        {"Wrap", "Closing cue", "18:45", EGUI_COLOR_HEX(0x2563EB)},
};

static void setup_strip(uint8_t count, uint8_t current_index)
{
    egui_view_chapter_strip_init(EGUI_VIEW_OF(&test_strip));
    egui_view_set_size(EGUI_VIEW_OF(&test_strip), 164, 96);
    egui_view_chapter_strip_set_items(EGUI_VIEW_OF(&test_strip), unit_items, count, current_index);
    egui_view_chapter_strip_set_current_part(EGUI_VIEW_OF(&test_strip), EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER);
}

static void layout_strip(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 164;
    region.size.height = 96;
    egui_view_layout(EGUI_VIEW_OF(&test_strip), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_strip)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_strip)->api->on_touch_event(EGUI_VIEW_OF(&test_strip), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_strip)->api->on_key_event(EGUI_VIEW_OF(&test_strip), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_strip)->api->on_key_event(EGUI_VIEW_OF(&test_strip), &event);
    return handled;
}

static void test_chapter_strip_clamps_metrics_and_regions(void)
{
    egui_region_t region;

    setup_strip(4, 9);
    layout_strip();
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_chapter_strip_get_chapter_count(EGUI_VIEW_OF(&test_strip)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_chapter_strip_get_current_index(EGUI_VIEW_OF(&test_strip)));
    EGUI_TEST_ASSERT_TRUE(egui_view_chapter_strip_get_part_region(EGUI_VIEW_OF(&test_strip), EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER, 3, &region));
    EGUI_TEST_ASSERT_FALSE(egui_view_chapter_strip_get_part_region(EGUI_VIEW_OF(&test_strip), EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER, 4, &region));
}

static void test_chapter_strip_tab_cycles_parts(void)
{
    setup_strip(4, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER, egui_view_chapter_strip_get_current_part(EGUI_VIEW_OF(&test_strip)));
    EGUI_TEST_ASSERT_TRUE(egui_view_chapter_strip_handle_navigation_key(EGUI_VIEW_OF(&test_strip), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_CHAPTER_STRIP_PART_NEXT, egui_view_chapter_strip_get_current_part(EGUI_VIEW_OF(&test_strip)));
    EGUI_TEST_ASSERT_TRUE(egui_view_chapter_strip_handle_navigation_key(EGUI_VIEW_OF(&test_strip), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS, egui_view_chapter_strip_get_current_part(EGUI_VIEW_OF(&test_strip)));
    EGUI_TEST_ASSERT_TRUE(egui_view_chapter_strip_handle_navigation_key(EGUI_VIEW_OF(&test_strip), EGUI_KEY_CODE_TAB));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER, egui_view_chapter_strip_get_current_part(EGUI_VIEW_OF(&test_strip)));
}

static void test_chapter_strip_keyboard_navigation(void)
{
    setup_strip(4, 1);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_chapter_strip_get_current_index(EGUI_VIEW_OF(&test_strip)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_HOME));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_chapter_strip_get_current_index(EGUI_VIEW_OF(&test_strip)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_END));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_chapter_strip_get_current_index(EGUI_VIEW_OF(&test_strip)));
}

static void test_chapter_strip_plus_minus_steps_chapters(void)
{
    setup_strip(4, 1);
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_PLUS));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_chapter_strip_get_current_index(EGUI_VIEW_OF(&test_strip)));
    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_MINUS));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_chapter_strip_get_current_index(EGUI_VIEW_OF(&test_strip)));
}

static void test_chapter_strip_touch_previous_next(void)
{
    egui_region_t region;

    setup_strip(4, 1);
    layout_strip();
    EGUI_TEST_ASSERT_TRUE(egui_view_chapter_strip_get_part_region(EGUI_VIEW_OF(&test_strip), EGUI_VIEW_CHAPTER_STRIP_PART_PREVIOUS, 0, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_chapter_strip_get_current_index(EGUI_VIEW_OF(&test_strip)));

    EGUI_TEST_ASSERT_TRUE(egui_view_chapter_strip_get_part_region(EGUI_VIEW_OF(&test_strip), EGUI_VIEW_CHAPTER_STRIP_PART_NEXT, 0, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_chapter_strip_get_current_index(EGUI_VIEW_OF(&test_strip)));
}

static void test_chapter_strip_touch_chapter_selects_visible_block(void)
{
    egui_region_t region;

    setup_strip(4, 1);
    layout_strip();
    EGUI_TEST_ASSERT_TRUE(egui_view_chapter_strip_get_part_region(EGUI_VIEW_OF(&test_strip), EGUI_VIEW_CHAPTER_STRIP_PART_CHAPTER, 3, &region));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_chapter_strip_get_current_index(EGUI_VIEW_OF(&test_strip)));
}

static void test_chapter_strip_read_only_and_compact_ignore_input(void)
{
    egui_region_t region;

    setup_strip(4, 1);
    egui_view_chapter_strip_set_read_only_mode(EGUI_VIEW_OF(&test_strip), 1);
    layout_strip();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(egui_view_chapter_strip_get_part_region(EGUI_VIEW_OF(&test_strip), EGUI_VIEW_CHAPTER_STRIP_PART_NEXT, 0, &region));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_chapter_strip_get_current_index(EGUI_VIEW_OF(&test_strip)));

    setup_strip(4, 1);
    egui_view_chapter_strip_set_compact_mode(EGUI_VIEW_OF(&test_strip), 1);
    layout_strip();
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_RIGHT));
    EGUI_TEST_ASSERT_TRUE(egui_view_chapter_strip_get_part_region(EGUI_VIEW_OF(&test_strip), EGUI_VIEW_CHAPTER_STRIP_PART_NEXT, 0, &region));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, region.location.x + region.size.width / 2, region.location.y + region.size.height / 2));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_chapter_strip_get_current_index(EGUI_VIEW_OF(&test_strip)));
}

void test_chapter_strip_run(void)
{
    EGUI_TEST_SUITE_BEGIN(chapter_strip);
    EGUI_TEST_RUN(test_chapter_strip_clamps_metrics_and_regions);
    EGUI_TEST_RUN(test_chapter_strip_tab_cycles_parts);
    EGUI_TEST_RUN(test_chapter_strip_keyboard_navigation);
    EGUI_TEST_RUN(test_chapter_strip_plus_minus_steps_chapters);
    EGUI_TEST_RUN(test_chapter_strip_touch_previous_next);
    EGUI_TEST_RUN(test_chapter_strip_touch_chapter_selects_visible_block);
    EGUI_TEST_RUN(test_chapter_strip_read_only_and_compact_ignore_input);
    EGUI_TEST_SUITE_END();
}
