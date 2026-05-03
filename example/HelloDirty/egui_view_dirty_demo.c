#include "egui_view_dirty_demo.h"

#include "egui.h"
#include "widget/egui_view_circle_dirty.h"
#include <string.h>

#define DEMO_W 480
#define DEMO_H 480

#define COLOR_BG      EGUI_COLOR_MAKE(14, 20, 30)
#define COLOR_PANEL   EGUI_COLOR_MAKE(24, 34, 48)
#define COLOR_GRID    EGUI_COLOR_MAKE(43, 55, 72)
#define COLOR_TEXT    EGUI_COLOR_MAKE(226, 232, 240)
#define COLOR_MUTED   EGUI_COLOR_MAKE(148, 163, 184)
#define COLOR_BLUE    EGUI_COLOR_MAKE(56, 189, 248)
#define COLOR_GREEN   EGUI_COLOR_MAKE(74, 222, 128)
#define COLOR_AMBER   EGUI_COLOR_MAKE(251, 191, 36)
#define COLOR_RED     EGUI_COLOR_MAKE(248, 113, 113)
#define COLOR_VIOLET  EGUI_COLOR_MAKE(167, 139, 250)
#define COLOR_TEAL    EGUI_COLOR_MAKE(45, 212, 191)
#define COLOR_ORANGE  EGUI_COLOR_MAKE(251, 146, 60)
#define COLOR_SURFACE EGUI_COLOR_MAKE(30, 41, 59)

#define CONTENT_X 32
#define CONTENT_Y 80
#define CONTENT_W 416
#define CONTENT_H 352

#define BALL_COUNT     EGUI_VIEW_DIRTY_DEMO_BALL_COUNT
#define WAVE_COUNT     EGUI_VIEW_DIRTY_DEMO_WAVE_COUNT
#define LIVE_COUNT     EGUI_VIEW_DIRTY_DEMO_LIVE_COUNT
#define SPECTRUM_COUNT EGUI_VIEW_DIRTY_DEMO_SPECTRUM_COUNT
#define CELL_COUNT     12

#define LIVE_SAMPLE_W   4
#define LIVE_PLOT_X     (CONTENT_X + 18)
#define LIVE_PLOT_Y     (CONTENT_Y + 48)
#define LIVE_BASE_Y     (CONTENT_Y + 268)
#define LIVE_SCROLL_Y   (CONTENT_Y + 44)
#define LIVE_SCROLL_H   232
#define LIVE_PLOT_WIDTH ((LIVE_COUNT - 1) * LIVE_SAMPLE_W + 1)

#define LINE_CENTER_X     240
#define LINE_CENTER_Y     250
#define LINE_RADIUS       145
#define LINE_STROKE_W     7
#define LINE_END_RADIUS   10
#define PROGRESS_TRACK_X  96
#define PROGRESS_TRACK_Y  228
#define PROGRESS_TRACK_W  288
#define PROGRESS_TRACK_H  28
#define PROGRESS_THUMB_R  15
#define PROGRESS_THUMB_CY (PROGRESS_TRACK_Y + PROGRESS_TRACK_H / 2)

#define DEMO_LOGIC_STEP_MS 100U
#define DEMO_FRAME_DT_MAX  50U
#define BALL_SPEED_SCALE   6
#define BALL_POS_FP_SHIFT  8
#define BALL_POS_FP_SCALE  (1 << BALL_POS_FP_SHIFT)
#define CELL_STEP_DIVIDER  2U
#define LIST_STEP_DIVIDER  2U
#define MAP_STEP_DIVIDER   2U

#define CELL_CURSOR_X 256
#define CELL_CURSOR_Y 356
#define CELL_CURSOR_W 10
#define CELL_CURSOR_H 24

#define CLOCK_CARD_X     116
#define CLOCK_CARD_Y     184
#define CLOCK_CARD_W     248
#define CLOCK_CARD_H     106
#define CLOCK_PREFIX_X   204
#define CLOCK_TEXT_Y     222
#define PROGRESS_TEXT_Y  292
#define PROGRESS_TEXT_CX 240

typedef void (*dirty_draw_fn_t)(egui_view_dirty_demo_t *local, egui_canvas_t *canvas);
typedef void (*dirty_step_fn_t)(egui_view_dirty_demo_t *local, egui_view_t *self);

static const char *const s_titles[EGUI_VIEW_DIRTY_DEMO_PAGE_COUNT] = {
        "Line dirty",    "Bouncing balls", "Streaming wave", "Live chart",  "Gauge delta", "Cell changes", "Radar sweep",
        "Spectrum bars", "Digital clock",  "Progress edge",  "List select", "Map marker",  "Chart cursor",
};

static const char *const s_subtitles[EGUI_VIEW_DIRTY_DEMO_PAGE_COUNT] = {
        "Old/new line bounds only",  "Old/new ball circles only", "Changed samples and cursor only", "Dynamic curve band only", "Needle and arc delta only",
        "Two cells and cursor only", "Old/new sweep lines only",  "Changed bar slots only",          "Seconds digits only",     "Moving fill edge only",
        "Old/new rows only",         "Old/new marker spots only", "Old/new cursor strips only",
};

static void dirty_region_local_to_screen(egui_view_t *self, const egui_region_t *local, egui_region_t *screen)
{
    *screen = *local;
    screen->location.x += self->region_screen.location.x;
    screen->location.y += self->region_screen.location.y;
}

static uint8_t dirty_region_is_active(egui_view_t *self, egui_canvas_t *canvas, const egui_region_t *local)
{
    egui_region_t screen;
    dirty_region_local_to_screen(self, local, &screen);
    return egui_canvas_is_region_active(canvas, &screen) ? 1U : 0U;
}

static void dirty_region_add_rect(egui_region_t *acc, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t pad)
{
    egui_view_circle_dirty_add_rect_region(acc, x, y, w, h, pad);
}

static void invalidate_rect(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t pad)
{
    egui_region_t dirty;

    egui_region_init_empty(&dirty);
    dirty_region_add_rect(&dirty, x, y, w, h, pad);
    egui_view_invalidate_region(self, &dirty);
}

static int32_t clamp_i32(int32_t value, int32_t min_value, int32_t max_value)
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

static int32_t ball_pos_to_fp(egui_dim_t value)
{
    return (int32_t)value * BALL_POS_FP_SCALE;
}

static egui_dim_t ball_pos_from_fp(int32_t value)
{
    return (egui_dim_t)((value + BALL_POS_FP_SCALE / 2) / BALL_POS_FP_SCALE);
}

static int32_t ball_delta_to_fp(int8_t velocity, uint16_t dt)
{
    return ((int32_t)velocity * BALL_SPEED_SCALE * (int32_t)dt * BALL_POS_FP_SCALE) / 100;
}

static void draw_text(egui_canvas_t *canvas, const char *text, egui_dim_t x, egui_dim_t y, egui_color_t color)
{
    egui_canvas_draw_text(canvas, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, text, x, y, color, EGUI_ALPHA_100);
}

static void measure_text(const char *text, egui_dim_t *width, egui_dim_t *height)
{
    const egui_font_t *font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    if (font != NULL && font->api != NULL && font->api->get_str_size != NULL && font->api->get_str_size(font, text, 0, 0, width, height) == 0)
    {
        return;
    }

    *width = (egui_dim_t)(strlen(text) * 8U);
    *height = 16;
}

static void get_text_region(const char *text, egui_dim_t x, egui_dim_t y, egui_dim_t pad, egui_region_t *region)
{
    egui_dim_t width;
    egui_dim_t height;

    measure_text(text, &width, &height);
    region->location.x = x - pad;
    region->location.y = y - pad;
    region->size.width = width + pad * 2 + 1;
    region->size.height = height + pad * 2 + 1;
}

static void draw_header(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    egui_canvas_draw_rectangle_fill(canvas, 0, 0, DEMO_W, DEMO_H, COLOR_BG, EGUI_ALPHA_100);
    egui_canvas_draw_rectangle_fill(canvas, 0, 0, DEMO_W, 62, EGUI_COLOR_MAKE(18, 25, 36), EGUI_ALPHA_100);
    draw_text(canvas, s_titles[local->page], 24, 16, COLOR_TEXT);
    draw_text(canvas, s_subtitles[local->page], 24, 40, COLOR_MUTED);

    for (uint8_t i = 0; i < EGUI_VIEW_DIRTY_DEMO_PAGE_COUNT; i++)
    {
        egui_color_t color = (i == local->page) ? COLOR_BLUE : COLOR_GRID;
        egui_canvas_draw_circle_fill(canvas, 294 + i * 14, 31, 4, color, EGUI_ALPHA_100);
    }
}

static void draw_demo_panel(egui_canvas_t *canvas)
{
    egui_canvas_draw_round_rectangle_fill(canvas, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H, 8, COLOR_PANEL, EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle(canvas, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H, 8, 1, COLOR_GRID, EGUI_ALPHA_100);
}

static void draw_grid(egui_canvas_t *canvas, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t step)
{
    egui_dim_t pos;

    for (pos = x + step; pos < x + w; pos += step)
    {
        egui_canvas_draw_line(canvas, pos, y, pos, y + h, 1, COLOR_GRID, EGUI_ALPHA_60);
    }
    for (pos = y + step; pos < y + h; pos += step)
    {
        egui_canvas_draw_line(canvas, x, pos, x + w, pos, 1, COLOR_GRID, EGUI_ALPHA_60);
    }
}

static void line_endpoint(int16_t angle, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_circle_dirty_get_circle_point(LINE_CENTER_X, LINE_CENTER_Y, LINE_RADIUS, angle, x, y);
}

static void step_line(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x1;
    egui_dim_t y1;
    egui_dim_t old_x1;
    egui_dim_t old_y1;
    egui_region_t dirty;

    local->old_line_angle = local->line_angle;
    local->line_angle = (int16_t)(local->line_angle + local->line_delta);
    if (local->line_angle >= 332 || local->line_angle <= 208)
    {
        local->line_delta = (int8_t)-local->line_delta;
        local->line_angle = (int16_t)(local->old_line_angle + local->line_delta);
    }

    line_endpoint(local->old_line_angle, &old_x1, &old_y1);
    line_endpoint(local->line_angle, &x1, &y1);
    x0 = LINE_CENTER_X;
    y0 = LINE_CENTER_Y;
    egui_region_init_empty(&dirty);
    egui_view_circle_dirty_add_line_region(&dirty, x0, y0, old_x1, old_y1, LINE_STROKE_W, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
    egui_view_circle_dirty_add_line_region(&dirty, x0, y0, x1, y1, LINE_STROKE_W, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
    egui_view_circle_dirty_add_circle_region(&dirty, x0, y0, 9, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 1);
    egui_view_circle_dirty_add_circle_region(&dirty, old_x1, old_y1, LINE_END_RADIUS, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
    egui_view_circle_dirty_add_circle_region(&dirty, x1, y1, LINE_END_RADIUS, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
    egui_view_invalidate_region(self, &dirty);
}

static void draw_line_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    egui_dim_t end_x;
    egui_dim_t end_y;
    EGUI_REGION_DEFINE(active, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);

    draw_demo_panel(canvas);
    draw_grid(canvas, CONTENT_X + 16, CONTENT_Y + 16, CONTENT_W - 32, CONTENT_H - 32, 32);
    egui_canvas_draw_circle(canvas, LINE_CENTER_X, LINE_CENTER_Y, 150, 2, COLOR_GRID, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(canvas, LINE_CENTER_X, LINE_CENTER_Y, 9, COLOR_AMBER, EGUI_ALPHA_100);

    if (dirty_region_is_active(EGUI_VIEW_OF(local), canvas, &active))
    {
        line_endpoint(local->line_angle, &end_x, &end_y);
        egui_canvas_draw_line_round_cap_hq(canvas, LINE_CENTER_X, LINE_CENTER_Y, end_x, end_y, LINE_STROKE_W, COLOR_BLUE, EGUI_ALPHA_100);
        egui_canvas_draw_circle_fill(canvas, end_x, end_y, LINE_END_RADIUS, COLOR_TEAL, EGUI_ALPHA_100);
    }

    draw_text(canvas, "moving line", 188, 376, COLOR_TEXT);
}

static void step_balls(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    static const egui_region_t bounds = {{CONTENT_X + 22, CONTENT_Y + 22}, {CONTENT_W - 44, CONTENT_H - 44}};
    uint16_t dt = local->frame_delta_ms;

    if (dt == 0U)
    {
        dt = 1000U / EGUI_CONFIG_MAX_FPS;
    }
    if (dt > DEMO_FRAME_DT_MAX)
    {
        dt = DEMO_FRAME_DT_MAX;
    }

    for (uint8_t i = 0; i < BALL_COUNT; i++)
    {
        egui_view_dirty_demo_ball_t *ball = &local->balls[i];
        int32_t min_x_fp = ball_pos_to_fp(bounds.location.x + ball->radius);
        int32_t max_x_fp = ball_pos_to_fp(bounds.location.x + bounds.size.width - ball->radius);
        int32_t min_y_fp = ball_pos_to_fp(bounds.location.y + ball->radius);
        int32_t max_y_fp = ball_pos_to_fp(bounds.location.y + bounds.size.height - ball->radius);
        int32_t next_x_fp;
        int32_t next_y_fp;

        ball->old_x = ball->x;
        ball->old_y = ball->y;

        next_x_fp = ball->x_fp + ball_delta_to_fp(ball->vx, dt);
        next_y_fp = ball->y_fp + ball_delta_to_fp(ball->vy, dt);
        if (next_x_fp < min_x_fp || next_x_fp > max_x_fp)
        {
            ball->vx = (int8_t)-ball->vx;
            next_x_fp = ball->x_fp + ball_delta_to_fp(ball->vx, dt);
        }
        if (next_y_fp < min_y_fp || next_y_fp > max_y_fp)
        {
            ball->vy = (int8_t)-ball->vy;
            next_y_fp = ball->y_fp + ball_delta_to_fp(ball->vy, dt);
        }

        ball->x_fp = clamp_i32(next_x_fp, min_x_fp, max_x_fp);
        ball->y_fp = clamp_i32(next_y_fp, min_y_fp, max_y_fp);
        ball->x = ball_pos_from_fp(ball->x_fp);
        ball->y = ball_pos_from_fp(ball->y_fp);
    }

    for (uint8_t i = 0; i < BALL_COUNT; i++)
    {
        egui_view_dirty_demo_ball_t *a = &local->balls[i];

        for (uint8_t j = (uint8_t)(i + 1U); j < BALL_COUNT; j++)
        {
            egui_view_dirty_demo_ball_t *b = &local->balls[j];
            int32_t dx = (int32_t)a->x - b->x;
            int32_t dy = (int32_t)a->y - b->y;
            int32_t min_dist = (int32_t)a->radius + b->radius;

            if ((dx * dx + dy * dy) <= (min_dist * min_dist))
            {
                int8_t tmp_vx = a->vx;
                int8_t tmp_vy = a->vy;

                a->vx = b->vx;
                a->vy = b->vy;
                b->vx = tmp_vx;
                b->vy = tmp_vy;
            }
        }
    }

    for (uint8_t i = 0; i < BALL_COUNT; i++)
    {
        egui_view_dirty_demo_ball_t *ball = &local->balls[i];
        egui_region_t dirty;

        egui_region_init_empty(&dirty);
        egui_view_circle_dirty_add_circle_region(&dirty, ball->old_x, ball->old_y, ball->radius, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
        egui_view_circle_dirty_add_circle_region(&dirty, ball->x, ball->y, ball->radius, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
        egui_view_invalidate_region(self, &dirty);
    }
}

static void draw_balls_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    draw_demo_panel(canvas);
    draw_grid(canvas, CONTENT_X + 18, CONTENT_Y + 18, CONTENT_W - 36, CONTENT_H - 36, 38);

    for (uint8_t i = 0; i < BALL_COUNT; i++)
    {
        egui_view_dirty_demo_ball_t *ball = &local->balls[i];
        EGUI_REGION_DEFINE(ball_rect, ball->x - ball->radius - 2, ball->y - ball->radius - 2, ball->radius * 2 + 4, ball->radius * 2 + 4);

        if (!dirty_region_is_active(EGUI_VIEW_OF(local), canvas, &ball_rect))
        {
            continue;
        }

        egui_canvas_draw_circle_fill(canvas, ball->x, ball->y, ball->radius, ball->color, EGUI_ALPHA_90);
        egui_canvas_draw_circle(canvas, ball->x, ball->y, ball->radius, 1, EGUI_COLOR_WHITE, EGUI_ALPHA_60);
    }
}

static uint8_t wave_value_for_tick(uint16_t tick)
{
    static const uint8_t pattern[24] = {62, 66, 71, 82, 102, 144, 112, 86, 73, 70, 68, 66, 64, 58, 44, 30, 48, 62, 76, 92, 86, 76, 69, 64};
    return pattern[tick % EGUI_ARRAY_SIZE(pattern)];
}

static void step_wave(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    uint8_t new_index;
    egui_dim_t sample_w = 4;

    local->old_wave_head = local->wave_head;
    local->wave_head = (uint8_t)((local->wave_head + 1U) % WAVE_COUNT);
    new_index = local->wave_head;
    local->wave_values[new_index] = wave_value_for_tick(local->tick);

    invalidate_rect(self, CONTENT_X + 16 + local->old_wave_head * sample_w - 5, CONTENT_Y + 28, 12, CONTENT_H - 56, 2);
    invalidate_rect(self, CONTENT_X + 16 + new_index * sample_w - 7, CONTENT_Y + 28, 18, CONTENT_H - 56, 2);
    if (new_index > 0U)
    {
        invalidate_rect(self, CONTENT_X + 16 + (new_index - 1U) * sample_w - 7, CONTENT_Y + 28, 18, CONTENT_H - 56, 2);
    }
    if (new_index + 1U < WAVE_COUNT)
    {
        invalidate_rect(self, CONTENT_X + 16 + (new_index + 1U) * sample_w - 7, CONTENT_Y + 28, 18, CONTENT_H - 56, 2);
    }
}

static void draw_wave_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    egui_dim_t x0 = CONTENT_X + 16;
    egui_dim_t y_mid = CONTENT_Y + 196;
    egui_dim_t sample_w = 4;

    draw_demo_panel(canvas);
    draw_grid(canvas, x0, CONTENT_Y + 32, WAVE_COUNT * sample_w, 248, 24);
    egui_canvas_draw_line(canvas, x0, y_mid, x0 + WAVE_COUNT * sample_w, y_mid, 1, COLOR_GRID, EGUI_ALPHA_100);

    for (uint8_t i = 1; i < WAVE_COUNT; i++)
    {
        egui_dim_t x_a = x0 + (i - 1U) * sample_w;
        egui_dim_t x_b = x0 + i * sample_w;
        egui_dim_t y_a = y_mid - local->wave_values[i - 1U];
        egui_dim_t y_b = y_mid - local->wave_values[i];
        EGUI_REGION_DEFINE(seg_rect, x_a - 4, EGUI_MIN(y_a, y_b) - 4, sample_w + 8, EGUI_ABS((int)y_a - (int)y_b) + 8);

        if (!dirty_region_is_active(EGUI_VIEW_OF(local), canvas, &seg_rect))
        {
            continue;
        }

        egui_canvas_draw_line(canvas, x_a, y_a, x_b, y_b, 2, COLOR_GREEN, EGUI_ALPHA_100);
    }

    {
        egui_dim_t cursor_x = x0 + local->wave_head * sample_w;
        egui_canvas_draw_line(canvas, cursor_x, CONTENT_Y + 32, cursor_x, CONTENT_Y + 280, 2, COLOR_AMBER, EGUI_ALPHA_80);
    }
}

static uint8_t live_value_for_tick(uint16_t tick)
{
    static const int8_t wave[32] = {0,  18, 35, 48, 56, 52, 38,  18,  -4,  -28, -48, -60, -52, -34, -12, 8,
                                    26, 44, 58, 54, 34, 10, -16, -40, -56, -62, -44, -22, 0,   20,  36,  18};
    uint16_t phase = tick & 0xFFU;
    uint8_t envelope = 18;
    int16_t value;

    if (phase >= 56U && phase < 96U)
    {
        envelope = (uint8_t)(18U + ((phase - 56U) * 70U) / 40U);
    }
    else if (phase >= 96U && phase < 144U)
    {
        envelope = 88;
    }
    else if (phase >= 144U && phase < 184U)
    {
        envelope = (uint8_t)(88U - ((phase - 144U) * 70U) / 40U);
    }

    value = (int16_t)(116 + ((int16_t)wave[tick % EGUI_ARRAY_SIZE(wave)] * envelope) / 64 + (int16_t)((tick * 5U) % 17U) - 8);
    return (uint8_t)clamp_i32(value, 32, 208);
}

static void draw_live_chart_frame(egui_canvas_t *canvas)
{
    draw_demo_panel(canvas);

    for (egui_dim_t pos = LIVE_PLOT_Y + 25; pos < LIVE_PLOT_Y + 220; pos += 25)
    {
        egui_canvas_draw_line(canvas, LIVE_PLOT_X, pos, LIVE_PLOT_X + LIVE_PLOT_WIDTH - 1, pos, 1, COLOR_GRID, EGUI_ALPHA_60);
    }

    egui_canvas_draw_line(canvas, LIVE_PLOT_X, LIVE_PLOT_Y, LIVE_PLOT_X, LIVE_BASE_Y, 2, COLOR_MUTED, EGUI_ALPHA_100);
    egui_canvas_draw_line(canvas, LIVE_PLOT_X, LIVE_BASE_Y, LIVE_PLOT_X + LIVE_PLOT_WIDTH - 1, LIVE_BASE_Y, 2, COLOR_MUTED, EGUI_ALPHA_100);
}

static void get_live_curve_region(const uint8_t *values, egui_region_t *region)
{
    EGUI_REGION_DEFINE(plot_region, LIVE_PLOT_X, LIVE_SCROLL_Y, LIVE_PLOT_WIDTH, LIVE_SCROLL_H);

    egui_region_init_empty(region);

    for (uint8_t i = 1; i < LIVE_COUNT; i++)
    {
        egui_dim_t x_a = LIVE_PLOT_X + (i - 1U) * LIVE_SAMPLE_W;
        egui_dim_t y_a = LIVE_BASE_Y - values[i - 1U];
        egui_dim_t y_b = LIVE_BASE_Y - values[i];
        EGUI_REGION_DEFINE(seg_region, x_a - 5, EGUI_MIN(y_a, y_b) - 5, LIVE_SAMPLE_W + 10, EGUI_ABS((int)y_a - (int)y_b) + 10);

        egui_region_intersect(&seg_region, &seg_region, &plot_region);
        egui_view_circle_dirty_union_region(region, &seg_region);
    }
}

static void step_live_chart(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    egui_region_t dirty;
    egui_region_t curve_dirty;
    uint8_t old_tail_value;

    local->old_live_head = local->live_head;
    old_tail_value = local->live_values[LIVE_COUNT - 1U];

    get_live_curve_region(local->live_values, &dirty);
    egui_view_circle_dirty_add_circle_region(&dirty, LIVE_PLOT_X + (LIVE_COUNT - 1U) * LIVE_SAMPLE_W, LIVE_BASE_Y - old_tail_value, 5,
                                             EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 1);

    local->live_head = (uint8_t)((local->live_head + 1U) % LIVE_COUNT);
    memmove(&local->live_values[0], &local->live_values[1], (LIVE_COUNT - 1U) * sizeof(local->live_values[0]));
    local->live_values[LIVE_COUNT - 1U] = live_value_for_tick((uint16_t)(local->tick + LIVE_COUNT - 1U));

    get_live_curve_region(local->live_values, &curve_dirty);
    egui_view_circle_dirty_union_region(&dirty, &curve_dirty);
    egui_view_circle_dirty_add_circle_region(&dirty, LIVE_PLOT_X + (LIVE_COUNT - 1U) * LIVE_SAMPLE_W, LIVE_BASE_Y - local->live_values[LIVE_COUNT - 1U], 5,
                                             EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 1);

    egui_view_invalidate_region(self, &dirty);
}

static void draw_live_chart_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    draw_live_chart_frame(canvas);

    for (uint8_t i = 1; i < LIVE_COUNT; i++)
    {
        egui_dim_t x_a = LIVE_PLOT_X + (i - 1U) * LIVE_SAMPLE_W;
        egui_dim_t x_b = LIVE_PLOT_X + i * LIVE_SAMPLE_W;
        egui_dim_t y_a = LIVE_BASE_Y - local->live_values[i - 1U];
        egui_dim_t y_b = LIVE_BASE_Y - local->live_values[i];
        EGUI_REGION_DEFINE(seg_rect, x_a - 5, EGUI_MIN(y_a, y_b) - 5, LIVE_SAMPLE_W + 10, EGUI_ABS((int)y_a - (int)y_b) + 10);

        if (!dirty_region_is_active(EGUI_VIEW_OF(local), canvas, &seg_rect))
        {
            continue;
        }

        egui_canvas_draw_line_round_cap_hq(canvas, x_a, y_a, x_b, y_b, 3, COLOR_GREEN, EGUI_ALPHA_100);
    }

    {
        egui_dim_t cursor_x = LIVE_PLOT_X + (LIVE_COUNT - 1U) * LIVE_SAMPLE_W;
        egui_dim_t cursor_y = LIVE_BASE_Y - local->live_values[LIVE_COUNT - 1U];

        egui_canvas_draw_line(canvas, cursor_x, LIVE_PLOT_Y, cursor_x, LIVE_BASE_Y, 1, COLOR_AMBER, EGUI_ALPHA_70);
        egui_canvas_draw_circle_fill(canvas, cursor_x, cursor_y, 5, COLOR_AMBER, EGUI_ALPHA_100);
    }

    egui_canvas_draw_line(canvas, LIVE_PLOT_X, LIVE_PLOT_Y, LIVE_PLOT_X, LIVE_BASE_Y, 2, COLOR_MUTED, EGUI_ALPHA_100);
}

static void gauge_point(int16_t angle, egui_dim_t radius, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_circle_dirty_get_circle_point(240, 260, radius, angle, x, y);
}

static void step_gauge(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    egui_dim_t old_x;
    egui_dim_t old_y;
    egui_dim_t new_x;
    egui_dim_t new_y;
    egui_region_t dirty;
    egui_region_t arc_dirty;
    int16_t dirty_start_angle;
    uint16_t dirty_sweep;

    local->old_gauge_angle = local->gauge_angle;
    local->gauge_angle = (int16_t)(local->gauge_angle + local->gauge_delta);
    if (local->gauge_angle >= 335 || local->gauge_angle <= 205)
    {
        local->gauge_delta = (int8_t)-local->gauge_delta;
        local->gauge_angle = (int16_t)(local->old_gauge_angle + local->gauge_delta);
    }

    gauge_point(local->old_gauge_angle, 118, &old_x, &old_y);
    gauge_point(local->gauge_angle, 118, &new_x, &new_y);
    egui_region_init_empty(&dirty);
    dirty_start_angle = EGUI_MIN(local->old_gauge_angle, local->gauge_angle);
    dirty_sweep = (uint16_t)EGUI_ABS((int)local->gauge_angle - (int)local->old_gauge_angle);
    if (egui_view_circle_dirty_compute_arc_region(240, 260, 137, 10 / 2 + EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2, dirty_start_angle, dirty_sweep, &arc_dirty))
    {
        egui_view_circle_dirty_union_region(&dirty, &arc_dirty);
    }
    egui_view_circle_dirty_add_line_region(&dirty, 240, 260, old_x, old_y, 6, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
    egui_view_circle_dirty_add_line_region(&dirty, 240, 260, new_x, new_y, 6, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
    egui_view_circle_dirty_add_circle_region(&dirty, 240, 260, 12, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 1);
    egui_view_invalidate_region(self, &dirty);
}

static void draw_gauge_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    egui_dim_t tip_x;
    egui_dim_t tip_y;

    draw_demo_panel(canvas);
    egui_canvas_draw_arc(canvas, 240, 260, 142, 205, 335, 10, COLOR_GRID, EGUI_ALPHA_100);
    egui_canvas_draw_arc(canvas, 240, 260, 142, 205, local->gauge_angle, 10, COLOR_BLUE, EGUI_ALPHA_100);

    for (int16_t angle = 210; angle <= 330; angle += 20)
    {
        egui_dim_t x1;
        egui_dim_t y1;
        egui_dim_t x2;
        egui_dim_t y2;

        gauge_point(angle, 126, &x1, &y1);
        gauge_point(angle, 144, &x2, &y2);
        egui_canvas_draw_line(canvas, x1, y1, x2, y2, 2, COLOR_MUTED, EGUI_ALPHA_80);
    }

    gauge_point(local->gauge_angle, 118, &tip_x, &tip_y);
    egui_canvas_draw_line_round_cap_hq(canvas, 240, 260, tip_x, tip_y, 6, COLOR_AMBER, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(canvas, 240, 260, 12, COLOR_ORANGE, EGUI_ALPHA_100);
    draw_text(canvas, "needle", 216, 346, COLOR_TEXT);
}

static void get_cell_region(uint8_t index, egui_region_t *region)
{
    uint8_t col = index % 4U;
    uint8_t row = index / 4U;

    region->location.x = CONTENT_X + 28 + col * 92;
    region->location.y = CONTENT_Y + 42 + row * 76;
    region->size.width = 70;
    region->size.height = 54;
}

static void step_cells(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    egui_region_t cell;

    if ((local->tick % CELL_STEP_DIVIDER) != 0U)
    {
        return;
    }

    local->old_selected_cell = local->selected_cell;
    local->selected_cell = (uint8_t)((local->selected_cell + 1U) % CELL_COUNT);
    local->cursor_on = local->cursor_on ? 0U : 1U;

    get_cell_region(local->old_selected_cell, &cell);
    egui_view_invalidate_region(self, &cell);
    get_cell_region(local->selected_cell, &cell);
    egui_view_invalidate_region(self, &cell);
    invalidate_rect(self, CELL_CURSOR_X, CELL_CURSOR_Y, CELL_CURSOR_W, CELL_CURSOR_H, 1);
}

static void draw_cells_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    draw_demo_panel(canvas);

    for (uint8_t i = 0; i < CELL_COUNT; i++)
    {
        egui_region_t cell;
        egui_color_t fill;
        get_cell_region(i, &cell);

        if (!dirty_region_is_active(EGUI_VIEW_OF(local), canvas, &cell))
        {
            continue;
        }

        fill = (i == local->selected_cell) ? COLOR_VIOLET : COLOR_SURFACE;
        egui_canvas_draw_round_rectangle_fill(canvas, cell.location.x, cell.location.y, cell.size.width, cell.size.height, 7, fill, EGUI_ALPHA_100);
        egui_canvas_draw_round_rectangle(canvas, cell.location.x, cell.location.y, cell.size.width, cell.size.height, 7, 1, COLOR_GRID, EGUI_ALPHA_100);
    }

    draw_text(canvas, "cursor", 194, 358, COLOR_TEXT);
    if (local->cursor_on)
    {
        egui_canvas_draw_rectangle_fill(canvas, CELL_CURSOR_X, CELL_CURSOR_Y, CELL_CURSOR_W, CELL_CURSOR_H, COLOR_GREEN, EGUI_ALPHA_100);
    }
}

static void radar_endpoint(int16_t angle, egui_dim_t radius, egui_dim_t *x, egui_dim_t *y)
{
    egui_view_circle_dirty_get_circle_point(240, 256, radius, angle, x, y);
}

static void step_radar(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    egui_dim_t old_x;
    egui_dim_t old_y;
    egui_dim_t new_x;
    egui_dim_t new_y;
    egui_region_t dirty;

    local->old_radar_angle = local->radar_angle;
    local->radar_angle = (int16_t)((local->radar_angle + 13) % 360);

    radar_endpoint(local->old_radar_angle, 146, &old_x, &old_y);
    radar_endpoint(local->radar_angle, 146, &new_x, &new_y);
    egui_region_init_empty(&dirty);
    egui_view_circle_dirty_add_line_region(&dirty, 240, 256, old_x, old_y, 4, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
    egui_view_circle_dirty_add_line_region(&dirty, 240, 256, new_x, new_y, 4, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
    egui_view_circle_dirty_add_circle_region(&dirty, 240, 256, 5, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 1);
    egui_view_invalidate_region(self, &dirty);
}

static void draw_radar_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    egui_dim_t end_x;
    egui_dim_t end_y;
    EGUI_REGION_DEFINE(active, CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H);

    draw_demo_panel(canvas);
    for (egui_dim_t r = 46; r <= 138; r += 46)
    {
        egui_canvas_draw_circle(canvas, 240, 256, r, 1, COLOR_GRID, EGUI_ALPHA_100);
    }
    egui_canvas_draw_line(canvas, 94, 256, 386, 256, 1, COLOR_GRID, EGUI_ALPHA_80);
    egui_canvas_draw_line(canvas, 240, 110, 240, 402, 1, COLOR_GRID, EGUI_ALPHA_80);
    egui_canvas_draw_circle_fill(canvas, 188, 202, 4, COLOR_GREEN, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(canvas, 300, 316, 5, COLOR_AMBER, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(canvas, 272, 174, 3, COLOR_RED, EGUI_ALPHA_100);

    if (dirty_region_is_active(EGUI_VIEW_OF(local), canvas, &active))
    {
        radar_endpoint(local->radar_angle, 146, &end_x, &end_y);
        egui_canvas_draw_line_round_cap_hq(canvas, 240, 256, end_x, end_y, 4, COLOR_TEAL, EGUI_ALPHA_90);
        egui_canvas_draw_circle_fill(canvas, 240, 256, 5, COLOR_TEAL, EGUI_ALPHA_100);
    }
}

static uint8_t spectrum_value_for(uint16_t tick, uint8_t index)
{
    static const uint8_t base[SPECTRUM_COUNT] = {36, 78, 52, 118, 90, 146, 70, 108, 132, 62, 154, 96, 124, 82, 140, 58};
    uint8_t wave = (uint8_t)((tick * (uint16_t)(index + 3U) + index * 17U) % 48U);

    return (uint8_t)(base[index] + wave - 24U);
}

static void get_spectrum_bar_region(uint8_t index, egui_region_t *region)
{
    region->location.x = CONTENT_X + 30 + index * 23;
    region->location.y = CONTENT_Y + 52;
    region->size.width = 14;
    region->size.height = 250;
}

static void step_spectrum(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    egui_region_t bar;
    uint8_t index = local->spectrum_index;

    local->spectrum_values[index] = spectrum_value_for(local->tick, index);
    get_spectrum_bar_region(index, &bar);
    egui_view_invalidate_region(self, &bar);

    index = (uint8_t)((index + 5U) % SPECTRUM_COUNT);
    local->spectrum_values[index] = spectrum_value_for((uint16_t)(local->tick + 9U), index);
    get_spectrum_bar_region(index, &bar);
    egui_view_invalidate_region(self, &bar);

    local->spectrum_index = (uint8_t)((local->spectrum_index + 1U) % SPECTRUM_COUNT);
}

static void draw_spectrum_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    draw_demo_panel(canvas);
    draw_grid(canvas, CONTENT_X + 20, CONTENT_Y + 40, CONTENT_W - 40, 270, 30);

    for (uint8_t i = 0; i < SPECTRUM_COUNT; i++)
    {
        egui_region_t bar;
        egui_dim_t value;
        egui_dim_t fill_y;
        egui_color_t color = (i & 1U) ? COLOR_GREEN : COLOR_BLUE;

        get_spectrum_bar_region(i, &bar);
        if (!dirty_region_is_active(EGUI_VIEW_OF(local), canvas, &bar))
        {
            continue;
        }

        value = local->spectrum_values[i];
        fill_y = bar.location.y + bar.size.height - value;
        egui_canvas_draw_round_rectangle_fill(canvas, bar.location.x, bar.location.y, bar.size.width, bar.size.height, 4, COLOR_SURFACE, EGUI_ALPHA_100);
        egui_canvas_draw_round_rectangle_fill(canvas, bar.location.x, fill_y, bar.size.width, value, 4, color, EGUI_ALPHA_100);
    }
}

static void format_clock_text(uint8_t seconds, char *buf)
{
    buf[0] = '1';
    buf[1] = '2';
    buf[2] = ':';
    buf[3] = '4';
    buf[4] = '8';
    buf[5] = ':';
    buf[6] = (char)('0' + seconds / 10U);
    buf[7] = (char)('0' + seconds % 10U);
    buf[8] = '\0';
}

static void step_clock(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    char old_text[9];
    char new_text[9];
    egui_region_t old_region;
    egui_region_t new_region;

    local->old_clock_seconds = local->clock_seconds;
    local->clock_seconds = (uint8_t)((local->clock_seconds + 1U) % 60U);

    format_clock_text(local->old_clock_seconds, old_text);
    format_clock_text(local->clock_seconds, new_text);
    get_text_region(&old_text[6], CLOCK_PREFIX_X + 58, CLOCK_TEXT_Y, 2, &old_region);
    get_text_region(&new_text[6], CLOCK_PREFIX_X + 58, CLOCK_TEXT_Y, 2, &new_region);
    egui_view_invalidate_region(self, &old_region);
    egui_view_invalidate_region(self, &new_region);
}

static void draw_clock_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    char buf[9];
    EGUI_REGION_DEFINE(seconds_region, CLOCK_PREFIX_X + 56, CLOCK_TEXT_Y - 2, 34, 24);

    draw_demo_panel(canvas);
    egui_canvas_draw_round_rectangle_fill(canvas, CLOCK_CARD_X, CLOCK_CARD_Y, CLOCK_CARD_W, CLOCK_CARD_H, 8, COLOR_SURFACE, EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle(canvas, CLOCK_CARD_X, CLOCK_CARD_Y, CLOCK_CARD_W, CLOCK_CARD_H, 8, 1, COLOR_GRID, EGUI_ALPHA_100);
    draw_text(canvas, "12:48:", CLOCK_PREFIX_X, CLOCK_TEXT_Y, COLOR_MUTED);

    if (dirty_region_is_active(EGUI_VIEW_OF(local), canvas, &seconds_region))
    {
        format_clock_text(local->clock_seconds, buf);
        draw_text(canvas, &buf[6], CLOCK_PREFIX_X + 58, CLOCK_TEXT_Y, COLOR_AMBER);
    }
}

static void format_progress_text(uint8_t value, char *text)
{
    uint8_t pos = 0;

    if (value >= 100U)
    {
        text[pos++] = '1';
        text[pos++] = '0';
        text[pos++] = '0';
    }
    else if (value >= 10U)
    {
        text[pos++] = (char)('0' + value / 10U);
        text[pos++] = (char)('0' + value % 10U);
    }
    else
    {
        text[pos++] = (char)('0' + value);
    }
    text[pos++] = '%';
    text[pos] = '\0';
}

static void get_progress_text_region(uint8_t value, egui_region_t *region)
{
    char text[5];
    egui_dim_t width;
    egui_dim_t height;

    format_progress_text(value, text);
    measure_text(text, &width, &height);
    region->location.x = PROGRESS_TEXT_CX - width / 2 - 2;
    region->location.y = PROGRESS_TEXT_Y - 2;
    region->size.width = width + 5;
    region->size.height = height + 5;
}

static void step_progress(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    egui_dim_t x;
    egui_dim_t old_x;
    egui_dim_t min_x;
    egui_dim_t max_x;
    egui_region_t old_text_region;
    egui_region_t new_text_region;

    local->old_progress_value = local->progress_value;
    if ((int16_t)local->progress_value + local->progress_delta >= 100 || (int16_t)local->progress_value + local->progress_delta <= 3)
    {
        local->progress_delta = (int8_t)-local->progress_delta;
    }
    local->progress_value = (uint8_t)((int16_t)local->progress_value + local->progress_delta);

    old_x = PROGRESS_TRACK_X + ((egui_dim_t)local->old_progress_value * PROGRESS_TRACK_W) / 100;
    x = PROGRESS_TRACK_X + ((egui_dim_t)local->progress_value * PROGRESS_TRACK_W) / 100;
    min_x = EGUI_MIN(old_x, x) - PROGRESS_THUMB_R;
    max_x = EGUI_MAX(old_x, x) + PROGRESS_THUMB_R;
    invalidate_rect(self, min_x, PROGRESS_THUMB_CY - PROGRESS_THUMB_R, max_x - min_x + 1, PROGRESS_THUMB_R * 2 + 1, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
    get_progress_text_region(local->old_progress_value, &old_text_region);
    get_progress_text_region(local->progress_value, &new_text_region);
    egui_view_invalidate_region(self, &old_text_region);
    egui_view_invalidate_region(self, &new_text_region);
}

static void draw_progress_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    egui_dim_t fill_w = ((egui_dim_t)local->progress_value * PROGRESS_TRACK_W) / 100;
    char text[5];

    draw_demo_panel(canvas);
    egui_canvas_draw_round_rectangle_fill(canvas, PROGRESS_TRACK_X, PROGRESS_TRACK_Y, PROGRESS_TRACK_W, PROGRESS_TRACK_H, 14, COLOR_SURFACE, EGUI_ALPHA_100);
    egui_canvas_draw_round_rectangle(canvas, PROGRESS_TRACK_X, PROGRESS_TRACK_Y, PROGRESS_TRACK_W, PROGRESS_TRACK_H, 14, 1, COLOR_GRID, EGUI_ALPHA_100);
    if (fill_w > 0)
    {
        egui_canvas_draw_round_rectangle_fill(canvas, PROGRESS_TRACK_X, PROGRESS_TRACK_Y, fill_w, PROGRESS_TRACK_H, 14, COLOR_GREEN, EGUI_ALPHA_100);
    }
    egui_canvas_draw_circle_fill(canvas, PROGRESS_TRACK_X + fill_w, PROGRESS_THUMB_CY, PROGRESS_THUMB_R, COLOR_AMBER, EGUI_ALPHA_100);

    format_progress_text(local->progress_value, text);
    {
        egui_dim_t text_w;
        egui_dim_t text_h;

        EGUI_UNUSED(text_h);
        measure_text(text, &text_w, &text_h);
        draw_text(canvas, text, PROGRESS_TEXT_CX - text_w / 2, PROGRESS_TEXT_Y, COLOR_TEXT);
    }
}

static void get_list_row_region(uint8_t row, egui_region_t *region)
{
    region->location.x = 78;
    region->location.y = CONTENT_Y + 38 + row * 42;
    region->size.width = 324;
    region->size.height = 34;
}

static void step_list(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    egui_region_t row;

    if ((local->tick % LIST_STEP_DIVIDER) != 0U)
    {
        return;
    }

    local->old_selected_row = local->selected_row;
    local->selected_row = (uint8_t)((local->selected_row + 1U) % 6U);
    get_list_row_region(local->old_selected_row, &row);
    egui_view_invalidate_region(self, &row);
    get_list_row_region(local->selected_row, &row);
    egui_view_invalidate_region(self, &row);
}

static void draw_list_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    static const char *const labels[6] = {"Inbox sync", "Sensor log", "BLE packet", "Flash page", "Widget tree", "Idle tick"};

    draw_demo_panel(canvas);
    for (uint8_t i = 0; i < 6U; i++)
    {
        egui_region_t row;
        egui_color_t fill;

        get_list_row_region(i, &row);
        if (!dirty_region_is_active(EGUI_VIEW_OF(local), canvas, &row))
        {
            continue;
        }

        fill = (i == local->selected_row) ? COLOR_BLUE : COLOR_SURFACE;
        egui_canvas_draw_round_rectangle_fill(canvas, row.location.x, row.location.y, row.size.width, row.size.height, 5, fill, EGUI_ALPHA_100);
        egui_canvas_draw_circle_fill(canvas, row.location.x + 18, row.location.y + 17, 5, (i == local->selected_row) ? COLOR_AMBER : COLOR_GRID,
                                     EGUI_ALPHA_100);
        draw_text(canvas, labels[i], row.location.x + 38, row.location.y + 9, COLOR_TEXT);
    }
}

static void get_route_point(uint8_t index, egui_dim_t *x, egui_dim_t *y)
{
    static const egui_dim_t route[10][2] = {
            {112, 336}, {146, 300}, {176, 318}, {212, 268}, {248, 284}, {280, 230}, {318, 246}, {346, 196}, {382, 216}, {404, 168},
    };
    uint8_t safe_index = index % EGUI_ARRAY_SIZE(route);

    *x = route[safe_index][0];
    *y = route[safe_index][1];
}

static void step_map(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    egui_dim_t x;
    egui_dim_t y;
    egui_region_t dirty;

    if ((local->tick % MAP_STEP_DIVIDER) != 0U)
    {
        return;
    }

    local->old_route_index = local->route_index;
    local->route_index = (uint8_t)((local->route_index + 1U) % 10U);

    egui_region_init_empty(&dirty);
    get_route_point(local->old_route_index, &x, &y);
    egui_view_circle_dirty_add_circle_region(&dirty, x, y, 12, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
    egui_view_invalidate_region(self, &dirty);

    egui_region_init_empty(&dirty);
    get_route_point(local->route_index, &x, &y);
    egui_view_circle_dirty_add_circle_region(&dirty, x, y, 12, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 2);
    egui_view_invalidate_region(self, &dirty);
}

static void draw_map_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    egui_dim_t marker_x;
    egui_dim_t marker_y;

    draw_demo_panel(canvas);
    for (uint8_t i = 0; i < 5U; i++)
    {
        egui_canvas_draw_line(canvas, 82, CONTENT_Y + 52 + i * 52, 398, CONTENT_Y + 52 + i * 52, 1, COLOR_GRID, EGUI_ALPHA_60);
        egui_canvas_draw_line(canvas, 112 + i * 62, 112, 112 + i * 62, 396, 1, COLOR_GRID, EGUI_ALPHA_60);
    }
    for (uint8_t i = 1; i < 10U; i++)
    {
        egui_dim_t x1;
        egui_dim_t y1;
        egui_dim_t x2;
        egui_dim_t y2;

        get_route_point((uint8_t)(i - 1U), &x1, &y1);
        get_route_point(i, &x2, &y2);
        egui_canvas_draw_line(canvas, x1, y1, x2, y2, 3, COLOR_BLUE, EGUI_ALPHA_80);
    }

    get_route_point(local->route_index, &marker_x, &marker_y);
    egui_canvas_draw_circle_fill(canvas, marker_x, marker_y, 12, COLOR_RED, EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(canvas, marker_x, marker_y, 5, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
}

static void get_candle_region(uint8_t index, egui_region_t *region)
{
    region->location.x = CONTENT_X + 30 + index * 27;
    region->location.y = CONTENT_Y + 34;
    region->size.width = 18;
    region->size.height = 272;
}

static void step_candles(egui_view_dirty_demo_t *local, egui_view_t *self)
{
    egui_region_t strip;

    local->old_candle_cursor = local->candle_cursor;
    local->candle_cursor = (uint8_t)((local->candle_cursor + 1U) % 14U);
    get_candle_region(local->old_candle_cursor, &strip);
    egui_view_invalidate_region(self, &strip);
    get_candle_region(local->candle_cursor, &strip);
    egui_view_invalidate_region(self, &strip);
}

static void draw_candles_page(egui_view_dirty_demo_t *local, egui_canvas_t *canvas)
{
    static const uint8_t high[14] = {66, 128, 92, 152, 118, 174, 84, 144, 106, 162, 78, 134, 98, 156};
    static const uint8_t body[14] = {32, 54, 42, 68, 48, 74, 36, 58, 44, 66, 34, 52, 40, 62};

    draw_demo_panel(canvas);
    draw_grid(canvas, CONTENT_X + 20, CONTENT_Y + 36, CONTENT_W - 40, 260, 32);

    for (uint8_t i = 0; i < 14U; i++)
    {
        egui_region_t strip;
        egui_dim_t cx;
        egui_dim_t y_top;
        egui_dim_t y_body;
        egui_color_t color;

        get_candle_region(i, &strip);
        if (!dirty_region_is_active(EGUI_VIEW_OF(local), canvas, &strip))
        {
            continue;
        }

        cx = strip.location.x + strip.size.width / 2;
        y_top = CONTENT_Y + 270 - high[i];
        y_body = CONTENT_Y + 270 - body[i];
        color = (i & 1U) ? COLOR_GREEN : COLOR_RED;
        egui_canvas_draw_line(canvas, cx, y_top, cx, CONTENT_Y + 286, 2, color, EGUI_ALPHA_100);
        egui_canvas_draw_rectangle_fill(canvas, strip.location.x + 2, y_body, strip.size.width - 4, body[i] / 2, color, EGUI_ALPHA_100);
        if (i == local->candle_cursor)
        {
            egui_canvas_draw_line(canvas, cx, CONTENT_Y + 38, cx, CONTENT_Y + 300, 2, COLOR_AMBER, EGUI_ALPHA_100);
        }
    }
}

static const dirty_draw_fn_t s_drawers[EGUI_VIEW_DIRTY_DEMO_PAGE_COUNT] = {
        draw_line_page,     draw_balls_page, draw_wave_page,     draw_live_chart_page, draw_gauge_page, draw_cells_page,   draw_radar_page,
        draw_spectrum_page, draw_clock_page, draw_progress_page, draw_list_page,       draw_map_page,   draw_candles_page,
};

static const dirty_step_fn_t s_steppers[EGUI_VIEW_DIRTY_DEMO_PAGE_COUNT] = {
        step_line,     step_balls, step_wave,     step_live_chart, step_gauge, step_cells,   step_radar,
        step_spectrum, step_clock, step_progress, step_list,       step_map,   step_candles,
};

static void init_balls(egui_view_dirty_demo_t *local)
{
    static const egui_color_t colors[BALL_COUNT] = {
            COLOR_BLUE, COLOR_GREEN, COLOR_AMBER, COLOR_RED, COLOR_VIOLET, COLOR_TEAL, COLOR_ORANGE, EGUI_COLOR_MAKE(244, 114, 182),
    };
    static const int16_t seed[BALL_COUNT][5] = {
            {92, 138, 3, 2, 13},   {166, 178, -2, 3, 16}, {250, 130, 4, -2, 12}, {340, 202, -3, -3, 15},
            {116, 306, 2, -4, 18}, {224, 330, -4, 2, 13}, {336, 320, 3, 4, 14},  {390, 128, -3, 2, 12},
    };

    for (uint8_t i = 0; i < BALL_COUNT; i++)
    {
        local->balls[i].x_fp = ball_pos_to_fp(seed[i][0]);
        local->balls[i].y_fp = ball_pos_to_fp(seed[i][1]);
        local->balls[i].x = seed[i][0];
        local->balls[i].y = seed[i][1];
        local->balls[i].old_x = local->balls[i].x;
        local->balls[i].old_y = local->balls[i].y;
        local->balls[i].vx = (int8_t)seed[i][2];
        local->balls[i].vy = (int8_t)seed[i][3];
        local->balls[i].radius = seed[i][4];
        local->balls[i].color = colors[i];
    }
}

static void init_wave(egui_view_dirty_demo_t *local)
{
    for (uint8_t i = 0; i < WAVE_COUNT; i++)
    {
        local->wave_values[i] = wave_value_for_tick(i);
    }
    local->wave_head = 0;
    local->old_wave_head = 0;
}

static void init_live_chart(egui_view_dirty_demo_t *local)
{
    for (uint8_t i = 0; i < LIVE_COUNT; i++)
    {
        local->live_values[i] = live_value_for_tick(i);
    }
    local->live_head = 0;
    local->old_live_head = 0;
}

static void init_spectrum(egui_view_dirty_demo_t *local)
{
    for (uint8_t i = 0; i < SPECTRUM_COUNT; i++)
    {
        local->spectrum_values[i] = spectrum_value_for(i, i);
    }
    local->spectrum_index = 0;
}

void egui_view_dirty_demo_on_draw(egui_view_t *self)
{
    egui_view_dirty_demo_t *local = (egui_view_dirty_demo_t *)self;
    egui_canvas_t *canvas = egui_view_get_canvas(self);

    if (local->page >= EGUI_VIEW_DIRTY_DEMO_PAGE_COUNT || canvas == NULL)
    {
        return;
    }

    draw_header(local, canvas);
    s_drawers[local->page](local, canvas);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_dirty_demo_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_dirty_demo_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
};

void egui_view_dirty_demo_init(egui_view_t *self, egui_core_t *core, uint8_t page)
{
    egui_view_dirty_demo_t *local = (egui_view_dirty_demo_t *)self;

    egui_view_init(self, core);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_dirty_demo_t);
    local->page = page;
    local->tick = 0;
    local->last_frame_time = 0;
    local->frame_delta_ms = 0;
    local->step_accum_ms = 0;
    local->has_frame_time = 0;
    local->line_angle = 236;
    local->old_line_angle = local->line_angle;
    local->line_delta = 5;
    local->gauge_angle = 220;
    local->old_gauge_angle = local->gauge_angle;
    local->gauge_delta = 4;
    local->selected_cell = 0;
    local->old_selected_cell = 0;
    local->cursor_on = 1;
    local->radar_angle = 24;
    local->old_radar_angle = local->radar_angle;
    local->clock_seconds = 0;
    local->old_clock_seconds = 0;
    local->progress_value = 42;
    local->old_progress_value = local->progress_value;
    local->progress_delta = 3;
    local->selected_row = 0;
    local->old_selected_row = 0;
    local->route_index = 0;
    local->old_route_index = 0;
    local->candle_cursor = 0;
    local->old_candle_cursor = 0;

    init_balls(local);
    init_wave(local);
    init_live_chart(local);
    init_spectrum(local);
}

void egui_view_dirty_demo_step(egui_view_t *self, uint32_t current_time)
{
    egui_view_dirty_demo_t *local = (egui_view_dirty_demo_t *)self;
    uint16_t step_count = 1;

    if (local->page >= EGUI_VIEW_DIRTY_DEMO_PAGE_COUNT)
    {
        return;
    }

    if (!local->has_frame_time)
    {
        local->has_frame_time = 1;
        local->last_frame_time = current_time;
        local->frame_delta_ms = 1000U / EGUI_CONFIG_MAX_FPS;
    }
    else
    {
        uint32_t elapsed = current_time - local->last_frame_time;

        if (elapsed > UINT16_MAX)
        {
            elapsed = UINT16_MAX;
        }
        if (elapsed > DEMO_FRAME_DT_MAX)
        {
            elapsed = DEMO_FRAME_DT_MAX;
        }
        local->last_frame_time = current_time;
        local->frame_delta_ms = (uint16_t)elapsed;
    }

    if (local->page != EGUI_VIEW_DIRTY_DEMO_PAGE_BALLS)
    {
        local->step_accum_ms = (uint16_t)(local->step_accum_ms + local->frame_delta_ms);
        if (local->step_accum_ms < DEMO_LOGIC_STEP_MS)
        {
            return;
        }
        step_count = local->step_accum_ms / DEMO_LOGIC_STEP_MS;
        local->step_accum_ms = (uint16_t)(local->step_accum_ms % DEMO_LOGIC_STEP_MS);
    }

    while (step_count > 0U)
    {
        local->tick++;
        s_steppers[local->page](local, self);
        step_count--;
    }
}

void egui_view_dirty_demo_reset_frame_clock(egui_view_t *self)
{
    egui_view_dirty_demo_t *local = (egui_view_dirty_demo_t *)self;

    local->last_frame_time = 0;
    local->frame_delta_ms = 0;
    local->step_accum_ms = 0;
    local->has_frame_time = 0;
}
