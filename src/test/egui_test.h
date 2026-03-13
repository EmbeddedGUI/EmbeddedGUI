#ifndef _EGUI_TEST_H_
#define _EGUI_TEST_H_

#include <stdint.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Test State
// ============================================================================
typedef struct egui_test_state
{
    int total_suites;
    int total_tests;
    int total_passed;
    int total_failed;
    int current_suite_tests;
    int current_suite_passed;
    int current_suite_failed;
    int current_test_failed;
    const char *current_suite_name;
    const char *current_test_name;
} egui_test_state_t;

extern egui_test_state_t g_egui_test_state;

// ============================================================================
// Test Suite / Test Case Macros
// ============================================================================

#define EGUI_TEST_SUITE_BEGIN(_name)                                                                                                                           \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        egui_test_suite_begin(#_name);                                                                                                                         \
    } while (0)

#define EGUI_TEST_SUITE_END()                                                                                                                                  \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        egui_test_suite_end();                                                                                                                                 \
    } while (0)

#define EGUI_TEST_RUN(_test_func)                                                                                                                              \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        egui_test_case_begin(#_test_func);                                                                                                                     \
        _test_func();                                                                                                                                          \
        egui_test_case_end();                                                                                                                                  \
    } while (0)

// ============================================================================
// Assertion Macros
// On failure: log error, mark test as failed, return from test function.
// ============================================================================

#define EGUI_TEST_ASSERT_TRUE(_expr)                                                                                                                           \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (!(_expr))                                                                                                                                          \
        {                                                                                                                                                      \
            egui_test_fail(__FILE__, __LINE__, #_expr, "expected true");                                                                                       \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
    } while (0)

#define EGUI_TEST_ASSERT_FALSE(_expr)                                                                                                                          \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if ((_expr))                                                                                                                                           \
        {                                                                                                                                                      \
            egui_test_fail(__FILE__, __LINE__, #_expr, "expected false");                                                                                      \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
    } while (0)

#define EGUI_TEST_ASSERT_EQUAL_INT(_expected, _actual)                                                                                                         \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        int _e = (_expected);                                                                                                                                  \
        int _a = (_actual);                                                                                                                                    \
        if (_e != _a)                                                                                                                                          \
        {                                                                                                                                                      \
            egui_test_fail_int(__FILE__, __LINE__, #_actual, _e, _a);                                                                                          \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
    } while (0)

#define EGUI_TEST_ASSERT_NOT_NULL(_ptr)                                                                                                                        \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if ((_ptr) == NULL)                                                                                                                                    \
        {                                                                                                                                                      \
            egui_test_fail(__FILE__, __LINE__, #_ptr, "expected non-NULL");                                                                                    \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
    } while (0)

#define EGUI_TEST_ASSERT_NULL(_ptr)                                                                                                                            \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if ((_ptr) != NULL)                                                                                                                                    \
        {                                                                                                                                                      \
            egui_test_fail(__FILE__, __LINE__, #_ptr, "expected NULL");                                                                                        \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
    } while (0)

#define EGUI_TEST_ASSERT_REGION_EQUAL(_expected, _actual)                                                                                                      \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (!egui_test_region_is_same((_expected), (_actual)))                                                                                                 \
        {                                                                                                                                                      \
            egui_test_fail_region(__FILE__, __LINE__, #_actual, (_expected), (_actual));                                                                       \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
    } while (0)

// ============================================================================
// Framework Functions (called by macros, not directly by test code)
// ============================================================================
void egui_test_init(void);
void egui_test_suite_begin(const char *name);
void egui_test_suite_end(void);
void egui_test_case_begin(const char *name);
void egui_test_case_end(void);
void egui_test_fail(const char *file, int line, const char *expr, const char *msg);
void egui_test_fail_int(const char *file, int line, const char *expr, int expected, int actual);

int egui_test_region_is_same(const egui_region_t *a, const egui_region_t *b);
void egui_test_fail_region(const char *file, int line, const char *expr, const egui_region_t *expected, const egui_region_t *actual);

int egui_test_get_result(void);
void egui_test_print_summary(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TEST_H_ */
