#include <string.h>

#include "egui.h"
#include "widget/egui_view_image.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_image_get_image.h"

static egui_view_image_t s_img;
static egui_image_t      s_img_resource;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();
    memset(&s_img,          0, sizeof(s_img));
    memset(&s_img_resource, 0, sizeof(s_img_resource));
    egui_view_image_init(EGUI_VIEW_OF(&s_img), core);
}

/* Default image pointer after init is NULL. */
static void test_image_get_image_default_null(void)
{
    setup();
    EGUI_TEST_ASSERT_NULL(egui_view_image_get_image(EGUI_VIEW_OF(&s_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_IMAGE_TYPE_NORMAL, egui_view_image_get_image_type(EGUI_VIEW_OF(&s_img)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_image_get_image_color(EGUI_VIEW_OF(&s_img)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_image_get_image_alpha(EGUI_VIEW_OF(&s_img)));
}

/* After set_image the getter returns that pointer. */
static void test_image_get_image_after_set(void)
{
    setup();
    egui_view_image_set_image(EGUI_VIEW_OF(&s_img), &s_img_resource);
    EGUI_TEST_ASSERT_EQUAL_INT((int)(uintptr_t)&s_img_resource,
                               (int)(uintptr_t)egui_view_image_get_image(EGUI_VIEW_OF(&s_img)));
}

/* set_image(NULL) clears the image pointer. */
static void test_image_get_image_clear_to_null(void)
{
    setup();
    egui_view_image_set_image(EGUI_VIEW_OF(&s_img), &s_img_resource);
    egui_view_image_set_image(EGUI_VIEW_OF(&s_img), NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_image_get_image(EGUI_VIEW_OF(&s_img)));
}

/* Image type getter returns the last configured draw mode. */
static void test_image_get_image_type_after_set(void)
{
    setup();
    egui_view_image_set_image_type(EGUI_VIEW_OF(&s_img), EGUI_VIEW_IMAGE_TYPE_RESIZE);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_IMAGE_TYPE_RESIZE, egui_view_image_get_image_type(EGUI_VIEW_OF(&s_img)));
}

/* Image tint getters return the configured color and alpha. */
static void test_image_get_image_color_after_set(void)
{
    egui_color_t color = EGUI_COLOR_GREEN;

    setup();
    egui_view_image_set_image_color(EGUI_VIEW_OF(&s_img), color, EGUI_ALPHA_60);

    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)egui_view_image_get_image_color(EGUI_VIEW_OF(&s_img)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ALPHA_60, (int)egui_view_image_get_image_alpha(EGUI_VIEW_OF(&s_img)));
}

/* Setting tint alpha to 0 keeps the color but disables tint drawing. */
static void test_image_get_image_color_alpha_zero(void)
{
    egui_color_t color = EGUI_COLOR_RED;

    setup();
    egui_view_image_set_image_color(EGUI_VIEW_OF(&s_img), color, 0);

    EGUI_TEST_ASSERT_EQUAL_INT((int)color.full, (int)egui_view_image_get_image_color(EGUI_VIEW_OF(&s_img)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_image_get_image_alpha(EGUI_VIEW_OF(&s_img)));
}

/* NULL self returns NULL without crash. */
static void test_image_get_image_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_image_get_image(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_image_get_image_type(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_image_get_image_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_image_get_image_alpha(NULL));
}

void test_image_get_image_run(void)
{
    EGUI_TEST_SUITE_BEGIN(image_get_image);

    EGUI_TEST_RUN(test_image_get_image_default_null);
    EGUI_TEST_RUN(test_image_get_image_after_set);
    EGUI_TEST_RUN(test_image_get_image_clear_to_null);
    EGUI_TEST_RUN(test_image_get_image_type_after_set);
    EGUI_TEST_RUN(test_image_get_image_color_after_set);
    EGUI_TEST_RUN(test_image_get_image_color_alpha_zero);
    EGUI_TEST_RUN(test_image_get_image_null_self);

    EGUI_TEST_SUITE_END();
}
