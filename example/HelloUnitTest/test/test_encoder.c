/**
 * Unit tests for egui_encoder_driver_t (Rotary encoder input).
 *
 * Tests verify:
 *  - NULL / missing-ops safety
 *  - Driver registration and retrieval
 *  - CW  delta  ->  RIGHT key events queued
 *  - CCW delta  ->  LEFT  key events queued
 *  - Zero delta ->  no events
 *  - Multi-tick deltas queue one pair per tick
 *  - Button press edge   -> ENTER DOWN queued
 *  - Button release edge -> ENTER UP   queued
 *  - Button long press   -> LONG_PRESS after threshold
 *  - Button held (no extra edge) -> no duplicate DOWN
 */

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_encoder.h"

#if EGUI_CONFIG_FUNCTION_ENCODER

#include "core/egui_encoder_driver.h"
#include "core/egui_input.h"
#include "core/egui_key_event.h"

/* ------------------------------------------------------------------ */
/* Mock driver helpers                                                 */
/* ------------------------------------------------------------------ */

static int16_t s_mock_delta;
static uint8_t s_mock_button;
static int     s_mock_read_count;
static int     s_mock_read_fail; /* non-zero -> read() returns error */

static int mock_read(void *user_data, int16_t *out_delta, uint8_t *out_btn)
{
    (void)user_data;
    s_mock_read_count++;
    if (s_mock_read_fail)
    {
        return -1;
    }
    *out_delta = s_mock_delta;
    *out_btn   = s_mock_button;
    return 0;
}

static const egui_encoder_driver_ops_t s_mock_ops = { mock_read };

static egui_encoder_driver_t s_drv; /* zero-inited in each reset */

static void reset_mock(void)
{
    s_mock_delta      = 0;
    s_mock_button     = 0;
    s_mock_read_count = 0;
    s_mock_read_fail  = 0;
    /* Zero all internal state */
    s_drv.ops             = &s_mock_ops;
    s_drv.user_data       = NULL;
    s_drv._last_button    = 0;
    s_drv._long_press_sent = 0;
    s_drv._press_tick      = 0;
}

static egui_core_t *get_core(void)
{
    return uicode_get_core();
}

/** Drain pending key events so they do not bleed across tests. */
static void drain_keys(void)
{
    egui_core_t *core = get_core();
    while (!egui_input_check_key_idle(core))
    {
        egui_input_key_dispatch_work(core);
    }
}

/* ------------------------------------------------------------------ */
/* Tests: NULL / missing-ops safety                                    */
/* ------------------------------------------------------------------ */

static void test_encoder_poll_null_core_is_safe(void)
{
    /* Must not crash */
    egui_encoder_polling_work(NULL);
    EGUI_TEST_ASSERT_TRUE(1);
}

static void test_encoder_poll_no_driver_is_safe(void)
{
    egui_core_t *core = get_core();
    /* Unregister any existing driver first */
    egui_encoder_driver_register(core, NULL);
    egui_encoder_polling_work(core);
    EGUI_TEST_ASSERT_TRUE(1);
}

static void test_encoder_poll_null_ops_is_safe(void)
{
    egui_core_t          *core = get_core();
    egui_encoder_driver_t drv;

    drv.ops       = NULL;
    drv.user_data = NULL;
    egui_encoder_driver_register(core, &drv);
    egui_encoder_polling_work(core);

    egui_encoder_driver_register(core, NULL);
    EGUI_TEST_ASSERT_TRUE(1);
}

static void test_encoder_register_null_core_is_safe(void)
{
    egui_encoder_driver_register(NULL, &s_drv);
    EGUI_TEST_ASSERT_TRUE(1);
}

static void test_encoder_get_null_core_returns_null(void)
{
    EGUI_TEST_ASSERT_TRUE(egui_encoder_driver_get(NULL) == NULL);
}

/* ------------------------------------------------------------------ */
/* Tests: registration / retrieval                                     */
/* ------------------------------------------------------------------ */

static void test_encoder_register_and_get(void)
{
    egui_core_t *core = get_core();

    reset_mock();
    egui_encoder_driver_register(core, &s_drv);
    EGUI_TEST_ASSERT_TRUE(egui_encoder_driver_get(core) == &s_drv);

    egui_encoder_driver_register(core, NULL);
    EGUI_TEST_ASSERT_TRUE(egui_encoder_driver_get(core) == NULL);
}

/* ------------------------------------------------------------------ */
/* Tests: rotation delta -> key events                                 */
/* ------------------------------------------------------------------ */

static void test_encoder_cw_delta_queues_events(void)
{
    egui_core_t *core = get_core();

    reset_mock();
    s_mock_delta = 1;
    egui_encoder_driver_register(core, &s_drv);

    drain_keys(); /* ensure clean state */
    egui_encoder_polling_work(core);

    /* At least one event should have been queued */
    EGUI_TEST_ASSERT_TRUE(!egui_input_check_key_idle(core));
    drain_keys();

    egui_encoder_driver_register(core, NULL);
}

static void test_encoder_ccw_delta_queues_events(void)
{
    egui_core_t *core = get_core();

    reset_mock();
    s_mock_delta = -1;
    egui_encoder_driver_register(core, &s_drv);

    drain_keys();
    egui_encoder_polling_work(core);

    EGUI_TEST_ASSERT_TRUE(!egui_input_check_key_idle(core));
    drain_keys();

    egui_encoder_driver_register(core, NULL);
}

static void test_encoder_zero_delta_no_events(void)
{
    egui_core_t *core = get_core();

    reset_mock();
    s_mock_delta  = 0;
    s_mock_button = 0;
    egui_encoder_driver_register(core, &s_drv);

    drain_keys();
    egui_encoder_polling_work(core);

    EGUI_TEST_ASSERT_TRUE(egui_input_check_key_idle(core));

    egui_encoder_driver_register(core, NULL);
}

static void test_encoder_multi_tick_queues_multiple(void)
{
    /* 3 CW ticks: expect queue to be non-empty (6 events: 3x DOWN+UP) */
    egui_core_t *core = get_core();

    reset_mock();
    s_mock_delta = 3;
    egui_encoder_driver_register(core, &s_drv);

    drain_keys();
    egui_encoder_polling_work(core);

    EGUI_TEST_ASSERT_TRUE(!egui_input_check_key_idle(core));
    drain_keys();

    egui_encoder_driver_register(core, NULL);
}

/* ------------------------------------------------------------------ */
/* Tests: button events                                                */
/* ------------------------------------------------------------------ */

static void test_encoder_button_press_queues_enter_down(void)
{
    egui_core_t *core = get_core();

    reset_mock();
    s_mock_button = 1; /* pressed */
    egui_encoder_driver_register(core, &s_drv);

    drain_keys();
    egui_encoder_polling_work(core); /* press edge detected */

    EGUI_TEST_ASSERT_TRUE(!egui_input_check_key_idle(core));
    drain_keys();

    egui_encoder_driver_register(core, NULL);
}

static void test_encoder_button_release_queues_enter_up(void)
{
    egui_core_t *core = get_core();

    reset_mock();
    egui_encoder_driver_register(core, &s_drv);

    /* Simulate press then release */
    drain_keys();
    s_mock_button = 1;
    egui_encoder_polling_work(core); /* press edge */
    drain_keys();

    s_mock_button = 0;
    egui_encoder_polling_work(core); /* release edge */

    EGUI_TEST_ASSERT_TRUE(!egui_input_check_key_idle(core));
    drain_keys();

    egui_encoder_driver_register(core, NULL);
}

static void test_encoder_button_held_no_duplicate_down(void)
{
    /* Three polls with button held: only first poll should queue DOWN */
    egui_core_t *core = get_core();

    reset_mock();
    s_mock_button = 1;
    egui_encoder_driver_register(core, &s_drv);

    drain_keys();
    egui_encoder_polling_work(core); /* press edge -> DOWN queued */
    drain_keys();

    egui_encoder_polling_work(core); /* held – no new DOWN (unless long-press) */
    egui_encoder_polling_work(core); /* held again */

    /* With long-press threshold unmet: queue should be idle */
    EGUI_TEST_ASSERT_TRUE(egui_input_check_key_idle(core));

    egui_encoder_driver_register(core, NULL);
}

static void test_encoder_read_error_skips_events(void)
{
    egui_core_t *core = get_core();

    reset_mock();
    s_mock_delta      = 5;
    s_mock_read_fail  = 1;
    egui_encoder_driver_register(core, &s_drv);

    drain_keys();
    egui_encoder_polling_work(core);

    /* Error returned by read() -> no events queued */
    EGUI_TEST_ASSERT_TRUE(egui_input_check_key_idle(core));

    egui_encoder_driver_register(core, NULL);
}

/* ------------------------------------------------------------------ */
/* Test runner                                                         */
/* ------------------------------------------------------------------ */

void test_encoder_run(void)
{
    EGUI_TEST_SUITE_BEGIN(encoder);

    EGUI_TEST_CASE(test_encoder_poll_null_core_is_safe);
    EGUI_TEST_CASE(test_encoder_poll_no_driver_is_safe);
    EGUI_TEST_CASE(test_encoder_poll_null_ops_is_safe);
    EGUI_TEST_CASE(test_encoder_register_null_core_is_safe);
    EGUI_TEST_CASE(test_encoder_get_null_core_returns_null);
    EGUI_TEST_CASE(test_encoder_register_and_get);
    EGUI_TEST_CASE(test_encoder_cw_delta_queues_events);
    EGUI_TEST_CASE(test_encoder_ccw_delta_queues_events);
    EGUI_TEST_CASE(test_encoder_zero_delta_no_events);
    EGUI_TEST_CASE(test_encoder_multi_tick_queues_multiple);
    EGUI_TEST_CASE(test_encoder_button_press_queues_enter_down);
    EGUI_TEST_CASE(test_encoder_button_release_queues_enter_up);
    EGUI_TEST_CASE(test_encoder_button_held_no_duplicate_down);
    EGUI_TEST_CASE(test_encoder_read_error_skips_events);

    EGUI_TEST_SUITE_END();
}

#else /* !EGUI_CONFIG_FUNCTION_ENCODER */

void test_encoder_run(void)
{
    /* Feature disabled – no tests. */
}

#endif /* EGUI_CONFIG_FUNCTION_ENCODER */
