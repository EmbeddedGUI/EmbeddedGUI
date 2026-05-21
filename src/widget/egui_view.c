#include <stdio.h>
#include <assert.h>

#include "egui_view.h"
#include "widget/egui_view_group.h"
#include "core/egui_core.h"
#include "core/egui_core_internal.h"
#include "core/egui_api.h"
#include "core/egui_timer.h"
#include "style/egui_theme.h"
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
#include "core/egui_event.h"
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY
#include "core/egui_core_activity.h"
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG
#include "core/egui_core_dialog.h"
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOAST
#include "app/egui_toast.h"
#include "core/egui_core_toast.h"
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#include "shadow/egui_shadow.h"
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#include "core/egui_focus.h"
#endif

static void egui_view_invalidate_visible_tree_internal(egui_view_t *self);

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#ifndef EGUI_VIEW_FOCUS_FRAME_MARGIN
#define EGUI_VIEW_FOCUS_FRAME_MARGIN 2
#endif
#ifndef EGUI_VIEW_FOCUS_FRAME_STROKE
#define EGUI_VIEW_FOCUS_FRAME_STROKE 2
#endif
#endif

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
static void egui_view_log_dirty_source(const char *kind, egui_view_t *self, const egui_region_t *region)
{
    if (self == NULL || region == NULL)
    {
        return;
    }

#if EGUI_CONFIG_DEBUG_CLASS_NAME && EGUI_CONFIG_DEBUG_VIEW_ID
    egui_api_log("DIRTY_SOURCE:kind=%s,view=%s,id=%u,ptr=%p,x=%d,y=%d,w=%d,h=%d\r\n", kind, self->name ? self->name : "(null)", self->id, (void *)self,
                 region->location.x, region->location.y, region->size.width, region->size.height);
#elif EGUI_CONFIG_DEBUG_CLASS_NAME
    egui_api_log("DIRTY_SOURCE:kind=%s,view=%s,ptr=%p,x=%d,y=%d,w=%d,h=%d\r\n", kind, self->name ? self->name : "(null)", (void *)self, region->location.x,
                 region->location.y, region->size.width, region->size.height);
#elif EGUI_CONFIG_DEBUG_VIEW_ID
    egui_api_log("DIRTY_SOURCE:kind=%s,id=%u,ptr=%p,x=%d,y=%d,w=%d,h=%d\r\n", kind, self->id, (void *)self, region->location.x, region->location.y,
                 region->size.width, region->size.height);
#else
    egui_api_log("DIRTY_SOURCE:kind=%s,ptr=%p,x=%d,y=%d,w=%d,h=%d\r\n", kind, (void *)self, region->location.x, region->location.y, region->size.width,
                 region->size.height);
#endif
}
#endif

int egui_view_is_visible(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }

    if ((!self->is_visible) || (self->is_gone))
    {
        return 0;
    }
    egui_view_t *p = (egui_view_t *)self->parent;
    while (p)
    {
        if ((!p->is_visible) || (p->is_gone))
        {
            return 0;
        }

        p = (egui_view_t *)p->parent;
    }

    return 1;
}

static int egui_view_background_has_pressed_param(egui_view_t *self)
{
    if (self == NULL || self->background == NULL || self->background->params == NULL)
    {
        return 0;
    }

    return self->background->params->pressed_param != NULL;
}

static void egui_view_invalidate_full_region(egui_view_t *self)
{
    egui_region_t dirty_region;

    if (self == NULL)
    {
        return;
    }

    if (self->region.size.width <= 0 || self->region.size.height <= 0)
    {
        egui_view_invalidate(self);
        return;
    }

    egui_region_init(&dirty_region, 0, 0, self->region.size.width, self->region.size.height);
    egui_view_invalidate_region(self, &dirty_region);
}

void egui_view_invalidate_full(egui_view_t *self)
{
    egui_view_invalidate_full_region(self);
}

static void egui_view_invalidate_visual_region(egui_view_t *self)
{
    if (self == NULL)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
    if (self->is_dirty_passthrough)
    {
        egui_view_invalidate_visible_tree_internal(self);
    }
    else
#endif
    {
        egui_view_invalidate(self);
    }
}

void egui_view_invalidate(egui_view_t *self)
{
    egui_core_t *core;

    if (self == NULL || self->api == NULL || self->api->request_layout == NULL)
    {
        return;
    }

    if (egui_view_is_visible(self))
    {
        core = egui_view_get_core(self);
        if (core != NULL)
        {
            self->last_dirty_epoch = egui_core_get_dirty_epoch(core);
        }
        self->api->request_layout(self);
    }
}

egui_core_t *egui_view_get_core(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }

    return self->core;
}

uint32_t egui_view_get_dirty_epoch(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);

    return (core != NULL) ? egui_core_get_dirty_epoch(core) : 0U;
}

void egui_view_update_region_dirty(egui_view_t *self, egui_region_t *region_dirty)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL || region_dirty == NULL)
    {
        return;
    }

    egui_core_update_region_dirty(core, region_dirty);
}

egui_canvas_t *egui_view_get_canvas(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);

    return (core != NULL) ? &core->canvas : NULL;
}

egui_view_t *egui_view_get_focused_view(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    return (core != NULL) ? egui_focus_manager_get_focused_view(core) : NULL;
#else
    EGUI_UNUSED(core);
    return NULL;
#endif
}

int egui_view_is_self_or_descendant_of(egui_view_t *view, egui_view_t *ancestor)
{
    while (view != NULL)
    {
        if (view == ancestor)
        {
            return 1;
        }

        view = (egui_view_t *)view->parent;
    }

    return 0;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void egui_view_clear_focus_if_subtree_unfocusable(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);
    egui_view_t *focused;

    if (core == NULL)
    {
        return;
    }

    focused = egui_focus_manager_get_focused_view(core);
    if (focused != NULL && egui_view_is_self_or_descendant_of(focused, self) && !egui_focus_view_is_focusable(focused))
    {
        egui_focus_manager_clear_focus(core);
    }
}

static void egui_view_invalidate_focused_descendant_region(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);
    egui_view_t *focused;

    if (core == NULL)
    {
        return;
    }

    focused = egui_focus_manager_get_focused_view(core);
    if (focused != NULL && egui_view_is_self_or_descendant_of(focused, self))
    {
        egui_view_invalidate_focus_region(focused);
    }
}
#endif

void egui_view_clear_focus(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (core != NULL)
    {
        egui_focus_manager_clear_focus(core);
    }
#else
    EGUI_UNUSED(core);
#endif
}

void egui_view_set_theme(egui_view_t *self, const egui_theme_t *theme)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL || theme == NULL)
    {
        return;
    }

    egui_theme_set(core, theme);
}

egui_activity_t *egui_view_get_activity(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL || self == NULL)
    {
        return NULL;
    }

    return egui_core_activity_get_by_view(core, self);
#else
    EGUI_UNUSED(self);
    return NULL;
#endif
}

egui_dialog_t *egui_view_get_dialog(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return NULL;
    }

    return egui_core_dialog_get(core);
#else
    EGUI_UNUSED(self);
    return NULL;
#endif
}

egui_toast_t *egui_view_get_toast(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOAST
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return NULL;
    }

    return egui_core_toast_get(core);
#else
    EGUI_UNUSED(self);
    return NULL;
#endif
}

void egui_view_show_toast_info_with_duration(egui_view_t *self, const char *text, uint16_t duration)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOAST
    egui_toast_t *toast = egui_view_get_toast(self);

    if (toast == NULL)
    {
        return;
    }

    egui_toast_show_info_with_duration(toast, text, duration);
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(text);
    EGUI_UNUSED(duration);
#endif
}

void egui_view_show_toast_info(egui_view_t *self, const char *text)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOAST
    egui_view_show_toast_info_with_duration(self, text, EGUI_CONFIG_PARAM_TOAST_DEFAULT_SHOW_TIME);
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(text);
#endif
}

egui_float_t egui_view_get_velocity_x(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return 0;
    }

    return egui_input_get_velocity_x(core);
}

egui_float_t egui_view_get_velocity_y(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return 0;
    }

    return egui_input_get_velocity_y(core);
}

int egui_view_start_timer(egui_view_t *self, egui_timer_t *handle, uint32_t ms, uint32_t period)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return -1;
    }

    return egui_timer_start_timer(core, handle, ms, period);
}

void egui_view_stop_timer(egui_view_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_timer_stop_timer(core, handle);
}

int egui_view_check_timer_start(egui_view_t *self, egui_timer_t *handle)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return 0;
    }

    return egui_timer_check_timer_start(core, handle);
}

void egui_view_add_to_root(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_add_root_view(core, self);
}

void egui_view_remove_from_user_root(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_remove_user_root_view(core, self);
}

void egui_view_layout_user_root(egui_view_t *self, uint8_t is_orientation_horizontal, uint8_t align_type)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_layout_childs_user_root_view(core, is_orientation_horizontal, align_type);
}

void egui_view_set_pfb_scan_direction(egui_view_t *self, uint8_t reverse_x, uint8_t reverse_y)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_set_pfb_scan_direction(core, reverse_x, reverse_y);
}

void egui_view_reset_pfb_scan_direction(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return;
    }

    egui_core_reset_pfb_scan_direction(core);
}

void egui_view_invalidate_region(egui_view_t *self, const egui_region_t *dirty_region)
{
    egui_core_t *core;

    if (self == NULL || dirty_region == NULL || !egui_view_is_visible(self))
    {
        return;
    }

    EGUI_REGION_DEFINE(screen_region, self->region_screen.location.x + dirty_region->location.x, self->region_screen.location.y + dirty_region->location.y,
                       dirty_region->size.width, dirty_region->size.height);
    egui_region_intersect(&screen_region, &self->region_screen, &screen_region);

    if (!egui_region_is_empty(&screen_region))
    {
        core = egui_view_get_core(self);
        if (core != NULL)
        {
            self->last_dirty_epoch = egui_core_get_dirty_epoch(core);
        }
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
        egui_view_log_dirty_source("subregion", self, &screen_region);
#endif
        egui_view_update_region_dirty(self, &screen_region);
    }
}

uint8_t egui_view_has_pending_dirty(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);

    if (self == NULL || core == NULL)
    {
        return 0;
    }

    return (self->last_dirty_epoch == egui_core_get_dirty_epoch(core)) ? 1U : 0U;
}

void egui_view_invalidate_sub_region(egui_view_t *self, const egui_sub_region_table_t *table, uint16_t index)
{
    if (table == NULL || table->regions == NULL || index >= table->count)
    {
        return;
    }

    egui_view_invalidate_region(self, &table->regions[index].region);
}

void egui_view_set_background(egui_view_t *self, egui_background_t *background)
{
    if (self == NULL)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
    egui_background_t *old_background = self->background;
#endif

    if (background == self->background)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
    if (self->is_dirty_passthrough && old_background != NULL && background == NULL)
    {
        egui_view_invalidate_full_region(self);
        self->background = background;
        return;
    }
#endif

    self->background = background;
    egui_view_invalidate_visual_region(self);
}

void egui_view_draw_background(egui_view_t *self)
{
    egui_canvas_t *canvas;
    const egui_background_t *bg;

    if (self == NULL)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE
    bg = egui_view_get_effective_background(self);
#else
    bg = self->background;
#endif

    if (bg == NULL || bg->api == NULL || bg->api->draw == NULL)
    {
        return;
    }

    canvas = egui_view_get_canvas(self);
    if (canvas == NULL)
    {
        return;
    }

    bg->api->draw((egui_background_t *)(uintptr_t)bg, canvas, self);
}

/* ---- Style cascade API (compiled only when the feature is enabled) ---- */

#if EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
/* Forward declaration — implementation follows after the cascade section. */
uint8_t egui_view_get_computed_state(const egui_view_t *self);
#endif

#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE

int egui_view_add_style(egui_view_t *self, const egui_view_style_t *style)
{
    if (self == NULL || style == NULL)
    {
        return -1;
    }
    if (self->style_count >= EGUI_CONFIG_STYLE_MAX_PER_VIEW)
    {
        return -1;
    }
    self->styles[self->style_count] = style;
    self->style_count++;
    egui_view_invalidate_visual_region(self);
    return 0;
}

int egui_view_remove_style(egui_view_t *self, const egui_view_style_t *style)
{
    uint8_t i;
    uint8_t found = 0;

    if (self == NULL || style == NULL)
    {
        return -1;
    }
    for (i = 0; i < self->style_count; i++)
    {
        if (self->styles[i] == style)
        {
            found = 1;
        }
        if (found && i + 1 < self->style_count)
        {
            self->styles[i] = self->styles[i + 1];
        }
    }
    if (!found)
    {
        return -1;
    }
    self->style_count--;
    self->styles[self->style_count] = NULL;
    egui_view_invalidate_visual_region(self);
    return 0;
}

void egui_view_clear_styles(egui_view_t *self)
{
    uint8_t i;

    if (self == NULL)
    {
        return;
    }
    for (i = 0; i < self->style_count; i++)
    {
        self->styles[i] = NULL;
    }
    self->style_count = 0;
    egui_view_invalidate_visual_region(self);
}

const egui_background_t *egui_view_get_effective_background(egui_view_t *self)
{
    int i;

    if (self == NULL)
    {
        return NULL;
    }
    /* Inline background has the highest priority. */
    if (self->background != NULL)
    {
        return self->background;
    }
    /* Walk styles from highest to lowest priority. */
    for (i = (int)self->style_count - 1; i >= 0; i--)
    {
        const egui_view_style_t *s = self->styles[i];
        if (s == NULL || s->background == NULL)
        {
            continue;
        }
#if EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
        if (s->state_mask != 0)
        {
            uint8_t cur = egui_view_get_computed_state(self);
            if ((cur & s->state_mask) != s->state_mask)
            {
                continue;
            }
        }
#endif
        return s->background;
    }
    return NULL;
}

egui_alpha_t egui_view_get_effective_alpha(egui_view_t *self)
{
    int i;

    if (self == NULL)
    {
        return EGUI_ALPHA_100;
    }
    /* If alpha was explicitly set on this view, it takes priority. */
    if (self->has_own_alpha)
    {
        return self->alpha;
    }
    /* Walk styles from highest to lowest priority. */
    for (i = (int)self->style_count - 1; i >= 0; i--)
    {
        const egui_view_style_t *s = self->styles[i];
        if (s == NULL || !s->has_alpha)
        {
            continue;
        }
#if EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
        if (s->state_mask != 0)
        {
            uint8_t cur = egui_view_get_computed_state(self);
            if ((cur & s->state_mask) != s->state_mask)
            {
                continue;
            }
        }
#endif
        return s->alpha;
    }
    return self->alpha; /* default EGUI_ALPHA_100 set in init */
}

#endif /* EGUI_CONFIG_FUNCTION_STYLE_CASCADE */

#if EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
uint8_t egui_view_get_computed_state(const egui_view_t *self)
{
    uint8_t state = 0;
    if (self == NULL)
    {
        return 0;
    }
    if (self->is_pressed)
    {
        state |= (uint8_t)EGUI_VIEW_STATE_PRESSED;
    }
    if (!self->is_enable)
    {
        state |= (uint8_t)EGUI_VIEW_STATE_DISABLED;
    }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (self->is_focused)
    {
        state |= (uint8_t)EGUI_VIEW_STATE_FOCUSED;
    }
#endif
    state |= self->view_state; /* user-settable bits (e.g. CHECKED) */
    return state;
}

void egui_view_set_state_checked(egui_view_t *self, int checked)
{
    if (self == NULL)
    {
        return;
    }
    if (checked)
    {
        self->view_state |= (uint8_t)EGUI_VIEW_STATE_CHECKED;
    }
    else
    {
        self->view_state &= (uint8_t)(~EGUI_VIEW_STATE_CHECKED);
    }
    egui_view_invalidate_full(self);
}

int egui_view_get_state_checked(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return (self->view_state & (uint8_t)EGUI_VIEW_STATE_CHECKED) != 0;
}
#endif /* EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES */

void egui_view_set_parent(egui_view_t *self, egui_view_group_t *parent)
{
    if (self == NULL)
    {
        return;
    }

    self->parent = parent;
}

egui_view_group_t *egui_view_get_parent(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return self->parent;
}

void egui_view_set_alpha(egui_view_t *self, egui_alpha_t alpha)
{
    if (self == NULL)
    {
        return;
    }

    if (alpha != self->alpha)
    {
        self->alpha = alpha;
#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE
        self->has_own_alpha = 1;
#endif
        egui_view_invalidate_visual_region(self);
    }
#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE
    else
    {
        /* Mark even when value unchanged so cascade knows the field was set. */
        self->has_own_alpha = 1;
    }
#endif
}

egui_alpha_t egui_view_get_alpha(egui_view_t *self)
{
    if (self == NULL)
    {
        return EGUI_ALPHA_100;
    }
    return self->alpha;
}

// TODO: need get raw pos static.
void egui_view_get_raw_pos(egui_view_t *self, egui_location_t *location)
{
    if (self == NULL || location == NULL)
    {
        return;
    }

    location->x += self->region.location.x;
    location->y += self->region.location.y;

    // recursion implement
    // if(self->parent)
    // {
    //     egui_view_get_raw_pos(self->parent, location);
    // }

    // implement without recursion
    egui_view_t *p = (egui_view_t *)self->parent;
    while (p)
    {
        location->x += p->region.location.x;
        location->y += p->region.location.y;

        p = (egui_view_t *)p->parent;
    }
}

static void egui_view_clip_to_visible_ancestors(egui_view_t *self, egui_region_t *clip)
{
    egui_view_t *p_clip;

    if (self == NULL || clip == NULL)
    {
        return;
    }

    p_clip = (egui_view_t *)self->parent;

    while (p_clip != NULL && !egui_region_is_empty(clip))
    {
        egui_region_intersect(clip, &p_clip->region_screen, clip);
        p_clip = (egui_view_t *)p_clip->parent;
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static egui_dim_t egui_view_get_focus_frame_expand(egui_view_t *self)
{
    if (self == NULL || !self->is_focus_frame_visible || self->focus_frame_stroke <= 0)
    {
        return 0;
    }

    return self->focus_frame_margin + self->focus_frame_stroke;
}

void egui_view_get_focus_frame_region(egui_view_t *self, egui_region_t *region)
{
    egui_dim_t expand;
    egui_core_t *core;

    if (region == NULL)
    {
        return;
    }

    expand = egui_view_get_focus_frame_expand(self);
    if (self == NULL || expand <= 0 || egui_region_is_empty(&self->region_screen))
    {
        egui_region_init_empty(region);
        return;
    }

    region->location.x = self->region_screen.location.x - expand;
    region->location.y = self->region_screen.location.y - expand;
    region->size.width = self->region_screen.size.width + (expand * 2);
    region->size.height = self->region_screen.size.height + (expand * 2);

    core = egui_view_get_core(self);
    if (core != NULL)
    {
        EGUI_REGION_DEFINE(screen_region, 0, 0, core->screen_width, core->screen_height);
        egui_region_intersect(region, &screen_region, region);
    }
}

void egui_view_invalidate_focus_region(egui_view_t *self)
{
    egui_core_t *core;
    egui_region_t dirty_clip;

    if (self == NULL || !egui_view_is_visible(self) || self->is_gone)
    {
        return;
    }

    egui_view_get_focus_frame_region(self, &dirty_clip);
    egui_view_clip_to_visible_ancestors(self, &dirty_clip);
    if (egui_region_is_empty(&dirty_clip))
    {
        return;
    }

    core = egui_view_get_core(self);
    if (core != NULL)
    {
        self->last_dirty_epoch = egui_core_get_dirty_epoch(core);
    }
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
    egui_view_log_dirty_source("focus_frame", self, &dirty_clip);
#endif
    egui_view_update_region_dirty(self, &dirty_clip);
}
#endif

static void egui_view_invalidate_visible_self_region(egui_view_t *self)
{
    egui_region_t dirty_clip;

    if (self == NULL || !egui_view_is_visible(self) || self->is_gone)
    {
        return;
    }

    egui_region_copy(&dirty_clip, &self->region_screen);
    egui_view_clip_to_visible_ancestors(self, &dirty_clip);
    if (!egui_region_is_empty(&dirty_clip))
    {
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
        egui_view_log_dirty_source("visible_tree", self, &dirty_clip);
#endif
        egui_view_update_region_dirty(self, &dirty_clip);
    }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (self->is_focused)
    {
        egui_view_invalidate_focus_region(self);
    }
#endif
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
static int egui_view_is_dirty_passthrough_group(egui_view_t *self)
{
    return self != NULL && self->api != NULL && self->is_dirty_passthrough && self->api->calculate_layout == egui_view_group_calculate_layout;
}
#endif

static void egui_view_invalidate_visible_tree_internal(egui_view_t *self)
{
    if (self == NULL || !egui_view_is_visible(self) || self->is_gone)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
    if (egui_view_is_dirty_passthrough_group(self) &&
#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE
        egui_view_get_effective_background(self) == NULL
#else
        self->background == NULL
#endif
    )
    {
        egui_view_group_t *group = (egui_view_group_t *)self;
        egui_dnode_t *p_head;

        if (!egui_dlist_is_empty(&group->childs))
        {
            EGUI_DLIST_FOR_EACH_NODE(&group->childs, p_head)
            {
                egui_view_t *child = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);
                egui_view_invalidate_visible_tree_internal(child);
            }
        }
        return;
    }
#endif

    egui_view_invalidate_visible_self_region(self);
}

void egui_view_invalidate_visible_tree(egui_view_t *self)
{
    egui_view_invalidate_visible_tree_internal(self);
}

void egui_view_layout(egui_view_t *self, egui_region_t *region)
{
    int changed;

    if (self == NULL || region == NULL)
    {
        return;
    }

    changed = !egui_region_equal(&self->region, region);

    /* The view's old region_screen may extend outside the visible viewport
     * (e.g. a scrolled container with negative top). Clip it against every
     * ancestor's region_screen before marking dirty so the dirty area stays
     * bounded by the actual viewport instead of inflating to full screen. */
#if EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
    if (!self->is_dirty_passthrough)
#endif
    {
        egui_region_t dirty_clip;
        egui_region_copy(&dirty_clip, &self->region_screen);
        egui_view_clip_to_visible_ancestors(self, &dirty_clip);
        egui_view_update_region_dirty(self, &dirty_clip);
    }

    // update region
    egui_region_copy(&self->region, region);

#if EGUI_CONFIG_FUNCTION_EVENT_LITE
    if (changed)
    {
        egui_view_send_event(self, EGUI_EVENT_LAYOUT_CHANGED, &self->region);
    }
#endif

    // EGUI_LOG_DBG("region_dirty new: %d %d %d %d\n", self->region_dirty.location.x, self->region_dirty.location.y, self->region_dirty.size.width,
    // self->region_dirty.size.height);

    egui_view_invalidate(self);
}

void egui_view_scroll_to(egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_region_t region;

    if (self == NULL)
    {
        return;
    }

    egui_region_copy(&region, &self->region);

    region.location.x = x;
    region.location.y = y;

    egui_view_layout(self, &region);
}

void egui_view_scroll_by(egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_region_t region;

    if (self == NULL)
    {
        return;
    }

    egui_region_copy(&region, &self->region);

    region.location.x += x;
    region.location.y += y;

    egui_view_layout(self, &region);
}

void egui_view_get_work_region(egui_view_t *self, egui_region_t *region)
{
    if (self == NULL || region == NULL)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
    region->location.x = self->padding.left;
    region->location.y = self->padding.top;
    region->size.width = self->region.size.width - (self->padding.left + self->padding.right);
    region->size.height = self->region.size.height - (self->padding.top + self->padding.bottom);
#else
    region->location.x = 0;
    region->location.y = 0;
    region->size.width = self->region.size.width;
    region->size.height = self->region.size.height;
#endif
}

void egui_view_copy_api(egui_view_t *self, egui_view_api_t *api)
{
    if (self == NULL || api == NULL || self->api == NULL)
    {
        return;
    }

    *api = *self->api;
    self->api = api;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
void egui_view_override_api_on_touch(egui_view_t *self, egui_view_api_t *api, egui_view_on_touch_listener_t listener)
{
    if (self == NULL || api == NULL || self->api == NULL)
    {
        return;
    }

    egui_view_copy_api(self, api);
    api->on_touch = listener;
}
#endif

void egui_view_set_on_click_listener(egui_view_t *self, egui_view_on_click_listener_t listener)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    if (self == NULL)
    {
        return;
    }

    self->on_click_listener = listener;
    self->is_clickable = true;
    egui_view_invalidate(self);
#else
    EGUI_UNUSED(listener);
    EGUI_UNUSED(self);
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
}

egui_view_on_click_listener_t egui_view_get_on_click_listener(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    if (self == NULL)
    {
        return NULL;
    }

    return self->on_click_listener;
#else
    EGUI_UNUSED(self);
    return NULL;
#endif
}

#if EGUI_CONFIG_FUNCTION_LONG_PRESS && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
void egui_view_set_on_long_press_listener(egui_view_t *self, egui_view_on_long_press_listener_t listener)
{
    if (self == NULL)
    {
        return;
    }
    self->on_long_press_listener = listener;
}

egui_view_on_long_press_listener_t egui_view_get_on_long_press_listener(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return self->on_long_press_listener;
}

void egui_view_poll_long_press(egui_view_t *self)
{
    uint32_t elapsed;

    if (self == NULL || !self->is_pressed || self->on_long_press_listener == NULL || !self->_lp_active || self->_lp_fired)
    {
        return;
    }
    elapsed = egui_timer_get_current_time() - self->_lp_press_tick;
    if (elapsed >= EGUI_CONFIG_LONG_PRESS_DURATION_MS)
    {
        self->_lp_fired = 1;
        self->on_long_press_listener(self);
    }
}
#endif /* EGUI_CONFIG_FUNCTION_LONG_PRESS */

#if EGUI_CONFIG_FUNCTION_SWIPE_LISTENER && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
void egui_view_set_on_swipe_listener(egui_view_t *self, egui_view_on_swipe_listener_t listener)
{
    if (self == NULL)
    {
        return;
    }
    self->on_swipe_listener = listener;
    self->is_clickable = (listener != NULL) || (self->on_click_listener != NULL);
}

egui_view_on_swipe_listener_t egui_view_get_on_swipe_listener(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return self->on_swipe_listener;
}
#endif /* EGUI_CONFIG_FUNCTION_SWIPE_LISTENER */

void egui_view_set_enable(egui_view_t *self, int is_enable)
{
    if (self == NULL)
    {
        return;
    }

    self->is_enable = is_enable;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (!is_enable)
    {
        egui_view_clear_focus_if_subtree_unfocusable(self);
    }
#endif

    egui_view_invalidate(self);
}

int egui_view_get_enable(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }

    return self->is_enable;
}

void egui_view_set_clickable(egui_view_t *self, int is_clickable)
{
    if (self == NULL)
    {
        return;
    }

    self->is_clickable = is_clickable;
}

int egui_view_get_clickable(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }

    return self->is_clickable;
}

void egui_view_set_position(egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    if (self == NULL)
    {
        return;
    }

    if (self->region.location.x == x && self->region.location.y == y)
    {
        return;
    }

    self->region.location.x = x;
    self->region.location.y = y;

#if EGUI_CONFIG_FUNCTION_EVENT_LITE
    egui_view_send_event(self, EGUI_EVENT_LAYOUT_CHANGED, &self->region);
#endif

    egui_view_invalidate(self);
}

egui_dim_t egui_view_get_x(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->region.location.x;
}

egui_dim_t egui_view_get_y(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->region.location.y;
}

void egui_view_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height)
{
    if (self == NULL)
    {
        return;
    }

    if (self->region.size.width == width && self->region.size.height == height)
    {
        return;
    }

    self->region.size.width = width;
    self->region.size.height = height;

#if EGUI_CONFIG_FUNCTION_EVENT_LITE
    egui_view_send_event(self, EGUI_EVENT_SIZE_CHANGED, &self->region.size);
#endif

    egui_view_invalidate(self);
}

egui_dim_t egui_view_get_width(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->region.size.width;
}

egui_dim_t egui_view_get_height(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->region.size.height;
}

egui_dim_t egui_view_get_content_width(egui_view_t *self)
{
    egui_dim_t w;
    egui_dim_t pad;

    if (self == NULL)
    {
        return 0;
    }
    w = self->region.size.width;
    pad = (egui_dim_t)(self->padding.left + self->padding.right);
    return (w > pad) ? (egui_dim_t)(w - pad) : 0;
}

egui_dim_t egui_view_get_content_height(egui_view_t *self)
{
    egui_dim_t h;
    egui_dim_t pad;

    if (self == NULL)
    {
        return 0;
    }
    h = self->region.size.height;
    pad = (egui_dim_t)(self->padding.top + self->padding.bottom);
    return (h > pad) ? (egui_dim_t)(h - pad) : 0;
}

egui_dim_t egui_view_get_screen_x(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->region_screen.location.x;
}

egui_dim_t egui_view_get_screen_y(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->region_screen.location.y;
}

void egui_view_set_pressed(egui_view_t *self, int is_pressed)
{
    if (self == NULL)
    {
        return;
    }

    if (self->is_pressed == is_pressed)
    {
        return;
    }

    self->is_pressed = is_pressed;

#if EGUI_CONFIG_FUNCTION_EVENT_LITE
    egui_view_send_event(self, is_pressed ? EGUI_EVENT_PRESSED : EGUI_EVENT_RELEASED, NULL);
#endif

    egui_view_invalidate_full_region(self);
}

int egui_view_set_pressed_with_region(egui_view_t *self, int is_pressed, const egui_region_t *dirty_region)
{
    if (self == NULL)
    {
        return 0;
    }

    if (self->is_pressed == is_pressed)
    {
        return 0;
    }

    self->is_pressed = is_pressed;

#if EGUI_CONFIG_FUNCTION_EVENT_LITE
    egui_view_send_event(self, is_pressed ? EGUI_EVENT_PRESSED : EGUI_EVENT_RELEASED, NULL);
#endif

    if (egui_view_background_has_pressed_param(self))
    {
        egui_view_invalidate_full_region(self);
    }
    else if (dirty_region != NULL)
    {
        egui_view_invalidate_region(self, dirty_region);
    }

    return 1;
}

int egui_view_get_pressed(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }

    return self->is_pressed;
}

void egui_view_set_visible(egui_view_t *self, int is_visible)
{
    if (self == NULL)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (!is_visible)
    {
        egui_view_invalidate_focused_descendant_region(self);
    }
#endif
    if (is_visible == self->is_visible)
    {
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (!is_visible)
        {
            egui_view_clear_focus_if_subtree_unfocusable(self);
        }
#endif
        return;
    }
    // avoid self change to invisible.
    egui_view_invalidate_visual_region(self);

    self->is_visible = is_visible;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (!is_visible)
    {
        egui_view_clear_focus_if_subtree_unfocusable(self);
    }
#endif

    // avoid self change to invisible.
    egui_view_invalidate_visual_region(self);
}

int egui_view_get_visible(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }

    return self->is_visible;
}

void egui_view_set_dirty_passthrough(egui_view_t *self, int on)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
    if (self == NULL)
    {
        return;
    }

    self->is_dirty_passthrough = on ? 1 : 0;
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(on);
#endif
}

int egui_view_get_dirty_passthrough(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
    if (self == NULL)
    {
        return 0;
    }

    return self->is_dirty_passthrough;
#else
    EGUI_UNUSED(self);
    return 0;
#endif
}

void egui_view_set_gone(egui_view_t *self, int is_gone)
{
    if (self == NULL)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (is_gone)
    {
        egui_view_invalidate_focused_descendant_region(self);
    }
#endif
    if (is_gone == self->is_gone)
    {
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (is_gone)
        {
            egui_view_clear_focus_if_subtree_unfocusable(self);
        }
#endif
        return;
    }

    // avoid self change to invisible.
    egui_view_invalidate(self);

    self->is_gone = is_gone;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (is_gone)
    {
        egui_view_clear_focus_if_subtree_unfocusable(self);
    }
#endif

    // avoid self change to invisible.
    egui_view_invalidate(self);
}

int egui_view_get_gone(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }

    return self->is_gone;
}

#if EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA
void egui_view_set_ext_click_area(egui_view_t *self, uint8_t extra_px)
{
    if (self == NULL)
    {
        return;
    }
    self->ext_click_area = extra_px;
}

uint8_t egui_view_get_ext_click_area(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->ext_click_area;
}
#endif /* EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA */

#if EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
void egui_view_set_draggable(egui_view_t *self, int is_draggable)
{
    if (self == NULL)
    {
        return;
    }
    self->is_draggable = is_draggable ? 1 : 0;
}

int egui_view_get_draggable(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->is_draggable;
}
#endif /* EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW */

void egui_view_set_padding(egui_view_t *self, egui_dim_margin_padding_t left, egui_dim_margin_padding_t right, egui_dim_margin_padding_t top,
                           egui_dim_margin_padding_t bottom)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
    if (self == NULL)
    {
        return;
    }

    self->padding.left = left;
    self->padding.right = right;
    self->padding.top = top;
    self->padding.bottom = bottom;

    egui_view_invalidate(self);
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(left);
    EGUI_UNUSED(right);
    EGUI_UNUSED(top);
    EGUI_UNUSED(bottom);
#endif
}

void egui_view_set_padding_all(egui_view_t *self, egui_dim_margin_padding_t padding)
{
    egui_view_set_padding(self, padding, padding, padding, padding);
}

egui_dim_margin_padding_t egui_view_get_padding_left(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
    return (self != NULL) ? self->padding.left : 0;
#else
    EGUI_UNUSED(self);
    return 0;
#endif
}

egui_dim_margin_padding_t egui_view_get_padding_right(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
    return (self != NULL) ? self->padding.right : 0;
#else
    EGUI_UNUSED(self);
    return 0;
#endif
}

egui_dim_margin_padding_t egui_view_get_padding_top(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
    return (self != NULL) ? self->padding.top : 0;
#else
    EGUI_UNUSED(self);
    return 0;
#endif
}

egui_dim_margin_padding_t egui_view_get_padding_bottom(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
    return (self != NULL) ? self->padding.bottom : 0;
#else
    EGUI_UNUSED(self);
    return 0;
#endif
}

void egui_view_set_margin(egui_view_t *self, egui_dim_margin_padding_t left, egui_dim_margin_padding_t right, egui_dim_margin_padding_t top,
                          egui_dim_margin_padding_t bottom)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
    if (self == NULL)
    {
        return;
    }

    self->margin.left = left;
    self->margin.right = right;
    self->margin.top = top;
    self->margin.bottom = bottom;

    egui_view_invalidate(self);
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(left);
    EGUI_UNUSED(right);
    EGUI_UNUSED(top);
    EGUI_UNUSED(bottom);
#endif
}

void egui_view_set_margin_all(egui_view_t *self, egui_dim_margin_padding_t margin)
{
    egui_view_set_margin(self, margin, margin, margin, margin);
}

egui_dim_margin_padding_t egui_view_get_margin_left(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
    return (self != NULL) ? self->margin.left : 0;
#else
    EGUI_UNUSED(self);
    return 0;
#endif
}

egui_dim_margin_padding_t egui_view_get_margin_right(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
    return (self != NULL) ? self->margin.right : 0;
#else
    EGUI_UNUSED(self);
    return 0;
#endif
}

egui_dim_margin_padding_t egui_view_get_margin_top(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
    return (self != NULL) ? self->margin.top : 0;
#else
    EGUI_UNUSED(self);
    return 0;
#endif
}

egui_dim_margin_padding_t egui_view_get_margin_bottom(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
    return (self != NULL) ? self->margin.bottom : 0;
#else
    EGUI_UNUSED(self);
    return 0;
#endif
}

void egui_view_align_to_parent(egui_view_t *self, uint8_t align_type, egui_dim_t offset_x, egui_dim_t offset_y)
{
    egui_view_t *parent;
    egui_dim_t parent_width;
    egui_dim_t parent_height;
    egui_dim_t x;
    egui_dim_t y;

    if (self == NULL)
    {
        return;
    }

    parent = (egui_view_t *)self->parent;
    if (parent != NULL)
    {
        parent_width = egui_view_get_content_width(parent);
        parent_height = egui_view_get_content_height(parent);
    }
    else if (self->core != NULL)
    {
        parent_width = self->core->screen_width;
        parent_height = self->core->screen_height;
    }
    else
    {
        parent_width = self->region.size.width;
        parent_height = self->region.size.height;
    }

    egui_common_align_get_x_y(parent_width, parent_height, self->region.size.width, self->region.size.height, align_type, &x, &y);
    egui_view_set_position(self, (egui_dim_t)(x + offset_x), (egui_dim_t)(y + offset_y));
}

#if EGUI_CONFIG_FUNCTION_FLEXLAYOUT
void egui_view_set_flex_grow(egui_view_t *self, uint8_t grow)
{
    if (self == NULL)
    {
        return;
    }
    self->flex_grow = grow;
}

uint8_t egui_view_get_flex_grow(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    return self->flex_grow;
}
#endif

void egui_view_set_shadow(egui_view_t *self, const egui_shadow_t *shadow)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
    if (self == NULL)
    {
        return;
    }

    if (self->shadow == shadow)
    {
        return;
    }

    self->shadow = shadow;
    egui_view_invalidate_visual_region(self);
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(shadow);
#endif
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
void egui_view_set_focus_frame_visible(egui_view_t *self, int is_visible)
{
    if (self == NULL)
    {
        return;
    }

    if (self->is_focus_frame_visible == (is_visible ? 1 : 0))
    {
        return;
    }

    egui_view_invalidate_focus_region(self);
    self->is_focus_frame_visible = is_visible ? 1 : 0;
    egui_view_invalidate_focus_region(self);
}

int egui_view_get_focus_frame_visible(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }

    return self->is_focus_frame_visible;
}

void egui_view_set_focus_frame_style(egui_view_t *self, egui_dim_t margin, egui_dim_t stroke, egui_color_t color, egui_alpha_t alpha)
{
    if (self == NULL)
    {
        return;
    }

    if (self->focus_frame_margin == margin && self->focus_frame_stroke == stroke && self->focus_frame_color.full == color.full &&
        self->focus_frame_alpha == alpha)
    {
        return;
    }

    egui_view_invalidate_focus_region(self);
    self->focus_frame_margin = margin;
    self->focus_frame_stroke = stroke;
    self->focus_frame_color = color;
    self->focus_frame_alpha = alpha;
    egui_view_invalidate_focus_region(self);
}

void egui_view_get_focus_frame_style(egui_view_t *self, egui_dim_t *margin, egui_dim_t *stroke, egui_color_t *color, egui_alpha_t *alpha)
{
    if (margin != NULL)
    {
        *margin = (self != NULL) ? self->focus_frame_margin : 0;
    }
    if (stroke != NULL)
    {
        *stroke = (self != NULL) ? self->focus_frame_stroke : 0;
    }
    if (color != NULL)
    {
        *color = (self != NULL) ? self->focus_frame_color : EGUI_THEME_FOCUS;
    }
    if (alpha != NULL)
    {
        *alpha = (self != NULL) ? self->focus_frame_alpha : EGUI_ALPHA_100;
    }
}
#endif

#if EGUI_CONFIG_DEBUG_CLASS_NAME
void egui_view_set_view_name(egui_view_t *self, const char *name)
{
    if (self == NULL)
    {
        return;
    }

    self->name = name;
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
int egui_view_perform_click(egui_view_t *self)
{
    int is_handled = 0;
    egui_view_on_click_listener_t listener;

    if (self == NULL)
    {
        return 0;
    }

    listener = self->on_click_listener;

    if (self->api != NULL && self->api->perform_click != NULL)
    {
        is_handled = self->api->perform_click(self);
    }

    if (listener != NULL)
    {
        listener(self);
        is_handled = 1;
    }

#if EGUI_CONFIG_FUNCTION_EVENT_LITE
    egui_view_send_event(self, EGUI_EVENT_CLICKED, NULL);
#endif

    return is_handled;
}
#else
int egui_view_perform_click(egui_view_t *self)
{
    EGUI_UNUSED(self);
    return 0;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
int egui_view_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(event);
    EGUI_UNUSED(self);
    // view object should not work here. just return 0.
    // EGUI_LOG_DBG("egui_view_on_intercept_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));

    return 0;
}

int egui_view_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    // EGUI_LOG_DBG("egui_view_dispatch_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));

    if (self == NULL || event == NULL || self->api == NULL)
    {
        return 0;
    }

    if (self->is_enable && self->api->on_touch != NULL && self->api->on_touch(self, event))
    {
        return 1;
    }

    return (self->api->on_touch_event != NULL) ? self->api->on_touch_event(self, event) : 0;
}

int egui_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    // EGUI_LOG_DBG("egui_view_on_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));
    int is_inside;

    if (self == NULL || event == NULL)
    {
        return 0;
    }

    is_inside = egui_view_hit_test(self, event->location.x, event->location.y);

    if (self->is_enable == false)
    {
        if (event->type == EGUI_MOTION_EVENT_ACTION_UP || event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
        {
            egui_view_set_pressed(self, false);
        }
        // A disabled view that is clickable still consumes the touch
        // events, it just doesn't respond to them.
        return (self->is_clickable);
    }

#if EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW
    if (self->is_draggable)
    {
        switch (event->type)
        {
        case EGUI_MOTION_EVENT_ACTION_DOWN:
            if (is_inside)
            {
                self->_drag_last_x = event->location.x;
                self->_drag_last_y = event->location.y;
                self->_drag_tracking = 1;
            }
            return is_inside;
        case EGUI_MOTION_EVENT_ACTION_MOVE:
            if (self->_drag_tracking)
            {
                egui_dim_t dx = (egui_dim_t)(event->location.x - self->_drag_last_x);
                egui_dim_t dy = (egui_dim_t)(event->location.y - self->_drag_last_y);
                self->_drag_last_x = event->location.x;
                self->_drag_last_y = event->location.y;
                if (dx != 0 || dy != 0)
                {
                    egui_region_t new_region;
                    egui_region_copy(&new_region, &self->region);
                    new_region.location.x = (egui_dim_t)(new_region.location.x + dx);
                    new_region.location.y = (egui_dim_t)(new_region.location.y + dy);
                    egui_view_layout(self, &new_region);
                }
                return 1;
            }
            return 0;
        case EGUI_MOTION_EVENT_ACTION_UP:
        case EGUI_MOTION_EVENT_ACTION_CANCEL:
            self->_drag_tracking = 0;
            return 1;
        default:
            break;
        }
    }
#endif /* EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW */

    if (self->is_clickable)
    {
        switch (event->type)
        {
        case EGUI_MOTION_EVENT_ACTION_UP:
        {
            int should_click = self->is_pressed && is_inside;

#if EGUI_CONFIG_FUNCTION_LONG_PRESS
            if (self->_lp_fired)
            {
                should_click = 0; /* long-press already dispatched; suppress the click */
            }
            self->_lp_active = 0;
            self->_lp_fired = 0;
#endif
            egui_view_set_pressed(self, false);
            if (should_click)
            {
                egui_view_perform_click(self);
            }
#if EGUI_CONFIG_FUNCTION_SWIPE_LISTENER
            if (self->on_swipe_listener != NULL)
            {
                egui_dim_t dx = (egui_dim_t)(event->location.x - self->_swipe_down_x);
                egui_dim_t dy = (egui_dim_t)(event->location.y - self->_swipe_down_y);
                egui_dim_t abs_dx = dx < 0 ? (egui_dim_t)(-dx) : dx;
                egui_dim_t abs_dy = dy < 0 ? (egui_dim_t)(-dy) : dy;

                if (abs_dx >= EGUI_CONFIG_SWIPE_MIN_DISPLACEMENT_PX || abs_dy >= EGUI_CONFIG_SWIPE_MIN_DISPLACEMENT_PX)
                {
                    egui_swipe_dir_t dir;
                    if (abs_dx >= abs_dy)
                    {
                        dir = dx < 0 ? EGUI_SWIPE_DIR_LEFT : EGUI_SWIPE_DIR_RIGHT;
                    }
                    else
                    {
                        dir = dy < 0 ? EGUI_SWIPE_DIR_UP : EGUI_SWIPE_DIR_DOWN;
                    }
                    self->on_swipe_listener(self, dir);
                }
            }
#endif /* EGUI_CONFIG_FUNCTION_SWIPE_LISTENER */
            break;
        }
        case EGUI_MOTION_EVENT_ACTION_DOWN:
            egui_view_set_pressed(self, is_inside);
#if EGUI_CONFIG_FUNCTION_LONG_PRESS
            if (is_inside)
            {
                self->_lp_press_tick = egui_timer_get_current_time();
                self->_lp_fired = 0;
                self->_lp_active = 1;
            }
#endif
#if EGUI_CONFIG_FUNCTION_SWIPE_LISTENER
            if (is_inside)
            {
                self->_swipe_down_x = event->location.x;
                self->_swipe_down_y = event->location.y;
            }
#endif /* EGUI_CONFIG_FUNCTION_SWIPE_LISTENER */
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
            if (is_inside && self->is_focusable)
            {
                egui_view_request_focus(self);
            }
            else if (is_inside && !self->is_no_focus_clear)
            {
                // Clear focus when a non-focusable widget is touched
                // (e.g. dismiss on-screen keyboard when tapping other controls).
                // Skip if is_no_focus_clear is set (e.g. keyboard keys must not dismiss the keyboard).
                egui_view_clear_focus(self);
            }
#endif
            break;
        case EGUI_MOTION_EVENT_ACTION_MOVE:
            if (self->is_pressed != is_inside)
            {
                egui_view_set_pressed(self, is_inside);
#if EGUI_CONFIG_FUNCTION_LONG_PRESS
                if (!is_inside)
                {
                    /* Finger slid out — cancel the pending long-press. */
                    self->_lp_active = 0;
                    self->_lp_fired = 0;
                }
#endif
            }
            break;
        case EGUI_MOTION_EVENT_ACTION_CANCEL:
            egui_view_set_pressed(self, false);
#if EGUI_CONFIG_FUNCTION_LONG_PRESS
            self->_lp_active = 0;
            self->_lp_fired = 0;
#endif
            break;
        default:
            break;
        }

        // if view clickable, return 1 to stop dispatch touch event to parent.
        return 1;
    }

    return 0;
}
#else
int egui_view_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(event);
    EGUI_UNUSED(self);
    return 0;
}

int egui_view_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(event);
    EGUI_UNUSED(self);
    return 0;
}

int egui_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(event);
    EGUI_UNUSED(self);
    return 0;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

void egui_view_dispatch_attach_to_window(egui_view_t *self)
{
    if (self == NULL || self->is_attached_to_window)
    {
        return;
    }

    self->is_attached_to_window = 1;
    if (self->api != NULL && self->api->on_attach_to_window != NULL)
    {
        self->api->on_attach_to_window(self);
    }
}

void egui_view_dispatch_detach_from_window(egui_view_t *self)
{
    if (self == NULL || !self->is_attached_to_window)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (self->is_focused)
    {
        egui_view_clear_focus(self);
    }
#endif

    if (self->api != NULL && self->api->on_detach_from_window != NULL)
    {
        self->api->on_detach_from_window(self);
    }
    self->is_attached_to_window = 0;
}

void egui_view_on_attach_to_window(egui_view_t *self)
{
    EGUI_UNUSED(self);
    // EGUI_LOG_DBG("on_attach_to_window %d\n", self->id);
}

void egui_view_on_draw(egui_view_t *self)
{
    EGUI_UNUSED(self);
    // EGUI_LOG_DBG("on_draw %d\n", self->id);
}

void egui_view_on_detach_from_window(egui_view_t *self)
{
    EGUI_UNUSED(self);
    // EGUI_LOG_DBG("on_detach_from_window %d\n", self->id);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void egui_view_draw_focus_frame(egui_view_t *self)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_region_t frame_region;

    if (self == NULL || canvas == NULL || !self->is_focused)
    {
        return;
    }

    egui_view_get_focus_frame_region(self, &frame_region);
    if (egui_region_is_empty(&frame_region))
    {
        return;
    }

    egui_canvas_calc_work_region(canvas, &frame_region);
    if (!egui_region_is_empty(egui_canvas_get_base_view_work_region(canvas)))
    {
        if (self->api != NULL && self->api->on_draw_focus_frame != NULL)
        {
            self->api->on_draw_focus_frame(self, &frame_region);
        }
        else
        {
            egui_canvas_draw_rectangle(canvas, 0, 0, frame_region.size.width, frame_region.size.height, self->focus_frame_stroke, self->focus_frame_color,
                                       self->focus_frame_alpha);
        }
    }
}
#endif

void egui_view_draw(egui_view_t *self)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_alpha_t alpha;

    if (self == NULL || canvas == NULL || self->api == NULL)
    {
        return;
    }

    alpha = egui_canvas_get_alpha(canvas);

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    egui_activity_t *activity = egui_view_get_activity(self);
    if (activity)
    {
#if EGUI_CONFIG_DEBUG_VIEW_ID
        EGUI_LOG_DBG("draw view id: %02d, visible: %d, name: %s, activity: %s\n", self->id, self->is_visible, self->name, activity->name);
#else
        EGUI_LOG_DBG("draw view visible: %d, name: %s, activity: %s\n", self->is_visible, self->name, activity->name);
#endif
    }
    else
    {
#if EGUI_CONFIG_DEBUG_VIEW_ID
        EGUI_LOG_DBG("draw view id: %02d, visible: %d, name: %s\n", self->id, self->is_visible, self->name);
#else
        EGUI_LOG_DBG("draw view visible: %d, name: %s\n", self->is_visible, self->name);
#endif
    }
#endif

    if (self->is_visible == false || self->is_gone == true)
    {
        return;
    }

    // clear canvas mask
    egui_canvas_clear_mask(canvas);
    // set canvase alpha
#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE
    egui_canvas_mix_alpha(canvas, egui_view_get_effective_alpha(self));
#else
    egui_canvas_mix_alpha(canvas, self->alpha);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
    // draw shadow with expanded work region (shadow extends beyond view bounds)
    if (self->shadow != NULL)
    {
        egui_region_t shadow_region;
        egui_shadow_get_region(self->shadow, &self->region_screen, &shadow_region);
        egui_canvas_calc_work_region(canvas, &shadow_region);
        if (!egui_region_is_empty(egui_canvas_get_base_view_work_region(canvas)))
        {
            egui_shadow_draw(canvas, self->shadow, &self->region_screen);
        }
    }
#endif

    // For fast drawing, we only draw the region that is intersected with the canvas.
    egui_canvas_calc_work_region(canvas, &self->region_screen);

    if (!egui_region_is_empty(egui_canvas_get_base_view_work_region(canvas)))
    {
        // draw background
        egui_view_draw_background(self);
        // call on_draw
        if (self->api->on_draw != NULL)
        {
            self->api->on_draw(self);
        }
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_draw_focus_frame(self);
#endif

    // restore canvas alpha
    egui_canvas_set_alpha(canvas, alpha);
}

void egui_view_request_layout(egui_view_t *self)
{
    if (self == NULL)
    {
        return;
    }

    self->is_request_layout = true;
}

void egui_view_compute_scroll(egui_view_t *self)
{
    EGUI_UNUSED(self);
    // work in child process.
}

/* Recompute self->region_screen from parent layout without emitting dirty. */
static void egui_view_recompute_region_screen_silent(egui_view_t *self)
{
    egui_region_t *p_raw_region;
    egui_view_t *p_parent;

    if (self == NULL)
    {
        return;
    }

    p_raw_region = &self->region_screen;
    p_parent = (egui_view_t *)self->parent;

    if (p_parent)
    {
#if EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING
        egui_region_intersect_with_size(&self->region, p_parent->region_screen.size.width - (p_parent->padding.left + p_parent->padding.right),
                                        p_parent->region_screen.size.height - (p_parent->padding.top + p_parent->padding.bottom), p_raw_region);
        p_raw_region->location.x += p_parent->region_screen.location.x + p_parent->padding.left;
        p_raw_region->location.y += p_parent->region_screen.location.y + p_parent->padding.top;
#else
        egui_region_intersect_with_size(&self->region, p_parent->region_screen.size.width, p_parent->region_screen.size.height, p_raw_region);
        p_raw_region->location.x += p_parent->region_screen.location.x;
        p_raw_region->location.y += p_parent->region_screen.location.y;
#endif
    }
    else
    {
        egui_region_copy(p_raw_region, &self->region);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
static void egui_view_recompute_subtree_silent(egui_view_t *self)
{
    if (self == NULL)
    {
        return;
    }

    egui_view_recompute_region_screen_silent(self);
    self->is_request_layout = false;

    if (self->api != NULL && self->api->calculate_layout == egui_view_group_calculate_layout)
    {
        egui_view_group_t *group = (egui_view_group_t *)self;
        egui_dnode_t *p_head;

        if (!egui_dlist_is_empty(&group->childs))
        {
            EGUI_DLIST_FOR_EACH_NODE(&group->childs, p_head)
            {
                egui_view_t *child = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);
                egui_view_recompute_subtree_silent(child);
            }
        }
    }
}

static void egui_view_recompute_children_silent(egui_view_t *self)
{
    if (self == NULL || self->api == NULL)
    {
        return;
    }

    if (self->api->calculate_layout == egui_view_group_calculate_layout)
    {
        egui_view_group_t *group = (egui_view_group_t *)self;
        egui_dnode_t *p_head;

        if (!egui_dlist_is_empty(&group->childs))
        {
            EGUI_DLIST_FOR_EACH_NODE(&group->childs, p_head)
            {
                egui_view_t *child = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);
                egui_view_recompute_subtree_silent(child);
            }
        }
    }
}

static void egui_view_request_children_layout(egui_view_t *self)
{
    egui_view_group_t *group;
    egui_dnode_t *p_head;

    if (self == NULL || self->api == NULL || self->api->calculate_layout != egui_view_group_calculate_layout)
    {
        return;
    }

    group = (egui_view_group_t *)self;
    if (!egui_dlist_is_empty(&group->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&group->childs, p_head)
        {
            egui_view_t *child = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);
            if (child != NULL && child->api != NULL && child->api->request_layout != NULL)
            {
                child->api->request_layout(child);
            }
        }
    }
}

static void egui_view_emit_swept_region(egui_view_t *self, const egui_region_t *old_rs, const egui_region_t *new_rs, const egui_region_t *clip,
                                        const char *kind)
{
    EGUI_UNUSED(kind);

    egui_region_t swept;
    egui_region_t clipped;
    egui_dim_t x0;
    egui_dim_t y0;
    egui_dim_t x1_old;
    egui_dim_t x1_new;
    egui_dim_t y1_old;
    egui_dim_t y1_new;
    egui_dim_t x1;
    egui_dim_t y1;

    if (self == NULL || old_rs == NULL || new_rs == NULL || clip == NULL || self->is_gone || !self->is_visible)
    {
        return;
    }

    if (egui_region_is_empty((egui_region_t *)old_rs))
    {
        egui_region_copy(&swept, new_rs);
    }
    else if (egui_region_is_empty((egui_region_t *)new_rs))
    {
        egui_region_copy(&swept, old_rs);
    }
    else
    {
        x0 = (old_rs->location.x < new_rs->location.x) ? old_rs->location.x : new_rs->location.x;
        y0 = (old_rs->location.y < new_rs->location.y) ? old_rs->location.y : new_rs->location.y;
        x1_old = old_rs->location.x + old_rs->size.width;
        x1_new = new_rs->location.x + new_rs->size.width;
        y1_old = old_rs->location.y + old_rs->size.height;
        y1_new = new_rs->location.y + new_rs->size.height;
        x1 = (x1_old > x1_new) ? x1_old : x1_new;
        y1 = (y1_old > y1_new) ? y1_old : y1_new;
        egui_region_init(&swept, x0, y0, x1 - x0, y1 - y0);
    }

    egui_region_intersect(&swept, clip, &clipped);
    if (!egui_region_is_empty(&clipped))
    {
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
        egui_view_log_dirty_source(kind, self, &clipped);
#endif
        egui_view_update_region_dirty(self, &clipped);
    }
}

static void egui_view_get_dirty_passthrough_self_clip(egui_view_t *self, egui_region_t *clip)
{
    egui_view_t *parent;
    egui_core_t *core;

    if (self == NULL || clip == NULL)
    {
        return;
    }

    parent = (egui_view_t *)self->parent;
    if (parent != NULL)
    {
        egui_region_copy(clip, &parent->region_screen);
    }
    else
    {
        core = egui_view_get_core(self);
        if (core != NULL)
        {
            egui_region_init(clip, 0, 0, core->screen_width, core->screen_height);
        }
        else
        {
            egui_region_copy(clip, &self->region_screen);
        }
    }

    egui_view_clip_to_visible_ancestors(self, clip);
}

static void egui_view_emit_swept_for_view(egui_view_t *self, const egui_region_t *clip)
{
    egui_region_t old_rs;
    egui_region_t new_rs;
    egui_region_t local_clip;

    if (self == NULL || clip == NULL || self->is_gone || !self->is_visible)
    {
        return;
    }

    egui_region_copy(&local_clip, clip);
    egui_view_clip_to_visible_ancestors(self, &local_clip);

    egui_region_copy(&old_rs, &self->region_screen);
    egui_view_recompute_region_screen_silent(self);
    self->is_request_layout = false;
    egui_region_copy(&new_rs, &self->region_screen);

    if (self->api != NULL && self->is_dirty_passthrough && self->api->calculate_layout == egui_view_group_calculate_layout)
    {
        egui_view_group_t *group = (egui_view_group_t *)self;
        egui_dnode_t *p_head;

        if (self->background != NULL)
        {
            egui_view_recompute_children_silent(self);
            egui_view_emit_swept_region(self, &old_rs, &new_rs, &local_clip, "dirty_passthrough_self");
            return;
        }

        if (!egui_dlist_is_empty(&group->childs))
        {
            EGUI_DLIST_FOR_EACH_NODE(&group->childs, p_head)
            {
                egui_view_t *child = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);
                egui_view_emit_swept_for_view(child, &local_clip);
            }
        }
        return;
    }

    egui_view_recompute_children_silent(self);
    egui_view_emit_swept_region(self, &old_rs, &new_rs, &local_clip, "dirty_passthrough_swept");
}

static void egui_view_emit_swept_per_child(egui_view_t *self, egui_dim_t dx, egui_dim_t dy)
{
    egui_view_group_t *group;
    egui_dnode_t *p_head;
    egui_region_t clip;

    if (self == NULL || self->api == NULL || self->api->calculate_layout != egui_view_group_calculate_layout)
    {
        return;
    }

    group = (egui_view_group_t *)self;
    egui_region_copy(&clip, &self->region_screen);
    egui_view_clip_to_visible_ancestors(self, &clip);

    if (!egui_dlist_is_empty(&group->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&group->childs, p_head)
        {
            egui_view_t *child = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);
            egui_view_emit_swept_for_view(child, &clip);
        }
    }

    EGUI_UNUSED(dx);
    EGUI_UNUSED(dy);
}
#endif

void egui_view_calculate_layout(egui_view_t *self)
{
    if (self == NULL)
    {
        return;
    }

    if (!self->is_request_layout)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
    if (self->is_dirty_passthrough)
    {
        egui_region_t prev;
        egui_region_t now;
        egui_region_t self_clip;
        int prev_empty;
        int bounds_changed;
        egui_dim_t dx;
        egui_dim_t dy;

        egui_region_copy(&prev, &self->region_screen);
        self->is_request_layout = false;
        egui_view_recompute_region_screen_silent(self);
        egui_region_copy(&now, &self->region_screen);

        prev_empty = egui_region_is_empty(&prev);
        bounds_changed = prev_empty || !egui_region_equal(&prev, &now);
        dx = now.location.x - prev.location.x;
        dy = now.location.y - prev.location.y;

        if (self->background != NULL && bounds_changed)
        {
            egui_view_get_dirty_passthrough_self_clip(self, &self_clip);
            egui_view_emit_swept_region(self, &prev, &now, &self_clip, "dirty_passthrough_self");
            egui_view_recompute_children_silent(self);
        }
        else if (!prev_empty && (dx != 0 || dy != 0))
        {
            egui_view_emit_swept_per_child(self, dx, dy);
        }
        else
        {
            egui_view_request_children_layout(self);
        }

        return;
    }
#endif

    self->is_request_layout = false;
    egui_view_recompute_region_screen_silent(self);

    // update dirty region
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
    egui_view_log_dirty_source("layout", self, &self->region_screen);
#endif
    /* Clip dirty against ancestors so scrolled children outside their viewport
     * do not inflate the dirty area. Keep region_screen intact so descendants

     * * can still compute correct draw positions. */
    {
        egui_region_t dirty_clip;
        egui_region_copy(&dirty_clip, &self->region_screen);
        egui_view_clip_to_visible_ancestors(self, &dirty_clip);
        egui_view_update_region_dirty(self, &dirty_clip);
    }
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
    if (self->shadow != NULL)
    {
        egui_region_t shadow_region;
        egui_shadow_get_region(self->shadow, &self->region_screen, &shadow_region);
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_TRACE
        egui_view_log_dirty_source("shadow", self, &shadow_region);
#endif
        egui_view_update_region_dirty(self, &shadow_region);
    }
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (self->is_focused)
    {
        egui_view_invalidate_focus_region(self);
    }
#endif
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
int egui_view_dispatch_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (self == NULL || event == NULL || self->api == NULL)
    {
        return 0;
    }

    if (self->is_enable && self->api->on_key != NULL && self->api->on_key(self, event))
    {
        return 1;
    }

    return (self->api->on_key_event != NULL) ? self->api->on_key_event(self, event) : 0;
}

void egui_view_override_api_on_key(egui_view_t *self, egui_view_api_t *api, egui_view_on_key_listener_t listener)
{
    if (self == NULL || api == NULL || self->api == NULL)
    {
        return;
    }

    egui_view_copy_api(self, api);
    api->on_key = listener;
}

int egui_view_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (self == NULL || event == NULL)
    {
        return 0;
    }

    if (self->is_enable == false)
    {
        return 0;
    }

    // If clickable, ENTER key triggers click
    if (self->is_clickable)
    {
        if (event->key_code == EGUI_KEY_CODE_ENTER || event->key_code == EGUI_KEY_CODE_SPACE)
        {
            if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
            {
                egui_view_set_pressed(self, true);
                return 1;
            }
            if (event->type == EGUI_KEY_EVENT_ACTION_UP)
            {
                int should_click = self->is_pressed;

                egui_view_set_pressed(self, false);
                if (should_click)
                {
                    egui_view_perform_click(self);
                }
                return 1;
            }
            if (event->type == EGUI_KEY_EVENT_ACTION_LONG_PRESS || event->type == EGUI_KEY_EVENT_ACTION_REPEAT)
            {
                return 1;
            }
        }
    }

    return 0;
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
void egui_view_set_focusable(egui_view_t *self, int is_focusable)
{
    if (self == NULL)
    {
        return;
    }

    self->is_focusable = is_focusable;
    if (!is_focusable)
    {
        egui_view_clear_focus_if_subtree_unfocusable(self);
    }
}

void egui_view_override_api_on_focus_changed(egui_view_t *self, egui_view_api_t *api, egui_view_on_focus_change_listener_t listener)
{
    if (self == NULL || api == NULL || self->api == NULL)
    {
        return;
    }

    egui_view_copy_api(self, api);
    api->on_focus_changed = listener;
}

void egui_view_override_api_on_draw_focus_frame(egui_view_t *self, egui_view_api_t *api, egui_view_on_draw_focus_frame_t listener)
{
    if (self == NULL || api == NULL || self->api == NULL)
    {
        return;
    }

    egui_view_copy_api(self, api);
    api->on_draw_focus_frame = listener;
}

int egui_view_get_focusable(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }

    return self->is_focusable;
}

void egui_view_request_focus(egui_view_t *self)
{
    egui_core_t *core = egui_view_get_core(self);

    if (core == NULL)
    {
        return;
    }

    if (egui_focus_view_is_focusable(self))
    {
        egui_focus_manager_set_focus(core, self);
    }
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
void egui_view_set_layer(egui_view_t *self, uint8_t layer)
{
    if (self == NULL)
    {
        return;
    }

    if (self->layer == layer)
    {
        return;
    }

    self->layer = layer;

    if (self->parent != NULL)
    {
        egui_view_group_reorder_child((egui_view_t *)self->parent, self);
    }

    egui_view_invalidate(self);
}

uint8_t egui_view_get_layer(egui_view_t *self)
{
    if (self == NULL)
    {
        return EGUI_VIEW_LAYER_DEFAULT;
    }

    return self->layer;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_LAYER

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch = NULL,
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .perform_click = NULL,
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .on_key = NULL,
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        .on_focus_changed = NULL,
        .on_draw_focus_frame = NULL,
#endif
};

void egui_view_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_ASSERT(core != NULL);

    self->core = core;

#if EGUI_CONFIG_DEBUG_VIEW_ID
    self->id = egui_core_get_unique_id(core);
#endif
    self->parent = NULL; // set parent later
    egui_dnode_init(&self->node);

    self->region.location.x = 0;
    self->region.location.y = 0;
    self->region.size.width = 0;
    self->region.size.height = 0;

    self->region_screen.location.x = 0;
    self->region_screen.location.y = 0;
    self->region_screen.size.width = 0;
    self->region_screen.size.height = 0;

    self->padding.left = 0;
    self->padding.right = 0;
    self->padding.top = 0;
    self->padding.bottom = 0;

    self->margin.left = 0;
    self->margin.right = 0;
    self->margin.top = 0;
    self->margin.bottom = 0;

    self->is_enable = true;

    self->is_clickable = false;
    self->is_pressed = false;
    self->is_visible = true;
    self->is_gone = false;
    self->is_request_layout = true;
    self->is_attached_to_window = false;
#if EGUI_CONFIG_FUNCTION_SUPPORT_DIRTY_PASSTHROUGH
    self->is_dirty_passthrough = false;
#endif

    self->background = NULL;
    self->last_dirty_epoch = UINT32_MAX;
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
    self->shadow = NULL;
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    self->on_click_listener = NULL;
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH || EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    self->is_focusable = false;
    self->is_focused = false;
    self->is_no_focus_clear = 0;
    self->is_focus_frame_visible = true;
    self->focus_frame_margin = EGUI_VIEW_FOCUS_FRAME_MARGIN;
    self->focus_frame_stroke = EGUI_VIEW_FOCUS_FRAME_STROKE;
    self->focus_frame_color = EGUI_THEME_FOCUS;
    self->focus_frame_alpha = EGUI_ALPHA_100;
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
    self->layer = EGUI_VIEW_LAYER_DEFAULT;
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_LAYER

#if EGUI_CONFIG_FUNCTION_FLEXLAYOUT
    self->flex_grow = 0;
#endif

#if EGUI_CONFIG_FUNCTION_VIEW_USER_DATA
    self->user_data = NULL;
#endif

    self->alpha = EGUI_ALPHA_100;

#if EGUI_CONFIG_FUNCTION_EXT_CLICK_AREA
    self->ext_click_area = 0;
#endif

#if EGUI_CONFIG_FUNCTION_DRAGGABLE_VIEW && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    self->is_draggable = 0;
    self->_drag_tracking = 0;
    self->_drag_last_x = 0;
    self->_drag_last_y = 0;
#endif

#if EGUI_CONFIG_FUNCTION_STYLE_CASCADE
    {
        uint8_t _si;
        for (_si = 0; _si < EGUI_CONFIG_STYLE_MAX_PER_VIEW; _si++)
        {
            self->styles[_si] = NULL;
        }
        self->style_count = 0;
        self->has_own_alpha = 0;
    }
#endif

#if EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
    self->view_state = 0;
#endif

#if EGUI_CONFIG_FUNCTION_EVENT_LITE
    {
        uint8_t _ei;
        for (_ei = 0; _ei < EGUI_CONFIG_EVENT_MAX_LISTENERS_PER_VIEW; _ei++)
        {
            self->event_listeners[_ei].code = EGUI_EVENT_ALL;
            self->event_listeners[_ei].cb = NULL;
            self->event_listeners[_ei].user_data = NULL;
        }
        self->event_listener_count = 0;
    }
#endif

    // init api
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_t);

    egui_view_set_view_name(self, "egui_view");
}

void egui_view_start_periodic(egui_view_t *view, egui_timer_t *timer, void *user_data, egui_timer_callback_func callback, uint32_t period_ms)
{
    egui_core_t *core;
    if (view == NULL || timer == NULL || callback == NULL)
    {
        return;
    }
    core = egui_view_get_core(view);
    if (core == NULL)
    {
        return;
    }
    egui_timer_init_timer(timer, user_data, callback);
    egui_timer_start_timer(core, timer, period_ms, period_ms);
}

void egui_view_stop_periodic(egui_view_t *view, egui_timer_t *timer)
{
    egui_core_t *core;
    if (view == NULL || timer == NULL)
    {
        return;
    }
    core = egui_view_get_core(view);
    if (core == NULL)
    {
        return;
    }
    egui_timer_stop_timer(core, timer);
}

#if EGUI_CONFIG_FUNCTION_VIEW_USER_DATA
void egui_view_set_user_data(egui_view_t *self, void *user_data)
{
    if (self == NULL)
    {
        return;
    }
    self->user_data = user_data;
}

void *egui_view_get_user_data(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    return self->user_data;
}
#endif /* EGUI_CONFIG_FUNCTION_VIEW_USER_DATA */
