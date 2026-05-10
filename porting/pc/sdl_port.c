#include "sdl_port.h"
#include <stdlib.h>
#include <stddef.h>

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "SDL2/SDL.h"
#include "core/egui_input.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifdef _WIN32
#include <direct.h>
#if EGUI_PC_LOG_TO_DEBUG_OUTPUT
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

#undef main

#define monochrome_2_RGB888(color) (color ? 0x000000 : 0xffffff)
#define GRAY8_2_RGB888(color)      (((color & 0xFF) << 16) + ((color & 0xFF) << 8) + ((color & 0xFF)))

#if defined(_WIN32) && EGUI_PC_LOG_TO_DEBUG_OUTPUT
static bool s_stdout_redirected_to_debug = false;

static void egui_pc_output_debug_string(const char *buffer)
{
    char debug_buffer[2048];
    size_t out_index = 0;

    if (buffer == NULL)
    {
        return;
    }

    for (size_t index = 0; buffer[index] != '\0'; index++)
    {
        if (out_index >= sizeof(debug_buffer) - 3)
        {
            debug_buffer[out_index] = '\0';
            OutputDebugStringA(debug_buffer);
            out_index = 0;
        }

        if (buffer[index] == '\n' && (index == 0 || buffer[index - 1] != '\r'))
        {
            debug_buffer[out_index++] = '\r';
        }
        debug_buffer[out_index++] = buffer[index];
    }

    if (out_index > 0)
    {
        debug_buffer[out_index] = '\0';
        OutputDebugStringA(debug_buffer);
    }
}

static bool egui_pc_stdout_available(void)
{
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (stdout_handle == NULL || stdout_handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    return GetFileType(stdout_handle) != FILE_TYPE_UNKNOWN;
}

static DWORD WINAPI egui_pc_stdout_debug_thread(LPVOID user_data)
{
    HANDLE read_pipe = (HANDLE)user_data;
    char buffer[512];
    DWORD bytes_read = 0;

    while (ReadFile(read_pipe, buffer, sizeof(buffer) - 1, &bytes_read, NULL) && bytes_read > 0)
    {
        buffer[bytes_read] = '\0';
        egui_pc_output_debug_string(buffer);
    }

    CloseHandle(read_pipe);
    return 0;
}
#endif

void egui_pc_log_init(void)
{
#if defined(_WIN32) && EGUI_PC_LOG_TO_DEBUG_OUTPUT
    HANDLE read_pipe = NULL;
    HANDLE write_pipe = NULL;
    HANDLE thread = NULL;
    SECURITY_ATTRIBUTES security_attr;
    int write_fd;

    if (s_stdout_redirected_to_debug || !IsDebuggerPresent())
    {
        return;
    }

    security_attr.nLength = sizeof(security_attr);
    security_attr.lpSecurityDescriptor = NULL;
    security_attr.bInheritHandle = FALSE;
    if (!CreatePipe(&read_pipe, &write_pipe, &security_attr, 0))
    {
        return;
    }

    thread = CreateThread(NULL, 0, egui_pc_stdout_debug_thread, read_pipe, 0, NULL);
    if (thread == NULL)
    {
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
        return;
    }
    CloseHandle(thread);

    write_fd = _open_osfhandle((intptr_t)write_pipe, _O_TEXT);
    if (write_fd < 0)
    {
        CloseHandle(write_pipe);
        return;
    }

    if (_dup2(write_fd, _fileno(stdout)) != 0)
    {
        _close(write_fd);
        return;
    }
    (void)_dup2(write_fd, _fileno(stderr));
    _close(write_fd);

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    s_stdout_redirected_to_debug = true;
#endif
}

void egui_pc_vlog(const char *format, va_list args)
{
#if defined(_WIN32) && EGUI_PC_LOG_TO_DEBUG_OUTPUT
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    buffer[sizeof(buffer) - 1] = '\0';
    egui_pc_output_debug_string(buffer);
    if (!s_stdout_redirected_to_debug && egui_pc_stdout_available())
    {
        fputs(buffer, stdout);
        fflush(stdout);
    }
#else
    vprintf(format, args);
    fflush(stdout);
#endif
}

void egui_pc_log(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    egui_pc_vlog(format, args);
    va_end(args);
}

static inline uint8_t rgb565_expand5(uint8_t value)
{
    return (uint8_t)((value << 3) | (value >> 2));
}

static inline uint8_t rgb565_expand6(uint8_t value)
{
    return (uint8_t)((value << 2) | (value >> 4));
}

static inline uint32_t rgb888_pack(uint8_t red, uint8_t green, uint8_t blue)
{
    return ((uint32_t)red << 16) | ((uint32_t)green << 8) | (uint32_t)blue;
}

static inline uint8_t sdl_get_color_16_swap(egui_core_t *core_ctx)
{
    if (core_ctx != NULL)
    {
        return core_ctx->render.color_16_swap ? 1U : 0U;
    }

    return EGUI_CONFIG_COLOR_16_SWAP ? 1U : 0U;
}

static inline uint32_t rgb565_to_rgb888(uint16_t color, uint8_t color_16_swap)
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    if (color_16_swap)
    {
        red = rgb565_expand5((uint8_t)((color >> 3) & 0x1FU));
        green = rgb565_expand6((uint8_t)((((color & 0x7U) << 3) | ((color >> 13) & 0x7U)) & 0x3FU));
        blue = rgb565_expand5((uint8_t)((color >> 8) & 0x1FU));
    }
    else
    {
        red = rgb565_expand5((uint8_t)((color >> 11) & 0x1FU));
        green = rgb565_expand6((uint8_t)((color >> 5) & 0x3FU));
        blue = rgb565_expand5((uint8_t)(color & 0x1FU));
    }

    return rgb888_pack(red, green, blue);
}

static inline uint16_t rgb888_to_rgb565(uint32_t color, uint8_t color_16_swap)
{
    uint16_t rgb565 = (uint16_t)(((((color) & 0xFF0000U) >> 19U) << 11U) | ((((color) & 0xFF00U) >> 10U) << 5U) | (((color) & 0xFFU) >> 3U));

    if (color_16_swap)
    {
        rgb565 = (uint16_t)((rgb565 >> 8) | (rgb565 << 8));
    }

    return rgb565;
}

static inline uint32_t sdl_color_to_rgb888(egui_core_t *core_ctx, egui_color_int_t color)
{
#if VT_COLOR_DEPTH == 1
    return monochrome_2_RGB888(color);
#elif VT_COLOR_DEPTH == 8
    return GRAY8_2_RGB888(color);
#elif VT_COLOR_DEPTH == 16
    return rgb565_to_rgb888((uint16_t)color, sdl_get_color_16_swap(core_ctx));
#elif VT_COLOR_DEPTH == 24 || VT_COLOR_DEPTH == 32
    return (uint32_t)color;
#else
    return 0;
#endif
}

static inline egui_color_int_t sdl_rgb888_to_color(egui_core_t *core_ctx, uint32_t color)
{
#if VT_COLOR_DEPTH == 1
    return (egui_color_int_t)((color & 0x00FFFFFFU) ? 0 : 1);
#elif VT_COLOR_DEPTH == 8
    return (egui_color_int_t)(((((color & 0xff0000) >> 16)) + (((color & 0xff00) >> 8)) + (((color & 0xff)))) / 3);
#elif VT_COLOR_DEPTH == 16
    return (egui_color_int_t)rgb888_to_rgb565(color, sdl_get_color_16_swap(core_ctx));
#elif VT_COLOR_DEPTH == 24 || VT_COLOR_DEPTH == 32
    return (egui_color_int_t)color;
#else
    return 0;
#endif
}

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
#if VT_SDL_NATIVE_RGB565
static uint16_t tft_fb[VT_WIDTH * VT_HEIGHT];
static uint16_t sdl_present_fb[VT_WIDTH * VT_HEIGHT];
#define VT_FB_PIXEL_SIZE    sizeof(uint16_t)
#define VT_SDL_PIXEL_FORMAT SDL_PIXELFORMAT_RGB565
#else
static uint32_t tft_fb[VT_WIDTH * VT_HEIGHT];
static uint32_t sdl_present_fb[VT_WIDTH * VT_HEIGHT];
#define VT_FB_PIXEL_SIZE    sizeof(uint32_t)
#define VT_SDL_PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888
#endif

static void sdl_log_core_task_caller_failure(const char *context, int display_id)
{
    egui_pc_log("[PC_CORE_TASK_CALLER] context=%s display=%d reason=post_rejected\n", context != NULL ? context : "unknown", display_id);
}
static SDL_atomic_t sdl_inited;
static SDL_atomic_t sdl_refr_qry;
static SDL_atomic_t sdl_quit_qry;
static bool sdl_window_visible = false;
static SDL_atomic_t sdl_has_presentable_frame;

static bool sdl_mouse_left_down = false;
static SDL_mutex *sdl_touch_mutex = NULL;
static SDL_mutex *sdl_frame_mutex = NULL;
static SDL_mutex *sdl_recording_mutex = NULL;

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
typedef struct sdl_touch_event
{
    uint8_t point_count;
    egui_touch_driver_point_t points[EGUI_TOUCH_DRIVER_MAX_POINTS];
} sdl_touch_event_t;

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
typedef struct sdl_touch_finger_state
{
    SDL_FingerID id;
    uint8_t active;
    int16_t x;
    int16_t y;
} sdl_touch_finger_state_t;
#endif
#endif

#define SDL_TOUCH_EVENT_QUEUE_SIZE 64

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static sdl_touch_event_t sdl_touch_event_queue[SDL_TOUCH_EVENT_QUEUE_SIZE];
static uint8_t sdl_touch_event_head = 0;
static uint8_t sdl_touch_event_tail = 0;
static uint8_t sdl_touch_event_count = 0;
static sdl_touch_event_t sdl_touch_last_state = {0};
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
static sdl_touch_finger_state_t sdl_touch_fingers[EGUI_TOUCH_DRIVER_MAX_POINTS];
#endif
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void sdl_touch_event_set_point_array(sdl_touch_event_t *event, uint8_t point_count, const egui_touch_driver_point_t *points, int16_t fallback_x,
                                            int16_t fallback_y)
{
    if (event == NULL)
    {
        return;
    }

    if (point_count > EGUI_TOUCH_DRIVER_MAX_POINTS)
    {
        point_count = EGUI_TOUCH_DRIVER_MAX_POINTS;
    }

    EGUI_UNUSED(fallback_x);
    EGUI_UNUSED(fallback_y);
    event->point_count = point_count;
    for (uint8_t i = 0; i < EGUI_TOUCH_DRIVER_MAX_POINTS; i++)
    {
        if (i < point_count && points != NULL)
        {
            event->points[i] = points[i];
        }
        else
        {
            event->points[i].x = 0;
            event->points[i].y = 0;
            event->points[i].id = i;
            event->points[i].pressure = 0;
        }
    }
}

static void sdl_touch_event_set_points(sdl_touch_event_t *event, uint8_t point_count, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    egui_touch_driver_point_t points[EGUI_TOUCH_DRIVER_MAX_POINTS];

    if (point_count > 2)
    {
        point_count = 2;
    }

    memset(points, 0, sizeof(points));
    if (EGUI_TOUCH_DRIVER_MAX_POINTS > 0)
    {
        points[0].x = x1;
        points[0].y = y1;
        points[0].id = 0;
        points[0].pressure = point_count > 0 ? 1 : 0;
    }
#if EGUI_TOUCH_DRIVER_MAX_POINTS > 1
    points[1].x = x2;
    points[1].y = y2;
    points[1].id = 1;
    points[1].pressure = point_count > 1 ? 1 : 0;
#else
    EGUI_UNUSED(x2);
    EGUI_UNUSED(y2);
#endif
    sdl_touch_event_set_point_array(event, point_count, points, x1, y1);
}

static void sdl_touch_driver_data_from_event(egui_touch_driver_data_t *data, const sdl_touch_event_t *event)
{
    if (data == NULL || event == NULL)
    {
        return;
    }

    memset(data, 0, sizeof(*data));
    data->point_count = event->point_count;
    if (data->point_count > EGUI_TOUCH_DRIVER_MAX_POINTS)
    {
        data->point_count = EGUI_TOUCH_DRIVER_MAX_POINTS;
    }

    for (uint8_t i = 0; i < data->point_count; i++)
    {
        data->points[i] = event->points[i];
    }
}

static void sdl_touch_queue_push(sdl_touch_event_t *queue, uint8_t *head, uint8_t *tail, uint8_t *count, sdl_touch_event_t *last,
                                 const sdl_touch_event_t *event)
{
    if (queue == NULL || head == NULL || tail == NULL || count == NULL || last == NULL || event == NULL)
    {
        return;
    }

    *last = *event;

    if (*count >= SDL_TOUCH_EVENT_QUEUE_SIZE)
    {
        *head = (uint8_t)((*head + 1) % SDL_TOUCH_EVENT_QUEUE_SIZE);
        (*count)--;
    }

    queue[*tail] = *event;
    *tail = (uint8_t)((*tail + 1) % SDL_TOUCH_EVENT_QUEUE_SIZE);
    (*count)++;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
static int16_t sdl_touch_float_to_coord(float value, int16_t size)
{
    if (value <= 0.0f)
    {
        return 0;
    }
    if (value >= 1.0f)
    {
        return size > 0 ? (int16_t)(size - 1) : 0;
    }
    return (int16_t)(value * (float)size);
}

static void sdl_touch_event_from_fingers(sdl_touch_event_t *event, const sdl_touch_finger_state_t *fingers, int16_t width, int16_t height, int16_t fallback_x,
                                         int16_t fallback_y)
{
    uint8_t point_count = 0;
    egui_touch_driver_point_t points[EGUI_TOUCH_DRIVER_MAX_POINTS];

    if (event == NULL || fingers == NULL)
    {
        return;
    }

    EGUI_UNUSED(width);
    EGUI_UNUSED(height);
    memset(points, 0, sizeof(points));

    for (uint8_t i = 0; i < EGUI_TOUCH_DRIVER_MAX_POINTS; i++)
    {
        if (!fingers[i].active)
        {
            continue;
        }

        points[point_count].x = fingers[i].x;
        points[point_count].y = fingers[i].y;
        points[point_count].id = (uint8_t)i;
        points[point_count].pressure = 1;
        point_count++;
    }

    sdl_touch_event_set_point_array(event, point_count, points, fallback_x, fallback_y);
}

static int sdl_touch_find_finger(sdl_touch_finger_state_t *fingers, SDL_FingerID id)
{
    for (uint8_t i = 0; i < EGUI_TOUCH_DRIVER_MAX_POINTS; i++)
    {
        if (fingers[i].active && fingers[i].id == id)
        {
            return i;
        }
    }
    return -1;
}

static int sdl_touch_alloc_finger(sdl_touch_finger_state_t *fingers, SDL_FingerID id)
{
    int existing = sdl_touch_find_finger(fingers, id);
    if (existing >= 0)
    {
        return existing;
    }

    for (uint8_t i = 0; i < EGUI_TOUCH_DRIVER_MAX_POINTS; i++)
    {
        if (!fingers[i].active)
        {
            fingers[i].id = id;
            fingers[i].active = 1;
            return i;
        }
    }
    return -1;
}

static void sdl_touch_update_fingers(sdl_touch_finger_state_t *fingers, uint32_t event_type, SDL_FingerID id, int16_t x, int16_t y)
{
    int index;

    if (event_type == SDL_FINGERUP)
    {
        index = sdl_touch_find_finger(fingers, id);
        if (index >= 0)
        {
            fingers[index].active = 0;
            fingers[index].x = x;
            fingers[index].y = y;
        }
        return;
    }

    index = sdl_touch_alloc_finger(fingers, id);
    if (index >= 0)
    {
        fingers[index].x = x;
        fingers[index].y = y;
    }
}
#endif
#endif

/* ---- Multi-display SDL support ---- */
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1

#define SDL_MAX_EXTRA_DISPLAYS (EGUI_CONFIG_MAX_DISPLAY_COUNT - 1)

typedef struct sdl_extra_display
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    uint32_t *tft_fb;
    uint32_t *present_fb;
    SDL_mutex *frame_mutex;
    SDL_mutex *touch_mutex;
    int16_t width;
    int16_t height;
    uint32_t sdl_window_id;
    bool has_presentable_frame;
    bool window_visible;
    bool mouse_left_down;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    sdl_touch_event_t touch_queue[SDL_TOUCH_EVENT_QUEUE_SIZE];
    uint8_t touch_head;
    uint8_t touch_tail;
    uint8_t touch_count;
    sdl_touch_event_t touch_last;
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    sdl_touch_finger_state_t touch_fingers[EGUI_TOUCH_DRIVER_MAX_POINTS];
#endif
#endif
} sdl_extra_display_t;

static sdl_extra_display_t sdl_extra[SDL_MAX_EXTRA_DISPLAYS];
static int sdl_extra_count = 0;
static uint32_t sdl_display0_window_id = 0;

static int sdl_get_display_for_window(uint32_t window_id)
{
    if (sdl_display0_window_id == window_id)
    {
        return 0;
    }
    for (int i = 0; i < sdl_extra_count; i++)
    {
        if (sdl_extra[i].sdl_window_id == window_id)
        {
            return i + 1;
        }
    }
    return -1;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void sdl_extra_touch_push(int idx, uint8_t pressed, int16_t x, int16_t y)
{
    sdl_extra_display_t *ctx = &sdl_extra[idx];
    sdl_touch_event_t event;

    SDL_LockMutex(ctx->touch_mutex);
    sdl_touch_event_set_points(&event, pressed ? 1 : 0, x, y, 0, 0);
    sdl_touch_queue_push(ctx->touch_queue, &ctx->touch_head, &ctx->touch_tail, &ctx->touch_count, &ctx->touch_last, &event);
    SDL_UnlockMutex(ctx->touch_mutex);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static void sdl_extra_touch_push_points(int idx, uint8_t point_count, const egui_touch_driver_point_t *points, int16_t fallback_x, int16_t fallback_y)
{
    sdl_extra_display_t *ctx = &sdl_extra[idx];
    sdl_touch_event_t event;

    SDL_LockMutex(ctx->touch_mutex);
    sdl_touch_event_set_point_array(&event, point_count, points, fallback_x, fallback_y);
    sdl_touch_queue_push(ctx->touch_queue, &ctx->touch_head, &ctx->touch_tail, &ctx->touch_count, &ctx->touch_last, &event);
    SDL_UnlockMutex(ctx->touch_mutex);
}
#endif

static void sdl_extra_touch_push_finger(int idx, uint32_t event_type, SDL_FingerID id, int16_t x, int16_t y)
{
    sdl_extra_display_t *ctx = &sdl_extra[idx];
    sdl_touch_event_t event;

    SDL_LockMutex(ctx->touch_mutex);
    sdl_touch_update_fingers(ctx->touch_fingers, event_type, id, x, y);
    sdl_touch_event_from_fingers(&event, ctx->touch_fingers, ctx->width, ctx->height, x, y);
    sdl_touch_queue_push(ctx->touch_queue, &ctx->touch_head, &ctx->touch_tail, &ctx->touch_count, &ctx->touch_last, &event);
    SDL_UnlockMutex(ctx->touch_mutex);
}
#endif

static int sdl_extra_touch_read(int idx, egui_touch_driver_data_t *data)
{
    sdl_extra_display_t *ctx = &sdl_extra[idx];
    sdl_touch_event_t event;

    if (data == NULL)
    {
        return -1;
    }

    SDL_LockMutex(ctx->touch_mutex);
    if (ctx->touch_count > 0)
    {
        event = ctx->touch_queue[ctx->touch_head];
        ctx->touch_head = (uint8_t)((ctx->touch_head + 1) % SDL_TOUCH_EVENT_QUEUE_SIZE);
        ctx->touch_count--;
        ctx->touch_last = event;
    }
    else
    {
        event = ctx->touch_last;
    }
    SDL_UnlockMutex(ctx->touch_mutex);

    sdl_touch_driver_data_from_event(data, &event);
    return 0;
}
#endif /* EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH */

static void sdl_extra_fill_colors(int idx, int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t *color_p)
{
    sdl_extra_display_t *ctx = &sdl_extra[idx];
    egui_core_t *core_ctx = egui_port_get_core_by_display_id(idx + 1);
    int16_t w = ctx->width;
    int16_t h = ctx->height;

    if (x2 < 0 || y2 < 0 || x1 > w - 1 || y1 > h - 1)
    {
        return;
    }

    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > w - 1 ? w - 1 : x2;
    int32_t act_y2 = y2 > h - 1 ? h - 1 : y2;

    for (int32_t y = act_y1; y <= act_y2; y++)
    {
        for (int32_t x = act_x1; x <= act_x2; x++)
        {
            ctx->tft_fb[y * w + x] = 0xff000000U | sdl_color_to_rgb888(core_ctx, *color_p);
            color_p++;
        }
        color_p += x2 - act_x2;
    }
}

static void sdl_extra_fill_single_color(int idx, int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t color)
{
    sdl_extra_display_t *ctx = &sdl_extra[idx];
    egui_core_t *core_ctx = egui_port_get_core_by_display_id(idx + 1);
    int16_t w = ctx->width;
    int16_t h = ctx->height;

    if (x2 < 0 || y2 < 0 || x1 > w - 1 || y1 > h - 1)
    {
        return;
    }

    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > w - 1 ? w - 1 : x2;
    int32_t act_y2 = y2 > h - 1 ? h - 1 : y2;

    for (int32_t y = act_y1; y <= act_y2; y++)
    {
        for (int32_t x = act_x1; x <= act_x2; x++)
        {
            ctx->tft_fb[y * w + x] = 0xff000000U | sdl_color_to_rgb888(core_ctx, color);
        }
    }
}

static void sdl_extra_set_point(int idx, int32_t x, int32_t y, egui_color_int_t color)
{
    sdl_extra_display_t *ctx = &sdl_extra[idx];
    egui_core_t *core_ctx = egui_port_get_core_by_display_id(idx + 1);
    int16_t w = ctx->width;
    int16_t h = ctx->height;

    if (x < 0 || y < 0 || x > w - 1 || y > h - 1)
    {
        return;
    }

    ctx->tft_fb[y * w + x] = 0xff000000U | sdl_color_to_rgb888(core_ctx, color);
}

static egui_color_int_t sdl_extra_get_point(int idx, int32_t x, int32_t y)
{
    sdl_extra_display_t *ctx = &sdl_extra[idx];
    egui_core_t *core_ctx = egui_port_get_core_by_display_id(idx + 1);
    int16_t w = ctx->width;
    int16_t h = ctx->height;

    if (x < 0 || y < 0 || x > w - 1 || y > h - 1)
    {
        return 0;
    }

    return sdl_rgb888_to_color(core_ctx, ctx->tft_fb[y * w + x]);
}

static void sdl_extra_commit_frame(int idx)
{
    sdl_extra_display_t *ctx = &sdl_extra[idx];
    SDL_LockMutex(ctx->frame_mutex);
    memcpy(ctx->present_fb, ctx->tft_fb, (size_t)ctx->width * ctx->height * sizeof(uint32_t));
    ctx->has_presentable_frame = true;
    SDL_UnlockMutex(ctx->frame_mutex);
}

static bool sdl_extra_upload_committed_frame(int idx)
{
    sdl_extra_display_t *ctx = &sdl_extra[idx];
    bool uploaded = false;

    SDL_LockMutex(ctx->frame_mutex);
    if (ctx->has_presentable_frame)
    {
        SDL_UpdateTexture(ctx->texture, NULL, ctx->present_fb, ctx->width * (int)sizeof(uint32_t));
        ctx->has_presentable_frame = false;
        uploaded = true;
    }
    SDL_UnlockMutex(ctx->frame_mutex);

    return uploaded;
}

static void sdl_extra_present_frame(int idx)
{
    sdl_extra_display_t *ctx = &sdl_extra[idx];

    SDL_RenderClear(ctx->renderer);
    SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
    SDL_RenderPresent(ctx->renderer);
}

static void sdl_extra_present_if_dirty(int idx)
{
    if (sdl_extra_upload_committed_frame(idx))
    {
        sdl_extra_present_frame(idx);
    }
}

void sdl_port_add_display(int display_id, int16_t w, int16_t h)
{
    int idx = display_id - 1;

    if (idx < 0 || idx >= SDL_MAX_EXTRA_DISPLAYS)
    {
        return;
    }

    sdl_extra_display_t *ctx = &sdl_extra[idx];
    memset(ctx, 0, sizeof(sdl_extra_display_t));

    ctx->width = w;
    ctx->height = h;

    char title[256];
    sprintf(title, "%s-Display%d-%dx%d@%d", EGUI_APP, display_id, w, h, EGUI_CONFIG_COLOR_DEPTH);

    ctx->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, 0);
    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, 0);
    ctx->texture = SDL_CreateTexture(ctx->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
    SDL_SetTextureBlendMode(ctx->texture, SDL_BLENDMODE_BLEND);

    size_t fb_size = (size_t)w * h;
    ctx->tft_fb = (uint32_t *)malloc(fb_size * sizeof(uint32_t));
    ctx->present_fb = (uint32_t *)malloc(fb_size * sizeof(uint32_t));
    memset(ctx->tft_fb, 77, fb_size * sizeof(uint32_t));
    memset(ctx->present_fb, 77, fb_size * sizeof(uint32_t));
    SDL_UpdateTexture(ctx->texture, NULL, ctx->present_fb, w * (int)sizeof(uint32_t));

    ctx->frame_mutex = SDL_CreateMutex();
    ctx->touch_mutex = SDL_CreateMutex();
    ctx->has_presentable_frame = false;
    ctx->window_visible = true;
    ctx->sdl_window_id = SDL_GetWindowID(ctx->window);

    if (idx >= sdl_extra_count)
    {
        sdl_extra_count = idx + 1;
    }
}

#endif /* EGUI_CONFIG_MAX_DISPLAY_COUNT > 1 */

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void sdl_touch_lock(void)
{
    if (sdl_touch_mutex == NULL)
    {
        sdl_touch_mutex = SDL_CreateMutex();
    }
    if (sdl_touch_mutex != NULL)
    {
        SDL_LockMutex(sdl_touch_mutex);
    }
}

static void sdl_touch_unlock(void)
{
    if (sdl_touch_mutex != NULL)
    {
        SDL_UnlockMutex(sdl_touch_mutex);
    }
}

static void sdl_port_touch_push_event(uint8_t pressed, int16_t x, int16_t y)
{
    sdl_touch_event_t event;

    sdl_touch_lock();

    sdl_touch_event_set_points(&event, pressed ? 1 : 0, x, y, 0, 0);
    sdl_touch_queue_push(sdl_touch_event_queue, &sdl_touch_event_head, &sdl_touch_event_tail, &sdl_touch_event_count, &sdl_touch_last_state, &event);

    sdl_touch_unlock();
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static void sdl_port_touch_push_points(uint8_t point_count, const egui_touch_driver_point_t *points, int16_t fallback_x, int16_t fallback_y)
{
    sdl_touch_event_t event;

    sdl_touch_lock();
    sdl_touch_event_set_point_array(&event, point_count, points, fallback_x, fallback_y);
    sdl_touch_queue_push(sdl_touch_event_queue, &sdl_touch_event_head, &sdl_touch_event_tail, &sdl_touch_event_count, &sdl_touch_last_state, &event);
    sdl_touch_unlock();
}
#endif

static void sdl_port_touch_push_finger(uint32_t event_type, SDL_FingerID id, int16_t x, int16_t y)
{
    sdl_touch_event_t event;

    sdl_touch_lock();
    sdl_touch_update_fingers(sdl_touch_fingers, event_type, id, x, y);
    sdl_touch_event_from_fingers(&event, sdl_touch_fingers, VT_WIDTH, VT_HEIGHT, x, y);
    sdl_touch_queue_push(sdl_touch_event_queue, &sdl_touch_event_head, &sdl_touch_event_tail, &sdl_touch_event_count, &sdl_touch_last_state, &event);
    sdl_touch_unlock();
}
#endif

int sdl_port_touch_read(egui_core_t *core_ctx, egui_touch_driver_data_t *data)
{
    sdl_touch_event_t event;

    if (data == NULL)
    {
        return -1;
    }

#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    int disp_id = core_ctx != NULL ? core_ctx->id : 0;
    if (disp_id > 0 && disp_id <= sdl_extra_count)
    {
        return sdl_extra_touch_read(disp_id - 1, data);
    }
#else
    EGUI_UNUSED(core_ctx);
#endif

    sdl_touch_lock();
    if (sdl_touch_event_count > 0)
    {
        event = sdl_touch_event_queue[sdl_touch_event_head];
        sdl_touch_event_head = (uint8_t)((sdl_touch_event_head + 1) % SDL_TOUCH_EVENT_QUEUE_SIZE);
        sdl_touch_event_count--;
        sdl_touch_last_state = event;
    }
    else
    {
        event = sdl_touch_last_state;
    }
    sdl_touch_unlock();

    sdl_touch_driver_data_from_event(data, &event);

    return 0;
}
#endif /* EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH */

// Recording state for GIF generation
#define RECORDING_MAX_FRAMES 1000
static bool g_recording_enabled = false;
static int g_recording_fps = 10;
static int g_recording_duration_ms = 10000;
static int g_recording_speed = 1; // Speed multiplier for action intervals
static int g_recording_clock_scale = 1;
static int g_recording_frame_count = 0;
static uint32_t g_recording_start_time = 0;
static uint32_t g_recording_last_frame_time = 0;
static char g_recording_output_dir[512] = "";
static bool g_sdl_headless = false;
static int g_recording_snapshot_settle_ms = 0;
static uint32_t g_recording_clock_anchor_real_ms = 0;
static uint32_t g_recording_clock_anchor_scaled_ms = 0;

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
// Auto-action simulation for recording
static uint32_t g_recording_last_action_time = 0;
static int g_recording_action_index = 0;
// Drag state
static bool g_recording_drag_in_progress = false;
static bool g_recording_click_release_pending = false;
static int g_recording_drag_current_step = 0;
static egui_sim_action_t g_recording_current_action;
// Snapshot-driven frame capture (replaces fixed-time settle)
static bool g_recording_snapshot_requested = false; // Snapshot requested by user code or auto-fallback
static bool g_recording_snapshot_blocks_actions = false;
static uint32_t g_recording_snapshot_request_time = 0;
static uint32_t g_recording_snapshot_request_frame_count = 0;
static uint32_t g_recording_snapshot_last_hash = 0;
static int g_recording_snapshot_same_hash_count = 0;
static bool g_recording_snapshot_seen_change = false;
static int g_recording_snapshot_stable_cycles = 1;
static int g_recording_snapshot_max_wait_ms = 1500;
static bool g_recording_quit_after_snapshot = false; // Quit after next snapshot (all actions done)
static bool g_recording_all_actions_done = false;    // No more actions to simulate
static uint32_t g_recording_completed_frame_count = 0;
static uint32_t g_recording_start_real_time = 0;
#endif

extern void VT_Init(void);
extern void VT_Fill_Single_Color(int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t color);
extern void VT_Fill_Multiple_Colors(int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t *color_p);
extern void VT_Set_Point(int32_t x, int32_t y, egui_color_int_t color);
extern egui_color_int_t VT_Get_Point(int32_t x, int32_t y);
extern void VT_Clear(egui_color_int_t color);
extern bool VT_Mouse_Get_Point(int16_t *x, int16_t *y);

void snap_shot(const char *file_name);
static void recording_save_frame(void);
static void sdl_port_present_frame(void);
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static void recording_simulate_action(void);
static uint32_t recording_calc_frame_hash(void);
static void recording_request_snapshot_internal(bool block_actions);
#endif
static uint32_t recording_apply_clock_scale(uint32_t real_ms);

static bool sdl_atomic_flag_get(SDL_atomic_t *flag)
{
    return SDL_AtomicGet(flag) != 0;
}

#define SHUTDOWN_MARKER_PREFIX "[SHUTDOWN_CHECK]"

static void sdl_atomic_flag_set(SDL_atomic_t *flag, bool value)
{
    SDL_AtomicSet(flag, value ? 1 : 0);
}

static void recording_lock(void)
{
    if (sdl_recording_mutex != NULL)
    {
        SDL_LockMutex(sdl_recording_mutex);
    }
}

static void recording_unlock(void)
{
    if (sdl_recording_mutex != NULL)
    {
        SDL_UnlockMutex(sdl_recording_mutex);
    }
}

int quit_filter(void *userdata, SDL_Event *event)
{
    (void)userdata;

    if (event->type == SDL_QUIT)
    {
        sdl_atomic_flag_set(&sdl_quit_qry, true);
    }

    return 1;
}

static void monitor_sdl_clean_up(void)
{
    int extra_window_count = 0;
    int primary_window_destroyed = window != NULL ? 1 : 0;

    sdl_atomic_flag_set(&sdl_inited, false);

#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    extra_window_count = sdl_extra_count;
    for (int i = 0; i < sdl_extra_count; i++)
    {
        sdl_extra_display_t *ctx = &sdl_extra[i];
        if (ctx->texture != NULL)
        {
            SDL_DestroyTexture(ctx->texture);
            ctx->texture = NULL;
        }
        if (ctx->renderer != NULL)
        {
            SDL_DestroyRenderer(ctx->renderer);
            ctx->renderer = NULL;
        }
        if (ctx->window != NULL)
        {
            SDL_DestroyWindow(ctx->window);
            ctx->window = NULL;
        }
        if (ctx->frame_mutex != NULL)
        {
            SDL_DestroyMutex(ctx->frame_mutex);
            ctx->frame_mutex = NULL;
        }
        if (ctx->touch_mutex != NULL)
        {
            SDL_DestroyMutex(ctx->touch_mutex);
            ctx->touch_mutex = NULL;
        }
        free(ctx->tft_fb);
        ctx->tft_fb = NULL;
        free(ctx->present_fb);
        ctx->present_fb = NULL;
    }
    sdl_extra_count = 0;
#endif

    SDL_DestroyTexture(texture);
    texture = NULL;
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
    SDL_DestroyWindow(window);
    window = NULL;
    if (sdl_touch_mutex != NULL)
    {
        SDL_DestroyMutex(sdl_touch_mutex);
        sdl_touch_mutex = NULL;
    }
    if (sdl_frame_mutex != NULL)
    {
        SDL_DestroyMutex(sdl_frame_mutex);
        sdl_frame_mutex = NULL;
    }
    if (sdl_recording_mutex != NULL)
    {
        SDL_DestroyMutex(sdl_recording_mutex);
        sdl_recording_mutex = NULL;
    }
    egui_pc_log("%s sdl_cleanup primary_window=%d extra_windows=%d\n", SHUTDOWN_MARKER_PREFIX, primary_window_destroyed, extra_window_count);
    SDL_Quit();
}

static void sdl_frame_lock(void)
{
    if (sdl_frame_mutex != NULL)
    {
        SDL_LockMutex(sdl_frame_mutex);
    }
}

static void sdl_frame_unlock(void)
{
    if (sdl_frame_mutex != NULL)
    {
        SDL_UnlockMutex(sdl_frame_mutex);
    }
}

static void sdl_port_commit_frame(egui_core_t *core_ctx)
{
    EGUI_UNUSED(core_ctx);
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    int disp_id = core_ctx != NULL ? core_ctx->id : 0;
    if (disp_id > 0 && disp_id <= sdl_extra_count)
    {
        sdl_extra_commit_frame(disp_id - 1);
        return;
    }
#endif
    sdl_frame_lock();
    memcpy(sdl_present_fb, tft_fb, sizeof(sdl_present_fb));
    sdl_atomic_flag_set(&sdl_has_presentable_frame, true);
    sdl_frame_unlock();
}

static void sdl_port_upload_committed_frame(void)
{
    sdl_frame_lock();
    if (sdl_atomic_flag_get(&sdl_has_presentable_frame))
    {
        SDL_UpdateTexture(texture, NULL, sdl_present_fb, VT_WIDTH * VT_FB_PIXEL_SIZE);
    }
    sdl_frame_unlock();
}

static void monitor_sdl_init(void)
{

    /*Initialize the SDL*/
    SDL_Init(SDL_INIT_VIDEO);
    if (sdl_touch_mutex == NULL)
    {
        sdl_touch_mutex = SDL_CreateMutex();
    }
    if (sdl_frame_mutex == NULL)
    {
        sdl_frame_mutex = SDL_CreateMutex();
    }
    if (sdl_recording_mutex == NULL)
    {
        sdl_recording_mutex = SDL_CreateMutex();
    }

    SDL_SetEventFilter(quit_filter, NULL);

    char title[0x400];
    sprintf(title, "%s-%dx%d@%d", EGUI_APP, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT, EGUI_CONFIG_COLOR_DEPTH);

    uint32_t window_flags = SDL_WINDOW_HIDDEN;

#ifdef __EMSCRIPTEN__
    if (!g_sdl_headless)
    {
        window_flags = 0;
    }
#endif

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, VT_WIDTH, VT_HEIGHT,
                              window_flags); /*last param. SDL_WINDOW_BORDERLESS to hide borders*/

    if (g_sdl_headless)
    {
        SDL_HideWindow(window);
    }

#if VT_VIRTUAL_MACHINE || defined(__EMSCRIPTEN__)
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
#else
    renderer = SDL_CreateRenderer(window, -1, 0);
#endif
    texture = SDL_CreateTexture(renderer, VT_SDL_PIXEL_FORMAT, SDL_TEXTUREACCESS_STATIC, VT_WIDTH, VT_HEIGHT);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    /*Initialize the frame buffer to gray (77 is an empirical value) */
    memset(tft_fb, 77, VT_WIDTH * VT_HEIGHT * VT_FB_PIXEL_SIZE);
    memset(sdl_present_fb, 77, VT_WIDTH * VT_HEIGHT * VT_FB_PIXEL_SIZE);
    SDL_UpdateTexture(texture, NULL, sdl_present_fb, VT_WIDTH * VT_FB_PIXEL_SIZE);

    sdl_atomic_flag_set(&sdl_has_presentable_frame, false);
    sdl_window_visible = (window_flags & SDL_WINDOW_HIDDEN) == 0;
    sdl_atomic_flag_set(&sdl_refr_qry, false);
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    sdl_display0_window_id = SDL_GetWindowID(window);
#endif
    sdl_atomic_flag_set(&sdl_inited, true);
}

static void sdl_port_present_frame(void)
{
    if (!g_sdl_headless && !sdl_window_visible && sdl_atomic_flag_get(&sdl_has_presentable_frame))
    {
        SDL_ShowWindow(window);
        sdl_window_visible = true;
    }

    SDL_RenderClear(renderer);

    /*Update the renderer with the texture containing the rendered image*/
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static void sdl_port_present_window_frame(uint32_t window_id)
{
    if (g_sdl_headless)
    {
        return;
    }

#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    if (window_id != sdl_display0_window_id)
    {
        int disp_id = sdl_get_display_for_window(window_id);
        if (disp_id > 0 && disp_id <= sdl_extra_count)
        {
            sdl_extra_upload_committed_frame(disp_id - 1);
            sdl_extra_present_frame(disp_id - 1);
            return;
        }
        if (disp_id < 0)
        {
            return;
        }
    }
#else
    EGUI_UNUSED(window_id);
#endif

    if (sdl_atomic_flag_get(&sdl_has_presentable_frame))
    {
        sdl_port_upload_committed_frame();
    }
    sdl_port_present_frame();
}

#if defined(_MSC_VER)
void egui_port_hanlde_key_event(int key, int event);
void egui_port_hanlde_key_event_default(int key, int event)
#else
__EGUI_WEAK__ void egui_port_hanlde_key_event(int key, int event)
#endif
{
    EGUI_UNUSED(event);
    EGUI_UNUSED(key);
    // printf("key event: %d, %d\n", key, event);
}
__EGUI_MSVC_ALTERNATE_NAME(egui_port_hanlde_key_event, egui_port_hanlde_key_event_default)

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
typedef struct sdl_scroll_task_data
{
    egui_dim_t x;
    egui_dim_t y;
    int16_t delta;
} sdl_scroll_task_data_t;

static void sdl_core_scroll_task(egui_core_t *core, uintptr_t user_data)
{
    sdl_scroll_task_data_t *task_data = (sdl_scroll_task_data_t *)user_data;

    if (core != NULL && task_data != NULL)
    {
        egui_input_add_scroll(core, task_data->x, task_data->y, task_data->delta);
    }

    if (task_data != NULL)
    {
        free(task_data);
    }
}

static int sdl_post_scroll_task(egui_core_t *core, egui_dim_t x, egui_dim_t y, int16_t delta)
{
    sdl_scroll_task_data_t *task_data;

    if (core == NULL)
    {
        return 0;
    }

    task_data = (sdl_scroll_task_data_t *)malloc(sizeof(sdl_scroll_task_data_t));
    if (task_data == NULL)
    {
        return 0;
    }

    task_data->x = x;
    task_data->y = y;
    task_data->delta = delta;
    if (!egui_port_post_core_task(core, sdl_core_scroll_task, (uintptr_t)task_data))
    {
        free(task_data);
        return 0;
    }

    return 1;
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY

static uintptr_t sdl_pack_key_task_data(uint8_t key_type, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl)
{
    return ((uintptr_t)key_type) | ((uintptr_t)key_code << 8) | ((uintptr_t)is_shift << 16) | ((uintptr_t)is_ctrl << 24);
}

static void sdl_core_key_task(egui_core_t *core, uintptr_t user_data)
{
    uint8_t key_type = (uint8_t)(user_data & 0xFFu);
    uint8_t key_code = (uint8_t)((user_data >> 8) & 0xFFu);
    uint8_t is_shift = (uint8_t)((user_data >> 16) & 0xFFu);
    uint8_t is_ctrl = (uint8_t)((user_data >> 24) & 0xFFu);

    if (core != NULL)
    {
        egui_input_add_key(core, key_type, key_code, is_shift, is_ctrl);
    }
}
#endif

static void sdl_core_screen_power_task(egui_core_t *core, uintptr_t user_data)
{
    if (core == NULL)
    {
        return;
    }

    if (user_data == 2)
    {
        if (egui_core_is_suspended(core))
        {
            egui_screen_on(core);
        }
        else
        {
            egui_screen_off(core);
        }
    }
    else if (user_data != 0)
    {
        egui_screen_on(core);
    }
    else
    {
        egui_screen_off(core);
    }
}

void VT_sdl_refresh_task(void)
{
    if (sdl_atomic_flag_get(&sdl_refr_qry))
    {
        sdl_atomic_flag_set(&sdl_refr_qry, false);
        if (sdl_atomic_flag_get(&sdl_has_presentable_frame))
        {
            sdl_port_upload_committed_frame();
            sdl_port_present_frame();
        }
    }

#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    /* Refresh extra displays */
    for (int i = 0; i < sdl_extra_count; i++)
    {
        sdl_extra_present_if_dirty(i);
    }
#endif

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {

        switch ((&event)->type)
        {
        case SDL_MOUSEBUTTONUP:
            if ((&event)->button.button == SDL_BUTTON_LEFT)
            {
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
                int target_up = sdl_get_display_for_window((&event)->button.windowID);
                if (target_up == 0)
                {
                    sdl_mouse_left_down = false;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                    sdl_port_touch_push_event(0, (&event)->button.x, (&event)->button.y);
#endif
                }
                else if (target_up > 0 && target_up <= sdl_extra_count)
                {
                    sdl_extra[target_up - 1].mouse_left_down = false;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                    sdl_extra_touch_push(target_up - 1, 0, (&event)->button.x, (&event)->button.y);
#endif
                }
                else
                {
                    break;
                }
#else
                sdl_mouse_left_down = false;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                sdl_port_touch_push_event(0, (&event)->button.x, (&event)->button.y);
#endif
#endif
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if ((&event)->button.button == SDL_BUTTON_LEFT)
            {
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
                int target_down = sdl_get_display_for_window((&event)->button.windowID);
                if (target_down == 0)
                {
                    sdl_mouse_left_down = true;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                    sdl_port_touch_push_event(1, (&event)->button.x, (&event)->button.y);
#endif
                }
                else if (target_down > 0 && target_down <= sdl_extra_count)
                {
                    sdl_extra[target_down - 1].mouse_left_down = true;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                    sdl_extra_touch_push(target_down - 1, 1, (&event)->button.x, (&event)->button.y);
#endif
                }
                else
                {
                    break;
                }
#else
                sdl_mouse_left_down = true;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                sdl_port_touch_push_event(1, (&event)->button.x, (&event)->button.y);
#endif
#endif
            }
            break;
        case SDL_MOUSEMOTION:
        {
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
            int target_motion = sdl_get_display_for_window((&event)->motion.windowID);
            if (target_motion == 0)
            {
                if (sdl_mouse_left_down)
                {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                    sdl_port_touch_push_event(1, (&event)->motion.x, (&event)->motion.y);
#endif
                }
            }
            else if (target_motion > 0 && target_motion <= sdl_extra_count)
            {
                if (sdl_extra[target_motion - 1].mouse_left_down)
                {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                    sdl_extra_touch_push(target_motion - 1, 1, (&event)->motion.x, (&event)->motion.y);
#endif
                }
            }
            else
            {
                break;
            }
#else
            if (sdl_mouse_left_down)
            {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                sdl_port_touch_push_event(1, (&event)->motion.x, (&event)->motion.y);
#endif
            }
#endif
            break;
        }

        case SDL_MOUSEWHEEL:
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
            int mx, my;
            egui_core_t *target_core = NULL;
            int target_display = 0;
            SDL_GetMouseState(&mx, &my);
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
            target_display = sdl_get_display_for_window((&event)->wheel.windowID);
#endif
            target_core = egui_port_get_core_by_display_id(target_display);
            if (target_core != NULL)
            {
                if (!sdl_post_scroll_task(target_core, (egui_dim_t)mx, (egui_dim_t)my, (int16_t)(&event)->wheel.y))
                {
                    sdl_log_core_task_caller_failure("scroll_event", target_display);
                }
            }
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
            break;
        }

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
        case SDL_FINGERDOWN:
        case SDL_FINGERMOTION:
        case SDL_FINGERUP:
        {
            int16_t tx;
            int16_t ty;
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
            int target_touch = sdl_get_display_for_window((&event)->tfinger.windowID);
            if (target_touch > 0 && target_touch <= sdl_extra_count)
            {
                tx = sdl_touch_float_to_coord((&event)->tfinger.x, sdl_extra[target_touch - 1].width);
                ty = sdl_touch_float_to_coord((&event)->tfinger.y, sdl_extra[target_touch - 1].height);
                sdl_extra_touch_push_finger(target_touch - 1, (&event)->type, (&event)->tfinger.fingerId, tx, ty);
                break;
            }
            if (target_touch < 0)
            {
                break;
            }
#endif
            tx = sdl_touch_float_to_coord((&event)->tfinger.x, VT_WIDTH);
            ty = sdl_touch_float_to_coord((&event)->tfinger.y, VT_HEIGHT);
            sdl_port_touch_push_finger((&event)->type, (&event)->tfinger.fingerId, tx, ty);
            break;
        }
#endif

        case SDL_WINDOWEVENT:
            switch ((&event)->window.event)
            {
#if SDL_VERSION_ATLEAST(2, 0, 5)
            case SDL_WINDOWEVENT_TAKE_FOCUS:
#endif
            case SDL_WINDOWEVENT_EXPOSED:
                sdl_port_present_window_frame(event.window.windowID);
                break;
            case SDL_WINDOWEVENT_CLOSE:
                sdl_atomic_flag_set(&sdl_quit_qry, true);
                break;
            default:
                break;
            }
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            // Keep legacy weak function for backwards compatibility
            if (event.key.keysym.sym >= '0' && event.key.keysym.sym <= '9')
            {
                egui_port_hanlde_key_event(event.key.keysym.sym - '0', (event.type == SDL_KEYDOWN) ? 1 : 0);
            }

            // F12: toggle screen on/off (demo power management)
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F12)
            {
                egui_core_t *target_core = NULL;
                int target_display = 0;
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
                target_display = sdl_get_display_for_window(event.key.windowID);
#endif
                target_core = egui_port_get_core_by_display_id(target_display);
                if (target_core != NULL)
                {
                    if (!egui_port_post_core_task(target_core, sdl_core_screen_power_task, 2))
                    {
                        sdl_log_core_task_caller_failure("screen_power", target_display);
                    }
                }
            }

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
            uint8_t key_type = (event.type == SDL_KEYDOWN) ? EGUI_KEY_EVENT_ACTION_DOWN : EGUI_KEY_EVENT_ACTION_UP;
            uint8_t is_shift = (event.key.keysym.mod & KMOD_SHIFT) ? 1 : 0;
            uint8_t is_ctrl = (event.key.keysym.mod & KMOD_CTRL) ? 1 : 0;
            uint8_t key_code = EGUI_KEY_CODE_NONE;
            egui_core_t *target_core = NULL;
            int target_display = 0;
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
            target_display = sdl_get_display_for_window(event.key.windowID);
#endif
            target_core = egui_port_get_core_by_display_id(target_display);
            if (event.type == SDL_KEYDOWN && event.key.repeat)
            {
                key_type = EGUI_KEY_EVENT_ACTION_REPEAT;
            }
            switch (event.key.keysym.sym)
            {
            // Navigation
            case SDLK_UP:
                key_code = EGUI_KEY_CODE_UP;
                break;
            case SDLK_DOWN:
                key_code = EGUI_KEY_CODE_DOWN;
                break;
            case SDLK_LEFT:
                key_code = EGUI_KEY_CODE_LEFT;
                break;
            case SDLK_RIGHT:
                key_code = EGUI_KEY_CODE_RIGHT;
                break;
            case SDLK_TAB:
                key_code = EGUI_KEY_CODE_TAB;
                break;
            // Action
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                key_code = EGUI_KEY_CODE_ENTER;
                break;
            case SDLK_ESCAPE:
                key_code = EGUI_KEY_CODE_ESCAPE;
                break;
            case SDLK_BACKSPACE:
                key_code = EGUI_KEY_CODE_BACKSPACE;
                break;
            case SDLK_DELETE:
                key_code = EGUI_KEY_CODE_DELETE;
                break;
            case SDLK_SPACE:
                key_code = EGUI_KEY_CODE_SPACE;
                break;
            case SDLK_HOME:
                key_code = EGUI_KEY_CODE_HOME;
                break;
            case SDLK_END:
                key_code = EGUI_KEY_CODE_END;
                break;
            // Digits
            case SDLK_0:
            case SDLK_KP_0:
                key_code = EGUI_KEY_CODE_0;
                break;
            case SDLK_1:
            case SDLK_KP_1:
                key_code = EGUI_KEY_CODE_1;
                break;
            case SDLK_2:
            case SDLK_KP_2:
                key_code = EGUI_KEY_CODE_2;
                break;
            case SDLK_3:
            case SDLK_KP_3:
                key_code = EGUI_KEY_CODE_3;
                break;
            case SDLK_4:
            case SDLK_KP_4:
                key_code = EGUI_KEY_CODE_4;
                break;
            case SDLK_5:
            case SDLK_KP_5:
                key_code = EGUI_KEY_CODE_5;
                break;
            case SDLK_6:
            case SDLK_KP_6:
                key_code = EGUI_KEY_CODE_6;
                break;
            case SDLK_7:
            case SDLK_KP_7:
                key_code = EGUI_KEY_CODE_7;
                break;
            case SDLK_8:
            case SDLK_KP_8:
                key_code = EGUI_KEY_CODE_8;
                break;
            case SDLK_9:
            case SDLK_KP_9:
                key_code = EGUI_KEY_CODE_9;
                break;
            // Letters
            case SDLK_a:
                key_code = EGUI_KEY_CODE_A;
                break;
            case SDLK_b:
                key_code = EGUI_KEY_CODE_B;
                break;
            case SDLK_c:
                key_code = EGUI_KEY_CODE_C;
                break;
            case SDLK_d:
                key_code = EGUI_KEY_CODE_D;
                break;
            case SDLK_e:
                key_code = EGUI_KEY_CODE_E;
                break;
            case SDLK_f:
                key_code = EGUI_KEY_CODE_F;
                break;
            case SDLK_g:
                key_code = EGUI_KEY_CODE_G;
                break;
            case SDLK_h:
                key_code = EGUI_KEY_CODE_H;
                break;
            case SDLK_i:
                key_code = EGUI_KEY_CODE_I;
                break;
            case SDLK_j:
                key_code = EGUI_KEY_CODE_J;
                break;
            case SDLK_k:
                key_code = EGUI_KEY_CODE_K;
                break;
            case SDLK_l:
                key_code = EGUI_KEY_CODE_L;
                break;
            case SDLK_m:
                key_code = EGUI_KEY_CODE_M;
                break;
            case SDLK_n:
                key_code = EGUI_KEY_CODE_N;
                break;
            case SDLK_o:
                key_code = EGUI_KEY_CODE_O;
                break;
            case SDLK_p:
                key_code = EGUI_KEY_CODE_P;
                break;
            case SDLK_q:
                key_code = EGUI_KEY_CODE_Q;
                break;
            case SDLK_r:
                key_code = EGUI_KEY_CODE_R;
                break;
            case SDLK_s:
                key_code = EGUI_KEY_CODE_S;
                break;
            case SDLK_t:
                key_code = EGUI_KEY_CODE_T;
                break;
            case SDLK_u:
                key_code = EGUI_KEY_CODE_U;
                break;
            case SDLK_v:
                key_code = EGUI_KEY_CODE_V;
                break;
            case SDLK_w:
                key_code = EGUI_KEY_CODE_W;
                break;
            case SDLK_x:
                key_code = EGUI_KEY_CODE_X;
                break;
            case SDLK_y:
                key_code = EGUI_KEY_CODE_Y;
                break;
            case SDLK_z:
                key_code = EGUI_KEY_CODE_Z;
                break;
            // Symbols
            case SDLK_PERIOD:
                key_code = EGUI_KEY_CODE_PERIOD;
                break;
            case SDLK_COMMA:
                key_code = EGUI_KEY_CODE_COMMA;
                break;
            case SDLK_MINUS:
                key_code = EGUI_KEY_CODE_MINUS;
                break;
            case SDLK_PLUS:
            case SDLK_KP_PLUS:
                key_code = EGUI_KEY_CODE_PLUS;
                break;
            case SDLK_SLASH:
                key_code = EGUI_KEY_CODE_SLASH;
                break;
            case SDLK_AT:
                key_code = EGUI_KEY_CODE_AT;
                break;
            case SDLK_UNDERSCORE:
                key_code = EGUI_KEY_CODE_UNDERSCORE;
                break;
            case SDLK_SEMICOLON:
                key_code = EGUI_KEY_CODE_SEMICOLON;
                break;
            case SDLK_COLON:
                key_code = EGUI_KEY_CODE_COLON;
                break;
            case SDLK_EXCLAIM:
                key_code = EGUI_KEY_CODE_EXCLAMATION;
                break;
            case SDLK_QUESTION:
                key_code = EGUI_KEY_CODE_QUESTION;
                break;
            default:
                break;
            }

            if (key_code != EGUI_KEY_CODE_NONE)
            {
                if (target_core != NULL)
                {
                    if (!egui_port_post_core_task(target_core, sdl_core_key_task, sdl_pack_key_task_data(key_type, key_code, is_shift, is_ctrl)))
                    {
                        sdl_log_core_task_caller_failure("key_event", target_display);
                    }
                }
            }
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY
            break;
        }
        case SDL_QUIT:
            sdl_atomic_flag_set(&sdl_quit_qry, true);
            break;
        default:
            break;
        }
    }

    // Simulate user interaction and save frame if recording is enabled
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
    recording_simulate_action();
#endif
    recording_save_frame();
}

bool VT_is_request_quit(void)
{
    return sdl_atomic_flag_get(&sdl_quit_qry);
}

void VT_deinit(void)
{
    monitor_sdl_clean_up();
    egui_pc_log("%s deinit_done\n", SHUTDOWN_MARKER_PREFIX);
}

void VT_sdl_flush(int32_t nMS)
{
    VT_sdl_flush_core(NULL, nMS);
}

void VT_sdl_flush_core(egui_core_t *core_ctx, int32_t nMS)
{
    EGUI_UNUSED(nMS);
#ifdef __EMSCRIPTEN__
    sdl_port_commit_frame(core_ctx);
    sdl_port_request_refresh();
#else
    if (sdl_atomic_flag_get(&sdl_quit_qry) || !sdl_atomic_flag_get(&sdl_inited))
    {
        return;
    }
    sdl_port_commit_frame(core_ctx);
    sdl_port_request_refresh();
#endif
}

void sdl_port_request_refresh(void)
{
    if (sdl_atomic_flag_get(&sdl_quit_qry) || !sdl_atomic_flag_get(&sdl_inited))
    {
        return;
    }
    sdl_atomic_flag_set(&sdl_refr_qry, true);
}

void VT_begin_shutdown(void)
{
    sdl_atomic_flag_set(&sdl_quit_qry, true);
    sdl_atomic_flag_set(&sdl_refr_qry, false);
    egui_pc_log("%s begin_shutdown\n", SHUTDOWN_MARKER_PREFIX);
}

void sdl_port_sleep(uint32_t nMS)
{
#ifndef __EMSCRIPTEN__
    SDL_Delay(nMS);
#endif
}

static uint32_t recording_apply_clock_scale(uint32_t real_ms)
{
    if (!g_recording_enabled || g_recording_clock_scale <= 1)
    {
        return real_ms;
    }

    if (g_recording_clock_anchor_real_ms == 0)
    {
        g_recording_clock_anchor_real_ms = real_ms;
        g_recording_clock_anchor_scaled_ms = real_ms;
        return real_ms;
    }

    uint32_t delta = real_ms - g_recording_clock_anchor_real_ms;
    uint64_t scaled = (uint64_t)g_recording_clock_anchor_scaled_ms + (uint64_t)delta * (uint64_t)g_recording_clock_scale;
    if (scaled > UINT32_MAX)
    {
        scaled = UINT32_MAX;
    }
    return (uint32_t)scaled;
}

#if defined(_POSIX_VERSION) || defined(CLOCK_MONOTONIC) || defined(__APPLE__)
static uint32_t sdl_get_system_timestamp_ms_raw(void)
{
    struct timespec timestamp;
    clock_gettime(CLOCK_MONOTONIC, &timestamp);

    return 1000ul * timestamp.tv_sec + timestamp.tv_nsec / 1000000ul;
}

uint32_t sdl_get_system_timestamp_us(void)
{
    struct timespec timestamp;
    clock_gettime(CLOCK_MONOTONIC, &timestamp);

    return 1000000ul * timestamp.tv_sec + timestamp.tv_nsec / 1000ul;
}
#else

static uint32_t sdl_get_system_timestamp_ms_raw(void)
{
    return (int64_t)clock();
}

#endif

uint32_t sdl_get_system_timestamp_ms(void)
{
    uint32_t real_ms = sdl_get_system_timestamp_ms_raw();
    return recording_apply_clock_scale(real_ms);
}

void VT_init(void)
{
    monitor_sdl_init();

    while (!sdl_atomic_flag_get(&sdl_inited))
    {
        SDL_Delay(1);
    }
}

static void VT_Fill_Single_Color_Core(egui_core_t *core_ctx, int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t color)
{
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    int disp_id = core_ctx != NULL ? core_ctx->id : 0;
    if (disp_id > 0 && disp_id <= sdl_extra_count)
    {
        sdl_extra_fill_single_color(disp_id - 1, x1, y1, x2, y2, color);
        return;
    }
#endif
    /*Return if the area is out the screen*/
    if (x2 < 0)
        return;
    if (y2 < 0)
        return;
    if (x1 > VT_WIDTH - 1)
        return;
    if (y1 > VT_HEIGHT - 1)
        return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > VT_WIDTH - 1 ? VT_WIDTH - 1 : x2;
    int32_t act_y2 = y2 > VT_HEIGHT - 1 ? VT_HEIGHT - 1 : y2;

    int32_t x;
    int32_t y;

    for (x = act_x1; x <= act_x2; x++)
    {
        for (y = act_y1; y <= act_y2; y++)
        {
#if VT_SDL_NATIVE_RGB565
            tft_fb[y * VT_WIDTH + x] = color;
#else
            tft_fb[y * VT_WIDTH + x] = 0xff000000U | sdl_color_to_rgb888(core_ctx, color);
#endif
        }
    }
}

void VT_Fill_Single_Color(int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t color)
{
    VT_Fill_Single_Color_Core(NULL, x1, y1, x2, y2, color);
}

void VT_Fill_Multiple_Colors_Core(egui_core_t *core_ctx, int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t *color_p)
{
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    int disp_id = core_ctx != NULL ? core_ctx->id : 0;
    if (disp_id > 0 && disp_id <= sdl_extra_count)
    {
        sdl_extra_fill_colors(disp_id - 1, x1, y1, x2, y2, color_p);
        return;
    }
#endif
    /*Return if the area is out the screen*/
    if (x2 < 0)
        return;
    if (y2 < 0)
        return;
    if (x1 > VT_WIDTH - 1)
        return;
    if (y1 > VT_HEIGHT - 1)
        return;

    /*Truncate the area to the screen*/
    int32_t act_x1 = x1 < 0 ? 0 : x1;
    int32_t act_y1 = y1 < 0 ? 0 : y1;
    int32_t act_x2 = x2 > VT_WIDTH - 1 ? VT_WIDTH - 1 : x2;
    int32_t act_y2 = y2 > VT_HEIGHT - 1 ? VT_HEIGHT - 1 : y2;

    int32_t x;
    int32_t y;

    for (y = act_y1; y <= act_y2; y++)
    {
        for (x = act_x1; x <= act_x2; x++)
        {
#if VT_SDL_NATIVE_RGB565
            tft_fb[y * VT_WIDTH + x] = *color_p;
#else
            tft_fb[y * VT_WIDTH + x] = 0xff000000U | sdl_color_to_rgb888(core_ctx, *color_p);
#endif
            color_p++;
        }

        color_p += x2 - act_x2;
    }
}

void VT_Fill_Multiple_Colors(int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t *color_p)
{
    VT_Fill_Multiple_Colors_Core(NULL, x1, y1, x2, y2, color_p);
}

static void VT_Set_Point_Core(egui_core_t *core_ctx, int32_t x, int32_t y, egui_color_int_t color)
{
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    int disp_id = core_ctx != NULL ? core_ctx->id : 0;
    if (disp_id > 0 && disp_id <= sdl_extra_count)
    {
        sdl_extra_set_point(disp_id - 1, x, y, color);
        return;
    }
#endif
    /*Return if the area is out the screen*/
    if (x < 0)
        return;
    if (y < 0)
        return;
    if (x > VT_WIDTH - 1)
        return;
    if (y > VT_HEIGHT - 1)
        return;

#if VT_SDL_NATIVE_RGB565
    tft_fb[y * VT_WIDTH + x] = color;
#else
    tft_fb[y * VT_WIDTH + x] = 0xff000000U | sdl_color_to_rgb888(core_ctx, color);
#endif
}

void VT_Set_Point(int32_t x, int32_t y, egui_color_int_t color)
{
    VT_Set_Point_Core(NULL, x, y, color);
}

static egui_color_int_t VT_Get_Point_Core(egui_core_t *core_ctx, int32_t x, int32_t y)
{
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    int disp_id = core_ctx != NULL ? core_ctx->id : 0;
    if (disp_id > 0 && disp_id <= sdl_extra_count)
    {
        return sdl_extra_get_point(disp_id - 1, x, y);
    }
#endif
    /*Return if the area is out the screen*/
    if (x < 0)
        return 0;
    if (y < 0)
        return 0;
    if (x > VT_WIDTH - 1)
        return 0;
    if (y > VT_HEIGHT - 1)
        return 0;

#if VT_SDL_NATIVE_RGB565
    return tft_fb[y * VT_WIDTH + x];
#else
    uint32_t color = tft_fb[y * VT_WIDTH + x];
    return sdl_rgb888_to_color(core_ctx, color);
#endif
}

egui_color_int_t VT_Get_Point(int32_t x, int32_t y)
{
    return VT_Get_Point_Core(NULL, x, y);
}

void snap_shot(const char *file_name)
{
    // Large showcase-sized canvases can exceed the default Windows stack
    // if the RGB snapshot buffer lives on the stack.
    size_t rgb_size = (size_t)VT_WIDTH * (size_t)VT_HEIGHT * 3U;
    unsigned char *rgb_data = (unsigned char *)malloc(rgb_size);

    if (rgb_data == NULL)
    {
        return;
    }

    sdl_frame_lock();

#if VT_SDL_NATIVE_RGB565
    egui_core_t *core_ctx = egui_port_get_core_by_display_id(0);

    for (int i = 0; i < VT_WIDTH * VT_HEIGHT; i++)
    {
        uint32_t rgb888 = rgb565_to_rgb888(sdl_present_fb[i], sdl_get_color_16_swap(core_ctx));
        rgb_data[i * 3 + 0] = (uint8_t)((rgb888 >> 16) & 0xFFU);
        rgb_data[i * 3 + 1] = (uint8_t)((rgb888 >> 8) & 0xFFU);
        rgb_data[i * 3 + 2] = (uint8_t)(rgb888 & 0xFFU);
    }
#else
    unsigned int *p_raw_data = (unsigned int *)sdl_present_fb;

    for (int i = 0; i < VT_WIDTH * VT_HEIGHT; i++)
    {
        unsigned int argb = p_raw_data[i];
        rgb_data[i * 3 + 0] = (argb >> 16) & 0xFF; // R
        rgb_data[i * 3 + 1] = (argb >> 8) & 0xFF;  // G
        rgb_data[i * 3 + 2] = argb & 0xFF;         // B
    }
#endif

    sdl_frame_unlock();

    stbi_write_png(file_name, VT_WIDTH, VT_HEIGHT, 3, rgb_data, VT_WIDTH * 3);
    free(rgb_data);
}

#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
static void snap_shot_extra_display(int idx, const char *file_name)
{
    if (idx < 0 || idx >= sdl_extra_count)
    {
        return;
    }

    sdl_extra_display_t *ctx = &sdl_extra[idx];
    int w = ctx->width;
    int h = ctx->height;
    size_t rgb_size = (size_t)w * (size_t)h * 3U;
    unsigned char *rgb_data = (unsigned char *)malloc(rgb_size);

    if (rgb_data == NULL)
    {
        return;
    }

    SDL_LockMutex(ctx->frame_mutex);

    /* Extra displays always use ARGB8888 */
    for (int i = 0; i < w * h; i++)
    {
        unsigned int argb = ctx->present_fb[i];
        rgb_data[i * 3 + 0] = (argb >> 16) & 0xFF; // R
        rgb_data[i * 3 + 1] = (argb >> 8) & 0xFF;  // G
        rgb_data[i * 3 + 2] = argb & 0xFF;         // B
    }

    SDL_UnlockMutex(ctx->frame_mutex);

    stbi_write_png(file_name, w, h, 3, rgb_data, w * 3);
    free(rgb_data);
}
#endif /* EGUI_CONFIG_MAX_DISPLAY_COUNT > 1 */

// Recording functions for GIF generation
void recording_init(const char *output_dir, int fps, int duration_sec)
{
    recording_lock();
    g_recording_enabled = true;
    g_recording_fps = fps > 0 ? fps : 10;
    g_recording_duration_ms = duration_sec > 0 ? duration_sec * 1000 : 10000;
    g_recording_frame_count = 0;
    g_recording_start_time = 0;
    g_recording_last_frame_time = 0;
    g_recording_clock_anchor_real_ms = 0;
    g_recording_clock_anchor_scaled_ms = 0;
    // g_recording_speed is set separately via recording_set_speed()
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
    g_recording_last_action_time = 0;
    g_recording_action_index = 0;
    g_recording_drag_in_progress = false;
    g_recording_click_release_pending = false;
    g_recording_drag_current_step = 0;
    memset(&g_recording_current_action, 0, sizeof(g_recording_current_action));
    g_recording_snapshot_requested = false;
    g_recording_snapshot_blocks_actions = false;
    g_recording_snapshot_request_time = 0;
    g_recording_snapshot_request_frame_count = 0;
    g_recording_snapshot_last_hash = 0;
    g_recording_snapshot_same_hash_count = 0;
    g_recording_snapshot_seen_change = false;
    g_recording_quit_after_snapshot = false;
    g_recording_all_actions_done = false;
    g_recording_completed_frame_count = 0;
    g_recording_start_real_time = 0;
#endif
    strncpy(g_recording_output_dir, output_dir, sizeof(g_recording_output_dir) - 1);
    g_recording_output_dir[sizeof(g_recording_output_dir) - 1] = '\0';
    recording_unlock();

    MKDIR(g_recording_output_dir);
    egui_pc_log("Recording enabled: dir=%s, fps=%d, duration=%ds\n", g_recording_output_dir, g_recording_fps, duration_sec);
}

bool recording_is_enabled(void)
{
    return g_recording_enabled;
}

bool recording_is_finished(void)
{
    return !g_recording_enabled && g_recording_frame_count > 0;
}

void recording_set_speed(int speed)
{
    g_recording_speed = speed > 1 ? speed : 1;
}

void recording_set_clock_scale(int scale)
{
    if (scale < 1)
    {
        scale = 1;
    }
    if (scale > 50)
    {
        scale = 50;
    }
    g_recording_clock_scale = scale;
    g_recording_clock_anchor_real_ms = 0;
    g_recording_clock_anchor_scaled_ms = 0;
}

void recording_set_snapshot_settle_ms(int settle_ms)
{
    if (settle_ms < 0)
    {
        settle_ms = 0;
    }
    if (settle_ms > 2000)
    {
        settle_ms = 2000;
    }
    recording_lock();
    g_recording_snapshot_settle_ms = settle_ms;
    recording_unlock();
}

void recording_set_snapshot_stability(int stable_cycles, int max_wait_ms)
{
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
    recording_lock();
    if (stable_cycles >= 0)
    {
        if (stable_cycles > 20)
        {
            stable_cycles = 20;
        }
        g_recording_snapshot_stable_cycles = stable_cycles;
    }

    if (max_wait_ms >= 0)
    {
        if (max_wait_ms > 10000)
        {
            max_wait_ms = 10000;
        }
        g_recording_snapshot_max_wait_ms = max_wait_ms;
    }
    recording_unlock();
#else
    EGUI_UNUSED(stable_cycles);
    EGUI_UNUSED(max_wait_ms);
#endif
}

static void recording_request_snapshot_internal(bool block_actions)
{
    EGUI_UNUSED(block_actions);
    recording_lock();
    if (g_recording_enabled)
    {
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
        g_recording_snapshot_requested = true;
        g_recording_snapshot_blocks_actions = block_actions;
        g_recording_snapshot_request_time = sdl_get_system_timestamp_ms_raw();
        g_recording_snapshot_request_frame_count = g_recording_completed_frame_count;
        g_recording_snapshot_last_hash = recording_calc_frame_hash();
        g_recording_snapshot_same_hash_count = 0;
        g_recording_snapshot_seen_change = false;
#endif
    }
    recording_unlock();
}

void recording_request_snapshot(void)
{
    recording_request_snapshot_internal(true);
}

void sdl_port_set_headless(bool headless)
{
    g_sdl_headless = headless;
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
void egui_port_notify_frame_render_complete(void)
{
    recording_lock();
    g_recording_completed_frame_count++;
    recording_unlock();
}
#else
void egui_port_notify_frame_render_complete(void)
{
}
#endif

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
/**
 * Weak function for apps to define custom actions during recording.
 * Override this function in your app to customize simulation behavior.
 * @return true if action is valid, false to stop simulation
 */
#if defined(_MSC_VER)
bool egui_port_get_recording_action_default(int action_index, egui_sim_action_t *p_action)
#else
__EGUI_WEAK__ bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
#endif
{
    // Default: click at screen center once
    if (action_index >= 1)
    {
        return false;
    }
    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = VT_WIDTH / 2;
    p_action->y1 = VT_HEIGHT / 2;
    p_action->interval_ms = 1000;
    return true;
}
__EGUI_MSVC_ALTERNATE_NAME(egui_port_get_recording_action, egui_port_get_recording_action_default)

#if defined(_MSC_VER)
const char *egui_port_get_recording_frame_label_default(void)
#else
__EGUI_WEAK__ const char *egui_port_get_recording_frame_label(void)
#endif
{
    return NULL;
}
__EGUI_MSVC_ALTERNATE_NAME(egui_port_get_recording_frame_label, egui_port_get_recording_frame_label_default)

static uint32_t sdl_hash_bytes(uint32_t hash, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        hash ^= data[i];
        hash *= 16777619u;
    }

    return hash;
}

static uint32_t recording_calc_frame_hash(void)
{
    uint32_t hash = 2166136261u;

    sdl_frame_lock();
    hash = sdl_hash_bytes(hash, (const uint8_t *)sdl_present_fb, (size_t)VT_WIDTH * (size_t)VT_HEIGHT * (size_t)VT_FB_PIXEL_SIZE);
    sdl_frame_unlock();

#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    /* Include extra display framebuffers in the hash */
    for (int d = 0; d < sdl_extra_count; d++)
    {
        sdl_extra_display_t *ctx = &sdl_extra[d];
        SDL_LockMutex(ctx->frame_mutex);
        hash = sdl_hash_bytes(hash, (const uint8_t *)ctx->present_fb, (size_t)ctx->width * (size_t)ctx->height * sizeof(uint32_t));
        SDL_UnlockMutex(ctx->frame_mutex);
    }
#endif
    return hash;
}

// Execute a single action step
static void recording_execute_action_step(void)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    const egui_sim_action_t *action = &g_recording_current_action;
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1 || EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
    int target_display = 0;
#endif
    int x1 = action->x1;
    int y1 = action->y1;
    int x2 = action->x2;
    int y2 = action->y2;
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    egui_touch_driver_point_t action_points[EGUI_TOUCH_DRIVER_MAX_POINTS];
    int action_end_x[EGUI_TOUCH_DRIVER_MAX_POINTS];
    int action_end_y[EGUI_TOUCH_DRIVER_MAX_POINTS];
    uint8_t action_point_count = action->type == EGUI_SIM_ACTION_MULTI_DRAG ? action->point_count : 0;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    if (action_point_count > EGUI_TOUCH_DRIVER_MAX_POINTS)
    {
        action_point_count = EGUI_TOUCH_DRIVER_MAX_POINTS;
    }
    for (uint8_t i = 0; i < EGUI_TOUCH_DRIVER_MAX_POINTS; i++)
    {
        action_points[i].x = i < action_point_count ? (int16_t)action->point_start_x[i] : 0;
        action_points[i].y = i < action_point_count ? (int16_t)action->point_start_y[i] : 0;
        action_points[i].id = i;
        action_points[i].pressure = i < action_point_count ? 1 : 0;
        action_end_x[i] = i < action_point_count ? action->point_end_x[i] : 0;
        action_end_y[i] = i < action_point_count ? action->point_end_y[i] : 0;
    }
#endif

#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    /* Route touch to the correct display's queue */
    target_display = action->display_id;
#define RECORDING_TOUCH_PUSH(pressed, x, y)                                                                                                                    \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (target_display > 0 && target_display <= sdl_extra_count)                                                                                           \
            sdl_extra_touch_push(target_display - 1, (pressed), (x), (y));                                                                                     \
        else                                                                                                                                                   \
            sdl_port_touch_push_event((pressed), (x), (y));                                                                                                    \
    } while (0)
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#define RECORDING_TOUCH_PUSH_POINTS(point_count, points, fallback_x, fallback_y)                                                                               \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (target_display > 0 && target_display <= sdl_extra_count)                                                                                           \
            sdl_extra_touch_push_points(target_display - 1, (point_count), (points), (fallback_x), (fallback_y));                                              \
        else                                                                                                                                                   \
            sdl_port_touch_push_points((point_count), (points), (fallback_x), (fallback_y));                                                                   \
    } while (0)
#endif
#else
#define RECORDING_TOUCH_PUSH(pressed, x, y) sdl_port_touch_push_event((pressed), (x), (y))
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#define RECORDING_TOUCH_PUSH_POINTS(point_count, points, fallback_x, fallback_y) sdl_port_touch_push_points((point_count), (points), (fallback_x), (fallback_y))
#endif
#endif

#if EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE
    // Recording actions use logical coordinates (from view region_screen),
    // but egui_input_add_motion expects physical coordinates (the polling layer
    // will transform physical -> logical). Convert logical -> physical here.
    // Keep the converted coordinates in locals so one click uses the same
    // physical point for both DOWN and UP.
    if (action->type == EGUI_SIM_ACTION_CLICK || action->type == EGUI_SIM_ACTION_DRAG || action->type == EGUI_SIM_ACTION_SWIPE
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
        || action->type == EGUI_SIM_ACTION_MULTI_DRAG
#endif
    )
    {
        egui_core_t *target_core = NULL;
        egui_port_display_runtime_info_t runtime_info;
        target_core = egui_port_get_core_by_display_id(target_display);
        if (target_core != NULL && egui_port_get_display_runtime_info(target_core, &runtime_info) && runtime_info.rotation != EGUI_DISPLAY_ROTATION_0 &&
            !runtime_info.has_hardware_rotation && runtime_info.software_rotation)
        {
            int16_t pw = runtime_info.physical_width;
            int16_t ph = runtime_info.physical_height;
            int lx, ly;

            switch (runtime_info.rotation)
            {
            case EGUI_DISPLAY_ROTATION_90:
                // Inverse: lx,ly -> px=pw-1-ly, py=lx
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
                if (action->type == EGUI_SIM_ACTION_MULTI_DRAG)
                {
                    for (uint8_t i = 0; i < action_point_count; i++)
                    {
                        int start_lx = action->point_start_x[i];
                        int start_ly = action->point_start_y[i];
                        int end_lx = action->point_end_x[i];
                        int end_ly = action->point_end_y[i];
                        action_points[i].x = (int16_t)(pw - 1 - start_ly);
                        action_points[i].y = (int16_t)start_lx;
                        action_end_x[i] = pw - 1 - end_ly;
                        action_end_y[i] = end_lx;
                    }
                    break;
                }
#endif
                lx = x1;
                ly = y1;
                x1 = pw - 1 - ly;
                y1 = lx;
                if (action->type != EGUI_SIM_ACTION_CLICK)
                {
                    lx = x2;
                    ly = y2;
                    x2 = pw - 1 - ly;
                    y2 = lx;
                }
                break;
            case EGUI_DISPLAY_ROTATION_180:
                // Inverse: lx,ly -> px=pw-1-lx, py=ph-1-ly
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
                if (action->type == EGUI_SIM_ACTION_MULTI_DRAG)
                {
                    for (uint8_t i = 0; i < action_point_count; i++)
                    {
                        action_points[i].x = (int16_t)(pw - 1 - action->point_start_x[i]);
                        action_points[i].y = (int16_t)(ph - 1 - action->point_start_y[i]);
                        action_end_x[i] = pw - 1 - action->point_end_x[i];
                        action_end_y[i] = ph - 1 - action->point_end_y[i];
                    }
                    break;
                }
#endif
                x1 = pw - 1 - x1;
                y1 = ph - 1 - y1;
                if (action->type != EGUI_SIM_ACTION_CLICK)
                {
                    x2 = pw - 1 - x2;
                    y2 = ph - 1 - y2;
                }
                break;
            case EGUI_DISPLAY_ROTATION_270:
                // Inverse: lx,ly -> px=ly, py=ph-1-lx
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
                if (action->type == EGUI_SIM_ACTION_MULTI_DRAG)
                {
                    for (uint8_t i = 0; i < action_point_count; i++)
                    {
                        int start_lx = action->point_start_x[i];
                        int start_ly = action->point_start_y[i];
                        int end_lx = action->point_end_x[i];
                        int end_ly = action->point_end_y[i];
                        action_points[i].x = (int16_t)start_ly;
                        action_points[i].y = (int16_t)(ph - 1 - start_lx);
                        action_end_x[i] = end_ly;
                        action_end_y[i] = ph - 1 - end_lx;
                    }
                    break;
                }
#endif
                lx = x1;
                ly = y1;
                x1 = ly;
                y1 = ph - 1 - lx;
                if (action->type != EGUI_SIM_ACTION_CLICK)
                {
                    lx = x2;
                    ly = y2;
                    x2 = ly;
                    y2 = ph - 1 - lx;
                }
                break;
            default:
                break;
            }
        }
    }
#endif

    switch (action->type)
    {
    case EGUI_SIM_ACTION_CLICK:
        if (!g_recording_click_release_pending)
        {
            RECORDING_TOUCH_PUSH(1, x1, y1);
            g_recording_click_release_pending = true;
        }
        else
        {
            RECORDING_TOUCH_PUSH(0, x1, y1);
            g_recording_click_release_pending = false;
        }
        break;

    case EGUI_SIM_ACTION_DRAG:
    case EGUI_SIM_ACTION_SWIPE:
    {
        int steps = action->steps > 0 ? action->steps : 10;
        int step = g_recording_drag_current_step;

        if (step == 0)
        {
            RECORDING_TOUCH_PUSH(1, x1, y1);
            g_recording_drag_in_progress = true;
        }
        else if (step <= steps)
        {
            int x = x1 + (x2 - x1) * step / steps;
            int y = y1 + (y2 - y1) * step / steps;
            RECORDING_TOUCH_PUSH(1, x, y);
        }

        if (step >= steps)
        {
            RECORDING_TOUCH_PUSH(0, x2, y2);
            g_recording_drag_in_progress = false;
        }
        break;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    case EGUI_SIM_ACTION_MULTI_DRAG:
    {
        int steps = action->steps > 0 ? action->steps : 10;
        int step = g_recording_drag_current_step;
        int fallback_x = action_point_count > 0 ? action_end_x[0] : x2;
        int fallback_y = action_point_count > 0 ? action_end_y[0] : y2;
        int start_x[EGUI_TOUCH_DRIVER_MAX_POINTS];
        int start_y[EGUI_TOUCH_DRIVER_MAX_POINTS];

        for (uint8_t i = 0; i < action_point_count; i++)
        {
            start_x[i] = action_points[i].x;
            start_y[i] = action_points[i].y;
        }

        if (step == 0)
        {
            RECORDING_TOUCH_PUSH_POINTS(action_point_count, action_points, fallback_x, fallback_y);
            g_recording_drag_in_progress = true;
        }
        else if (step <= steps)
        {
            for (uint8_t i = 0; i < action_point_count; i++)
            {
                action_points[i].x = (int16_t)(start_x[i] + (action_end_x[i] - start_x[i]) * step / steps);
                action_points[i].y = (int16_t)(start_y[i] + (action_end_y[i] - start_y[i]) * step / steps);
                action_points[i].pressure = 1;
            }
            RECORDING_TOUCH_PUSH_POINTS(action_point_count, action_points, fallback_x, fallback_y);
        }

        if (step >= steps)
        {
            RECORDING_TOUCH_PUSH_POINTS(0, action_points, fallback_x, fallback_y);
            g_recording_drag_in_progress = false;
        }
        break;
    }
#endif

    case EGUI_SIM_ACTION_WAIT:
    case EGUI_SIM_ACTION_NONE:
    default:
        // No touch action needed
        break;
    }
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#undef RECORDING_TOUCH_PUSH_POINTS
#endif
#undef RECORDING_TOUCH_PUSH
#endif
}

// Simulate user actions during recording
static void recording_simulate_action(void)
{
    bool snapshot_blocks_actions;
    bool snapshot_requested;

    if (!g_recording_enabled)
    {
        return;
    }

    uint32_t now = sdl_get_system_timestamp_ms();

    // Initialize action time
    if (g_recording_last_action_time == 0)
    {
        g_recording_last_action_time = now;
        return;
    }

    recording_lock();
    snapshot_blocks_actions = g_recording_snapshot_requested && g_recording_snapshot_blocks_actions;
    recording_unlock();
    if (snapshot_blocks_actions)
    {
        return;
    }

    // If drag is in progress, continue it
    if (g_recording_drag_in_progress)
    {
        // Drag step interval is NOT accelerated by speed - UI needs real time to process touch events
        uint32_t drag_step_interval = 50; // 50ms between drag steps
        if ((now - g_recording_last_action_time) >= drag_step_interval)
        {
            g_recording_last_action_time = now;
            g_recording_drag_current_step++;
            recording_execute_action_step();

            // Check if drag finished
            int steps = g_recording_current_action.steps > 0 ? g_recording_current_action.steps : 10;
            if (g_recording_drag_current_step >= steps)
            {
                g_recording_drag_in_progress = false;
                g_recording_action_index++;
                // Auto-snapshot after drag completes (fallback)
                recording_request_snapshot_internal(false);
            }
        }
        return;
    }

    if (g_recording_click_release_pending)
    {
        uint32_t click_release_interval = 50;
        if ((now - g_recording_last_action_time) < click_release_interval)
        {
            return;
        }

        g_recording_last_action_time = now;
        recording_execute_action_step();
        g_recording_action_index++;
        recording_request_snapshot_internal(false);
        return;
    }

    // Get next action
    egui_sim_action_t action;
    if (!egui_port_get_recording_action(g_recording_action_index, &action))
    {
        // No more actions - request final snapshot and quit
        if (!g_recording_all_actions_done)
        {
            g_recording_all_actions_done = true;
            g_recording_quit_after_snapshot = true;
            recording_request_snapshot_internal(false);
        }
        return;
    }

    recording_lock();
    snapshot_blocks_actions = g_recording_snapshot_requested && g_recording_snapshot_blocks_actions;
    snapshot_requested = g_recording_snapshot_requested;
    recording_unlock();
    if (snapshot_blocks_actions)
    {
        return;
    }

    if ((action.type == EGUI_SIM_ACTION_WAIT || action.type == EGUI_SIM_ACTION_NONE) && action.interval_ms <= 0)
    {
        if (snapshot_requested)
        {
            return;
        }
        g_recording_last_action_time = now;
        g_recording_current_action = action;
        g_recording_drag_current_step = 0;
        recording_execute_action_step();
        g_recording_action_index++;
        return;
    }

    // Check action interval (accelerated by speed multiplier)
    uint32_t effective_interval = (uint32_t)action.interval_ms / g_recording_speed;
    if (effective_interval < 50)
    {
        effective_interval = 50;
    }
    if ((now - g_recording_last_action_time) < effective_interval)
    {
        return;
    }
    g_recording_last_action_time = now;

    // Store current action for multi-step actions (drag/swipe)
    g_recording_current_action = action;
    g_recording_drag_current_step = 0;

    // Execute the action
    recording_execute_action_step();

    // For single-step actions, move to next immediately
    if (action.type == EGUI_SIM_ACTION_WAIT || action.type == EGUI_SIM_ACTION_NONE)
    {
        g_recording_action_index++;
        // Auto-snapshot after action completes (fallback)
        recording_request_snapshot_internal(false);
    }
}
#endif // EGUI_CONFIG_FUNCTION_RECORDING_TEST

static void recording_do_save_frame(void)
{
    char filename[1024];
    snprintf(filename, sizeof(filename), "%s/frame_%04d.png", g_recording_output_dir, g_recording_frame_count);
    snap_shot(filename);
#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1
    /* Save extra display framebuffers alongside the main frame */
    for (int d = 0; d < sdl_extra_count; d++)
    {
        char extra_filename[1024];
        snprintf(extra_filename, sizeof(extra_filename), "%s/frame_%04d_disp%d.png", g_recording_output_dir, g_recording_frame_count, d + 1);
        snap_shot_extra_display(d, extra_filename);
    }
#endif
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
    const char *frame_name = NULL;
    const char *windows_frame_name = NULL;
    frame_name = strrchr(filename, '/');
    windows_frame_name = strrchr(filename, '\\');
    if (frame_name == NULL || (windows_frame_name != NULL && windows_frame_name > frame_name))
    {
        frame_name = windows_frame_name;
    }
    frame_name = frame_name != NULL ? frame_name + 1 : filename;
    {
        const char *label = egui_port_get_recording_frame_label();
        if (label != NULL && label[0] != '\0')
        {
            egui_pc_log("PERF_FRAME:%s:%s\r\n", frame_name, label);
        }
    }
#endif
    g_recording_frame_count++;
}

static void recording_save_frame(void)
{
    if (!g_recording_enabled)
    {
        return;
    }

    uint32_t now = sdl_get_system_timestamp_ms();
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
    uint32_t real_now = sdl_get_system_timestamp_ms_raw();
#endif

    // Initialize start time on first frame
    if (g_recording_start_time == 0)
    {
        g_recording_start_time = now;
        g_recording_last_frame_time = now;
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
        g_recording_start_real_time = real_now;
        // In recording-test mode, wait for the first stable rendered frame
        // instead of saving the uninitialized backbuffer as frame_0000.
        recording_request_snapshot_internal(true);
#else
        // Save the initial frame (page 1 before any action)
        recording_do_save_frame();
#endif
    }

    // Check if recording finished (timeout safety)
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
    if ((real_now - g_recording_start_real_time) >= (uint32_t)g_recording_duration_ms)
#else
    if ((now - g_recording_start_time) >= (uint32_t)g_recording_duration_ms)
#endif
    {
        recording_lock();
        g_recording_enabled = false;
        recording_unlock();
        sdl_atomic_flag_set(&sdl_quit_qry, true);
        egui_pc_log("Recording finished (timeout): %d frames saved\n", g_recording_frame_count);
        return;
    }

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
    // Snapshot-driven frame capture: save when requested by user code or auto-fallback
    recording_lock();
    if (g_recording_snapshot_requested)
    {
        bool frame_completed = g_recording_completed_frame_count > g_recording_snapshot_request_frame_count;
        bool timeout_ready = (real_now - g_recording_snapshot_request_time) >= (uint32_t)g_recording_snapshot_max_wait_ms;

        if (!frame_completed && !timeout_ready)
        {
            recording_unlock();
            return;
        }

        if ((real_now - g_recording_snapshot_request_time) < (uint32_t)g_recording_snapshot_settle_ms)
        {
            recording_unlock();
            return;
        }

        uint32_t frame_hash = recording_calc_frame_hash();
        if (frame_hash == g_recording_snapshot_last_hash)
        {
            g_recording_snapshot_same_hash_count++;
        }
        else
        {
            g_recording_snapshot_last_hash = frame_hash;
            g_recording_snapshot_same_hash_count = 0;
            g_recording_snapshot_seen_change = true;
        }

        bool stable_ready = g_recording_snapshot_seen_change && g_recording_snapshot_same_hash_count >= g_recording_snapshot_stable_cycles;
        if (!stable_ready && !timeout_ready)
        {
            recording_unlock();
            return;
        }

        g_recording_snapshot_requested = false;
        g_recording_snapshot_blocks_actions = false;
        recording_unlock();
        g_recording_last_frame_time = now;
        recording_do_save_frame();

        // Auto-quit after all actions done and final frame captured
        if (g_recording_quit_after_snapshot)
        {
            recording_lock();
            g_recording_enabled = false;
            recording_unlock();
            sdl_atomic_flag_set(&sdl_quit_qry, true);
            egui_pc_log("Recording finished (all actions done): %d frames saved\n", g_recording_frame_count);
            return;
        }
    }
    else
    {
        recording_unlock();
    }
#else
    // Non-test mode: periodic FPS-based capture
    uint32_t frame_interval_ms = 1000 / g_recording_fps;
    if ((now - g_recording_last_frame_time) >= frame_interval_ms)
    {
        g_recording_last_frame_time = now;
        recording_do_save_frame();
    }
#endif

    // Safety limit
    if (g_recording_frame_count >= RECORDING_MAX_FRAMES)
    {
        recording_lock();
        g_recording_enabled = false;
        recording_unlock();
        sdl_atomic_flag_set(&sdl_quit_qry, true);
        egui_pc_log("Recording stopped: max frames reached (%d)\n", RECORDING_MAX_FRAMES);
    }
}
