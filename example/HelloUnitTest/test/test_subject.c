#include "egui.h"
#include "test/egui_test.h"
#include "core/egui_subject.h"
#include "test_subject.h"

#if EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER

/* ------------------------------------------------------------------ */
/* Shared callback helpers                                              */
/* ------------------------------------------------------------------ */

/* Accumulated call count for simple tests */
static int s_call_count;
/* Last data pointer received */
static const void *s_last_data;
/* Last user_data pointer received */
static void *s_last_user;

static void cb_counter(egui_subject_t *subject, const void *data, void *user_data)
{
    EGUI_UNUSED(subject);
    s_call_count++;
    s_last_data = data;
    s_last_user = user_data;
}

static void cb_counter_b(egui_subject_t *subject, const void *data, void *user_data)
{
    EGUI_UNUSED(subject);
    EGUI_UNUSED(data);
    EGUI_UNUSED(user_data);
    s_call_count += 10; /* distinctive increment to track which cb fired */
}

/* ------------------------------------------------------------------ */
/* Tests                                                                */
/* ------------------------------------------------------------------ */

static void test_subject_init_defaults(void)
{
    egui_subject_t subj;
    egui_subject_init(&subj);

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_subject_observer_count(&subj));
}

static void test_subject_subscribe_single(void)
{
    egui_subject_t subj;
    egui_observer_t obs;

    egui_subject_init(&subj);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_subject_subscribe(&subj, &obs, cb_counter, NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_subject_observer_count(&subj));
}

static void test_subject_notify_calls_callback(void)
{
    egui_subject_t subj;
    egui_observer_t obs;
    int value = 42;

    s_call_count = 0;
    s_last_data = NULL;
    s_last_user = NULL;

    egui_subject_init(&subj);
    egui_subject_subscribe(&subj, &obs, cb_counter, (void *)&value);
    egui_subject_notify(&subj, &value);

    EGUI_TEST_ASSERT_EQUAL_INT(1, s_call_count);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)(s_last_data == &value));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)(s_last_user == &value));
}

static void test_subject_notify_null_data(void)
{
    egui_subject_t subj;
    egui_observer_t obs;

    s_call_count = 0;
    s_last_data = (const void *)1; /* non-NULL sentinel */

    egui_subject_init(&subj);
    egui_subject_subscribe(&subj, &obs, cb_counter, NULL);
    egui_subject_notify(&subj, NULL);

    EGUI_TEST_ASSERT_EQUAL_INT(1, s_call_count);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)(s_last_data != NULL));
}

static void test_subject_multiple_observers_ordered(void)
{
    egui_subject_t subj;
    egui_observer_t obs_a;
    egui_observer_t obs_b;

    s_call_count = 0;

    egui_subject_init(&subj);
    egui_subject_subscribe(&subj, &obs_a, cb_counter, NULL);
    egui_subject_subscribe(&subj, &obs_b, cb_counter_b, NULL);

    egui_subject_notify(&subj, NULL);

    /* cb_counter adds 1, cb_counter_b adds 10 => total 11 */
    EGUI_TEST_ASSERT_EQUAL_INT(11, s_call_count);
}

static void test_subject_unsubscribe_removes(void)
{
    egui_subject_t subj;
    egui_observer_t obs;

    s_call_count = 0;

    egui_subject_init(&subj);
    egui_subject_subscribe(&subj, &obs, cb_counter, NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_subject_unsubscribe(&subj, &obs));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_subject_observer_count(&subj));

    egui_subject_notify(&subj, NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_call_count);
}

static void test_subject_unsubscribe_missing_returns_error(void)
{
    egui_subject_t subj;
    egui_observer_t obs;

    egui_subject_init(&subj);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_subject_unsubscribe(&subj, &obs));
}

static void test_subject_unsubscribe_compacts(void)
{
    egui_subject_t subj;
    egui_observer_t obs_a;
    egui_observer_t obs_b;
    egui_observer_t obs_c;

    s_call_count = 0;

    egui_subject_init(&subj);
    egui_subject_subscribe(&subj, &obs_a, cb_counter, NULL);
    egui_subject_subscribe(&subj, &obs_b, cb_counter, NULL);
    egui_subject_subscribe(&subj, &obs_c, cb_counter, NULL);

    /* Remove middle observer */
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_subject_unsubscribe(&subj, &obs_b));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_subject_observer_count(&subj));

    /* Both remaining observers should fire */
    egui_subject_notify(&subj, NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(2, s_call_count);
}

static void test_subject_clear_removes_all(void)
{
    egui_subject_t subj;
    egui_observer_t obs_a;
    egui_observer_t obs_b;

    s_call_count = 0;

    egui_subject_init(&subj);
    egui_subject_subscribe(&subj, &obs_a, cb_counter, NULL);
    egui_subject_subscribe(&subj, &obs_b, cb_counter, NULL);

    egui_subject_clear(&subj);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_subject_observer_count(&subj));

    egui_subject_notify(&subj, NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, s_call_count);
}

static void test_subject_duplicate_subscribe_rejected(void)
{
    egui_subject_t subj;
    egui_observer_t obs;

    egui_subject_init(&subj);
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_subject_subscribe(&subj, &obs, cb_counter, NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_subject_subscribe(&subj, &obs, cb_counter, NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_subject_observer_count(&subj));
}

static void test_subject_capacity_limit(void)
{
    egui_subject_t subj;
    egui_observer_t obs[EGUI_CONFIG_SUBJECT_MAX_OBSERVERS + 1];
    int i;

    egui_subject_init(&subj);
    for (i = 0; i < EGUI_CONFIG_SUBJECT_MAX_OBSERVERS; i++)
    {
        EGUI_TEST_ASSERT_EQUAL_INT(0, egui_subject_subscribe(&subj, &obs[i], cb_counter, NULL));
    }
    /* One more should fail */
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_subject_subscribe(&subj, &obs[EGUI_CONFIG_SUBJECT_MAX_OBSERVERS], cb_counter, NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_CONFIG_SUBJECT_MAX_OBSERVERS, (int)egui_subject_observer_count(&subj));
}

static void test_subject_notify_no_observers_is_safe(void)
{
    egui_subject_t subj;
    egui_subject_init(&subj);
    /* Should not crash */
    egui_subject_notify(&subj, NULL);
}

static void test_subject_null_args_safe(void)
{
    egui_subject_t subj;
    egui_observer_t obs;

    egui_subject_init(&subj);

    /* NULL subject */
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_subject_subscribe(NULL, &obs, cb_counter, NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_subject_unsubscribe(NULL, &obs));
    egui_subject_notify(NULL, NULL); /* must not crash */
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_subject_observer_count(NULL));

    /* NULL observer */
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_subject_subscribe(&subj, NULL, cb_counter, NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_subject_unsubscribe(&subj, NULL));

    /* NULL callback */
    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_subject_subscribe(&subj, &obs, NULL, NULL));
}

static void test_subject_resubscribe_after_unsubscribe(void)
{
    egui_subject_t subj;
    egui_observer_t obs;

    s_call_count = 0;

    egui_subject_init(&subj);
    egui_subject_subscribe(&subj, &obs, cb_counter, NULL);
    egui_subject_unsubscribe(&subj, &obs);

    /* Re-subscribe and verify it works */
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_subject_subscribe(&subj, &obs, cb_counter, NULL));
    egui_subject_notify(&subj, NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_call_count);
}

/* ------------------------------------------------------------------ */
/* Runner                                                               */
/* ------------------------------------------------------------------ */

void test_subject_run(void)
{
    EGUI_TEST_SUITE_BEGIN(subject);

    EGUI_TEST_CASE(test_subject_init_defaults);
    EGUI_TEST_CASE(test_subject_subscribe_single);
    EGUI_TEST_CASE(test_subject_notify_calls_callback);
    EGUI_TEST_CASE(test_subject_notify_null_data);
    EGUI_TEST_CASE(test_subject_multiple_observers_ordered);
    EGUI_TEST_CASE(test_subject_unsubscribe_removes);
    EGUI_TEST_CASE(test_subject_unsubscribe_missing_returns_error);
    EGUI_TEST_CASE(test_subject_unsubscribe_compacts);
    EGUI_TEST_CASE(test_subject_clear_removes_all);
    EGUI_TEST_CASE(test_subject_duplicate_subscribe_rejected);
    EGUI_TEST_CASE(test_subject_capacity_limit);
    EGUI_TEST_CASE(test_subject_notify_no_observers_is_safe);
    EGUI_TEST_CASE(test_subject_null_args_safe);
    EGUI_TEST_CASE(test_subject_resubscribe_after_unsubscribe);

    EGUI_TEST_SUITE_END();
}

#else /* EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER */

void test_subject_run(void)
{
    /* Subject-Observer disabled; nothing to run. */
}

#endif /* EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER */
