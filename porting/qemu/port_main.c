#include <stdint.h>

#include "egui.h"
#include "core/egui_api.h"
#include "core/egui_input.h"
#include "core/egui_input_simulator.h"
#include "uicode.h"

extern void egui_port_init(void);
extern void qemu_exit(int code);
extern void qemu_systick_init(void);
extern void qemu_heap_reset_stats(void);
extern uint32_t qemu_heap_get_alloc_count(void);
extern uint32_t qemu_heap_get_current_bytes(void);
extern uint32_t qemu_heap_get_free_count(void);
extern uint32_t qemu_heap_get_peak_bytes(void);
extern void qemu_log_printf(const char *format, ...);
extern void qemu_log_write(const char *str);

static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_BUFFER_COUNT][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

#ifndef QEMU_HEAP_MEASURE
#define QEMU_HEAP_MEASURE 0
#endif

#ifndef QEMU_HEAP_IDLE_WAIT_MS
#define QEMU_HEAP_IDLE_WAIT_MS 1200U
#endif

#ifndef QEMU_HEAP_SETTLE_WAIT_MS
#define QEMU_HEAP_SETTLE_WAIT_MS 500U
#endif

#ifndef QEMU_HEAP_TOUCH_STEP_MS
#define QEMU_HEAP_TOUCH_STEP_MS 40U
#endif

#ifndef QEMU_HEAP_DRAG_STEP_MS
#define QEMU_HEAP_DRAG_STEP_MS 50U
#endif

#ifndef QEMU_HEAP_ACTIONS_SHOWCASE_COMMON
#define QEMU_HEAP_ACTIONS_SHOWCASE_COMMON 0
#endif

#ifndef QEMU_HEAP_ACTIONS_APP_RECORDING
#define QEMU_HEAP_ACTIONS_APP_RECORDING 0
#endif

#ifndef QEMU_HEAP_TRACE_ACTIONS
#define QEMU_HEAP_TRACE_ACTIONS 0
#endif

#if !QEMU_HEAP_MEASURE
/* Flag set by benchmark code when all tests are done */
volatile int g_qemu_perf_complete = 0;

void qemu_notify_perf_complete(void)
{
    g_qemu_perf_complete = 1;
}
#else
void qemu_notify_perf_complete(void)
{
}

typedef struct qemu_heap_stats
{
    uint32_t current;
    uint32_t peak;
    uint32_t allocs;
    uint32_t frees;
} qemu_heap_stats_t;

static const egui_sim_action_t s_showcase_common_actions[] = {
        EGUI_SIM_WAIT(600),
        EGUI_SIM_CLICK(1218, 34, 350),
        EGUI_SIM_CLICK(1148, 34, 350),
        EGUI_SIM_CLICK(267, 44, 300),
        EGUI_SIM_CLICK(755, 224, 300),
        EGUI_SIM_DRAG(710, 886, 710, 804, 10, 350),
        EGUI_SIM_CLICK(91, 535, 350),
        EGUI_SIM_CLICK(91, 595, 250),
        EGUI_SIM_DRAG(124, 376, 56, 376, 8, 350),
        EGUI_SIM_WAIT(500),
        EGUI_SIM_END(),
};

#if EGUI_CONFIG_RECORDING_TEST
__EGUI_WEAK__ bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    EGUI_UNUSED(action_index);
    EGUI_UNUSED(p_action);
    return false;
}
#endif

static qemu_heap_stats_t qemu_heap_capture_stats(void)
{
    qemu_heap_stats_t stats = {
            .current = qemu_heap_get_current_bytes(),
            .peak = qemu_heap_get_peak_bytes(),
            .allocs = qemu_heap_get_alloc_count(),
            .frees = qemu_heap_get_free_count(),
    };

    return stats;
}

static void qemu_heap_print_metric(const char *key, uint32_t value)
{
    qemu_log_printf("HEAP_RESULT:%s=%lu\n", key, (unsigned long)value);
}

static void qemu_run_for_ms(uint32_t wait_ms)
{
    uint32_t start = egui_api_timer_get_current();

    while ((uint32_t)(egui_api_timer_get_current() - start) < wait_ms)
    {
        egui_polling_work();
    }
}

static void qemu_send_touch(uint8_t type, int x, int y)
{
    egui_input_add_motion(type, (egui_dim_t)x, (egui_dim_t)y);
    egui_polling_work();
}

static void qemu_execute_action(const egui_sim_action_t *action)
{
    int steps;
    int step;
    int x;
    int y;

    switch (action->type)
    {
    case EGUI_SIM_ACTION_CLICK:
        qemu_send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, action->x1, action->y1);
        qemu_run_for_ms(QEMU_HEAP_TOUCH_STEP_MS);
        qemu_send_touch(EGUI_MOTION_EVENT_ACTION_UP, action->x1, action->y1);
        qemu_run_for_ms(QEMU_HEAP_TOUCH_STEP_MS);
        break;

    case EGUI_SIM_ACTION_DRAG:
    case EGUI_SIM_ACTION_SWIPE:
        steps = action->steps > 0 ? action->steps : (action->type == EGUI_SIM_ACTION_SWIPE ? 5 : 10);
        qemu_send_touch(EGUI_MOTION_EVENT_ACTION_DOWN, action->x1, action->y1);
        qemu_run_for_ms(QEMU_HEAP_TOUCH_STEP_MS);
        for (step = 1; step <= steps; ++step)
        {
            x = action->x1 + (action->x2 - action->x1) * step / steps;
            y = action->y1 + (action->y2 - action->y1) * step / steps;
            qemu_send_touch(EGUI_MOTION_EVENT_ACTION_MOVE, x, y);
            qemu_run_for_ms(QEMU_HEAP_DRAG_STEP_MS);
        }
        qemu_send_touch(EGUI_MOTION_EVENT_ACTION_UP, action->x2, action->y2);
        qemu_run_for_ms(QEMU_HEAP_TOUCH_STEP_MS);
        break;

    case EGUI_SIM_ACTION_WAIT:
    case EGUI_SIM_ACTION_NONE:
    default:
        break;
    }
}

static int qemu_get_heap_measure_action(int action_index, egui_sim_action_t *action)
{
#if QEMU_HEAP_ACTIONS_APP_RECORDING && EGUI_CONFIG_RECORDING_TEST
    return egui_port_get_recording_action(action_index, action) ? 1 : 0;
#elif QEMU_HEAP_ACTIONS_SHOWCASE_COMMON
    if (action_index < 0)
    {
        return 0;
    }

    *action = s_showcase_common_actions[action_index];
    return action->type != EGUI_SIM_ACTION_NONE;
#else
    EGUI_UNUSED(action_index);
    EGUI_UNUSED(action);
    return 0;
#endif
}

static uint32_t qemu_run_heap_measure_actions(void)
{
    uint32_t action_count = 0U;
    egui_sim_action_t action;

    while (qemu_get_heap_measure_action((int)action_count, &action))
    {
        qemu_run_for_ms((uint32_t)action.interval_ms);
        qemu_execute_action(&action);
#if QEMU_HEAP_TRACE_ACTIONS
        {
            qemu_heap_stats_t action_stats = qemu_heap_capture_stats();
            qemu_log_printf("HEAP_ACTION:%lu:current=%lu:peak=%lu:allocs=%lu:frees=%lu\n",
                            (unsigned long)action_count,
                            (unsigned long)action_stats.current,
                            (unsigned long)action_stats.peak,
                            (unsigned long)action_stats.allocs,
                            (unsigned long)action_stats.frees);
        }
#endif
        action_count++;
    }

    return action_count;
}
#endif

int main(void)
{
    /* Initialize SysTick for millisecond timing */
    qemu_systick_init();

    /* Initialize EGUI */
    egui_port_init();
    egui_init(egui_pfb);

#if QEMU_HEAP_MEASURE
    qemu_heap_stats_t idle_stats;
    qemu_heap_stats_t interaction_stats = {0};
    uint32_t action_count = 0U;
    uint32_t interaction_total_current;
    uint32_t interaction_total_peak;

    qemu_log_write("QEMU EGUI Heap Measure\n");

    qemu_heap_reset_stats();
    uicode_create_ui();
    egui_screen_on();

    qemu_run_for_ms(QEMU_HEAP_IDLE_WAIT_MS);
    idle_stats = qemu_heap_capture_stats();

    qemu_heap_reset_stats();
    action_count = qemu_run_heap_measure_actions();
    if (action_count > 0U)
    {
        qemu_run_for_ms(QEMU_HEAP_SETTLE_WAIT_MS);
        interaction_stats = qemu_heap_capture_stats();
    }

    interaction_total_current = idle_stats.current + interaction_stats.current;
    interaction_total_peak = idle_stats.current + interaction_stats.peak;

    qemu_heap_print_metric("idle_current", idle_stats.current);
    qemu_heap_print_metric("idle_peak", idle_stats.peak);
    qemu_heap_print_metric("idle_allocs", idle_stats.allocs);
    qemu_heap_print_metric("idle_frees", idle_stats.frees);
    qemu_heap_print_metric("interaction_action_count", action_count);
    qemu_heap_print_metric("interaction_delta_current", interaction_stats.current);
    qemu_heap_print_metric("interaction_delta_peak", interaction_stats.peak);
    qemu_heap_print_metric("interaction_delta_allocs", interaction_stats.allocs);
    qemu_heap_print_metric("interaction_delta_frees", interaction_stats.frees);
    qemu_heap_print_metric("interaction_total_current", interaction_total_current);
    qemu_heap_print_metric("interaction_total_peak", interaction_total_peak);
    qemu_log_write("HEAP_EXIT\n");
    qemu_exit(0);
#else
    qemu_log_write("QEMU EGUI Performance Benchmark\n");

    /* Create UI and run benchmark */
    uicode_create_ui();

    /* Turn on screen - this resumes the core and enables rendering */
    egui_screen_on();

    /* Run polling loop until benchmark completes.
     * HelloPerformance uses a timer callback that runs
     * all test modes then sets g_qemu_perf_complete. */
    while (!g_qemu_perf_complete)
    {
        egui_polling_work();
    }

    qemu_log_write("PERF_EXIT\n");
    qemu_exit(0);
#endif

    return 0;
}
