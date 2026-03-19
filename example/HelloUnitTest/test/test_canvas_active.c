#include "egui.h"
#include "test/egui_test.h"
#include "test_canvas_active.h"

static egui_color_int_t test_pfb[20 * 20];

static void setup_canvas(const egui_region_t *pfb_region)
{
    egui_region_t base_region;

    egui_canvas_init(test_pfb, (egui_region_t *)pfb_region);
    egui_region_init(&base_region, 30, 50, 80, 80);
    egui_canvas_calc_work_region(&base_region);
}

static void test_canvas_is_region_active_inside(void)
{
    egui_region_t pfb_region;
    egui_region_t region;

    egui_region_init(&pfb_region, 40, 60, 20, 20);
    setup_canvas(&pfb_region);

    egui_region_init(&region, 45, 65, 10, 10);
    EGUI_TEST_ASSERT_TRUE(egui_canvas_is_region_active(&region));
}

static void test_canvas_is_region_active_outside(void)
{
    egui_region_t pfb_region;
    egui_region_t region;

    egui_region_init(&pfb_region, 40, 60, 20, 20);
    setup_canvas(&pfb_region);

    egui_region_init(&region, 5, 5, 10, 10);
    EGUI_TEST_ASSERT_FALSE(egui_canvas_is_region_active(&region));
}

static void test_canvas_is_region_active_partial(void)
{
    egui_region_t pfb_region;
    egui_region_t region;

    egui_region_init(&pfb_region, 40, 60, 20, 20);
    setup_canvas(&pfb_region);

    egui_region_init(&region, 55, 75, 20, 10);
    EGUI_TEST_ASSERT_TRUE(egui_canvas_is_region_active(&region));
}

static void test_canvas_is_region_active_edge(void)
{
    egui_region_t pfb_region;
    egui_region_t region;

    egui_region_init(&pfb_region, 40, 60, 20, 20);
    setup_canvas(&pfb_region);

    egui_region_init(&region, 60, 60, 10, 10);
    EGUI_TEST_ASSERT_FALSE(egui_canvas_is_region_active(&region));
}

void test_canvas_active_run(void)
{
    EGUI_TEST_SUITE_BEGIN(canvas_active);
    EGUI_TEST_RUN(test_canvas_is_region_active_inside);
    EGUI_TEST_RUN(test_canvas_is_region_active_outside);
    EGUI_TEST_RUN(test_canvas_is_region_active_partial);
    EGUI_TEST_RUN(test_canvas_is_region_active_edge);
    EGUI_TEST_SUITE_END();
}
