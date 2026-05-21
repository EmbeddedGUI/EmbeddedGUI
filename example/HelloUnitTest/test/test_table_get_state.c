#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_table_get_state.h"

static egui_view_table_t s_table;

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_table, 0, sizeof(s_table));
    egui_view_table_init(EGUI_VIEW_OF(&s_table), core);
}

static void test_table_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_table_get_row_count(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_table_get_col_count(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_table_get_header_rows(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(20, (int)egui_view_table_get_row_height(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_table_get_show_grid(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY_DARK.full, (int)egui_view_table_get_header_bg_color(EGUI_VIEW_OF(&s_table)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT.full, (int)egui_view_table_get_header_text_color(EGUI_VIEW_OF(&s_table)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TEXT.full, (int)egui_view_table_get_cell_text_color(EGUI_VIEW_OF(&s_table)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TRACK_BG.full, (int)egui_view_table_get_grid_color(EGUI_VIEW_OF(&s_table)).full);
    EGUI_TEST_ASSERT_TRUE(egui_view_table_get_font(EGUI_VIEW_OF(&s_table)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_NULL(egui_view_table_get_cell(EGUI_VIEW_OF(&s_table), 0, 0));
}

static void test_table_get_state_after_setters(void)
{
    egui_color_t header_bg = {.full = 0x1234};
    egui_color_t header_text = {.full = 0x2345};
    egui_color_t cell_text = {.full = 0x3456};
    egui_color_t grid = {.full = 0x4567};
    const char *cell = "hello";

    setup();
    egui_view_table_set_size(EGUI_VIEW_OF(&s_table), 4, 3);
    egui_view_table_set_cell(EGUI_VIEW_OF(&s_table), 2, 1, cell);
    egui_view_table_set_header_rows(EGUI_VIEW_OF(&s_table), 2);
    egui_view_table_set_row_height(EGUI_VIEW_OF(&s_table), 24);
    egui_view_table_set_show_grid(EGUI_VIEW_OF(&s_table), 0);
    egui_view_table_set_header_bg_color(EGUI_VIEW_OF(&s_table), header_bg);
    egui_view_table_set_header_text_color(EGUI_VIEW_OF(&s_table), header_text);
    egui_view_table_set_cell_text_color(EGUI_VIEW_OF(&s_table), cell_text);
    egui_view_table_set_grid_color(EGUI_VIEW_OF(&s_table), grid);
    egui_view_table_set_font(EGUI_VIEW_OF(&s_table), NULL);

    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_table_get_row_count(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_table_get_col_count(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_TRUE(egui_view_table_get_cell(EGUI_VIEW_OF(&s_table), 2, 1) == cell);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_table_get_header_rows(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(24, (int)egui_view_table_get_row_height(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_table_get_show_grid(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)header_bg.full, (int)egui_view_table_get_header_bg_color(EGUI_VIEW_OF(&s_table)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)header_text.full, (int)egui_view_table_get_header_text_color(EGUI_VIEW_OF(&s_table)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)cell_text.full, (int)egui_view_table_get_cell_text_color(EGUI_VIEW_OF(&s_table)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)grid.full, (int)egui_view_table_get_grid_color(EGUI_VIEW_OF(&s_table)).full);
    EGUI_TEST_ASSERT_TRUE(egui_view_table_get_font(EGUI_VIEW_OF(&s_table)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    egui_view_table_set_font(EGUI_VIEW_OF(&s_table), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_table_get_font(EGUI_VIEW_OF(&s_table)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
}

static void test_table_get_state_clamp_and_invalid_cell(void)
{
    setup();
    egui_view_table_set_size(EGUI_VIEW_OF(&s_table), 99, 99);
    egui_view_table_set_cell(EGUI_VIEW_OF(&s_table), EGUI_VIEW_TABLE_MAX_ROWS, 0, "ignored");
    egui_view_table_set_cell(EGUI_VIEW_OF(&s_table), 0, EGUI_VIEW_TABLE_MAX_COLS, "ignored");

    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TABLE_MAX_ROWS, (int)egui_view_table_get_row_count(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_VIEW_TABLE_MAX_COLS, (int)egui_view_table_get_col_count(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_NULL(egui_view_table_get_cell(EGUI_VIEW_OF(&s_table), EGUI_VIEW_TABLE_MAX_ROWS, 0));
    EGUI_TEST_ASSERT_NULL(egui_view_table_get_cell(EGUI_VIEW_OF(&s_table), 0, EGUI_VIEW_TABLE_MAX_COLS));
    EGUI_TEST_ASSERT_NULL(egui_view_table_get_cell(EGUI_VIEW_OF(&s_table), 0, 0));
}

static void test_table_get_state_apply_params(void)
{
    static const egui_view_table_params_t params = {
            .region = {{3, 4}, {120, 80}},
            .row_count = 5,
            .col_count = 4,
    };
    static const egui_view_table_params_t overflow_params = {
            .region = {{5, 6}, {140, 90}},
            .row_count = EGUI_VIEW_TABLE_MAX_ROWS + 1,
            .col_count = EGUI_VIEW_TABLE_MAX_COLS + 1,
    };

    setup();
    egui_view_table_apply_params(EGUI_VIEW_OF(&s_table), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(120, (int)egui_view_get_width(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(80, (int)egui_view_get_height(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_table_get_row_count(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_table_get_col_count(EGUI_VIEW_OF(&s_table)));

    egui_view_table_apply_params(EGUI_VIEW_OF(&s_table), &overflow_params);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_get_x(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_get_y(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(140, (int)egui_view_get_width(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_get_height(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_table_get_row_count(EGUI_VIEW_OF(&s_table)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_table_get_col_count(EGUI_VIEW_OF(&s_table)));
}

static void test_table_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_table_get_cell(NULL, 0, 0));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_table_get_row_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_table_get_col_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_table_get_header_rows(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_table_get_row_height(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_table_get_show_grid(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_table_get_header_bg_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_table_get_header_text_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_table_get_cell_text_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_table_get_grid_color(NULL).full);
    EGUI_TEST_ASSERT_NULL(egui_view_table_get_font(NULL));
}

void test_table_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(table_get_state);

    EGUI_TEST_RUN(test_table_get_state_defaults);
    EGUI_TEST_RUN(test_table_get_state_after_setters);
    EGUI_TEST_RUN(test_table_get_state_clamp_and_invalid_cell);
    EGUI_TEST_RUN(test_table_get_state_apply_params);
    EGUI_TEST_RUN(test_table_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
