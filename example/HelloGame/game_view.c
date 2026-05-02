#include "game_view.h"

#include <stdio.h>
#include <string.h>

#define HG_MAX_BOARD       16
#define HG_MAX_OBJECTS     32
#define HG_HUD_HEIGHT      68
#define HG_STATE_RUNNING   0
#define HG_STATE_WIN       1
#define HG_STATE_OVER      2
#define HG_DIR_UP          0
#define HG_DIR_RIGHT       1
#define HG_DIR_DOWN        2
#define HG_DIR_LEFT        3
#define HG_DIR_NONE        4
#define HG_SNAKE_EMPTY     0
#define HG_SNAKE_BODY      1
#define HG_SNAKE_FOOD      2
#define HG_SNAKE_WALL      3
#define HG_MINE_VALUE_MASK 0x0F
#define HG_MINE_FLAG       0x20
#define HG_MINE_REVEALED   0x40
#define HG_MINE_BOMB       0x80
#define HG_SOKO_WALL       0x01
#define HG_SOKO_TARGET     0x02
#define HG_SOKO_BOX        0x04
#define HG_SOKO_PLAYER     0x08

static void hello_game_reset(hello_game_view_t *local);

static egui_color_t hello_game_palette(uint8_t index)
{
    static const egui_color_t colors[] = {
            EGUI_COLOR_MAKE(80, 160, 255),  EGUI_COLOR_MAKE(48, 204, 120), EGUI_COLOR_MAKE(255, 188, 66),  EGUI_COLOR_MAKE(246, 92, 92),
            EGUI_COLOR_MAKE(190, 125, 255), EGUI_COLOR_MAKE(78, 210, 220), EGUI_COLOR_MAKE(240, 124, 200), EGUI_COLOR_MAKE(170, 220, 80),
    };

    return colors[index % (uint8_t)EGUI_ARRAY_SIZE(colors)];
}

static void hello_game_region_init(egui_region_t *region, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_init(region, x, y, width, height);
}

static void hello_game_invalidate_rect(hello_game_view_t *local, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_region_t region;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    if (x < 0)
    {
        width += x;
        x = 0;
    }
    if (y < 0)
    {
        height += y;
        y = 0;
    }
    if (x + width > EGUI_CONFIG_SCREEN_WIDTH)
    {
        width = EGUI_CONFIG_SCREEN_WIDTH - x;
    }
    if (y + height > EGUI_CONFIG_SCREEN_HEIGHT)
    {
        height = EGUI_CONFIG_SCREEN_HEIGHT - y;
    }
    if (width <= 0 || height <= 0)
    {
        return;
    }

    hello_game_region_init(&region, x, y, width, height);
    egui_view_invalidate_region(EGUI_VIEW_OF(local), &region);
}

static void hello_game_invalidate_hud(hello_game_view_t *local)
{
    hello_game_invalidate_rect(local, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, HG_HUD_HEIGHT);
}

static void hello_game_get_board_region(hello_game_view_t *local, egui_region_t *region)
{
    hello_game_region_init(region, local->board_x, local->board_y, (egui_dim_t)(local->board_w * local->cell_px),
                           (egui_dim_t)(local->board_h * local->cell_px));
}

static void hello_game_invalidate_board(hello_game_view_t *local)
{
    egui_region_t region;

    hello_game_get_board_region(local, &region);
    hello_game_invalidate_rect(local, region.location.x - 2, region.location.y - 2, region.size.width + 4, region.size.height + 4);
}

static void hello_game_mark_cell_dirty(hello_game_view_t *local, int16_t x, int16_t y)
{
    if (x < 0 || y < 0 || x >= local->board_w || y >= local->board_h)
    {
        return;
    }

    local->dirty[y][x] = 1;
}

static void hello_game_mark_all_cells_dirty(hello_game_view_t *local)
{
    for (uint8_t row = 0; row < local->board_h; row++)
    {
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            local->dirty[row][col] = 1;
        }
    }
}

static void hello_game_snapshot_board(hello_game_view_t *local)
{
    memcpy(local->prev_board, local->board, sizeof(local->prev_board));
}

static void hello_game_mark_changed_cells(hello_game_view_t *local)
{
    for (uint8_t row = 0; row < local->board_h; row++)
    {
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            if (local->prev_board[row][col] != local->board[row][col])
            {
                local->dirty[row][col] = 1;
            }
        }
    }
}

static void hello_game_flush_dirty_cells(hello_game_view_t *local)
{
    uint8_t dirty_count = 0;

    for (uint8_t row = 0; row < local->board_h; row++)
    {
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            if (local->dirty[row][col])
            {
                dirty_count++;
            }
        }
    }

    if (dirty_count == 0)
    {
        return;
    }

    if (dirty_count > 12)
    {
        hello_game_invalidate_board(local);
    }
    else
    {
        for (uint8_t row = 0; row < local->board_h; row++)
        {
            for (uint8_t col = 0; col < local->board_w; col++)
            {
                if (local->dirty[row][col])
                {
                    hello_game_invalidate_rect(local, (egui_dim_t)(local->board_x + col * local->cell_px), (egui_dim_t)(local->board_y + row * local->cell_px),
                                               local->cell_px, local->cell_px);
                }
            }
        }
    }

    memset(local->dirty, 0, sizeof(local->dirty));
    hello_game_snapshot_board(local);
}

static void hello_game_setup_board(hello_game_view_t *local, uint8_t board_w, uint8_t board_h, uint8_t preferred_cell)
{
    uint8_t cell_from_width = (uint8_t)((EGUI_CONFIG_SCREEN_WIDTH - 24) / board_w);
    uint8_t cell_from_height = (uint8_t)((EGUI_CONFIG_SCREEN_HEIGHT - HG_HUD_HEIGHT - 12) / board_h);
    uint8_t cell_px = preferred_cell;

    cell_px = EGUI_MIN(cell_px, cell_from_width);
    cell_px = EGUI_MIN(cell_px, cell_from_height);
    if (cell_px < 8)
    {
        cell_px = 8;
    }

    local->board_w = board_w;
    local->board_h = board_h;
    local->cell_px = cell_px;
    local->board_x = (egui_dim_t)((EGUI_CONFIG_SCREEN_WIDTH - board_w * cell_px) / 2);
    local->board_y = (egui_dim_t)HG_HUD_HEIGHT;
    if (local->board_y + board_h * cell_px > EGUI_CONFIG_SCREEN_HEIGHT - 6)
    {
        local->board_y = (egui_dim_t)(EGUI_CONFIG_SCREEN_HEIGHT - 6 - board_h * cell_px);
    }
}

static void hello_game_draw_text(egui_canvas_t *canvas, const char *text, egui_dim_t x, egui_dim_t y, egui_color_t color)
{
    egui_canvas_draw_text(canvas, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, text, x, y, color, EGUI_ALPHA_100);
}

static void hello_game_draw_text_center(egui_canvas_t *canvas, const char *text, egui_region_t *region, egui_color_t color)
{
    egui_canvas_draw_text_in_rect(canvas, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT, text, region, EGUI_ALIGN_CENTER, color, EGUI_ALPHA_100);
}

static void hello_game_draw_hud(hello_game_view_t *local, egui_canvas_t *canvas)
{
    char score_text[32];
    const char *state_text = "RUN";
    egui_region_t state_region;
    egui_region_t level_region;
    egui_region_t title_active_region;

    if (local->state == HG_STATE_WIN)
    {
        state_text = "WIN";
    }
    else if (local->state == HG_STATE_OVER)
    {
        state_text = "OVER";
    }

    hello_game_region_init(&title_active_region, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, HG_HUD_HEIGHT);
    if (!egui_canvas_is_region_active(canvas, &title_active_region))
    {
        return;
    }

    hello_game_draw_text(canvas, local->descriptor->title, 8, 8, EGUI_COLOR_WHITE);
    snprintf(score_text, sizeof(score_text), "Score %u", local->score);
    hello_game_draw_text(canvas, score_text, 8, 30, EGUI_COLOR_MAKE(190, 210, 230));
    snprintf(score_text, sizeof(score_text), "L%u", local->descriptor->level);
    hello_game_region_init(&level_region, EGUI_CONFIG_SCREEN_WIDTH - 84, 8, 32, 20);
    hello_game_draw_text_center(canvas, score_text, &level_region, EGUI_COLOR_MAKE(190, 210, 230));
    hello_game_region_init(&state_region, EGUI_CONFIG_SCREEN_WIDTH - 52, 8, 44, 22);
    egui_canvas_draw_round_rectangle_fill(canvas, state_region.location.x, state_region.location.y, state_region.size.width, state_region.size.height, 5,
                                          local->state == HG_STATE_RUNNING ? EGUI_COLOR_MAKE(42, 90, 120) : EGUI_COLOR_MAKE(150, 70, 80), EGUI_ALPHA_100);
    hello_game_draw_text_center(canvas, state_text, &state_region, EGUI_COLOR_WHITE);
}

static void hello_game_draw_base_cell(hello_game_view_t *local, egui_canvas_t *canvas, uint8_t col, uint8_t row, egui_color_t color)
{
    egui_dim_t cell_px = local->cell_px;
    egui_dim_t x = (egui_dim_t)(local->board_x + col * cell_px);
    egui_dim_t y = (egui_dim_t)(local->board_y + row * cell_px);
    egui_region_t screen_region;

    hello_game_region_init(&screen_region, x + EGUI_VIEW_OF(local)->region_screen.location.x, y + EGUI_VIEW_OF(local)->region_screen.location.y, cell_px,
                           cell_px);
    if (!egui_canvas_is_region_active(canvas, &screen_region))
    {
        return;
    }

    egui_canvas_draw_rectangle_fill(canvas, x + 1, y + 1, cell_px - 2, cell_px - 2, color, EGUI_ALPHA_100);
}

static void hello_game_draw_link_cell(hello_game_view_t *local, egui_canvas_t *canvas, uint8_t col, uint8_t row, uint8_t value)
{
    char label[4];
    egui_region_t text_region;
    egui_color_t color = value == 0 ? EGUI_COLOR_MAKE(26, 31, 40) : hello_game_palette(value - 1);

    hello_game_draw_base_cell(local, canvas, col, row, color);
    if (value != 0)
    {
        snprintf(label, sizeof(label), "%u", value);
        hello_game_region_init(&text_region, (egui_dim_t)(local->board_x + col * local->cell_px), (egui_dim_t)(local->board_y + row * local->cell_px),
                               local->cell_px, local->cell_px);
        hello_game_draw_text_center(canvas, label, &text_region, EGUI_COLOR_BLACK);
    }
    if ((local->aux0 && local->x[0] == col && local->y[0] == row) || (local->aux1 == col && local->aux2 == row))
    {
        egui_canvas_draw_rectangle(canvas, (egui_dim_t)(local->board_x + col * local->cell_px), (egui_dim_t)(local->board_y + row * local->cell_px),
                                   local->cell_px, local->cell_px, 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    }
}

static void hello_game_draw_2048_cell(hello_game_view_t *local, egui_canvas_t *canvas, uint8_t col, uint8_t row, uint8_t value)
{
    char label[8];
    egui_region_t text_region;
    egui_color_t color;
    uint16_t number = 0;

    if (value == 0)
    {
        color = EGUI_COLOR_MAKE(42, 48, 58);
    }
    else
    {
        color = hello_game_palette(value);
        number = (uint16_t)(1U << value);
    }

    hello_game_draw_base_cell(local, canvas, col, row, color);
    if (value != 0)
    {
        snprintf(label, sizeof(label), "%u", number);
        hello_game_region_init(&text_region, (egui_dim_t)(local->board_x + col * local->cell_px), (egui_dim_t)(local->board_y + row * local->cell_px),
                               local->cell_px, local->cell_px);
        hello_game_draw_text_center(canvas, label, &text_region, EGUI_COLOR_BLACK);
    }
}

static void hello_game_draw_mine_cell(hello_game_view_t *local, egui_canvas_t *canvas, uint8_t col, uint8_t row, uint8_t value)
{
    char label[4];
    egui_region_t text_region;
    uint8_t revealed = value & HG_MINE_REVEALED;
    egui_color_t color = revealed ? EGUI_COLOR_MAKE(178, 190, 205) : EGUI_COLOR_MAKE(55, 70, 86);

    if ((value & HG_MINE_FLAG) != 0)
    {
        color = EGUI_COLOR_MAKE(240, 180, 70);
    }
    if (revealed && (value & HG_MINE_BOMB) != 0)
    {
        color = EGUI_COLOR_MAKE(220, 70, 80);
    }

    hello_game_draw_base_cell(local, canvas, col, row, color);
    hello_game_region_init(&text_region, (egui_dim_t)(local->board_x + col * local->cell_px), (egui_dim_t)(local->board_y + row * local->cell_px),
                           local->cell_px, local->cell_px);

    if ((value & HG_MINE_FLAG) != 0)
    {
        hello_game_draw_text_center(canvas, "F", &text_region, EGUI_COLOR_BLACK);
    }
    else if (revealed && (value & HG_MINE_BOMB) != 0)
    {
        hello_game_draw_text_center(canvas, "*", &text_region, EGUI_COLOR_WHITE);
    }
    else if (revealed && (value & HG_MINE_VALUE_MASK) != 0)
    {
        snprintf(label, sizeof(label), "%u", value & HG_MINE_VALUE_MASK);
        hello_game_draw_text_center(canvas, label, &text_region, EGUI_COLOR_BLACK);
    }

    if (local->aux1 == col && local->aux2 == row)
    {
        egui_canvas_draw_rectangle(canvas, (egui_dim_t)(local->board_x + col * local->cell_px), (egui_dim_t)(local->board_y + row * local->cell_px),
                                   local->cell_px, local->cell_px, 2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    }
}

static void hello_game_draw_sokoban_cell(hello_game_view_t *local, egui_canvas_t *canvas, uint8_t col, uint8_t row, uint8_t value)
{
    egui_dim_t cell_px = local->cell_px;
    egui_dim_t x = (egui_dim_t)(local->board_x + col * cell_px);
    egui_dim_t y = (egui_dim_t)(local->board_y + row * cell_px);

    if ((value & HG_SOKO_WALL) != 0)
    {
        hello_game_draw_base_cell(local, canvas, col, row, EGUI_COLOR_MAKE(80, 92, 110));
        return;
    }

    hello_game_draw_base_cell(local, canvas, col, row, EGUI_COLOR_MAKE(28, 34, 44));
    if ((value & HG_SOKO_TARGET) != 0)
    {
        egui_canvas_draw_circle_fill(canvas, x + cell_px / 2, y + cell_px / 2, cell_px / 5, EGUI_COLOR_MAKE(80, 180, 120), EGUI_ALPHA_100);
    }
    if ((value & HG_SOKO_BOX) != 0)
    {
        egui_canvas_draw_rectangle_fill(canvas, x + 4, y + 4, cell_px - 8, cell_px - 8, EGUI_COLOR_MAKE(210, 150, 70), EGUI_ALPHA_100);
        egui_canvas_draw_rectangle(canvas, x + 4, y + 4, cell_px - 8, cell_px - 8, 1, EGUI_COLOR_MAKE(95, 60, 30), EGUI_ALPHA_100);
    }
    if ((value & HG_SOKO_PLAYER) != 0)
    {
        egui_canvas_draw_circle_fill(canvas, x + cell_px / 2, y + cell_px / 2, cell_px / 3, EGUI_COLOR_MAKE(80, 160, 255), EGUI_ALPHA_100);
    }
}

static void hello_game_draw_grid_cell(hello_game_view_t *local, egui_canvas_t *canvas, uint8_t col, uint8_t row)
{
    uint8_t value = local->board[row][col];

    switch (local->descriptor->kind)
    {
    case HELLO_GAME_KIND_SNAKE:
        if (value == HG_SNAKE_WALL)
        {
            hello_game_draw_base_cell(local, canvas, col, row, EGUI_COLOR_MAKE(72, 82, 96));
        }
        else if (value == HG_SNAKE_BODY)
        {
            hello_game_draw_base_cell(local, canvas, col, row, EGUI_COLOR_MAKE(54, 210, 116));
        }
        else if (value == HG_SNAKE_FOOD)
        {
            hello_game_draw_base_cell(local, canvas, col, row, EGUI_COLOR_MAKE(240, 85, 92));
        }
        else
        {
            hello_game_draw_base_cell(local, canvas, col, row, EGUI_COLOR_MAKE(22, 30, 36));
        }
        break;
    case HELLO_GAME_KIND_LINK_MATCH:
        hello_game_draw_link_cell(local, canvas, col, row, value);
        break;
    case HELLO_GAME_KIND_2048:
        hello_game_draw_2048_cell(local, canvas, col, row, value);
        break;
    case HELLO_GAME_KIND_TETRIS:
        hello_game_draw_base_cell(local, canvas, col, row, value == 0 ? EGUI_COLOR_MAKE(20, 26, 34) : hello_game_palette(value - 1));
        break;
    case HELLO_GAME_KIND_MINESWEEPER:
        hello_game_draw_mine_cell(local, canvas, col, row, value);
        break;
    case HELLO_GAME_KIND_SOKOBAN:
        hello_game_draw_sokoban_cell(local, canvas, col, row, value);
        break;
    case HELLO_GAME_KIND_BOUNCY_BALL:
    case HELLO_GAME_KIND_BRICK_BREAKER:
        if (value != 0)
        {
            hello_game_draw_base_cell(local, canvas, col, row, hello_game_palette(value - 1));
        }
        break;
    default:
        break;
    }
}

static void hello_game_draw_grid(hello_game_view_t *local, egui_canvas_t *canvas)
{
    egui_region_t board_region;
    egui_region_t screen_region;

    hello_game_get_board_region(local, &board_region);
    hello_game_region_init(&screen_region, board_region.location.x + EGUI_VIEW_OF(local)->region_screen.location.x,
                           board_region.location.y + EGUI_VIEW_OF(local)->region_screen.location.y, board_region.size.width, board_region.size.height);
    if (!egui_canvas_is_region_active(canvas, &screen_region))
    {
        return;
    }

    egui_canvas_draw_rectangle_fill(canvas, board_region.location.x - 2, board_region.location.y - 2, board_region.size.width + 4, board_region.size.height + 4,
                                    EGUI_COLOR_MAKE(14, 18, 24), EGUI_ALPHA_100);
    for (uint8_t row = 0; row < local->board_h; row++)
    {
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            hello_game_draw_grid_cell(local, canvas, col, row);
        }
    }
}

static const int8_t tetris_shapes[7][4][4][2] = {
        {{{0, 1}, {1, 1}, {2, 1}, {3, 1}}, {{2, 0}, {2, 1}, {2, 2}, {2, 3}}, {{0, 2}, {1, 2}, {2, 2}, {3, 2}}, {{1, 0}, {1, 1}, {1, 2}, {1, 3}}},
        {{{0, 0}, {0, 1}, {1, 1}, {2, 1}}, {{1, 0}, {2, 0}, {1, 1}, {1, 2}}, {{0, 1}, {1, 1}, {2, 1}, {2, 2}}, {{1, 0}, {1, 1}, {0, 2}, {1, 2}}},
        {{{2, 0}, {0, 1}, {1, 1}, {2, 1}}, {{1, 0}, {1, 1}, {1, 2}, {2, 2}}, {{0, 1}, {1, 1}, {2, 1}, {0, 2}}, {{0, 0}, {1, 0}, {1, 1}, {1, 2}}},
        {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}, {{1, 0}, {2, 0}, {1, 1}, {2, 1}}, {{1, 0}, {2, 0}, {1, 1}, {2, 1}}, {{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
        {{{1, 0}, {2, 0}, {0, 1}, {1, 1}}, {{1, 0}, {1, 1}, {2, 1}, {2, 2}}, {{1, 1}, {2, 1}, {0, 2}, {1, 2}}, {{0, 0}, {0, 1}, {1, 1}, {1, 2}}},
        {{{1, 0}, {0, 1}, {1, 1}, {2, 1}}, {{1, 0}, {1, 1}, {2, 1}, {1, 2}}, {{0, 1}, {1, 1}, {2, 1}, {1, 2}}, {{1, 0}, {0, 1}, {1, 1}, {1, 2}}},
        {{{0, 0}, {1, 0}, {1, 1}, {2, 1}}, {{2, 0}, {1, 1}, {2, 1}, {1, 2}}, {{0, 1}, {1, 1}, {1, 2}, {2, 2}}, {{1, 0}, {0, 1}, {1, 1}, {0, 2}}},
};

static void hello_game_tetris_mark_piece(hello_game_view_t *local, uint8_t piece, uint8_t rotation, int16_t px, int16_t py)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        int16_t cell_x = px + tetris_shapes[piece][rotation][i][0];
        int16_t cell_y = py + tetris_shapes[piece][rotation][i][1];
        hello_game_mark_cell_dirty(local, cell_x, cell_y);
    }
}

static void hello_game_draw_tetris_piece(hello_game_view_t *local, egui_canvas_t *canvas)
{
    uint8_t piece = local->aux0;
    uint8_t rotation = local->aux1;

    for (uint8_t i = 0; i < 4; i++)
    {
        int16_t cell_x = local->x[0] + tetris_shapes[piece][rotation][i][0];
        int16_t cell_y = local->y[0] + tetris_shapes[piece][rotation][i][1];
        if (cell_x >= 0 && cell_y >= 0 && cell_x < local->board_w && cell_y < local->board_h)
        {
            egui_dim_t rect_x = (egui_dim_t)(local->board_x + cell_x * local->cell_px);
            egui_dim_t rect_y = (egui_dim_t)(local->board_y + cell_y * local->cell_px);
            egui_region_t screen_region;
            hello_game_region_init(&screen_region, rect_x, rect_y, local->cell_px, local->cell_px);
            if (egui_canvas_is_region_active(canvas, &screen_region))
            {
                egui_canvas_draw_rectangle_fill(canvas, rect_x + 1, rect_y + 1, local->cell_px - 2, local->cell_px - 2, hello_game_palette(piece),
                                                EGUI_ALPHA_100);
            }
        }
    }
}

static void hello_game_get_arena(hello_game_view_t *local, egui_region_t *arena)
{
    EGUI_UNUSED(local);
    hello_game_region_init(arena, 16, 76, EGUI_CONFIG_SCREEN_WIDTH - 32, 198);
}

static void hello_game_draw_physics(hello_game_view_t *local, egui_canvas_t *canvas)
{
    egui_region_t arena;
    egui_region_t screen_arena;
    egui_dim_t paddle_y;
    egui_dim_t paddle_h = 8;
    egui_dim_t radius = local->aux2;

    hello_game_get_arena(local, &arena);
    hello_game_region_init(&screen_arena, arena.location.x, arena.location.y, arena.size.width, arena.size.height);
    if (!egui_canvas_is_region_active(canvas, &screen_arena))
    {
        return;
    }

    egui_canvas_draw_rectangle_fill(canvas, arena.location.x, arena.location.y, arena.size.width, arena.size.height, EGUI_COLOR_MAKE(15, 20, 28),
                                    EGUI_ALPHA_100);
    egui_canvas_draw_rectangle(canvas, arena.location.x, arena.location.y, arena.size.width, arena.size.height, 1, EGUI_COLOR_MAKE(75, 90, 110),
                               EGUI_ALPHA_100);
    hello_game_draw_grid(local, canvas);

    paddle_y = (egui_dim_t)(arena.location.y + arena.size.height - 18);
    egui_canvas_draw_round_rectangle_fill(canvas, local->aux0, paddle_y, local->aux1, paddle_h, 4, EGUI_COLOR_MAKE(88, 170, 255), EGUI_ALPHA_100);
    egui_canvas_draw_circle_fill(canvas, local->x[0], local->y[0], radius,
                                 local->descriptor->kind == HELLO_GAME_KIND_BOUNCY_BALL ? EGUI_COLOR_MAKE(255, 210, 80) : EGUI_COLOR_MAKE(255, 95, 95),
                                 EGUI_ALPHA_100);
}

static void hello_game_on_draw(egui_view_t *self)
{
    hello_game_view_t *local = (hello_game_view_t *)self;
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_region_t view_region;

    if (canvas == NULL)
    {
        return;
    }

    hello_game_region_init(&view_region, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);
    egui_canvas_draw_rectangle_fill(canvas, view_region.location.x, view_region.location.y, view_region.size.width, view_region.size.height,
                                    EGUI_COLOR_MAKE(18, 23, 31), EGUI_ALPHA_100);
    hello_game_draw_hud(local, canvas);

    if (local->descriptor->kind == HELLO_GAME_KIND_BOUNCY_BALL || local->descriptor->kind == HELLO_GAME_KIND_BRICK_BREAKER)
    {
        hello_game_draw_physics(local, canvas);
    }
    else
    {
        hello_game_draw_grid(local, canvas);
        if (local->descriptor->kind == HELLO_GAME_KIND_TETRIS)
        {
            hello_game_draw_tetris_piece(local, canvas);
        }
    }
}

static void hello_game_snake_place_food(hello_game_view_t *local)
{
    uint16_t start = (uint16_t)((local->tick * 5U + local->score + local->descriptor->level * 3U) % (local->board_w * local->board_h));

    for (uint16_t i = 0; i < (uint16_t)(local->board_w * local->board_h); i++)
    {
        uint16_t index = (uint16_t)((start + i) % (local->board_w * local->board_h));
        uint8_t x = (uint8_t)(index % local->board_w);
        uint8_t y = (uint8_t)(index / local->board_w);
        if (local->board[y][x] == HG_SNAKE_EMPTY)
        {
            local->board[y][x] = HG_SNAKE_FOOD;
            return;
        }
    }
}

static void hello_game_snake_init(hello_game_view_t *local)
{
    hello_game_setup_board(local, 12, 15, 15);
    for (uint8_t row = 0; row < local->board_h; row++)
    {
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            if (row == 0 || col == 0 || row + 1 == local->board_h || col + 1 == local->board_w || (col == 7 && row > 3 && row < 10))
            {
                local->board[row][col] = HG_SNAKE_WALL;
            }
        }
    }

    local->x[0] = 5;
    local->y[0] = 7;
    local->x[1] = 4;
    local->y[1] = 7;
    local->x[2] = 3;
    local->y[2] = 7;
    local->count = 3;
    local->input_dir = HG_DIR_RIGHT;
    local->pending_dir = HG_DIR_RIGHT;
    for (uint8_t i = 0; i < local->count; i++)
    {
        local->board[local->y[i]][local->x[i]] = HG_SNAKE_BODY;
    }
    local->board[7][9] = HG_SNAKE_FOOD;
}

static uint8_t hello_game_is_opposite_dir(uint8_t a, uint8_t b)
{
    return (uint8_t)((a + 2U) & 3U) == b;
}

static void hello_game_snake_tick(hello_game_view_t *local)
{
    static const int8_t dx[] = {0, 1, 0, -1};
    static const int8_t dy[] = {-1, 0, 1, 0};
    int16_t head_x;
    int16_t head_y;
    uint8_t next_cell;
    uint8_t grow;

    if (local->state != HG_STATE_RUNNING)
    {
        return;
    }

    hello_game_snapshot_board(local);
    if (!hello_game_is_opposite_dir(local->input_dir, local->pending_dir))
    {
        local->input_dir = local->pending_dir;
    }

    head_x = local->x[0] + dx[local->input_dir];
    head_y = local->y[0] + dy[local->input_dir];
    next_cell = local->board[head_y][head_x];
    grow = next_cell == HG_SNAKE_FOOD;

    if (next_cell == HG_SNAKE_WALL || next_cell == HG_SNAKE_BODY)
    {
        local->state = HG_STATE_OVER;
        hello_game_invalidate_hud(local);
        return;
    }

    if (!grow)
    {
        local->board[local->y[local->count - 1]][local->x[local->count - 1]] = HG_SNAKE_EMPTY;
    }
    else if (local->count < HG_MAX_OBJECTS)
    {
        local->count++;
        local->score = (uint16_t)(local->score + 10U);
        hello_game_invalidate_hud(local);
    }

    for (uint8_t i = local->count - 1; i > 0; i--)
    {
        local->x[i] = local->x[i - 1];
        local->y[i] = local->y[i - 1];
    }
    local->x[0] = head_x;
    local->y[0] = head_y;
    local->board[head_y][head_x] = HG_SNAKE_BODY;
    if (grow)
    {
        hello_game_snake_place_food(local);
    }

    hello_game_mark_changed_cells(local);
    hello_game_flush_dirty_cells(local);
}

static void hello_game_physics_init(hello_game_view_t *local, uint8_t brick_breaker)
{
    hello_game_setup_board(local, 10, 4, 20);
    local->board_x = 20;
    local->board_y = 92;
    for (uint8_t row = 0; row < local->board_h; row++)
    {
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            local->board[row][col] = (uint8_t)(1 + (row + col) % 6);
        }
    }
    local->x[0] = 70;
    local->y[0] = 214;
    local->vx = brick_breaker ? 4 : 3;
    local->vy = brick_breaker ? -5 : -3;
    local->ay = brick_breaker ? 0 : 1;
    local->aux0 = (uint8_t)((EGUI_CONFIG_SCREEN_WIDTH - 58) / 2);
    local->aux1 = 58;
    local->aux2 = brick_breaker ? 5 : 6;
}

static uint8_t hello_game_physics_any_brick(hello_game_view_t *local)
{
    for (uint8_t row = 0; row < local->board_h; row++)
    {
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            if (local->board[row][col] != 0)
            {
                return 1;
            }
        }
    }
    return 0;
}

static uint8_t hello_game_rect_contains_point(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, int16_t px, int16_t py)
{
    return px >= x && px < x + width && py >= y && py < y + height;
}

static void hello_game_invalidate_ball(hello_game_view_t *local, int16_t old_x, int16_t old_y, int16_t new_x, int16_t new_y)
{
    egui_dim_t radius = local->aux2;
    egui_dim_t x0 = EGUI_MIN(old_x, new_x) - radius - 2;
    egui_dim_t y0 = EGUI_MIN(old_y, new_y) - radius - 2;
    egui_dim_t x1 = EGUI_MAX(old_x, new_x) + radius + 2;
    egui_dim_t y1 = EGUI_MAX(old_y, new_y) + radius + 2;

    hello_game_invalidate_rect(local, x0, y0, (egui_dim_t)(x1 - x0), (egui_dim_t)(y1 - y0));
}

static void hello_game_physics_move_paddle(hello_game_view_t *local, int16_t delta)
{
    egui_region_t arena;
    uint8_t old_x = local->aux0;
    int16_t next_x = local->aux0 + delta;

    hello_game_get_arena(local, &arena);
    if (next_x < arena.location.x + 4)
    {
        next_x = arena.location.x + 4;
    }
    if (next_x + local->aux1 > arena.location.x + arena.size.width - 4)
    {
        next_x = arena.location.x + arena.size.width - 4 - local->aux1;
    }
    local->aux0 = (uint8_t)next_x;
    if (old_x != local->aux0)
    {
        egui_dim_t paddle_y = (egui_dim_t)(arena.location.y + arena.size.height - 18);
        hello_game_invalidate_rect(local, old_x, paddle_y - 2, local->aux1 + (egui_dim_t)EGUI_ABS((int16_t)local->aux0 - (int16_t)old_x), 14);
    }
}

static void hello_game_physics_tick(hello_game_view_t *local)
{
    egui_region_t arena;
    int16_t old_x = local->x[0];
    int16_t old_y = local->y[0];
    egui_dim_t paddle_y;
    uint8_t score_changed = 0;

    if (local->state != HG_STATE_RUNNING)
    {
        return;
    }

    hello_game_get_arena(local, &arena);
    hello_game_snapshot_board(local);
    local->vy += local->ay;
    if (local->vy > 8)
    {
        local->vy = 8;
    }
    local->x[0] += local->vx;
    local->y[0] += local->vy;

    if (local->x[0] - local->aux2 <= arena.location.x)
    {
        local->x[0] = (int16_t)(arena.location.x + local->aux2);
        local->vx = (int16_t)EGUI_ABS(local->vx);
    }
    if (local->x[0] + local->aux2 >= arena.location.x + arena.size.width)
    {
        local->x[0] = (int16_t)(arena.location.x + arena.size.width - local->aux2);
        local->vx = (int16_t)-EGUI_ABS(local->vx);
    }
    if (local->y[0] - local->aux2 <= arena.location.y)
    {
        local->y[0] = (int16_t)(arena.location.y + local->aux2);
        local->vy = (int16_t)EGUI_ABS(local->vy);
    }

    paddle_y = (egui_dim_t)(arena.location.y + arena.size.height - 18);
    if (local->y[0] + local->aux2 >= paddle_y && local->y[0] + local->aux2 <= paddle_y + 10 && local->x[0] >= local->aux0 &&
        local->x[0] <= local->aux0 + local->aux1)
    {
        int16_t offset = local->x[0] - (int16_t)(local->aux0 + local->aux1 / 2);
        local->vy = (int16_t)-(EGUI_ABS(local->vy) + 1);
        local->vx = (int16_t)(offset / 9);
        if (local->vx == 0)
        {
            local->vx = offset < 0 ? -2 : 2;
        }
        local->score++;
        score_changed = 1;
    }
    else if (local->y[0] + local->aux2 >= arena.location.y + arena.size.height)
    {
        local->state = HG_STATE_OVER;
        hello_game_invalidate_hud(local);
    }

    for (uint8_t row = 0; row < local->board_h; row++)
    {
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            if (local->board[row][col] != 0)
            {
                egui_dim_t brick_x = (egui_dim_t)(local->board_x + col * local->cell_px);
                egui_dim_t brick_y = (egui_dim_t)(local->board_y + row * local->cell_px);
                if (hello_game_rect_contains_point(brick_x, brick_y, local->cell_px, local->cell_px, local->x[0], local->y[0]))
                {
                    local->board[row][col] = 0;
                    local->vy = (int16_t)-local->vy;
                    local->score = (uint16_t)(local->score + 5U);
                    score_changed = 1;
                    row = local->board_h;
                    break;
                }
            }
        }
    }
    if (!hello_game_physics_any_brick(local))
    {
        local->state = HG_STATE_WIN;
        hello_game_invalidate_hud(local);
    }
    if (score_changed)
    {
        hello_game_invalidate_hud(local);
    }
    hello_game_mark_changed_cells(local);
    hello_game_flush_dirty_cells(local);
    hello_game_invalidate_ball(local, old_x, old_y, local->x[0], local->y[0]);
}

static uint8_t hello_game_link_cell_empty(hello_game_view_t *local, int16_t x, int16_t y)
{
    if (x < 0 || y < 0 || x >= local->board_w || y >= local->board_h)
    {
        return 1;
    }
    return local->board[y][x] == 0;
}

static uint8_t hello_game_link_clear_line(hello_game_view_t *local, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int16_t dx = x1 > x0 ? 1 : (x1 < x0 ? -1 : 0);
    int16_t dy = y1 > y0 ? 1 : (y1 < y0 ? -1 : 0);
    int16_t x = (int16_t)(x0 + dx);
    int16_t y = (int16_t)(y0 + dy);

    while (x != x1 || y != y1)
    {
        if (!hello_game_link_cell_empty(local, x, y))
        {
            return 0;
        }
        x = (int16_t)(x + dx);
        y = (int16_t)(y + dy);
    }
    return 1;
}

static uint8_t hello_game_link_can_connect(hello_game_view_t *local, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    if (x0 == x1 && hello_game_link_clear_line(local, x0, y0, x1, y1))
    {
        return 1;
    }
    if (y0 == y1 && hello_game_link_clear_line(local, x0, y0, x1, y1))
    {
        return 1;
    }
    if (hello_game_link_cell_empty(local, x0, y1) && hello_game_link_clear_line(local, x0, y0, x0, y1) && hello_game_link_clear_line(local, x0, y1, x1, y1))
    {
        return 1;
    }
    if (hello_game_link_cell_empty(local, x1, y0) && hello_game_link_clear_line(local, x0, y0, x1, y0) && hello_game_link_clear_line(local, x1, y0, x1, y1))
    {
        return 1;
    }
    for (int16_t x = -1; x <= local->board_w; x++)
    {
        if (hello_game_link_cell_empty(local, x, y0) && hello_game_link_cell_empty(local, x, y1) && hello_game_link_clear_line(local, x0, y0, x, y0) &&
            hello_game_link_clear_line(local, x, y0, x, y1) && hello_game_link_clear_line(local, x, y1, x1, y1))
        {
            return 1;
        }
    }
    for (int16_t y = -1; y <= local->board_h; y++)
    {
        if (hello_game_link_cell_empty(local, x0, y) && hello_game_link_cell_empty(local, x1, y) && hello_game_link_clear_line(local, x0, y0, x0, y) &&
            hello_game_link_clear_line(local, x0, y, x1, y) && hello_game_link_clear_line(local, x1, y, x1, y1))
        {
            return 1;
        }
    }
    return 0;
}

static void hello_game_link_select(hello_game_view_t *local, uint8_t col, uint8_t row)
{
    if (col >= local->board_w || row >= local->board_h || local->board[row][col] == 0)
    {
        return;
    }

    hello_game_snapshot_board(local);
    if (!local->aux0)
    {
        local->aux0 = 1;
        local->x[0] = col;
        local->y[0] = row;
        hello_game_mark_cell_dirty(local, col, row);
    }
    else if (local->x[0] == col && local->y[0] == row)
    {
        local->aux0 = 0;
        hello_game_mark_cell_dirty(local, col, row);
    }
    else if (local->board[local->y[0]][local->x[0]] == local->board[row][col] && hello_game_link_can_connect(local, local->x[0], local->y[0], col, row))
    {
        local->board[local->y[0]][local->x[0]] = 0;
        local->board[row][col] = 0;
        local->score = (uint16_t)(local->score + 10U);
        local->aux0 = 0;
        hello_game_invalidate_hud(local);
    }
    else
    {
        hello_game_mark_cell_dirty(local, local->x[0], local->y[0]);
        local->x[0] = col;
        local->y[0] = row;
        hello_game_mark_cell_dirty(local, col, row);
    }

    if (local->score >= 320U)
    {
        local->state = HG_STATE_WIN;
        hello_game_invalidate_hud(local);
    }
    hello_game_mark_changed_cells(local);
    hello_game_flush_dirty_cells(local);
}

static void hello_game_link_init(hello_game_view_t *local)
{
    hello_game_setup_board(local, 8, 8, 24);
    for (uint8_t row = 0; row < local->board_h; row++)
    {
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            local->board[row][col] = (uint8_t)(1 + ((row * 4 + col / 2) % 8));
        }
    }
    local->aux1 = 0;
    local->aux2 = 0;
}

static uint8_t hello_game_2048_slide_line(uint8_t line[4], uint16_t *score)
{
    uint8_t compact[4] = {0, 0, 0, 0};
    uint8_t out[4] = {0, 0, 0, 0};
    uint8_t compact_count = 0;
    uint8_t out_count = 0;
    uint8_t changed = 0;

    for (uint8_t i = 0; i < 4; i++)
    {
        if (line[i] != 0)
        {
            compact[compact_count++] = line[i];
        }
    }
    for (uint8_t i = 0; i < compact_count; i++)
    {
        if (i + 1 < compact_count && compact[i] == compact[i + 1])
        {
            out[out_count] = (uint8_t)(compact[i] + 1U);
            *score = (uint16_t)(*score + (1U << out[out_count]));
            out_count++;
            i++;
        }
        else
        {
            out[out_count++] = compact[i];
        }
    }
    for (uint8_t i = 0; i < 4; i++)
    {
        if (line[i] != out[i])
        {
            changed = 1;
            line[i] = out[i];
        }
    }
    return changed;
}

static void hello_game_2048_spawn(hello_game_view_t *local)
{
    uint8_t start = (uint8_t)((local->tick * 3U + local->score) % 16U);
    for (uint8_t i = 0; i < 16; i++)
    {
        uint8_t index = (uint8_t)((start + i) % 16U);
        uint8_t row = (uint8_t)(index / 4U);
        uint8_t col = (uint8_t)(index % 4U);
        if (local->board[row][col] == 0)
        {
            local->board[row][col] = (local->tick % 5U == 0U) ? 2U : 1U;
            return;
        }
    }
}

static void hello_game_2048_move(hello_game_view_t *local, uint8_t dir)
{
    uint8_t changed = 0;

    hello_game_snapshot_board(local);
    for (uint8_t i = 0; i < 4; i++)
    {
        uint8_t line[4];
        for (uint8_t j = 0; j < 4; j++)
        {
            uint8_t row = (dir == HG_DIR_LEFT || dir == HG_DIR_RIGHT) ? i : ((dir == HG_DIR_UP) ? j : (uint8_t)(3 - j));
            uint8_t col = (dir == HG_DIR_UP || dir == HG_DIR_DOWN) ? i : ((dir == HG_DIR_LEFT) ? j : (uint8_t)(3 - j));
            line[j] = local->board[row][col];
        }
        changed |= hello_game_2048_slide_line(line, &local->score);
        for (uint8_t j = 0; j < 4; j++)
        {
            uint8_t row = (dir == HG_DIR_LEFT || dir == HG_DIR_RIGHT) ? i : ((dir == HG_DIR_UP) ? j : (uint8_t)(3 - j));
            uint8_t col = (dir == HG_DIR_UP || dir == HG_DIR_DOWN) ? i : ((dir == HG_DIR_LEFT) ? j : (uint8_t)(3 - j));
            local->board[row][col] = line[j];
        }
    }
    if (changed)
    {
        hello_game_2048_spawn(local);
        hello_game_invalidate_hud(local);
    }
    hello_game_mark_changed_cells(local);
    hello_game_flush_dirty_cells(local);
}

static void hello_game_2048_init(hello_game_view_t *local)
{
    hello_game_setup_board(local, 4, 4, 48);
    local->board[0][0] = 1;
    local->board[0][1] = 1;
    local->board[1][0] = 2;
}

static uint8_t hello_game_tetris_collision(hello_game_view_t *local, uint8_t piece, uint8_t rotation, int16_t px, int16_t py)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        int16_t cell_x = px + tetris_shapes[piece][rotation][i][0];
        int16_t cell_y = py + tetris_shapes[piece][rotation][i][1];
        if (cell_x < 0 || cell_x >= local->board_w || cell_y >= local->board_h)
        {
            return 1;
        }
        if (cell_y >= 0 && local->board[cell_y][cell_x] != 0)
        {
            return 1;
        }
    }
    return 0;
}

static void hello_game_tetris_spawn(hello_game_view_t *local)
{
    local->aux0 = (uint8_t)((local->aux0 + 1U) % 7U);
    local->aux1 = 0;
    local->x[0] = 3;
    local->y[0] = 0;
    if (hello_game_tetris_collision(local, local->aux0, local->aux1, local->x[0], local->y[0]))
    {
        local->state = HG_STATE_OVER;
        hello_game_invalidate_hud(local);
    }
}

static void hello_game_tetris_lock(hello_game_view_t *local)
{
    for (uint8_t i = 0; i < 4; i++)
    {
        int16_t cell_x = local->x[0] + tetris_shapes[local->aux0][local->aux1][i][0];
        int16_t cell_y = local->y[0] + tetris_shapes[local->aux0][local->aux1][i][1];
        if (cell_x >= 0 && cell_y >= 0 && cell_x < local->board_w && cell_y < local->board_h)
        {
            local->board[cell_y][cell_x] = (uint8_t)(local->aux0 + 1U);
        }
    }
}

static void hello_game_tetris_clear_lines(hello_game_view_t *local)
{
    for (int16_t row = (int16_t)local->board_h - 1; row >= 0; row--)
    {
        uint8_t full = 1;
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            if (local->board[row][col] == 0)
            {
                full = 0;
                break;
            }
        }
        if (full)
        {
            for (int16_t move_row = row; move_row > 0; move_row--)
            {
                for (uint8_t col = 0; col < local->board_w; col++)
                {
                    local->board[move_row][col] = local->board[move_row - 1][col];
                }
            }
            for (uint8_t col = 0; col < local->board_w; col++)
            {
                local->board[0][col] = 0;
            }
            local->score = (uint16_t)(local->score + 100U);
            row++;
            hello_game_invalidate_hud(local);
        }
    }
}

static void hello_game_tetris_tick(hello_game_view_t *local)
{
    int16_t old_x = local->x[0];
    int16_t old_y = local->y[0];
    uint8_t old_piece = local->aux0;
    uint8_t old_rotation = local->aux1;

    if (local->state != HG_STATE_RUNNING)
    {
        return;
    }

    hello_game_snapshot_board(local);
    hello_game_tetris_mark_piece(local, old_piece, old_rotation, old_x, old_y);
    if (!hello_game_tetris_collision(local, local->aux0, local->aux1, local->x[0], (int16_t)(local->y[0] + 1)))
    {
        local->y[0]++;
    }
    else
    {
        hello_game_tetris_lock(local);
        hello_game_tetris_clear_lines(local);
        hello_game_tetris_spawn(local);
    }
    hello_game_tetris_mark_piece(local, local->aux0, local->aux1, local->x[0], local->y[0]);
    hello_game_mark_changed_cells(local);
    hello_game_flush_dirty_cells(local);
}

static void hello_game_tetris_input(hello_game_view_t *local, uint8_t dir)
{
    int16_t old_x = local->x[0];
    int16_t old_y = local->y[0];
    uint8_t old_rotation = local->aux1;
    uint8_t new_rotation = local->aux1;
    int16_t new_x = local->x[0];
    int16_t new_y = local->y[0];

    if (local->state != HG_STATE_RUNNING)
    {
        return;
    }

    if (dir == HG_DIR_LEFT)
    {
        new_x--;
    }
    else if (dir == HG_DIR_RIGHT)
    {
        new_x++;
    }
    else if (dir == HG_DIR_UP)
    {
        new_rotation = (uint8_t)((new_rotation + 1U) & 3U);
    }
    else if (dir == HG_DIR_DOWN)
    {
        new_y++;
    }

    hello_game_tetris_mark_piece(local, local->aux0, old_rotation, old_x, old_y);
    if (!hello_game_tetris_collision(local, local->aux0, new_rotation, new_x, new_y))
    {
        local->x[0] = new_x;
        local->y[0] = new_y;
        local->aux1 = new_rotation;
    }
    hello_game_tetris_mark_piece(local, local->aux0, local->aux1, local->x[0], local->y[0]);
    hello_game_flush_dirty_cells(local);
}

static void hello_game_tetris_init(hello_game_view_t *local)
{
    hello_game_setup_board(local, 10, 18, 13);
    local->aux0 = 6;
    hello_game_tetris_spawn(local);
}

static uint8_t hello_game_mine_count(hello_game_view_t *local, int16_t x, int16_t y)
{
    uint8_t count = 0;
    for (int16_t dy = -1; dy <= 1; dy++)
    {
        for (int16_t dx = -1; dx <= 1; dx++)
        {
            int16_t nx = (int16_t)(x + dx);
            int16_t ny = (int16_t)(y + dy);
            if ((dx != 0 || dy != 0) && nx >= 0 && ny >= 0 && nx < local->board_w && ny < local->board_h && (local->board[ny][nx] & HG_MINE_BOMB) != 0)
            {
                count++;
            }
        }
    }
    return count;
}

static void hello_game_mines_init(hello_game_view_t *local)
{
    hello_game_setup_board(local, 10, 10, 20);
    for (uint8_t i = 0; i < 12; i++)
    {
        uint8_t index = (uint8_t)((i * 17U + 7U) % 100U);
        while ((local->board[index / 10][index % 10] & HG_MINE_BOMB) != 0)
        {
            index = (uint8_t)((index + 11U) % 100U);
        }
        local->board[index / 10][index % 10] = HG_MINE_BOMB;
    }
    for (uint8_t row = 0; row < local->board_h; row++)
    {
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            local->board[row][col] |= hello_game_mine_count(local, col, row);
        }
    }
    local->aux1 = 0;
    local->aux2 = 0;
}

static void hello_game_mines_reveal(hello_game_view_t *local, uint8_t col, uint8_t row)
{
    uint8_t value;

    if (col >= local->board_w || row >= local->board_h || local->state != HG_STATE_RUNNING)
    {
        return;
    }
    hello_game_snapshot_board(local);
    value = local->board[row][col];
    if ((value & HG_MINE_REVEALED) != 0)
    {
        return;
    }
    local->board[row][col] |= HG_MINE_REVEALED;
    if ((value & HG_MINE_BOMB) != 0)
    {
        local->state = HG_STATE_OVER;
        hello_game_invalidate_hud(local);
    }
    else
    {
        local->score++;
        if ((value & HG_MINE_VALUE_MASK) == 0)
        {
            for (int16_t dy = -1; dy <= 1; dy++)
            {
                for (int16_t dx = -1; dx <= 1; dx++)
                {
                    int16_t nx = (int16_t)(col + dx);
                    int16_t ny = (int16_t)(row + dy);
                    if (nx >= 0 && ny >= 0 && nx < local->board_w && ny < local->board_h && (local->board[ny][nx] & HG_MINE_BOMB) == 0)
                    {
                        local->board[ny][nx] |= HG_MINE_REVEALED;
                    }
                }
            }
        }
        if (local->score >= 88U)
        {
            local->state = HG_STATE_WIN;
        }
        hello_game_invalidate_hud(local);
    }
    hello_game_mark_changed_cells(local);
    hello_game_flush_dirty_cells(local);
}

static void hello_game_sokoban_init(hello_game_view_t *local)
{
    static const char *level[] = {
            "########", "#  .   #", "# $$   #", "#  @   #", "#  .   #", "#      #", "#      #", "########",
    };

    hello_game_setup_board(local, 8, 8, 24);
    for (uint8_t row = 0; row < local->board_h; row++)
    {
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            char ch = level[row][col];
            if (ch == '#')
            {
                local->board[row][col] = HG_SOKO_WALL;
            }
            else if (ch == '.')
            {
                local->board[row][col] = HG_SOKO_TARGET;
            }
            else if (ch == '$')
            {
                local->board[row][col] = HG_SOKO_BOX;
            }
            else if (ch == '@')
            {
                local->board[row][col] = HG_SOKO_PLAYER;
                local->x[0] = col;
                local->y[0] = row;
            }
        }
    }
}

static uint8_t hello_game_sokoban_is_win(hello_game_view_t *local)
{
    for (uint8_t row = 0; row < local->board_h; row++)
    {
        for (uint8_t col = 0; col < local->board_w; col++)
        {
            if ((local->board[row][col] & HG_SOKO_BOX) != 0 && (local->board[row][col] & HG_SOKO_TARGET) == 0)
            {
                return 0;
            }
        }
    }
    return 1;
}

static void hello_game_sokoban_move(hello_game_view_t *local, uint8_t dir)
{
    static const int8_t dx[] = {0, 1, 0, -1};
    static const int8_t dy[] = {-1, 0, 1, 0};
    int16_t px = local->x[0];
    int16_t py = local->y[0];
    int16_t nx = (int16_t)(px + dx[dir]);
    int16_t ny = (int16_t)(py + dy[dir]);
    int16_t bx = (int16_t)(nx + dx[dir]);
    int16_t by = (int16_t)(ny + dy[dir]);
    uint8_t target;

    if (local->state != HG_STATE_RUNNING)
    {
        return;
    }
    target = local->board[ny][nx];
    if ((target & HG_SOKO_WALL) != 0)
    {
        return;
    }
    if ((target & HG_SOKO_BOX) != 0 && ((local->board[by][bx] & (HG_SOKO_WALL | HG_SOKO_BOX)) != 0))
    {
        return;
    }

    hello_game_snapshot_board(local);
    local->board[py][px] &= (uint8_t)~HG_SOKO_PLAYER;
    if ((target & HG_SOKO_BOX) != 0)
    {
        local->board[ny][nx] &= (uint8_t)~HG_SOKO_BOX;
        local->board[by][bx] |= HG_SOKO_BOX;
    }
    local->board[ny][nx] |= HG_SOKO_PLAYER;
    local->x[0] = nx;
    local->y[0] = ny;
    local->score++;
    if (hello_game_sokoban_is_win(local))
    {
        local->state = HG_STATE_WIN;
    }
    hello_game_invalidate_hud(local);
    hello_game_mark_changed_cells(local);
    hello_game_flush_dirty_cells(local);
}

static void hello_game_apply_direction(hello_game_view_t *local, uint8_t dir)
{
    if (dir > HG_DIR_LEFT)
    {
        return;
    }

    switch (local->descriptor->kind)
    {
    case HELLO_GAME_KIND_SNAKE:
        local->pending_dir = dir;
        break;
    case HELLO_GAME_KIND_BOUNCY_BALL:
    case HELLO_GAME_KIND_BRICK_BREAKER:
        hello_game_physics_move_paddle(local, dir == HG_DIR_LEFT ? -18 : (dir == HG_DIR_RIGHT ? 18 : 0));
        break;
    case HELLO_GAME_KIND_LINK_MATCH:
        hello_game_mark_cell_dirty(local, local->aux1, local->aux2);
        if (dir == HG_DIR_LEFT && local->aux1 > 0)
        {
            local->aux1--;
        }
        else if (dir == HG_DIR_RIGHT && local->aux1 + 1 < local->board_w)
        {
            local->aux1++;
        }
        else if (dir == HG_DIR_UP && local->aux2 > 0)
        {
            local->aux2--;
        }
        else if (dir == HG_DIR_DOWN && local->aux2 + 1 < local->board_h)
        {
            local->aux2++;
        }
        hello_game_mark_cell_dirty(local, local->aux1, local->aux2);
        hello_game_flush_dirty_cells(local);
        break;
    case HELLO_GAME_KIND_2048:
        hello_game_2048_move(local, dir);
        break;
    case HELLO_GAME_KIND_TETRIS:
        hello_game_tetris_input(local, dir);
        break;
    case HELLO_GAME_KIND_MINESWEEPER:
        hello_game_mark_cell_dirty(local, local->aux1, local->aux2);
        if (dir == HG_DIR_LEFT && local->aux1 > 0)
        {
            local->aux1--;
        }
        else if (dir == HG_DIR_RIGHT && local->aux1 + 1 < local->board_w)
        {
            local->aux1++;
        }
        else if (dir == HG_DIR_UP && local->aux2 > 0)
        {
            local->aux2--;
        }
        else if (dir == HG_DIR_DOWN && local->aux2 + 1 < local->board_h)
        {
            local->aux2++;
        }
        hello_game_mark_cell_dirty(local, local->aux1, local->aux2);
        hello_game_flush_dirty_cells(local);
        break;
    case HELLO_GAME_KIND_SOKOBAN:
        hello_game_sokoban_move(local, dir);
        break;
    default:
        break;
    }
}

static void hello_game_step(hello_game_view_t *local)
{
    local->tick++;
    switch (local->descriptor->kind)
    {
    case HELLO_GAME_KIND_SNAKE:
        hello_game_snake_tick(local);
        break;
    case HELLO_GAME_KIND_BOUNCY_BALL:
    case HELLO_GAME_KIND_BRICK_BREAKER:
        hello_game_physics_tick(local);
        break;
    case HELLO_GAME_KIND_TETRIS:
        hello_game_tetris_tick(local);
        break;
    default:
        break;
    }
}

static void hello_game_timer_callback(egui_timer_t *timer)
{
    hello_game_view_t *local = (hello_game_view_t *)timer->user_data;

    hello_game_step(local);
}

static void hello_game_update_timer(egui_view_t *self)
{
    hello_game_view_t *local = (hello_game_view_t *)self;

    if (local->descriptor != NULL && local->descriptor->tick_ms > 0 && self->is_attached_to_window)
    {
        if (!egui_view_check_timer_start(self, &local->timer))
        {
            egui_view_start_timer(self, &local->timer, local->descriptor->tick_ms, local->descriptor->tick_ms);
        }
    }
    else
    {
        egui_view_stop_timer(self, &local->timer);
    }
}

static void hello_game_on_attach_to_window(egui_view_t *self)
{
    egui_view_on_attach_to_window(self);
    hello_game_update_timer(self);
}

static void hello_game_on_detach_from_window(egui_view_t *self)
{
    hello_game_update_timer(self);
    egui_view_on_detach_from_window(self);
}

static void hello_game_click_cell(hello_game_view_t *local, int16_t local_x, int16_t local_y)
{
    int16_t col = (int16_t)((local_x - local->board_x) / local->cell_px);
    int16_t row = (int16_t)((local_y - local->board_y) / local->cell_px);

    if (col < 0 || row < 0 || col >= local->board_w || row >= local->board_h)
    {
        return;
    }

    switch (local->descriptor->kind)
    {
    case HELLO_GAME_KIND_LINK_MATCH:
        hello_game_link_select(local, (uint8_t)col, (uint8_t)row);
        break;
    case HELLO_GAME_KIND_MINESWEEPER:
        hello_game_mines_reveal(local, (uint8_t)col, (uint8_t)row);
        break;
    default:
        break;
    }
}

static void hello_game_set_paddle_from_touch(hello_game_view_t *local, int16_t local_x)
{
    egui_region_t arena;
    uint8_t old_x = local->aux0;
    int16_t next_x;

    hello_game_get_arena(local, &arena);
    next_x = (int16_t)(local_x - local->aux1 / 2);
    if (next_x < arena.location.x + 4)
    {
        next_x = arena.location.x + 4;
    }
    if (next_x + local->aux1 > arena.location.x + arena.size.width - 4)
    {
        next_x = arena.location.x + arena.size.width - 4 - local->aux1;
    }
    local->aux0 = (uint8_t)next_x;
    if (old_x != local->aux0)
    {
        egui_dim_t paddle_y = (egui_dim_t)(arena.location.y + arena.size.height - 18);
        hello_game_invalidate_rect(local, EGUI_MIN(old_x, local->aux0), paddle_y - 2,
                                   (egui_dim_t)(local->aux1 + EGUI_ABS((int16_t)local->aux0 - (int16_t)old_x)), 14);
    }
}

static int hello_game_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    hello_game_view_t *local = (hello_game_view_t *)self;
    int16_t local_x;
    int16_t local_y;

    if (event == NULL)
    {
        return 0;
    }

    local_x = (int16_t)(event->location.x - self->region_screen.location.x);
    local_y = (int16_t)(event->location.y - self->region_screen.location.y);

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        local->dragging = 1;
        local->down_x = local_x;
        local->down_y = local_y;
        return 1;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->dragging && (local->descriptor->kind == HELLO_GAME_KIND_BOUNCY_BALL || local->descriptor->kind == HELLO_GAME_KIND_BRICK_BREAKER))
        {
            hello_game_set_paddle_from_touch(local, local_x);
        }
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        if (local->dragging)
        {
            int16_t dx = (int16_t)(local_x - local->down_x);
            int16_t dy = (int16_t)(local_y - local->down_y);
            local->dragging = 0;
            if (EGUI_ABS(dx) > 16 || EGUI_ABS(dy) > 16)
            {
                if (EGUI_ABS(dx) > EGUI_ABS(dy))
                {
                    hello_game_apply_direction(local, dx > 0 ? HG_DIR_RIGHT : HG_DIR_LEFT);
                }
                else
                {
                    hello_game_apply_direction(local, dy > 0 ? HG_DIR_DOWN : HG_DIR_UP);
                }
            }
            else if (local->descriptor->kind == HELLO_GAME_KIND_BOUNCY_BALL || local->descriptor->kind == HELLO_GAME_KIND_BRICK_BREAKER)
            {
                hello_game_set_paddle_from_touch(local, local_x);
            }
            else
            {
                hello_game_click_cell(local, local_x, local_y);
            }
        }
        return 1;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->dragging = 0;
        return 1;
    default:
        return 0;
    }
}

static void hello_game_clear_state(hello_game_view_t *local)
{
    memset(local->board, 0, sizeof(local->board));
    memset(local->prev_board, 0, sizeof(local->prev_board));
    memset(local->dirty, 0, sizeof(local->dirty));
    memset(local->x, 0, sizeof(local->x));
    memset(local->y, 0, sizeof(local->y));
    local->tick = 0;
    local->score = 0;
    local->input_dir = HG_DIR_NONE;
    local->pending_dir = HG_DIR_NONE;
    local->state = HG_STATE_RUNNING;
    local->vx = 0;
    local->vy = 0;
    local->ax = 0;
    local->ay = 0;
    local->count = 0;
    local->aux0 = 0;
    local->aux1 = 0;
    local->aux2 = 0;
    local->aux3 = 0;
    local->dragging = 0;
}

static void hello_game_reset(hello_game_view_t *local)
{
    hello_game_clear_state(local);

    switch (local->descriptor->kind)
    {
    case HELLO_GAME_KIND_SNAKE:
        hello_game_snake_init(local);
        break;
    case HELLO_GAME_KIND_BOUNCY_BALL:
        hello_game_physics_init(local, 0);
        break;
    case HELLO_GAME_KIND_LINK_MATCH:
        hello_game_link_init(local);
        break;
    case HELLO_GAME_KIND_2048:
        hello_game_2048_init(local);
        break;
    case HELLO_GAME_KIND_TETRIS:
        hello_game_tetris_init(local);
        break;
    case HELLO_GAME_KIND_MINESWEEPER:
        hello_game_mines_init(local);
        break;
    case HELLO_GAME_KIND_BRICK_BREAKER:
        hello_game_physics_init(local, 1);
        break;
    case HELLO_GAME_KIND_SOKOBAN:
        hello_game_sokoban_init(local);
        break;
    default:
        break;
    }
    hello_game_snapshot_board(local);
    hello_game_mark_all_cells_dirty(local);
}

void hello_game_view_record_step(hello_game_view_t *local, uint8_t step)
{
    static const uint8_t dirs[] = {HG_DIR_RIGHT, HG_DIR_DOWN, HG_DIR_LEFT, HG_DIR_UP};

    if (local == NULL)
    {
        return;
    }

    switch (local->descriptor->kind)
    {
    case HELLO_GAME_KIND_LINK_MATCH:
        hello_game_link_select(local, (uint8_t)((step * 2U) % local->board_w), 0);
        hello_game_link_select(local, (uint8_t)((step * 2U + 1U) % local->board_w), 0);
        break;
    case HELLO_GAME_KIND_MINESWEEPER:
        hello_game_mines_reveal(local, (uint8_t)((step * 2U + 1U) % local->board_w), (uint8_t)((step + 1U) % local->board_h));
        break;
    case HELLO_GAME_KIND_SOKOBAN:
        hello_game_apply_direction(local, dirs[step % (uint8_t)EGUI_ARRAY_SIZE(dirs)]);
        break;
    case HELLO_GAME_KIND_2048:
        hello_game_apply_direction(local, dirs[step % (uint8_t)EGUI_ARRAY_SIZE(dirs)]);
        break;
    case HELLO_GAME_KIND_TETRIS:
        hello_game_apply_direction(local, dirs[step % (uint8_t)EGUI_ARRAY_SIZE(dirs)]);
        hello_game_step(local);
        break;
    case HELLO_GAME_KIND_BOUNCY_BALL:
    case HELLO_GAME_KIND_BRICK_BREAKER:
        hello_game_apply_direction(local, step & 1U ? HG_DIR_RIGHT : HG_DIR_LEFT);
        for (uint8_t i = 0; i < 4; i++)
        {
            hello_game_step(local);
        }
        break;
    case HELLO_GAME_KIND_SNAKE:
        hello_game_apply_direction(local, dirs[step % (uint8_t)EGUI_ARRAY_SIZE(dirs)]);
        hello_game_step(local);
        break;
    default:
        break;
    }
}

void hello_game_view_init(hello_game_view_t *local, egui_core_t *core, const hello_game_descriptor_t *descriptor)
{
    egui_view_t *view = EGUI_VIEW_OF(local);

    egui_view_init(view, core);
    egui_view_copy_api(view, &local->api);
    local->api.on_draw = hello_game_on_draw;
    local->api.on_touch_event = hello_game_on_touch_event;
    local->api.on_attach_to_window = hello_game_on_attach_to_window;
    local->api.on_detach_from_window = hello_game_on_detach_from_window;
    local->descriptor = descriptor;
    egui_view_set_size(view, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);
    egui_timer_init_timer(&local->timer, local, hello_game_timer_callback);
    hello_game_reset(local);
    egui_view_invalidate(view);
}
