#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_canvas_viewport.h"

static egui_view_canvas_viewport_t s_viewport;

static egui_core_t *test_canvas_viewport_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void test_canvas_viewport_setup(egui_dim_t width, egui_dim_t height, egui_dim_t canvas_width, egui_dim_t canvas_height)
{
    egui_core_t *core = test_canvas_viewport_get_core();
    egui_region_t region;

    egui_view_canvas_viewport_init(EGUI_VIEW_OF(&s_viewport), core);
    EGUI_VIEW_OF(&s_viewport)->core = core;
    egui_region_init(&region, 0, 0, width, height);
    egui_view_layout(EGUI_VIEW_OF(&s_viewport), &region);
    egui_view_canvas_viewport_set_canvas_size(EGUI_VIEW_OF(&s_viewport), canvas_width, canvas_height);
}

static void test_canvas_viewport_get_max_offsets(void)
{
    test_canvas_viewport_setup(100, 80, 180, 140);

    EGUI_TEST_ASSERT_EQUAL_INT(80, egui_view_canvas_viewport_get_max_offset_x(EGUI_VIEW_OF(&s_viewport)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, egui_view_canvas_viewport_get_max_offset_y(EGUI_VIEW_OF(&s_viewport)));

    egui_view_canvas_viewport_set_canvas_size(EGUI_VIEW_OF(&s_viewport), 40, 20);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_canvas_viewport_get_max_offset_x(EGUI_VIEW_OF(&s_viewport)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_canvas_viewport_get_max_offset_y(EGUI_VIEW_OF(&s_viewport)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_canvas_viewport_get_max_offset_x(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_canvas_viewport_get_max_offset_y(NULL));
}

static void test_canvas_viewport_offset_clamps_to_max_offsets(void)
{
    test_canvas_viewport_setup(100, 80, 180, 140);

    egui_view_canvas_viewport_set_offset(EGUI_VIEW_OF(&s_viewport), 200, 100);
    EGUI_TEST_ASSERT_EQUAL_INT(80, egui_view_canvas_viewport_get_offset_x(EGUI_VIEW_OF(&s_viewport)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, egui_view_canvas_viewport_get_offset_y(EGUI_VIEW_OF(&s_viewport)));
    EGUI_TEST_ASSERT_EQUAL_INT(80, egui_view_canvas_viewport_get_max_offset_x(EGUI_VIEW_OF(&s_viewport)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, egui_view_canvas_viewport_get_max_offset_y(EGUI_VIEW_OF(&s_viewport)));
}

void test_canvas_viewport_run(void)
{
    EGUI_TEST_SUITE_BEGIN(canvas_viewport);

    EGUI_TEST_RUN(test_canvas_viewport_get_max_offsets);
    EGUI_TEST_RUN(test_canvas_viewport_offset_clamps_to_max_offsets);

    EGUI_TEST_SUITE_END();
}
