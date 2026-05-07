#include <string.h>

#include "egui.h"
#include "egui_touch.h"
#include "test/egui_test.h"
#include "test_input.h"
#include "uicode_disp0.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#define TEST_INPUT_MAX_EVENTS  12
#define TEST_INPUT_MAX_SAMPLES 6

typedef struct test_input_motion_log
{
    uint8_t type;
    uint8_t pointer_count;
    egui_location_t locations[EGUI_TOUCH_DRIVER_MAX_POINTS];
} test_input_motion_log_t;

static egui_view_t test_input_probe_view;
static egui_view_api_t test_input_probe_api;
static egui_touch_driver_t test_input_touch_driver;
static egui_touch_driver_ops_t test_input_touch_ops;
static egui_touch_driver_data_t test_input_samples[TEST_INPUT_MAX_SAMPLES];
static uint8_t test_input_sample_count;
static uint8_t test_input_sample_index;
static test_input_motion_log_t test_input_events[TEST_INPUT_MAX_EVENTS];
static uint8_t test_input_event_count;

typedef struct test_input_touch_snapshot
{
    egui_touch_driver_t *driver;
    egui_touch_driver_t hal_bridge_driver;
    egui_touch_driver_ops_t hal_bridge_ops;
    void *hal_bridge_hal_driver;
    uint8_t hal_touch_last_pressed;
} test_input_touch_snapshot_t;

static egui_hal_touch_driver_t test_input_hal_driver;
static egui_hal_touch_data_t test_input_hal_sample;
static int test_input_hal_reset_result;
static int test_input_hal_init_result;
static int test_input_hal_read_result;
static uint8_t test_input_hal_int_level;
static uint8_t test_input_hal_write_junk_on_error;
static uint8_t test_input_hal_reset_count;
static uint8_t test_input_hal_init_count;
static uint8_t test_input_hal_read_count;
static uint8_t test_input_hal_get_int_count;

static egui_core_t *test_input_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void test_input_touch_snapshot_save(egui_core_t *core, test_input_touch_snapshot_t *snapshot)
{
    EGUI_ASSERT(core != NULL);
    EGUI_ASSERT(snapshot != NULL);

    snapshot->driver = core->touch.driver;
    snapshot->hal_bridge_driver = core->touch.hal_bridge_driver;
    snapshot->hal_bridge_ops = core->touch.hal_bridge_ops;
    snapshot->hal_bridge_hal_driver = core->touch.hal_bridge_hal_driver;
    snapshot->hal_touch_last_pressed = core->touch.hal_touch_last_pressed;
}

static void test_input_touch_snapshot_restore(egui_core_t *core, const test_input_touch_snapshot_t *snapshot)
{
    EGUI_ASSERT(core != NULL);
    EGUI_ASSERT(snapshot != NULL);

    core->touch.driver = snapshot->driver;
    core->touch.hal_bridge_driver = snapshot->hal_bridge_driver;
    core->touch.hal_bridge_ops = snapshot->hal_bridge_ops;
    core->touch.hal_bridge_hal_driver = snapshot->hal_bridge_hal_driver;
    core->touch.hal_touch_last_pressed = snapshot->hal_touch_last_pressed;
    egui_input_init(core);
}

static int test_input_hal_reset(egui_hal_touch_driver_t *self)
{
    EGUI_UNUSED(self);
    test_input_hal_reset_count++;
    return test_input_hal_reset_result;
}

static int test_input_hal_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    test_input_hal_init_count++;
    if (self == NULL || config == NULL)
    {
        return -1;
    }

    memcpy(&self->config, config, sizeof(*config));
    return test_input_hal_init_result;
}

static int test_input_hal_read(egui_hal_touch_driver_t *self, egui_core_t *core, egui_hal_touch_data_t *data)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(core);
    test_input_hal_read_count++;
    if (data == NULL)
    {
        return -1;
    }

    memset(data, 0, sizeof(*data));
    if (test_input_hal_read_result != 0)
    {
        if (test_input_hal_write_junk_on_error)
        {
            memset(data, 0x7F, sizeof(*data));
            data->point_count = 0xFF;
        }
        return test_input_hal_read_result;
    }

    *data = test_input_hal_sample;
    return 0;
}

static uint8_t test_input_hal_get_int(void)
{
    test_input_hal_get_int_count++;
    return test_input_hal_int_level;
}

static void test_input_hal_setup(uint8_t use_int)
{
    memset(&test_input_hal_driver, 0, sizeof(test_input_hal_driver));
    memset(&test_input_hal_sample, 0, sizeof(test_input_hal_sample));
    test_input_hal_reset_result = 0;
    test_input_hal_init_result = 0;
    test_input_hal_read_result = 0;
    test_input_hal_int_level = 1;
    test_input_hal_write_junk_on_error = 0;
    test_input_hal_reset_count = 0;
    test_input_hal_init_count = 0;
    test_input_hal_read_count = 0;
    test_input_hal_get_int_count = 0;

    test_input_hal_driver.name = "TEST_HAL_TOUCH";
    test_input_hal_driver.max_points = EGUI_HAL_TOUCH_MAX_POINTS;
    test_input_hal_driver.reset = test_input_hal_reset;
    test_input_hal_driver.init = test_input_hal_init;
    test_input_hal_driver.read = test_input_hal_read;
    test_input_hal_driver.get_int = use_int ? test_input_hal_get_int : NULL;
}

static void test_input_hal_set_sample(uint8_t point_count)
{
    memset(&test_input_hal_sample, 0, sizeof(test_input_hal_sample));
    test_input_hal_sample.point_count = point_count;
    for (uint8_t i = 0; i < EGUI_HAL_TOUCH_MAX_POINTS; i++)
    {
        test_input_hal_sample.points[i].x = (int16_t)(10 + i);
        test_input_hal_sample.points[i].y = (int16_t)(20 + i);
        test_input_hal_sample.points[i].id = (uint8_t)(40 + i);
        test_input_hal_sample.points[i].pressure = (uint8_t)(80 + i);
    }
}

static int test_input_probe_on_touch(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(self);

    if (event != NULL && test_input_event_count < TEST_INPUT_MAX_EVENTS)
    {
        test_input_motion_log_t *log = &test_input_events[test_input_event_count++];
        log->type = event->type;
        log->pointer_count = event->pointer_count;
        for (uint8_t i = 0; i < EGUI_TOUCH_DRIVER_MAX_POINTS; i++)
        {
            log->locations[i].x = event->locations[i].x;
            log->locations[i].y = event->locations[i].y;
        }
    }

    return 1;
}

static int test_input_touch_read(egui_core_t *read_core, egui_touch_driver_data_t *data)
{
    uint8_t index;

    EGUI_UNUSED(read_core);
    if (data == NULL)
    {
        return -1;
    }

    memset(data, 0, sizeof(*data));
    if (test_input_sample_count == 0)
    {
        return 0;
    }

    index = test_input_sample_index;
    if (index >= test_input_sample_count)
    {
        index = (uint8_t)(test_input_sample_count - 1U);
    }
    else
    {
        test_input_sample_index++;
    }

    *data = test_input_samples[index];
    return 0;
}

static void test_input_set_point_sample_ids(uint8_t index, uint8_t point_count, const int16_t *xs, const int16_t *ys, const uint8_t *ids)
{
    egui_touch_driver_data_t *sample = &test_input_samples[index];

    memset(sample, 0, sizeof(*sample));
    if (point_count > EGUI_TOUCH_DRIVER_MAX_POINTS)
    {
        point_count = EGUI_TOUCH_DRIVER_MAX_POINTS;
    }
    sample->point_count = point_count;
    for (uint8_t i = 0; i < point_count; i++)
    {
        sample->points[i].x = xs[i];
        sample->points[i].y = ys[i];
        sample->points[i].id = ids != NULL ? ids[i] : i;
        sample->points[i].pressure = 1;
    }
}

static void test_input_set_point_sample(uint8_t index, uint8_t point_count, const int16_t *xs, const int16_t *ys)
{
    test_input_set_point_sample_ids(index, point_count, xs, ys, NULL);
}

static void test_input_setup_probe_view(egui_core_t *core)
{
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);
    egui_view_group_t *root = egui_core_get_root_view(core);

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
    egui_view_init(&test_input_probe_view, core);
    egui_view_override_api_on_touch(&test_input_probe_view, &test_input_probe_api, test_input_probe_on_touch);
    egui_view_set_position(&test_input_probe_view, 0, 0);
    egui_view_set_size(&test_input_probe_view, 160, 140);
    egui_core_add_user_root_view(&test_input_probe_view);
    EGUI_VIEW_OF(root)->api->calculate_layout(EGUI_VIEW_OF(root));
}

static void test_input_setup_samples(void)
{
    static const int16_t xs0[] = {20};
    static const int16_t ys0[] = {30};
    static const int16_t xs1[] = {20, 80, 130};
    static const int16_t ys1[] = {30, 90, 40};
    static const int16_t xs2[] = {24, 84, 134};
    static const int16_t ys2[] = {36, 96, 48};
    static const int16_t xs3[] = {26, 86};
    static const int16_t ys3[] = {38, 98};
    static const int16_t xs4[] = {28};
    static const int16_t ys4[] = {42};
    EGUI_TEST_ASSERT_TRUE(EGUI_TOUCH_DRIVER_MAX_POINTS >= 3);

    memset(test_input_events, 0, sizeof(test_input_events));
    memset(test_input_samples, 0, sizeof(test_input_samples));
    test_input_event_count = 0;
    test_input_sample_index = 0;
    test_input_sample_count = 6;

    test_input_set_point_sample(0, 1, xs0, ys0);
    test_input_set_point_sample(1, 3, xs1, ys1);
    test_input_set_point_sample(2, 3, xs2, ys2);
    test_input_set_point_sample(3, 2, xs3, ys3);
    test_input_set_point_sample(4, 1, xs4, ys4);
    test_input_set_point_sample(5, 0, NULL, NULL);
}

static void test_input_assert_event(uint8_t index, uint8_t type, uint8_t pointer_count, const int16_t *xs, const int16_t *ys)
{
    EGUI_TEST_ASSERT_TRUE(index < test_input_event_count);
    EGUI_TEST_ASSERT_EQUAL_INT(type, test_input_events[index].type);
    EGUI_TEST_ASSERT_EQUAL_INT(pointer_count, test_input_events[index].pointer_count);
    for (uint8_t i = 0; i < pointer_count; i++)
    {
        EGUI_TEST_ASSERT_EQUAL_INT(xs[i], test_input_events[index].locations[i].x);
        EGUI_TEST_ASSERT_EQUAL_INT(ys[i], test_input_events[index].locations[i].y);
    }
}

static void test_input_assert_event_first_two(uint8_t index, egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2)
{
    EGUI_TEST_ASSERT_TRUE(index < test_input_event_count);
    EGUI_TEST_ASSERT_EQUAL_INT(x1, test_input_events[index].locations[0].x);
    EGUI_TEST_ASSERT_EQUAL_INT(y1, test_input_events[index].locations[0].y);
    EGUI_TEST_ASSERT_EQUAL_INT(x2, test_input_events[index].locations[1].x);
    EGUI_TEST_ASSERT_EQUAL_INT(y2, test_input_events[index].locations[1].y);
}

static void test_input_polling_read_emits_multi_touch_sequence(void)
{
    egui_core_t *core = test_input_get_core();
    egui_touch_driver_t *saved_touch_driver = egui_touch_driver_get(core);
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);

    test_input_setup_probe_view(core);
    test_input_setup_samples();

    memset(&test_input_touch_ops, 0, sizeof(test_input_touch_ops));
    memset(&test_input_touch_driver, 0, sizeof(test_input_touch_driver));
    test_input_touch_ops.read = test_input_touch_read;
    test_input_touch_driver.ops = &test_input_touch_ops;

    egui_touch_driver_register(core, &test_input_touch_driver);
    egui_input_init(core);

    for (uint8_t i = 0; i < test_input_sample_count; i++)
    {
        egui_input_polling_work(core);
    }

    const int16_t xs_down[] = {20};
    const int16_t ys_down[] = {30};
    const int16_t xs_pointer2[] = {20, 80};
    const int16_t ys_pointer2[] = {30, 90};
    const int16_t xs_pointer3[] = {20, 80, 130};
    const int16_t ys_pointer3[] = {30, 90, 40};
    const int16_t xs_move3[] = {24, 84, 134};
    const int16_t ys_move3[] = {36, 96, 48};
    const int16_t xs_up3[] = {26, 86, 134};
    const int16_t ys_up3[] = {38, 98, 48};
    const int16_t xs_up2[] = {28, 86};
    const int16_t ys_up2[] = {42, 98};
    const int16_t xs_up1[] = {28};
    const int16_t ys_up1[] = {42};

    EGUI_TEST_ASSERT_EQUAL_INT(9, test_input_event_count);
    test_input_assert_event(0, EGUI_MOTION_EVENT_ACTION_DOWN, 1, xs_down, ys_down);
    test_input_assert_event(1, EGUI_MOTION_EVENT_ACTION_POINTER_DOWN, 2, xs_pointer2, ys_pointer2);
    test_input_assert_event(2, EGUI_MOTION_EVENT_ACTION_POINTER_DOWN, 3, xs_pointer3, ys_pointer3);
    test_input_assert_event(3, EGUI_MOTION_EVENT_ACTION_MOVE, 3, xs_move3, ys_move3);
    test_input_assert_event(4, EGUI_MOTION_EVENT_ACTION_POINTER_UP, 3, xs_up3, ys_up3);
    test_input_assert_event(5, EGUI_MOTION_EVENT_ACTION_MOVE, 2, xs_up3, ys_up3);
    test_input_assert_event(6, EGUI_MOTION_EVENT_ACTION_POINTER_UP, 2, xs_up2, ys_up2);
    test_input_assert_event(7, EGUI_MOTION_EVENT_ACTION_MOVE, 1, xs_up1, ys_up1);
    test_input_assert_event(8, EGUI_MOTION_EVENT_ACTION_UP, 1, xs_up1, ys_up1);
    test_input_assert_event_first_two(2, 20, 30, 80, 90);

    egui_touch_driver_register(core, saved_touch_driver);
    egui_input_init(core);
    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
}

static void test_input_polling_read_infers_simultaneous_releases(void)
{
    egui_core_t *core = test_input_get_core();
    egui_touch_driver_t *saved_touch_driver = egui_touch_driver_get(core);
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);
    static const int16_t xs0[] = {10, 40, 70};
    static const int16_t ys0[] = {12, 14, 16};
    static const uint8_t ids0[] = {10, 11, 12};
    static const int16_t xs1[] = {75};
    static const int16_t ys1[] = {18};
    static const uint8_t ids1[] = {12};
    const int16_t xs_down[] = {10};
    const int16_t ys_down[] = {12};
    const int16_t xs_pointer2[] = {10, 40};
    const int16_t ys_pointer2[] = {12, 14};
    const int16_t xs_pointer3[] = {10, 40, 70};
    const int16_t ys_pointer3[] = {12, 14, 16};
    const int16_t xs_release3[] = {10, 75, 40};
    const int16_t ys_release3[] = {12, 18, 14};
    const int16_t xs_release2[] = {75, 10};
    const int16_t ys_release2[] = {18, 12};
    const int16_t xs_move1[] = {75};
    const int16_t ys_move1[] = {18};

    test_input_setup_probe_view(core);

    memset(test_input_events, 0, sizeof(test_input_events));
    memset(test_input_samples, 0, sizeof(test_input_samples));
    test_input_event_count = 0;
    test_input_sample_index = 0;
    test_input_sample_count = 3;
    test_input_set_point_sample_ids(0, 3, xs0, ys0, ids0);
    test_input_set_point_sample_ids(1, 1, xs1, ys1, ids1);
    test_input_set_point_sample(2, 0, NULL, NULL);

    memset(&test_input_touch_ops, 0, sizeof(test_input_touch_ops));
    memset(&test_input_touch_driver, 0, sizeof(test_input_touch_driver));
    test_input_touch_ops.read = test_input_touch_read;
    test_input_touch_driver.ops = &test_input_touch_ops;

    egui_touch_driver_register(core, &test_input_touch_driver);
    egui_input_init(core);

    for (uint8_t i = 0; i < test_input_sample_count; i++)
    {
        egui_input_polling_work(core);
    }

    EGUI_TEST_ASSERT_EQUAL_INT(7, test_input_event_count);
    test_input_assert_event(0, EGUI_MOTION_EVENT_ACTION_DOWN, 1, xs_down, ys_down);
    test_input_assert_event(1, EGUI_MOTION_EVENT_ACTION_POINTER_DOWN, 2, xs_pointer2, ys_pointer2);
    test_input_assert_event(2, EGUI_MOTION_EVENT_ACTION_POINTER_DOWN, 3, xs_pointer3, ys_pointer3);
    test_input_assert_event(3, EGUI_MOTION_EVENT_ACTION_POINTER_UP, 3, xs_release3, ys_release3);
    test_input_assert_event(4, EGUI_MOTION_EVENT_ACTION_POINTER_UP, 2, xs_release2, ys_release2);
    test_input_assert_event(5, EGUI_MOTION_EVENT_ACTION_MOVE, 1, xs_move1, ys_move1);
    test_input_assert_event(6, EGUI_MOTION_EVENT_ACTION_UP, 1, xs_move1, ys_move1);

    egui_touch_driver_register(core, saved_touch_driver);
    egui_input_init(core);
    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
}

static void test_input_hal_touch_register_rejects_bad_driver_states(void)
{
    egui_core_t *core = test_input_get_core();
    test_input_touch_snapshot_t snapshot;
    egui_hal_touch_config_t config = {
            .width = 160,
            .height = 140,
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
    };

    test_input_touch_snapshot_save(core, &snapshot);

    test_input_hal_setup(0);
    egui_hal_touch_register(NULL, &test_input_hal_driver, &config);
    egui_hal_touch_register(core, NULL, &config);
    egui_hal_touch_register(core, &test_input_hal_driver, NULL);
    EGUI_TEST_ASSERT_TRUE(snapshot.driver == egui_touch_driver_get(core));
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_input_hal_reset_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_input_hal_init_count);

    test_input_hal_setup(0);
    test_input_hal_reset_result = -1;
    egui_hal_touch_register(core, &test_input_hal_driver, &config);
    EGUI_TEST_ASSERT_TRUE(snapshot.driver == egui_touch_driver_get(core));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_input_hal_reset_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_input_hal_init_count);

    test_input_hal_setup(0);
    test_input_hal_init_result = -1;
    egui_hal_touch_register(core, &test_input_hal_driver, &config);
    EGUI_TEST_ASSERT_TRUE(snapshot.driver == egui_touch_driver_get(core));
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_input_hal_reset_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_input_hal_init_count);

    test_input_touch_snapshot_restore(core, &snapshot);
}

static void test_input_hal_touch_read_error_isolated_from_input_state(void)
{
    egui_core_t *core = test_input_get_core();
    test_input_touch_snapshot_t snapshot;
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);
    egui_hal_touch_config_t config = {
            .width = 160,
            .height = 140,
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
    };
    egui_touch_driver_data_t direct_data;
    const int16_t xs_down[] = {10};
    const int16_t ys_down[] = {20};

    test_input_touch_snapshot_save(core, &snapshot);
    test_input_setup_probe_view(core);
    memset(test_input_events, 0, sizeof(test_input_events));
    test_input_event_count = 0;

    test_input_hal_setup(0);
    test_input_hal_set_sample(1);
    egui_hal_touch_register(core, &test_input_hal_driver, &config);
    egui_input_init(core);

    egui_input_polling_work(core);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_input_event_count);
    test_input_assert_event(0, EGUI_MOTION_EVENT_ACTION_DOWN, 1, xs_down, ys_down);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core->touch.prev_point_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core->touch.hal_touch_last_pressed);

    test_input_hal_read_result = -7;
    test_input_hal_write_junk_on_error = 1;
    memset(&direct_data, 0x5A, sizeof(direct_data));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_touch_driver_get(core)->ops->read(core, &direct_data));
    EGUI_TEST_ASSERT_EQUAL_INT(0, direct_data.point_count);
    for (uint8_t i = 0; i < EGUI_TOUCH_DRIVER_MAX_POINTS; i++)
    {
        EGUI_TEST_ASSERT_EQUAL_INT(0, direct_data.points[i].x);
        EGUI_TEST_ASSERT_EQUAL_INT(0, direct_data.points[i].y);
        EGUI_TEST_ASSERT_EQUAL_INT(0, direct_data.points[i].id);
        EGUI_TEST_ASSERT_EQUAL_INT(0, direct_data.points[i].pressure);
    }
    EGUI_TEST_ASSERT_EQUAL_INT(1, core->touch.prev_point_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core->touch.hal_touch_last_pressed);

    egui_input_polling_work(core);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_input_event_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core->touch.prev_point_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, core->touch.hal_touch_last_pressed);

    test_input_hal_read_result = 0;
    test_input_hal_write_junk_on_error = 0;
    test_input_hal_set_sample(0);
    egui_input_polling_work(core);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_input_event_count);
    test_input_assert_event(1, EGUI_MOTION_EVENT_ACTION_UP, 1, xs_down, ys_down);
    EGUI_TEST_ASSERT_EQUAL_INT(0, core->touch.prev_point_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, core->touch.hal_touch_last_pressed);

    test_input_touch_snapshot_restore(core, &snapshot);
    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
}

static void test_input_hal_touch_read_int_idle_skips_driver_read(void)
{
    egui_core_t *core = test_input_get_core();
    test_input_touch_snapshot_t snapshot;
    egui_hal_touch_config_t config = {
            .width = 160,
            .height = 140,
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
    };
    egui_touch_driver_data_t data;

    test_input_touch_snapshot_save(core, &snapshot);

    test_input_hal_setup(1);
    test_input_hal_int_level = 0;
    test_input_hal_set_sample(1);
    egui_hal_touch_register(core, &test_input_hal_driver, &config);
    egui_input_init(core);

    memset(&data, 0x5A, sizeof(data));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_touch_driver_get(core)->ops->read(core, &data));
    EGUI_TEST_ASSERT_EQUAL_INT(0, data.point_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_input_hal_get_int_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, test_input_hal_read_count);

    core->touch.hal_touch_last_pressed = 1;
    memset(&data, 0x5A, sizeof(data));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_touch_driver_get(core)->ops->read(core, &data));
    EGUI_TEST_ASSERT_EQUAL_INT(1, data.point_count);
    EGUI_TEST_ASSERT_EQUAL_INT(2, test_input_hal_get_int_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, test_input_hal_read_count);

    test_input_touch_snapshot_restore(core, &snapshot);
}

static void test_input_hal_touch_read_clamps_oversized_sample(void)
{
    egui_core_t *core = test_input_get_core();
    test_input_touch_snapshot_t snapshot;
    egui_hal_touch_config_t config = {
            .width = 200,
            .height = 100,
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 0,
    };
    egui_touch_driver_data_t data;
    uint8_t expected_count = EGUI_TOUCH_DRIVER_MAX_POINTS;

    if (expected_count > EGUI_HAL_TOUCH_MAX_POINTS)
    {
        expected_count = EGUI_HAL_TOUCH_MAX_POINTS;
    }

    test_input_touch_snapshot_save(core, &snapshot);

    test_input_hal_setup(0);
    test_input_hal_set_sample(0xFF);
    egui_hal_touch_register(core, &test_input_hal_driver, &config);
    egui_input_init(core);

    memset(&data, 0x5A, sizeof(data));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_touch_driver_get(core)->ops->read(core, &data));
    EGUI_TEST_ASSERT_EQUAL_INT(expected_count, data.point_count);
    for (uint8_t i = 0; i < expected_count; i++)
    {
        EGUI_TEST_ASSERT_EQUAL_INT(179 - i, data.points[i].x);
        EGUI_TEST_ASSERT_EQUAL_INT(10 + i, data.points[i].y);
        EGUI_TEST_ASSERT_EQUAL_INT(40 + i, data.points[i].id);
        EGUI_TEST_ASSERT_EQUAL_INT(80 + i, data.points[i].pressure);
    }

    test_input_touch_snapshot_restore(core, &snapshot);
}
#endif

void test_input_run(void)
{
    EGUI_TEST_SUITE_BEGIN(input);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    EGUI_TEST_RUN(test_input_polling_read_emits_multi_touch_sequence);
    EGUI_TEST_RUN(test_input_polling_read_infers_simultaneous_releases);
    EGUI_TEST_RUN(test_input_hal_touch_register_rejects_bad_driver_states);
    EGUI_TEST_RUN(test_input_hal_touch_read_error_isolated_from_input_state);
    EGUI_TEST_RUN(test_input_hal_touch_read_int_idle_skips_driver_read);
    EGUI_TEST_RUN(test_input_hal_touch_read_clamps_oversized_sample);
#endif
    EGUI_TEST_SUITE_END();
}
