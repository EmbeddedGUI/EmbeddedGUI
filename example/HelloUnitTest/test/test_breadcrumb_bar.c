#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_breadcrumb_bar.h"

#include "../../HelloCustomWidgets/navigation/breadcrumb_bar/egui_view_breadcrumb_bar.h"
#include "../../HelloCustomWidgets/navigation/breadcrumb_bar/egui_view_breadcrumb_bar.c"

static egui_view_breadcrumb_bar_t test_bar;
static int click_count;

static const char *g_items_a[] = {"Home", "Docs", "Nav", "Details"};
static const char *g_items_b[] = {"Home", "Demos", "Forms", "Value"};
static const char *g_items_long[] = {"Home", "Documents", "Navigation", "Details"};

static const egui_view_breadcrumb_bar_snapshot_t g_snapshots[] = {
        {"Docs path", g_items_a, 4, 3},
        {"Forms path", g_items_b, 4, 3},
        {"Long path", g_items_long, 4, 3},
};

static const egui_view_breadcrumb_bar_snapshot_t g_overflow_snapshots[] = {
        {"A", g_items_a, 4, 3}, {"B", g_items_b, 4, 3},    {"C", g_items_long, 4, 3}, {"D", g_items_a, 4, 3},
        {"E", g_items_b, 4, 3}, {"F", g_items_long, 4, 3}, {"G", g_items_a, 4, 3},
};

static void on_bar_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    click_count++;
}

static void setup_bar(void)
{
    egui_view_breadcrumb_bar_init(EGUI_VIEW_OF(&test_bar));
    egui_view_set_size(EGUI_VIEW_OF(&test_bar), 196, 48);
    egui_view_breadcrumb_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), g_snapshots, 3);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&test_bar), on_bar_click);
    click_count = 0;
}

static int send_touch(uint8_t type)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = 48;
    event.location.y = 26;
    return EGUI_VIEW_OF(&test_bar)->api->on_touch_event(EGUI_VIEW_OF(&test_bar), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_bar)->api->on_key_event(EGUI_VIEW_OF(&test_bar), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_bar)->api->on_key_event(EGUI_VIEW_OF(&test_bar), &event);
    return handled;
}

static void test_breadcrumb_bar_set_snapshots_clamp_and_reset_current(void)
{
    setup_bar();

    egui_view_breadcrumb_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), g_overflow_snapshots, 7);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BREADCRUMB_BAR_MAX_SNAPSHOTS, test_bar.snapshot_count);

    test_bar.current_snapshot = 5;
    egui_view_breadcrumb_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_bar.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_breadcrumb_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));

    egui_view_breadcrumb_bar_set_snapshots(EGUI_VIEW_OF(&test_bar), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_bar.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_breadcrumb_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));
}

static void test_breadcrumb_bar_set_current_snapshot_ignores_out_of_range(void)
{
    setup_bar();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_breadcrumb_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));
    egui_view_breadcrumb_bar_set_current_snapshot(EGUI_VIEW_OF(&test_bar), 2);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_breadcrumb_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));

    egui_view_breadcrumb_bar_set_current_snapshot(EGUI_VIEW_OF(&test_bar), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_breadcrumb_bar_get_current_snapshot(EGUI_VIEW_OF(&test_bar)));
}

static void test_breadcrumb_bar_font_modes_and_palette_update(void)
{
    setup_bar();

    egui_view_breadcrumb_bar_set_font(EGUI_VIEW_OF(&test_bar), NULL);
    EGUI_TEST_ASSERT_TRUE(test_bar.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_breadcrumb_bar_set_compact_mode(EGUI_VIEW_OF(&test_bar), 2);
    egui_view_breadcrumb_bar_set_locked_mode(EGUI_VIEW_OF(&test_bar), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_bar.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_bar.locked_mode);

    egui_view_breadcrumb_bar_set_palette(EGUI_VIEW_OF(&test_bar), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                         EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_bar.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_bar.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_bar.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_bar.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_bar.accent_color.full);
}

static void test_breadcrumb_bar_touch_and_key_click_listener(void)
{
    setup_bar();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_bar)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_bar)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, click_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);

    egui_view_set_enable(EGUI_VIEW_OF(&test_bar), 0);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_bar)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

static void test_breadcrumb_bar_internal_helpers_cover_entries_and_labels(void)
{
    egui_view_breadcrumb_bar_display_entry_t entries[4];
    egui_view_breadcrumb_bar_display_entry_t copied[4];
    egui_view_breadcrumb_bar_display_entry_t entry;
    char label[16];
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BREADCRUMB_BAR_MAX_SNAPSHOTS, egui_view_breadcrumb_bar_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BREADCRUMB_BAR_MAX_ITEMS, egui_view_breadcrumb_bar_clamp_item_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_breadcrumb_bar_text_len(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(4, egui_view_breadcrumb_bar_text_len("Home"));
    EGUI_TEST_ASSERT_EQUAL_INT(10, egui_view_breadcrumb_bar_separator_gap(0));
    EGUI_TEST_ASSERT_EQUAL_INT(5, egui_view_breadcrumb_bar_separator_gap(1));

    egui_view_breadcrumb_bar_copy_elided(label, sizeof(label), "Documents", 6);
    EGUI_TEST_ASSERT_TRUE(strcmp("Doc...", label) == 0);
    egui_view_breadcrumb_bar_copy_elided(label, sizeof(label), "Long", 3);
    EGUI_TEST_ASSERT_TRUE(strcmp("...", label) == 0);

    egui_view_breadcrumb_bar_set_item_entry(&entry, 3);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BREADCRUMB_BAR_ENTRY_ITEM, entry.kind);
    EGUI_TEST_ASSERT_EQUAL_INT(3, entry.item_index);
    egui_view_breadcrumb_bar_set_overflow_entry(&entry);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BREADCRUMB_BAR_ENTRY_OVERFLOW, entry.kind);
    EGUI_TEST_ASSERT_EQUAL_INT(0, entry.item_index);

    egui_view_breadcrumb_bar_set_item_entry(&entries[0], 0);
    egui_view_breadcrumb_bar_set_overflow_entry(&entries[1]);
    egui_view_breadcrumb_bar_set_item_entry(&entries[2], 3);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_breadcrumb_bar_copy_entries(copied, entries, 3));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BREADCRUMB_BAR_ENTRY_ITEM, copied[0].kind);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BREADCRUMB_BAR_ENTRY_OVERFLOW, copied[1].kind);
    EGUI_TEST_ASSERT_EQUAL_INT(3, copied[2].item_index);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_breadcrumb_bar_prepare_entry_label(&g_snapshots[2], 0, 3, &entries[1], label, sizeof(label)));
    EGUI_TEST_ASSERT_TRUE(strcmp("...", label) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_breadcrumb_bar_prepare_entry_label(&g_snapshots[2], 1, 3, &entries[2], label, sizeof(label)));
    EGUI_TEST_ASSERT_TRUE(strcmp("Det...", label) == 0);

    EGUI_TEST_ASSERT_EQUAL_INT(14, egui_view_breadcrumb_bar_measure_entry(0, 0, "..."));
    EGUI_TEST_ASSERT_EQUAL_INT(10, egui_view_breadcrumb_bar_measure_entry(1, 0, "..."));
    EGUI_TEST_ASSERT_TRUE(egui_view_breadcrumb_bar_measure_entry_set(&g_snapshots[0], 0, 3, entries, 3) > 0);

    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_breadcrumb_bar_build_entries(&g_snapshots[0], 0, 120, entries));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BREADCRUMB_BAR_ENTRY_ITEM, entries[0].kind);
    EGUI_TEST_ASSERT_EQUAL_INT(0, entries[0].item_index);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_BREADCRUMB_BAR_ENTRY_OVERFLOW, entries[1].kind);
    EGUI_TEST_ASSERT_EQUAL_INT(3, entries[2].item_index);

    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_breadcrumb_bar_build_entries(&g_snapshots[2], 1, 57, entries));
    EGUI_TEST_ASSERT_EQUAL_INT(0, entries[0].item_index);
    EGUI_TEST_ASSERT_EQUAL_INT(3, entries[1].item_index);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_breadcrumb_bar_build_entries(&g_snapshots[2], 1, 29, entries));
    EGUI_TEST_ASSERT_EQUAL_INT(3, entries[0].item_index);

    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 62).full, egui_view_breadcrumb_bar_mix_disabled(sample).full);
}

void test_breadcrumb_bar_run(void)
{
    EGUI_TEST_SUITE_BEGIN(breadcrumb_bar);
    EGUI_TEST_RUN(test_breadcrumb_bar_set_snapshots_clamp_and_reset_current);
    EGUI_TEST_RUN(test_breadcrumb_bar_set_current_snapshot_ignores_out_of_range);
    EGUI_TEST_RUN(test_breadcrumb_bar_font_modes_and_palette_update);
    EGUI_TEST_RUN(test_breadcrumb_bar_touch_and_key_click_listener);
    EGUI_TEST_RUN(test_breadcrumb_bar_internal_helpers_cover_entries_and_labels);
    EGUI_TEST_SUITE_END();
}
