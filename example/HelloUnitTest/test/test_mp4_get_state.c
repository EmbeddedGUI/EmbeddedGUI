#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_mp4_get_state.h"

static egui_view_mp4_t s_mp4;
static egui_image_t s_frame0;
static egui_image_t s_frame1;
static const egui_image_t *s_frames[2];
static int s_callback_count;

static void setup(void)
{
    memset(&s_mp4, 0, sizeof(s_mp4));
    memset(&s_frame0, 0, sizeof(s_frame0));
    memset(&s_frame1, 0, sizeof(s_frame1));
    s_frames[0] = &s_frame0;
    s_frames[1] = &s_frame1;
    s_callback_count = 0;
    egui_view_mp4_init(EGUI_VIEW_OF(&s_mp4), uicode_get_core());
}

static void on_mp4_end(egui_view_mp4_t *mp4, int is_end)
{
    EGUI_UNUSED(mp4);
    if (is_end)
    {
        s_callback_count++;
    }
}

static void test_mp4_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_ALIGN_CENTER, (int)egui_view_mp4_get_align_type(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_NULL(egui_view_mp4_get_callback(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_NULL(egui_view_mp4_get_mp4_image_list(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mp4_get_mp4_image_count(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mp4_get_mp4_image_index(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mp4_get_frame_interval_ms(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mp4_is_playing(EGUI_VIEW_OF(&s_mp4)));
}

static void test_mp4_get_state_after_setters(void)
{
    setup();
    egui_view_mp4_set_align_type(EGUI_VIEW_OF(&s_mp4), EGUI_ALIGN_RIGHT | EGUI_ALIGN_BOTTOM);
    egui_view_mp4_set_callback(EGUI_VIEW_OF(&s_mp4), on_mp4_end);
    egui_view_mp4_set_mp4_image_list(EGUI_VIEW_OF(&s_mp4), s_frames, 2);

    EGUI_TEST_ASSERT_EQUAL_INT((int)(EGUI_ALIGN_RIGHT | EGUI_ALIGN_BOTTOM), (int)egui_view_mp4_get_align_type(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_TRUE(egui_view_mp4_get_callback(EGUI_VIEW_OF(&s_mp4)) == on_mp4_end);
    EGUI_TEST_ASSERT_TRUE(egui_view_mp4_get_mp4_image_list(EGUI_VIEW_OF(&s_mp4)) == s_frames);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_mp4_get_mp4_image_count(EGUI_VIEW_OF(&s_mp4)));
}

static void test_mp4_get_state_start_stop(void)
{
    setup();
    s_mp4.mp4_image_index = 1;
    egui_view_mp4_set_mp4_image_list(EGUI_VIEW_OF(&s_mp4), s_frames, 2);
    egui_view_mp4_start_work(EGUI_VIEW_OF(&s_mp4), 75);

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_mp4_is_playing(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mp4_get_mp4_image_index(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_EQUAL_INT(75, (int)egui_view_mp4_get_frame_interval_ms(EGUI_VIEW_OF(&s_mp4)));

    egui_view_mp4_stop_work(EGUI_VIEW_OF(&s_mp4));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mp4_is_playing(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_EQUAL_INT(75, (int)egui_view_mp4_get_frame_interval_ms(EGUI_VIEW_OF(&s_mp4)));
}

static void test_mp4_get_state_clear_optional(void)
{
    setup();
    egui_view_mp4_set_callback(EGUI_VIEW_OF(&s_mp4), on_mp4_end);
    egui_view_mp4_set_mp4_image_list(EGUI_VIEW_OF(&s_mp4), s_frames, 2);

    egui_view_mp4_set_callback(EGUI_VIEW_OF(&s_mp4), NULL);
    egui_view_mp4_set_mp4_image_list(EGUI_VIEW_OF(&s_mp4), NULL, 0);

    EGUI_TEST_ASSERT_NULL(egui_view_mp4_get_callback(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_NULL(egui_view_mp4_get_mp4_image_list(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mp4_get_mp4_image_count(EGUI_VIEW_OF(&s_mp4)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_callback_count);
}

static void test_mp4_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mp4_get_align_type(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_mp4_get_callback(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_mp4_get_mp4_image_list(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mp4_get_mp4_image_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mp4_get_mp4_image_index(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mp4_get_frame_interval_ms(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_mp4_is_playing(NULL));
}

void test_mp4_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(mp4_get_state);

    EGUI_TEST_RUN(test_mp4_get_state_defaults);
    EGUI_TEST_RUN(test_mp4_get_state_after_setters);
    EGUI_TEST_RUN(test_mp4_get_state_start_stop);
    EGUI_TEST_RUN(test_mp4_get_state_clear_optional);
    EGUI_TEST_RUN(test_mp4_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
