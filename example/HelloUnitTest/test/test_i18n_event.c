#include <string.h>

#include "egui.h"
#include "test/egui_test.h"
#include "test_i18n_event.h"
#include "uicode_disp0.h"

enum
{
    TEST_I18N_HELLO = 0,
    TEST_I18N_ONLY_DEFAULT,
    TEST_I18N_COUNT,
};

static const char *s_i18n_en[TEST_I18N_COUNT] = {
        "Hello",
        "Default",
};

static const char *s_i18n_zh[TEST_I18N_COUNT] = {
        "Nihao",
        "",
};

static const egui_i18n_locale_t s_i18n_locales[] = {
        {.locale_code = "en", .strings = s_i18n_en, .count = TEST_I18N_COUNT},
        {.locale_code = "zh", .strings = s_i18n_zh, .count = TEST_I18N_COUNT},
};

#if EGUI_CONFIG_FUNCTION_EVENT_LITE
static int s_i18n_event_count;

static void test_i18n_event_cb(egui_event_t *event)
{
    EGUI_UNUSED(event);
    s_i18n_event_count++;
}
#endif

static void test_i18n_get_and_fallback(void)
{
    egui_core_t *core = uicode_get_core();

    egui_i18n_init(core, s_i18n_locales, EGUI_ARRAY_SIZE(s_i18n_locales));
    EGUI_TEST_ASSERT_TRUE(strcmp(egui_i18n_get(core, TEST_I18N_HELLO), "Hello") == 0);

    egui_i18n_set_locale_by_code(core, "zh");
    EGUI_TEST_ASSERT_TRUE(strcmp(egui_i18n_get(core, TEST_I18N_HELLO), "Nihao") == 0);
    EGUI_TEST_ASSERT_TRUE(strcmp(egui_i18n_get(core, TEST_I18N_ONLY_DEFAULT), "Default") == 0);
}

#if EGUI_CONFIG_FUNCTION_EVENT_LITE
static void test_i18n_language_changed_event(void)
{
    egui_core_t *core = uicode_get_core();
    egui_view_t listener_view;
    egui_view_group_t *root = egui_core_get_root_view(core);

    egui_i18n_init(core, s_i18n_locales, EGUI_ARRAY_SIZE(s_i18n_locales));
    egui_view_init(&listener_view, core);
    egui_view_add_event_listener(&listener_view, EGUI_EVENT_LANGUAGE_CHANGED, test_i18n_event_cb, NULL);
    egui_view_group_add_child(EGUI_VIEW_OF(root), &listener_view);

    s_i18n_event_count = 0;
    egui_i18n_set_locale(core, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_i18n_event_count);

    egui_i18n_set_locale(core, 1);
    EGUI_TEST_ASSERT_EQUAL_INT(1, s_i18n_event_count);

    egui_view_group_remove_child(EGUI_VIEW_OF(root), &listener_view);
}
#endif

void test_i18n_event_run(void)
{
    EGUI_TEST_SUITE_BEGIN(i18n_event);

    EGUI_TEST_RUN(test_i18n_get_and_fallback);
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
    EGUI_TEST_RUN(test_i18n_language_changed_event);
#endif

    EGUI_TEST_SUITE_END();
}
