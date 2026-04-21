#include "uicode_disp0.h"
#include "uicode_disp1.h"

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
#include "core/egui_input_simulator.h"
#if EGUI_PORT == EGUI_PORT_TYPE_PC
#include "sdl_port.h"
#endif

#ifndef EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_BURST_COUNT
#define EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_BURST_COUNT 0
#endif

#ifndef EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_DELAY_MS
#define EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_DELAY_MS 2
#endif

#ifndef EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_POST_GAP_MS
#define EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_POST_GAP_MS 0
#endif

#ifndef EGUI_MULTI_DISPLAY_HETERO_SUB_CLICK_SETTLE_WAIT_MS
#define EGUI_MULTI_DISPLAY_HETERO_SUB_CLICK_SETTLE_WAIT_MS 400
#endif

#ifndef EGUI_MULTI_DISPLAY_HETERO_SUB_CLICK_SETTLE_POLL_MS
#define EGUI_MULTI_DISPLAY_HETERO_SUB_CLICK_SETTLE_POLL_MS 20
#endif

#ifndef EGUI_MULTI_DISPLAY_HETERO_STRESS_IDLE_WAIT_MS
#define EGUI_MULTI_DISPLAY_HETERO_STRESS_IDLE_WAIT_MS 200
#endif

#ifndef EGUI_MULTI_DISPLAY_HETERO_STRESS_IDLE_POLL_MS
#define EGUI_MULTI_DISPLAY_HETERO_STRESS_IDLE_POLL_MS 10
#endif

#ifndef EGUI_MULTI_DISPLAY_HETERO_STRESS_IDLE_STABLE_POLLS
#define EGUI_MULTI_DISPLAY_HETERO_STRESS_IDLE_STABLE_POLLS 3
#endif

#ifndef EGUI_MULTI_DISPLAY_HETERO_STRESS_STAGE_SETTLE_MS
#define EGUI_MULTI_DISPLAY_HETERO_STRESS_STAGE_SETTLE_MS 80
#endif

#ifndef EGUI_MULTI_DISPLAY_HETERO_SUB_PAGE_SYNC_WAIT_MS
#define EGUI_MULTI_DISPLAY_HETERO_SUB_PAGE_SYNC_WAIT_MS 400
#endif

#ifndef EGUI_MULTI_DISPLAY_HETERO_SUB_PAGE_SYNC_POLL_MS
#define EGUI_MULTI_DISPLAY_HETERO_SUB_PAGE_SYNC_POLL_MS 20
#endif

static const char *g_recording_frame_label_pending;
static uint8_t g_runtime_fail_reported;
static uint32_t g_tick_before_main_drag_1;
static uint32_t g_tick_before_main_drag_2;
static uint32_t g_tick_before_main_restore;

static void hello_multi_display_hetero_report_runtime_failure(const char *message);

static void multi_display_hetero_request_snapshot(const char *label)
{
    g_recording_frame_label_pending = label;
    recording_request_snapshot();
}

const char *egui_port_get_recording_frame_label(void)
{
    const char *label = g_recording_frame_label_pending;
    g_recording_frame_label_pending = NULL;
    return label;
}

#if EGUI_PORT == EGUI_PORT_TYPE_PC
typedef struct hello_multi_display_hetero_tick_query_request
{
    uint32_t tick_counter;
    int page_index;
    uint8_t ok;
} hello_multi_display_hetero_tick_query_request_t;

static void hello_multi_display_hetero_tick_query_task(egui_core_t *core, uintptr_t user_data)
{
    hello_multi_display_hetero_tick_query_request_t *request = (hello_multi_display_hetero_tick_query_request_t *)user_data;

    EGUI_UNUSED(core);
    if (request == NULL)
    {
        return;
    }

    request->tick_counter = hello_multi_display_hetero_sub_get_tick_counter();
    request->page_index = hello_multi_display_hetero_sub_get_page_index();
    request->ok = 1;
}

static void hello_multi_display_hetero_core_task_stress_task(egui_core_t *core, uintptr_t user_data)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(user_data);
#if EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_DELAY_MS > 0
    SDL_Delay(EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_DELAY_MS);
#endif
}

static void hello_multi_display_hetero_append_reject_snapshot(char *message, size_t message_size, const egui_port_core_task_queue_metrics_t *metrics)
{
    if (message == NULL || message_size == 0 || metrics == NULL)
    {
        return;
    }
    if (metrics->post_reject_count == 0)
    {
        return;
    }

    snprintf(message + strlen(message), message_size - strlen(message), " reject_snapshot=pending=%u inflight=%u",
             (unsigned int)metrics->last_reject_pending_count, (unsigned int)metrics->last_reject_inflight_count);
    if (metrics->last_reject_context != NULL)
    {
        snprintf(message + strlen(message), message_size - strlen(message), "(%s)", metrics->last_reject_context);
    }
    if (metrics->last_reject_inflight_context != NULL)
    {
        snprintf(message + strlen(message), message_size - strlen(message), " reject_inflight_ctx=%s", metrics->last_reject_inflight_context);
    }
    if (metrics->last_reject_pending_context != NULL)
    {
        snprintf(message + strlen(message), message_size - strlen(message), " reject_pending_ctx=%s", metrics->last_reject_pending_context);
    }
}

static void hello_multi_display_hetero_format_stress_burst_failure(char *message, size_t message_size, const char *stage_label, int display_id, int index,
                                                                   int total)
{
    egui_core_t *core = egui_port_get_core_by_display_id(display_id);
    egui_port_core_task_queue_metrics_t metrics;
    const char *stage = stage_label != NULL ? stage_label : "unknown";

    if (message == NULL || message_size == 0)
    {
        return;
    }

    if (core != NULL && egui_port_get_core_task_queue_metrics(core, &metrics))
    {
        snprintf(message, message_size,
                 "failed to post hetero stress burst task on display %d during %s at %d/%d: pending=%u peak=%u/%u retries=%lu rejected=%lu max_retry_burst=%lu",
                 display_id, stage, index, total, (unsigned int)metrics.pending_count, (unsigned int)metrics.peak_count, (unsigned int)metrics.queue_capacity,
                 (unsigned long)metrics.post_retry_count, (unsigned long)metrics.post_reject_count, (unsigned long)metrics.post_max_retry_burst);
        if (metrics.inflight_count > 0)
        {
            snprintf(message + strlen(message), message_size - strlen(message), " inflight=%u(%s)", (unsigned int)metrics.inflight_count,
                     metrics.inflight_context != NULL ? metrics.inflight_context : "unknown");
        }
        if (metrics.pending_context != NULL)
        {
            snprintf(message + strlen(message), message_size - strlen(message), " pending_ctx=%s", metrics.pending_context);
        }
        hello_multi_display_hetero_append_reject_snapshot(message, message_size, &metrics);
        return;
    }

    snprintf(message, message_size, "failed to post hetero stress burst task on display %d during %s at %d/%d", display_id, stage, index, total);
}

static void hello_multi_display_hetero_post_core_task_stress_burst(const char *stage_label, int display_id)
{
#if EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_BURST_COUNT > 0
    egui_core_t *core = egui_port_get_core_by_display_id(display_id);
    char message[320];

    if (core == NULL)
    {
        hello_multi_display_hetero_report_runtime_failure("failed to resolve hetero display core for stress burst");
        return;
    }

    for (int i = 0; i < EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_BURST_COUNT; i++)
    {
        if (!egui_port_post_core_task_named(core, hello_multi_display_hetero_core_task_stress_task, 0,
                                            stage_label != NULL ? stage_label : "hetero_stress_burst"))
        {
            hello_multi_display_hetero_format_stress_burst_failure(message, sizeof(message), stage_label, display_id, i + 1,
                                                                   EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_BURST_COUNT);
            hello_multi_display_hetero_report_runtime_failure(message);
            break;
        }
#if EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_POST_GAP_MS > 0
        if (i + 1 < EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_BURST_COUNT)
        {
            SDL_Delay(EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_POST_GAP_MS);
        }
#endif
    }
#else
    EGUI_UNUSED(stage_label);
    EGUI_UNUSED(display_id);
#endif
}
#endif

static void hello_multi_display_hetero_report_runtime_failure(const char *message)
{
    if (g_runtime_fail_reported)
    {
        return;
    }

    g_runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

static uint8_t hello_multi_display_hetero_set_click_action(int display_id, egui_view_t *view, int interval_ms, egui_sim_action_t *p_action)
{
    if (p_action == NULL || view == NULL)
    {
        return 0;
    }

#if EGUI_PORT == EGUI_PORT_TYPE_PC
    {
        egui_core_t *core = egui_port_get_core_by_display_id(display_id);

        if (core == NULL || !egui_port_get_view_center(core, view, &p_action->x1, &p_action->y1))
        {
            return 0;
        }
    }
#else
    EGUI_SIM_SET_CLICK_VIEW(p_action, view, interval_ms);
#endif

    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->interval_ms = interval_ms;
    p_action->display_id = display_id;
    return 1;
}

static uint8_t hello_multi_display_hetero_query_tick_counter(uint32_t *tick_counter)
{
#if EGUI_PORT == EGUI_PORT_TYPE_PC
    egui_core_t *sub_core = egui_port_get_core_by_display_id(1);
    hello_multi_display_hetero_tick_query_request_t request;

    if (tick_counter == NULL || sub_core == NULL)
    {
        return 0;
    }

    request.tick_counter = 0;
    request.ok = 0;
    if (!egui_port_post_core_task_sync_named(sub_core, hello_multi_display_hetero_tick_query_task, (uintptr_t)&request, 2000, "hetero_tick_query"))
    {
        return 0;
    }
    if (!request.ok)
    {
        return 0;
    }

    *tick_counter = request.tick_counter;
    return 1;
#else
    if (tick_counter == NULL)
    {
        return 0;
    }

    *tick_counter = hello_multi_display_hetero_sub_get_tick_counter();
    return 1;
#endif
}

static uint8_t hello_multi_display_hetero_query_page_index(int *page_index)
{
#if EGUI_PORT == EGUI_PORT_TYPE_PC
    egui_core_t *sub_core = egui_port_get_core_by_display_id(1);
    hello_multi_display_hetero_tick_query_request_t request;

    if (page_index == NULL || sub_core == NULL)
    {
        return 0;
    }

    request.tick_counter = 0;
    request.page_index = -1;
    request.ok = 0;
    if (!egui_port_post_core_task_sync_named(sub_core, hello_multi_display_hetero_tick_query_task, (uintptr_t)&request, 2000, "hetero_page_query"))
    {
        return 0;
    }
    if (!request.ok)
    {
        return 0;
    }

    *page_index = request.page_index;
    return 1;
#else
    if (page_index == NULL)
    {
        return 0;
    }

    *page_index = hello_multi_display_hetero_sub_get_page_index();
    return 1;
#endif
}

static uint8_t hello_multi_display_hetero_wait_for_sub_click_reset(uint32_t *tick_counter)
{
    uint32_t observed_tick = 0;

    if (tick_counter == NULL)
    {
        return 0;
    }

    if (!hello_multi_display_hetero_query_tick_counter(&observed_tick))
    {
        return 0;
    }

#if EGUI_PORT == EGUI_PORT_TYPE_PC
    for (uint32_t waited_ms = 0; observed_tick != 0 && waited_ms < EGUI_MULTI_DISPLAY_HETERO_SUB_CLICK_SETTLE_WAIT_MS;
         waited_ms += EGUI_MULTI_DISPLAY_HETERO_SUB_CLICK_SETTLE_POLL_MS)
    {
        SDL_Delay(EGUI_MULTI_DISPLAY_HETERO_SUB_CLICK_SETTLE_POLL_MS);
        if (!hello_multi_display_hetero_query_tick_counter(&observed_tick))
        {
            return 0;
        }
    }
#endif

    *tick_counter = observed_tick;
    return 1;
}

static uint8_t hello_multi_display_hetero_wait_for_sub_page_sync(const char *stage_label, int expected_page_index)
{
    int observed_page_index = -1;

    if (!hello_multi_display_hetero_query_page_index(&observed_page_index))
    {
        return 0;
    }

#if EGUI_PORT == EGUI_PORT_TYPE_PC
    for (uint32_t waited_ms = 0; observed_page_index != expected_page_index && waited_ms < EGUI_MULTI_DISPLAY_HETERO_SUB_PAGE_SYNC_WAIT_MS;
         waited_ms += EGUI_MULTI_DISPLAY_HETERO_SUB_PAGE_SYNC_POLL_MS)
    {
        SDL_Delay(EGUI_MULTI_DISPLAY_HETERO_SUB_PAGE_SYNC_POLL_MS);
        if (!hello_multi_display_hetero_query_page_index(&observed_page_index))
        {
            return 0;
        }
    }
#endif

    if (observed_page_index != expected_page_index)
    {
        char message[160];

        snprintf(message, sizeof(message), "sub page sync did not settle before %s: expected=%d actual=%d", stage_label != NULL ? stage_label : "unknown",
                 expected_page_index, observed_page_index);
        hello_multi_display_hetero_report_runtime_failure(message);
        return 0;
    }

    return 1;
}

static uint8_t hello_multi_display_hetero_wait_for_idle_queue(const char *stage_label, int display_id)
{
#if EGUI_PORT == EGUI_PORT_TYPE_PC
    egui_core_t *core = egui_port_get_core_by_display_id(display_id);
    egui_port_core_task_queue_metrics_t metrics;
    char message[320];
    uint32_t idle_polls = 0;
    const char *stage = stage_label != NULL ? stage_label : "unknown";

    if (core == NULL)
    {
        hello_multi_display_hetero_report_runtime_failure("failed to resolve hetero display core for idle wait");
        return 0;
    }

    for (uint32_t waited_ms = 0;; waited_ms += EGUI_MULTI_DISPLAY_HETERO_STRESS_IDLE_POLL_MS)
    {
        if (!egui_port_get_core_task_queue_metrics(core, &metrics))
        {
            hello_multi_display_hetero_report_runtime_failure("failed to query hetero core task queue metrics before stress burst");
            return 0;
        }
        if (metrics.pending_count == 0 && metrics.inflight_count == 0)
        {
            idle_polls++;
            if (idle_polls >= EGUI_MULTI_DISPLAY_HETERO_STRESS_IDLE_STABLE_POLLS)
            {
                return 1;
            }
        }
        else
        {
            idle_polls = 0;
        }
        if (waited_ms >= EGUI_MULTI_DISPLAY_HETERO_STRESS_IDLE_WAIT_MS)
        {
            snprintf(message, sizeof(message),
                     "hetero core task queue did not drain before %s on display %d: pending=%u peak=%u/%u retries=%lu rejected=%lu max_retry_burst=%lu", stage,
                     display_id, (unsigned int)metrics.pending_count, (unsigned int)metrics.peak_count, (unsigned int)metrics.queue_capacity,
                     (unsigned long)metrics.post_retry_count, (unsigned long)metrics.post_reject_count, (unsigned long)metrics.post_max_retry_burst);
            if (metrics.inflight_count > 0)
            {
                snprintf(message + strlen(message), sizeof(message) - strlen(message), " inflight=%u(%s)", (unsigned int)metrics.inflight_count,
                         metrics.inflight_context != NULL ? metrics.inflight_context : "unknown");
            }
            if (metrics.pending_context != NULL)
            {
                snprintf(message + strlen(message), sizeof(message) - strlen(message), " pending_ctx=%s", metrics.pending_context);
            }
            hello_multi_display_hetero_append_reject_snapshot(message, sizeof(message), &metrics);
            hello_multi_display_hetero_report_runtime_failure(message);
            return 0;
        }
        SDL_Delay(EGUI_MULTI_DISPLAY_HETERO_STRESS_IDLE_POLL_MS);
    }
#else
    EGUI_UNUSED(stage_label);
    EGUI_UNUSED(display_id);
    return 1;
#endif
}

static void hello_multi_display_hetero_wait_before_stress_burst(void)
{
#if EGUI_PORT == EGUI_PORT_TYPE_PC && EGUI_MULTI_DISPLAY_HETERO_STRESS_STAGE_SETTLE_MS > 0
    SDL_Delay(EGUI_MULTI_DISPLAY_HETERO_STRESS_STAGE_SETTLE_MS);
#endif
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);
    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            multi_display_hetero_request_snapshot("initial");
        }
        EGUI_SIM_SET_WAIT(p_action, 600);
        return true;

    case 1:
        if (first_call && !hello_multi_display_hetero_query_tick_counter(&g_tick_before_main_drag_1))
        {
            hello_multi_display_hetero_report_runtime_failure("failed to query sub tick before main drag 1");
        }
        p_action->type = EGUI_SIM_ACTION_DRAG;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 15;
        p_action->interval_ms = 800;
        p_action->display_id = 0;
        return true;

    case 2:
        if (first_call)
        {
            uint32_t tick_after_main_drag_1 = 0;

            if (!hello_multi_display_hetero_query_tick_counter(&tick_after_main_drag_1))
            {
                hello_multi_display_hetero_report_runtime_failure("failed to query sub tick after main drag 1");
            }
            else if (tick_after_main_drag_1 <= g_tick_before_main_drag_1)
            {
                hello_multi_display_hetero_report_runtime_failure("sub tick stalled during main drag 1");
            }
            hello_multi_display_hetero_wait_for_sub_page_sync("after_main_drag_1", 1);
            multi_display_hetero_request_snapshot("after_main_drag_1");
        }
        EGUI_SIM_SET_WAIT(p_action, 600);
        return true;

    case 3:
        if (first_call)
        {
            hello_multi_display_hetero_wait_before_stress_burst();
            if (hello_multi_display_hetero_wait_for_idle_queue("after_main_drag_1", 1))
            {
                hello_multi_display_hetero_post_core_task_stress_burst("after_main_drag_1", 1);
            }
        }
        EGUI_SIM_SET_WAIT(p_action, 150);
        return true;

    case 4:
        if (first_call && !hello_multi_display_hetero_query_tick_counter(&g_tick_before_main_drag_2))
        {
            hello_multi_display_hetero_report_runtime_failure("failed to query sub tick before main drag 2");
        }
        p_action->type = EGUI_SIM_ACTION_DRAG;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 15;
        p_action->interval_ms = 800;
        p_action->display_id = 0;
        return true;

    case 5:
        if (first_call)
        {
            uint32_t tick_after_main_drag_2 = 0;

            if (!hello_multi_display_hetero_query_tick_counter(&tick_after_main_drag_2))
            {
                hello_multi_display_hetero_report_runtime_failure("failed to query sub tick after main drag 2");
            }
            else if (tick_after_main_drag_2 <= g_tick_before_main_drag_2)
            {
                hello_multi_display_hetero_report_runtime_failure("sub tick stalled during main drag 2");
            }
            multi_display_hetero_request_snapshot("after_main_drag_2");
        }
        EGUI_SIM_SET_WAIT(p_action, 600);
        return true;

    case 6:
        if (!hello_multi_display_hetero_set_click_action(1, hello_multi_display_hetero_sub_get_interaction_target(), 600, p_action))
        {
            hello_multi_display_hetero_report_runtime_failure("failed to resolve sub interaction target center");
            EGUI_SIM_SET_WAIT(p_action, 50);
        }
        return true;

    case 7:
        if (first_call)
        {
            uint32_t tick_after_sub_click = 0;

            if (!hello_multi_display_hetero_wait_for_sub_click_reset(&tick_after_sub_click))
            {
                hello_multi_display_hetero_report_runtime_failure("failed to query sub tick after sub click");
            }
            else if (tick_after_sub_click != 0)
            {
                hello_multi_display_hetero_report_runtime_failure("sub click did not reset tick counter");
            }
            multi_display_hetero_request_snapshot("after_disp1_click");
        }
        EGUI_SIM_SET_WAIT(p_action, 600);
        return true;

    case 8:
        if (first_call)
        {
            hello_multi_display_hetero_wait_before_stress_burst();
            if (hello_multi_display_hetero_wait_for_idle_queue("after_disp1_click", 1))
            {
                hello_multi_display_hetero_post_core_task_stress_burst("after_disp1_click", 1);
            }
        }
        EGUI_SIM_SET_WAIT(p_action, 150);
        return true;

    case 9:
        if (first_call && !hello_multi_display_hetero_query_tick_counter(&g_tick_before_main_restore))
        {
            hello_multi_display_hetero_report_runtime_failure("failed to query sub tick before main restore");
        }
        p_action->type = EGUI_SIM_ACTION_DRAG;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 15;
        p_action->interval_ms = 800;
        p_action->display_id = 0;
        return true;

    case 10:
        if (first_call)
        {
            uint32_t tick_after_main_restore = 0;

            if (!hello_multi_display_hetero_query_tick_counter(&tick_after_main_restore))
            {
                hello_multi_display_hetero_report_runtime_failure("failed to query sub tick after main restore");
            }
            else if (tick_after_main_restore <= g_tick_before_main_restore)
            {
                hello_multi_display_hetero_report_runtime_failure("sub tick stalled during main restore");
            }
            multi_display_hetero_request_snapshot("after_main_restore");
        }
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;

    default:
        return false;
    }
}
#endif /* EGUI_CONFIG_FUNCTION_RECORDING_TEST */
