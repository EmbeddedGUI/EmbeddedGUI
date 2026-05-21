#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_window_get_state.h"

static egui_view_window_t s_window;
static egui_view_label_t s_child;
static uint8_t s_close_count;

static void on_window_close(egui_view_t *self)
{
    EGUI_UNUSED(self);
    s_close_count++;
}

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_window, 0, sizeof(s_window));
    memset(&s_child, 0, sizeof(s_child));
    s_close_count = 0;
    egui_view_window_init(EGUI_VIEW_OF(&s_window), core);
    egui_view_label_init(EGUI_VIEW_OF(&s_child), core);
}

static void test_window_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_window_get_title(EGUI_VIEW_OF(&s_window)));
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_window_get_header_height(EGUI_VIEW_OF(&s_window)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_PRIMARY_DARK.full, (int)egui_view_window_get_header_color(EGUI_VIEW_OF(&s_window)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)EGUI_THEME_SURFACE.full, (int)egui_view_window_get_content_bg_color(EGUI_VIEW_OF(&s_window)).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(egui_view_window_get_close_icon(EGUI_VIEW_OF(&s_window)), EGUI_ICON_MS_CROSS));
    EGUI_TEST_ASSERT_NULL(egui_view_window_get_close_icon_font(EGUI_VIEW_OF(&s_window)));
    EGUI_TEST_ASSERT_TRUE(egui_view_window_get_content(EGUI_VIEW_OF(&s_window)) == EGUI_VIEW_OF(&s_window.content));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_view_group_get_child_count(egui_view_window_get_content(EGUI_VIEW_OF(&s_window))));
    EGUI_TEST_ASSERT_NULL(egui_view_window_get_on_close(EGUI_VIEW_OF(&s_window)));
}

static void test_window_get_state_after_setters(void)
{
    egui_color_t header_color = {.full = 0x1357};
    egui_color_t content_color = {.full = 0x2468};
    const char *title = "Panel";
    const char *close_icon = "x";

    setup();
    egui_view_window_set_title(EGUI_VIEW_OF(&s_window), title);
    egui_view_window_set_header_height(EGUI_VIEW_OF(&s_window), 24);
    egui_view_window_set_header_color(EGUI_VIEW_OF(&s_window), header_color);
    egui_view_window_set_content_bg_color(EGUI_VIEW_OF(&s_window), content_color);
    egui_view_window_set_close_icon(EGUI_VIEW_OF(&s_window), close_icon);
    egui_view_window_set_close_icon_font(EGUI_VIEW_OF(&s_window), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_window_set_on_close(EGUI_VIEW_OF(&s_window), on_window_close);

    EGUI_TEST_ASSERT_TRUE(egui_view_window_get_title(EGUI_VIEW_OF(&s_window)) == title);
    EGUI_TEST_ASSERT_EQUAL_INT(24, (int)egui_view_window_get_header_height(EGUI_VIEW_OF(&s_window)));
    EGUI_TEST_ASSERT_EQUAL_INT((int)header_color.full, (int)egui_view_window_get_header_color(EGUI_VIEW_OF(&s_window)).full);
    EGUI_TEST_ASSERT_EQUAL_INT((int)content_color.full, (int)egui_view_window_get_content_bg_color(EGUI_VIEW_OF(&s_window)).full);
    EGUI_TEST_ASSERT_TRUE(egui_view_window_get_close_icon(EGUI_VIEW_OF(&s_window)) == close_icon);
    EGUI_TEST_ASSERT_TRUE(egui_view_window_get_close_icon_font(EGUI_VIEW_OF(&s_window)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_window_get_on_close(EGUI_VIEW_OF(&s_window)) == on_window_close);

    egui_view_window_set_close_icon(EGUI_VIEW_OF(&s_window), NULL);
    egui_view_window_set_close_icon_font(EGUI_VIEW_OF(&s_window), NULL);
    egui_view_window_set_on_close(EGUI_VIEW_OF(&s_window), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_window_get_close_icon(EGUI_VIEW_OF(&s_window)));
    EGUI_TEST_ASSERT_NULL(egui_view_window_get_close_icon_font(EGUI_VIEW_OF(&s_window)));
    EGUI_TEST_ASSERT_NULL(egui_view_window_get_on_close(EGUI_VIEW_OF(&s_window)));
}

static void test_window_get_state_apply_params(void)
{
    static const egui_view_window_params_t params = {
            .region = {{3, 4}, {80, 60}},
            .header_height = 18,
            .title = "Prefs",
    };

    setup();
    egui_view_window_apply_params(EGUI_VIEW_OF(&s_window), &params);

    EGUI_TEST_ASSERT_TRUE(egui_view_window_get_title(EGUI_VIEW_OF(&s_window)) == params.title);
    EGUI_TEST_ASSERT_EQUAL_INT(18, (int)egui_view_window_get_header_height(EGUI_VIEW_OF(&s_window)));
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_window)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_window)));
    EGUI_TEST_ASSERT_EQUAL_INT(80, (int)egui_view_get_width(EGUI_VIEW_OF(&s_window)));
    EGUI_TEST_ASSERT_EQUAL_INT(60, (int)egui_view_get_height(EGUI_VIEW_OF(&s_window)));
    EGUI_TEST_ASSERT_EQUAL_INT(18, (int)egui_view_get_y(egui_view_window_get_content(EGUI_VIEW_OF(&s_window))));
    EGUI_TEST_ASSERT_EQUAL_INT(42, (int)egui_view_get_height(egui_view_window_get_content(EGUI_VIEW_OF(&s_window))));
}

static void test_window_get_state_content_group(void)
{
    setup();

    egui_view_window_add_content(EGUI_VIEW_OF(&s_window), EGUI_VIEW_OF(&s_child));

    EGUI_TEST_ASSERT_EQUAL_INT(1, egui_view_group_get_child_count(egui_view_window_get_content(EGUI_VIEW_OF(&s_window))));
    EGUI_TEST_ASSERT_TRUE(egui_view_group_get_child_at(egui_view_window_get_content(EGUI_VIEW_OF(&s_window)), 0) == EGUI_VIEW_OF(&s_child));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(egui_view_get_parent(EGUI_VIEW_OF(&s_child))) == egui_view_window_get_content(EGUI_VIEW_OF(&s_window)));
}

static void test_window_get_state_close_callback(void)
{
    setup();
    egui_view_window_set_on_close(EGUI_VIEW_OF(&s_window), on_window_close);

    egui_view_perform_click(EGUI_VIEW_OF(&s_window.close_label));

    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)s_close_count);
    EGUI_TEST_ASSERT_FALSE(egui_view_get_gone(EGUI_VIEW_OF(&s_window)));
    EGUI_TEST_ASSERT_TRUE(egui_view_window_get_on_close(EGUI_VIEW_OF(&s_window)) == on_window_close);
}

static void test_window_get_state_null_self(void)
{
    EGUI_TEST_ASSERT_NULL(egui_view_window_get_title(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_window_get_header_height(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_window_get_header_color(NULL).full);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_window_get_content_bg_color(NULL).full);
    EGUI_TEST_ASSERT_NULL(egui_view_window_get_close_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_window_get_close_icon_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_window_get_content(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_window_get_on_close(NULL));
}

void test_window_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(window_get_state);

    EGUI_TEST_RUN(test_window_get_state_defaults);
    EGUI_TEST_RUN(test_window_get_state_after_setters);
    EGUI_TEST_RUN(test_window_get_state_apply_params);
    EGUI_TEST_RUN(test_window_get_state_content_group);
    EGUI_TEST_RUN(test_window_get_state_close_callback);
    EGUI_TEST_RUN(test_window_get_state_null_self);

    EGUI_TEST_SUITE_END();
}
