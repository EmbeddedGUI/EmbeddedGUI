#include "sdl_port.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "SDL2/SDL.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

#undef main

#define monochrome_2_RGB888(color) (color ? 0x000000 : 0xffffff)
#define GRAY8_2_RGB888(color)      (((color & 0xFF) << 16) + ((color & 0xFF) << 8) + ((color & 0xFF)))
#if EGUI_CONFIG_COLOR_16_SWAP == 0
#define RGB565_2_RGB888(color) (((color & 0xF800) << 8) + ((color & 0x7E0) << 5) + ((color & 0x1F) << 3))
#else
#define RGB565_2_RGB888(color)                                                                                                                                 \
    (((((egui_color_rgb565_t *)&color)->red) << (16 + 3)) +                                                                                                    \
     (((((egui_color_rgb565_t *)&color)->green_h << 3) + (((egui_color_rgb565_t *)&color)->green_l)) << (8 + 2)) +                                             \
     ((((egui_color_rgb565_t *)&color)->blue) << 3))
#endif

#define RGB888_2_monochrome(color) ((color) ? 0 : 1)
#define RGB888_2_GRAY8(color)      (((((color & 0xff0000) >> 16)) + (((color & 0xff00) >> 8)) + (((color & 0xff)))) / 3)
#if EGUI_CONFIG_COLOR_16_SWAP == 0
#define RGB888_2_RGB565(color) ((((color & 0xff0000) >> 19) << 11) + (((color & 0xff00) >> 10) << 5) + (((color & 0xff) >> 3)))
#else
#define RGB888_2_RGB565(color) (EGUI_COLOR_MAKE(color & 0xff0000, color & 0xff00, color & 0xff)).full
#endif

// 1 8(233) 16(565) 24(888) 32(8888)
#if VT_COLOR_DEPTH == 1
#define DEV_2_VT_RGB(color) monochrome_2_RGB888(color)
#define VT_RGB_2_DEV(color) RGB888_2_monochrome(color)
#elif VT_COLOR_DEPTH == 8
#define DEV_2_VT_RGB(color) GRAY8_2_RGB888(color)
#define VT_RGB_2_DEV(color) RGB888_2_GRAY8(color)
#elif VT_COLOR_DEPTH == 16
#define DEV_2_VT_RGB(color) RGB565_2_RGB888(color)
#define VT_RGB_2_DEV(color) RGB888_2_RGB565(color)
#elif VT_COLOR_DEPTH == 24 || VT_COLOR_DEPTH == 32
#define DEV_2_VT_RGB(color) (color)
#define VT_RGB_2_DEV(color) (color)
#endif

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
#if VT_SDL_NATIVE_RGB565
static uint16_t tft_fb[VT_WIDTH * VT_HEIGHT];
#define VT_FB_PIXEL_SIZE    sizeof(uint16_t)
#define VT_SDL_PIXEL_FORMAT SDL_PIXELFORMAT_RGB565
#else
static uint32_t tft_fb[VT_WIDTH * VT_HEIGHT];
#define VT_FB_PIXEL_SIZE    sizeof(uint32_t)
#define VT_SDL_PIXEL_FORMAT SDL_PIXELFORMAT_ARGB8888
#endif
static volatile bool sdl_inited = false;
static volatile bool sdl_refr_qry = false;
static volatile bool sdl_refr_cpl = false;
static volatile bool sdl_quit_qry = false;

static bool sdl_mouse_left_down = false;

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

#if EGUI_CONFIG_RECORDING_TEST
// Auto-action simulation for recording
static uint32_t g_recording_last_action_time = 0;
static int g_recording_action_index = 0;
// Drag state
static bool g_recording_drag_in_progress = false;
static int g_recording_drag_current_step = 0;
static egui_sim_action_t g_recording_current_action;
// Snapshot-driven frame capture (replaces fixed-time settle)
static bool g_recording_snapshot_requested = false; // Snapshot requested by user code or auto-fallback
static uint32_t g_recording_snapshot_request_time = 0;
static uint32_t g_recording_snapshot_last_hash = 0;
static int g_recording_snapshot_same_hash_count = 0;
static int g_recording_snapshot_stable_cycles = 1;
static int g_recording_snapshot_max_wait_ms = 1500;
static bool g_recording_quit_after_snapshot = false; // Quit after next snapshot (all actions done)
static bool g_recording_all_actions_done = false;    // No more actions to simulate
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
#if EGUI_CONFIG_RECORDING_TEST
static void recording_simulate_action(void);
static uint32_t recording_calc_frame_hash(void);
#endif
static uint32_t recording_apply_clock_scale(uint32_t real_ms);

int quit_filter(void *userdata, SDL_Event *event)
{
    (void)userdata;

    if (event->type == SDL_QUIT)
    {
        sdl_quit_qry = true;
    }

    return 1;
}

static void monitor_sdl_clean_up(void)
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void monitor_sdl_init(void)
{

    /*Initialize the SDL*/
    SDL_Init(SDL_INIT_VIDEO);

    SDL_SetEventFilter(quit_filter, NULL);

    char title[0x400];
    sprintf(title, "%s-%dx%d@%d", EGUI_APP, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, EGUI_CONFIG_COLOR_DEPTH);

    uint32_t window_flags = 0;
    if (g_sdl_headless)
    {
        window_flags |= SDL_WINDOW_HIDDEN;
    }

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, VT_WIDTH, VT_HEIGHT,
                              window_flags); /*last param. SDL_WINDOW_BORDERLESS to hide borders*/

    if (g_sdl_headless)
    {
        SDL_HideWindow(window);
    }

#if VT_VIRTUAL_MACHINE
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
#else
    renderer = SDL_CreateRenderer(window, -1, 0);
#endif
    texture = SDL_CreateTexture(renderer, VT_SDL_PIXEL_FORMAT, SDL_TEXTUREACCESS_STATIC, VT_WIDTH, VT_HEIGHT);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    /*Initialize the frame buffer to gray (77 is an empirical value) */
    memset(tft_fb, 77, VT_WIDTH * VT_HEIGHT * VT_FB_PIXEL_SIZE);
    SDL_UpdateTexture(texture, NULL, tft_fb, VT_WIDTH * VT_FB_PIXEL_SIZE);

    sdl_refr_cpl = true;
    sdl_refr_qry = false;
    sdl_inited = true;
}

__EGUI_WEAK__ void egui_port_hanlde_key_event(int key, int event)
{
    // printf("key event: %d, %d\n", key, event);
}

void VT_sdl_refresh_task(void)
{
    if (sdl_refr_qry != false)
    {
        // printf("VT_sdl_refresh_task\n");
        {
            sdl_refr_qry = false;
            SDL_UpdateTexture(texture, NULL, tft_fb, VT_WIDTH * VT_FB_PIXEL_SIZE);
            SDL_RenderClear(renderer);

            /*Update the renderer with the texture containing the rendered image*/
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            sdl_refr_cpl = true;
        }
    }
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {

        switch ((&event)->type)
        {
        case SDL_MOUSEBUTTONUP:
            if ((&event)->button.button == SDL_BUTTON_LEFT)
            {
                sdl_mouse_left_down = false;

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_UP, (&event)->motion.x, (&event)->motion.y);
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if ((&event)->button.button == SDL_BUTTON_LEFT)
            {
                sdl_mouse_left_down = true;

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_DOWN, (&event)->motion.x, (&event)->motion.y);
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
            }
            break;
        case SDL_MOUSEMOTION:
            if (sdl_mouse_left_down)
            {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
                egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_MOVE, (&event)->motion.x, (&event)->motion.y);
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
            }
            break;

        case SDL_MOUSEWHEEL:
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            egui_input_add_scroll((egui_dim_t)mx, (egui_dim_t)my, (int16_t)(&event)->wheel.y);
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
            break;
        }

        case SDL_WINDOWEVENT:
            switch ((&event)->window.event)
            {
#if SDL_VERSION_ATLEAST(2, 0, 5)
            case SDL_WINDOWEVENT_TAKE_FOCUS:
#endif
            case SDL_WINDOWEVENT_EXPOSED:
                SDL_UpdateTexture(texture, NULL, tft_fb, VT_WIDTH * VT_FB_PIXEL_SIZE);
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
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
                if (egui_core_is_suspended())
                {
                    egui_screen_on();
                }
                else
                {
                    egui_screen_off();
                }
            }

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
            uint8_t key_type = (event.type == SDL_KEYDOWN) ? EGUI_KEY_EVENT_ACTION_DOWN : EGUI_KEY_EVENT_ACTION_UP;
            uint8_t is_shift = (event.key.keysym.mod & KMOD_SHIFT) ? 1 : 0;
            uint8_t is_ctrl = (event.key.keysym.mod & KMOD_CTRL) ? 1 : 0;
            uint8_t key_code = EGUI_KEY_CODE_NONE;
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
                egui_input_add_key(key_type, key_code, is_shift, is_ctrl);
            }
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY
            break;
        }
        default:
            break;
        }
    }

    // Simulate user interaction and save frame if recording is enabled
#if EGUI_CONFIG_RECORDING_TEST
    recording_simulate_action();
#endif
    recording_save_frame();
}

bool VT_is_request_quit(void)
{
    return sdl_quit_qry;
}

void VT_deinit(void)
{
    monitor_sdl_clean_up();
    exit(0);
}

void VT_sdl_flush(int32_t nMS)
{
#ifdef __EMSCRIPTEN__
    // Single-threaded: set flag and return, VT_sdl_refresh_task() handles rendering
    sdl_refr_cpl = false;
    sdl_refr_qry = true;
#else
    nMS = EGUI_MAX(1, nMS);
    while (!sdl_refr_cpl)
    {
        SDL_Delay(nMS);
    }
    sdl_refr_cpl = false;
    sdl_refr_qry = true;
#endif
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

    while (sdl_inited == false)
        continue;
}

void VT_Fill_Single_Color(int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t color)
{
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
            tft_fb[y * VT_WIDTH + x] = 0xff000000 | DEV_2_VT_RGB(color);
#endif
        }
    }
}

void VT_Fill_Multiple_Colors(int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t *color_p)
{
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
            tft_fb[y * VT_WIDTH + x] = 0xff000000 | DEV_2_VT_RGB(*color_p);
#endif
            color_p++;
        }

        color_p += x2 - act_x2;
    }
}

void VT_Set_Point(int32_t x, int32_t y, egui_color_int_t color)
{
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
    tft_fb[y * VT_WIDTH + x] = 0xff000000 | DEV_2_VT_RGB(color);
#endif
}

egui_color_int_t VT_Get_Point(int32_t x, int32_t y)
{
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
    return VT_RGB_2_DEV(color);
#endif
}

void snap_shot(const char *file_name)
{
    // Convert framebuffer to RGB888 for PNG output
    unsigned char rgb_data[VT_WIDTH * VT_HEIGHT * 3];

#if VT_SDL_NATIVE_RGB565
    for (int i = 0; i < VT_WIDTH * VT_HEIGHT; i++)
    {
        egui_color_rgb565_t c;
        c.full = tft_fb[i];
        rgb_data[i * 3 + 0] = c.color.red << 3;
        rgb_data[i * 3 + 1] = c.color.green << 2;
        rgb_data[i * 3 + 2] = c.color.blue << 3;
    }
#else
    unsigned int *p_raw_data = (unsigned int *)tft_fb;

    for (int i = 0; i < VT_WIDTH * VT_HEIGHT; i++)
    {
        unsigned int argb = p_raw_data[i];
        rgb_data[i * 3 + 0] = (argb >> 16) & 0xFF; // R
        rgb_data[i * 3 + 1] = (argb >> 8) & 0xFF;  // G
        rgb_data[i * 3 + 2] = argb & 0xFF;         // B
    }
#endif

    stbi_write_png(file_name, VT_WIDTH, VT_HEIGHT, 3, rgb_data, VT_WIDTH * 3);
}

// Recording functions for GIF generation
void recording_init(const char *output_dir, int fps, int duration_sec)
{
    g_recording_enabled = true;
    g_recording_fps = fps > 0 ? fps : 10;
    g_recording_duration_ms = duration_sec > 0 ? duration_sec * 1000 : 10000;
    g_recording_frame_count = 0;
    g_recording_start_time = 0;
    g_recording_last_frame_time = 0;
    g_recording_clock_anchor_real_ms = 0;
    g_recording_clock_anchor_scaled_ms = 0;
    // g_recording_speed is set separately via recording_set_speed()
#if EGUI_CONFIG_RECORDING_TEST
    g_recording_last_action_time = 0;
    g_recording_action_index = 0;
    g_recording_drag_in_progress = false;
    g_recording_drag_current_step = 0;
    memset(&g_recording_current_action, 0, sizeof(g_recording_current_action));
    g_recording_snapshot_requested = false;
    g_recording_snapshot_request_time = 0;
    g_recording_snapshot_last_hash = 0;
    g_recording_snapshot_same_hash_count = 0;
    g_recording_quit_after_snapshot = false;
    g_recording_all_actions_done = false;
#endif
    strncpy(g_recording_output_dir, output_dir, sizeof(g_recording_output_dir) - 1);
    g_recording_output_dir[sizeof(g_recording_output_dir) - 1] = '\0';

    MKDIR(g_recording_output_dir);
    printf("Recording enabled: dir=%s, fps=%d, duration=%ds\n", g_recording_output_dir, g_recording_fps, duration_sec);
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
    g_recording_snapshot_settle_ms = settle_ms;
}

void recording_set_snapshot_stability(int stable_cycles, int max_wait_ms)
{
#if EGUI_CONFIG_RECORDING_TEST
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
#else
    EGUI_UNUSED(stable_cycles);
    EGUI_UNUSED(max_wait_ms);
#endif
}

void recording_request_snapshot(void)
{
    if (g_recording_enabled)
    {
#if EGUI_CONFIG_RECORDING_TEST
        g_recording_snapshot_requested = true;
        g_recording_snapshot_request_time = sdl_get_system_timestamp_ms();
        g_recording_snapshot_last_hash = recording_calc_frame_hash();
        g_recording_snapshot_same_hash_count = 0;
#endif
    }
}

void sdl_port_set_headless(bool headless)
{
    g_sdl_headless = headless;
}

#if EGUI_CONFIG_RECORDING_TEST
/**
 * Weak function for apps to define custom actions during recording.
 * Override this function in your app to customize simulation behavior.
 * @return true if action is valid, false to stop simulation
 */
__EGUI_WEAK__ bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
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

static uint32_t recording_calc_frame_hash(void)
{
    const uint8_t *data = (const uint8_t *)tft_fb;
    size_t len = (size_t)VT_WIDTH * (size_t)VT_HEIGHT * (size_t)VT_FB_PIXEL_SIZE;
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++)
    {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

// Execute a single action step
static void recording_execute_action_step(void)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_sim_action_t *action = &g_recording_current_action;

#if EGUI_CONFIG_SOFTWARE_ROTATION
    // Recording actions use logical coordinates (from view region_screen),
    // but egui_input_add_motion expects physical coordinates (the polling layer
    // will transform physical -> logical). Convert logical -> physical here.
    {
        egui_display_driver_t *drv = egui_display_driver_get();
        if (drv != NULL && drv->rotation != EGUI_DISPLAY_ROTATION_0 && drv->ops->set_rotation == NULL)
        {
            int16_t pw = drv->physical_width;
            int16_t ph = drv->physical_height;
            int lx, ly;

            switch (drv->rotation)
            {
            case EGUI_DISPLAY_ROTATION_90:
                // Inverse: lx,ly -> px=pw-1-ly, py=lx
                lx = action->x1;
                ly = action->y1;
                action->x1 = pw - 1 - ly;
                action->y1 = lx;
                lx = action->x2;
                ly = action->y2;
                action->x2 = pw - 1 - ly;
                action->y2 = lx;
                break;
            case EGUI_DISPLAY_ROTATION_180:
                // Inverse: lx,ly -> px=pw-1-lx, py=ph-1-ly
                action->x1 = pw - 1 - action->x1;
                action->y1 = ph - 1 - action->y1;
                action->x2 = pw - 1 - action->x2;
                action->y2 = ph - 1 - action->y2;
                break;
            case EGUI_DISPLAY_ROTATION_270:
                // Inverse: lx,ly -> px=ly, py=ph-1-lx
                lx = action->x1;
                ly = action->y1;
                action->x1 = ly;
                action->y1 = ph - 1 - lx;
                lx = action->x2;
                ly = action->y2;
                action->x2 = ly;
                action->y2 = ph - 1 - lx;
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
        // Single click: down + up
        egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_DOWN, action->x1, action->y1);
        egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_UP, action->x1, action->y1);
        break;

    case EGUI_SIM_ACTION_DRAG:
    case EGUI_SIM_ACTION_SWIPE:
    {
        int steps = action->steps > 0 ? action->steps : 10;
        int step = g_recording_drag_current_step;

        if (step == 0)
        {
            // First step: touch down at start position
            egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_DOWN, action->x1, action->y1);
            g_recording_drag_in_progress = true;
        }
        else if (step <= steps)
        {
            // Intermediate steps: move
            int x = action->x1 + (action->x2 - action->x1) * step / steps;
            int y = action->y1 + (action->y2 - action->y1) * step / steps;
            egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_MOVE, x, y);
        }

        if (step >= steps)
        {
            // Last step: touch up at end position
            egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_UP, action->x2, action->y2);
            g_recording_drag_in_progress = false;
        }
        break;
    }

    case EGUI_SIM_ACTION_WAIT:
    case EGUI_SIM_ACTION_NONE:
    default:
        // No touch action needed
        break;
    }
#endif
}

// Simulate user actions during recording
static void recording_simulate_action(void)
{
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
                recording_request_snapshot();
            }
        }
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
            recording_request_snapshot();
        }
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
    if (action.type == EGUI_SIM_ACTION_CLICK || action.type == EGUI_SIM_ACTION_WAIT || action.type == EGUI_SIM_ACTION_NONE)
    {
        g_recording_action_index++;
        // Auto-snapshot after action completes (fallback)
        recording_request_snapshot();
    }
}
#endif // EGUI_CONFIG_RECORDING_TEST

static void recording_do_save_frame(void)
{
    char filename[1024];
    snprintf(filename, sizeof(filename), "%s/frame_%04d.png", g_recording_output_dir, g_recording_frame_count);
    snap_shot(filename);
    g_recording_frame_count++;
}

static void recording_save_frame(void)
{
    if (!g_recording_enabled)
    {
        return;
    }

    uint32_t now = sdl_get_system_timestamp_ms();

    // Initialize start time on first frame
    if (g_recording_start_time == 0)
    {
        g_recording_start_time = now;
        g_recording_last_frame_time = now;
        // Save the initial frame (page 1 before any action)
        recording_do_save_frame();
    }

    // Check if recording finished (timeout safety)
    if ((now - g_recording_start_time) >= (uint32_t)g_recording_duration_ms)
    {
        g_recording_enabled = false;
        sdl_quit_qry = true;
        printf("Recording finished (timeout): %d frames saved\n", g_recording_frame_count);
        return;
    }

#if EGUI_CONFIG_RECORDING_TEST
    // Snapshot-driven frame capture: save when requested by user code or auto-fallback
    if (g_recording_snapshot_requested)
    {
        if ((now - g_recording_snapshot_request_time) < (uint32_t)g_recording_snapshot_settle_ms)
        {
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
        }

        bool stable_ready = g_recording_snapshot_same_hash_count >= g_recording_snapshot_stable_cycles;
        bool timeout_ready = (now - g_recording_snapshot_request_time) >= (uint32_t)g_recording_snapshot_max_wait_ms;
        if (!stable_ready && !timeout_ready)
        {
            return;
        }

        g_recording_snapshot_requested = false;
        g_recording_last_frame_time = now;
        recording_do_save_frame();

        // Auto-quit after all actions done and final frame captured
        if (g_recording_quit_after_snapshot)
        {
            g_recording_enabled = false;
            sdl_quit_qry = true;
            printf("Recording finished (all actions done): %d frames saved\n", g_recording_frame_count);
            return;
        }
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
        g_recording_enabled = false;
        sdl_quit_qry = true;
        printf("Recording stopped: max frames reached (%d)\n", RECORDING_MAX_FRAMES);
    }
}
