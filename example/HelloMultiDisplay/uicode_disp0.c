/**
 * HelloMultiDisplay display 0:
 * green/red activity stack hosted on the primary core/display.
 */
#include "egui.h"
#include "uicode_disp0.h"

#define HELLO_MULTI_DISPLAY_ACTIVITY_POOL_SIZE 3

typedef struct hello_multi_display_disp0_activity hello_multi_display_disp0_activity_t;
struct hello_multi_display_disp0_activity
{
    egui_activity_t base;
    int index;
    char label_str[32];
    egui_view_linearlayout_t layout;
    egui_view_label_t label;
    egui_view_button_t btn_next;
    egui_view_button_t btn_finish;
};

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(hello_multi_display_disp0_bg_green_param, EGUI_COLOR_DARK_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(hello_multi_display_disp0_bg_green_params, &hello_multi_display_disp0_bg_green_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(hello_multi_display_disp0_bg_green, &hello_multi_display_disp0_bg_green_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(hello_multi_display_disp0_bg_red_param, EGUI_COLOR_RED, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(hello_multi_display_disp0_bg_red_params, &hello_multi_display_disp0_bg_red_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(hello_multi_display_disp0_bg_red, &hello_multi_display_disp0_bg_red_params);

static const egui_background_color_t *hello_multi_display_disp0_bg_list[] = {
        &hello_multi_display_disp0_bg_green,
        &hello_multi_display_disp0_bg_red,
        &hello_multi_display_disp0_bg_green,
};

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(hello_multi_display_disp0_anim_start_open_params, EGUI_CONFIG_SCEEN_WIDTH, 0, 0, 0);
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(hello_multi_display_disp0_anim_start_close_params, 0, -EGUI_CONFIG_SCEEN_WIDTH, 0, 0);
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(hello_multi_display_disp0_anim_finish_open_params, -EGUI_CONFIG_SCEEN_WIDTH, 0, 0, 0);
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(hello_multi_display_disp0_anim_finish_close_params, 0, EGUI_CONFIG_SCEEN_WIDTH, 0, 0);

static hello_multi_display_disp0_activity_t s_hello_multi_display_disp0_activities[HELLO_MULTI_DISPLAY_ACTIVITY_POOL_SIZE];
static int s_hello_multi_display_disp0_activity_slot = 0;
static int s_hello_multi_display_disp0_activity_index = 0;
static egui_toast_std_t s_hello_multi_display_disp0_toast;
static egui_animation_translate_t s_hello_multi_display_disp0_anim_start_open;
static egui_animation_translate_t s_hello_multi_display_disp0_anim_start_close;
static egui_animation_translate_t s_hello_multi_display_disp0_anim_finish_open;
static egui_animation_translate_t s_hello_multi_display_disp0_anim_finish_close;

static void hello_multi_display_disp0_start_next_activity(egui_core_t *core, egui_activity_t *current);

static void hello_multi_display_disp0_setup_activity_anims(egui_activity_t *activity)
{
    egui_animation_translate_init(EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_start_open));
    egui_animation_translate_params_set(&s_hello_multi_display_disp0_anim_start_open, &hello_multi_display_disp0_anim_start_open_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_start_open), 300);

    egui_animation_translate_init(EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_start_close));
    egui_animation_translate_params_set(&s_hello_multi_display_disp0_anim_start_close, &hello_multi_display_disp0_anim_start_close_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_start_close), 300);
    egui_animation_is_fill_before_set(EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_start_close), true);

    egui_animation_translate_init(EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_finish_open));
    egui_animation_translate_params_set(&s_hello_multi_display_disp0_anim_finish_open, &hello_multi_display_disp0_anim_finish_open_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_finish_open), 300);

    egui_animation_translate_init(EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_finish_close));
    egui_animation_translate_params_set(&s_hello_multi_display_disp0_anim_finish_close, &hello_multi_display_disp0_anim_finish_close_params);
    egui_animation_duration_set(EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_finish_close), 300);
    egui_animation_is_fill_before_set(EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_finish_close), true);

    egui_activity_set_start_anim(activity, EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_start_open),
                                 EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_start_close));
    egui_activity_set_finish_anim(activity, EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_finish_open),
                                  EGUI_ANIM_OF(&s_hello_multi_display_disp0_anim_finish_close));
}

static void hello_multi_display_disp0_btn_next_click(egui_view_t *self)
{
    egui_activity_t *activity = egui_view_get_activity(self);
    hello_multi_display_disp0_start_next_activity(NULL, activity);
}

static void hello_multi_display_disp0_btn_finish_click(egui_view_t *self)
{
    egui_activity_t *activity = egui_view_get_activity(self);

    if (activity != NULL)
    {
        egui_activity_finish(activity);
    }
}

static void hello_multi_display_disp0_activity_on_create(egui_activity_t *self)
{
    hello_multi_display_disp0_activity_t *local = (hello_multi_display_disp0_activity_t *)self;
    egui_core_t *core = egui_activity_get_core(self);

    egui_activity_on_create(self);

    egui_view_linearlayout_init((egui_view_t *)&local->layout, core);
    egui_view_set_position((egui_view_t *)&local->layout, 0, 0);
    egui_view_set_size((egui_view_t *)&local->layout, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_linearlayout_set_align_type((egui_view_t *)&local->layout, EGUI_ALIGN_CENTER);

    egui_view_label_init((egui_view_t *)&local->label, core);
    egui_view_set_size((egui_view_t *)&local->label, 180, 30);
    egui_view_set_margin_all((egui_view_t *)&local->label, 5);
    egui_view_label_set_text((egui_view_t *)&local->label, local->label_str);
    egui_view_label_set_align_type((egui_view_t *)&local->label, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&local->label, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&local->label, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    egui_view_button_init((egui_view_t *)&local->btn_next, core);
    egui_view_set_size((egui_view_t *)&local->btn_next, 150, 30);
    egui_view_set_margin_all((egui_view_t *)&local->btn_next, 5);
    egui_view_label_set_text((egui_view_t *)&local->btn_next, "Next Activity");
    egui_view_label_set_align_type((egui_view_t *)&local->btn_next, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&local->btn_next, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&local->btn_next, EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    egui_view_set_on_click_listener((egui_view_t *)&local->btn_next, hello_multi_display_disp0_btn_next_click);

    egui_view_button_init((egui_view_t *)&local->btn_finish, core);
    egui_view_set_size((egui_view_t *)&local->btn_finish, 150, 30);
    egui_view_set_margin_all((egui_view_t *)&local->btn_finish, 5);
    egui_view_label_set_text((egui_view_t *)&local->btn_finish, "Finish");
    egui_view_label_set_align_type((egui_view_t *)&local->btn_finish, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&local->btn_finish, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&local->btn_finish, EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    egui_view_set_on_click_listener((egui_view_t *)&local->btn_finish, hello_multi_display_disp0_btn_finish_click);

    egui_view_group_add_child((egui_view_t *)&local->layout, (egui_view_t *)&local->label);
    egui_view_group_add_child((egui_view_t *)&local->layout, (egui_view_t *)&local->btn_next);
    egui_view_group_add_child((egui_view_t *)&local->layout, (egui_view_t *)&local->btn_finish);
    egui_view_linearlayout_layout_childs((egui_view_t *)&local->layout);

    egui_activity_add_view(self, (egui_view_t *)&local->layout);
    egui_view_set_background((egui_view_t *)&local->layout,
                             (egui_background_t *)hello_multi_display_disp0_bg_list[local->index % EGUI_ARRAY_SIZE(hello_multi_display_disp0_bg_list)]);
}

static const egui_activity_api_t hello_multi_display_disp0_activity_api = {
        .on_create = hello_multi_display_disp0_activity_on_create,
        .on_start = egui_activity_on_start,
        .on_resume = egui_activity_on_resume,
        .on_pause = egui_activity_on_pause,
        .on_stop = egui_activity_on_stop,
        .on_destroy = egui_activity_on_destroy,
};

static void hello_multi_display_disp0_activity_init(hello_multi_display_disp0_activity_t *activity, egui_core_t *core, int index)
{
    egui_activity_init((egui_activity_t *)activity, core);
    ((egui_activity_t *)activity)->api = &hello_multi_display_disp0_activity_api;
    activity->index = index;
    egui_api_sprintf(activity->label_str, "D0 Activity %d", index);
}

static void hello_multi_display_disp0_start_next_activity(egui_core_t *core, egui_activity_t *current)
{
    hello_multi_display_disp0_activity_t *activity;

    if (core == NULL)
    {
        if (current != NULL)
        {
            core = egui_activity_get_core(current);
        }
        else
        {
            core = egui_toast_get_core((egui_toast_t *)&s_hello_multi_display_disp0_toast);
        }
    }

    if (s_hello_multi_display_disp0_activity_slot >= HELLO_MULTI_DISPLAY_ACTIVITY_POOL_SIZE)
    {
        if (current != NULL)
        {
            egui_activity_show_toast_info(current, "No more activities");
        }
        else
        {
            egui_toast_show_info((egui_toast_t *)&s_hello_multi_display_disp0_toast, "No more activities");
        }
        return;
    }

    activity = &s_hello_multi_display_disp0_activities[s_hello_multi_display_disp0_activity_slot];
    s_hello_multi_display_disp0_activity_slot++;
    hello_multi_display_disp0_activity_init(activity, core, s_hello_multi_display_disp0_activity_index);
    hello_multi_display_disp0_setup_activity_anims((egui_activity_t *)activity);
    s_hello_multi_display_disp0_activity_index++;
    egui_activity_start((egui_activity_t *)activity, current);
}

void uicode_disp0_init(egui_core_t *core)
{
    egui_toast_std_init((egui_toast_t *)&s_hello_multi_display_disp0_toast, core);
    egui_toast_set_as_default((egui_toast_t *)&s_hello_multi_display_disp0_toast);
    hello_multi_display_disp0_start_next_activity(core, NULL);
}

egui_view_t *hello_multi_display_disp0_get_next_button(int slot_index)
{
    if (slot_index < 0 || slot_index >= s_hello_multi_display_disp0_activity_slot)
    {
        return NULL;
    }

    return (egui_view_t *)&s_hello_multi_display_disp0_activities[slot_index].btn_next;
}

egui_view_t *hello_multi_display_disp0_get_finish_button(int slot_index)
{
    if (slot_index < 0 || slot_index >= s_hello_multi_display_disp0_activity_slot)
    {
        return NULL;
    }

    return (egui_view_t *)&s_hello_multi_display_disp0_activities[slot_index].btn_finish;
}

int hello_multi_display_disp0_get_activity_slot_count(void)
{
    return s_hello_multi_display_disp0_activity_slot;
}
