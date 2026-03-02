#include "egui.h"
#include "test/egui_test.h"
#include "test_region.h"

static void test_region_init(void)
{
    egui_region_t r;
    egui_region_init(&r, 10, 20, 100, 200);
    EGUI_TEST_ASSERT_EQUAL_INT(10, r.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(20, r.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(100, r.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(200, r.size.height);
}

static void test_region_init_empty(void)
{
    egui_region_t r;
    egui_region_init(&r, 10, 20, 100, 200);
    egui_region_init_empty(&r);
    EGUI_TEST_ASSERT_EQUAL_INT(0, r.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(0, r.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(0, r.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(0, r.size.height);
}

static void test_region_is_empty(void)
{
    egui_region_t r;
    egui_region_init_empty(&r);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&r));

    egui_region_init(&r, 0, 0, 10, 10);
    EGUI_TEST_ASSERT_FALSE(egui_region_is_empty(&r));

    // Zero-width region: set manually since egui_region_init asserts width > 0
    r.location.x = 0;
    r.location.y = 0;
    r.size.width = 0;
    r.size.height = 10;
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&r));

    // Zero-height region
    r.size.width = 10;
    r.size.height = 0;
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&r));

    // NULL region should be empty
    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(NULL));
}

static void test_region_pt_in_rect(void)
{
    egui_region_t r;
    egui_region_init(&r, 10, 10, 50, 50);

    // Inside
    EGUI_TEST_ASSERT_TRUE(egui_region_pt_in_rect(&r, 20, 20));
    // Top-left corner (inclusive)
    EGUI_TEST_ASSERT_TRUE(egui_region_pt_in_rect(&r, 10, 10));
    // Bottom-right corner (exclusive)
    EGUI_TEST_ASSERT_FALSE(egui_region_pt_in_rect(&r, 60, 60));
    // Just inside bottom-right
    EGUI_TEST_ASSERT_TRUE(egui_region_pt_in_rect(&r, 59, 59));
    // Outside left
    EGUI_TEST_ASSERT_FALSE(egui_region_pt_in_rect(&r, 5, 20));
    // Outside top
    EGUI_TEST_ASSERT_FALSE(egui_region_pt_in_rect(&r, 20, 5));
}

static void test_region_equal(void)
{
    egui_region_t a;
    egui_region_t b;
    egui_region_init(&a, 10, 20, 30, 40);
    egui_region_init(&b, 10, 20, 30, 40);
    EGUI_TEST_ASSERT_TRUE(egui_region_equal(&a, &b));

    egui_region_init(&b, 10, 20, 30, 41);
    EGUI_TEST_ASSERT_FALSE(egui_region_equal(&a, &b));

    egui_region_init(&b, 11, 20, 30, 40);
    EGUI_TEST_ASSERT_FALSE(egui_region_equal(&a, &b));
}

static void test_region_copy(void)
{
    egui_region_t a;
    egui_region_t b;
    egui_region_init(&a, 10, 20, 30, 40);
    egui_region_init_empty(&b);
    egui_region_copy(&b, &a);
    EGUI_TEST_ASSERT_TRUE(egui_region_equal(&a, &b));
}

static void test_region_intersect_overlap(void)
{
    egui_region_t a;
    egui_region_t b;
    egui_region_t result;

    egui_region_init(&a, 0, 0, 50, 50);
    egui_region_init(&b, 25, 25, 50, 50);
    egui_region_intersect(&a, &b, &result);

    EGUI_TEST_ASSERT_EQUAL_INT(25, result.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(25, result.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(25, result.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(25, result.size.height);
}

static void test_region_intersect_disjoint(void)
{
    egui_region_t a;
    egui_region_t b;
    egui_region_t result;

    egui_region_init(&a, 0, 0, 10, 10);
    egui_region_init(&b, 20, 20, 10, 10);
    egui_region_intersect(&a, &b, &result);

    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&result));
}

static void test_region_intersect_contain(void)
{
    egui_region_t a;
    egui_region_t b;
    egui_region_t result;

    egui_region_init(&a, 0, 0, 100, 100);
    egui_region_init(&b, 10, 10, 20, 20);
    egui_region_intersect(&a, &b, &result);

    EGUI_TEST_ASSERT_EQUAL_INT(10, result.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(10, result.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(20, result.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(20, result.size.height);
}

static void test_region_intersect_empty(void)
{
    egui_region_t a;
    egui_region_t b;
    egui_region_t result;

    egui_region_init(&a, 0, 0, 50, 50);
    egui_region_init_empty(&b);
    egui_region_intersect(&a, &b, &result);

    EGUI_TEST_ASSERT_TRUE(egui_region_is_empty(&result));
}

static void test_region_is_intersect(void)
{
    egui_region_t a;
    egui_region_t b;

    egui_region_init(&a, 0, 0, 50, 50);
    egui_region_init(&b, 25, 25, 50, 50);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_intersect(&a, &b));

    egui_region_init(&b, 60, 60, 10, 10);
    EGUI_TEST_ASSERT_FALSE(egui_region_is_intersect(&a, &b));
}

static void test_region_union_basic(void)
{
    egui_region_t a;
    egui_region_t b;
    egui_region_t result;

    egui_region_init(&a, 0, 0, 50, 50);
    egui_region_init(&b, 25, 25, 50, 50);
    egui_region_union(&a, &b, &result);

    EGUI_TEST_ASSERT_EQUAL_INT(0, result.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(0, result.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(75, result.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(75, result.size.height);
}

static void test_region_union_disjoint(void)
{
    egui_region_t a;
    egui_region_t b;
    egui_region_t result;

    egui_region_init(&a, 0, 0, 10, 10);
    egui_region_init(&b, 50, 50, 10, 10);
    egui_region_union(&a, &b, &result);

    EGUI_TEST_ASSERT_EQUAL_INT(0, result.location.x);
    EGUI_TEST_ASSERT_EQUAL_INT(0, result.location.y);
    EGUI_TEST_ASSERT_EQUAL_INT(60, result.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(60, result.size.height);
}

static void test_region_is_same(void)
{
    egui_region_t a;
    egui_region_t b;

    egui_region_init(&a, 10, 20, 30, 40);
    egui_region_init(&b, 10, 20, 30, 40);
    EGUI_TEST_ASSERT_TRUE(egui_region_is_same(&a, &b));

    egui_region_init(&b, 10, 20, 31, 40);
    EGUI_TEST_ASSERT_FALSE(egui_region_is_same(&a, &b));
}

void test_region_run(void)
{
    EGUI_TEST_SUITE_BEGIN(region);

    EGUI_TEST_RUN(test_region_init);
    EGUI_TEST_RUN(test_region_init_empty);
    EGUI_TEST_RUN(test_region_is_empty);
    EGUI_TEST_RUN(test_region_pt_in_rect);
    EGUI_TEST_RUN(test_region_equal);
    EGUI_TEST_RUN(test_region_copy);
    EGUI_TEST_RUN(test_region_intersect_overlap);
    EGUI_TEST_RUN(test_region_intersect_disjoint);
    EGUI_TEST_RUN(test_region_intersect_contain);
    EGUI_TEST_RUN(test_region_intersect_empty);
    EGUI_TEST_RUN(test_region_is_intersect);
    EGUI_TEST_RUN(test_region_union_basic);
    EGUI_TEST_RUN(test_region_union_disjoint);
    EGUI_TEST_RUN(test_region_is_same);

    EGUI_TEST_SUITE_END();
}
