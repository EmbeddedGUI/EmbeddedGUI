#include "sdl_port.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "SDL2/SDL.h"

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
static uint32_t tft_fb[VT_WIDTH * VT_HEIGHT];
static volatile bool sdl_inited = false;
static volatile bool sdl_refr_qry = false;
static volatile bool sdl_refr_cpl = false;
static volatile bool sdl_quit_qry = false;

static bool sdl_mouse_left_down = false;

extern void VT_Init(void);
extern void VT_Fill_Single_Color(int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t color);
extern void VT_Fill_Multiple_Colors(int32_t x1, int32_t y1, int32_t x2, int32_t y2, egui_color_int_t *color_p);
extern void VT_Set_Point(int32_t x, int32_t y, egui_color_int_t color);
extern egui_color_int_t VT_Get_Point(int32_t x, int32_t y);
extern void VT_Clear(egui_color_int_t color);
extern bool VT_Mouse_Get_Point(int16_t *x, int16_t *y);

void snap_shot(const char *file_name);

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

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, VT_WIDTH, VT_HEIGHT,
                              0); /*last param. SDL_WINDOW_BORDERLESS to hide borders*/

#if VT_VIRTUAL_MACHINE
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
#else
    renderer = SDL_CreateRenderer(window, -1, 0);
#endif
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, VT_WIDTH, VT_HEIGHT);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    /*Initialize the frame buffer to gray (77 is an empirical value) */
    memset(tft_fb, 77, VT_WIDTH * VT_HEIGHT * sizeof(uint32_t));
    SDL_UpdateTexture(texture, NULL, tft_fb, VT_WIDTH * sizeof(uint32_t));

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
            SDL_UpdateTexture(texture, NULL, tft_fb, VT_WIDTH * sizeof(uint32_t));
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

        case SDL_WINDOWEVENT:
            switch ((&event)->window.event)
            {
#if SDL_VERSION_ATLEAST(2, 0, 5)
            case SDL_WINDOWEVENT_TAKE_FOCUS:
#endif
            case SDL_WINDOWEVENT_EXPOSED:
                SDL_UpdateTexture(texture, NULL, tft_fb, VT_WIDTH * sizeof(uint32_t));
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                break;
            default:
                break;
            }
            break;

        case SDL_KEYDOWN:
            // 检测按键按下事件
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                break;
            } else {
                // printf("Key pressed: %c\n", event.key.keysym.sym);
                if(event.key.keysym.sym >= '0' && event.key.keysym.sym <= '9') {
                    egui_port_hanlde_key_event(event.key.keysym.sym - '0', 1);
                }
            }
            break;
        case SDL_KEYUP:
            // 检测按键按下事件
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                break;
            } else {
                // printf("Key pressed: %c\n", event.key.keysym.sym);
                if(event.key.keysym.sym >= '0' && event.key.keysym.sym <= '9') {
                    egui_port_hanlde_key_event(event.key.keysym.sym - '0', 0);
                }
            }
            break;
        default:
            break;
        }
    }
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
    nMS = EGUI_MAX(1, nMS);
    while (!sdl_refr_cpl)
    {
        SDL_Delay(nMS);
    }
    sdl_refr_cpl = false;
    sdl_refr_qry = true;

    // For snapshot.
    // snap_shot("test.bmp");
}

void sdl_port_sleep(uint32_t nMS)
{
    SDL_Delay(nMS);
}

#if defined(_POSIX_VERSION) || defined(CLOCK_MONOTONIC) || defined(__APPLE__)
uint32_t sdl_get_system_timestamp_ms(void)
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

uint32_t sdl_get_system_timestamp_ms(void)
{
    return (int64_t)clock();
}

#endif

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
            tft_fb[y * VT_WIDTH + x] = 0xff000000 | DEV_2_VT_RGB(color);
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
            tft_fb[y * VT_WIDTH + x] = 0xff000000 | DEV_2_VT_RGB(*color_p);
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

    tft_fb[y * VT_WIDTH + x] = 0xff000000 | DEV_2_VT_RGB(color);
}

egui_color_int_t VT_Get_Point(int32_t x, int32_t y)
{
    uint32_t color = 0;
    /*Return if the area is out the screen*/
    if (x < 0)
        return 0;
    if (y < 0)
        return 0;
    if (x > VT_WIDTH - 1)
        return 0;
    if (y > VT_HEIGHT - 1)
        return 0;

    color = tft_fb[y * VT_WIDTH + x];

    return VT_RGB_2_DEV(color);
}

#pragma pack(push, 1)
typedef struct
{
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
} FileHead;

typedef struct
{
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompress;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
    unsigned int biRedMask;
    unsigned int biGreenMask;
    unsigned int biBlueMask;
} Infohead;
#pragma pack(pop)

int build_bmp(const char *filename, unsigned int width, unsigned int height, unsigned char *data)
{
    FileHead bmp_head;
    Infohead bmp_info;
    int size = width * height * 2;

    // initialize bmp head.
    bmp_head.bfType = 0x4d42;
    bmp_head.bfSize = size + sizeof(FileHead) + sizeof(Infohead);
    bmp_head.bfReserved1 = bmp_head.bfReserved2 = 0;
    bmp_head.bfOffBits = bmp_head.bfSize - size;

    // initialize bmp info.
    bmp_info.biSize = 40;
    bmp_info.biWidth = width;
    bmp_info.biHeight = height;
    bmp_info.biPlanes = 1;
    bmp_info.biBitCount = 16;
    bmp_info.biCompress = 3;
    bmp_info.biSizeImage = size;
    bmp_info.biXPelsPerMeter = 0;
    bmp_info.biYPelsPerMeter = 0;
    bmp_info.biClrUsed = 0;
    bmp_info.biClrImportant = 0;

    // RGB565
    bmp_info.biRedMask = 0xF800;
    bmp_info.biGreenMask = 0x07E0;
    bmp_info.biBlueMask = 0x001F;

    // copy the data
    FILE *fp;
    if (!(fp = fopen(filename, "wb")))
    {
        return -1;
    }

    fwrite(&bmp_head, 1, sizeof(FileHead), fp);
    fwrite(&bmp_info, 1, sizeof(Infohead), fp);

    // fwrite(data, 1, size, fp);//top <-> bottom
    for (int i = (height - 1); i >= 0; --i)
    {
        fwrite(&data[i * width * 2], 1, width * 2, fp);
    }

    fclose(fp);
    return 0;
}

void snap_shot(const char *file_name)
{
    void *m_phy_fb = (void *)tft_fb;

    // 32 bits framebuffer
    unsigned short p_bmp565_data[VT_WIDTH * VT_HEIGHT];
    unsigned int *p_raw_data = (unsigned int *)m_phy_fb;

    for (int i = 0; i < VT_WIDTH * VT_HEIGHT; i++)
    {
        unsigned int rgb = *p_raw_data++;
        p_bmp565_data[i] = RGB888_2_RGB565(rgb);
    }

    build_bmp(file_name, VT_WIDTH, VT_HEIGHT, (unsigned char *)p_bmp565_data);
}
