#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "background/egui_background_color.h"
#include "core/egui_core_internal.h"
#include "style/egui_theme.h"
#include "test/egui_test.h"
#include "test_view.h"

static egui_view_t test_view;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int g_view_click_count;
#endif
static int g_test_setup_driver_init_count;
static int g_test_setup_driver_rotation_count;
static int g_test_setup_driver_brightness_count;
static int g_test_setup_touch_register_count;
static int g_test_setup_uicode_init_count;
static egui_core_t *g_test_setup_driver_init_core;
static egui_core_t *g_test_setup_touch_register_core;
static egui_core_t *g_test_setup_uicode_init_core;
static egui_display_rotation_t g_test_setup_driver_rotation;
static uint8_t g_test_setup_driver_brightness;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_view_bg_normal_param, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(s_view_bg_pressed_param, EGUI_COLOR_RED, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(s_view_bg_params, &s_view_bg_normal_param, &s_view_bg_pressed_param, NULL);
static egui_background_color_t s_view_pressed_background;
static uint8_t s_view_pressed_background_ready;

static egui_core_t *test_view_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void test_view_click_cb(egui_view_t *self)
{
    EGUI_UNUSED(self);
    g_view_click_count++;
}

static void test_view_layout_rect(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    region.location.x = x;
    region.location.y = y;
    region.size.width = width;
    region.size.height = height;
    egui_view_layout(&test_view, &region);
    egui_region_copy(&test_view.region_screen, &region);
}

static int test_view_send_touch(uint8_t type, egui_dim_t x, egui_dim_t y)
{
    egui_motion_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.location.x = x;
    event.location.y = y;
    return test_view.api->dispatch_touch_event(&test_view, &event);
}
#endif

static void test_view_timer_noop_cb(egui_timer_t *timer)
{
    EGUI_UNUSED(timer);
}

static void test_setup_display_driver_init(egui_core_t *core)
{
    g_test_setup_driver_init_count++;
    g_test_setup_driver_init_core = core;
}

static void test_setup_display_driver_set_rotation(egui_core_t *core, egui_display_rotation_t rotation)
{
    EGUI_UNUSED(core);
    g_test_setup_driver_rotation_count++;
    g_test_setup_driver_rotation = rotation;
}

static void test_setup_display_driver_set_brightness(egui_core_t *core, uint8_t level)
{
    EGUI_UNUSED(core);
    g_test_setup_driver_brightness_count++;
    g_test_setup_driver_brightness = level;
}

static void test_setup_display_driver_draw_area(egui_core_t *core, int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(w);
    EGUI_UNUSED(h);
    EGUI_UNUSED(data);
}

static void test_setup_display_driver_flush(egui_core_t *core)
{
    EGUI_UNUSED(core);
}

static void test_setup_display_driver_set_power(egui_core_t *core, uint8_t on)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(on);
}

static void test_setup_touch_register(egui_core_t *core)
{
    g_test_setup_touch_register_count++;
    g_test_setup_touch_register_core = core;
}

static void test_setup_uicode_init(egui_core_t *core)
{
    g_test_setup_uicode_init_count++;
    g_test_setup_uicode_init_core = core;
}

static void test_setup_display_reset_state(void)
{
    g_test_setup_driver_init_count = 0;
    g_test_setup_driver_rotation_count = 0;
    g_test_setup_driver_brightness_count = 0;
    g_test_setup_touch_register_count = 0;
    g_test_setup_uicode_init_count = 0;
    g_test_setup_driver_init_core = NULL;
    g_test_setup_touch_register_core = NULL;
    g_test_setup_uicode_init_core = NULL;
    g_test_setup_driver_rotation = EGUI_DISPLAY_ROTATION_0;
    g_test_setup_driver_brightness = 0;
}

static void test_view_init_defaults(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.core = core;
    EGUI_TEST_ASSERT_TRUE(test_view.is_enable == 1);
    EGUI_TEST_ASSERT_TRUE(test_view.is_visible == 1);
    EGUI_TEST_ASSERT_TRUE(test_view.is_gone == 0);
    EGUI_TEST_ASSERT_TRUE(test_view.is_pressed == 0);
    EGUI_TEST_ASSERT_TRUE(test_view.is_clickable == 0);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, test_view.alpha);
    EGUI_TEST_ASSERT_NULL(test_view.parent);
    EGUI_TEST_ASSERT_NULL(test_view.background);
}

static void test_view_set_position(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.core = core;
    egui_view_set_position(&test_view, 10, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(10, test_view.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(20, test_view.region.location.y);
}

static void test_view_set_size(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.core = core;
    egui_view_set_size(&test_view, 100, 200);
    EGUI_TEST_ASSERT_EQUAL_INT(100, test_view.region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(200, test_view.region.size.height);
}

static void test_view_visibility(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.core = core;

    // Default visible
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_visible(&test_view));

    // Set invisible
    egui_view_set_visible(&test_view, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_visible(&test_view));

    // Set visible again
    egui_view_set_visible(&test_view, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_visible(&test_view));
}

static void test_view_gone(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.core = core;

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_gone(&test_view));

    egui_view_set_gone(&test_view, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_gone(&test_view));

    egui_view_set_gone(&test_view, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_gone(&test_view));
}

static void test_view_enable(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.core = core;

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_enable(&test_view));

    egui_view_set_enable(&test_view, 0);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_enable(&test_view));
}

static void test_view_clickable(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.core = core;

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_get_clickable(&test_view));

    egui_view_set_clickable(&test_view, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_get_clickable(&test_view));
}

static void test_view_alpha(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.core = core;

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_100, test_view.alpha);

    egui_view_set_alpha(&test_view, EGUI_ALPHA_50);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_50, test_view.alpha);
}

static void test_view_padding(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.core = core;

    egui_view_set_padding(&test_view, 1, 2, 3, 4);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_view.padding.left);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_view.padding.right);
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_view.padding.top);
    EGUI_TEST_ASSERT_EQUAL_INT(4, test_view.padding.bottom);

    egui_view_set_padding_all(&test_view, 5);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.padding.left);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.padding.right);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.padding.top);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.padding.bottom);
}

static void test_view_margin(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.core = core;

    egui_view_set_margin(&test_view, 1, 2, 3, 4);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_view.margin.left);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_view.margin.right);
    EGUI_TEST_ASSERT_EQUAL_INT(3, test_view.margin.top);
    EGUI_TEST_ASSERT_EQUAL_INT(4, test_view.margin.bottom);

    egui_view_set_margin_all(&test_view, 5);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.margin.left);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.margin.right);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.margin.top);
    EGUI_TEST_ASSERT_EQUAL_INT(5, test_view.margin.bottom);
}

static void ensure_view_pressed_background(void)
{
    if (s_view_pressed_background_ready)
    {
        return;
    }

    egui_background_color_init_with_params((egui_background_t *)&s_view_pressed_background, &s_view_bg_params);
    s_view_pressed_background_ready = 1;
}

static void test_view_set_pressed_same_state_skips_dirty(void)
{
    egui_core_t *core = test_view_get_core();
    egui_region_t region;
    egui_region_t *arr = egui_core_get_region_dirty_arr(core);

    egui_view_init(&test_view, core);
    test_view.core = core;
    egui_region_init(&region, 10, 20, 100, 40);
    egui_region_copy(&test_view.region, &region);
    egui_region_copy(&test_view.region_screen, &region);
    test_view.is_request_layout = 0;

    egui_core_clear_region_dirty(core);
    egui_view_set_pressed(&test_view, false);

    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_view_set_pressed_with_region_respects_background_pressed_state(void)
{
    egui_core_t *core = test_view_get_core();
    egui_region_t region;
    egui_region_t dirty_region;
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr(core);

    egui_view_init(&test_view, core);
    test_view.core = core;
    egui_region_init(&region, 10, 20, 100, 40);
    egui_region_copy(&test_view.region, &region);
    egui_region_copy(&test_view.region_screen, &region);
    test_view.is_request_layout = 0;

    egui_region_init(&dirty_region, 5, 6, 20, 10);

    egui_core_clear_region_dirty(core);
    egui_view_set_pressed_with_region(&test_view, true, &dirty_region);
    egui_region_init(&expected, 15, 26, 20, 10);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));

    egui_core_clear_region_dirty(core);
    egui_view_set_pressed_with_region(&test_view, false, &dirty_region);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));

    ensure_view_pressed_background();
    egui_view_set_background(&test_view, (egui_background_t *)&s_view_pressed_background.base);
    egui_core_clear_region_dirty(core);
    egui_view_set_pressed_with_region(&test_view, true, &dirty_region);
    EGUI_TEST_ASSERT_REGION_EQUAL(&test_view.region_screen, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_core_add_user_root_view_accepts_matching_core_view(void)
{
    egui_core_t *core = test_view_get_core();
    egui_view_group_t parent_view;
    egui_view_t child_view;
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));

    egui_view_group_init(EGUI_VIEW_OF(&parent_view), core);
    egui_view_init(&child_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent_view), &child_view);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&parent_view)) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&child_view) == core);
    EGUI_TEST_ASSERT_NULL(EGUI_VIEW_OF(&parent_view)->parent);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&parent_view));

    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&parent_view)) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&child_view) == core);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&parent_view)->parent == user_root);

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
}

static void test_core_add_root_view_accepts_matching_core_view(void)
{
    egui_core_t *core = test_view_get_core();
    egui_view_t root_child_view;
    egui_view_group_t *root_view = egui_core_get_root_view(core);

    egui_view_init(&root_child_view, core);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&root_child_view) == core);
    EGUI_TEST_ASSERT_NULL(root_child_view.parent);

    egui_core_add_root_view(core, &root_child_view);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&root_child_view) == core);
    EGUI_TEST_ASSERT_TRUE(root_child_view.parent == root_view);

    egui_view_group_remove_child(EGUI_VIEW_OF(root_view), &root_child_view);
}

static void test_core_add_user_root_view_moves_existing_parent_to_target_root(void)
{
    egui_core_t *core = test_view_get_core();
    egui_view_group_t holder_view;
    egui_view_group_t parent_view;
    egui_view_t child_view;
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));

    egui_view_group_init(EGUI_VIEW_OF(&holder_view), core);
    egui_view_group_init(EGUI_VIEW_OF(&parent_view), core);
    egui_view_init(&child_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent_view), &child_view);
    egui_view_group_add_child(EGUI_VIEW_OF(&holder_view), EGUI_VIEW_OF(&parent_view));

    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&parent_view)->parent == &holder_view);
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_child_count(EGUI_VIEW_OF(&holder_view)) == 1);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&parent_view));

    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_child_count(EGUI_VIEW_OF(&holder_view)) == 0);
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_child_count(EGUI_VIEW_OF(user_root)) == 1);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&parent_view)->parent == user_root);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&parent_view)) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&child_view) == core);

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
}

static void test_core_add_root_view_moves_existing_parent_to_target_root(void)
{
    egui_core_t *core = test_view_get_core();
    egui_view_group_t holder_view;
    egui_view_group_t parent_view;
    egui_view_t child_view;
    egui_view_group_t *root_view = egui_core_get_root_view(core);
    int root_child_count_before = egui_view_group_get_child_count(EGUI_VIEW_OF(root_view));

    egui_view_group_init(EGUI_VIEW_OF(&holder_view), core);
    egui_view_group_init(EGUI_VIEW_OF(&parent_view), core);
    egui_view_init(&child_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent_view), &child_view);
    egui_view_group_add_child(EGUI_VIEW_OF(&holder_view), EGUI_VIEW_OF(&parent_view));

    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&parent_view)->parent == &holder_view);
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_child_count(EGUI_VIEW_OF(&holder_view)) == 1);

    egui_core_add_root_view(core, EGUI_VIEW_OF(&parent_view));

    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_child_count(EGUI_VIEW_OF(&holder_view)) == 0);
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_child_count(EGUI_VIEW_OF(root_view)) == root_child_count_before + 1);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&parent_view)->parent == root_view);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&parent_view)) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&child_view) == core);

    egui_view_group_remove_child(EGUI_VIEW_OF(root_view), EGUI_VIEW_OF(&parent_view));
}

static void test_view_add_to_root_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    egui_view_group_t *root_view = egui_core_get_root_view(core);

    egui_view_init(&test_view, core);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&test_view) == core);
    EGUI_TEST_ASSERT_NULL(test_view.parent);

    egui_view_add_to_root(&test_view);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&test_view) == core);
    EGUI_TEST_ASSERT_TRUE(test_view.parent == root_view);

    egui_view_group_remove_child(EGUI_VIEW_OF(root_view), &test_view);
}

static void test_view_get_core_requires_self_bound_core(void)
{
    egui_core_t *core = test_view_get_core();
    egui_view_group_t parent_view;
    egui_view_t child_view;

    egui_view_group_init(EGUI_VIEW_OF(&parent_view), core);
    egui_view_init(&child_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent_view), &child_view);
    child_view.core = NULL;

    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&parent_view)->core == core);
    EGUI_TEST_ASSERT_NULL(child_view.core);

    EGUI_TEST_ASSERT_NULL(egui_view_get_core(&child_view));
    EGUI_TEST_ASSERT_NULL(child_view.core);
    EGUI_TEST_ASSERT_NULL(egui_view_get_canvas(&child_view));
}

static void test_view_remove_from_user_root_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
    egui_view_init(&test_view, core);
    egui_core_add_user_root_view(&test_view);

    EGUI_TEST_ASSERT_TRUE(test_view.parent == user_root);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&test_view) == core);

    egui_view_remove_from_user_root(&test_view);

    EGUI_TEST_ASSERT_NULL(test_view.parent);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&test_view) == core);
}

static void test_view_get_canvas_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_canvas(&test_view) == &core->canvas);
}

static void test_view_dirty_epoch_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    uint32_t expected_dirty_epoch;

    egui_view_init(&test_view, core);
    egui_core_clear_region_dirty(core);
    expected_dirty_epoch = egui_core_get_dirty_epoch(core);

    EGUI_TEST_ASSERT_EQUAL_INT((int)expected_dirty_epoch, (int)egui_view_get_dirty_epoch(&test_view));
}

static void test_view_update_region_dirty_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    egui_region_t dirty_region;
    egui_region_t *arr = egui_core_get_region_dirty_arr(core);

    egui_view_init(&test_view, core);
    egui_region_init(&dirty_region, 5, 6, 20, 10);
    egui_core_clear_region_dirty(core);

    egui_view_update_region_dirty(&test_view, &dirty_region);

    EGUI_TEST_ASSERT_REGION_EQUAL(&dirty_region, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));

    egui_core_clear_region_dirty(core);
}

static void test_view_has_pending_dirty_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    egui_region_t region;
    egui_region_t dirty_region;

    egui_view_init(&test_view, core);
    egui_region_init(&region, 10, 20, 100, 50);
    egui_region_copy(&test_view.region, &region);
    egui_region_copy(&test_view.region_screen, &region);
    test_view.is_request_layout = 0;
    egui_region_init(&dirty_region, 5, 6, 20, 10);
    egui_core_clear_region_dirty(core);

    egui_view_invalidate_region(&test_view, &dirty_region);

    EGUI_TEST_ASSERT_TRUE(egui_view_has_pending_dirty(&test_view));

    egui_core_clear_region_dirty(core);
    EGUI_TEST_ASSERT_FALSE(egui_view_has_pending_dirty(&test_view));
}

static void test_view_has_pending_dirty_requires_self_bound_core(void)
{
    egui_core_t *core = test_view_get_core();
    egui_view_group_t parent_view;
    egui_view_t child_view;
    egui_region_t region;
    egui_region_t dirty_region;
    egui_region_t *arr = egui_core_get_region_dirty_arr(core);

    egui_view_group_init(EGUI_VIEW_OF(&parent_view), core);
    egui_view_init(&child_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent_view), &child_view);
    egui_region_init(&region, 10, 20, 100, 50);
    egui_region_copy(&child_view.region, &region);
    egui_region_copy(&child_view.region_screen, &region);
    child_view.is_request_layout = 0;
    child_view.core = NULL;
    egui_region_init(&dirty_region, 5, 6, 20, 10);
    egui_core_clear_region_dirty(core);

    egui_view_invalidate_region(&child_view, &dirty_region);

    EGUI_TEST_ASSERT_NULL(child_view.core);
    EGUI_TEST_ASSERT_FALSE(egui_view_has_pending_dirty(&child_view));
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));

    EGUI_TEST_ASSERT_FALSE(egui_view_has_pending_dirty(&child_view));
}

static void test_view_invalidate_sub_region_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    egui_region_t region;
    egui_sub_region_t regions[2];
    egui_sub_region_table_t table = {
            .regions = regions,
            .count = 2,
    };
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr(core);

    egui_view_init(&test_view, core);
    egui_region_init(&region, 10, 20, 100, 50);
    egui_region_copy(&test_view.region, &region);
    egui_region_copy(&test_view.region_screen, &region);
    test_view.is_request_layout = 0;
    egui_core_clear_region_dirty(core);

    egui_region_init(&regions[0].region, 0, 0, 10, 10);
    egui_region_init(&regions[1].region, 30, 15, 20, 10);

    egui_view_invalidate_sub_region(&test_view, &table, 1);

    egui_region_init(&expected, 40, 35, 20, 10);
    EGUI_TEST_ASSERT_REGION_EQUAL(&expected, &arr[0]);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));

    egui_core_clear_region_dirty(core);
}

static void test_view_invalidate_sub_region_requires_self_bound_core(void)
{
    egui_core_t *core = test_view_get_core();
    egui_view_group_t parent_view;
    egui_view_t child_view;
    egui_region_t region;
    egui_sub_region_t regions[2];
    egui_sub_region_table_t table = {
            .regions = regions,
            .count = 2,
    };
    egui_region_t expected;
    egui_region_t *arr = egui_core_get_region_dirty_arr(core);

    egui_view_group_init(EGUI_VIEW_OF(&parent_view), core);
    egui_view_init(&child_view, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&parent_view), &child_view);
    egui_region_init(&region, 10, 20, 100, 50);
    egui_region_copy(&child_view.region, &region);
    egui_region_copy(&child_view.region_screen, &region);
    child_view.is_request_layout = 0;
    child_view.core = NULL;
    egui_core_clear_region_dirty(core);

    egui_region_init(&regions[0].region, 0, 0, 10, 10);
    egui_region_init(&regions[1].region, 30, 15, 20, 10);

    egui_view_invalidate_sub_region(&child_view, &table, 1);

    egui_region_init(&expected, 40, 35, 20, 10);
    EGUI_UNUSED(expected);
    EGUI_TEST_ASSERT_NULL(child_view.core);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[0]));
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&arr[1]));
}

static void test_view_set_theme_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    const egui_theme_t *prev_theme = egui_theme_get(core);
    const egui_theme_t *target_theme = (prev_theme == &egui_theme_light) ? &egui_theme_dark : &egui_theme_light;

    egui_view_init(&test_view, core);

    egui_view_set_theme(&test_view, target_theme);

    EGUI_TEST_ASSERT_TRUE(egui_theme_get(core) == target_theme);

    if (prev_theme != NULL)
    {
        egui_theme_set(core, prev_theme);
    }
}

static void test_view_layout_user_root_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);
    egui_view_t carrier_view;
    egui_view_t child_view_1;
    egui_view_t child_view_2;

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));

    egui_view_init(&carrier_view, core);

    egui_view_init(&child_view_1, core);
    egui_view_set_size(&child_view_1, 20, 15);
    egui_view_init(&child_view_2, core);
    egui_view_set_size(&child_view_2, 30, 25);

    egui_core_add_user_root_view(&child_view_1);
    egui_core_add_user_root_view(&child_view_2);

    egui_view_set_position(&child_view_1, 0, 0);
    egui_view_set_position(&child_view_2, 0, 0);

    egui_view_layout_user_root(&carrier_view, EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_TOP_LEFT);

    EGUI_TEST_ASSERT_EQUAL_INT(0, child_view_1.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(0, child_view_1.region.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(0, child_view_2.region.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(child_view_1.region.size.height, child_view_2.region.location.y);

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
}

static void test_view_timer_helpers_use_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    egui_timer_t timer;

    egui_view_init(&test_view, core);
    egui_timer_init_timer(&timer, NULL, test_view_timer_noop_cb);

    EGUI_TEST_ASSERT_FALSE(egui_view_check_timer_start(&test_view, &timer));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_start_timer(&test_view, &timer, 100, 0));
    EGUI_TEST_ASSERT_TRUE(egui_view_check_timer_start(&test_view, &timer));

    egui_view_stop_timer(&test_view, &timer);
    EGUI_TEST_ASSERT_FALSE(egui_view_check_timer_start(&test_view, &timer));
}

static void test_view_pfb_scan_helpers_use_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    uint8_t prev_reverse_x = egui_core_get_pfb_scan_reverse_x(core);
    uint8_t prev_reverse_y = egui_core_get_pfb_scan_reverse_y(core);

    egui_view_init(&test_view, core);
    egui_core_reset_pfb_scan_direction(core);

    egui_view_set_pfb_scan_direction(&test_view, 1, 1);

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_core_get_pfb_scan_reverse_x(core));
    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_core_get_pfb_scan_reverse_y(core));

    egui_view_reset_pfb_scan_direction(&test_view);

    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_core_get_pfb_scan_reverse_x(core));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_core_get_pfb_scan_reverse_y(core));

    egui_core_set_pfb_scan_direction(core, prev_reverse_x, prev_reverse_y);
}

static void test_view_get_activity_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    egui_activity_t activity;
    egui_view_t child_view;

    egui_activity_init(&activity, core);
    egui_view_init(&child_view, core);

    egui_activity_start(&activity, NULL);
    egui_activity_add_view(&activity, &child_view);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&activity.root_view)) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&child_view) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_activity(EGUI_VIEW_OF(&activity.root_view)) == &activity);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_activity(&child_view) == &activity);

    egui_activity_finish(&activity);
    egui_focus_manager_clear_focus(core);
}

static void test_view_get_dialog_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    egui_activity_t activity;
    egui_dialog_t dialog;
    egui_view_t child_view;

    egui_activity_init(&activity, core);
    egui_dialog_init(&dialog, core);
    egui_view_init(&child_view, core);

    egui_activity_start(&activity, NULL);
    egui_dialog_start(&dialog, &activity);
    egui_dialog_add_view(&dialog, &child_view);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&dialog.root_view)) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&dialog.user_root_view)) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&child_view) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_dialog(EGUI_VIEW_OF(&dialog.root_view)) == &dialog);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_dialog(EGUI_VIEW_OF(&dialog.user_root_view)) == &dialog);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_dialog(&child_view) == &dialog);

    egui_dialog_finish(&dialog);
    egui_activity_finish(&activity);
    egui_focus_manager_clear_focus(core);
}

static void test_view_get_toast_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();
    egui_toast_t toast;

    egui_view_init(&test_view, core);

    egui_toast_init(&toast, core);
    egui_toast_set_as_default(&toast);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(&test_view) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_toast(&test_view) == &toast);

    egui_toast_clear_as_default(&toast);
}

static void test_view_velocity_helpers_use_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_FUNCTION_INPUT_VELOCITY_TRACKER
    egui_float_t prev_velocity_x = core->input.velocity_tracker.velocity_x;
    egui_float_t prev_velocity_y = core->input.velocity_tracker.velocity_y;
    egui_float_t target_velocity_x = EGUI_FLOAT_VALUE(0.25f);
    egui_float_t target_velocity_y = EGUI_FLOAT_VALUE(-0.50f);

    core->input.velocity_tracker.velocity_x = target_velocity_x;
    core->input.velocity_tracker.velocity_y = target_velocity_y;

    EGUI_TEST_ASSERT_TRUE(egui_view_get_velocity_x(&test_view) == target_velocity_x);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_velocity_y(&test_view) == target_velocity_y);

    core->input.velocity_tracker.velocity_x = prev_velocity_x;
    core->input.velocity_tracker.velocity_y = prev_velocity_y;
#else
    EGUI_TEST_ASSERT_TRUE(egui_view_get_velocity_x(&test_view) == 0);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_velocity_y(&test_view) == 0);
#endif
}

static void test_init_display_sets_canvas_core(void)
{
    egui_core_t local_core;
    static egui_color_int_t local_pfb[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];
    egui_color_int_t *pfb_bufs[1] = {local_pfb};

    egui_init_display(&local_core, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, pfb_bufs, 1, EGUI_CONFIG_PFB_WIDTH, EGUI_CONFIG_PFB_HEIGHT);

    EGUI_TEST_ASSERT_TRUE(egui_canvas_get_core(&local_core.canvas) == &local_core);
}

static void test_setup_display_registers_driver_platform_and_hooks(void)
{
    egui_core_t local_core;
    static egui_color_int_t local_pfb[32 * 24];
    egui_color_int_t *pfb_bufs[1] = {local_pfb};
    egui_platform_t *registered_platform = egui_platform_get();
    static egui_display_driver_ops_t driver_ops = {
            .init = test_setup_display_driver_init,
            .draw_area = test_setup_display_driver_draw_area,
            .flush = test_setup_display_driver_flush,
            .set_brightness = test_setup_display_driver_set_brightness,
            .set_power = test_setup_display_driver_set_power,
            .set_rotation = test_setup_display_driver_set_rotation,
    };
    static egui_display_driver_t driver = {
            .ops = &driver_ops,
            .physical_width = 320,
            .physical_height = 240,
            .rotation = EGUI_DISPLAY_ROTATION_90,
            .brightness = 123,
    };
    egui_display_setup_t setup = {
            .screen_width = 320,
            .screen_height = 240,
            .pfb_width = 32,
            .pfb_height = 24,
            .pfb_buffers = pfb_bufs,
            .pfb_buffer_count = 1,
            .display_driver = &driver,
            .touch_register = test_setup_touch_register,
            .uicode_init = test_setup_uicode_init,
            .display_id = 2,
    };

    memset(&local_core, 0, sizeof(local_core));
    test_setup_display_reset_state();

    egui_setup_display(&local_core, &setup);

    EGUI_TEST_ASSERT_TRUE(local_core.id == 2);
    EGUI_TEST_ASSERT_TRUE(egui_canvas_get_core(&local_core.canvas) == &local_core);
    EGUI_TEST_ASSERT_TRUE(egui_display_driver_get(&local_core) == &driver);
    EGUI_TEST_ASSERT_TRUE(registered_platform != NULL);
    EGUI_TEST_ASSERT_TRUE(egui_platform_get() == registered_platform);
    EGUI_TEST_ASSERT_EQUAL_INT(320, egui_display_get_width(&local_core));
    EGUI_TEST_ASSERT_EQUAL_INT(240, egui_display_get_height(&local_core));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_test_setup_driver_init_count);
    EGUI_TEST_ASSERT_TRUE(g_test_setup_driver_init_core == &local_core);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_test_setup_driver_rotation_count);
    EGUI_TEST_ASSERT_TRUE(g_test_setup_driver_rotation == EGUI_DISPLAY_ROTATION_90);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_test_setup_driver_brightness_count);
    EGUI_TEST_ASSERT_TRUE(g_test_setup_driver_brightness == 123);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_test_setup_touch_register_count);
    EGUI_TEST_ASSERT_TRUE(g_test_setup_touch_register_core == &local_core);
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_test_setup_uicode_init_count);
    EGUI_TEST_ASSERT_TRUE(g_test_setup_uicode_init_core == &local_core);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void test_view_focus_helpers_bridge_core_focus_manager(void)
{
    egui_core_t *core = test_view_get_core();
    egui_view_t focused_view;

    egui_view_init(&test_view, core);
    egui_view_init(&focused_view, core);
    focused_view.is_focusable = true;

    egui_focus_manager_clear_focus(core);
    egui_focus_manager_set_focus(core, &focused_view);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_focused_view(&test_view) == &focused_view);

    egui_view_clear_focus(&test_view);
    EGUI_TEST_ASSERT_NULL(egui_focus_manager_get_focused_view(core));
}

static void test_view_request_focus_uses_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.is_focusable = true;

    egui_focus_manager_clear_focus(core);

    egui_view_request_focus(&test_view);

    EGUI_TEST_ASSERT_TRUE(egui_focus_manager_get_focused_view(core) == &test_view);
    EGUI_TEST_ASSERT_TRUE(test_view.is_focused);

    egui_focus_manager_clear_focus(core);
}

static void test_view_request_focus_without_bound_core_is_noop(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.core = NULL;
    test_view.is_focusable = true;

    egui_focus_manager_clear_focus(core);
    egui_view_request_focus(&test_view);

    EGUI_TEST_ASSERT_NULL(egui_focus_manager_get_focused_view(core));
    EGUI_TEST_ASSERT_FALSE(test_view.is_focused);
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void test_view_click_requires_release_inside(void)
{
    egui_core_t *core = test_view_get_core();

    egui_view_init(&test_view, core);
    test_view.core = core;
    test_view_layout_rect(10, 20, 100, 40);
    egui_view_set_on_click_listener(&test_view, test_view_click_cb);
    g_view_click_count = 0;

    EGUI_TEST_ASSERT_TRUE(test_view_send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 20, 30));
    EGUI_TEST_ASSERT_TRUE(test_view.is_pressed);

    EGUI_TEST_ASSERT_TRUE(test_view_send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 160, 30));
    EGUI_TEST_ASSERT_FALSE(test_view.is_pressed);

    EGUI_TEST_ASSERT_TRUE(test_view_send_touch(EGUI_MOTION_EVENT_ACTION_UP, 160, 30));
    EGUI_TEST_ASSERT_FALSE(test_view.is_pressed);
    EGUI_TEST_ASSERT_EQUAL_INT(0, g_view_click_count);

    EGUI_TEST_ASSERT_TRUE(test_view_send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, 20, 30));
    EGUI_TEST_ASSERT_TRUE(test_view_send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, 24, 34));
    EGUI_TEST_ASSERT_TRUE(test_view.is_pressed);
    EGUI_TEST_ASSERT_TRUE(test_view_send_touch(EGUI_MOTION_EVENT_ACTION_UP, 24, 34));
    EGUI_TEST_ASSERT_EQUAL_INT(1, g_view_click_count);
}
#endif

void test_view_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view);

    EGUI_TEST_RUN(test_view_init_defaults);
    EGUI_TEST_RUN(test_view_set_position);
    EGUI_TEST_RUN(test_view_set_size);
    EGUI_TEST_RUN(test_view_visibility);
    EGUI_TEST_RUN(test_view_gone);
    EGUI_TEST_RUN(test_view_enable);
    EGUI_TEST_RUN(test_view_clickable);
    EGUI_TEST_RUN(test_view_alpha);
    EGUI_TEST_RUN(test_view_padding);
    EGUI_TEST_RUN(test_view_margin);
    EGUI_TEST_RUN(test_view_set_pressed_same_state_skips_dirty);
    EGUI_TEST_RUN(test_view_set_pressed_with_region_respects_background_pressed_state);
    EGUI_TEST_RUN(test_core_add_user_root_view_accepts_matching_core_view);
    EGUI_TEST_RUN(test_core_add_root_view_accepts_matching_core_view);
    EGUI_TEST_RUN(test_core_add_user_root_view_moves_existing_parent_to_target_root);
    EGUI_TEST_RUN(test_core_add_root_view_moves_existing_parent_to_target_root);
    EGUI_TEST_RUN(test_view_add_to_root_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_get_core_requires_self_bound_core);
    EGUI_TEST_RUN(test_view_remove_from_user_root_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_get_canvas_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_dirty_epoch_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_update_region_dirty_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_has_pending_dirty_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_has_pending_dirty_requires_self_bound_core);
    EGUI_TEST_RUN(test_view_invalidate_sub_region_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_invalidate_sub_region_requires_self_bound_core);
    EGUI_TEST_RUN(test_view_set_theme_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_layout_user_root_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_timer_helpers_use_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_pfb_scan_helpers_use_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_get_activity_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_get_dialog_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_get_toast_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_velocity_helpers_use_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_init_display_sets_canvas_core);
    EGUI_TEST_RUN(test_setup_display_registers_driver_platform_and_hooks);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    EGUI_TEST_RUN(test_view_focus_helpers_bridge_core_focus_manager);
    EGUI_TEST_RUN(test_view_request_focus_uses_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_request_focus_without_bound_core_is_noop);
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    EGUI_TEST_RUN(test_view_click_requires_release_inside);
#endif

    EGUI_TEST_SUITE_END();
}
