#include <string.h>

#include "egui_test.h"
#include "core/egui_api.h"
#include "core/egui_region.h"

egui_test_state_t g_egui_test_state;

static int egui_test_filter_token_matches(const char *name, const char *token_start, const char *token_end)
{
    while (token_start < token_end && (*token_start == ' ' || *token_start == '\t'))
    {
        token_start++;
    }
    while (token_end > token_start && (token_end[-1] == ' ' || token_end[-1] == '\t'))
    {
        token_end--;
    }

    if (token_start == token_end)
    {
        return 0;
    }
    if ((token_end - token_start) == 1 && token_start[0] == '*')
    {
        return 1;
    }
    if (strlen(name) != (size_t)(token_end - token_start))
    {
        return 0;
    }
    return strncmp(name, token_start, (size_t)(token_end - token_start)) == 0;
}

static int egui_test_suite_filter_matches(const char *name, const char *filter)
{
    const char *token_start;
    const char *cursor;

    if (filter == NULL || filter[0] == '\0')
    {
        return 1;
    }

    token_start = filter;
    cursor = filter;
    while (1)
    {
        if (*cursor == ',' || *cursor == '\0')
        {
            if (egui_test_filter_token_matches(name, token_start, cursor))
            {
                return 1;
            }
            if (*cursor == '\0')
            {
                break;
            }
            token_start = cursor + 1;
        }
        cursor++;
    }

    return 0;
}

void egui_test_init(void)
{
    egui_api_memset(&g_egui_test_state, 0, sizeof(g_egui_test_state));
    g_egui_test_state.current_suite_enabled = 1;
}

void egui_test_set_suite_filter(const char *filter)
{
    g_egui_test_state.suite_filter = filter;
}

void egui_test_suite_begin(const char *name)
{
    g_egui_test_state.current_suite_name = name;
    g_egui_test_state.current_suite_tests = 0;
    g_egui_test_state.current_suite_passed = 0;
    g_egui_test_state.current_suite_failed = 0;
    g_egui_test_state.current_suite_enabled = (uint8_t)egui_test_suite_filter_matches(name, g_egui_test_state.suite_filter);
    if (!g_egui_test_state.current_suite_enabled)
    {
        return;
    }
    g_egui_test_state.total_suites++;
    egui_api_log("\n===== Suite: %s =====\n", name);
}

void egui_test_suite_end(void)
{
    if (!g_egui_test_state.current_suite_enabled)
    {
        return;
    }
    egui_api_log("----- Suite: %s  [%d/%d passed] -----\n", g_egui_test_state.current_suite_name, g_egui_test_state.current_suite_passed,
                 g_egui_test_state.current_suite_tests);
}

void egui_test_case_begin(const char *name)
{
    if (!g_egui_test_state.current_suite_enabled)
    {
        return;
    }
    g_egui_test_state.current_test_name = name;
    g_egui_test_state.total_tests++;
    g_egui_test_state.current_suite_tests++;
    g_egui_test_state.current_test_failed = 0;
}

void egui_test_case_end(void)
{
    if (!g_egui_test_state.current_suite_enabled)
    {
        return;
    }
    if (g_egui_test_state.current_test_failed)
    {
        g_egui_test_state.total_failed++;
        g_egui_test_state.current_suite_failed++;
        egui_api_log("  [FAIL] %s\n", g_egui_test_state.current_test_name);
    }
    else
    {
        g_egui_test_state.total_passed++;
        g_egui_test_state.current_suite_passed++;
        egui_api_log("  [PASS] %s\n", g_egui_test_state.current_test_name);
    }
}

int egui_test_is_current_suite_enabled(void)
{
    return g_egui_test_state.current_suite_enabled;
}

void egui_test_fail(const char *file, int line, const char *expr, const char *msg)
{
    g_egui_test_state.current_test_failed = 1;
    egui_api_log("  ASSERT FAILED: %s:%d: %s (%s)\n", file, line, expr, msg);
}

void egui_test_fail_int(const char *file, int line, const char *expr, int expected, int actual)
{
    g_egui_test_state.current_test_failed = 1;
    egui_api_log("  ASSERT FAILED: %s:%d: %s expected=%d actual=%d\n", file, line, expr, expected, actual);
}

int egui_test_region_is_same(const egui_region_t *a, const egui_region_t *b)
{
    return egui_region_equal(a, b);
}

void egui_test_fail_region(const char *file, int line, const char *expr, const egui_region_t *expected, const egui_region_t *actual)
{
    g_egui_test_state.current_test_failed = 1;
    egui_api_log("  ASSERT FAILED: %s:%d: %s\n"
                 "    expected: (%d,%d,%d,%d)\n"
                 "    actual:   (%d,%d,%d,%d)\n",
                 file, line, expr, expected->location.x, expected->location.y, expected->size.width, expected->size.height, actual->location.x,
                 actual->location.y, actual->size.width, actual->size.height);
}

int egui_test_get_result(void)
{
    return (g_egui_test_state.total_failed > 0) ? 1 : 0;
}

void egui_test_print_summary(void)
{
    egui_api_log("\n========================================\n");
    egui_api_log("TEST SUMMARY\n");
    if (g_egui_test_state.suite_filter != NULL && g_egui_test_state.suite_filter[0] != '\0')
    {
        egui_api_log("  Suite filter: %s\n", g_egui_test_state.suite_filter);
    }
    egui_api_log("  Suites: %d\n", g_egui_test_state.total_suites);
    egui_api_log("  Tests:  %d total, %d passed, %d failed\n", g_egui_test_state.total_tests, g_egui_test_state.total_passed, g_egui_test_state.total_failed);
    egui_api_log("  Result: %s\n", g_egui_test_state.total_failed == 0 ? "ALL PASSED" : "FAILED");
    egui_api_log("========================================\n");
}
