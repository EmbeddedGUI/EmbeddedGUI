#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_image_transform.h"

#if EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM

static egui_view_image_t s_img;

static egui_core_t *get_core(void)
{
    egui_core_t *core = uicode_get_core();
    EGUI_ASSERT(core != NULL);
    return core;
}

static void init_img(void)
{
    egui_view_image_init(EGUI_VIEW_OF(&s_img), get_core());
}

#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
/* angle_deg defaults to 0 after init. */
static void test_default_angle(void)
{
    init_img();
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_image_get_angle(EGUI_VIEW_OF(&s_img)));
}
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

/* scale_q8 defaults to 256 (1x) after init. */
static void test_default_scale(void)
{
    init_img();
    EGUI_TEST_ASSERT_EQUAL_INT(256, egui_view_image_get_scale(EGUI_VIEW_OF(&s_img)));
}

#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
/* set_angle stores and retrieves the value. */
static void test_set_angle(void)
{
    init_img();
    egui_view_image_set_angle(EGUI_VIEW_OF(&s_img), 90);
    EGUI_TEST_ASSERT_EQUAL_INT(90, egui_view_image_get_angle(EGUI_VIEW_OF(&s_img)));
}
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

/* set_scale stores and retrieves the value. */
static void test_set_scale(void)
{
    init_img();
    egui_view_image_set_scale(EGUI_VIEW_OF(&s_img), 512);
    EGUI_TEST_ASSERT_EQUAL_INT(512, egui_view_image_get_scale(EGUI_VIEW_OF(&s_img)));
}

#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
/* set_transform updates both fields. */
static void test_set_transform(void)
{
    init_img();
    egui_view_image_set_transform(EGUI_VIEW_OF(&s_img), 45, 128);
    EGUI_TEST_ASSERT_EQUAL_INT(45, egui_view_image_get_angle(EGUI_VIEW_OF(&s_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(128, egui_view_image_get_scale(EGUI_VIEW_OF(&s_img)));
}

/* set_angle with same value does not crash (idempotent). */
static void test_set_angle_same(void)
{
    init_img();
    egui_view_image_set_angle(EGUI_VIEW_OF(&s_img), 180);
    egui_view_image_set_angle(EGUI_VIEW_OF(&s_img), 180);
    EGUI_TEST_ASSERT_EQUAL_INT(180, egui_view_image_get_angle(EGUI_VIEW_OF(&s_img)));
}
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

/* set_scale with same value does not crash (idempotent). */
static void test_set_scale_same(void)
{
    init_img();
    egui_view_image_set_scale(EGUI_VIEW_OF(&s_img), 256);
    egui_view_image_set_scale(EGUI_VIEW_OF(&s_img), 256);
    EGUI_TEST_ASSERT_EQUAL_INT(256, egui_view_image_get_scale(EGUI_VIEW_OF(&s_img)));
}

#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
/* set_transform with same values does not crash (idempotent). */
static void test_set_transform_same(void)
{
    init_img();
    egui_view_image_set_transform(EGUI_VIEW_OF(&s_img), 0, 256);
    egui_view_image_set_transform(EGUI_VIEW_OF(&s_img), 0, 256);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_image_get_angle(EGUI_VIEW_OF(&s_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(256, egui_view_image_get_scale(EGUI_VIEW_OF(&s_img)));
}

/* NULL self returns default zero values without crash. */
static void test_transform_angle_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_image_get_angle(NULL));
}
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

/* NULL self returns default zero values without crash. */
static void test_transform_scale_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_image_get_scale(NULL));
}

#endif /* EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */

void test_image_transform_run(void)
{
#if EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
    EGUI_TEST_RUN(test_default_angle);
#endif
    EGUI_TEST_RUN(test_default_scale);
#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
    EGUI_TEST_RUN(test_set_angle);
#endif
    EGUI_TEST_RUN(test_set_scale);
#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
    EGUI_TEST_RUN(test_set_transform);
    EGUI_TEST_RUN(test_set_angle_same);
#endif
    EGUI_TEST_RUN(test_set_scale_same);
#if EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM
    EGUI_TEST_RUN(test_set_transform_same);
    EGUI_TEST_RUN(test_transform_angle_null_self);
#endif
    EGUI_TEST_RUN(test_transform_scale_null_self);
#endif /* EGUI_CONFIG_FUNCTION_IMAGE_SCALE_LITE || EGUI_CONFIG_FUNCTION_IMAGE_TRANSFORM */
}
