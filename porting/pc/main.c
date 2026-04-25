#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "sdl_port.h"

static egui_core_t g_cores[EGUI_CONFIG_MAX_DISPLAY_COUNT];
static egui_core_t *g_registered_cores[EGUI_CONFIG_MAX_DISPLAY_COUNT];

#ifndef EGUI_PORT_PC_CORE_TASK_QUEUE_CAPACITY
#define EGUI_PORT_PC_CORE_TASK_QUEUE_CAPACITY 32
#endif

#ifndef EGUI_PORT_PC_CORE_TASK_POST_RETRY_COUNT
#define EGUI_PORT_PC_CORE_TASK_POST_RETRY_COUNT 1
#endif

#ifndef EGUI_PORT_PC_CORE_TASK_POST_RETRY_DELAY_MS
#define EGUI_PORT_PC_CORE_TASK_POST_RETRY_DELAY_MS 4
#endif

#define PC_CORE_TASK_QUEUE_CAPACITY  EGUI_PORT_PC_CORE_TASK_QUEUE_CAPACITY
#define PC_CORE_TASK_WAIT_TIMEOUT_MS 2000
#define SHUTDOWN_MARKER_PREFIX       "[SHUTDOWN_CHECK]"

typedef struct pc_core_task
{
    egui_port_core_task_func_t task_func;
    uintptr_t user_data;
    SDL_sem *done_sem;
    const char *context;
    uint32_t enqueue_tick_ms;
} pc_core_task_t;

typedef struct pc_core_thread_context
{
    egui_core_t *core;
    SDL_Thread *thread;
    SDL_atomic_t running;
    SDL_mutex *task_mutex;
    SDL_sem *wake_sem;
    pc_core_task_t task_queue[PC_CORE_TASK_QUEUE_CAPACITY];
    uint16_t task_head;
    uint16_t task_tail;
    uint16_t task_count;
    uint16_t task_peak_count;
    uint16_t task_inflight_count;
    const char *task_inflight_context;
    uint16_t task_last_reject_pending_count;
    uint16_t task_last_reject_inflight_count;
    const char *task_last_reject_context;
    const char *task_last_reject_pending_context;
    const char *task_last_reject_inflight_context;
    uint32_t task_post_success_count;
    uint32_t task_post_retry_count;
    uint32_t task_post_max_retry_burst;
    uint32_t task_post_reject_count;
    uint32_t task_wait_timeout_count;
    uint32_t task_max_queue_wait_ms;
    const char *task_max_queue_wait_context;
    uint32_t task_max_exec_time_ms;
    const char *task_max_exec_time_context;
    uint8_t display_id;
    char name[32];
} pc_core_thread_context_t;

static pc_core_thread_context_t g_core_threads[EGUI_CONFIG_MAX_DISPLAY_COUNT];

static void pc_log_core_task_post_failure(const char *context, int display_id, const char *reason)
{
    printf("[PC_CORE_TASK_FAIL] context=%s display=%d reason=%s\n", context != NULL ? context : "unknown", display_id, reason != NULL ? reason : "unknown");
}

static void pc_record_core_task_wait_timeout(int display_id)
{
    if (display_id >= 0 && display_id < EGUI_CONFIG_MAX_DISPLAY_COUNT)
    {
        g_core_threads[display_id].task_wait_timeout_count++;
    }
}

static void pc_wake_core_thread(pc_core_thread_context_t *thread_ctx)
{
    if (thread_ctx != NULL && thread_ctx->wake_sem != NULL)
    {
        SDL_SemPost(thread_ctx->wake_sem);
    }
}

static int pc_wait_core_task_done_with_timeout(pc_core_thread_context_t *thread_ctx, SDL_sem *done_sem, const char *context, int display_id,
                                               uint32_t timeout_ms);

static int pc_enqueue_core_task(pc_core_thread_context_t *thread_ctx, egui_port_core_task_func_t task_func, uintptr_t user_data, SDL_sem *done_sem,
                                const char *context, int display_id)
{
    int retry_count = 0;

    if (thread_ctx == NULL || thread_ctx->task_mutex == NULL || task_func == NULL)
    {
        pc_log_core_task_post_failure(context, display_id, "thread_not_ready");
        return 0;
    }

    while (1)
    {
        SDL_LockMutex(thread_ctx->task_mutex);
        if (thread_ctx->task_count < PC_CORE_TASK_QUEUE_CAPACITY)
        {
            thread_ctx->task_queue[thread_ctx->task_tail].task_func = task_func;
            thread_ctx->task_queue[thread_ctx->task_tail].user_data = user_data;
            thread_ctx->task_queue[thread_ctx->task_tail].done_sem = done_sem;
            thread_ctx->task_queue[thread_ctx->task_tail].context = context;
            thread_ctx->task_queue[thread_ctx->task_tail].enqueue_tick_ms = SDL_GetTicks();
            thread_ctx->task_tail = (uint16_t)((thread_ctx->task_tail + 1) % PC_CORE_TASK_QUEUE_CAPACITY);
            thread_ctx->task_count++;
            thread_ctx->task_post_success_count++;
            if (thread_ctx->task_count > thread_ctx->task_peak_count)
            {
                thread_ctx->task_peak_count = thread_ctx->task_count;
            }
            if ((uint32_t)retry_count > thread_ctx->task_post_max_retry_burst)
            {
                thread_ctx->task_post_max_retry_burst = (uint32_t)retry_count;
            }
            SDL_UnlockMutex(thread_ctx->task_mutex);
            pc_wake_core_thread(thread_ctx);
            return 1;
        }
        thread_ctx->task_last_reject_pending_count = thread_ctx->task_count;
        thread_ctx->task_last_reject_inflight_count = thread_ctx->task_inflight_count;
        thread_ctx->task_last_reject_context = context;
        thread_ctx->task_last_reject_pending_context = thread_ctx->task_count > 0 ? thread_ctx->task_queue[thread_ctx->task_head].context : NULL;
        thread_ctx->task_last_reject_inflight_context = thread_ctx->task_inflight_context;
        SDL_UnlockMutex(thread_ctx->task_mutex);

        if (retry_count >= EGUI_PORT_PC_CORE_TASK_POST_RETRY_COUNT)
        {
            if ((uint32_t)retry_count > thread_ctx->task_post_max_retry_burst)
            {
                thread_ctx->task_post_max_retry_burst = (uint32_t)retry_count;
            }
            thread_ctx->task_post_reject_count++;
            pc_log_core_task_post_failure(context, display_id, "queue_full");
            return 0;
        }

        retry_count++;
        thread_ctx->task_post_retry_count++;
        pc_wake_core_thread(thread_ctx);
#if EGUI_PORT_PC_CORE_TASK_POST_RETRY_DELAY_MS > 0
        SDL_Delay(EGUI_PORT_PC_CORE_TASK_POST_RETRY_DELAY_MS);
#endif
    }
}

static int pc_cancel_queued_sync_task(pc_core_thread_context_t *thread_ctx, SDL_sem *done_sem)
{
    pc_core_task_t retained_tasks[PC_CORE_TASK_QUEUE_CAPACITY];
    uint16_t retained_count = 0;
    int canceled = 0;

    if (thread_ctx == NULL || thread_ctx->task_mutex == NULL || done_sem == NULL)
    {
        return 0;
    }

    SDL_LockMutex(thread_ctx->task_mutex);
    for (uint16_t index = 0; index < thread_ctx->task_count; index++)
    {
        pc_core_task_t task = thread_ctx->task_queue[(thread_ctx->task_head + index) % PC_CORE_TASK_QUEUE_CAPACITY];

        if (!canceled && task.done_sem == done_sem)
        {
            canceled = 1;
            continue;
        }

        retained_tasks[retained_count++] = task;
    }

    if (canceled)
    {
        for (uint16_t index = 0; index < retained_count; index++)
        {
            thread_ctx->task_queue[index] = retained_tasks[index];
        }
        thread_ctx->task_head = 0;
        thread_ctx->task_tail = retained_count % PC_CORE_TASK_QUEUE_CAPACITY;
        thread_ctx->task_count = retained_count;
    }
    SDL_UnlockMutex(thread_ctx->task_mutex);
    return canceled;
}

static int pc_wait_core_task_done(pc_core_thread_context_t *thread_ctx, SDL_sem *done_sem, const char *context, int display_id)
{
    return pc_wait_core_task_done_with_timeout(thread_ctx, done_sem, context, display_id, PC_CORE_TASK_WAIT_TIMEOUT_MS);
}

static int pc_wait_core_task_done_with_timeout(pc_core_thread_context_t *thread_ctx, SDL_sem *done_sem, const char *context, int display_id,
                                               uint32_t timeout_ms)
{
    int wait_result;

    if (done_sem == NULL)
    {
        pc_log_core_task_post_failure(context, display_id, "invalid_semaphore");
        return 0;
    }

    wait_result = SDL_SemWaitTimeout(done_sem, timeout_ms);
    if (wait_result != 0)
    {
        pc_record_core_task_wait_timeout(display_id);
        pc_log_core_task_post_failure(context, display_id, "wait_timeout");

        if (pc_cancel_queued_sync_task(thread_ctx, done_sem))
        {
            return 0;
        }

        // The task has already started, or completion raced with timeout handling.
        // Wait for completion before returning so caller-owned stack payloads and
        // synchronization primitives are not released while the worker still uses them.
        SDL_SemWait(done_sem);
        return 0;
    }

    return 1;
}

#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
__EGUI_WEAK__ int egui_port_get_additional_display_descriptors(egui_port_extra_display_descriptor_t *descriptors, int max_count)
{
    EGUI_UNUSED(descriptors);
    EGUI_UNUSED(max_count);
    return 0;
}
#endif

void egui_port_register_core(egui_core_t *core_inst)
{
    int display_id;
    EGUI_ASSERT(core_inst != NULL);
    display_id = (int)core_inst->id;
    EGUI_ASSERT(display_id >= 0 && display_id < EGUI_CONFIG_MAX_DISPLAY_COUNT);
    EGUI_ASSERT(core_inst == &g_cores[display_id]);
    __atomic_store_n(&g_registered_cores[display_id], core_inst, __ATOMIC_RELEASE);
}

egui_core_t *egui_port_get_core_by_display_id(int display_id)
{
    EGUI_ASSERT(display_id >= 0);
    if (display_id >= EGUI_CONFIG_MAX_DISPLAY_COUNT)
    {
        return NULL;
    }
    return __atomic_load_n(&g_registered_cores[display_id], __ATOMIC_ACQUIRE);
}

int egui_port_post_core_task(egui_core_t *core, egui_port_core_task_func_t task_func, uintptr_t user_data)
{
    return egui_port_post_core_task_named(core, task_func, user_data, "post");
}

int egui_port_post_core_task_named(egui_core_t *core, egui_port_core_task_func_t task_func, uintptr_t user_data, const char *context)
{
    pc_core_thread_context_t *thread_ctx;
    int display_id;

    if (core == NULL || task_func == NULL)
    {
        pc_log_core_task_post_failure(context != NULL ? context : "post", -1, "invalid_argument");
        return 0;
    }

    display_id = (int)core->id;
    if (display_id < 0 || display_id >= EGUI_CONFIG_MAX_DISPLAY_COUNT)
    {
        pc_log_core_task_post_failure(context != NULL ? context : "post", display_id, "invalid_display_id");
        return 0;
    }

    thread_ctx = &g_core_threads[display_id];
    return pc_enqueue_core_task(thread_ctx, task_func, user_data, NULL, context != NULL ? context : "post", display_id);
}

int egui_port_post_core_task_sync(egui_core_t *core, egui_port_core_task_func_t task_func, uintptr_t user_data, uint32_t timeout_ms)
{
    return egui_port_post_core_task_sync_named(core, task_func, user_data, timeout_ms, "post_sync");
}

int egui_port_post_core_task_sync_named(egui_core_t *core, egui_port_core_task_func_t task_func, uintptr_t user_data, uint32_t timeout_ms, const char *context)
{
    pc_core_thread_context_t *thread_ctx;
    SDL_sem *done_sem;
    int display_id;

    if (core == NULL || task_func == NULL)
    {
        pc_log_core_task_post_failure(context != NULL ? context : "post_sync", -1, "invalid_argument");
        return 0;
    }

    display_id = (int)core->id;
    if (display_id < 0 || display_id >= EGUI_CONFIG_MAX_DISPLAY_COUNT)
    {
        pc_log_core_task_post_failure(context != NULL ? context : "post_sync", display_id, "invalid_display_id");
        return 0;
    }

    thread_ctx = &g_core_threads[display_id];
    if (thread_ctx->task_mutex == NULL)
    {
        pc_log_core_task_post_failure(context != NULL ? context : "post_sync", display_id, "thread_not_ready");
        return 0;
    }

    if (thread_ctx->thread != NULL && SDL_ThreadID() == SDL_GetThreadID(thread_ctx->thread))
    {
        task_func(core, user_data);
        return 1;
    }

    done_sem = SDL_CreateSemaphore(0);
    if (done_sem == NULL)
    {
        pc_log_core_task_post_failure(context != NULL ? context : "post_sync", display_id, "create_semaphore_failed");
        return 0;
    }

    if (!pc_enqueue_core_task(thread_ctx, task_func, user_data, done_sem, context != NULL ? context : "post_sync", display_id))
    {
        SDL_DestroySemaphore(done_sem);
        return 0;
    }

    if (timeout_ms == 0)
    {
        timeout_ms = PC_CORE_TASK_WAIT_TIMEOUT_MS;
    }
    if (!pc_wait_core_task_done_with_timeout(thread_ctx, done_sem, context != NULL ? context : "post_sync", display_id, timeout_ms))
    {
        SDL_DestroySemaphore(done_sem);
        return 0;
    }

    SDL_DestroySemaphore(done_sem);
    return 1;
}

typedef struct pc_display_runtime_info_request
{
    egui_port_display_runtime_info_t *info;
} pc_display_runtime_info_request_t;

typedef struct pc_view_center_request
{
    egui_view_t *view;
    int *x;
    int *y;
    uint8_t ok;
} pc_view_center_request_t;

static void pc_fill_display_runtime_info_task(egui_core_t *core, uintptr_t user_data)
{
    pc_display_runtime_info_request_t *request = (pc_display_runtime_info_request_t *)user_data;
    egui_display_driver_t *driver;

    if (core == NULL || request == NULL || request->info == NULL)
    {
        return;
    }

    driver = egui_display_driver_get(core);
    if (driver == NULL)
    {
        memset(request->info, 0, sizeof(*request->info));
        return;
    }

    request->info->physical_width = driver->physical_width;
    request->info->physical_height = driver->physical_height;
    request->info->rotation = driver->rotation;
    request->info->software_rotation = core->render.software_rotation;
    request->info->has_hardware_rotation = driver->ops->set_rotation != NULL ? 1 : 0;
}

static void pc_fill_view_center_task(egui_core_t *core, uintptr_t user_data)
{
    pc_view_center_request_t *request = (pc_view_center_request_t *)user_data;
    egui_view_t *view;

    EGUI_UNUSED(core);

    if (request == NULL || request->view == NULL || request->x == NULL || request->y == NULL)
    {
        return;
    }

    view = request->view;
    if (view->region_screen.size.width <= 0 || view->region_screen.size.height <= 0)
    {
        return;
    }

    *request->x = view->region_screen.location.x + view->region_screen.size.width / 2;
    *request->y = view->region_screen.location.y + view->region_screen.size.height / 2;
    request->ok = 1;
}

int egui_port_get_display_runtime_info(egui_core_t *core, egui_port_display_runtime_info_t *info)
{
    pc_core_thread_context_t *thread_ctx;
    pc_display_runtime_info_request_t request;
    SDL_sem *done_sem;
    int display_id;

    if (core == NULL || info == NULL)
    {
        pc_log_core_task_post_failure("display_runtime_info", -1, "invalid_argument");
        return 0;
    }

    display_id = (int)core->id;
    if (display_id < 0 || display_id >= EGUI_CONFIG_MAX_DISPLAY_COUNT)
    {
        pc_log_core_task_post_failure("display_runtime_info", display_id, "invalid_display_id");
        return 0;
    }

    thread_ctx = &g_core_threads[display_id];
    if (thread_ctx->task_mutex == NULL)
    {
        pc_log_core_task_post_failure("display_runtime_info", display_id, "thread_not_ready");
        return 0;
    }

    if (thread_ctx->thread != NULL && SDL_ThreadID() == SDL_GetThreadID(thread_ctx->thread))
    {
        request.info = info;
        pc_fill_display_runtime_info_task(core, (uintptr_t)&request);
        return 1;
    }

    done_sem = SDL_CreateSemaphore(0);
    if (done_sem == NULL)
    {
        pc_log_core_task_post_failure("display_runtime_info", display_id, "create_semaphore_failed");
        return 0;
    }

    request.info = info;
    if (!pc_enqueue_core_task(thread_ctx, pc_fill_display_runtime_info_task, (uintptr_t)&request, done_sem, "display_runtime_info", display_id))
    {
        SDL_DestroySemaphore(done_sem);
        return 0;
    }

    if (!pc_wait_core_task_done(thread_ctx, done_sem, "display_runtime_info", display_id))
    {
        SDL_DestroySemaphore(done_sem);
        return 0;
    }
    SDL_DestroySemaphore(done_sem);
    return 1;
}

int egui_port_get_view_center(egui_core_t *core, egui_view_t *view, int *x, int *y)
{
    pc_core_thread_context_t *thread_ctx;
    pc_view_center_request_t request;
    SDL_sem *done_sem;
    int resolved_x = 0;
    int resolved_y = 0;
    int display_id;

    if (core == NULL || view == NULL || x == NULL || y == NULL)
    {
        pc_log_core_task_post_failure("view_center", -1, "invalid_argument");
        return 0;
    }

    display_id = (int)core->id;
    if (display_id < 0 || display_id >= EGUI_CONFIG_MAX_DISPLAY_COUNT)
    {
        pc_log_core_task_post_failure("view_center", display_id, "invalid_display_id");
        return 0;
    }

    thread_ctx = &g_core_threads[display_id];
    if (thread_ctx->task_mutex == NULL)
    {
        pc_log_core_task_post_failure("view_center", display_id, "thread_not_ready");
        return 0;
    }

    request.view = view;
    request.x = &resolved_x;
    request.y = &resolved_y;
    request.ok = 0;

    if (thread_ctx->thread != NULL && SDL_ThreadID() == SDL_GetThreadID(thread_ctx->thread))
    {
        pc_fill_view_center_task(core, (uintptr_t)&request);
        if (!request.ok)
        {
            return 0;
        }
        *x = resolved_x;
        *y = resolved_y;
        return 1;
    }

    done_sem = SDL_CreateSemaphore(0);
    if (done_sem == NULL)
    {
        pc_log_core_task_post_failure("view_center", display_id, "create_semaphore_failed");
        return 0;
    }

    if (!pc_enqueue_core_task(thread_ctx, pc_fill_view_center_task, (uintptr_t)&request, done_sem, "view_center", display_id))
    {
        SDL_DestroySemaphore(done_sem);
        return 0;
    }

    if (!pc_wait_core_task_done(thread_ctx, done_sem, "view_center", display_id))
    {
        SDL_DestroySemaphore(done_sem);
        return 0;
    }
    SDL_DestroySemaphore(done_sem);
    if (!request.ok)
    {
        return 0;
    }

    *x = resolved_x;
    *y = resolved_y;
    return 1;
}

int egui_port_get_core_task_queue_metrics(egui_core_t *core, egui_port_core_task_queue_metrics_t *metrics)
{
    pc_core_thread_context_t *thread_ctx;
    int display_id;

    if (core == NULL || metrics == NULL)
    {
        pc_log_core_task_post_failure("core_task_queue_metrics", -1, "invalid_argument");
        return 0;
    }

    display_id = (int)core->id;
    if (display_id < 0 || display_id >= EGUI_CONFIG_MAX_DISPLAY_COUNT)
    {
        pc_log_core_task_post_failure("core_task_queue_metrics", display_id, "invalid_display_id");
        return 0;
    }

    thread_ctx = &g_core_threads[display_id];
    if (thread_ctx->task_mutex == NULL)
    {
        pc_log_core_task_post_failure("core_task_queue_metrics", display_id, "thread_not_ready");
        return 0;
    }

    SDL_LockMutex(thread_ctx->task_mutex);
    metrics->queue_capacity = PC_CORE_TASK_QUEUE_CAPACITY;
    metrics->pending_count = thread_ctx->task_count;
    metrics->inflight_count = thread_ctx->task_inflight_count;
    metrics->peak_count = thread_ctx->task_peak_count;
    metrics->pending_context = thread_ctx->task_count > 0 ? thread_ctx->task_queue[thread_ctx->task_head].context : NULL;
    metrics->inflight_context = thread_ctx->task_inflight_context;
    metrics->last_reject_pending_count = thread_ctx->task_last_reject_pending_count;
    metrics->last_reject_inflight_count = thread_ctx->task_last_reject_inflight_count;
    metrics->last_reject_context = thread_ctx->task_last_reject_context;
    metrics->last_reject_pending_context = thread_ctx->task_last_reject_pending_context;
    metrics->last_reject_inflight_context = thread_ctx->task_last_reject_inflight_context;
    metrics->post_success_count = thread_ctx->task_post_success_count;
    metrics->post_retry_count = thread_ctx->task_post_retry_count;
    metrics->post_max_retry_burst = thread_ctx->task_post_max_retry_burst;
    metrics->post_reject_count = thread_ctx->task_post_reject_count;
    metrics->wait_timeout_count = thread_ctx->task_wait_timeout_count;
    metrics->max_queue_wait_ms = thread_ctx->task_max_queue_wait_ms;
    metrics->max_queue_wait_context = thread_ctx->task_max_queue_wait_context;
    metrics->max_exec_time_ms = thread_ctx->task_max_exec_time_ms;
    metrics->max_exec_time_context = thread_ctx->task_max_exec_time_context;
    SDL_UnlockMutex(thread_ctx->task_mutex);
    return 1;
}

static void pc_drain_core_tasks(pc_core_thread_context_t *thread_ctx)
{
    pc_core_task_t task;
    uint32_t queue_wait_ms;
    uint32_t exec_begin_ms;
    uint32_t exec_time_ms;

    if (thread_ctx == NULL || thread_ctx->core == NULL || thread_ctx->task_mutex == NULL)
    {
        return;
    }

    while (1)
    {
        SDL_LockMutex(thread_ctx->task_mutex);
        if (thread_ctx->task_count == 0)
        {
            SDL_UnlockMutex(thread_ctx->task_mutex);
            break;
        }

        task = thread_ctx->task_queue[thread_ctx->task_head];
        thread_ctx->task_head = (uint16_t)((thread_ctx->task_head + 1) % PC_CORE_TASK_QUEUE_CAPACITY);
        thread_ctx->task_count--;
        thread_ctx->task_inflight_count = 1;
        thread_ctx->task_inflight_context = task.context;
        queue_wait_ms = SDL_GetTicks() - task.enqueue_tick_ms;
        if (queue_wait_ms > thread_ctx->task_max_queue_wait_ms)
        {
            thread_ctx->task_max_queue_wait_ms = queue_wait_ms;
            thread_ctx->task_max_queue_wait_context = task.context;
        }
        SDL_UnlockMutex(thread_ctx->task_mutex);

        exec_begin_ms = SDL_GetTicks();
        if (task.task_func != NULL)
        {
            task.task_func(thread_ctx->core, task.user_data);
        }
        exec_time_ms = SDL_GetTicks() - exec_begin_ms;
        SDL_LockMutex(thread_ctx->task_mutex);
        thread_ctx->task_inflight_count = 0;
        thread_ctx->task_inflight_context = NULL;
        if (exec_time_ms > thread_ctx->task_max_exec_time_ms)
        {
            thread_ctx->task_max_exec_time_ms = exec_time_ms;
            thread_ctx->task_max_exec_time_context = task.context;
        }
        SDL_UnlockMutex(thread_ctx->task_mutex);
        if (task.done_sem != NULL)
        {
            SDL_SemPost(task.done_sem);
        }
    }
}

static void pc_wait_core_thread_wakeup(pc_core_thread_context_t *thread_ctx)
{
    if (thread_ctx == NULL || SDL_AtomicGet(&thread_ctx->running) == 0)
    {
        return;
    }

    if (thread_ctx->wake_sem != NULL)
    {
        SDL_SemWaitTimeout(thread_ctx->wake_sem, 1);
        return;
    }

    SDL_Delay(1);
}

static int pc_core_thread_entry(void *argument)
{
    pc_core_thread_context_t *thread_ctx = (pc_core_thread_context_t *)argument;

    if (thread_ctx == NULL || thread_ctx->core == NULL)
    {
        return -1;
    }

    while (SDL_AtomicGet(&thread_ctx->running) != 0)
    {
        pc_drain_core_tasks(thread_ctx);
        egui_polling_work(thread_ctx->core);
        pc_wait_core_thread_wakeup(thread_ctx);
    }

    pc_drain_core_tasks(thread_ctx);
    return 0;
}

void app_set_gpio(uint8_t pin, uint8_t state)
{
}

EGUI_CONFIG_PFB_BUFFER_DECLARE(egui_pfb);

#define MAX_PATH 0x1000
char input_file_path[MAX_PATH];

char *pc_get_input_file_path(void)
{
    return input_file_path;
}

static void pasre_input_params(int argc, const char *argv[])
{
    int i = 1;

    while (i < argc)
    {
        if (strcmp(argv[i], "--headless") == 0 || strcmp(argv[i], "--touch-trace") == 0)
        {
            i++;
            continue;
        }
        if ((strcmp(argv[i], "--record") == 0 && i + 3 < argc) || (strcmp(argv[i], "--speed") == 0 && i + 1 < argc) ||
            (strcmp(argv[i], "--clock-scale") == 0 && i + 1 < argc) || (strcmp(argv[i], "--snapshot-settle-ms") == 0 && i + 1 < argc) ||
            (strcmp(argv[i], "--snapshot-stable-cycles") == 0 && i + 1 < argc) || (strcmp(argv[i], "--snapshot-max-wait-ms") == 0 && i + 1 < argc) ||
            (strcmp(argv[i], "--touch-trace-limit") == 0 && i + 1 < argc))
        {
            if (strcmp(argv[i], "--record") == 0)
            {
                i += 4;
            }
            else
            {
                i += 2;
            }
            continue;
        }
        if (argv[i][0] == '-' && argv[i][1] == '-')
        {
            i++;
            continue;
        }

        strcpy(input_file_path, argv[i]);
        return;
    }

    strcpy(input_file_path, "app_egui_resource_merge.bin");
}

static void parse_recording_params(int argc, const char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--headless") == 0)
        {
            sdl_port_set_headless(true);
        }
        if (strcmp(argv[i], "--record") == 0 && i + 3 < argc)
        {
            const char *output_dir = argv[i + 1];
            int fps = atoi(argv[i + 2]);
            int duration = atoi(argv[i + 3]);
            recording_init(output_dir, fps, duration);
        }
        else if (strcmp(argv[i], "--speed") == 0 && i + 1 < argc)
        {
            int speed = atoi(argv[i + 1]);
            recording_set_speed(speed);
        }
        else if (strcmp(argv[i], "--clock-scale") == 0 && i + 1 < argc)
        {
            int clock_scale = atoi(argv[i + 1]);
            recording_set_clock_scale(clock_scale);
        }
        else if (strcmp(argv[i], "--snapshot-settle-ms") == 0 && i + 1 < argc)
        {
            int settle_ms = atoi(argv[i + 1]);
            recording_set_snapshot_settle_ms(settle_ms);
        }
        else if (strcmp(argv[i], "--snapshot-stable-cycles") == 0 && i + 1 < argc)
        {
            int stable_cycles = atoi(argv[i + 1]);
            recording_set_snapshot_stability(stable_cycles, -1);
        }
        else if (strcmp(argv[i], "--snapshot-max-wait-ms") == 0 && i + 1 < argc)
        {
            int max_wait_ms = atoi(argv[i + 1]);
            recording_set_snapshot_stability(-1, max_wait_ms);
        }
    }
}

static int pc_setup_display(egui_core_t *core, uint8_t display_id, int screen_width, int screen_height, int pfb_width, int pfb_height,
                            egui_color_int_t **pfb_buffers, int pfb_buffer_count, egui_display_driver_t *display_driver,
                            const egui_core_render_config_t *render_config, egui_touch_register_func_t touch_register, egui_uicode_init_func_t uicode_init)
{
    egui_display_setup_t setup;

    if (core == NULL || pfb_buffers == NULL || pfb_buffer_count <= 0 || display_driver == NULL || uicode_init == NULL)
    {
        return -1;
    }

    memset(&setup, 0, sizeof(setup));
    setup.screen_width = screen_width;
    setup.screen_height = screen_height;
    setup.pfb_width = pfb_width;
    setup.pfb_height = pfb_height;
    setup.pfb_buffers = pfb_buffers;
    setup.pfb_buffer_count = pfb_buffer_count;
    setup.display_driver = display_driver;
    setup.render_config = render_config;
    setup.touch_register = touch_register;
    setup.uicode_init = uicode_init;
    setup.display_id = display_id;

    egui_setup_display(core, &setup);
    egui_port_register_core(core);
    return 0;
}

static void pc_stop_core_threads(void)
{
    int stopped_threads = 0;

    for (int display_id = 0; display_id < EGUI_CONFIG_MAX_DISPLAY_COUNT; display_id++)
    {
        if (g_core_threads[display_id].thread != NULL)
        {
            SDL_AtomicSet(&g_core_threads[display_id].running, 0);
            pc_wake_core_thread(&g_core_threads[display_id]);
        }
    }

    for (int display_id = 0; display_id < EGUI_CONFIG_MAX_DISPLAY_COUNT; display_id++)
    {
        if (g_core_threads[display_id].thread != NULL)
        {
            SDL_WaitThread(g_core_threads[display_id].thread, NULL);
            g_core_threads[display_id].thread = NULL;
            stopped_threads++;
        }
        if (g_core_threads[display_id].core != NULL)
        {
            printf("%s display=%d queue_capacity=%d posted=%lu retries=%lu max_retry_burst=%lu rejected=%lu wait_timeouts=%lu peak_queue=%u pending=%u "
                   "inflight=%u max_queue_wait_ms=%lu max_queue_wait_ctx=%s max_exec_time_ms=%lu max_exec_time_ctx=%s\n",
                   SHUTDOWN_MARKER_PREFIX, display_id, PC_CORE_TASK_QUEUE_CAPACITY, (unsigned long)g_core_threads[display_id].task_post_success_count,
                   (unsigned long)g_core_threads[display_id].task_post_retry_count, (unsigned long)g_core_threads[display_id].task_post_max_retry_burst,
                   (unsigned long)g_core_threads[display_id].task_post_reject_count, (unsigned long)g_core_threads[display_id].task_wait_timeout_count,
                   (unsigned int)g_core_threads[display_id].task_peak_count, (unsigned int)g_core_threads[display_id].task_count,
                   (unsigned int)g_core_threads[display_id].task_inflight_count, (unsigned long)g_core_threads[display_id].task_max_queue_wait_ms,
                   g_core_threads[display_id].task_max_queue_wait_context != NULL ? g_core_threads[display_id].task_max_queue_wait_context : "none",
                   (unsigned long)g_core_threads[display_id].task_max_exec_time_ms,
                   g_core_threads[display_id].task_max_exec_time_context != NULL ? g_core_threads[display_id].task_max_exec_time_context : "none");
        }
        if (g_core_threads[display_id].task_mutex != NULL)
        {
            SDL_DestroyMutex(g_core_threads[display_id].task_mutex);
            g_core_threads[display_id].task_mutex = NULL;
        }
        if (g_core_threads[display_id].wake_sem != NULL)
        {
            SDL_DestroySemaphore(g_core_threads[display_id].wake_sem);
            g_core_threads[display_id].wake_sem = NULL;
        }
        g_core_threads[display_id].core = NULL;
    }

    printf("%s core_threads_stopped=%d\n", SHUTDOWN_MARKER_PREFIX, stopped_threads);
}

static int pc_start_registered_core_threads(void)
{
    memset(g_core_threads, 0, sizeof(g_core_threads));

    for (int display_id = 0; display_id < EGUI_CONFIG_MAX_DISPLAY_COUNT; display_id++)
    {
        egui_core_t *registered = __atomic_load_n(&g_registered_cores[display_id], __ATOMIC_ACQUIRE);
        pc_core_thread_context_t *thread_ctx;

        if (registered == NULL)
        {
            continue;
        }

        thread_ctx = &g_core_threads[display_id];
        thread_ctx->core = registered;
        thread_ctx->display_id = (uint8_t)display_id;
        thread_ctx->task_mutex = SDL_CreateMutex();
        if (thread_ctx->task_mutex == NULL)
        {
            printf("Failed to create task mutex for display %d: %s\n", display_id, SDL_GetError());
            pc_stop_core_threads();
            return -1;
        }
        thread_ctx->wake_sem = SDL_CreateSemaphore(0);
        if (thread_ctx->wake_sem == NULL)
        {
            printf("Failed to create wake semaphore for display %d: %s\n", display_id, SDL_GetError());
            pc_stop_core_threads();
            return -1;
        }
        SDL_AtomicSet(&thread_ctx->running, 1);
        sprintf(thread_ctx->name, "egui-disp-%d", display_id);
        thread_ctx->thread = SDL_CreateThread(pc_core_thread_entry, thread_ctx->name, thread_ctx);
        if (thread_ctx->thread == NULL)
        {
            printf("Failed to create egui thread for display %d: %s\n", display_id, SDL_GetError());
            pc_stop_core_threads();
            return -1;
        }
    }

    return 0;
}

int main(int argc, const char *argv[])
{
    printf("Hello, egui!\n");

    pasre_input_params(argc, argv);
    parse_recording_params(argc, argv);

    VT_init();

    egui_color_int_t *pfb_bufs[EGUI_CONFIG_PFB_BUFFER_COUNT];

    for (int i = 0; i < EGUI_CONFIG_PFB_BUFFER_COUNT; i++)
    {
        pfb_bufs[i] = egui_pfb[i];
    }

    egui_port_init();

    if (pc_setup_display(&g_cores[0], 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, EGUI_CONFIG_PFB_WIDTH, EGUI_CONFIG_PFB_HEIGHT, pfb_bufs,
                         EGUI_CONFIG_PFB_BUFFER_COUNT, egui_port_get_display_driver(), NULL,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                         egui_port_register_touch_driver,
#else
                         NULL,
#endif
                         uicode_disp0_init) != 0)
    {
        printf("Failed to setup primary display.\n");
        VT_deinit();
        return -1;
    }

#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    {
        egui_port_extra_display_descriptor_t extra_displays[EGUI_CONFIG_MAX_DISPLAY_COUNT - 1];
        int extra_display_count;

        memset(extra_displays, 0, sizeof(extra_displays));
        extra_display_count = egui_port_get_additional_display_descriptors(extra_displays, EGUI_CONFIG_MAX_DISPLAY_COUNT - 1);

        if (extra_display_count < 0 || extra_display_count > (EGUI_CONFIG_MAX_DISPLAY_COUNT - 1))
        {
            printf("Invalid additional display count: %d\n", extra_display_count);
            VT_deinit();
            return -1;
        }

        for (int index = 0; index < extra_display_count; index++)
        {
            int display_id = index + 1;
            egui_port_extra_display_descriptor_t *descriptor = &extra_displays[index];
            egui_display_driver_t *sub_driver;

            sub_driver = egui_port_create_sub_display(&g_cores[0], display_id, descriptor->screen_width, descriptor->screen_height);
            if (sub_driver == NULL)
            {
                printf("Failed to create sub display driver for display %d.\n", display_id);
                VT_deinit();
                return -1;
            }

            if (pc_setup_display(&g_cores[display_id], (uint8_t)display_id, descriptor->screen_width, descriptor->screen_height, descriptor->pfb_width,
                                 descriptor->pfb_height, descriptor->pfb_buffers, descriptor->pfb_buffer_count, sub_driver, descriptor->render_config,
                                 descriptor->touch_register, descriptor->uicode_init) != 0)
            {
                printf("Failed to setup additional display %d.\n", display_id);
                VT_deinit();
                return -1;
            }
        }
    }
#endif

    if (pc_start_registered_core_threads() != 0)
    {
        VT_deinit();
        return -1;
    }

    while (1)
    {
        VT_sdl_refresh_task();
        if (VT_is_request_quit())
        {
            break;
        }
        SDL_Delay(1);
    }

    VT_begin_shutdown();
    pc_stop_core_threads();
    VT_deinit();
    return 0;
}
