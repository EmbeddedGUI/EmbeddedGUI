#include <string.h>

#include "egui.h"
#include "widget/egui_view_tileview.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_tileview_get_state.h"

static egui_view_tileview_t s_tileview;
static egui_view_t s_tile0;
static egui_view_t s_tile1;
static uint8_t s_changed_col;
static uint8_t s_changed_row;

static void on_changed(egui_view_t *self, uint8_t col, uint8_t row)
{
    EGUI_UNUSED(self);
    s_changed_col = col;
    s_changed_row = row;
}

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_tileview, 0, sizeof(s_tileview));
    memset(&s_tile0, 0, sizeof(s_tile0));
    memset(&s_tile1, 0, sizeof(s_tile1));
    s_changed_col = 0xFF;
    s_changed_row = 0xFF;

    egui_view_tileview_init(EGUI_VIEW_OF(&s_tileview), core);
    egui_view_set_size(EGUI_VIEW_OF(&s_tileview), 100, 80);
    egui_view_init(&s_tile0, core);
    egui_view_set_size(&s_tile0, 100, 80);
    egui_view_init(&s_tile1, core);
    egui_view_set_size(&s_tile1, 100, 80);
}

static void test_tileview_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tileview_get_tile_count(EGUI_VIEW_OF(&s_tileview)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tileview_get_current_col(EGUI_VIEW_OF(&s_tileview)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tileview_get_current_row(EGUI_VIEW_OF(&s_tileview)));
    EGUI_TEST_ASSERT_NULL(egui_view_tileview_get_on_changed(EGUI_VIEW_OF(&s_tileview)));
}

static void test_tileview_get_state_after_add_tiles(void)
{
    setup();
    egui_view_tileview_add_tile(EGUI_VIEW_OF(&s_tileview), &s_tile0, 0, 0);
    egui_view_tileview_add_tile(EGUI_VIEW_OF(&s_tileview), &s_tile1, 1, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_tileview_get_tile_count(EGUI_VIEW_OF(&s_tileview)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tileview_get_current_col(EGUI_VIEW_OF(&s_tileview)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tileview_get_current_row(EGUI_VIEW_OF(&s_tileview)));
}

static void test_tileview_get_state_set_current(void)
{
    setup();
    egui_view_tileview_set_on_changed(EGUI_VIEW_OF(&s_tileview), on_changed);
    egui_view_tileview_add_tile(EGUI_VIEW_OF(&s_tileview), &s_tile0, 0, 0);
    egui_view_tileview_add_tile(EGUI_VIEW_OF(&s_tileview), &s_tile1, 1, 0);
    egui_view_tileview_set_current(EGUI_VIEW_OF(&s_tileview), 1, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_tileview_get_current_col(EGUI_VIEW_OF(&s_tileview)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tileview_get_current_row(EGUI_VIEW_OF(&s_tileview)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_changed_col);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)s_changed_row);
}

static void test_tileview_get_state_invalid_current_keeps_state(void)
{
    setup();
    egui_view_tileview_add_tile(EGUI_VIEW_OF(&s_tileview), &s_tile0, 0, 0);
    egui_view_tileview_add_tile(EGUI_VIEW_OF(&s_tileview), &s_tile1, 1, 0);
    egui_view_tileview_set_current(EGUI_VIEW_OF(&s_tileview), 1, 0);
    egui_view_tileview_set_current(EGUI_VIEW_OF(&s_tileview), 2, 0);

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_tileview_get_current_col(EGUI_VIEW_OF(&s_tileview)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tileview_get_current_row(EGUI_VIEW_OF(&s_tileview)));
}

static void test_tileview_get_state_listener_clear(void)
{
    setup();
    egui_view_tileview_set_on_changed(EGUI_VIEW_OF(&s_tileview), on_changed);
    EGUI_TEST_ASSERT_TRUE(egui_view_tileview_get_on_changed(EGUI_VIEW_OF(&s_tileview)) == on_changed);

    egui_view_tileview_set_on_changed(EGUI_VIEW_OF(&s_tileview), NULL);
    EGUI_TEST_ASSERT_NULL(egui_view_tileview_get_on_changed(EGUI_VIEW_OF(&s_tileview)));
}

static void test_tileview_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tileview_get_tile_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tileview_get_current_col(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_tileview_get_current_row(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_tileview_get_on_changed(NULL));
}

void test_tileview_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(tileview_get_state);

    EGUI_TEST_RUN(test_tileview_get_state_defaults);
    EGUI_TEST_RUN(test_tileview_get_state_after_add_tiles);
    EGUI_TEST_RUN(test_tileview_get_state_set_current);
    EGUI_TEST_RUN(test_tileview_get_state_invalid_current_keeps_state);
    EGUI_TEST_RUN(test_tileview_get_state_listener_clear);
    EGUI_TEST_RUN(test_tileview_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
