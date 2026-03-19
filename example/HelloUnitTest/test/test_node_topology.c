#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_node_topology.h"

#include "../../HelloCustomWidgets/display/node_topology/egui_view_node_topology.h"
#include "../../HelloCustomWidgets/display/node_topology/egui_view_node_topology.c"

static egui_view_node_topology_t test_topology;
static int click_count;

static const egui_view_node_topology_node_t g_nodes_a[] = {
        {80, 10, 9, "Core", 0}, {34, 42, 8, "API", 1}, {126, 42, 8, "Cache", 0}, {28, 74, 8, "DB", 2}, {132, 74, 8, "Queue", 1},
};

static const egui_view_node_topology_link_t g_links_a[] = {
        {0, 1, 1}, {0, 2, 1}, {1, 3, 1}, {2, 4, 1}, {1, 4, 0}, {2, 3, 0},
};

static const egui_view_node_topology_node_t g_nodes_b[] = {
        {80, 12, 9, "Hub", 0}, {24, 44, 8, "Edge", 1}, {136, 44, 8, "Index", 2}, {44, 72, 8, "Bill", 0}, {116, 72, 8, "Jobs", 1},
};

static const egui_view_node_topology_link_t g_links_b[] = {
        {0, 1, 1}, {0, 2, 1}, {1, 3, 1}, {2, 4, 1}, {0, 4, 0}, {1, 2, 0},
};

static const egui_view_node_topology_snapshot_t g_snapshots[] = {
        {"Cluster A", g_nodes_a, g_links_a, 5, 6, 1},
        {"Cluster B", g_nodes_b, g_links_b, 5, 6, 2},
};

static const egui_view_node_topology_snapshot_t g_overflow_snapshots[] = {
        {"A", g_nodes_a, g_links_a, 5, 6, 0},
        {"B", g_nodes_b, g_links_b, 5, 6, 1},
        {"C", g_nodes_a, g_links_a, 5, 6, 2},
        {"D", g_nodes_b, g_links_b, 5, 6, 3},
};

static void on_topology_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    click_count++;
}

static void setup_topology(void)
{
    egui_view_node_topology_init(EGUI_VIEW_OF(&test_topology));
    egui_view_set_size(EGUI_VIEW_OF(&test_topology), 176, 132);
    egui_view_node_topology_set_snapshots(EGUI_VIEW_OF(&test_topology), g_snapshots, 2);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&test_topology), on_topology_click);
    click_count = 0;
}

static void layout_topology(void)
{
    egui_region_t region;

    region.location.x = 10;
    region.location.y = 20;
    region.size.width = 176;
    region.size.height = 132;
    egui_view_layout(EGUI_VIEW_OF(&test_topology), &region);
    egui_region_copy(&EGUI_VIEW_OF(&test_topology)->region_screen, &region);
}

static int send_touch(uint8_t type)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = 64;
    event.location.y = 76;
    return EGUI_VIEW_OF(&test_topology)->api->on_touch_event(EGUI_VIEW_OF(&test_topology), &event);
}

static int send_key(uint8_t key_code)
{
    egui_key_event_t event;
    int handled = 0;

    memset(&event, 0, sizeof(event));
    event.type = EGUI_KEY_EVENT_ACTION_DOWN;
    event.key_code = key_code;
    handled |= EGUI_VIEW_OF(&test_topology)->api->on_key_event(EGUI_VIEW_OF(&test_topology), &event);
    event.type = EGUI_KEY_EVENT_ACTION_UP;
    handled |= EGUI_VIEW_OF(&test_topology)->api->on_key_event(EGUI_VIEW_OF(&test_topology), &event);
    return handled;
}

static void test_node_topology_set_snapshots_clamp_and_reset_current(void)
{
    setup_topology();

    egui_view_node_topology_set_snapshots(EGUI_VIEW_OF(&test_topology), g_overflow_snapshots, 4);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NODE_TOPOLOGY_MAX_SNAPSHOTS, test_topology.snapshot_count);

    test_topology.current_snapshot = 3;
    egui_view_node_topology_set_snapshots(EGUI_VIEW_OF(&test_topology), g_snapshots, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_topology.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_node_topology_get_current_snapshot(EGUI_VIEW_OF(&test_topology)));

    egui_view_node_topology_set_current_snapshot(EGUI_VIEW_OF(&test_topology), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_node_topology_get_current_snapshot(EGUI_VIEW_OF(&test_topology)));

    egui_view_node_topology_set_snapshots(EGUI_VIEW_OF(&test_topology), NULL, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_topology.snapshot_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_node_topology_get_current_snapshot(EGUI_VIEW_OF(&test_topology)));
}

static void test_node_topology_set_current_snapshot_and_focus_update(void)
{
    setup_topology();

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_node_topology_get_current_snapshot(EGUI_VIEW_OF(&test_topology)));
    egui_view_node_topology_set_current_snapshot(EGUI_VIEW_OF(&test_topology), 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_node_topology_get_current_snapshot(EGUI_VIEW_OF(&test_topology)));

    egui_view_node_topology_set_current_snapshot(EGUI_VIEW_OF(&test_topology), 8);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_node_topology_get_current_snapshot(EGUI_VIEW_OF(&test_topology)));

    egui_view_node_topology_set_focus_node(EGUI_VIEW_OF(&test_topology), 4);
    EGUI_TEST_ASSERT_EQUAL_INT(4, test_topology.focus_node);
}

static void test_node_topology_font_modes_and_palette_update(void)
{
    setup_topology();

    egui_view_node_topology_set_font(EGUI_VIEW_OF(&test_topology), NULL);
    EGUI_TEST_ASSERT_TRUE(test_topology.font == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_node_topology_set_show_header(EGUI_VIEW_OF(&test_topology), 2);
    egui_view_node_topology_set_compact_mode(EGUI_VIEW_OF(&test_topology), 3);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_topology.show_header);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_topology.compact_mode);

    egui_view_node_topology_set_show_header(EGUI_VIEW_OF(&test_topology), 0);
    egui_view_node_topology_set_compact_mode(EGUI_VIEW_OF(&test_topology), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_topology.show_header);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_topology.compact_mode);

    egui_view_node_topology_set_palette(EGUI_VIEW_OF(&test_topology), EGUI_COLOR_HEX(0x101112), EGUI_COLOR_HEX(0x202122), EGUI_COLOR_HEX(0x303132),
                                        EGUI_COLOR_HEX(0x404142), EGUI_COLOR_HEX(0x505152));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x101112).full, test_topology.surface_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x202122).full, test_topology.border_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x303132).full, test_topology.text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x404142).full, test_topology.muted_text_color.full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x505152).full, test_topology.active_color.full);
}

static void test_node_topology_internal_helpers_cover_clamp_status_and_mix(void)
{
    egui_color_t sample = EGUI_COLOR_HEX(0x123456);
    egui_color_t mixed = egui_view_node_topology_mix_disabled(sample);

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NODE_TOPOLOGY_MAX_SNAPSHOTS, egui_view_node_topology_clamp_snapshot_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NODE_TOPOLOGY_MAX_NODES, egui_view_node_topology_clamp_node_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_NODE_TOPOLOGY_MAX_LINKS, egui_view_node_topology_clamp_link_count(9));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x34D399).full, egui_view_node_topology_status_color(0).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xF59E0B).full, egui_view_node_topology_status_color(1).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0xFB7185).full, egui_view_node_topology_status_color(2).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_COLOR_HEX(0x34D399).full, egui_view_node_topology_status_color(9).full);
    EGUI_TEST_ASSERT_EQUAL_INT(egui_rgb_mix(sample, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70).full, mixed.full);
}

static void test_node_topology_touch_and_key_click_listener(void)
{
    setup_topology();
    layout_topology();

    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_topology)->is_pressed);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_topology)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(1, click_count);

    EGUI_TEST_ASSERT_TRUE(send_key(EGUI_KEY_CODE_ENTER));
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);

    egui_view_set_enable(EGUI_VIEW_OF(&test_topology), 0);
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_DOWN));
    EGUI_TEST_ASSERT_TRUE(send_touch(EGUI_MOTION_EVENT_ACTION_UP));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_topology)->is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(2, click_count);
    EGUI_TEST_ASSERT_FALSE(send_key(EGUI_KEY_CODE_ENTER));
}

void test_node_topology_run(void)
{
    EGUI_TEST_SUITE_BEGIN(node_topology);
    EGUI_TEST_RUN(test_node_topology_set_snapshots_clamp_and_reset_current);
    EGUI_TEST_RUN(test_node_topology_set_current_snapshot_and_focus_update);
    EGUI_TEST_RUN(test_node_topology_font_modes_and_palette_update);
    EGUI_TEST_RUN(test_node_topology_internal_helpers_cover_clamp_status_and_mix);
    EGUI_TEST_RUN(test_node_topology_touch_and_key_click_listener);
    EGUI_TEST_SUITE_END();
}
