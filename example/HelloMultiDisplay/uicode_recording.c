#include "uicode_disp0.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#include "uicode_disp1.h"
#include "utils/egui_slist.h"
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

#ifndef EGUI_MULTI_DISPLAY_STRESS_IDLE_WAIT_MS
#define EGUI_MULTI_DISPLAY_STRESS_IDLE_WAIT_MS 200
#endif

#ifndef EGUI_MULTI_DISPLAY_STRESS_IDLE_POLL_MS
#define EGUI_MULTI_DISPLAY_STRESS_IDLE_POLL_MS 10
#endif

#ifndef EGUI_MULTI_DISPLAY_STRESS_IDLE_STABLE_POLLS
#define EGUI_MULTI_DISPLAY_STRESS_IDLE_STABLE_POLLS 3
#endif

#ifndef EGUI_MULTI_DISPLAY_STRESS_STAGE_SETTLE_MS
#define EGUI_MULTI_DISPLAY_STRESS_STAGE_SETTLE_MS 80
#endif

static const char *g_recording_frame_label_pending;
static uint8_t g_runtime_fail_reported;
static int g_disp0_slot_before_click;
static int g_disp1_slot_before_click;
static int g_disp0_slot_before_dual_click;
static int g_disp1_slot_before_dual_click;

static void hello_multi_display_report_runtime_failure(const char *message);

static void multi_display_request_snapshot(const char *label)
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
typedef struct hello_multi_display_slot_query_request
{
    int activity_slot;
    int active_anim_count;
    uint8_t ok;
} hello_multi_display_slot_query_request_t;

static void hello_multi_display_slot_query_task(egui_core_t *core, uintptr_t user_data)
{
    hello_multi_display_slot_query_request_t *request = (hello_multi_display_slot_query_request_t *)user_data;

    if (request == NULL)
    {
        return;
    }

    if (core != NULL)
    {
        request->active_anim_count = egui_slist_size(&core->scene.anims);
    }

    if (core != NULL && core->id == 0)
    {
        request->activity_slot = hello_multi_display_disp0_get_activity_slot_count();
        request->ok = 1;
    }
    else if (core != NULL && core->id == 1)
    {
        request->activity_slot = hello_multi_display_disp1_get_activity_slot_count();
        request->ok = 1;
    }
}

static void hello_multi_display_core_task_stress_task(egui_core_t *core, uintptr_t user_data)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(user_data);
#if EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_DELAY_MS > 0
    SDL_Delay(EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_DELAY_MS);
#endif
}

static void hello_multi_display_append_reject_snapshot(char *message, size_t message_size, const egui_port_core_task_queue_metrics_t *metrics)
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

static void hello_multi_display_format_stress_burst_failure(char *message, size_t message_size, const char *stage_label, int display_id, int index, int total)
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
                 "failed to post stress burst task on display %d during %s at %d/%d: pending=%u peak=%u/%u retries=%lu rejected=%lu max_retry_burst=%lu",
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
        hello_multi_display_append_reject_snapshot(message, message_size, &metrics);
        return;
    }

    snprintf(message, message_size, "failed to post stress burst task on display %d during %s at %d/%d", display_id, stage, index, total);
}

static void hello_multi_display_post_core_task_stress_burst(const char *stage_label, int display_id)
{
#if EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_BURST_COUNT > 0
    egui_core_t *core = egui_port_get_core_by_display_id(display_id);
    char message[320];

    if (core == NULL)
    {
        hello_multi_display_report_runtime_failure("failed to resolve display core for stress burst");
        return;
    }

    for (int i = 0; i < EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_BURST_COUNT; i++)
    {
        if (!egui_port_post_core_task_named(core, hello_multi_display_core_task_stress_task, 0, stage_label != NULL ? stage_label : "stress_burst"))
        {
            hello_multi_display_format_stress_burst_failure(message, sizeof(message), stage_label, display_id, i + 1,
                                                            EGUI_MULTI_DISPLAY_CORE_TASK_STRESS_BURST_COUNT);
            hello_multi_display_report_runtime_failure(message);
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

static void hello_multi_display_report_runtime_failure(const char *message)
{
    if (g_runtime_fail_reported)
    {
        return;
    }

    g_runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

static uint8_t hello_multi_display_set_click_action(int display_id, egui_view_t *view, int interval_ms, egui_sim_action_t *p_action)
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

static uint8_t hello_multi_display_query_runtime_state(int display_id, int *activity_slot, int *active_anim_count)
{
#if EGUI_PORT == EGUI_PORT_TYPE_PC
    egui_core_t *core = egui_port_get_core_by_display_id(display_id);
    hello_multi_display_slot_query_request_t request;

    if (core == NULL || (activity_slot == NULL && active_anim_count == NULL))
    {
        return 0;
    }

    request.activity_slot = -1;
    request.active_anim_count = -1;
    request.ok = 0;
    if (!egui_port_post_core_task_sync_named(core, hello_multi_display_slot_query_task, (uintptr_t)&request, 2000, "runtime_state_query"))
    {
        return 0;
    }
    if (!request.ok)
    {
        return 0;
    }

    if (activity_slot != NULL)
    {
        *activity_slot = request.activity_slot;
    }
    if (active_anim_count != NULL)
    {
        *active_anim_count = request.active_anim_count;
    }
    return 1;
#else
    if (activity_slot == NULL && active_anim_count == NULL)
    {
        return 0;
    }

    if (display_id == 0)
    {
        if (activity_slot != NULL)
        {
            *activity_slot = hello_multi_display_disp0_get_activity_slot_count();
        }
        if (active_anim_count != NULL)
        {
            *active_anim_count = 0;
        }
        return 1;
    }

    if (display_id == 1)
    {
        if (activity_slot != NULL)
        {
            *activity_slot = hello_multi_display_disp1_get_activity_slot_count();
        }
        if (active_anim_count != NULL)
        {
            *active_anim_count = 0;
        }
        return 1;
    }

    return 0;
#endif
}

static uint8_t hello_multi_display_wait_for_idle_queue(const char *stage_label, int display_id)
{
#if EGUI_PORT == EGUI_PORT_TYPE_PC
    egui_core_t *core = egui_port_get_core_by_display_id(display_id);
    egui_port_core_task_queue_metrics_t metrics;
    char message[320];
    uint32_t idle_polls = 0;
    const char *stage = stage_label != NULL ? stage_label : "unknown";

    if (core == NULL)
    {
        hello_multi_display_report_runtime_failure("failed to resolve display core for idle wait");
        return 0;
    }

    for (uint32_t waited_ms = 0;; waited_ms += EGUI_MULTI_DISPLAY_STRESS_IDLE_POLL_MS)
    {
        if (!egui_port_get_core_task_queue_metrics(core, &metrics))
        {
            hello_multi_display_report_runtime_failure("failed to query core task queue metrics before stress burst");
            return 0;
        }
        if (metrics.pending_count == 0 && metrics.inflight_count == 0)
        {
            idle_polls++;
            if (idle_polls >= EGUI_MULTI_DISPLAY_STRESS_IDLE_STABLE_POLLS)
            {
                return 1;
            }
        }
        else
        {
            idle_polls = 0;
        }
        if (waited_ms >= EGUI_MULTI_DISPLAY_STRESS_IDLE_WAIT_MS)
        {
            snprintf(message, sizeof(message),
                     "core task queue did not drain before %s on display %d: pending=%u peak=%u/%u retries=%lu rejected=%lu max_retry_burst=%lu", stage,
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
            hello_multi_display_append_reject_snapshot(message, sizeof(message), &metrics);
            hello_multi_display_report_runtime_failure(message);
            return 0;
        }
        SDL_Delay(EGUI_MULTI_DISPLAY_STRESS_IDLE_POLL_MS);
    }
#else
    EGUI_UNUSED(stage_label);
    EGUI_UNUSED(display_id);
    return 1;
#endif
}

static void hello_multi_display_wait_before_stress_burst(void)
{
#if EGUI_PORT == EGUI_PORT_TYPE_PC && EGUI_MULTI_DISPLAY_STRESS_STAGE_SETTLE_MS > 0
    SDL_Delay(EGUI_MULTI_DISPLAY_STRESS_STAGE_SETTLE_MS);
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
            multi_display_request_snapshot("initial");
        }
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;

    case 1:
        if (first_call)
        {
            if (!hello_multi_display_query_runtime_state(0, &g_disp0_slot_before_click, NULL) ||
                !hello_multi_display_query_runtime_state(1, &g_disp1_slot_before_click, NULL))
            {
                hello_multi_display_report_runtime_failure("failed to query activity slots before disp0 click");
            }
        }
        if (!hello_multi_display_set_click_action(0, hello_multi_display_disp0_get_next_button(0), 800, p_action))
        {
            hello_multi_display_report_runtime_failure("failed to resolve disp0 next button center");
            EGUI_SIM_SET_WAIT(p_action, 50);
        }
        return true;

    case 2:
        if (first_call)
        {
            int disp0_slot_after_click = -1;
            int disp1_slot_after_click = -1;

            if (!hello_multi_display_query_runtime_state(0, &disp0_slot_after_click, NULL) ||
                !hello_multi_display_query_runtime_state(1, &disp1_slot_after_click, NULL))
            {
                hello_multi_display_report_runtime_failure("failed to query activity slots after disp0 click");
            }
            else if (disp0_slot_after_click != g_disp0_slot_before_click + 1)
            {
                hello_multi_display_report_runtime_failure("disp0 click did not advance primary activity");
            }
            else if (disp1_slot_after_click != g_disp1_slot_before_click)
            {
                hello_multi_display_report_runtime_failure("disp0 click unexpectedly changed secondary activity");
            }
            multi_display_request_snapshot("after_disp0_next");
        }
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;

    case 3:
        if (first_call)
        {
            hello_multi_display_wait_before_stress_burst();
            if (hello_multi_display_wait_for_idle_queue("after_disp0_next", 0))
            {
                hello_multi_display_post_core_task_stress_burst("after_disp0_next", 0);
            }
            if (hello_multi_display_wait_for_idle_queue("after_disp0_next", 1))
            {
                hello_multi_display_post_core_task_stress_burst("after_disp0_next", 1);
            }
        }
        EGUI_SIM_SET_WAIT(p_action, 150);
        return true;

    case 4:
        if (first_call)
        {
            if (!hello_multi_display_query_runtime_state(0, &g_disp0_slot_before_dual_click, NULL) ||
                !hello_multi_display_query_runtime_state(1, &g_disp1_slot_before_dual_click, NULL))
            {
                hello_multi_display_report_runtime_failure("failed to query activity slots before concurrent next clicks");
            }
        }
        if (!hello_multi_display_set_click_action(0, hello_multi_display_disp0_get_next_button(1), 800, p_action))
        {
            hello_multi_display_report_runtime_failure("failed to resolve disp0 concurrent next button center");
            EGUI_SIM_SET_WAIT(p_action, 50);
        }
        return true;

    case 5:
        if (!hello_multi_display_set_click_action(1, hello_multi_display_disp1_get_next_button(0), 50, p_action))
        {
            hello_multi_display_report_runtime_failure("failed to resolve disp1 next button center");
            EGUI_SIM_SET_WAIT(p_action, 50);
        }
        return true;

    case 6:
        if (first_call)
        {
            int disp0_slot_after_dual_click = -1;
            int disp1_slot_after_dual_click = -1;
            int disp0_anim_count = -1;
            int disp1_anim_count = -1;

            if (!hello_multi_display_query_runtime_state(0, &disp0_slot_after_dual_click, &disp0_anim_count) ||
                !hello_multi_display_query_runtime_state(1, &disp1_slot_after_dual_click, &disp1_anim_count))
            {
                hello_multi_display_report_runtime_failure("failed to query runtime state during concurrent activity animations");
            }
            else if (disp0_slot_after_dual_click != g_disp0_slot_before_dual_click + 1)
            {
                hello_multi_display_report_runtime_failure("concurrent primary click did not advance primary activity");
            }
            else if (disp1_slot_after_dual_click != g_disp1_slot_before_dual_click + 1)
            {
                hello_multi_display_report_runtime_failure("concurrent secondary click did not advance secondary activity");
            }
            else if (disp0_anim_count <= 0 || disp1_anim_count <= 0)
            {
                hello_multi_display_report_runtime_failure("concurrent activity animations were not active on both displays");
            }
        }
        EGUI_SIM_SET_WAIT(p_action, 150);
        return true;

    case 7:
        if (first_call)
        {
            hello_multi_display_wait_before_stress_burst();
            if (hello_multi_display_wait_for_idle_queue("during_concurrent_next", 0))
            {
                hello_multi_display_post_core_task_stress_burst("during_concurrent_next", 0);
            }
            if (hello_multi_display_wait_for_idle_queue("during_concurrent_next", 1))
            {
                hello_multi_display_post_core_task_stress_burst("during_concurrent_next", 1);
            }
        }
        EGUI_SIM_SET_WAIT(p_action, 150);
        return true;

    case 8:
        if (first_call)
        {
            multi_display_request_snapshot("after_disp1_next");
        }
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;

    case 9:
        if (!hello_multi_display_set_click_action(0, hello_multi_display_disp0_get_finish_button(2), 800, p_action))
        {
            hello_multi_display_report_runtime_failure("failed to resolve disp0 finish button center");
            EGUI_SIM_SET_WAIT(p_action, 50);
        }
        return true;

    case 10:
        if (first_call)
        {
            multi_display_request_snapshot("after_finish");
        }
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;

    default:
        return false;
    }
}
#endif /* EGUI_CONFIG_RECORDING_TEST */
