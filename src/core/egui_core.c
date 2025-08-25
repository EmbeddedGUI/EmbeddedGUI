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

egui_region_t *egui_core_get_region_dirty_arr(void)
{
    return egui_core.region_dirty_arr;
}

void egui_core_update_region_dirty(egui_region_t *region_dirty)
{
    int i, j;
    int is_changed = 0;

    // change to the window dirty region
    EGUI_REGION_DEFINE(region_new_in_window, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_region_intersect(&region_new_in_window, region_dirty, &region_new_in_window);

    if(egui_region_is_empty(&region_new_in_window))
    {
        // EGUI_LOG_WRN("region_new_in_window is empty\r\n"); // change EGUI_CONFIG_DIRTY_AREA_COUNT
        return;
    }

    for (i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region_dirty = &egui_core.region_dirty_arr[i];
        if(!is_changed)
        {
            if (egui_region_is_intersect(p_region_dirty, &region_new_in_window) || egui_region_is_empty(p_region_dirty))
            {
                egui_region_union(p_region_dirty, &region_new_in_window, p_region_dirty);

                is_changed = 1;
            }
        }

        // merge other region
        if(!is_changed)
        {
            continue;
        }

        for(j = i + 1; j < EGUI_CONFIG_DIRTY_AREA_COUNT; j++)
        {
            if (egui_region_is_intersect(p_region_dirty, &egui_core.region_dirty_arr[j]) || egui_region_is_empty(p_region_dirty))
            {
                egui_region_union(p_region_dirty, &egui_core.region_dirty_arr[j], p_region_dirty);
                // clear the intersect region
                egui_region_init_empty(&egui_core.region_dirty_arr[j]);
            }
        }
    }

    // if no region intersect, let all region dirty
    if(!is_changed)
    {
        egui_core_clear_region_dirty();
        egui_region_init(&egui_core.region_dirty_arr[0], 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    }
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

void egui_core_draw_data(egui_region_t *p_region)
{
    egui_api_draw_data(p_region->location.x, p_region->location.y, p_region->size.width, p_region->size.height, egui_core.pfb);
}


void egui_core_draw_view_group(egui_region_t *p_region_dirty, int is_debug_mode)
{
    egui_view_group_t *view_group = &egui_core.root_view_group;
    egui_dim_t x, y, x_pos, y_pos;
    egui_dim_t x_pos_base = p_region_dirty->location.x;
    egui_dim_t y_pos_base = p_region_dirty->location.y;
    egui_dim_t width_dirty = p_region_dirty->size.width;
    egui_dim_t height_dirty = p_region_dirty->size.height;
    egui_dim_t pfb_width = egui_core.pfb_width;
    egui_dim_t pfb_height = egui_core.pfb_height;
    egui_dim_t tmp_pfb_width;
    egui_dim_t tmp_pfb_height;
    egui_dim_t pfb_width_count, pfb_height_count;

    EGUI_LOG_DBG("region_dirty, x: %d, y: %d, width: %d, height: %d\n"
        , p_region_dirty->location.x, p_region_dirty->location.y, p_region_dirty->size.width, p_region_dirty->size.height);

    // change pfb size to fit the dirty region
    if(pfb_width > width_dirty)
    {
        pfb_width = width_dirty;
        pfb_height = (egui_core.pfb_total_buffer_size / sizeof(egui_color_int_t)) / pfb_width;
    }
    else if(pfb_height > height_dirty)
    {
        pfb_height = height_dirty;
        pfb_width = (egui_core.pfb_total_buffer_size / sizeof(egui_color_int_t)) / pfb_height;
    }

    pfb_width_count = (width_dirty + pfb_width - 1) / pfb_width;
    pfb_height_count = (height_dirty + pfb_height - 1) / pfb_height;

    EGUI_LOG_DBG("pfb_update, pfb_width_count: %d, pfb_height_count: %d, pfb_width: %d, pfb_height: %d\n"
        , pfb_width_count, pfb_height_count, pfb_width, pfb_height);

    // start draw
    y_pos = y_pos_base;
    for (y = 0; y < pfb_height_count; y++)
    {
        x_pos = x_pos_base;
        for (x = 0; x < pfb_width_count; x++)
        {
            tmp_pfb_width = EGUI_MIN(pfb_width, x_pos_base + width_dirty- x_pos);
            tmp_pfb_height = EGUI_MIN(pfb_height, y_pos_base + height_dirty - y_pos);
            EGUI_LOG_DBG("pfb_region, x_pos: %d, y_pos: %d, pfb_width: %d, pfb_height: %d\n", x_pos, y_pos, tmp_pfb_width, tmp_pfb_height);

            EGUI_REGION_DEFINE(region, x_pos, y_pos, tmp_pfb_width, tmp_pfb_height);

            egui_canvas_init(egui_core.pfb, &region);

            int pfb_total_buffer_size = region.size.width * region.size.height * sizeof(egui_color_int_t);
            egui_api_pfb_clear(egui_core.pfb, pfb_total_buffer_size);

            view_group->base.api->draw((egui_view_t *)view_group);

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
            if(is_debug_mode)
            {
                egui_region_t *p_region;
                // change to screen coordinate 
                EGUI_REGION_DEFINE(region_screen, 0, 0, egui_core.screen_width, egui_core.screen_height);
                egui_canvas_calc_work_region(&region_screen);

#if EGUI_CONFIG_DEBUG_PFB_REFRESH
                p_region = egui_canvas_get_pfb_region();
                egui_canvas_draw_rectangle(p_region->location.x, p_region->location.y, p_region->size.width, p_region->size.height, 1, EGUI_COLOR_RED,
                                        EGUI_ALPHA_100);
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
                p_region = p_region_dirty;
                egui_canvas_draw_rectangle(p_region->location.x, p_region->location.y, p_region->size.width, p_region->size.height, 2, EGUI_COLOR_BLUE,
                                        EGUI_ALPHA_100);
#endif // EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
            }
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH

            egui_core_draw_data(&region);

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
            if(is_debug_mode)
            {
                egui_api_delay(EGUI_CONFIG_DEBUG_REFRESH_DELAY);
                egui_api_refresh_display();
            }
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH

            x_pos += pfb_width;
        }
        y_pos += pfb_height;
    }
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

void egui_polling_refresh_display(void)
{
#if EGUI_CONFIG_DEBUG_INFO_SHOW
    uint32_t start_time = egui_api_timer_get_current();
#endif

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
    // clear last all dirty region
    EGUI_REGION_DEFINE(region, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_core_draw_view_group(&region, false);
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH

    for(int i = 0; i < EGUI_CONFIG_DIRTY_AREA_COUNT; i++)
    {
        egui_region_t *p_region_dirty = &egui_core.region_dirty_arr[i];

        if (egui_region_is_empty(p_region_dirty))
        {
            break;
        }

        egui_core_draw_view_group(p_region_dirty, EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH);
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


int egui_core_activity_check_in_process(egui_activity_t *activity)
{
    // find the activity in the activitys list
    egui_dnode_t *p_head;
    egui_activity_t *tmp;

    if (!egui_dlist_is_empty(&egui_core.activitys))
    {
        EGUI_DLIST_FOR_EACH_NODE(&egui_core.activitys, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_activity_t, node);

            if (activity == tmp)
            {
                return 1;
            }
        }
    }

    return 0;
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

    if(self == prev_activity)
    {
        return;
    }

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


void egui_core_activity_start_with_current(egui_activity_t *self)
{
    egui_core_activity_start(self, egui_core_activity_get_current());
}

void egui_core_activity_finish(egui_activity_t *self)
{
    // find a last activity to start
    egui_dnode_t *p_prev = egui_dlist_peek_prev(&egui_core.activitys, &self->node);
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

void egui_core_activity_force_finish_to_activity(egui_activity_t *activity)
{
    // find the activity in the activitys list
    egui_dnode_t *p_head;
    egui_dnode_t *p_next;
    egui_activity_t *tmp;

    EGUI_LOG_DBG("egui_core_activity_force_finish_to_activity(), %d\n", egui_dlist_is_empty(&egui_core.activitys));
    if (!egui_dlist_is_empty(&egui_core.activitys))
    {
        EGUI_LOG_DBG("egui_core_activity_force_finish_to_activity() work\n");
        EGUI_DLIST_FOR_EACH_NODE_REVERSE_SAFE(&egui_core.activitys, p_head, p_next)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_activity_t, node);
            if (tmp == activity)
            {
                break;
            }
#if EGUI_CONFIG_DEBUG_CLASS_NAME
            EGUI_LOG_INF("force finish activity: %s, state: %d\n", tmp->name, tmp->state);
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

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_INF("restart activity: %s, state: %d\n", activity->name, activity->state);
#endif
    if (activity->state == EGUI_ACTIVITY_STATE_STOP)
    {
        activity->api->on_start(activity);
    }
    if (activity->state == EGUI_ACTIVITY_STATE_START)
    {
        activity->api->on_resume(activity);

        egui_view_set_position((egui_view_t *)&activity->root_view, 0, 0);
        egui_view_set_size((egui_view_t *)&activity->root_view, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    }

    // refresh the screen
    egui_core_update_region_dirty_all();
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

void egui_core_dialog_start_with_current(egui_dialog_t *self)
{
    egui_core_dialog_start(egui_core_activity_get_current(), self);
}

int egui_core_dialog_check_in_process(egui_dialog_t *dialog)
{
    if (egui_core.dialog == dialog)
    {
        return 1;
    }
    return 0;
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
    if(self->state == EGUI_DIALOG_STATE_NONE)
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
            egui_core.dialog = NULL;
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
    if (egui_check_need_refresh())
    {
        egui_polling_refresh_display();

        // after drawing, refresh the display
        egui_api_refresh_display();

#if EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
        // wait for a while to see the result
        egui_api_delay(EGUI_CONFIG_DEBUG_REFRESH_DELAY);
#endif // EGUI_CONFIG_DEBUG_PFB_REFRESH || EGUI_CONFIG_DEBUG_DIRTY_REGION_REFRESH
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

void egui_core_power_off(void)
{
    egui_timer_stop_timer(&egui_refresh_timer);
}

void egui_core_power_on(void)
{
    egui_timer_start_timer(&egui_refresh_timer, 0, 0);
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
    egui_view_set_size((egui_view_t *)&label_debug, EGUI_CONFIG_SCEEN_WIDTH, 0);
    egui_view_label_set_text((egui_view_t *)&label_debug, NULL);
    egui_view_label_set_align_type((egui_view_t *)&label_debug, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_font_with_std_height((egui_view_t *)&label_debug, (egui_font_t *)&egui_res_font_montserrat_10_4);
    egui_view_label_set_font_color((egui_view_t *)&label_debug, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
    egui_view_set_background((egui_view_t *)&label_debug, (egui_background_t *)&bg_debug);

    egui_core_add_root_view((egui_view_t *)&label_debug);

    egui_debug_update_work_time(0);
#endif

    egui_refresh_timer.callback = egui_refresh_timer_callback;
    egui_timer_start_timer(&egui_refresh_timer, 0, 0);

    // egui_canvas_test_circle();
}

