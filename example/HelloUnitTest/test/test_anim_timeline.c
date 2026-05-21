#include <string.h>

#include "egui.h"
#include "anim/egui_animation_timeline.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_anim_timeline.h"

#if EGUI_CONFIG_FUNCTION_ANIM_DELAY

static egui_animation_timeline_t    s_tl;
static egui_animation_translate_t   s_anim0;
static egui_animation_translate_t   s_anim1;
static egui_animation_translate_t   s_anim2;

static egui_view_t       s_view0;
static egui_view_t       s_view1;
static egui_view_t       s_view2;
static egui_view_group_t s_parent;

static const egui_animation_translate_params_t s_params = {
    .from_x = 0, .to_x = 10, .from_y = 0, .to_y = 0
};

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    egui_slist_init(&core->scene.anims);

    memset(&s_parent, 0, sizeof(s_parent));
    memset(&s_view0,  0, sizeof(s_view0));
    memset(&s_view1,  0, sizeof(s_view1));
    memset(&s_view2,  0, sizeof(s_view2));
    memset(&s_anim0,  0, sizeof(s_anim0));
    memset(&s_anim1,  0, sizeof(s_anim1));
    memset(&s_anim2,  0, sizeof(s_anim2));

    egui_view_group_init(EGUI_VIEW_OF(&s_parent), core);
    egui_view_init(&s_view0, core);
    egui_view_init(&s_view1, core);
    egui_view_init(&s_view2, core);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), &s_view0);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), &s_view1);
    egui_view_group_add_child(EGUI_VIEW_OF(&s_parent), &s_view2);

    egui_animation_translate_init(EGUI_ANIM_OF(&s_anim0));
    egui_animation_translate_params_set(&s_anim0, &s_params);
    egui_animation_target_view_set(EGUI_ANIM_OF(&s_anim0), &s_view0);
    egui_animation_duration_set(EGUI_ANIM_OF(&s_anim0), 100);

    egui_animation_translate_init(EGUI_ANIM_OF(&s_anim1));
    egui_animation_translate_params_set(&s_anim1, &s_params);
    egui_animation_target_view_set(EGUI_ANIM_OF(&s_anim1), &s_view1);
    egui_animation_duration_set(EGUI_ANIM_OF(&s_anim1), 200);

    egui_animation_translate_init(EGUI_ANIM_OF(&s_anim2));
    egui_animation_translate_params_set(&s_anim2, &s_params);
    egui_animation_target_view_set(EGUI_ANIM_OF(&s_anim2), &s_view2);
    egui_animation_duration_set(EGUI_ANIM_OF(&s_anim2), 50);

    egui_animation_timeline_init(&s_tl);
}

/* After init, count is 0. */
static void test_timeline_init_empty(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_tl.count);
}

/* Adding entries increments count. */
static void test_timeline_add_increments_count(void)
{
    setup();
    egui_animation_timeline_add(&s_tl, EGUI_ANIM_OF(&s_anim0), 0);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_tl.count);
    egui_animation_timeline_add(&s_tl, EGUI_ANIM_OF(&s_anim1), 100);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)s_tl.count);
}

/* get_duration: max(start+dur) across entries. */
static void test_timeline_get_duration(void)
{
    setup();
    /* anim0 start=0   dur=100 → end=100 */
    /* anim1 start=200 dur=200 → end=400 */
    /* anim2 start=300 dur=50  → end=350 */
    egui_animation_timeline_add(&s_tl, EGUI_ANIM_OF(&s_anim0), 0);
    egui_animation_timeline_add(&s_tl, EGUI_ANIM_OF(&s_anim1), 200);
    egui_animation_timeline_add(&s_tl, EGUI_ANIM_OF(&s_anim2), 300);
    EGUI_TEST_ASSERT_EQUAL_INT(400, (int)egui_animation_timeline_get_duration(&s_tl));
}

/* Empty timeline duration is 0. */
static void test_timeline_duration_empty(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_animation_timeline_get_duration(&s_tl));
}

/* NULL self does not crash. */
static void test_timeline_null_safe(void)
{
    egui_animation_timeline_init(NULL);
    egui_animation_timeline_add(NULL, EGUI_ANIM_OF(&s_anim0), 0);
    egui_animation_timeline_start(NULL);
    egui_animation_timeline_stop(NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_animation_timeline_get_duration(NULL));
}

/* start sets delay on each animation entry. */
static void test_timeline_start_sets_delay(void)
{
    setup();
    egui_animation_timeline_add(&s_tl, EGUI_ANIM_OF(&s_anim0), 150);
    egui_animation_timeline_add(&s_tl, EGUI_ANIM_OF(&s_anim1), 300);
    egui_animation_timeline_start(&s_tl);
    EGUI_TEST_ASSERT_EQUAL_INT(150, (int)s_anim0.base.delay_ms);
    EGUI_TEST_ASSERT_EQUAL_INT(300, (int)s_anim1.base.delay_ms);
}

#endif /* EGUI_CONFIG_FUNCTION_ANIM_DELAY */

void test_anim_timeline_run(void)
{
    EGUI_TEST_SUITE_BEGIN(anim_timeline);

#if EGUI_CONFIG_FUNCTION_ANIM_DELAY
    EGUI_TEST_RUN(test_timeline_init_empty);
    EGUI_TEST_RUN(test_timeline_add_increments_count);
    EGUI_TEST_RUN(test_timeline_get_duration);
    EGUI_TEST_RUN(test_timeline_duration_empty);
    EGUI_TEST_RUN(test_timeline_null_safe);
    EGUI_TEST_RUN(test_timeline_start_sets_delay);
#endif

    EGUI_TEST_SUITE_END();
}
