#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_toast.h"

static egui_toast_t test_toast;
static egui_toast_std_t test_toast_std;

static egui_core_t *test_toast_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void test_toast_show_requires_bound_core_when_unbound(void)
{
    egui_core_t *core = test_toast_get_core();
    static const char *test_text = "toast";

    egui_toast_init(&test_toast, core);
    test_toast.core = NULL;
    egui_toast_set_duration(&test_toast, 250);

    EGUI_TEST_ASSERT_TRUE(egui_toast_get_core(&test_toast) == NULL);
    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_show(&test_toast, test_text);

    EGUI_TEST_ASSERT_TRUE(egui_toast_get_core(&test_toast) == NULL);
    EGUI_TEST_ASSERT_TRUE(test_toast.info == NULL);
    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));
}

static void test_toast_show_starts_hide_timer_via_init_core_when_active_is_null(void)
{
    egui_core_t *core = test_toast_get_core();
    static const char *test_text = "toast bound";

    egui_toast_init(&test_toast, core);
    egui_toast_set_duration(&test_toast, 260);

    EGUI_TEST_ASSERT_TRUE(egui_toast_get_core(&test_toast) == core);
    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_show(&test_toast, test_text);

    EGUI_TEST_ASSERT_TRUE(egui_toast_get_core(&test_toast) == core);
    EGUI_TEST_ASSERT_TRUE(test_toast.info == test_text);
    EGUI_TEST_ASSERT_TRUE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_stop_timer(&test_toast, &test_toast.hide_timer);
    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));
}

static void test_toast_timer_helpers_require_bound_core(void)
{
    egui_core_t *core = test_toast_get_core();

    egui_toast_init(&test_toast, core);
    test_toast.core = NULL;

    EGUI_TEST_ASSERT_EQUAL_INT(-1, egui_toast_start_timer(&test_toast, &test_toast.hide_timer, 120, 0));
    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_stop_timer(&test_toast, &test_toast.hide_timer);
    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));
}

static void test_toast_timer_helpers_use_init_core_when_active_is_null(void)
{
    egui_core_t *core = test_toast_get_core();

    egui_toast_init(&test_toast, core);

    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));
    EGUI_TEST_ASSERT_EQUAL_INT(0, egui_toast_start_timer(&test_toast, &test_toast.hide_timer, 120, 0));
    EGUI_TEST_ASSERT_TRUE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_stop_timer(&test_toast, &test_toast.hide_timer);
    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));
}

static void test_toast_set_as_default_binds_std_toast_label_to_root(void)
{
    egui_core_t *core = test_toast_get_core();

    egui_toast_std_init((egui_toast_t *)&test_toast_std, core);

    EGUI_TEST_ASSERT_TRUE(egui_toast_get_core((egui_toast_t *)&test_toast_std) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_toast_std.label)) == core);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&test_toast_std.label)) == (egui_view_t *)egui_core_get_root_view(core));

    egui_toast_set_as_default((egui_toast_t *)&test_toast_std);

    EGUI_TEST_ASSERT_TRUE(egui_toast_get_core((egui_toast_t *)&test_toast_std) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_toast_std.label)) == core);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&test_toast_std.label)) == (egui_view_t *)egui_core_get_root_view(core));
    EGUI_TEST_ASSERT_TRUE(egui_view_get_toast((egui_view_t *)egui_core_get_root_view(core)) == (egui_toast_t *)&test_toast_std);

    egui_view_group_remove_child((egui_view_t *)egui_core_get_root_view(core), EGUI_VIEW_OF(&test_toast_std.label));
    egui_toast_clear_as_default((egui_toast_t *)&test_toast_std);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_toast((egui_view_t *)egui_core_get_root_view(core)) == NULL);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&test_toast_std.label)) == NULL);
}

static void test_toast_set_as_default_reattaches_detached_std_label_same_core(void)
{
    egui_core_t *core = test_toast_get_core();

    egui_toast_std_init((egui_toast_t *)&test_toast_std, core);

    egui_view_group_remove_child((egui_view_t *)egui_core_get_root_view(core), EGUI_VIEW_OF(&test_toast_std.label));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&test_toast_std.label)) == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_toast_std.label)) == core);

    egui_toast_set_as_default((egui_toast_t *)&test_toast_std);

    EGUI_TEST_ASSERT_TRUE(egui_toast_get_core((egui_toast_t *)&test_toast_std) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_toast_std.label)) == core);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&test_toast_std.label)) == (egui_view_t *)egui_core_get_root_view(core));
    EGUI_TEST_ASSERT_TRUE(egui_view_get_toast((egui_view_t *)egui_core_get_root_view(core)) == (egui_toast_t *)&test_toast_std);

    egui_toast_clear_as_default((egui_toast_t *)&test_toast_std);
}

static void test_toast_std_show_uses_init_core_when_active_is_null(void)
{
    egui_core_t *core = test_toast_get_core();
    static const char *test_text = "toast std bound label";

    egui_toast_std_init((egui_toast_t *)&test_toast_std, core);
    egui_toast_set_as_default((egui_toast_t *)&test_toast_std);

    egui_toast_show_info((egui_toast_t *)&test_toast_std, test_text);

    EGUI_TEST_ASSERT_TRUE(egui_toast_get_core((egui_toast_t *)&test_toast_std) == core);
    EGUI_TEST_ASSERT_TRUE(test_toast_std.base.info == test_text);
    EGUI_TEST_ASSERT_TRUE(egui_toast_check_timer_start((egui_toast_t *)&test_toast_std, &test_toast_std.base.hide_timer));

    egui_toast_stop_timer((egui_toast_t *)&test_toast_std, &test_toast_std.base.hide_timer);
    egui_toast_clear_as_default((egui_toast_t *)&test_toast_std);
}

static void test_toast_set_as_default_uses_explicit_init_core(void)
{
    egui_core_t local_core;
    static egui_color_int_t local_pfb[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];
    egui_color_int_t *pfb_bufs[1] = {local_pfb};

    egui_init_display(&local_core, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, pfb_bufs, 1, EGUI_CONFIG_PFB_WIDTH, EGUI_CONFIG_PFB_HEIGHT);

    egui_toast_std_init((egui_toast_t *)&test_toast_std, &local_core);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&test_toast_std.label)) == (egui_view_t *)egui_core_get_root_view(&local_core));

    egui_toast_set_as_default((egui_toast_t *)&test_toast_std);

    EGUI_TEST_ASSERT_TRUE(test_toast_std.base.core == &local_core);
    EGUI_TEST_ASSERT_TRUE(egui_toast_get_core((egui_toast_t *)&test_toast_std) == &local_core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_toast_std.label)) == &local_core);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&test_toast_std.label)) == (egui_view_t *)egui_core_get_root_view(&local_core));
    EGUI_TEST_ASSERT_TRUE(egui_view_get_toast((egui_view_t *)egui_core_get_root_view(&local_core)) == (egui_toast_t *)&test_toast_std);

    egui_toast_clear_as_default((egui_toast_t *)&test_toast_std);
}

static void test_toast_default_helpers_use_init_core_when_active_is_null(void)
{
    egui_core_t *core = test_toast_get_core();
    egui_view_group_t *root_view = egui_core_get_root_view(core);

    egui_toast_init(&test_toast, core);
    egui_toast_clear_as_default(&test_toast);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_toast((egui_view_t *)root_view) == NULL);

    egui_toast_set_as_default(&test_toast);

    EGUI_TEST_ASSERT_TRUE(egui_toast_get_core(&test_toast) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_toast((egui_view_t *)root_view) == &test_toast);

    egui_toast_clear_as_default(&test_toast);

    EGUI_TEST_ASSERT_TRUE(egui_view_get_toast((egui_view_t *)root_view) == NULL);
}

static void test_view_toast_helpers_use_view_init_core(void)
{
    egui_core_t *core = test_toast_get_core();
    egui_view_t test_view;
    static const char *duration_text = "view toast duration";
    static const char *default_text = "view toast default";

    egui_view_init(&test_view, core);
    egui_toast_init(&test_toast, core);
    egui_toast_set_as_default(&test_toast);

    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_view_show_toast_info_with_duration(&test_view, duration_text, 321);

    EGUI_TEST_ASSERT_TRUE(test_toast.info == duration_text);
    EGUI_TEST_ASSERT_EQUAL_INT(321, test_toast.duration);
    EGUI_TEST_ASSERT_TRUE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_stop_timer(&test_toast, &test_toast.hide_timer);
    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_view_show_toast_info(&test_view, default_text);

    EGUI_TEST_ASSERT_TRUE(test_toast.info == default_text);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME, test_toast.duration);
    EGUI_TEST_ASSERT_TRUE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_stop_timer(&test_toast, &test_toast.hide_timer);
    egui_toast_clear_as_default(&test_toast);
}

static void test_page_base_toast_helpers_use_init_core(void)
{
    egui_core_t *core = test_toast_get_core();
    egui_page_base_t page;
    static const char *duration_text = "page toast duration";
    static const char *default_text = "page toast default";

    egui_page_base_init(&page, core);

    egui_toast_init(&test_toast, core);
    egui_toast_set_as_default(&test_toast);

    egui_page_base_show_toast_info_with_duration(&page, duration_text, 432);

    EGUI_TEST_ASSERT_TRUE(test_toast.info == duration_text);
    EGUI_TEST_ASSERT_EQUAL_INT(432, test_toast.duration);
    EGUI_TEST_ASSERT_TRUE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_stop_timer(&test_toast, &test_toast.hide_timer);
    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_page_base_show_toast_info(&page, default_text);

    EGUI_TEST_ASSERT_TRUE(test_toast.info == default_text);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME, test_toast.duration);
    EGUI_TEST_ASSERT_TRUE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_stop_timer(&test_toast, &test_toast.hide_timer);
    egui_toast_clear_as_default(&test_toast);
}

static void test_activity_toast_helpers_use_init_core(void)
{
    egui_core_t *core = test_toast_get_core();
    egui_activity_t activity;
    static const char *duration_text = "activity toast duration";
    static const char *default_text = "activity toast default";

    egui_activity_init(&activity, core);

    egui_toast_init(&test_toast, core);
    egui_toast_set_as_default(&test_toast);

    egui_activity_show_toast_info_with_duration(&activity, duration_text, 543);

    EGUI_TEST_ASSERT_TRUE(test_toast.info == duration_text);
    EGUI_TEST_ASSERT_EQUAL_INT(543, test_toast.duration);
    EGUI_TEST_ASSERT_TRUE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_stop_timer(&test_toast, &test_toast.hide_timer);
    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_activity_show_toast_info(&activity, default_text);

    EGUI_TEST_ASSERT_TRUE(test_toast.info == default_text);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME, test_toast.duration);
    EGUI_TEST_ASSERT_TRUE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_stop_timer(&test_toast, &test_toast.hide_timer);
    egui_toast_clear_as_default(&test_toast);
}

static void test_dialog_toast_helpers_use_init_core(void)
{
    egui_core_t *core = test_toast_get_core();
    egui_dialog_t dialog;
    static const char *duration_text = "dialog toast duration";
    static const char *default_text = "dialog toast default";

    egui_dialog_init(&dialog, core);

    egui_toast_init(&test_toast, core);
    egui_toast_set_as_default(&test_toast);

    egui_dialog_show_toast_info_with_duration(&dialog, duration_text, 654);

    EGUI_TEST_ASSERT_TRUE(test_toast.info == duration_text);
    EGUI_TEST_ASSERT_EQUAL_INT(654, test_toast.duration);
    EGUI_TEST_ASSERT_TRUE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_stop_timer(&test_toast, &test_toast.hide_timer);
    EGUI_TEST_ASSERT_FALSE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_dialog_show_toast_info(&dialog, default_text);

    EGUI_TEST_ASSERT_TRUE(test_toast.info == default_text);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME, test_toast.duration);
    EGUI_TEST_ASSERT_TRUE(egui_toast_check_timer_start(&test_toast, &test_toast.hide_timer));

    egui_toast_stop_timer(&test_toast, &test_toast.hide_timer);
    egui_toast_clear_as_default(&test_toast);
}

void test_toast_run(void)
{
    EGUI_TEST_SUITE_BEGIN(toast);
    EGUI_TEST_RUN(test_toast_show_requires_bound_core_when_unbound);
    EGUI_TEST_RUN(test_toast_show_starts_hide_timer_via_init_core_when_active_is_null);
    EGUI_TEST_RUN(test_toast_timer_helpers_require_bound_core);
    EGUI_TEST_RUN(test_toast_timer_helpers_use_init_core_when_active_is_null);
    EGUI_TEST_RUN(test_toast_set_as_default_binds_std_toast_label_to_root);
    EGUI_TEST_RUN(test_toast_set_as_default_reattaches_detached_std_label_same_core);
    EGUI_TEST_RUN(test_toast_std_show_uses_init_core_when_active_is_null);
    EGUI_TEST_RUN(test_toast_set_as_default_uses_explicit_init_core);
    EGUI_TEST_RUN(test_toast_default_helpers_use_init_core_when_active_is_null);
    EGUI_TEST_RUN(test_view_toast_helpers_use_view_init_core);
    EGUI_TEST_RUN(test_page_base_toast_helpers_use_init_core);
    EGUI_TEST_RUN(test_activity_toast_helpers_use_init_core);
    EGUI_TEST_RUN(test_dialog_toast_helpers_use_init_core);
    EGUI_TEST_SUITE_END();
}
