#include <string.h>

#include "egui_test.h"
#include "core/egui_api.h"
#include "core/egui_region.h"

static egui_test_state_t s_egui_test_state;

void egui_test_init(void)
{
    egui_api_memset(&s_egui_test_state, 0, sizeof(s_egui_test_state));
}

void egui_test_suite_begin(const char *name)
{
    s_egui_test_state.current_suite_name = name;
    s_egui_test_state.current_suite_tests = 0;
    s_egui_test_state.current_suite_passed = 0;
    s_egui_test_state.current_suite_failed = 0;
    s_egui_test_state.total_suites++;
    egui_api_log("\n===== Suite: %s =====\n", name);
}

void egui_test_suite_end(void)
{
    egui_api_log("----- Suite: %s  [%d/%d passed] -----\n", s_egui_test_state.current_suite_name, s_egui_test_state.current_suite_passed,
                 s_egui_test_state.current_suite_tests);
}

void egui_test_case_begin(const char *name)
{
    s_egui_test_state.current_test_name = name;
    s_egui_test_state.total_tests++;
    s_egui_test_state.current_suite_tests++;
    s_egui_test_state.current_test_failed = 0;
}

void egui_test_case_end(void)
{
    if (s_egui_test_state.current_test_failed)
    {
        s_egui_test_state.total_failed++;
        s_egui_test_state.current_suite_failed++;
        egui_api_log("  [FAIL] %s\n", s_egui_test_state.current_test_name);
    }
    else
    {
        s_egui_test_state.total_passed++;
        s_egui_test_state.current_suite_passed++;
        egui_api_log("  [PASS] %s\n", s_egui_test_state.current_test_name);
    }
}

void egui_test_fail(const char *file, int line, const char *expr, const char *msg)
{
    s_egui_test_state.current_test_failed = 1;
    egui_api_log("  ASSERT FAILED: %s:%d: %s (%s)\n", file, line, expr, msg);
}

void egui_test_fail_int(const char *file, int line, const char *expr, int expected, int actual)
{
    s_egui_test_state.current_test_failed = 1;
    egui_api_log("  ASSERT FAILED: %s:%d: %s expected=%d actual=%d\n", file, line, expr, expected, actual);
}

int egui_test_region_is_same(const egui_region_t *a, const egui_region_t *b)
{
    return egui_region_equal(a, b);
}

void egui_test_fail_region(const char *file, int line, const char *expr, const egui_region_t *expected, const egui_region_t *actual)
{
    s_egui_test_state.current_test_failed = 1;
    egui_api_log("  ASSERT FAILED: %s:%d: %s\n"
                 "    expected: (%d,%d,%d,%d)\n"
                 "    actual:   (%d,%d,%d,%d)\n",
                 file, line, expr, expected->location.x, expected->location.y, expected->size.width, expected->size.height, actual->location.x,
                 actual->location.y, actual->size.width, actual->size.height);
}

int egui_test_get_result(void)
{
    return (s_egui_test_state.total_failed > 0) ? 1 : 0;
}

void egui_test_print_summary(void)
{
    egui_api_log("\n========================================\n");
    egui_api_log("TEST SUMMARY\n");
    egui_api_log("  Suites: %d\n", s_egui_test_state.total_suites);
    egui_api_log("  Tests:  %d total, %d passed, %d failed\n", s_egui_test_state.total_tests, s_egui_test_state.total_passed, s_egui_test_state.total_failed);
    egui_api_log("  Result: %s\n", s_egui_test_state.total_failed == 0 ? "ALL PASSED" : "FAILED");
    egui_api_log("========================================\n");
}
