#include <string.h>

#include "egui.h"
#include "anim/egui_view_fade.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_view_fade.h"

static egui_view_fade_t s_fade;
static egui_view_label_t s_label;
static egui_view_group_t s_parent;

static void setup_view(void)
{
    egui_core_t *core = uicode_get_core();

    egui_slist_init(&core->scene.anims);
    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_label, 0, sizeof(s_label));
    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_label_init(EGUI_VIEW_OF(&s_label), core);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), EGUI_VIEW_OF(&s_label));
}

/* fade_in sets view visible and starts alpha animation. */
static void test_fade_in_sets_visible(void)
{
    setup_view();
    egui_view_set_visible(EGUI_VIEW_OF(&s_label), 0);
    egui_view_fade_in(&s_fade, EGUI_VIEW_OF(&s_label), 200, 0);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&s_label)->is_visible);
    EGUI_TEST_ASSERT_TRUE(s_fade.anim.base.is_running);
    EGUI_TEST_ASSERT_FALSE(s_fade.is_fade_out);
    egui_animation_stop(EGUI_ANIM_OF(&s_fade.anim));
}

/* fade_in with duration=0 no-crashes. */
static void test_fade_in_zero_duration_no_crash(void)
{
    setup_view();
    egui_view_fade_in(&s_fade, EGUI_VIEW_OF(&s_label), 0, 0);
    EGUI_TEST_ASSERT_TRUE(1); /* no crash */
    if (s_fade.anim.base.is_running)
        egui_animation_stop(EGUI_ANIM_OF(&s_fade.anim));
}

/* fade_out starts animation and marks is_fade_out. */
static void test_fade_out_marks_flag(void)
{
    setup_view();
    egui_view_set_visible(EGUI_VIEW_OF(&s_label), 1);
    egui_view_fade_out(&s_fade, EGUI_VIEW_OF(&s_label), 200, 0);
    EGUI_TEST_ASSERT_TRUE(s_fade.is_fade_out);
    EGUI_TEST_ASSERT_TRUE(s_fade.anim.base.is_running);
    egui_animation_stop(EGUI_ANIM_OF(&s_fade.anim));
}

/* fade_out hides view when animation completes. */
static void test_fade_out_hides_on_complete(void)
{
    setup_view();
    egui_view_set_visible(EGUI_VIEW_OF(&s_label), 1);
    egui_view_fade_out(&s_fade, EGUI_VIEW_OF(&s_label), 100, 0);
    egui_animation_update(EGUI_ANIM_OF(&s_fade.anim), 0);
    /* Force animation to finish */
    egui_animation_complete(EGUI_ANIM_OF(&s_fade.anim));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&s_label)->is_visible);
}

/* fade_in on already-visible view does not crash. */
static void test_fade_in_already_visible_no_crash(void)
{
    setup_view();
    egui_view_set_visible(EGUI_VIEW_OF(&s_label), 1);
    egui_view_fade_in(&s_fade, EGUI_VIEW_OF(&s_label), 100, 0);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&s_label)->is_visible);
    if (s_fade.anim.base.is_running)
        egui_animation_stop(EGUI_ANIM_OF(&s_fade.anim));
}

void test_view_fade_run(void)
{
    EGUI_TEST_SUITE_BEGIN(view_fade);

    EGUI_TEST_RUN(test_fade_in_sets_visible);
    EGUI_TEST_RUN(test_fade_in_zero_duration_no_crash);
    EGUI_TEST_RUN(test_fade_out_marks_flag);
    EGUI_TEST_RUN(test_fade_out_hides_on_complete);
    EGUI_TEST_RUN(test_fade_in_already_visible_no_crash);

    EGUI_TEST_SUITE_END();
}
