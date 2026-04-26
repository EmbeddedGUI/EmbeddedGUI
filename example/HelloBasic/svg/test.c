#include "egui.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "uicode_disp0.h"

#define HELLO_SVG_FRAME_MS       50
#define HELLO_SVG_PAGE_COUNT     3
#define HELLO_SVG_BUFFER_SIZE    12288
#define HELLO_SVG_TITLE_H        20
#define HELLO_SVG_PAGE_LABEL_W   36
#define HELLO_SVG_OVERLAY_MARGIN 14
#define HELLO_SVG_PI             3.14159265358979323846f

typedef float hello_svg_real_t;

typedef struct hello_svg_page
{
    egui_view_group_t page;
    egui_view_image_t image;
    egui_image_svg_t svg;
} hello_svg_page_t;

static egui_view_group_t root;
static egui_view_viewpage_t viewpage;
static hello_svg_page_t pages[HELLO_SVG_PAGE_COUNT];
static egui_view_label_t title_label;
static egui_view_label_t page_label;
static egui_timer_t hero_timer;
static uint32_t hero_tick;
static uint8_t current_page_index;
static char page_label_text[8];
static char svg_buffers[HELLO_SVG_PAGE_COUNT][HELLO_SVG_BUFFER_SIZE];
static const char *const hello_svg_titles[HELLO_SVG_PAGE_COUNT] = {"Pulse", "Tunnel", "Orbit"};

EGUI_VIEW_GROUP_PARAMS_INIT(root_params, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);
EGUI_VIEW_VIEWPAGE_PARAMS_INIT(viewpage_params, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);
EGUI_VIEW_GROUP_PARAMS_INIT(page_params, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);
EGUI_VIEW_IMAGE_PARAMS_INIT(page_image_params, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT, NULL);
EGUI_VIEW_LABEL_PARAMS_INIT(title_label_params, HELLO_SVG_OVERLAY_MARGIN, HELLO_SVG_OVERLAY_MARGIN,
                            EGUI_CONFIG_SCREEN_WIDTH - HELLO_SVG_OVERLAY_MARGIN * 2 - HELLO_SVG_PAGE_LABEL_W, HELLO_SVG_TITLE_H, NULL, EGUI_CONFIG_FONT_DEFAULT,
                            EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(page_label_params, EGUI_CONFIG_SCREEN_WIDTH - HELLO_SVG_OVERLAY_MARGIN - HELLO_SVG_PAGE_LABEL_W, HELLO_SVG_OVERLAY_MARGIN,
                            HELLO_SVG_PAGE_LABEL_W, HELLO_SVG_TITLE_H, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

static void hello_svg_append(char *buffer, size_t size, size_t *offset, const char *format, ...)
{
    va_list args;
    int written;

    if (*offset >= size)
    {
        return;
    }

    va_start(args, format);
    written = vsnprintf(buffer + *offset, size - *offset, format, args);
    va_end(args);

    if (written < 0)
    {
        return;
    }

    if ((size_t)written >= size - *offset)
    {
        *offset = size - 1;
    }
    else
    {
        *offset += (size_t)written;
    }
}

static hello_svg_real_t hello_svg_deg_to_rad(int degrees)
{
    return (hello_svg_real_t)degrees * HELLO_SVG_PI / 180.0f;
}

static int hello_svg_clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

static int hello_svg_round_to_int(hello_svg_real_t value)
{
    if (value >= 0.0f)
    {
        return (int)(value + 0.5f);
    }
    return (int)(value - 0.5f);
}

static void hello_svg_orbit_point(int cx, int cy, int rx, int ry, int tilt_deg, int phase_deg, int *x, int *y)
{
    hello_svg_real_t orbit_angle = hello_svg_deg_to_rad(tilt_deg);
    hello_svg_real_t phase_angle = hello_svg_deg_to_rad(phase_deg);
    hello_svg_real_t local_x = (hello_svg_real_t)rx * cosf(phase_angle);
    hello_svg_real_t local_y = (hello_svg_real_t)ry * sinf(phase_angle);

    *x = cx + hello_svg_round_to_int(local_x * cosf(orbit_angle) - local_y * sinf(orbit_angle));
    *y = cy + hello_svg_round_to_int(local_x * sinf(orbit_angle) + local_y * cosf(orbit_angle));
}

static void hello_svg_append_rotated_ellipse_band(char *buffer, size_t size, size_t *offset, int cx, int cy, int rx, int ry, int tilt_deg, int thickness,
                                                  const char *fill, int opacity)
{
    int samples = hello_svg_clamp_int(((rx > ry ? rx : ry) / 2) + 4, 32, 48);
    hello_svg_real_t tilt = hello_svg_deg_to_rad(tilt_deg);
    hello_svg_real_t cos_tilt = cosf(tilt);
    hello_svg_real_t sin_tilt = sinf(tilt);
    hello_svg_real_t half_thickness = (hello_svg_real_t)thickness * 0.5f;
    hello_svg_real_t outer_rx = (hello_svg_real_t)rx + half_thickness;
    hello_svg_real_t outer_ry = (hello_svg_real_t)ry + half_thickness;
    hello_svg_real_t inner_rx = (hello_svg_real_t)rx - half_thickness;
    hello_svg_real_t inner_ry = (hello_svg_real_t)ry - half_thickness;
    int i;

    if (inner_rx < 1.0f)
    {
        inner_rx = 1.0f;
    }
    if (inner_ry < 1.0f)
    {
        inner_ry = 1.0f;
    }

    hello_svg_append(buffer, size, offset, "<path d='");
    for (i = 0; i < samples; i++)
    {
        hello_svg_real_t angle = (hello_svg_real_t)i * HELLO_SVG_PI * 2.0f / (hello_svg_real_t)samples;
        hello_svg_real_t local_x = outer_rx * cosf(angle);
        hello_svg_real_t local_y = outer_ry * sinf(angle);
        int x = cx + hello_svg_round_to_int(local_x * cos_tilt - local_y * sin_tilt);
        int y = cy + hello_svg_round_to_int(local_x * sin_tilt + local_y * cos_tilt);

        hello_svg_append(buffer, size, offset, i == 0 ? "M%d %d " : "L%d %d ", x, y);
    }
    hello_svg_append(buffer, size, offset, "Z ");
    for (i = 0; i < samples; i++)
    {
        hello_svg_real_t angle = (hello_svg_real_t)i * HELLO_SVG_PI * 2.0f / (hello_svg_real_t)samples;
        hello_svg_real_t local_x = inner_rx * cosf(angle);
        hello_svg_real_t local_y = inner_ry * sinf(angle);
        int x = cx + hello_svg_round_to_int(local_x * cos_tilt - local_y * sin_tilt);
        int y = cy + hello_svg_round_to_int(local_x * sin_tilt + local_y * cos_tilt);

        hello_svg_append(buffer, size, offset, i == 0 ? "M%d %d " : "L%d %d ", x, y);
    }
    hello_svg_append(buffer, size, offset, "Z' fill='%s' fill-rule='evenodd' opacity='%d%%'/>", fill, opacity);
}

static void hello_svg_append_wave_band(char *buffer, size_t size, size_t *offset, int x_start, int x_end, int y_base, int amplitude, int thickness,
                                       hello_svg_real_t cycles, hello_svg_real_t speed, hello_svg_real_t phase, const char *fill, int opacity, uint32_t tick)
{
    int samples = hello_svg_clamp_int((x_end - x_start) / 8, 24, 32);
    hello_svg_real_t half_thickness = (hello_svg_real_t)thickness * 0.5f;
    hello_svg_real_t motion = (hello_svg_real_t)tick * speed + phase;
    int i;

    hello_svg_append(buffer, size, offset, "<polygon points='");
    for (i = 0; i < samples; i++)
    {
        hello_svg_real_t t = (hello_svg_real_t)i / (hello_svg_real_t)(samples - 1);
        hello_svg_real_t wave = sinf(t * cycles * HELLO_SVG_PI * 2.0f + motion);
        int x = x_start + hello_svg_round_to_int((hello_svg_real_t)(x_end - x_start) * t);
        int y = y_base + hello_svg_round_to_int((hello_svg_real_t)amplitude * wave);

        hello_svg_append(buffer, size, offset, "%d,%d ", x, hello_svg_round_to_int((hello_svg_real_t)y - half_thickness));
    }
    for (i = samples - 1; i >= 0; i--)
    {
        hello_svg_real_t t = (hello_svg_real_t)i / (hello_svg_real_t)(samples - 1);
        hello_svg_real_t wave = sinf(t * cycles * HELLO_SVG_PI * 2.0f + motion);
        int x = x_start + hello_svg_round_to_int((hello_svg_real_t)(x_end - x_start) * t);
        int y = y_base + hello_svg_round_to_int((hello_svg_real_t)amplitude * wave);

        hello_svg_append(buffer, size, offset, "%d,%d ", x, hello_svg_round_to_int((hello_svg_real_t)y + half_thickness));
    }
    hello_svg_append(buffer, size, offset, "' fill='%s' opacity='%d%%'/>", fill, opacity);
}

static void hello_svg_build_header(uint8_t page_index)
{
    snprintf(page_label_text, sizeof(page_label_text), "%u/%u", (unsigned)page_index + 1U, (unsigned)HELLO_SVG_PAGE_COUNT);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), hello_svg_titles[page_index]);
    egui_view_label_set_text(EGUI_VIEW_OF(&page_label), page_label_text);
}

static void hello_svg_build_pulse_frame(char *buffer, size_t size, uint32_t tick)
{
    size_t offset = 0;
    int w = EGUI_CONFIG_SCREEN_WIDTH;
    int h = EGUI_CONFIG_SCREEN_HEIGHT;
    int cx = w / 2;
    int cy = h / 2 + 10;
    int min_side = w < h ? w : h;
    int grid_left = 24;
    int grid_right = w - 24;
    int grid_top = 36;
    int grid_bottom = h - 54;
    int sweep_angle = (int)((tick * 7U) % 360U);
    int frame_angle = (int)((tick * 3U) % 360U);
    int diamond_angle = (int)((360U - (tick * 5U) % 360U) % 360U);
    int orbit_angle = (int)((tick * 9U) % 360U);
    int orbit_angle_b = (int)((tick * 4U + 48U) % 360U);
    int core_angle = (int)((tick * 6U) % 360U);
    hello_svg_real_t wave_a = sinf((hello_svg_real_t)tick * 0.14f);
    hello_svg_real_t wave_b = cosf((hello_svg_real_t)tick * 0.09f);
    hello_svg_real_t wave_c = sinf((hello_svg_real_t)tick * 0.11f + 1.2f);
    int outer_r = min_side / 2 - 24;
    int mid_r = outer_r - 24;
    int inner_r = mid_r - 24;
    int pulse_radius = 22 + (int)(6.0f * (wave_a + 1.0f));
    int halo_radius = 36 + (int)(9.0f * (wave_b + 1.0f));
    int beam_opacity = 24 + (int)(28.0f * (wave_c + 1.0f) * 0.5f);
    int core_opacity = 50 + (int)(24.0f * (wave_b + 1.0f) * 0.5f);
    int halo_opacity = 20 + (int)(18.0f * (wave_a + 1.0f) * 0.5f);
    int telemetry_x = 30;
    int telemetry_w = w - telemetry_x * 2;
    int telemetry_a = 46 + (int)((hello_svg_real_t)(telemetry_w - 36) * (wave_a + 1.0f) * 0.5f);
    int telemetry_b = 28 + (int)((hello_svg_real_t)(telemetry_w - 24) * (wave_b + 1.0f) * 0.5f);
    int telemetry_c = 20 + (int)((hello_svg_real_t)(telemetry_w - 60) * (wave_c + 1.0f) * 0.5f);
    int telemetry_marker_a = telemetry_x + 12 + (int)((tick * 9U) % (uint32_t)(telemetry_w - 24));
    int telemetry_marker_b = telemetry_x + 12 + (int)((tick * 7U + 36U) % (uint32_t)(telemetry_w - 30));
    int bar_y_a = h - 38;
    int bar_y_b = h - 28;
    int bar_y_c = h - 20;

    hello_svg_append(buffer, size, &offset, "<svg viewBox='0 0 %d %d'>", w, h);
    hello_svg_append(buffer, size, &offset, "<rect x='0' y='0' width='%d' height='%d' fill='#030913'/>", w, h);
    hello_svg_append(buffer, size, &offset, "<g opacity='18%%'>");
    hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#14384B' stroke-width='1'/>", grid_left, grid_top + 16, grid_right,
                     grid_top + 16);
    hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#14384B' stroke-width='1'/>", grid_left, grid_top + 60, grid_right,
                     grid_top + 60);
    hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#14384B' stroke-width='1'/>", grid_left, grid_bottom - 60,
                     grid_right, grid_bottom - 60);
    hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#14384B' stroke-width='1'/>", grid_left, grid_bottom - 16,
                     grid_right, grid_bottom - 16);
    hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#14384B' stroke-width='1'/>", 48, grid_top, 48, grid_bottom);
    hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#14384B' stroke-width='1'/>", cx, grid_top - 8, cx, grid_bottom + 8);
    hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#14384B' stroke-width='1'/>", w - 48, grid_top, w - 48, grid_bottom);
    hello_svg_append(buffer, size, &offset, "</g>");

    hello_svg_append(buffer, size, &offset, "<g opacity='24%%'>");
    hello_svg_append(buffer, size, &offset, "<path fill='#173B4E' d='M24 72 H58 V76 H28 V108 H24 Z'/>");
    hello_svg_append(buffer, size, &offset, "<path fill='#173B4E' d='M%d 72 H%d V76 H%d V108 H%d Z'/>", w - 24, w - 58, w - 28, w - 24);
    hello_svg_append(buffer, size, &offset, "<path fill='#173B4E' d='M24 %d H58 V%d H28 V%d H24 Z'/>", h - 72, h - 76, h - 108);
    hello_svg_append(buffer, size, &offset, "<path fill='#173B4E' d='M%d %d H%d V%d H%d V%d H%d Z'/>", w - 24, h - 72, w - 58, h - 76, w - 28, h - 108, w - 24);
    hello_svg_append(buffer, size, &offset, "</g>");

    hello_svg_append(buffer, size, &offset, "<g transform='translate(%d,%d)'>", cx, cy);
    hello_svg_append(buffer, size, &offset, "<circle cx='0' cy='0' r='%d' fill='none' stroke='#12384B' stroke-width='2' opacity='34%%'/>", outer_r);
    hello_svg_append(buffer, size, &offset, "<circle cx='0' cy='0' r='%d' fill='none' stroke='#12384B' stroke-width='1' opacity='40%%'/>", mid_r);
    hello_svg_append(buffer, size, &offset, "<circle cx='0' cy='0' r='%d' fill='none' stroke='#12384B' stroke-width='1' opacity='48%%'/>", inner_r);
    hello_svg_append(buffer, size, &offset, "<g transform='rotate(%d)'>", sweep_angle);
    hello_svg_append(buffer, size, &offset, "<polygon points='0,-%d 16,-18 -16,-18' fill='#16D7FF' opacity='%d%%'/>", outer_r + 6, beam_opacity);
    hello_svg_append(buffer, size, &offset, "<line x1='0' y1='-18' x2='0' y2='-%d' stroke='#9AFBFF' stroke-width='2' opacity='78%%'/>", outer_r + 8);
    hello_svg_append(buffer, size, &offset, "</g>");
    hello_svg_append(buffer, size, &offset, "<g transform='rotate(%d)'>", frame_angle);
    hello_svg_append(buffer, size, &offset, "<rect x='-%d' y='-%d' width='%d' height='%d' fill='none' stroke='#0F7FA8' stroke-width='2' opacity='56%%'/>",
                     mid_r - 6, mid_r - 6, (mid_r - 6) * 2, (mid_r - 6) * 2);
    hello_svg_append(buffer, size, &offset, "</g>");
    hello_svg_append(buffer, size, &offset, "<g transform='rotate(%d)'>", diamond_angle);
    hello_svg_append(buffer, size, &offset, "<rect x='-%d' y='-%d' width='%d' height='%d' fill='none' stroke='#87F6FF' stroke-width='3' opacity='72%%'/>",
                     inner_r - 6, inner_r - 6, (inner_r - 6) * 2, (inner_r - 6) * 2);
    hello_svg_append(buffer, size, &offset, "<polygon points='0,-32 32,0 0,32 -32,0' fill='none' stroke='#87F6FF' stroke-width='2' opacity='72%%'/>");
    hello_svg_append(buffer, size, &offset, "</g>");
    hello_svg_append(buffer, size, &offset, "<g transform='rotate(%d)'>", orbit_angle);
    hello_svg_append(buffer, size, &offset, "<circle cx='0' cy='-%d' r='7' fill='#FFD166'/>", outer_r - 2);
    hello_svg_append(buffer, size, &offset, "<circle cx='0' cy='%d' r='5' fill='#7EF7FF'/>", outer_r - 2);
    hello_svg_append(buffer, size, &offset, "</g>");
    hello_svg_append(buffer, size, &offset, "<g transform='rotate(%d)'>", orbit_angle_b);
    hello_svg_append(buffer, size, &offset, "<circle cx='0' cy='-%d' r='4' fill='#FF5E7A'/>", mid_r - 4);
    hello_svg_append(buffer, size, &offset, "<circle cx='%d' cy='0' r='4' fill='#57FFA0'/>", mid_r - 4);
    hello_svg_append(buffer, size, &offset, "<circle cx='0' cy='%d' r='4' fill='#7EF7FF'/>", mid_r - 4);
    hello_svg_append(buffer, size, &offset, "<circle cx='-%d' cy='0' r='4' fill='#FF5E7A'/>", mid_r - 4);
    hello_svg_append(buffer, size, &offset, "</g>");
    hello_svg_append(buffer, size, &offset, "<circle cx='0' cy='0' r='%d' fill='#0B1B29' stroke='#7EF7FF' stroke-width='2' opacity='%d%%'/>", pulse_radius,
                     core_opacity);
    hello_svg_append(buffer, size, &offset, "<circle cx='0' cy='0' r='%d' fill='none' stroke='#1ED8FF' stroke-width='2' opacity='%d%%'/>", halo_radius,
                     halo_opacity);
    hello_svg_append(buffer, size, &offset, "<g transform='rotate(%d)'>", core_angle);
    hello_svg_append(buffer, size, &offset, "<polygon points='0,-20 20,0 0,20 -20,0' fill='#7EF7FF' opacity='18%%'/>");
    hello_svg_append(buffer, size, &offset, "<path fill='#B7FBFF' d='M-18 0 L0 -18 L18 0 L0 18 Z'/>");
    hello_svg_append(buffer, size, &offset, "</g>");
    hello_svg_append(buffer, size, &offset, "</g>");

    hello_svg_append(buffer, size, &offset, "<rect x='%d' y='%d' width='%d' height='1' fill='#1A4258' opacity='38%%'/>", telemetry_x, bar_y_a - 8, telemetry_w);
    hello_svg_append(buffer, size, &offset, "<rect x='%d' y='%d' width='%d' height='4' fill='#081824' opacity='88%%'/>", telemetry_x, bar_y_a, telemetry_w);
    hello_svg_append(buffer, size, &offset, "<rect x='%d' y='%d' width='%d' height='4' fill='#1ED8FF' opacity='52%%'/>", telemetry_x, bar_y_a, telemetry_a);
    hello_svg_append(buffer, size, &offset, "<circle cx='%d' cy='%d' r='3' fill='#B7FBFF' opacity='86%%'/>", telemetry_marker_a, bar_y_a + 2);
    hello_svg_append(buffer, size, &offset, "<rect x='%d' y='%d' width='%d' height='3' fill='#081824' opacity='82%%'/>", telemetry_x, bar_y_b, telemetry_w);
    hello_svg_append(buffer, size, &offset, "<rect x='%d' y='%d' width='%d' height='3' fill='#57FFA0' opacity='56%%'/>", telemetry_x, bar_y_b, telemetry_b);
    hello_svg_append(buffer, size, &offset, "<rect x='%d' y='%d' width='18' height='5' fill='#FFD166' opacity='82%%'/>", telemetry_marker_b, bar_y_b - 1);
    hello_svg_append(buffer, size, &offset, "<rect x='%d' y='%d' width='%d' height='2' fill='#081824' opacity='72%%'/>", telemetry_x, bar_y_c, telemetry_w);
    hello_svg_append(buffer, size, &offset, "<rect x='%d' y='%d' width='%d' height='2' fill='#FF5E7A' opacity='78%%'/>", telemetry_x, bar_y_c, telemetry_c);
    hello_svg_append(buffer, size, &offset, "</svg>");
}

static void hello_svg_build_tunnel_frame(char *buffer, size_t size, uint32_t tick)
{
    size_t offset = 0;
    int w = EGUI_CONFIG_SCREEN_WIDTH;
    int h = EGUI_CONFIG_SCREEN_HEIGHT;
    int cx = w / 2;
    int cy = h / 2 + 8;
    int ring_i;
    int packet_i;
    int gate_angle = (int)((tick * 5U) % 360U);
    int gate_angle_b = (int)((360U - (tick * 4U) % 360U) % 360U);
    hello_svg_real_t glow = sinf((hello_svg_real_t)tick * 0.08f);
    int core_radius = 20 + (int)(4.0f * (glow + 1.0f));
    int core_halo = 36 + (int)(10.0f * (glow + 1.0f));

    hello_svg_append(buffer, size, &offset, "<svg viewBox='0 0 %d %d'>", w, h);
    hello_svg_append(buffer, size, &offset, "<rect x='0' y='0' width='%d' height='%d' fill='#04070D'/>", w, h);
    hello_svg_append(buffer, size, &offset, "<g opacity='18%%'>");
    hello_svg_append(buffer, size, &offset,
                     "<polyline points='0,%d %d,%d 0,%d' fill='none' stroke='#12364A' stroke-width='2' stroke-linejoin='round' stroke-linecap='round'/>",
                     cy - 58, cx, cy, cy + 58);
    hello_svg_append(buffer, size, &offset,
                     "<polyline points='%d,%d %d,%d %d,%d' fill='none' stroke='#12364A' stroke-width='2' stroke-linejoin='round' stroke-linecap='round'/>", w,
                     cy - 58, cx, cy, w, cy + 58);
    hello_svg_append(buffer, size, &offset, "<line x1='20' y1='%d' x2='%d' y2='%d' stroke='#12364A' stroke-width='1' stroke-linecap='round'/>", cy - 96, cx,
                     cy - 8);
    hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#12364A' stroke-width='1' stroke-linecap='round'/>", w - 20, cy - 96,
                     cx, cy - 8);
    hello_svg_append(buffer, size, &offset, "<line x1='20' y1='%d' x2='%d' y2='%d' stroke='#12364A' stroke-width='1' stroke-linecap='round'/>", cy + 96, cx,
                     cy + 8);
    hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#12364A' stroke-width='1' stroke-linecap='round'/>", w - 20, cy + 96,
                     cx, cy + 8);
    hello_svg_append(buffer, size, &offset, "</g>");

    for (ring_i = 0; ring_i < 6; ring_i++)
    {
        hello_svg_real_t phase = fmodf((hello_svg_real_t)tick * 0.045f + (hello_svg_real_t)ring_i * 0.17f, 1.0f);
        hello_svg_real_t depth = 1.0f - phase;
        int half_w = 18 + (int)(depth * (hello_svg_real_t)(w * 38 / 100));
        int half_h = 12 + (int)(depth * (hello_svg_real_t)(h * 34 / 100));
        int notch = 8 + (int)(depth * 16.0f);
        int opacity = 10 + (int)(depth * 66.0f);
        int stroke = ring_i < 2 ? 2 : 1;
        const char *color = (ring_i & 1) ? "#1ED8FF" : "#57FFA0";

        hello_svg_append(buffer, size, &offset,
                         "<polygon points='%d,%d %d,%d %d,%d %d,%d %d,%d %d,%d %d,%d' fill='none' stroke='%s' stroke-width='%d' opacity='%d%%' "
                         "stroke-linejoin='round'/>",
                         cx, cy - half_h, cx + half_w - notch, cy - half_h / 2, cx + half_w, cy, cx + half_w - notch, cy + half_h / 2, cx, cy + half_h,
                         cx - half_w + notch, cy + half_h / 2, cx - half_w, cy, color, stroke, opacity);
        hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='%s' stroke-width='1' opacity='%d%%' stroke-linecap='round'/>",
                         cx - half_w, cy, cx + half_w, cy, color, opacity / 2 + 8);
    }

    for (packet_i = 0; packet_i < 4; packet_i++)
    {
        hello_svg_real_t phase = fmodf((hello_svg_real_t)tick * 0.06f + (hello_svg_real_t)packet_i * 0.25f, 1.0f);
        int left_x = 22 + (int)((hello_svg_real_t)(cx - 22) * phase);
        int right_x = w - left_x;
        int upper_y = cy - 102 + (int)(94.0f * phase);
        int lower_y = h - upper_y;
        int radius = 2 + (int)(phase * 5.0f);
        int opacity = 24 + (int)(phase * 58.0f);

        hello_svg_append(buffer, size, &offset, "<circle cx='%d' cy='%d' r='%d' fill='#FFD166' opacity='%d%%'/>", left_x, upper_y, radius, opacity);
        hello_svg_append(buffer, size, &offset, "<circle cx='%d' cy='%d' r='%d' fill='#7EF7FF' opacity='%d%%'/>", right_x, upper_y, radius, opacity);
        hello_svg_append(buffer, size, &offset, "<circle cx='%d' cy='%d' r='%d' fill='#57FFA0' opacity='%d%%'/>", left_x, lower_y, radius, opacity / 2 + 12);
        hello_svg_append(buffer, size, &offset, "<circle cx='%d' cy='%d' r='%d' fill='#FF5E7A' opacity='%d%%'/>", right_x, lower_y, radius, opacity / 2 + 12);
    }

    hello_svg_append(buffer, size, &offset, "<g transform='translate(%d,%d)'>", cx, cy);
    hello_svg_append(buffer, size, &offset, "<circle cx='0' cy='0' r='%d' fill='none' stroke='#0E3246' stroke-width='2' opacity='42%%'/>", core_halo);
    hello_svg_append(buffer, size, &offset, "<circle cx='0' cy='0' r='%d' fill='#07131C' stroke='#B7FBFF' stroke-width='2' opacity='82%%'/>", core_radius);
    hello_svg_append(buffer, size, &offset, "<g transform='rotate(%d)'>", gate_angle);
    hello_svg_append(buffer, size, &offset, "<rect x='-38' y='-8' width='76' height='16' fill='none' stroke='#FF8E3C' stroke-width='2' opacity='76%%'/>");
    hello_svg_append(buffer, size, &offset, "</g>");
    hello_svg_append(buffer, size, &offset, "<g transform='rotate(%d)'>", gate_angle_b);
    hello_svg_append(buffer, size, &offset, "<rect x='-8' y='-38' width='16' height='76' fill='none' stroke='#1ED8FF' stroke-width='2' opacity='70%%'/>");
    hello_svg_append(buffer, size, &offset, "</g>");
    hello_svg_append(buffer, size, &offset, "<polygon points='0,-18 14,0 0,18 -14,0' fill='#FFFFFF' opacity='84%%'/>");
    hello_svg_append(buffer, size, &offset, "</g>");
    hello_svg_append(buffer, size, &offset, "</svg>");
}

static void hello_svg_build_orbit_frame(char *buffer, size_t size, uint32_t tick)
{
    size_t offset = 0;
    int w = EGUI_CONFIG_SCREEN_WIDTH;
    int h = EGUI_CONFIG_SCREEN_HEIGHT;
    int cx = w / 2;
    int cy = h / 2 + 4;
    int sat1_x;
    int sat1_y;
    int sat1_prev_x;
    int sat1_prev_y;
    int sat2_x;
    int sat2_y;
    int sat2_prev_x;
    int sat2_prev_y;
    int sat3_x;
    int sat3_y;
    int sat3_prev_x;
    int sat3_prev_y;
    int star_i;
    hello_svg_real_t pulse = sinf((hello_svg_real_t)tick * 0.10f);
    int aura_radius = 18 + (int)(6.0f * (pulse + 1.0f));
    int halo_radius = 34 + (int)(10.0f * (pulse + 1.0f));
    int phase_a = (int)((tick * 6U) % 360U);
    int phase_b = (int)((tick * 9U + 120U) % 360U);
    int phase_c = (int)((tick * 4U + 220U) % 360U);
    int wave_y = h - 66;
    int wave_y_2 = h - 46;
    int wave_x_start = 18;
    int wave_x_end = w - 18;
    static const uint8_t star_x_ratio[] = {18, 32, 76, 88, 64, 12};
    static const uint8_t star_y_ratio[] = {16, 26, 18, 34, 10, 38};

    hello_svg_orbit_point(cx, cy, 86, 34, 18, phase_a, &sat1_x, &sat1_y);
    hello_svg_orbit_point(cx, cy, 86, 34, 18, phase_a - 18, &sat1_prev_x, &sat1_prev_y);
    hello_svg_orbit_point(cx, cy, 58, 18, -22, phase_b, &sat2_x, &sat2_y);
    hello_svg_orbit_point(cx, cy, 58, 18, -22, phase_b - 22, &sat2_prev_x, &sat2_prev_y);
    hello_svg_orbit_point(cx, cy, 28, 84, 0, phase_c, &sat3_x, &sat3_y);
    hello_svg_orbit_point(cx, cy, 28, 84, 0, phase_c - 24, &sat3_prev_x, &sat3_prev_y);

    hello_svg_append(buffer, size, &offset, "<svg viewBox='0 0 %d %d'>", w, h);
    hello_svg_append(buffer, size, &offset, "<rect x='0' y='0' width='%d' height='%d' fill='#041018'/>", w, h);
    for (star_i = 0; star_i < 6; star_i++)
    {
        int star_x = (int)((hello_svg_real_t)w * (hello_svg_real_t)star_x_ratio[star_i] / 100.0f);
        int star_y = (int)((hello_svg_real_t)h * (hello_svg_real_t)star_y_ratio[star_i] / 100.0f);
        int opacity = 18 + (int)(40.0f * (sinf((hello_svg_real_t)tick * 0.09f + (hello_svg_real_t)star_i) + 1.0f) * 0.5f);

        hello_svg_append(buffer, size, &offset, "<circle cx='%d' cy='%d' r='2' fill='#B7FBFF' opacity='%d%%'/>", star_x, star_y, opacity);
    }

    hello_svg_append(buffer, size, &offset, "<g transform='translate(%d,%d)'>", cx, cy);
    hello_svg_append_rotated_ellipse_band(buffer, size, &offset, 0, 0, 96, 96, 0, 2, "#103848", 20);
    hello_svg_append_rotated_ellipse_band(buffer, size, &offset, 0, 0, 86, 34, 18, 3, "#1ED8FF", 48);
    hello_svg_append_rotated_ellipse_band(buffer, size, &offset, 0, 0, 58, 18, -22, 3, "#57FFA0", 52);
    hello_svg_append_rotated_ellipse_band(buffer, size, &offset, 0, 0, 28, 84, 0, 3, "#FF5E7A", 46);
    hello_svg_append_rotated_ellipse_band(buffer, size, &offset, 0, 0, halo_radius, halo_radius, 0, 3, "#FFD166", 36);
    hello_svg_append(buffer, size, &offset, "<circle cx='0' cy='0' r='%d' fill='#0B1B29' opacity='90%%'/>", aura_radius);
    hello_svg_append_rotated_ellipse_band(buffer, size, &offset, 0, 0, aura_radius, aura_radius, 0, 3, "#B7FBFF", 90);
    hello_svg_append(buffer, size, &offset, "<polygon points='0,-14 12,-6 12,8 0,16 -12,8 -12,-6' fill='#7EF7FF' opacity='22%%'/>");
    hello_svg_append(buffer, size, &offset, "<path fill='#FFFFFF' d='M-10 0 L0 -10 L10 0 L0 10 Z' opacity='78%%'/>");
    hello_svg_append(buffer, size, &offset, "</g>");

    hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#1ED8FF' stroke-width='2' opacity='38%%' stroke-linecap='butt'/>",
                     sat1_prev_x, sat1_prev_y, sat1_x, sat1_y);
    hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#57FFA0' stroke-width='2' opacity='42%%' stroke-linecap='butt'/>",
                     sat2_prev_x, sat2_prev_y, sat2_x, sat2_y);
    hello_svg_append(buffer, size, &offset, "<line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#FF5E7A' stroke-width='2' opacity='40%%' stroke-linecap='butt'/>",
                     sat3_prev_x, sat3_prev_y, sat3_x, sat3_y);
    hello_svg_append(buffer, size, &offset, "<circle cx='%d' cy='%d' r='7' fill='#1ED8FF' opacity='86%%'/>", sat1_x, sat1_y);
    hello_svg_append(buffer, size, &offset, "<circle cx='%d' cy='%d' r='6' fill='#57FFA0' opacity='88%%'/>", sat2_x, sat2_y);
    hello_svg_append(buffer, size, &offset, "<circle cx='%d' cy='%d' r='5' fill='#FF5E7A' opacity='90%%'/>", sat3_x, sat3_y);

    hello_svg_append_wave_band(buffer, size, &offset, wave_x_start, wave_x_end, wave_y, 8, 3, 1.35f, 0.14f, 0.2f, "#1ED8FF", 64, tick);
    hello_svg_append_wave_band(buffer, size, &offset, wave_x_start, wave_x_end, wave_y_2, 6, 2, 1.15f, 0.11f, 1.1f, "#FFD166", 68, tick);
    hello_svg_append(buffer, size, &offset, "</svg>");
}

static void hello_svg_build_page_frame(uint8_t page_index, char *buffer, size_t size, uint32_t tick)
{
    switch (page_index)
    {
    case 0:
        hello_svg_build_pulse_frame(buffer, size, tick);
        break;
    case 1:
        hello_svg_build_tunnel_frame(buffer, size, tick);
        break;
    case 2:
    default:
        hello_svg_build_orbit_frame(buffer, size, tick);
        break;
    }
}

static void hello_svg_refresh_page(uint8_t page_index, uint32_t tick)
{
    if (page_index >= HELLO_SVG_PAGE_COUNT)
    {
        return;
    }

    hello_svg_build_page_frame(page_index, svg_buffers[page_index], sizeof(svg_buffers[page_index]), tick);
    if (egui_image_svg_load_memory(&pages[page_index].svg, svg_buffers[page_index]))
    {
        egui_view_invalidate(EGUI_VIEW_OF(&pages[page_index].image));
    }
}

static void hello_svg_refresh_visible_pages(uint32_t tick)
{
    int start = current_page_index > 0 ? (int)current_page_index - 1 : 0;
    int end = current_page_index + 1U < HELLO_SVG_PAGE_COUNT ? (int)current_page_index + 1 : HELLO_SVG_PAGE_COUNT - 1;
    int page_index;

    for (page_index = start; page_index <= end; page_index++)
    {
        hello_svg_refresh_page((uint8_t)page_index, tick);
    }
}

static void hello_svg_on_page_changed(egui_view_t *self, int page_index)
{
    EGUI_UNUSED(self);

    if (page_index < 0 || page_index >= HELLO_SVG_PAGE_COUNT)
    {
        return;
    }

    current_page_index = (uint8_t)page_index;
    hello_svg_build_header(current_page_index);
    hello_svg_refresh_visible_pages(hero_tick);
}

static void hello_svg_anim_cb(egui_timer_t *timer)
{
    EGUI_UNUSED(timer);

    hero_tick++;
    hello_svg_refresh_visible_pages(hero_tick);
}

static void hello_svg_init_page(egui_core_t *core, uint8_t page_index)
{
    egui_view_group_init_with_params(EGUI_VIEW_OF(&pages[page_index].page), core, &page_params);
    egui_image_svg_init(&pages[page_index].svg);

    egui_view_image_init_with_params(EGUI_VIEW_OF(&pages[page_index].image), core, &page_image_params);
    egui_view_image_set_image(EGUI_VIEW_OF(&pages[page_index].image), (egui_image_t *)&pages[page_index].svg);
    egui_view_image_set_image_type(EGUI_VIEW_OF(&pages[page_index].image), EGUI_VIEW_IMAGE_TYPE_RESIZE);

    egui_view_group_add_child(EGUI_VIEW_OF(&pages[page_index].page), EGUI_VIEW_OF(&pages[page_index].image));
}

static void hello_svg_init_ui(egui_core_t *core)
{
    uint8_t page_index;

    egui_view_group_init_with_params(EGUI_VIEW_OF(&root), core, &root_params);
    egui_view_viewpage_init_with_params(EGUI_VIEW_OF(&viewpage), core, &viewpage_params);
    egui_view_viewpage_set_on_page_changed(EGUI_VIEW_OF(&viewpage), hello_svg_on_page_changed);

    for (page_index = 0; page_index < HELLO_SVG_PAGE_COUNT; page_index++)
    {
        hello_svg_init_page(core, page_index);
        egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&pages[page_index].page));
    }
    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&viewpage));

    egui_view_label_init_with_params(EGUI_VIEW_OF(&title_label), core, &title_label_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&title_label), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&page_label), core, &page_label_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&page_label), EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER);

    current_page_index = 0;
    hero_tick = 0;
    hello_svg_build_header(current_page_index);
    for (page_index = 0; page_index < HELLO_SVG_PAGE_COUNT; page_index++)
    {
        hello_svg_refresh_page(page_index, hero_tick);
    }

    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&viewpage));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&title_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&root), EGUI_VIEW_OF(&page_label));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&root));
}

void test_init_ui(egui_core_t *core)
{
    hello_svg_init_ui(core);
    egui_timer_init_timer(&hero_timer, NULL, hello_svg_anim_cb);
    egui_timer_start_timer(core, &hero_timer, HELLO_SVG_FRAME_MS, HELLO_SVG_FRAME_MS);
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
#if EGUI_PORT == EGUI_PORT_TYPE_PC
const char *egui_port_get_recording_frame_label(void)
{
    if (current_page_index >= HELLO_SVG_PAGE_COUNT)
    {
        return NULL;
    }
    return hello_svg_titles[current_page_index];
}
#endif

static void hello_svg_set_swipe_left_action(egui_sim_action_t *p_action, uint16_t interval_ms)
{
    p_action->type = EGUI_SIM_ACTION_SWIPE;
    p_action->x1 = EGUI_CONFIG_SCREEN_WIDTH * 3 / 4;
    p_action->y1 = EGUI_CONFIG_SCREEN_HEIGHT / 2;
    p_action->x2 = EGUI_CONFIG_SCREEN_WIDTH / 4;
    p_action->y2 = EGUI_CONFIG_SCREEN_HEIGHT / 2;
    p_action->steps = 5;
    p_action->interval_ms = interval_ms;
}

static void hello_svg_set_swipe_right_action(egui_sim_action_t *p_action, uint16_t interval_ms)
{
    p_action->type = EGUI_SIM_ACTION_SWIPE;
    p_action->x1 = EGUI_CONFIG_SCREEN_WIDTH / 4;
    p_action->y1 = EGUI_CONFIG_SCREEN_HEIGHT / 2;
    p_action->x2 = EGUI_CONFIG_SCREEN_WIDTH * 3 / 4;
    p_action->y2 = EGUI_CONFIG_SCREEN_HEIGHT / 2;
    p_action->steps = 5;
    p_action->interval_ms = interval_ms;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);

    last_action = action_index;
    switch (action_index)
    {
    case 0:
    case 2:
    case 4:
    case 6:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 320);
        return true;
    case 1:
    case 3:
        hello_svg_set_swipe_left_action(p_action, 520);
        return true;
    case 5:
        hello_svg_set_swipe_right_action(p_action, 520);
        return true;
    default:
        return false;
    }
}
#endif
