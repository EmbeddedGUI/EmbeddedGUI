#include <string.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"
#include "test_page_activity_lifecycle.h"

typedef struct test_lifecycle_page test_lifecycle_page_t;
typedef struct test_lifecycle_activity test_lifecycle_activity_t;

struct test_lifecycle_page
{
    egui_page_base_t base;
    egui_view_lyric_scroller_t lyric;
    egui_view_textblock_t textblock;
};

struct test_lifecycle_activity
{
    egui_activity_t base;
    egui_view_lyric_scroller_t lyric;
    egui_view_textblock_t textblock;
};

static test_lifecycle_page_t test_page_instance;
static test_lifecycle_activity_t test_activity_a;
static test_lifecycle_activity_t test_activity_b;
static egui_dialog_t test_dialog;

static const char *test_lifecycle_long_line = "A long single line lyric should overflow and scroll once it becomes attached inside the page or activity root.";

static egui_core_t *test_page_activity_lifecycle_get_core(void)
{
    egui_core_t *core = uicode_get_core();

    EGUI_ASSERT(core != NULL);
    return core;
}

static void test_lifecycle_prepare_lyric(egui_view_t *view, egui_core_t *target_core, egui_dim_t x, egui_dim_t y, egui_dim_t width)
{
    egui_view_lyric_scroller_init(view, target_core);
    egui_view_set_position(view, x, y);
    egui_view_set_size(view, width, 24);
    egui_view_set_padding(view, 2, 2, 0, 0);
    egui_view_lyric_scroller_set_font(view, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_lyric_scroller_set_interval_ms(view, 50);
    egui_view_lyric_scroller_set_scroll_step(view, 1);
    egui_view_lyric_scroller_set_pause_duration_ms(view, 0);
    egui_view_lyric_scroller_set_text(view, test_lifecycle_long_line);
}

static void test_lifecycle_prepare_textblock(egui_view_t *view, egui_core_t *target_core, egui_dim_t x, egui_dim_t y, egui_dim_t width)
{
    egui_view_textblock_init(view, target_core);
    egui_view_set_position(view, x, y);
    egui_view_set_size(view, width, 28);
    egui_view_set_padding(view, 2, 2, 0, 0);
    egui_view_textblock_set_font(view, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_textblock_set_text(view, "Editable lifecycle text");
    egui_view_textblock_set_editable(view, 1);
}

static void test_lifecycle_page_on_open(egui_page_base_t *self)
{
    test_lifecycle_page_t *local = (test_lifecycle_page_t *)self;
    egui_core_t *target_core = egui_page_base_get_core(self);

    egui_page_base_on_open(self);
    test_lifecycle_prepare_lyric(EGUI_VIEW_OF(&local->lyric), target_core, 8, 8, 48);
    test_lifecycle_prepare_textblock(EGUI_VIEW_OF(&local->textblock), target_core, 8, 40, 112);

    egui_page_base_add_view(self, EGUI_VIEW_OF(&local->lyric));
    egui_page_base_add_view(self, EGUI_VIEW_OF(&local->textblock));
    egui_view_request_focus(EGUI_VIEW_OF(&local->textblock));
}

static void test_lifecycle_page_on_close(egui_page_base_t *self)
{
    egui_page_base_on_close(self);
}

static const egui_page_base_api_t test_lifecycle_page_api = {
        .on_open = test_lifecycle_page_on_open,
        .on_close = test_lifecycle_page_on_close,
        .on_refresh = NULL,
        .on_key_pressed = egui_page_base_on_key_pressed,
};

static void test_lifecycle_page_init(test_lifecycle_page_t *page)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();

    memset(page, 0, sizeof(*page));
    egui_page_base_init(&page->base, core);
    page->base.api = &test_lifecycle_page_api;
}

static void test_lifecycle_activity_on_create(egui_activity_t *self)
{
    test_lifecycle_activity_t *local = (test_lifecycle_activity_t *)self;
    egui_core_t *target_core = egui_activity_get_core(self);

    egui_activity_on_create(self);
    test_lifecycle_prepare_lyric(EGUI_VIEW_OF(&local->lyric), target_core, 8, 8, 48);
    test_lifecycle_prepare_textblock(EGUI_VIEW_OF(&local->textblock), target_core, 8, 40, 112);

    egui_activity_add_view(self, EGUI_VIEW_OF(&local->lyric));
    egui_activity_add_view(self, EGUI_VIEW_OF(&local->textblock));
    egui_view_request_focus(EGUI_VIEW_OF(&local->textblock));
}

static void test_lifecycle_activity_on_destroy(egui_activity_t *self)
{
    egui_activity_on_destroy(self);
}

static const egui_activity_api_t test_lifecycle_activity_api = {
        .on_create = test_lifecycle_activity_on_create,
        .on_start = egui_activity_on_start,
        .on_resume = egui_activity_on_resume,
        .on_pause = egui_activity_on_pause,
        .on_stop = egui_activity_on_stop,
        .on_destroy = test_lifecycle_activity_on_destroy,
};

static void test_lifecycle_activity_init(test_lifecycle_activity_t *activity)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();

    memset(activity, 0, sizeof(*activity));
    egui_activity_init(&activity->base, core);
    activity->base.api = &test_lifecycle_activity_api;
}

static void test_page_open_close_propagates_attach_detach_to_dynamic_children(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();

    test_lifecycle_page_init(&test_page_instance);

    egui_page_base_open(&test_page_instance.base);
    EGUI_TEST_ASSERT_TRUE(egui_page_base_get_core(&test_page_instance.base) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_page_instance.base.root_view)) == core);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_page_instance.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_page_instance.lyric)->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(egui_page_base_check_timer_start(&test_page_instance.base, &test_page_instance.lyric.scroll_timer));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_page_instance.textblock)->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_page_instance.textblock)->is_focused);
    EGUI_TEST_ASSERT_TRUE(egui_page_base_check_timer_start(&test_page_instance.base, &test_page_instance.textblock.cursor_timer));

    egui_page_base_close(&test_page_instance.base);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_page_instance.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_page_instance.lyric)->is_attached_to_window);
    EGUI_TEST_ASSERT_FALSE(egui_page_base_check_timer_start(&test_page_instance.base, &test_page_instance.lyric.scroll_timer));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_page_instance.textblock)->is_attached_to_window);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_page_instance.textblock)->is_focused);
    EGUI_TEST_ASSERT_FALSE(egui_page_base_check_timer_start(&test_page_instance.base, &test_page_instance.textblock.cursor_timer));
}

static void test_page_open_uses_init_core_when_active_is_null(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();
    egui_page_base_t page;
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
    egui_page_base_init(&page, core);

    egui_page_base_open(&page);

    EGUI_TEST_ASSERT_TRUE(egui_page_base_get_core(&page) == core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&page.root_view)) == core);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&page.root_view)) == EGUI_VIEW_OF(user_root));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&page.root_view)->is_attached_to_window);

    egui_page_base_close(&page);

    EGUI_TEST_ASSERT_NULL(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&page.root_view)));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&page.root_view)->is_attached_to_window);
}

static void test_page_open_uses_explicit_bound_core_dimensions(void)
{
    egui_core_t local_core;
    egui_page_base_t page;
    static egui_color_int_t local_pfb[16 * 8];
    egui_color_int_t *pfb_bufs[1] = {local_pfb};
    egui_view_group_t *user_root;

    egui_init_display(&local_core, 128, 64, pfb_bufs, 1, 16, 8);
    user_root = egui_core_get_user_root_view(&local_core);
    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));

    egui_page_base_init(&page, &local_core);
    egui_page_base_open(&page);

    EGUI_TEST_ASSERT_TRUE(egui_page_base_get_core(&page) == &local_core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&page.root_view)) == &local_core);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&page.root_view)) == EGUI_VIEW_OF(user_root));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&page.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_EQUAL_INT(128, EGUI_VIEW_OF(&page.root_view)->region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(64, EGUI_VIEW_OF(&page.root_view)->region.size.height);

    egui_page_base_close(&page);
}

static void test_page_open_requires_bound_core_when_unbound(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();
    egui_page_base_t page;
    egui_view_group_t *user_root = egui_core_get_user_root_view(core);

    egui_view_group_clear_childs(EGUI_VIEW_OF(user_root));
    egui_page_base_init(&page, core);
    page.core = NULL;
    EGUI_VIEW_OF(&page.root_view)->core = NULL;

    EGUI_TEST_ASSERT_TRUE(egui_page_base_get_core(&page) == NULL);

    egui_page_base_open(&page);

    EGUI_TEST_ASSERT_TRUE(egui_page_base_get_core(&page) == NULL);
    EGUI_TEST_ASSERT_NULL(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&page.root_view)));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&page.root_view)->is_attached_to_window);
}

static void test_activity_switch_detaches_background_resources_and_resume_reattaches(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();

    test_lifecycle_activity_init(&test_activity_a);
    test_lifecycle_activity_init(&test_activity_b);

    egui_activity_start((egui_activity_t *)&test_activity_a, NULL);
    EGUI_TEST_ASSERT_TRUE(egui_activity_get_core((egui_activity_t *)&test_activity_a) == core);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_activity_a.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(egui_activity_check_timer_start((egui_activity_t *)&test_activity_a, &test_activity_a.lyric.scroll_timer));
    EGUI_TEST_ASSERT_TRUE(egui_activity_check_timer_start((egui_activity_t *)&test_activity_a, &test_activity_a.textblock.cursor_timer));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_activity_a.textblock)->is_focused);

    egui_activity_start((egui_activity_t *)&test_activity_b, (egui_activity_t *)&test_activity_a);
    EGUI_TEST_ASSERT_TRUE(egui_activity_get_core((egui_activity_t *)&test_activity_b) == core);
    EGUI_TEST_ASSERT_EQUAL_INT(EGUI_ACTIVITY_STATE_STOP, test_activity_a.base.state);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_activity_a.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_FALSE(egui_activity_check_timer_start((egui_activity_t *)&test_activity_a, &test_activity_a.lyric.scroll_timer));
    EGUI_TEST_ASSERT_FALSE(egui_activity_check_timer_start((egui_activity_t *)&test_activity_a, &test_activity_a.textblock.cursor_timer));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_activity_a.textblock)->is_focused);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_activity_b.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(egui_activity_check_timer_start((egui_activity_t *)&test_activity_b, &test_activity_b.lyric.scroll_timer));

    egui_activity_finish((egui_activity_t *)&test_activity_b);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_activity_a.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_TRUE(egui_activity_check_timer_start((egui_activity_t *)&test_activity_a, &test_activity_a.lyric.scroll_timer));
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_activity_a.textblock)->is_focused);
    EGUI_TEST_ASSERT_FALSE(egui_activity_check_timer_start((egui_activity_t *)&test_activity_a, &test_activity_a.textblock.cursor_timer));

    egui_activity_finish((egui_activity_t *)&test_activity_a);
    egui_focus_manager_clear_focus(core);
}

static void test_dialog_anim_requires_bound_core_when_unbound(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();
    egui_animation_translate_t open_anim;
    egui_animation_translate_t close_anim;
    egui_animation_t *prev_dialog_anim_start = core->scene.dialog_anim_start;
    egui_animation_t *prev_dialog_anim_finish = core->scene.dialog_anim_finish;

    egui_dialog_init(&test_dialog, core);
    egui_animation_translate_init(EGUI_ANIM_OF(&open_anim));
    egui_animation_translate_init(EGUI_ANIM_OF(&close_anim));

    core->scene.dialog_anim_start = NULL;
    core->scene.dialog_anim_finish = NULL;

    test_dialog.core = NULL;
    EGUI_VIEW_OF(&test_dialog.root_view)->core = NULL;
    EGUI_VIEW_OF(&test_dialog.user_root_view)->core = NULL;

    EGUI_TEST_ASSERT_TRUE(egui_dialog_get_core(&test_dialog) == NULL);

    egui_dialog_set_anim(&test_dialog, EGUI_ANIM_OF(&open_anim), EGUI_ANIM_OF(&close_anim));

    EGUI_TEST_ASSERT_TRUE(egui_dialog_get_core(&test_dialog) == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_dialog.root_view)) == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_dialog.user_root_view)) == NULL);
    EGUI_TEST_ASSERT_TRUE(core->scene.dialog_anim_start == NULL);
    EGUI_TEST_ASSERT_TRUE(core->scene.dialog_anim_finish == NULL);

    core->scene.dialog_anim_start = prev_dialog_anim_start;
    core->scene.dialog_anim_finish = prev_dialog_anim_finish;
}

static void test_activity_anim_helpers_use_init_core_when_active_is_null(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();
    egui_animation_translate_t start_open_anim;
    egui_animation_translate_t start_close_anim;
    egui_animation_translate_t finish_open_anim;
    egui_animation_translate_t finish_close_anim;
    egui_animation_t *prev_activity_anim_start_open = core->scene.activity_anim_start_open;
    egui_animation_t *prev_activity_anim_start_close = core->scene.activity_anim_start_close;
    egui_animation_t *prev_activity_anim_finish_open = core->scene.activity_anim_finish_open;
    egui_animation_t *prev_activity_anim_finish_close = core->scene.activity_anim_finish_close;

    test_lifecycle_activity_init(&test_activity_a);
    egui_animation_translate_init(EGUI_ANIM_OF(&start_open_anim));
    egui_animation_translate_init(EGUI_ANIM_OF(&start_close_anim));
    egui_animation_translate_init(EGUI_ANIM_OF(&finish_open_anim));
    egui_animation_translate_init(EGUI_ANIM_OF(&finish_close_anim));

    core->scene.activity_anim_start_open = NULL;
    core->scene.activity_anim_start_close = NULL;
    core->scene.activity_anim_finish_open = NULL;
    core->scene.activity_anim_finish_close = NULL;

    egui_activity_set_start_anim((egui_activity_t *)&test_activity_a, EGUI_ANIM_OF(&start_open_anim), EGUI_ANIM_OF(&start_close_anim));
    egui_activity_set_finish_anim((egui_activity_t *)&test_activity_a, EGUI_ANIM_OF(&finish_open_anim), EGUI_ANIM_OF(&finish_close_anim));

    EGUI_TEST_ASSERT_TRUE(egui_activity_get_core((egui_activity_t *)&test_activity_a) == core);
    EGUI_TEST_ASSERT_TRUE(core->scene.activity_anim_start_open == EGUI_ANIM_OF(&start_open_anim));
    EGUI_TEST_ASSERT_TRUE(core->scene.activity_anim_start_close == EGUI_ANIM_OF(&start_close_anim));
    EGUI_TEST_ASSERT_TRUE(core->scene.activity_anim_finish_open == EGUI_ANIM_OF(&finish_open_anim));
    EGUI_TEST_ASSERT_TRUE(core->scene.activity_anim_finish_close == EGUI_ANIM_OF(&finish_close_anim));

    core->scene.activity_anim_start_open = prev_activity_anim_start_open;
    core->scene.activity_anim_start_close = prev_activity_anim_start_close;
    core->scene.activity_anim_finish_open = prev_activity_anim_finish_open;
    core->scene.activity_anim_finish_close = prev_activity_anim_finish_close;
}

static void test_dialog_anim_uses_init_core_when_active_is_null(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();
    egui_animation_translate_t open_anim;
    egui_animation_translate_t close_anim;
    egui_animation_t *prev_dialog_anim_start = core->scene.dialog_anim_start;
    egui_animation_t *prev_dialog_anim_finish = core->scene.dialog_anim_finish;

    egui_dialog_init(&test_dialog, core);
    egui_animation_translate_init(EGUI_ANIM_OF(&open_anim));
    egui_animation_translate_init(EGUI_ANIM_OF(&close_anim));

    core->scene.dialog_anim_start = NULL;
    core->scene.dialog_anim_finish = NULL;

    egui_dialog_set_anim(&test_dialog, EGUI_ANIM_OF(&open_anim), EGUI_ANIM_OF(&close_anim));

    EGUI_TEST_ASSERT_TRUE(egui_dialog_get_core(&test_dialog) == core);
    EGUI_TEST_ASSERT_TRUE(core->scene.dialog_anim_start == EGUI_ANIM_OF(&open_anim));
    EGUI_TEST_ASSERT_TRUE(core->scene.dialog_anim_finish == EGUI_ANIM_OF(&close_anim));

    core->scene.dialog_anim_start = prev_dialog_anim_start;
    core->scene.dialog_anim_finish = prev_dialog_anim_finish;
}

static void test_activity_start_requires_bound_core_when_prev_activity_is_null(void)
{
    test_lifecycle_activity_init(&test_activity_a);
    test_activity_a.base.core = NULL;
    EGUI_VIEW_OF(&test_activity_a.base.root_view)->core = NULL;

    EGUI_TEST_ASSERT_TRUE(egui_activity_get_core((egui_activity_t *)&test_activity_a) == NULL);

    egui_activity_start((egui_activity_t *)&test_activity_a, NULL);

    EGUI_TEST_ASSERT_TRUE(egui_activity_get_core((egui_activity_t *)&test_activity_a) == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_activity_a.base.root_view)) == NULL);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_activity_a.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_FALSE(egui_activity_check_in_process((egui_activity_t *)&test_activity_a));
}

static void test_dialog_start_requires_bound_core_with_explicit_activity(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();

    test_lifecycle_activity_init(&test_activity_a);
    egui_dialog_init(&test_dialog, core);
    test_dialog.core = NULL;
    EGUI_VIEW_OF(&test_dialog.root_view)->core = NULL;
    EGUI_VIEW_OF(&test_dialog.user_root_view)->core = NULL;

    egui_activity_start((egui_activity_t *)&test_activity_a, NULL);

    EGUI_TEST_ASSERT_TRUE(egui_dialog_get_core(&test_dialog) == NULL);

    egui_dialog_start(&test_dialog, (egui_activity_t *)&test_activity_a);

    EGUI_TEST_ASSERT_TRUE(egui_dialog_get_core(&test_dialog) == NULL);
    EGUI_TEST_ASSERT_TRUE(test_dialog.bind_activity == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_dialog.root_view)) == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_dialog.user_root_view)) == NULL);
    EGUI_TEST_ASSERT_FALSE(egui_dialog_check_in_process(&test_dialog));

    egui_activity_finish((egui_activity_t *)&test_activity_a);
    egui_focus_manager_clear_focus(core);
}

static void test_activity_start_requires_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();

    test_lifecycle_activity_init(&test_activity_a);
    test_lifecycle_activity_init(&test_activity_b);
    test_activity_b.base.core = NULL;
    EGUI_VIEW_OF(&test_activity_b.base.root_view)->core = NULL;

    egui_activity_start((egui_activity_t *)&test_activity_a, NULL);

    EGUI_TEST_ASSERT_TRUE(egui_activity_get_core((egui_activity_t *)&test_activity_b) == NULL);

    egui_activity_start((egui_activity_t *)&test_activity_b, (egui_activity_t *)&test_activity_a);

    EGUI_TEST_ASSERT_TRUE(egui_activity_get_core((egui_activity_t *)&test_activity_b) == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_activity_b.base.root_view)) == NULL);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_activity_b.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_FALSE(egui_activity_check_in_process((egui_activity_t *)&test_activity_b));

    egui_activity_finish((egui_activity_t *)&test_activity_a);
    egui_focus_manager_clear_focus(core);
}

static void test_activity_start_requires_self_core_when_prev_core_is_null(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();

    test_lifecycle_activity_init(&test_activity_a);
    test_lifecycle_activity_init(&test_activity_b);
    test_activity_b.base.core = NULL;
    EGUI_VIEW_OF(&test_activity_b.base.root_view)->core = NULL;

    egui_activity_start((egui_activity_t *)&test_activity_a, NULL);

    test_activity_a.base.core = NULL;

    EGUI_TEST_ASSERT_TRUE(test_activity_a.base.core == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_activity_a.base.root_view)) == core);
    EGUI_TEST_ASSERT_TRUE(test_activity_b.base.core == NULL);

    egui_activity_start((egui_activity_t *)&test_activity_b, (egui_activity_t *)&test_activity_a);

    EGUI_TEST_ASSERT_TRUE(egui_activity_get_core((egui_activity_t *)&test_activity_b) == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_activity_b.base.root_view)) == NULL);
    EGUI_TEST_ASSERT_FALSE(EGUI_VIEW_OF(&test_activity_b.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_FALSE(egui_activity_check_in_process((egui_activity_t *)&test_activity_b));

    egui_activity_finish((egui_activity_t *)&test_activity_a);
    egui_focus_manager_clear_focus(core);
}

static void test_activity_start_uses_explicit_bound_core(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();
    egui_core_t local_core;
    static egui_color_int_t local_pfb[16 * 8];
    egui_color_int_t *pfb_bufs[1] = {local_pfb};

    test_lifecycle_activity_init(&test_activity_a);

    egui_init_display(&local_core, 128, 64, pfb_bufs, 1, 16, 8);
    egui_activity_init((egui_activity_t *)&test_activity_b, &local_core);

    egui_activity_start((egui_activity_t *)&test_activity_a, NULL);

    EGUI_TEST_ASSERT_TRUE(test_activity_b.base.core == &local_core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_activity_b.base.root_view)) == &local_core);

    egui_activity_start((egui_activity_t *)&test_activity_b, (egui_activity_t *)&test_activity_a);

    EGUI_TEST_ASSERT_TRUE(egui_activity_get_core((egui_activity_t *)&test_activity_b) == &local_core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_activity_b.base.root_view)) == &local_core);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&test_activity_b.base.root_view)) == EGUI_VIEW_OF(egui_core_get_user_root_view(&local_core)));
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_OF(&test_activity_b.base.root_view)->is_attached_to_window);
    EGUI_TEST_ASSERT_EQUAL_INT(128, EGUI_VIEW_OF(&test_activity_b.base.root_view)->region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(64, EGUI_VIEW_OF(&test_activity_b.base.root_view)->region.size.height);
    EGUI_TEST_ASSERT_TRUE(egui_activity_check_in_process((egui_activity_t *)&test_activity_b));

    egui_activity_finish((egui_activity_t *)&test_activity_b);
    egui_activity_finish((egui_activity_t *)&test_activity_a);
    egui_focus_manager_clear_focus(&local_core);
    egui_focus_manager_clear_focus(core);
}

static void test_dialog_start_requires_bound_core_when_active_is_null(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();

    test_lifecycle_activity_init(&test_activity_a);
    egui_dialog_init(&test_dialog, core);
    test_dialog.core = NULL;
    EGUI_VIEW_OF(&test_dialog.root_view)->core = NULL;
    EGUI_VIEW_OF(&test_dialog.user_root_view)->core = NULL;

    egui_activity_start((egui_activity_t *)&test_activity_a, NULL);

    EGUI_TEST_ASSERT_TRUE(egui_dialog_get_core(&test_dialog) == NULL);

    egui_dialog_start(&test_dialog, (egui_activity_t *)&test_activity_a);

    EGUI_TEST_ASSERT_TRUE(egui_dialog_get_core(&test_dialog) == NULL);
    EGUI_TEST_ASSERT_TRUE(test_dialog.bind_activity == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_dialog.root_view)) == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_dialog.user_root_view)) == NULL);
    EGUI_TEST_ASSERT_FALSE(egui_dialog_check_in_process(&test_dialog));

    egui_activity_finish((egui_activity_t *)&test_activity_a);
    egui_focus_manager_clear_focus(core);
}

static void test_dialog_start_requires_self_core_when_activity_core_is_null(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();

    test_lifecycle_activity_init(&test_activity_a);
    egui_dialog_init(&test_dialog, core);
    test_dialog.core = NULL;
    EGUI_VIEW_OF(&test_dialog.root_view)->core = NULL;
    EGUI_VIEW_OF(&test_dialog.user_root_view)->core = NULL;

    egui_activity_start((egui_activity_t *)&test_activity_a, NULL);

    test_activity_a.base.core = NULL;

    EGUI_TEST_ASSERT_TRUE(test_dialog.core == NULL);
    EGUI_TEST_ASSERT_TRUE(test_activity_a.base.core == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_activity_a.base.root_view)) == core);

    egui_dialog_start(&test_dialog, (egui_activity_t *)&test_activity_a);

    EGUI_TEST_ASSERT_TRUE(egui_dialog_get_core(&test_dialog) == NULL);
    EGUI_TEST_ASSERT_TRUE(test_dialog.bind_activity == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_dialog.root_view)) == NULL);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_dialog.user_root_view)) == NULL);
    EGUI_TEST_ASSERT_FALSE(egui_dialog_check_in_process(&test_dialog));

    egui_activity_finish((egui_activity_t *)&test_activity_a);
    egui_focus_manager_clear_focus(core);
}

static void test_dialog_start_uses_explicit_bound_core(void)
{
    egui_core_t *core = test_page_activity_lifecycle_get_core();
    egui_core_t local_core;
    static egui_color_int_t local_pfb[16 * 8];
    egui_color_int_t *pfb_bufs[1] = {local_pfb};

    test_lifecycle_activity_init(&test_activity_a);

    egui_init_display(&local_core, 128, 64, pfb_bufs, 1, 16, 8);
    egui_dialog_init(&test_dialog, &local_core);

    egui_activity_start((egui_activity_t *)&test_activity_a, NULL);

    EGUI_TEST_ASSERT_TRUE(test_dialog.core == &local_core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_dialog.root_view)) == &local_core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_dialog.user_root_view)) == &local_core);

    egui_dialog_start(&test_dialog, (egui_activity_t *)&test_activity_a);

    EGUI_TEST_ASSERT_TRUE(egui_dialog_get_core(&test_dialog) == &local_core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_dialog.root_view)) == &local_core);
    EGUI_TEST_ASSERT_TRUE(egui_view_get_core(EGUI_VIEW_OF(&test_dialog.user_root_view)) == &local_core);
    EGUI_TEST_ASSERT_TRUE(EGUI_VIEW_PARENT(EGUI_VIEW_OF(&test_dialog.root_view)) == EGUI_VIEW_OF(egui_core_get_user_root_view(&local_core)));
    EGUI_TEST_ASSERT_TRUE(egui_dialog_check_in_process(&test_dialog));
    EGUI_TEST_ASSERT_EQUAL_INT(128, EGUI_VIEW_OF(&test_dialog.root_view)->region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(64, EGUI_VIEW_OF(&test_dialog.root_view)->region.size.height);
    EGUI_TEST_ASSERT_EQUAL_INT(128, EGUI_VIEW_OF(&test_dialog.user_root_view)->region.size.width);
    EGUI_TEST_ASSERT_EQUAL_INT(64, EGUI_VIEW_OF(&test_dialog.user_root_view)->region.size.height);

    egui_dialog_finish(&test_dialog);
    egui_activity_finish((egui_activity_t *)&test_activity_a);
    egui_focus_manager_clear_focus(&local_core);
    egui_focus_manager_clear_focus(core);
}

void test_page_activity_lifecycle_run(void)
{
    EGUI_TEST_SUITE_BEGIN(page_activity_lifecycle);
    EGUI_TEST_RUN(test_page_open_close_propagates_attach_detach_to_dynamic_children);
    EGUI_TEST_RUN(test_page_open_uses_init_core_when_active_is_null);
    EGUI_TEST_RUN(test_page_open_uses_explicit_bound_core_dimensions);
    EGUI_TEST_RUN(test_page_open_requires_bound_core_when_unbound);
    EGUI_TEST_RUN(test_activity_switch_detaches_background_resources_and_resume_reattaches);
    EGUI_TEST_RUN(test_dialog_anim_requires_bound_core_when_unbound);
    EGUI_TEST_RUN(test_activity_anim_helpers_use_init_core_when_active_is_null);
    EGUI_TEST_RUN(test_dialog_anim_uses_init_core_when_active_is_null);
    EGUI_TEST_RUN(test_activity_start_requires_bound_core_when_prev_activity_is_null);
    EGUI_TEST_RUN(test_dialog_start_requires_bound_core_with_explicit_activity);
    EGUI_TEST_RUN(test_activity_start_requires_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_activity_start_requires_self_core_when_prev_core_is_null);
    EGUI_TEST_RUN(test_activity_start_uses_explicit_bound_core);
    EGUI_TEST_RUN(test_dialog_start_requires_bound_core_when_active_is_null);
    EGUI_TEST_RUN(test_dialog_start_requires_self_core_when_activity_core_is_null);
    EGUI_TEST_RUN(test_dialog_start_uses_explicit_bound_core);
    EGUI_TEST_SUITE_END();
}
