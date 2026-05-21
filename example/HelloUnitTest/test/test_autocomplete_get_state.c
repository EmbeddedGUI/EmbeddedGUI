#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_autocomplete_get_state.h"

static egui_view_autocomplete_t s_autocomplete;
static const char *s_suggestions[] = {"alpha", "beta", "gamma"};
static const char *s_more_suggestions[] = {"delta", "epsilon"};
static int s_selected_index;

static void on_selected(egui_view_t *self, uint8_t index)
{
    EGUI_UNUSED(self);
    s_selected_index = (int)index;
}

static void setup(void)
{
    egui_core_t *core = uicode_get_core();

    memset(&s_autocomplete, 0, sizeof(s_autocomplete));
    s_selected_index = -1;
    egui_view_autocomplete_init(EGUI_VIEW_OF(&s_autocomplete), core);
}

static void test_autocomplete_get_state_defaults(void)
{
    setup();

    EGUI_TEST_ASSERT_NULL(egui_view_autocomplete_get_suggestions(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_autocomplete_get_suggestion_count(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_NULL(egui_view_autocomplete_get_current_text(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_autocomplete_get_max_visible_items(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_get_font(EGUI_VIEW_OF(&s_autocomplete)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_autocomplete_is_expanded(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_NULL(egui_view_autocomplete_get_on_selected_listener(EGUI_VIEW_OF(&s_autocomplete)));
}

static void test_autocomplete_get_state_after_setters(void)
{
    setup();
    egui_view_autocomplete_set_suggestions(EGUI_VIEW_OF(&s_autocomplete), s_suggestions, 3);
    egui_view_autocomplete_set_current_index(EGUI_VIEW_OF(&s_autocomplete), 1);
    egui_view_autocomplete_set_max_visible_items(EGUI_VIEW_OF(&s_autocomplete), 2);
    egui_view_autocomplete_set_font(EGUI_VIEW_OF(&s_autocomplete), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_autocomplete_set_on_selected_listener(EGUI_VIEW_OF(&s_autocomplete), on_selected);

    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_get_suggestions(EGUI_VIEW_OF(&s_autocomplete)) == s_suggestions);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_autocomplete_get_suggestion_count(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_get_current_text(EGUI_VIEW_OF(&s_autocomplete)) == s_suggestions[1]);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_autocomplete_get_max_visible_items(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_get_font(EGUI_VIEW_OF(&s_autocomplete)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_get_on_selected_listener(EGUI_VIEW_OF(&s_autocomplete)) == on_selected);
    EGUI_TEST_ASSERT_EQUAL_INT(-1, s_selected_index);
}

static void test_autocomplete_get_state_update_and_clamp(void)
{
    setup();
    egui_view_autocomplete_set_suggestions(EGUI_VIEW_OF(&s_autocomplete), s_suggestions, 3);
    egui_view_autocomplete_set_current_index(EGUI_VIEW_OF(&s_autocomplete), 2);
    egui_view_autocomplete_set_suggestions(EGUI_VIEW_OF(&s_autocomplete), s_more_suggestions, 2);
    egui_view_autocomplete_set_max_visible_items(EGUI_VIEW_OF(&s_autocomplete), 0);

    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_get_suggestions(EGUI_VIEW_OF(&s_autocomplete)) == s_more_suggestions);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_autocomplete_get_suggestion_count(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_get_current_text(EGUI_VIEW_OF(&s_autocomplete)) == s_more_suggestions[0]);
    EGUI_TEST_ASSERT_EQUAL_INT(1, (int)egui_view_autocomplete_get_max_visible_items(EGUI_VIEW_OF(&s_autocomplete)));

    egui_view_autocomplete_set_current_index(EGUI_VIEW_OF(&s_autocomplete), 9);
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&s_autocomplete)));
}

static void test_autocomplete_get_state_apply_params(void)
{
    static const egui_view_autocomplete_params_t params = {
            .region = {{3, 4}, {80, 30}},
            .suggestions = s_suggestions,
            .suggestion_count = 3,
            .current_index = 2,
    };
    static const egui_view_autocomplete_params_t overflow_params = {
            .region = {{5, 6}, {90, 32}},
            .suggestions = s_more_suggestions,
            .suggestion_count = 2,
            .current_index = 8,
    };

    setup();
    egui_view_autocomplete_apply_params(EGUI_VIEW_OF(&s_autocomplete), &params);

    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_get_x(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(4, (int)egui_view_get_y(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(80, (int)egui_view_get_width(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(30, (int)egui_view_get_height(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_get_suggestions(EGUI_VIEW_OF(&s_autocomplete)) == s_suggestions);
    EGUI_TEST_ASSERT_EQUAL_INT(3, (int)egui_view_autocomplete_get_suggestion_count(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&s_autocomplete)));

    egui_view_autocomplete_apply_params(EGUI_VIEW_OF(&s_autocomplete), &overflow_params);
    EGUI_TEST_ASSERT_EQUAL_INT(5, (int)egui_view_get_x(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(6, (int)egui_view_get_y(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(90, (int)egui_view_get_width(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(32, (int)egui_view_get_height(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_get_suggestions(EGUI_VIEW_OF(&s_autocomplete)) == s_more_suggestions);
    EGUI_TEST_ASSERT_EQUAL_INT(2, (int)egui_view_autocomplete_get_suggestion_count(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_autocomplete_get_current_index(EGUI_VIEW_OF(&s_autocomplete)));
}

static void test_autocomplete_get_state_clear_and_null_self(void)
{
    setup();
    egui_view_autocomplete_set_suggestions(EGUI_VIEW_OF(&s_autocomplete), s_suggestions, 3);
    egui_view_autocomplete_set_suggestions(EGUI_VIEW_OF(&s_autocomplete), NULL, 0);
    egui_view_autocomplete_set_on_selected_listener(EGUI_VIEW_OF(&s_autocomplete), on_selected);
    egui_view_autocomplete_set_on_selected_listener(EGUI_VIEW_OF(&s_autocomplete), NULL);
    egui_view_autocomplete_set_font(EGUI_VIEW_OF(&s_autocomplete), NULL);

    EGUI_TEST_ASSERT_NULL(egui_view_autocomplete_get_suggestions(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_autocomplete_get_suggestion_count(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_NULL(egui_view_autocomplete_get_current_text(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_NULL(egui_view_autocomplete_get_on_selected_listener(EGUI_VIEW_OF(&s_autocomplete)));
    EGUI_TEST_ASSERT_TRUE(egui_view_autocomplete_get_font(EGUI_VIEW_OF(&s_autocomplete)) == (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);

    EGUI_TEST_ASSERT_NULL(egui_view_autocomplete_get_suggestions(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_autocomplete_get_suggestion_count(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_autocomplete_get_current_index(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_autocomplete_get_current_text(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_autocomplete_get_max_visible_items(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_autocomplete_get_font(NULL));
    EGUI_TEST_ASSERT_EQUAL_INT(0, (int)egui_view_autocomplete_is_expanded(NULL));
    EGUI_TEST_ASSERT_NULL(egui_view_autocomplete_get_on_selected_listener(NULL));
}

void test_autocomplete_get_state_run(void)
{
    EGUI_TEST_SUITE_BEGIN(autocomplete_get_state);

    EGUI_TEST_RUN(test_autocomplete_get_state_defaults);
    EGUI_TEST_RUN(test_autocomplete_get_state_after_setters);
    EGUI_TEST_RUN(test_autocomplete_get_state_update_and_clamp);
    EGUI_TEST_RUN(test_autocomplete_get_state_apply_params);
    EGUI_TEST_RUN(test_autocomplete_get_state_clear_and_null_self);

    EGUI_TEST_SUITE_END();
}
