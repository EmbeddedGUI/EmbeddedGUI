#ifndef _HELLO_GAME_VIEW_H_
#define _HELLO_GAME_VIEW_H_

#include "egui.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum hello_game_kind
{
    HELLO_GAME_KIND_SNAKE,
    HELLO_GAME_KIND_BOUNCY_BALL,
    HELLO_GAME_KIND_LINK_MATCH,
    HELLO_GAME_KIND_2048,
    HELLO_GAME_KIND_TETRIS,
    HELLO_GAME_KIND_MINESWEEPER,
    HELLO_GAME_KIND_BRICK_BREAKER,
    HELLO_GAME_KIND_SOKOBAN,
} hello_game_kind_t;

typedef struct hello_game_descriptor
{
    hello_game_kind_t kind;
    const char *title;
    uint8_t level;
    uint16_t tick_ms;
} hello_game_descriptor_t;

typedef struct hello_game_view
{
    egui_view_t base;
    egui_view_api_t api;
    egui_timer_t timer;
    const hello_game_descriptor_t *descriptor;
    uint16_t tick;
    uint16_t score;
    uint8_t input_dir;
    uint8_t pending_dir;
    uint8_t state;
    uint8_t paused;
    uint8_t board_w;
    uint8_t board_h;
    uint8_t cell_px;
    egui_dim_t board_x;
    egui_dim_t board_y;
    uint8_t board[16][16];
    uint8_t prev_board[16][16];
    uint8_t dirty[16][16];
    int16_t x[32];
    int16_t y[32];
    int16_t vx;
    int16_t vy;
    int16_t ax;
    int16_t ay;
    uint8_t count;
    uint8_t aux0;
    uint8_t aux1;
    uint8_t aux2;
    uint8_t aux3;
    uint8_t dragging;
    int16_t down_x;
    int16_t down_y;
} hello_game_view_t;

void hello_game_view_init(hello_game_view_t *self, egui_core_t *core, const hello_game_descriptor_t *descriptor);
void hello_game_view_record_step(hello_game_view_t *self, uint8_t step);
hello_game_view_t *hello_game_get_view(void);

#ifdef __cplusplus
}
#endif

#endif /* _HELLO_GAME_VIEW_H_ */
