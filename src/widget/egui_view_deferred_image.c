#include <string.h>

#include "egui_view_deferred_image.h"
#include "style/egui_theme.h"

#if EGUI_CONFIG_FUNCTION_IMAGE_FILE

static egui_core_t *egui_view_deferred_image_get_core(egui_view_deferred_image_t *local)
{
    return local != NULL ? egui_view_get_core(EGUI_VIEW_OF(local)) : NULL;
}

static int egui_view_deferred_image_copy_string(egui_core_t *core, char **target, const char *value)
{
    size_t len;
    char *copy;

    if (target == NULL)
    {
        return 0;
    }

    if (value == NULL || value[0] == '\0')
    {
        if (*target != NULL)
        {
            egui_free(core, *target);
            *target = NULL;
        }
        return 1;
    }

    len = strlen(value);
    copy = (char *)egui_malloc(core, (int)(len + 1));
    if (copy == NULL)
    {
        return 0;
    }

    memcpy(copy, value, len + 1);
    if (*target != NULL)
    {
        egui_free(core, *target);
    }
    *target = copy;
    return 1;
}

static void egui_view_deferred_image_stop_timers(egui_view_deferred_image_t *local)
{
    if (local == NULL)
    {
        return;
    }

    egui_view_stop_timer(EGUI_VIEW_OF(local), &local->delay_timer);
    egui_view_stop_timer(EGUI_VIEW_OF(local), &local->poll_timer);
}

static int egui_view_deferred_image_has_loader(const egui_view_deferred_image_t *local)
{
    return local != NULL && local->loader != NULL && local->loader->start != NULL && local->loader->poll != NULL;
}

static int egui_view_deferred_image_is_config_ready(const egui_view_deferred_image_t *local)
{
    return local != NULL && local->source_uri != NULL && local->source_uri[0] != '\0' && local->cache_path != NULL && local->cache_path[0] != '\0' &&
           egui_view_deferred_image_has_loader(local);
}

static void egui_view_deferred_image_reset_loaded_image(egui_view_deferred_image_t *local)
{
    egui_core_t *core;

    if (local == NULL)
    {
        return;
    }

    core = egui_view_deferred_image_get_core(local);
    egui_image_file_deinit(&local->loaded_image);
    egui_image_file_init(&local->loaded_image, core);
    egui_image_file_set_placeholder(&local->loaded_image, local->file_placeholder);
}

static void egui_view_deferred_image_release_request(egui_view_deferred_image_t *local)
{
    if (local == NULL || local->request_handle == NULL)
    {
        return;
    }

    if (local->loader != NULL && local->loader->cancel != NULL)
    {
        local->loader->cancel(local->loader->user_data, local->request_handle);
    }
    local->request_handle = NULL;
}

static void egui_view_deferred_image_reset_to_placeholder(egui_view_deferred_image_t *local, egui_view_deferred_image_status_t status)
{
    if (local == NULL)
    {
        return;
    }

    egui_view_deferred_image_stop_timers(local);
    egui_view_deferred_image_release_request(local);
    local->display_image = local->placeholder_image;
    egui_view_deferred_image_reset_loaded_image(local);
    local->status = (uint8_t)status;
}

static void egui_view_deferred_image_start_schedule(egui_view_t *self);

static void egui_view_deferred_image_finish_ready(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);

    egui_view_deferred_image_stop_timers(local);
    egui_view_deferred_image_release_request(local);
    egui_image_file_set_placeholder(&local->loaded_image, local->file_placeholder);
    if (!egui_image_file_set_path(&local->loaded_image, local->cache_path) || !egui_image_file_reload(&local->loaded_image))
    {
        local->display_image = local->placeholder_image;
        local->status = EGUI_VIEW_DEFERRED_IMAGE_STATUS_FAILED;
        egui_view_invalidate(self);
        return;
    }

    local->display_image = (const egui_image_t *)&local->loaded_image;
    local->status = EGUI_VIEW_DEFERRED_IMAGE_STATUS_READY;
    egui_view_invalidate(self);
}

static void egui_view_deferred_image_fail(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);

    egui_view_deferred_image_stop_timers(local);
    egui_view_deferred_image_release_request(local);
    local->display_image = local->placeholder_image;
    egui_view_deferred_image_reset_loaded_image(local);
    local->status = EGUI_VIEW_DEFERRED_IMAGE_STATUS_FAILED;
    egui_view_invalidate(self);
}

static void egui_view_deferred_image_poll_timer_callback(egui_timer_t *timer)
{
    egui_view_deferred_image_t *local = (egui_view_deferred_image_t *)timer->user_data;
    egui_view_deferred_image_loader_poll_result_t result;

    if (local == NULL || local->status != EGUI_VIEW_DEFERRED_IMAGE_STATUS_LOADING || local->loader == NULL || local->loader->poll == NULL)
    {
        return;
    }

    result = local->loader->poll(local->loader->user_data, local->request_handle);
    if (result == EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_PENDING)
    {
        return;
    }
    if (result == EGUI_VIEW_DEFERRED_IMAGE_LOADER_POLL_SUCCESS)
    {
        egui_view_deferred_image_finish_ready(EGUI_VIEW_OF(local));
        return;
    }

    egui_view_deferred_image_fail(EGUI_VIEW_OF(local));
}

static void egui_view_deferred_image_begin_load(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);
    void *request_handle = NULL;

    egui_view_stop_timer(self, &local->delay_timer);
    if (!egui_view_deferred_image_is_config_ready(local))
    {
        local->status = EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE;
        return;
    }

    if (!local->loader->start(local->loader->user_data, local->source_uri, local->cache_path, &request_handle))
    {
        egui_view_deferred_image_fail(self);
        return;
    }

    local->request_handle = request_handle;
    local->status = EGUI_VIEW_DEFERRED_IMAGE_STATUS_LOADING;
    egui_view_start_timer(self, &local->poll_timer, local->poll_interval_ms, local->poll_interval_ms);
}

static void egui_view_deferred_image_delay_timer_callback(egui_timer_t *timer)
{
    egui_view_deferred_image_t *local = (egui_view_deferred_image_t *)timer->user_data;

    if (local == NULL)
    {
        return;
    }

    egui_view_deferred_image_begin_load(EGUI_VIEW_OF(local));
}

static void egui_view_deferred_image_start_schedule(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);

    if (!self->is_attached_to_window || !egui_view_deferred_image_is_config_ready(local))
    {
        return;
    }

    egui_view_deferred_image_stop_timers(local);
    local->status = EGUI_VIEW_DEFERRED_IMAGE_STATUS_DELAY;
    if (local->load_delay_ms == 0u)
    {
        egui_view_deferred_image_begin_load(self);
        return;
    }

    egui_view_start_timer(self, &local->delay_timer, local->load_delay_ms, 0);
}

static void egui_view_deferred_image_draw_default_placeholder(egui_canvas_t *canvas, const egui_region_t *region)
{
    egui_dim_t radius;

    if (canvas == NULL || region == NULL || region->size.width <= 0 || region->size.height <= 0)
    {
        return;
    }

    radius = EGUI_MIN(region->size.width, region->size.height) / 6;
    if (radius < EGUI_THEME_RADIUS_SM)
    {
        radius = EGUI_THEME_RADIUS_SM;
    }
    if (radius > EGUI_THEME_RADIUS_LG)
    {
        radius = EGUI_THEME_RADIUS_LG;
    }

    egui_canvas_draw_round_rectangle_fill(canvas, region->location.x, region->location.y, region->size.width, region->size.height, radius, EGUI_THEME_BORDER,
                                          EGUI_ALPHA_100);
    if (region->size.width > 2 && region->size.height > 2)
    {
        egui_dim_t inner_radius = radius - 1;

        if (inner_radius < 0)
        {
            inner_radius = 0;
        }
        egui_canvas_draw_round_rectangle_fill(canvas, region->location.x + 1, region->location.y + 1, region->size.width - 2, region->size.height - 2,
                                              inner_radius, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
    }
}

static void egui_view_deferred_image_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_region_t region;

    egui_view_get_work_region(self, &region);
    if (local->display_image != NULL)
    {
        egui_canvas_draw_image_resize(canvas, local->display_image, region.location.x, region.location.y, region.size.width, region.size.height);
        return;
    }

    egui_view_deferred_image_draw_default_placeholder(canvas, &region);
}

static void egui_view_deferred_image_on_attach_to_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);

    egui_view_on_attach_to_window(self);
    if (local->auto_start_on_attach && local->status == EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE)
    {
        egui_view_deferred_image_start_schedule(self);
    }
}

static void egui_view_deferred_image_on_detach_from_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);

    egui_view_deferred_image_stop_timers(local);
    if (local->status == EGUI_VIEW_DEFERRED_IMAGE_STATUS_LOADING)
    {
        egui_view_deferred_image_release_request(local);
        local->display_image = local->placeholder_image;
        egui_view_deferred_image_reset_loaded_image(local);
        local->status = EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE;
    }
    else if (local->status == EGUI_VIEW_DEFERRED_IMAGE_STATUS_DELAY)
    {
        local->status = EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE;
    }

    egui_view_on_detach_from_window(self);
}

void egui_view_deferred_image_apply_params(egui_view_t *self, const egui_view_deferred_image_params_t *params)
{
    if (params == NULL)
    {
        return;
    }

    self->region = params->region;
    egui_view_invalidate(self);
}

void egui_view_deferred_image_set_source_uri(egui_view_t *self, const char *source_uri)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);
    const egui_image_t *old_display = local->display_image;

    if ((local->source_uri == NULL && (source_uri == NULL || source_uri[0] == '\0')) ||
        (local->source_uri != NULL && source_uri != NULL && strcmp(local->source_uri, source_uri) == 0))
    {
        return;
    }
    if (!egui_view_deferred_image_copy_string(egui_view_get_core(self), &local->source_uri, source_uri))
    {
        local->status = EGUI_VIEW_DEFERRED_IMAGE_STATUS_FAILED;
        return;
    }

    egui_view_deferred_image_reset_to_placeholder(local, EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE);
    if (old_display != local->display_image)
    {
        egui_view_invalidate(self);
    }
    if (local->auto_start_on_attach)
    {
        egui_view_deferred_image_start_schedule(self);
    }
}

const char *egui_view_deferred_image_get_source_uri(const egui_view_t *self)
{
    const egui_view_deferred_image_t *local = (const egui_view_deferred_image_t *)self;

    if (local == NULL)
    {
        return NULL;
    }
    return local->source_uri;
}

void egui_view_deferred_image_set_cache_path(egui_view_t *self, const char *cache_path)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);
    const egui_image_t *old_display = local->display_image;

    if ((local->cache_path == NULL && (cache_path == NULL || cache_path[0] == '\0')) ||
        (local->cache_path != NULL && cache_path != NULL && strcmp(local->cache_path, cache_path) == 0))
    {
        return;
    }
    if (!egui_view_deferred_image_copy_string(egui_view_get_core(self), &local->cache_path, cache_path))
    {
        local->status = EGUI_VIEW_DEFERRED_IMAGE_STATUS_FAILED;
        return;
    }

    egui_view_deferred_image_reset_to_placeholder(local, EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE);
    if (old_display != local->display_image)
    {
        egui_view_invalidate(self);
    }
    if (local->auto_start_on_attach)
    {
        egui_view_deferred_image_start_schedule(self);
    }
}

const char *egui_view_deferred_image_get_cache_path(const egui_view_t *self)
{
    const egui_view_deferred_image_t *local = (const egui_view_deferred_image_t *)self;

    if (local == NULL)
    {
        return NULL;
    }
    return local->cache_path;
}

void egui_view_deferred_image_set_placeholder_image(egui_view_t *self, const egui_image_t *image)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);

    if (local->placeholder_image == image)
    {
        return;
    }

    local->placeholder_image = image;
    if (local->status != EGUI_VIEW_DEFERRED_IMAGE_STATUS_READY)
    {
        local->display_image = image;
        egui_view_invalidate(self);
    }
}

const egui_image_t *egui_view_deferred_image_get_placeholder_image(const egui_view_t *self)
{
    const egui_view_deferred_image_t *local = (const egui_view_deferred_image_t *)self;

    if (local == NULL)
    {
        return NULL;
    }
    return local->placeholder_image;
}

void egui_view_deferred_image_set_file_placeholder(egui_view_t *self, const egui_image_t *image)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);

    if (local->file_placeholder == image)
    {
        return;
    }

    local->file_placeholder = image;
    egui_image_file_set_placeholder(&local->loaded_image, image);
}

const egui_image_t *egui_view_deferred_image_get_file_placeholder(const egui_view_t *self)
{
    const egui_view_deferred_image_t *local = (const egui_view_deferred_image_t *)self;

    if (local == NULL)
    {
        return NULL;
    }
    return local->file_placeholder;
}

void egui_view_deferred_image_set_loader(egui_view_t *self, const egui_view_deferred_image_loader_t *loader)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);
    const egui_image_t *old_display = local->display_image;

    if (local->loader == loader)
    {
        return;
    }

    local->loader = loader;
    egui_view_deferred_image_reset_to_placeholder(local, EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE);
    if (old_display != local->display_image)
    {
        egui_view_invalidate(self);
    }
    if (local->auto_start_on_attach)
    {
        egui_view_deferred_image_start_schedule(self);
    }
}

const egui_view_deferred_image_loader_t *egui_view_deferred_image_get_loader(const egui_view_t *self)
{
    const egui_view_deferred_image_t *local = (const egui_view_deferred_image_t *)self;

    if (local == NULL)
    {
        return NULL;
    }
    return local->loader;
}

void egui_view_deferred_image_set_auto_start_on_attach(egui_view_t *self, int auto_start_on_attach)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);
    uint8_t next_value = auto_start_on_attach ? 1u : 0u;

    if (local->auto_start_on_attach == next_value)
    {
        return;
    }

    local->auto_start_on_attach = next_value;
    if (local->auto_start_on_attach && self->is_attached_to_window && local->status == EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE)
    {
        egui_view_deferred_image_start_schedule(self);
    }
}

uint8_t egui_view_deferred_image_get_auto_start_on_attach(const egui_view_t *self)
{
    const egui_view_deferred_image_t *local = (const egui_view_deferred_image_t *)self;

    if (local == NULL)
    {
        return 0;
    }
    return local->auto_start_on_attach;
}

void egui_view_deferred_image_set_load_delay_ms(egui_view_t *self, uint16_t delay_ms)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);

    local->load_delay_ms = delay_ms;
}

uint16_t egui_view_deferred_image_get_load_delay_ms(const egui_view_t *self)
{
    const egui_view_deferred_image_t *local = (const egui_view_deferred_image_t *)self;

    if (local == NULL)
    {
        return 0;
    }
    return local->load_delay_ms;
}

const egui_image_t *egui_view_deferred_image_get_display_image(const egui_view_t *self)
{
    const egui_view_deferred_image_t *local = (const egui_view_deferred_image_t *)self;

    if (local == NULL)
    {
        return NULL;
    }
    return local->display_image;
}

void egui_view_deferred_image_reload(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);

    egui_view_deferred_image_reset_to_placeholder(local, EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE);
    egui_view_invalidate(self);
    egui_view_deferred_image_start_schedule(self);
}

void egui_view_deferred_image_cancel(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);

    egui_view_deferred_image_reset_to_placeholder(local, EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE);
    egui_view_invalidate(self);
}

egui_view_deferred_image_status_t egui_view_deferred_image_get_status(const egui_view_t *self)
{
    const egui_view_deferred_image_t *local = (const egui_view_deferred_image_t *)self;

    if (local == NULL)
    {
        return EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE;
    }

    return (egui_view_deferred_image_status_t)local->status;
}

void egui_view_deferred_image_deinit(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_deferred_image_t);
    egui_core_t *core = egui_view_get_core(self);

    egui_view_deferred_image_stop_timers(local);
    egui_view_deferred_image_release_request(local);
    egui_image_file_deinit(&local->loaded_image);
    if (local->source_uri != NULL)
    {
        egui_free(core, local->source_uri);
        local->source_uri = NULL;
    }
    if (local->cache_path != NULL)
    {
        egui_free(core, local->cache_path);
        local->cache_path = NULL;
    }
    local->display_image = NULL;
    local->placeholder_image = NULL;
    local->file_placeholder = NULL;
    local->loader = NULL;
    local->status = EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE;
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_deferred_image_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_deferred_image_on_attach_to_window,
        .on_draw = egui_view_deferred_image_on_draw,
        .on_detach_from_window = egui_view_deferred_image_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_deferred_image_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_deferred_image_params_t *params)
{
    egui_view_deferred_image_init(self, core);
    egui_view_deferred_image_apply_params(self, params);
}

void egui_view_deferred_image_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_deferred_image_t);

    egui_view_init(self, core);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_deferred_image_t);

    local->source_uri = NULL;
    local->cache_path = NULL;
    local->placeholder_image = NULL;
    local->file_placeholder = NULL;
    local->display_image = NULL;
    local->loader = NULL;
    local->request_handle = NULL;
    local->load_delay_ms = 50u;
    local->poll_interval_ms = 50u;
    local->status = EGUI_VIEW_DEFERRED_IMAGE_STATUS_IDLE;
    local->auto_start_on_attach = 1u;

    egui_image_file_init(&local->loaded_image, core);
    egui_timer_init_timer(&local->delay_timer, local, egui_view_deferred_image_delay_timer_callback);
    egui_timer_init_timer(&local->poll_timer, local, egui_view_deferred_image_poll_timer_callback);

    egui_view_set_view_name(self, "egui_view_deferred_image");
}

#endif /* EGUI_CONFIG_FUNCTION_IMAGE_FILE */
