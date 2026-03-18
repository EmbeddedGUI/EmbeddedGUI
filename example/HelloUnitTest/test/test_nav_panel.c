#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_nav_panel.h"

#include "../../HelloCustomWidgets/navigation/nav_panel/egui_view_nav_panel.h"
#include "../../HelloCustomWidgets/navigation/nav_panel/egui_view_nav_panel.c"

static egui_view_nav_panel_t test_nav_panel;
static uint8_t g_selection_index;
static uint8_t g_selection_count;

static const egui_view_nav_panel_item_t g_items_primary[] = {
        {"Overview", "O"},
        {"Library", "L"},
        {"People", "P"},
};

static const egui_view_nav_panel_item_t g_items_overflow[] = {
        {"Home", "H"}, {"Files", "F"}, {"Rules", "R"}, {"Audit", "A"}, {"Admin", "M"},
};

static void on_selection_changed(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    g_selection_index = index;
    g_selection_count++;
}

static void setup_nav_panel(void)
{
    egui_view_nav_panel_init(EGUI_VIEW_OF(&test_nav_panel));
    egui_view_set_size(EGUI_VIEW_OF(&test_nav_panel), 198, 112);
    egui_view_nav_panel_set_items(EGUI_VIEW_OF(&test_nav_panel), g_items_primary, 3);
    egui_view_nav_panel_set_header_text(EGUI_VIEW_OF(&test_nav_panel), "Workspace");
    egui_view_nav_panel_set_footer_text(EGUI_VIEW_OF(&test_nav_panel), "Settings");
    egui_view_nav_panel_set_footer_badge(EGUI_VIEW_OF(&test_nav_panel), "S");
    egui_view_nav_panel_set_on_selection_changed_listener(EGUI_VIEW_OF(&test_nav_panel), on_selection_changed);
    g_selection_index = EGUI_VIEW_NAV_PANEL_INDEX_NONE;
    g_selection_count = 0;
}

static void layout_nav_panel(egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(EGUI_VIEW_OF(&test_nav_panel), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_nav_panel)->region_screen, &region);
}

static int send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return EGUI_VIEW_OF(&test_nav_panel)->api->on_touch_event(EGUI_VIEW_OF(&test_nav_panel), &event);
}

static uint8_t get_item_center(uint8_t item_index, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_nav_panel_metrics_t metrics;

    egui_view_nav_panel_get_metrics(&test_nav_panel, EGUI_VIEW_OF(&test_nav_panel), &metrics);
    if (item_index >= metrics.visible_item_count)
    {
        return 0;
    }

    *x = metrics.item_regions[item_index].location.x + metrics.item_regions[item_index].size.width / 2;
    *y = metrics.item_regions[item_index].location.y + metrics.item_regions[item_index].size.height / 2;
    return 1;
}

static void test_nav_panel_set_items_and_current_index_clamp(void)
{
    setup_nav_panel();

    test_nav_panel.current_index = 9;
    egui_view_nav_panel_set_items(EGUI_VIEW_OF(&test_nav_panel), g_items_overflow, 5);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_nav_panel.item_count);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NAV_PANEL_MAX_ITEMS, egui_view_nav_panel_get_visible_item_count(&test_nav_panel));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_nav_panel_get_current_index(EGUI_VIEW_OF(&test_nav_panel)));
    EGUI_TEST_ASSERT_NOT_NULL(egui_view_nav_panel_get_item(&test_nav_panel, 3));
    EGUI_TEST_ASSERT_NULL(egui_view_nav_panel_get_item(&test_nav_panel, 4));

    egui_view_nav_panel_set_current_index(EGUI_VIEW_OF(&test_nav_panel), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(3, egui_view_nav_panel_get_current_index(EGUI_VIEW_OF(&test_nav_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);
    EGUI_TEST_ASSERT_EQUAL_INT(3, g_selection_index);

    egui_view_nav_panel_set_current_index(EGUI_VIEW_OF(&test_nav_panel), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);

    egui_view_nav_panel_set_items(EGUI_VIEW_OF(&test_nav_panel), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_nav_panel_get_visible_item_count(&test_nav_panel));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_nav_panel_get_current_index(EGUI_VIEW_OF(&test_nav_panel)));
}

static void test_nav_panel_font_palette_and_helper_functions(void)
{
    egui_view_nav_panel_item_t fallback_item = {"Files", ""};
    egui_view_nav_panel_item_t empty_item = {NULL, NULL};
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);

    setup_nav_panel();

    egui_view_nav_panel_set_font(EGUI_VIEW_OF(&test_nav_panel), NULL);
    egui_view_nav_panel_set_meta_font(EGUI_VIEW_OF(&test_nav_panel), NULL);
    EGUI_TEST_ASSERT_TRUE(test_nav_panel.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(test_nav_panel.meta_font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    test_nav_panel.pressed_index = 2;
    egui_view_nav_panel_set_compact_mode(EGUI_VIEW_OF(&test_nav_panel), 2);
    egui_view_nav_panel_set_locked_mode(EGUI_VIEW_OF(&test_nav_panel), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_nav_panel.compact_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_nav_panel.locked_mode);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NAV_PANEL_INDEX_NONE, test_nav_panel.pressed_index);

    egui_view_nav_panel_set_palette(EGUI_VIEW_OF(&test_nav_panel), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                    EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_nav_panel.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_nav_panel.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_nav_panel.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_nav_panel.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_nav_panel.accent_color.full);

    EGUI_TEST_ASSERT_TRUE(strcmp("O", egui_view_nav_panel_get_badge_text(&g_items_primary[0])) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("Files", egui_view_nav_panel_get_badge_text(&fallback_item)) == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp("", egui_view_nav_panel_get_badge_text(&empty_item)) == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, 68).full, egui_view_nav_panel_mix_disabled(sample).full);
}

static void test_nav_panel_metrics_and_hit_testing(void)
{
    egui_view_nav_panel_metrics_t metrics;
    egui_dim_t x;
    egui_dim_t y;

    setup_nav_panel();
    layout_nav_panel(198, 112);
    egui_view_nav_panel_get_metrics(&test_nav_panel, EGUI_VIEW_OF(&test_nav_panel), &metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(3, metrics.visible_item_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.show_header);
    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.show_footer);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NAV_PANEL_STANDARD_HEADER_HEIGHT, metrics.header_region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NAV_PANEL_STANDARD_FOOTER_HEIGHT, metrics.footer_region.size.height);
    EGUI_TEST_ASSERT_TRUE(metrics.item_regions[1].location.y > metrics.item_regions[0].location.y);

    EGUI_TEST_ASSERT_TRUE(get_item_center(1, &x, &y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_nav_panel_hit_item(&test_nav_panel, EGUI_VIEW_OF(&test_nav_panel), x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NAV_PANEL_INDEX_NONE, egui_view_nav_panel_hit_item(&test_nav_panel, EGUI_VIEW_OF(&test_nav_panel), 0, 0));

    egui_view_nav_panel_set_compact_mode(EGUI_VIEW_OF(&test_nav_panel), 1);
    layout_nav_panel(58, 74);
    egui_view_nav_panel_get_metrics(&test_nav_panel, EGUI_VIEW_OF(&test_nav_panel), &metrics);
    EGUI_TEST_ASSERT_EQUAL_INT(0, metrics.show_header);
    EGUI_TEST_ASSERT_EQUAL_INT(1, metrics.show_footer);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NAV_PANEL_COMPACT_FOOTER_HEIGHT, metrics.footer_region.size.height);
    EGUI_TEST_ASSERT_TRUE(metrics.item_regions[2].location.y > metrics.item_regions[1].location.y);
}

static void test_nav_panel_touch_selects_item_and_cancel_resets_pressed_state(void)
{
    egui_dim_t x;
    egui_dim_t y;

    setup_nav_panel();
    layout_nav_panel(198, 112);
    EGUI_TEST_ASSERT_TRUE(get_item_center(2, &x, &y));

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_nav_panel.pressed_index);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_nav_panel)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_nav_panel_get_current_index(EGUI_VIEW_OF(&test_nav_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_selection_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, g_selection_index);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NAV_PANEL_INDEX_NONE, test_nav_panel.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_nav_panel)->is_pressed);

    EGUI_TEST_ASSERT_TRUE(get_item_center(1, &x, &y));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_nav_panel.pressed_index);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_CANCEL, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(2, egui_view_nav_panel_get_current_index(EGUI_VIEW_OF(&test_nav_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NAV_PANEL_INDEX_NONE, test_nav_panel.pressed_index);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_nav_panel)->is_pressed);
}

static void test_nav_panel_locked_or_disabled_ignores_touch(void)
{
    egui_dim_t x;
    egui_dim_t y;

    setup_nav_panel();
    layout_nav_panel(198, 112);
    EGUI_TEST_ASSERT_TRUE(get_item_center(1, &x, &y));

    egui_view_nav_panel_set_locked_mode(EGUI_VIEW_OF(&test_nav_panel), 1);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_nav_panel_get_current_index(EGUI_VIEW_OF(&test_nav_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_selection_count);

    egui_view_nav_panel_set_locked_mode(EGUI_VIEW_OF(&test_nav_panel), 0);
    egui_view_set_enable(EGUI_VIEW_OF(&test_nav_panel), 0);
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, x, y));
    EGUI_TEST_ASSERT_FALSE(send_touch(EGUI_MOTION_EVENT_ACTION_UP, x, y));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_nav_panel_get_current_index(EGUI_VIEW_OF(&test_nav_panel)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_selection_count);
}

void test_nav_panel_run(void)
{
    EGUI_TEST_SUITE_BEGIN(nav_panel);
    EGUI_TEST_RUN(test_nav_panel_set_items_and_current_index_clamp);
    EGUI_TEST_RUN(test_nav_panel_font_palette_and_helper_functions);
    EGUI_TEST_RUN(test_nav_panel_metrics_and_hit_testing);
    EGUI_TEST_RUN(test_nav_panel_touch_selects_item_and_cancel_resets_pressed_state);
    EGUI_TEST_RUN(test_nav_panel_locked_or_disabled_ignores_touch);
    EGUI_TEST_SUITE_END();
}
