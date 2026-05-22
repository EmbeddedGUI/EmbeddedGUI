#include <string.h>

#include "egui.h"
#include "widget/egui_view_switch.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_switch_get_bk_color_off.h"

static egui_view_switch_t s_sw;

static void setup(void)
{
    memset(&s_sw, 0, sizeof(s_sw));
    egui_view_switch_init(EGUI_VIEW_OF(&s_sw), uicode_get_core());
}

static void test_switch_get_bk_color_off_default(void)
{
    setup();
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_TRACK_OFF.full, (int)egui_view_switch_get_bk_color_off(EGUI_VIEW_OF(&s_sw)).full);
}

static void test_switch_get_bk_color_off_after_set(void)
{
    egui_color_t c;
    c.full = 0x778899u;
    setup();
    s_sw.bk_color_off = c;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c.full, (int)egui_view_switch_get_bk_color_off(EGUI_VIEW_OF(&s_sw)).full);
}

static void test_switch_get_bk_color_off_update(void)
{
    egui_color_t c1, c2;
    c1.full = 0x778899u;
    c2.full = 0xAABBCCu;
    setup();
    s_sw.bk_color_off = c1;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c1.full, (int)egui_view_switch_get_bk_color_off(EGUI_VIEW_OF(&s_sw)).full);
    s_sw.bk_color_off = c2;
    EGUI_TEST_ASSERT_EQUAL_INT((int)c2.full, (int)egui_view_switch_get_bk_color_off(EGUI_VIEW_OF(&s_sw)).full);
}

static void test_switch_get_bk_color_off_null_self(void)
{
    egui_color_t got = egui_view_switch_get_bk_color_off(NULL);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)got.full);
}

void test_switch_get_bk_color_off_run(void)
{
    EGUI_TEST_SUITE_BEGIN(switch_get_bk_color_off);

    EGUI_TEST_RUN(test_switch_get_bk_color_off_default);
    EGUI_TEST_RUN(test_switch_get_bk_color_off_after_set);
    EGUI_TEST_RUN(test_switch_get_bk_color_off_update);
    EGUI_TEST_RUN(test_switch_get_bk_color_off_null_self);

    EGUI_TEST_SUITE_END();
}
