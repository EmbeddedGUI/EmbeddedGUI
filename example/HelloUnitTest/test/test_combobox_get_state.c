#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_combobox_get_state.h"

static egui_view_combobox_t s_combobox;
static const char *s_items[] = {"One", "Two", "Three"};
static const char *s_more_items[] = {"Four", "Five"};
static const char *s_icons[] = {"1", "2", "3"};
static const char s_expand_icon[] = "v";
static const char s_collapse_icon[] = "^";
static int s_selected_index;

static void on_selected(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    s_selected_index = (int)index;
}

static void assert_default_arrow_icons(egui_view_t *self)
{
    const char *expand_icon = egui_view_combobox_get_expand_icon(self);
    const char *collapse_icon = egui_view_combobox_get_collapse_icon(self);

    EGUI_TEST_ASSERT_NOT_NULL(expand_icon);
    EGUI_TEST_ASSERT_NOT_NULL(collapse_icon);
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(EGUI_ICON_MS_EXPAND_MORE, expand_icon));
    EGUI_TEST_ASSERT_EQUAL_INT(0, strcmp(EGUI_ICON_MS_EXPAND_LESS, collapse_icon));
}

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_combobox, 0, sizeof(s_combobox));
    s_selected_index = -1;
    egui_view_combobox_init(EGUI_VIEW_OF(&s_combobox), core);
}

static void test_combobox_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_items(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_get_item_count(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_item_icons(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_get_current_index(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_current_text(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_combobox_get_max_visible_items(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_font(EGUI_VIEW_OF(&s_combobox)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_icon_font(EGUI_VIEW_OF(&s_combobox)));
    assert_default_arrow_icons(EGUI_VIEW_OF(&s_combobox));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_combobox_get_icon_text_gap(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_is_expanded(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_on_selected_listener(EGUI_VIEW_OF(&s_combobox)));
}

static void test_combobox_get_state_after_setters(void)
{
    setup();
    egui_view_combobox_set_items(EGUI_VIEW_OF(&s_combobox), s_items, 3);
    egui_view_combobox_set_item_icons(EGUI_VIEW_OF(&s_combobox), s_icons);
    egui_view_combobox_set_current_index(EGUI_VIEW_OF(&s_combobox), 1);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&s_combobox), 2);
    egui_view_combobox_set_font(EGUI_VIEW_OF(&s_combobox), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&s_combobox), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&s_combobox), s_expand_icon, s_collapse_icon);
    egui_view_combobox_set_icon_text_gap(EGUI_VIEW_OF(&s_combobox), 9);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&s_combobox), on_selected);

    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_items(EGUI_VIEW_OF(&s_combobox)) == s_items);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_combobox_get_item_count(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_item_icons(EGUI_VIEW_OF(&s_combobox)) == s_icons);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_combobox_get_current_index(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_current_text(EGUI_VIEW_OF(&s_combobox)) == s_items[1]);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_combobox_get_max_visible_items(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_font(EGUI_VIEW_OF(&s_combobox)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_icon_font(EGUI_VIEW_OF(&s_combobox)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_expand_icon(EGUI_VIEW_OF(&s_combobox)) == s_expand_icon);
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_collapse_icon(EGUI_VIEW_OF(&s_combobox)) == s_collapse_icon);
    EGUI_TEST_ASSERT_EQUAL_INT(9, (int)egui_view_combobox_get_icon_text_gap(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_on_selected_listener(EGUI_VIEW_OF(&s_combobox)) == on_selected);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, s_selected_index);
}

static void test_combobox_get_state_update_and_clamp(void)
{
    setup();
    egui_view_combobox_set_items(EGUI_VIEW_OF(&s_combobox), s_items, 3);
    egui_view_combobox_set_current_index(EGUI_VIEW_OF(&s_combobox), 2);
    egui_view_combobox_set_items(EGUI_VIEW_OF(&s_combobox), s_more_items, 2);
    egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&s_combobox), 0);
    egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&s_combobox), NULL, NULL);

    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_items(EGUI_VIEW_OF(&s_combobox)) == s_more_items);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_combobox_get_item_count(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_get_current_index(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_current_text(EGUI_VIEW_OF(&s_combobox)) == s_more_items[0]);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_combobox_get_max_visible_items(EGUI_VIEW_OF(&s_combobox)));
    assert_default_arrow_icons(EGUI_VIEW_OF(&s_combobox));

    egui_view_combobox_set_current_index(EGUI_VIEW_OF(&s_combobox), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_get_current_index(EGUI_VIEW_OF(&s_combobox)));
}

static void test_combobox_get_state_apply_params(void)
{
    static const egui_view_combobox_params_t params = {
            .region = {{3, 4}, {80, 30}},
            .items = s_items,
            .item_icons = s_icons,
            .item_count = 3,
            .current_index = 2,
    };
    static const egui_view_combobox_params_t overflow_params = {
            .region = {{5, 6}, {90, 32}},
            .items = s_more_items,
            .item_icons = NULL,
            .item_count = 2,
            .current_index = 8,
    };

    setup();
    egui_view_combobox_apply_params(EGUI_VIEW_OF(&s_combobox), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(80, (int)egui_view_get_width(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_get_height(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_items(EGUI_VIEW_OF(&s_combobox)) == s_items);
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_item_icons(EGUI_VIEW_OF(&s_combobox)) == s_icons);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_combobox_get_item_count(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_combobox_get_current_index(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_current_text(EGUI_VIEW_OF(&s_combobox)) == s_items[2]);

    egui_view_combobox_apply_params(EGUI_VIEW_OF(&s_combobox), &overflow_params);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_get_x(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_get_y(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_get_width(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(32, (int)egui_view_get_height(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_TRUE(egui_view_combobox_get_items(EGUI_VIEW_OF(&s_combobox)) == s_more_items);
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_item_icons(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_combobox_get_item_count(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_get_current_index(EGUI_VIEW_OF(&s_combobox)));
}

static void test_combobox_get_state_clear_and_null_self(void)
{
    setup();
    egui_view_combobox_set_items(EGUI_VIEW_OF(&s_combobox), s_items, 3);
    egui_view_combobox_set_item_icons(EGUI_VIEW_OF(&s_combobox), s_icons);
    egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&s_combobox), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_combobox_set_items(EGUI_VIEW_OF(&s_combobox), NULL, 0);
    egui_view_combobox_set_item_icons(EGUI_VIEW_OF(&s_combobox), NULL);
    egui_view_combobox_set_font(EGUI_VIEW_OF(&s_combobox), NULL);
    egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&s_combobox), NULL);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&s_combobox), on_selected);
    egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&s_combobox), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_items(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_get_item_count(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_item_icons(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_current_text(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_font(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_icon_font(EGUI_VIEW_OF(&s_combobox)));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_on_selected_listener(EGUI_VIEW_OF(&s_combobox)));

    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_items(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_get_item_count(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_item_icons(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_get_current_index(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_current_text(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_get_max_visible_items(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_icon_font(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_expand_icon(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_collapse_icon(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_get_icon_text_gap(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_combobox_is_expanded(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_combobox_get_on_selected_listener(NULL));
}

void test_combobox_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(combobox_get_state);

    EGUI_TEST_RUN(test_combobox_get_state_defaults);
    EGUI_TEST_RUN(test_combobox_get_state_after_setters);
    EGUI_TEST_RUN(test_combobox_get_state_update_and_clamp);
    EGUI_TEST_RUN(test_combobox_get_state_apply_params);
    EGUI_TEST_RUN(test_combobox_get_state_clear_and_null_self);

    EGUI_TEST_SUITE_END();
}
