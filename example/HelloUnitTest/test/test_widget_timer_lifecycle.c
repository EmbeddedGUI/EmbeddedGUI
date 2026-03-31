#include "egui.h"
#include "test/egui_test.h"
#include "test_widget_timer_lifecycle.h"

static egui_view_spinner_t test_spinner;
static egui_view_led_t test_led;
static egui_view_heart_rate_t test_heart_rate;
static egui_view_mp4_t test_mp4;

static void test_widget_timer_lifecycle_spinner_respects_attach(void)
{
    egui_view_spinner_init(EGUI_VIEW_OF(&test_spinner));
    egui_view_spinner_start(EGUI_VIEW_OF(&test_spinner));

    EGUI_TEST_ASSERT_TRUE(test_spinner.is_spinning);
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_spinner.spin_timer));

    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&test_spinner));
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&test_spinner.spin_timer));

    egui_view_dispatch_detach_from_window(EGUI_VIEW_OF(&test_spinner));
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_spinner.spin_timer));
    EGUI_TEST_ASSERT_TRUE(test_spinner.is_spinning);
}

static void test_widget_timer_lifecycle_led_respects_attach(void)
{
    egui_view_led_init(EGUI_VIEW_OF(&test_led));
    egui_view_led_set_blink(EGUI_VIEW_OF(&test_led), 120);

    EGUI_TEST_ASSERT_TRUE(test_led.is_blinking);
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_led.blink_timer));

    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&test_led));
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&test_led.blink_timer));

    egui_view_dispatch_detach_from_window(EGUI_VIEW_OF(&test_led));
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_led.blink_timer));
    EGUI_TEST_ASSERT_TRUE(test_led.is_blinking);
}

static void test_widget_timer_lifecycle_heart_rate_respects_attach(void)
{
    egui_view_heart_rate_init(EGUI_VIEW_OF(&test_heart_rate));
    egui_view_heart_rate_set_bpm(EGUI_VIEW_OF(&test_heart_rate), 72);
    egui_view_heart_rate_set_animate(EGUI_VIEW_OF(&test_heart_rate), 1);

    EGUI_TEST_ASSERT_TRUE(test_heart_rate.animate);
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_heart_rate.anim_timer));

    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&test_heart_rate));
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&test_heart_rate.anim_timer));

    egui_view_dispatch_detach_from_window(EGUI_VIEW_OF(&test_heart_rate));
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_heart_rate.anim_timer));
    EGUI_TEST_ASSERT_TRUE(test_heart_rate.animate);
}

static void test_widget_timer_lifecycle_mp4_respects_attach(void)
{
    static const egui_image_t *dummy_frames[] = {
            (const egui_image_t *)1,
            (const egui_image_t *)2,
    };

    egui_view_mp4_init(EGUI_VIEW_OF(&test_mp4));
    egui_view_mp4_set_mp4_image_list(EGUI_VIEW_OF(&test_mp4), dummy_frames, 2);
    egui_view_mp4_start_work(EGUI_VIEW_OF(&test_mp4), 100);

    EGUI_TEST_ASSERT_TRUE(test_mp4.is_playing);
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_mp4.anim_timer));

    egui_view_dispatch_attach_to_window(EGUI_VIEW_OF(&test_mp4));
    EGUI_TEST_ASSERT_TRUE(egui_timer_check_timer_start(&test_mp4.anim_timer));

    egui_view_dispatch_detach_from_window(EGUI_VIEW_OF(&test_mp4));
    EGUI_TEST_ASSERT_FALSE(egui_timer_check_timer_start(&test_mp4.anim_timer));
    EGUI_TEST_ASSERT_TRUE(test_mp4.is_playing);
}

void test_widget_timer_lifecycle_run(void)
{
    EGUI_TEST_SUITE_BEGIN(widget_timer_lifecycle);
    EGUI_TEST_RUN(test_widget_timer_lifecycle_spinner_respects_attach);
    EGUI_TEST_RUN(test_widget_timer_lifecycle_led_respects_attach);
    EGUI_TEST_RUN(test_widget_timer_lifecycle_heart_rate_respects_attach);
    EGUI_TEST_RUN(test_widget_timer_lifecycle_mp4_respects_attach);
    EGUI_TEST_SUITE_END();
}
