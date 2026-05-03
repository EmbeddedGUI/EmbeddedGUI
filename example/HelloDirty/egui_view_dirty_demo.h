#ifndef _EGUI_VIEW_DIRTY_DEMO_H_
#define _EGUI_VIEW_DIRTY_DEMO_H_

#include "widget/egui_view.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define EGUI_VIEW_DIRTY_DEMO_PAGE_COUNT     13
#define EGUI_VIEW_DIRTY_DEMO_BALL_COUNT     8
#define EGUI_VIEW_DIRTY_DEMO_WAVE_COUNT     96
#define EGUI_VIEW_DIRTY_DEMO_LIVE_COUNT     96
#define EGUI_VIEW_DIRTY_DEMO_SPECTRUM_COUNT 16

typedef enum egui_view_dirty_demo_page
{
    EGUI_VIEW_DIRTY_DEMO_PAGE_LINE = 0,
    EGUI_VIEW_DIRTY_DEMO_PAGE_BALLS,
    EGUI_VIEW_DIRTY_DEMO_PAGE_WAVE,
    EGUI_VIEW_DIRTY_DEMO_PAGE_LIVE_CHART,
    EGUI_VIEW_DIRTY_DEMO_PAGE_GAUGE,
    EGUI_VIEW_DIRTY_DEMO_PAGE_CELLS,
    EGUI_VIEW_DIRTY_DEMO_PAGE_RADAR,
    EGUI_VIEW_DIRTY_DEMO_PAGE_SPECTRUM,
    EGUI_VIEW_DIRTY_DEMO_PAGE_CLOCK,
    EGUI_VIEW_DIRTY_DEMO_PAGE_PROGRESS,
    EGUI_VIEW_DIRTY_DEMO_PAGE_LIST,
    EGUI_VIEW_DIRTY_DEMO_PAGE_MAP,
    EGUI_VIEW_DIRTY_DEMO_PAGE_CANDLES,
} egui_view_dirty_demo_page_t;

typedef struct egui_view_dirty_demo_ball
{
    int32_t x_fp;
    int32_t y_fp;
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t old_x;
    egui_dim_t old_y;
    int8_t vx;
    int8_t vy;
    egui_dim_t radius;
    egui_color_t color;
} egui_view_dirty_demo_ball_t;

typedef struct egui_view_dirty_demo egui_view_dirty_demo_t;
struct egui_view_dirty_demo
{
    egui_view_t base;
    uint8_t page;
    uint16_t tick;
    uint32_t last_frame_time;
    uint16_t frame_delta_ms;
    uint16_t step_accum_ms;
    uint8_t has_frame_time;

    int16_t line_angle;
    int16_t old_line_angle;
    int8_t line_delta;

    egui_view_dirty_demo_ball_t balls[EGUI_VIEW_DIRTY_DEMO_BALL_COUNT];

    uint8_t wave_values[EGUI_VIEW_DIRTY_DEMO_WAVE_COUNT];
    uint8_t wave_head;
    uint8_t old_wave_head;

    uint8_t live_values[EGUI_VIEW_DIRTY_DEMO_LIVE_COUNT];
    uint8_t live_head;
    uint8_t old_live_head;

    int16_t gauge_angle;
    int16_t old_gauge_angle;
    int8_t gauge_delta;

    uint8_t selected_cell;
    uint8_t old_selected_cell;
    uint8_t cursor_on;

    int16_t radar_angle;
    int16_t old_radar_angle;

    uint8_t spectrum_values[EGUI_VIEW_DIRTY_DEMO_SPECTRUM_COUNT];
    uint8_t spectrum_index;

    uint8_t clock_seconds;
    uint8_t old_clock_seconds;

    uint8_t progress_value;
    uint8_t old_progress_value;
    int8_t progress_delta;

    uint8_t selected_row;
    uint8_t old_selected_row;

    uint8_t route_index;
    uint8_t old_route_index;

    uint8_t candle_cursor;
    uint8_t old_candle_cursor;
};

void egui_view_dirty_demo_init(egui_view_t *self, egui_core_t *core, uint8_t page);
void egui_view_dirty_demo_step(egui_view_t *self, uint32_t current_time);
void egui_view_dirty_demo_reset_frame_clock(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_DIRTY_DEMO_H_ */
