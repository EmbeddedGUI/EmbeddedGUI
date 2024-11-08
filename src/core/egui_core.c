#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_api.h"
#include "egui_common.h"
#include "egui_core.h"
#include "egui_timer.h"
#include "egui_theme.h"
#include "egui_input.h"
#include "resource/egui_resource.h"
#include "widget/egui_view.h"
#include "widget/egui_view_label.h"
#include "background/egui_background_color.h"
#include "utils/egui_slist.h"
#include "utils/egui_dlist.h"

#define EGUI_CORE_REFRESH_INTERVAL_MS (1000 / EGUI_CONFIG_MAX_FPS)

static egui_core_t egui_core;

// EGUI_VIEW_SUB_DEFINE(egui_view_group_t, test_view_group_1);

// static egui_view_tree_t view_tree_main[] = {
//     EGUI_VIEW_TREE_INIT(&test_view, EGUI_ID_TEST_VIEW, 0, 0, 200, 200),
//     EGUI_VIEW_TREE_INIT(&test_view_group_1, EGUI_ID_TEST_VIEW_GROUP_1, 50, 50, 100, 100),
// };


egui_view_group_t *egui_core_get_root_view(void)
{
    return &egui_core.root_view_group;
}

void egui_core_add_root_view(egui_view_t *view)
{
    egui_view_group_add_child((egui_view_t *)&egui_core.root_view_group, view);
}

int egui_core_check_region_dirty_intersect(egui_region_t *region_dirty)
{
    int i;
    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region = &egui_core.region_dirty_arr[i];
        if (egui_region_is_intersect(p_region, region_dirty))
        {
            return 1;
        }
    }

    return 0;
}

void egui_core_update_region_dirty(egui_region_t *region_dirty)
{
    int i;
    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region = &egui_core.region_dirty_arr[i];
        if (egui_region_is_intersect(p_region, region_dirty))
        {
            break;
        }
        else if (egui_region_is_empty(p_region))
        {
            break;
        }
    }

    // TODO: select neareast region to update
    if (i == EGUI_CONFIG_DIRTY_AREA_COUNT)
    {
        i = 0;
    }
    egui_region_union(&egui_core.region_dirty_arr[i], region_dirty, &egui_core.region_dirty_arr[i]);
    // EGUI_LOG_DBG("update region dirty: %d, %d, %d, %d\n", region_dirty->location.x, region_dirty->location.y, region_dirty->size.width,
    // region_dirty->size.height); EGUI_LOG_DBG("region dirty: %d, %d, %d, %d\n", egui_core.region_dirty_arr[i].location.x,
    // egui_core.region_dirty_arr[i].location.y, egui_core.region_dirty_arr[i].size.width, egui_core.region_dirty_arr[i].size.height);
}

void egui_core_clear_region_dirty(void)
{

    int i;
    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_init_empty(&egui_core.region_dirty_arr[i]);
    }
}

void egui_core_update_region_dirty_all(void)
{
    EGUI_REGION_DEFINE(region_screen, 0, 0, egui_core.screen_width, egui_core.screen_height);
    egui_core_update_region_dirty(&region_screen);
}

void egui_core_force_refresh(void)
{
    egui_core_update_region_dirty_all();
}

egui_view_group_t *egui_core_get_user_root_view(void)
{
    return &egui_core.user_root_view_group;
}

void egui_core_add_user_root_view(egui_view_t *view)
{
    egui_view_group_add_child((egui_view_t *)&egui_core.user_root_view_group, view);
}

void egui_core_remove_user_root_view(egui_view_t *view)
{
    egui_view_group_remove_child((egui_view_t *)&egui_core.user_root_view_group, view);

    egui_core_update_region_dirty_all();
}

void egui_core_layout_childs_user_root_view(uint8_t is_orientation_horizontal, uint8_t align_type)
{
    egui_view_group_layout_childs((egui_view_t *)&egui_core.user_root_view_group, is_orientation_horizontal, 0, 0, align_type);
}

int egui_core_draw_view_group(void)
{
    egui_view_group_t *view_group = &egui_core.root_view_group;

    // EGUI_LOG_DBG("================= draw view group =================\n");

    // Check and only update the dirty region
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
    {
        egui_api_pfb_clear(egui_core.pfb, egui_core.pfb_total_buffer_size);

        view_group->base.api->draw((egui_view_t *)view_group);

        {
            EGUI_REGION_DEFINE(region_screen, 0, 0, egui_core.screen_width, egui_core.screen_height);
            egui_canvas_calc_work_region(&region_screen);
            for (int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
            {
                egui_region_t *p_region = &egui_core.region_dirty_arr[i];
                egui_canvas_draw_rectangle(p_region->location.x, p_region->location.y, p_region->size.width, p_region->size.height, 1, EGUI_COLOR_RED,
                                           EGUI_ALPHA_100);
            }
        }

        if (egui_core_check_region_dirty_intersect(egui_canvas_get_pfb_region()))
        {
            return 1;
        }
    }
#else
    if (egui_core_check_region_dirty_intersect(egui_canvas_get_pfb_region()) || EGUI_CONFIG_DEBUG_FORCE_REFRESH_ALL)
    {
        egui_api_pfb_clear(egui_core.pfb, egui_core.pfb_total_buffer_size);
        // EGUI_LOG_DBG("pfb_region %d, %d, %d, %d\n", canvas->pfb_region.location.x, canvas->pfb_region.location.y, canvas->pfb_region.size.width,
        // canvas->pfb_region.size.height);
        view_group->base.api->draw((egui_view_t *)view_group);

        return 1;
    }
#endif

    return 0;
}
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
void egui_core_process_input_motion(egui_motion_event_t *motion_event)
{
    egui_view_group_dispatch_touch_event((egui_view_t *)egui_core_get_root_view(), motion_event);
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

uint16_t egui_core_get_unique_id(void)
{
    return egui_core.unique_id++;
}

#if EGUI_CONFIG_DEBUG_INFO_SHOW
static uint32_t debug_last_work_time = 0;
static char debug_string_info[100];

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_debug_normal, EGUI_COLOR_MAKE(128, 128, 128), 128);
EGUI_BACKGROUND_PARAM_INIT(bg_debug_params, &bg_debug_normal, NULL, NULL);
static egui_background_color_t bg_debug;
static egui_view_label_t label_debug;

static void egui_debug_update_work_time(uint32_t work_time)
{
    if (work_time != debug_last_work_time)
    {
        uint32_t fps = 0;
        debug_last_work_time = work_time;
        int cpu_use_percent = 100 * 100; // User 100 to save .xx info.
        if (work_time < (1000 / EGUI_CONFIG_MAX_FPS))
        {
            cpu_use_percent = (100 * 100) * work_time * EGUI_CONFIG_MAX_FPS / 1000;

            fps = EGUI_CONFIG_MAX_FPS;
        }
        else
        {
            if (work_time != 0)
            {
                fps = 1000 / work_time;
            }
        }

        egui_api_sprintf(debug_string_info, "FPS: %d, CPU %d.%d%%, LCD-Latency: %dms", fps, cpu_use_percent / 100, cpu_use_percent % 100, debug_last_work_time);
        egui_view_label_set_text((egui_view_t *)&label_debug, debug_string_info);
    }
}
#endif

static int test_color_value;
int32_t test_pfb_data(void)
{
    int y_pos, x_pos;
    test_color_value += 5;
    for (y_pos = 0; y_pos < egui_core.pfb_height; y_pos++)
    {
        for (x_pos = 0; x_pos < egui_core.pfb_width; x_pos++)
        {
            egui_core.pfb[y_pos * egui_core.pfb_width + x_pos] = 0; // EGUI_COLOR_MAKE(test_color_value, test_color_value, test_color_value);
        }
    }

    return 0;
}

void egui_polling_refresh_display(void)
{
    egui_dim_t x, y;
    egui_dim_t x_pos = 0;
    egui_dim_t y_pos = 0;

#if EGUI_CONFIG_DEBUG_INFO_SHOW
    uint32_t start_time = egui_api_timer_get_current();
#endif

    for (y = 0; y < egui_core.pfb_height_count; y++)
    {
        x_pos = 0;
        // test_pfb_data(); // For test
        for (x = 0; x < egui_core.pfb_width_count; x++)
        {
            // create a new canvas for the view group
            EGUI_REGION_DEFINE(region, x_pos, y_pos, egui_core.pfb_width, egui_core.pfb_height);
            egui_canvas_init(egui_core.pfb, &region);

            // EGUI_LOG_DBG("draw view group: %d, %d, %d, %d\n", x_pos, y_pos, egui_core.pfb_width, egui_core.pfb_height);
            // test_pfb_data(); // For test
            int is_update = egui_core_draw_view_group();

            // EGUI_LOG_DBG("is_update: %d\n", is_update);

#if EGUI_CONFIG_DEBUG_PFB_REFRESH
            if (is_update)
            {
                egui_color_int_t egui_pfb_debug_saved[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];
                egui_api_refresh_display();
                memcpy(egui_pfb_debug_saved, egui_core.pfb, sizeof(egui_pfb_debug_saved)); // svae last pfb data
                // draw a rectangle on the canvas to show the refresh area
                egui_canvas_calc_work_region(&region);

                egui_canvas_draw_rectangle(0, 0, egui_core.pfb_width, egui_core.pfb_height, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);
#if !EGUI_CONFIG_DEBUG_SKIP_DRAW_ALL
                egui_api_draw_data(x_pos, y_pos, egui_core.pfb_width, egui_core.pfb_height, egui_core.pfb);
#endif
                egui_api_refresh_display();
                memcpy(egui_core.pfb, egui_pfb_debug_saved, sizeof(egui_pfb_debug_saved)); // restore last pfb data

                egui_api_delay(EGUI_CONFIG_DEBUG_PFB_REFRESH_DELAY);
            }
#endif

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
#if !EGUI_CONFIG_DEBUG_SKIP_DRAW_ALL
            egui_api_draw_data(x_pos, y_pos, egui_core.pfb_width, egui_core.pfb_height, egui_core.pfb);
#endif
#else
            if (is_update)
            {
#if !EGUI_CONFIG_DEBUG_SKIP_DRAW_ALL
                egui_api_draw_data(x_pos, y_pos, egui_core.pfb_width, egui_core.pfb_height, egui_core.pfb);
#endif
            }
#endif

            x_pos += egui_core.pfb_width;
        }
        y_pos += egui_core.pfb_height;
    }

    // clear the dirty region
    egui_core_clear_region_dirty();

#if EGUI_CONFIG_DEBUG_INFO_SHOW
    // refresh in next frame.
    start_time = egui_api_timer_get_current() - start_time;
    egui_debug_update_work_time(start_time);
#endif
}

int egui_check_need_refresh(void)
{
    int i;
    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        if (!egui_region_is_empty(&egui_core.region_dirty_arr[i]))
        {
            return 1;
        }
    }

    return 0;
}

void egui_core_draw_view_group_pre_work(void)
{
    egui_view_group_t *view_group = &egui_core.root_view_group;

    // Calculate the layout of the view group
    view_group->base.api->compute_scroll((egui_view_t *)view_group);

    // Calculate the layout of the view group
    view_group->base.api->calculate_layout((egui_view_t *)view_group);
}

void egui_polling_work(void)
{
    egui_timer_polling_work();

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    if (!egui_input_check_idle())
    {
        egui_input_polling_work();
    }
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
}

egui_activity_t *egui_core_activity_get_by_view(egui_view_t *view)
{
    egui_view_t *view_activity = NULL;
    int found = 0;
    // the activity is in the parent view group, and is in user_root_view_group
    while (view)
    {
        if (view == (egui_view_t *)&egui_core.user_root_view_group)
        {
            found = 1;
            break;
        }

        view_activity = view;
        view = (egui_view_t *)view->parent;
    }

    if (found)
    {
        // find the activity in the activitys list
        egui_dnode_t *p_head;
        egui_activity_t *tmp;

        if (!egui_dlist_is_empty(&egui_core.activitys))
        {
            EGUI_DLIST_FOR_EACH_NODE(&egui_core.activitys, p_head)
            {
                tmp = EGUI_DLIST_ENTRY(p_head, egui_activity_t, node);

                if (view_activity == (egui_view_t *)&tmp->root_view)
                {
                    return tmp;
                }
            }
        }
    }

    return NULL;
}

void egui_core_activity_append(egui_activity_t *activity)
{
    egui_dlist_append(&egui_core.activitys, &activity->node);
}

void egui_core_activity_remove(egui_activity_t *activity)
{
    egui_dlist_remove(&activity->node);
}

void egui_core_activity_start(egui_activity_t *self, egui_activity_t *prev_activity)
{
    egui_core.activity_open = self;
    egui_core.activity_close = prev_activity;

    self->api->on_create(self);

    if (egui_core.activity_anim_start_open != NULL)
    {
        egui_animation_target_view_set(egui_core.activity_anim_start_open, (egui_view_t *)&self->root_view);
        egui_animation_start(egui_core.activity_anim_start_open);
    }
    else
    {
        self->api->on_resume(self);
    }

    if (prev_activity)
    {
        prev_activity->api->on_pause(prev_activity);
        // check anim
        if (egui_core.activity_anim_start_close != NULL)
        {
            egui_animation_target_view_set(egui_core.activity_anim_start_close, (egui_view_t *)&prev_activity->root_view);
            egui_animation_start(egui_core.activity_anim_start_close);
        }
        else
        {
            prev_activity->api->on_stop(prev_activity);
        }
    }
}

void egui_core_activity_finish(egui_activity_t *self)
{
    egui_core.activity_close = self;

    // avoid enter twice
    if (self->is_need_finish)
    {
        return;
    }
    self->api->on_pause(self);
    self->is_need_finish = true;
    if (self->state < EGUI_ACTIVITY_STATE_STOP)
    {
        // check anim
        if (egui_core.activity_anim_finish_close != NULL)
        {
            egui_animation_target_view_set(egui_core.activity_anim_finish_close, (egui_view_t *)&self->root_view);
            egui_animation_start(egui_core.activity_anim_finish_close);
        }
        else
        {
            self->api->on_stop(self);
        }
    }
    else
    {
        // something error.
        if (self->state < EGUI_ACTIVITY_STATE_DESTROY)
        {
            self->api->on_destroy(self);
        }
    }

    // find a last activity to start
    egui_dnode_t *p_prev = egui_dlist_peek_prev(&egui_core.activitys, &self->node);
    if (p_prev)
    {
        egui_activity_t *tmp = EGUI_DLIST_ENTRY(p_prev, egui_activity_t, node);
        egui_core.activity_open = tmp;

        tmp->api->on_start(tmp);

        if (egui_core.activity_anim_finish_open != NULL)
        {
            egui_animation_target_view_set(egui_core.activity_anim_finish_open, (egui_view_t *)&tmp->root_view);
            egui_animation_start(egui_core.activity_anim_finish_open);
        }
        else
        {
            tmp->api->on_resume(tmp);
        }
    }

    // refresh the screen
    egui_core_update_region_dirty_all();
}

egui_activity_t *egui_core_activity_get_current(void)
{
    egui_dnode_t *tmp = egui_dlist_peek_tail(&egui_core.activitys);
    if(tmp == NULL)
    {
        return NULL;
    }
    egui_activity_t *p_activity = EGUI_DLIST_ENTRY(tmp, egui_activity_t, node);
    return p_activity;
}

void egui_core_activity_force_finish_all(void)
{
    // find the activity in the activitys list
    egui_dnode_t *p_head;
    egui_dnode_t *p_next;
    egui_activity_t *tmp;

    EGUI_LOG_DBG("egui_core_activity_force_finish_all(), %d\n", egui_dlist_is_empty(&egui_core.activitys));
    if (!egui_dlist_is_empty(&egui_core.activitys))
    {
        EGUI_LOG_DBG("egui_core_activity_force_finish_all() work\n");
        EGUI_DLIST_FOR_EACH_NODE_SAFE(&egui_core.activitys, p_head, p_next)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_activity_t, node);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
            EGUI_LOG_DBG("force finish activity: %s, state: %d\n", tmp->name, tmp->state);
#endif
            if (tmp->state < EGUI_ACTIVITY_STATE_STOP)
            {
                tmp->api->on_stop(tmp);
            }
            if (tmp->state < EGUI_ACTIVITY_STATE_DESTROY)
            {
                tmp->api->on_destroy(tmp);
            }
        }
    }

    // refresh the screen
    egui_core_update_region_dirty_all();
}

static void on_activity_anim_start_open_end(egui_animation_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_start_open_end\n");
#endif
    if (egui_core.activity_open)
    {
        egui_core.activity_open->api->on_resume(egui_core.activity_open);
    }
}

static const egui_animation_handle_t activity_anim_start_open_hanlde = {
        .start = NULL,
        .end = on_activity_anim_start_open_end,
        .repeat = NULL,
};

static void on_activity_anim_start_close_end(egui_animation_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_start_close_end\n");
#endif
    if (egui_core.activity_close)
    {
        egui_core.activity_close->api->on_stop(egui_core.activity_close);
    }
}

static const egui_animation_handle_t activity_anim_start_close_hanlde = {
        .start = NULL,
        .end = on_activity_anim_start_close_end,
        .repeat = NULL,
};

static void on_activity_anim_finish_open_end(egui_animation_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_finish_open_end\n");
#endif
    if (egui_core.activity_open)
    {
        egui_core.activity_open->api->on_resume(egui_core.activity_open);
    }
}

static const egui_animation_handle_t activity_anim_finish_open_hanlde = {
        .start = NULL,
        .end = on_activity_anim_finish_open_end,
        .repeat = NULL,
};

static void on_activity_anim_finish_close_end(egui_animation_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_finish_close_end\n");
#endif

    if (egui_core.activity_close)
    {
        egui_core.activity_close->api->on_stop(egui_core.activity_close);
    }
}

static const egui_animation_handle_t activity_anim_finish_close_hanlde = {
        .start = NULL,
        .end = on_activity_anim_finish_close_end,
        .repeat = NULL,
};

void egui_core_activity_set_start_anim(egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    egui_core.activity_anim_start_open = open_anim;
    egui_core.activity_anim_start_close = close_anim;

    if (open_anim)
    {
        egui_animation_handle_set(open_anim, &activity_anim_start_open_hanlde);
    }
    if (close_anim)
    {
        egui_animation_handle_set(close_anim, &activity_anim_start_close_hanlde);
    }
}

void egui_core_activity_set_finish_anim(egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    egui_core.activity_anim_finish_open = open_anim;
    egui_core.activity_anim_finish_close = close_anim;

    if (open_anim)
    {
        egui_animation_handle_set(open_anim, &activity_anim_finish_open_hanlde);
    }
    if (close_anim)
    {
        egui_animation_handle_set(close_anim, &activity_anim_finish_close_hanlde);
    }
}

egui_dialog_t *egui_core_dialog_get(void)
{
    return egui_core.dialog;
}

void egui_core_dialog_start(egui_activity_t *activity, egui_dialog_t *self)
{
    egui_core.dialog = self;
    self->is_need_finish = 0;

    self->bind_activity = activity;

    self->api->on_create(self);
    activity->api->on_pause(activity);

    if (egui_core.dialog_anim_start != NULL)
    {
        egui_animation_target_view_set(egui_core.dialog_anim_start, (egui_view_t *)&self->user_root_view);
        egui_animation_start(egui_core.dialog_anim_start);
    }
    else
    {
        self->api->on_resume(self);
    }
}

void egui_core_dialog_finish(egui_dialog_t *self)
{
    egui_core.dialog = self;
    EGUI_LOG_DBG("egui_core_dialog_finish %p, self->is_need_finish: %d\n", self, self->is_need_finish);
    // avoid enter twice
    if (self->is_need_finish)
    {
        return;
    }

    self->is_need_finish = true;
    self->api->on_pause(self);
    if (self->state < EGUI_DIALOG_STATE_STOP)
    {
        // check anim
        if (egui_core.dialog_anim_finish != NULL)
        {
            egui_animation_target_view_set(egui_core.dialog_anim_finish, (egui_view_t *)&self->user_root_view);
            egui_animation_start(egui_core.dialog_anim_finish);
        }
        else
        {
            self->api->on_stop(self);
            self->bind_activity->api->on_resume(self->bind_activity);
        }
    }
    else
    {
        // something error.
        if (self->state < EGUI_DIALOG_STATE_DESTROY)
        {
            self->api->on_destroy(self);
        }
    }

    // refresh the screen
    egui_core_update_region_dirty(&self->root_view.base.region);
}

static void on_dialog_anim_start_end(egui_animation_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_dialog_anim_start_end\n");
#endif
    if (egui_core.dialog)
    {
        egui_core.dialog->api->on_resume(egui_core.dialog);
    }
}

static const egui_animation_handle_t dialog_anim_start_hanlde = {
        .start = NULL,
        .end = on_dialog_anim_start_end,
        .repeat = NULL,
};

static void on_dialog_anim_finish_end(egui_animation_t *self)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_dialog_anim_finish_end\n");
#endif

    if (egui_core.dialog)
    {
        egui_core.dialog->bind_activity->api->on_resume(egui_core.dialog->bind_activity);

        egui_core.dialog->api->on_stop(egui_core.dialog);
        egui_core.dialog = NULL;
    }
}

static const egui_animation_handle_t dialog_anim_finish_hanlde = {
        .start = NULL,
        .end = on_dialog_anim_finish_end,
        .repeat = NULL,
};

void egui_core_dialog_set_anim(egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    egui_core.dialog_anim_start = open_anim;
    egui_core.dialog_anim_finish = close_anim;

    if (open_anim)
    {
        egui_animation_handle_set(open_anim, &dialog_anim_start_hanlde);
    }
    if (close_anim)
    {
        egui_animation_handle_set(close_anim, &dialog_anim_finish_hanlde);
    }
}

egui_toast_t *egui_core_toast_get(void)
{
    return egui_core.toast;
}

void egui_core_toast_set(egui_toast_t *toast)
{
    egui_core.toast = toast;
}

void egui_core_toast_show_info_with_duration(const char *text, uint16_t duration)
{
    if (!egui_core.toast)
    {
        return;
    }
    egui_toast_set_duration(egui_core.toast, duration);
    egui_toast_show(egui_core.toast, text);
}

void egui_core_toast_show_info(const char *text)
{
    egui_core_toast_show_info_with_duration(text, EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME);
}

void egui_core_animation_append(egui_animation_t *anim)
{
    egui_slist_append(&egui_core.anims, &anim->node);
}

void egui_core_animation_remove(egui_animation_t *anim)
{
    egui_slist_find_and_remove(&egui_core.anims, &anim->node);
}

void egui_core_animation_polling_work(void)
{
    egui_snode_t *p_head;
    egui_snode_t *p_next;
    egui_animation_t *tmp;

    uint32_t anim_work_timestamp = egui_api_timer_get_current();

    if (!egui_slist_is_empty(&egui_core.anims))
    {
        EGUI_SLIST_FOR_EACH_NODE_SAFE(&egui_core.anims, p_head, p_next)
        {
            tmp = EGUI_SLIST_ENTRY(p_head, egui_animation_t, node);

            tmp->api->update(tmp, anim_work_timestamp);
        }
    }
}

void egui_core_refresh_screen(void)
{
    egui_core_animation_polling_work();
    egui_core_draw_view_group_pre_work();
    if (egui_check_need_refresh() || EGUI_CONFIG_DEBUG_FORCE_REFRESH_ALL)
    {
        egui_polling_refresh_display();

        // after drawing, refresh the display
        egui_api_refresh_display();

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
        // wait for a while to see the result
        egui_api_delay(EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH_DELAY);
#endif
    }
}

static egui_timer_t egui_refresh_timer;
static void egui_refresh_timer_callback(egui_timer_t *timer)
{
    uint32_t start_time = egui_api_timer_get_current();
    egui_core_refresh_screen();
    // get the time used to refresh the screen.
    // avoid refresh too frequently.
    start_time = egui_api_timer_get_current() - start_time;
    if(start_time >= EGUI_CORE_REFRESH_INTERVAL_MS)
    {
        egui_timer_start_timer(&egui_refresh_timer, 1, 0);
    }
    else
    {
        egui_timer_start_timer(&egui_refresh_timer, EGUI_CORE_REFRESH_INTERVAL_MS - start_time, 0);
    }
}

void egui_core_stop_auto_refresh_screen(void)
{
    egui_timer_stop_timer(&egui_refresh_timer);
}

void egui_core_set_pfb_buffer_ptr(egui_color_int_t *pfb)
{
    egui_core.pfb = pfb;
}

egui_color_int_t *egui_core_get_pfb_buffer_ptr(void)
{
    return egui_core.pfb;
}

void egui_core_pfb_set_buffer(egui_color_int_t *pfb, uint16_t width, uint16_t height)
{
    egui_core.pfb = pfb;
    egui_core.pfb_width = width;
    egui_core.pfb_height = height;
    egui_core.pfb_total_buffer_size = width * height * egui_core.color_bytes;

    // Calculate the number of pixels in each row and column of the PFB
    egui_core.pfb_width_count = (EGUI_CONFIG_SCEEN_WIDTH + width - 1) / width;
    egui_core.pfb_height_count = (EGUI_CONFIG_SCEEN_HEIGHT + height - 1) / height;
}

void egui_init(egui_color_int_t *pfb)
{
    egui_core.screen_width = EGUI_CONFIG_SCEEN_WIDTH;
    egui_core.screen_height = EGUI_CONFIG_SCEEN_HEIGHT;
    egui_core.color_bytes = EGUI_CONFIG_COLOR_DEPTH >> 3;

    egui_core_pfb_set_buffer(pfb, EGUI_CONFIG_PFB_WIDTH, EGUI_CONFIG_PFB_HEIGHT);

    // reset the unique id
    egui_core.unique_id = 0;

    egui_theme_init();
    egui_timer_init();
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_input_init();
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

    egui_core_update_region_dirty_all();

    egui_slist_init(&egui_core.anims);
    egui_dlist_init(&egui_core.activitys);

    egui_core.activity_anim_start_open = NULL;
    egui_core.activity_anim_start_close = NULL;
    egui_core.activity_anim_finish_open = NULL;
    egui_core.activity_anim_finish_close = NULL;

    egui_core.dialog_anim_start = NULL;
    egui_core.dialog_anim_finish = NULL;
    egui_core.dialog = NULL;

    egui_core.toast = NULL;

    // Initialize the root view group
    egui_view_group_init((egui_view_t *)&egui_core.root_view_group);
    egui_view_set_position((egui_view_t *)&egui_core.root_view_group, 0, 0);
    egui_view_set_size((egui_view_t *)&egui_core.root_view_group, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

    egui_view_group_init((egui_view_t *)&egui_core.user_root_view_group);
    egui_view_set_position((egui_view_t *)&egui_core.user_root_view_group, 0, 0);
    egui_view_set_size((egui_view_t *)&egui_core.user_root_view_group, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

    egui_core_add_root_view((egui_view_t *)&egui_core.user_root_view_group);

#if EGUI_CONFIG_DEBUG_INFO_SHOW
    // Inintialize the debug view
    egui_background_color_init((egui_background_t *)&bg_debug);
    egui_background_set_params((egui_background_t *)&bg_debug, &bg_debug_params);

    egui_view_label_init((egui_view_t *)&label_debug);
    egui_view_set_position((egui_view_t *)&label_debug, 0, EGUI_CONFIG_SCEEN_HEIGHT - 40);
    egui_view_set_size((egui_view_t *)&label_debug, EGUI_CONFIG_SCEEN_WIDTH, 10);
    egui_view_label_set_text((egui_view_t *)&label_debug, NULL);
    egui_view_label_set_align_type((egui_view_t *)&label_debug, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font((egui_view_t *)&label_debug, (egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color((egui_view_t *)&label_debug, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_view_set_background((egui_view_t *)&label_debug, (egui_background_t *)&bg_debug);

    egui_core_add_root_view((egui_view_t *)&label_debug);

    egui_debug_update_work_time(0);
#endif

    egui_refresh_timer.callback = egui_refresh_timer_callback;
    egui_timer_start_timer(&egui_refresh_timer, 0, 0);

    // egui_canvas_test_circle();
}
