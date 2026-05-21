#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_animated_image_get_state.h"

static egui_view_animated_image_t s_anim_img;
static egui_image_t               s_frame0;
static egui_image_t               s_frame1;
static egui_image_t               s_frame2;
static const egui_image_t        *s_frames[3];

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_anim_img, 0, sizeof(s_anim_img));
    memset(&s_frame0,   0, sizeof(s_frame0));
    memset(&s_frame1,   0, sizeof(s_frame1));
    memset(&s_frame2,   0, sizeof(s_frame2));

    s_frames[0] = &s_frame0;
    s_frames[1] = &s_frame1;
    s_frames[2] = &s_frame2;

    egui_view_animated_image_init(EGUI_VIEW_OF(&s_anim_img), core);
}

static void test_animated_image_get_state_defaults(void)
{
    setup();
    EGUI_TEST_ASSERT_NULL(egui_view_animated_image_get_frames(EGUI_VIEW_OF(&s_anim_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_get_frame_count(EGUI_VIEW_OF(&s_anim_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_get_current_frame(EGUI_VIEW_OF(&s_anim_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_is_playing(EGUI_VIEW_OF(&s_anim_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_animated_image_get_loop(EGUI_VIEW_OF(&s_anim_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(100, (int)egui_view_animated_image_get_interval(EGUI_VIEW_OF(&s_anim_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_get_elapsed(EGUI_VIEW_OF(&s_anim_img)));
}

static void test_animated_image_get_state_after_setters(void)
{
    setup();
    egui_view_animated_image_set_frames(EGUI_VIEW_OF(&s_anim_img), s_frames, 3);
    egui_view_animated_image_set_interval(EGUI_VIEW_OF(&s_anim_img), 80);
    egui_view_animated_image_set_loop(EGUI_VIEW_OF(&s_anim_img), 0);
    egui_view_animated_image_set_current_frame(EGUI_VIEW_OF(&s_anim_img), 2);

    EGUI_TEST_ASSERT_TRUE(egui_view_animated_image_get_frames(EGUI_VIEW_OF(&s_anim_img)) == s_frames);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_animated_image_get_frame_count(EGUI_VIEW_OF(&s_anim_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(80, (int)egui_view_animated_image_get_interval(EGUI_VIEW_OF(&s_anim_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_get_loop(EGUI_VIEW_OF(&s_anim_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_animated_image_get_current_frame(EGUI_VIEW_OF(&s_anim_img)));
}

static void test_animated_image_get_state_tracks_update(void)
{
    setup();
    egui_view_animated_image_set_frames(EGUI_VIEW_OF(&s_anim_img), s_frames, 3);
    egui_view_animated_image_set_interval(EGUI_VIEW_OF(&s_anim_img), 50);
    egui_view_animated_image_play(EGUI_VIEW_OF(&s_anim_img));

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_animated_image_is_playing(EGUI_VIEW_OF(&s_anim_img)));

    egui_view_animated_image_update(EGUI_VIEW_OF(&s_anim_img), 35);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_get_current_frame(EGUI_VIEW_OF(&s_anim_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(35, (int)egui_view_animated_image_get_elapsed(EGUI_VIEW_OF(&s_anim_img)));

    egui_view_animated_image_update(EGUI_VIEW_OF(&s_anim_img), 20);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_animated_image_get_current_frame(EGUI_VIEW_OF(&s_anim_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_animated_image_get_elapsed(EGUI_VIEW_OF(&s_anim_img)));

    egui_view_animated_image_stop(EGUI_VIEW_OF(&s_anim_img));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_is_playing(EGUI_VIEW_OF(&s_anim_img)));
}

static void test_animated_image_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_animated_image_get_frames(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_get_frame_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_get_current_frame(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_is_playing(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_get_loop(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_get_interval(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_animated_image_get_elapsed(NULL));
}

void test_animated_image_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(animated_image_get_state);

    EGUI_TEST_RUN(test_animated_image_get_state_defaults);
    EGUI_TEST_RUN(test_animated_image_get_state_after_setters);
    EGUI_TEST_RUN(test_animated_image_get_state_tracks_update);
    EGUI_TEST_RUN(test_animated_image_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
