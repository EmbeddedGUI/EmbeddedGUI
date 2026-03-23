#include <string.h>

#include "egui_view_stopwatch.h"
#include "core/egui_common.h"
#include "resource/egui_resource.h"
#include "utils/egui_sprintf.h"

static uint8_t stopwatch_get_text_dirty_region(egui_view_t *self, const egui_view_label_t *label, const char *text, egui_region_t *dirty_region)
{
    egui_region_t work_region;
    egui_dim_t text_width;
    egui_dim_t text_height;
    egui_dim_t offset_x;
    egui_dim_t offset_y;

    if (dirty_region == NULL)
    {
        return 0;
    }

    egui_region_init_empty(dirty_region);

    if (label == NULL || text == NULL || label->font == NULL || label->font->api == NULL || label->font->api->get_str_size == NULL)
    {
        return 0;
    }

    egui_view_get_work_region(self, &work_region);
    if (egui_region_is_empty(&work_region))
    {
        return 0;
    }

    label->font->api->get_str_size(label->font, text, 1, label->line_space, &text_width, &text_height);
    if (text_width <= 0 || text_height <= 0)
    {
        return 0;
    }

    text_width = EGUI_MIN(text_width, work_region.size.width);
    text_height = EGUI_MIN(text_height, work_region.size.height);

    egui_common_align_get_x_y(work_region.size.width, work_region.size.height, text_width, text_height, label->align_type, &offset_x, &offset_y);

    dirty_region->location.x = work_region.location.x + offset_x;
    dirty_region->location.y = work_region.location.y + offset_y;
    dirty_region->size.width = text_width;
    dirty_region->size.height = text_height;

    // Keep a 1px pad around glyph edges to avoid text AA artifacts.
    if (dirty_region->location.x > work_region.location.x)
    {
        dirty_region->location.x--;
        dirty_region->size.width++;
    }
    if (dirty_region->location.y > work_region.location.y)
    {
        dirty_region->location.y--;
        dirty_region->size.height++;
    }
    if (dirty_region->location.x + dirty_region->size.width < work_region.location.x + work_region.size.width)
    {
        dirty_region->size.width++;
    }
    if (dirty_region->location.y + dirty_region->size.height < work_region.location.y + work_region.size.height)
    {
        dirty_region->size.height++;
    }

    egui_region_intersect(dirty_region, &work_region, dirty_region);
    return egui_region_is_empty(dirty_region) ? 0 : 1;
}

static void stopwatch_update_text(egui_view_t *self, egui_view_stopwatch_t *local, const egui_region_t *old_region, uint8_t has_old_region)
{
    egui_region_t new_region;
    egui_region_t dirty_region;
    uint8_t has_new_region;

    if (self->region_screen.size.width <= 0 || self->region_screen.size.height <= 0)
    {
        local->base.text = local->time_buffer;
        egui_view_invalidate(self);
        return;
    }

    local->base.text = local->time_buffer;
    has_new_region = stopwatch_get_text_dirty_region(self, &local->base, local->time_buffer, &new_region);

    if (!has_old_region && !has_new_region)
    {
        egui_view_invalidate(self);
        return;
    }

    if (!has_old_region)
    {
        egui_region_copy(&dirty_region, &new_region);
    }
    else if (!has_new_region)
    {
        egui_region_copy(&dirty_region, old_region);
    }
    else
    {
        egui_region_t old_region_copy;
        egui_region_copy(&old_region_copy, old_region);
        egui_region_union(&old_region_copy, &new_region, &dirty_region);
    }

    egui_view_invalidate_region(self, &dirty_region);
}

static void format_elapsed(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_stopwatch_t);
    egui_region_t old_text_region;
    uint8_t has_old_text_region;
    char old_text[sizeof(local->time_buffer)];
    const char *old_text_ptr = local->base.text;

    uint32_t total_sec = local->elapsed_ms / 1000;
    uint32_t hours = total_sec / 3600;
    uint32_t minutes = (total_sec % 3600) / 60;
    uint32_t seconds = total_sec % 60;
    uint32_t centisec = (local->elapsed_ms % 1000) / 10;
    int pos;
    int remain;

    if (old_text_ptr != NULL)
    {
        strncpy(old_text, old_text_ptr, sizeof(old_text) - 1);
        old_text[sizeof(old_text) - 1] = '\0';
        old_text_ptr = old_text;
    }

    has_old_text_region = stopwatch_get_text_dirty_region(self, &local->base, old_text_ptr, &old_text_region);

    pos = 0;
    remain = (int)sizeof(local->time_buffer);
    if (hours > 0)
    {
        pos += egui_sprintf_int(&local->time_buffer[pos], remain - pos, (int)hours);
        pos += egui_sprintf_char(&local->time_buffer[pos], remain - pos, ':');
        pos += egui_sprintf_int_pad(&local->time_buffer[pos], remain - pos, (int)minutes, 2, '0');
        pos += egui_sprintf_char(&local->time_buffer[pos], remain - pos, ':');
        pos += egui_sprintf_int_pad(&local->time_buffer[pos], remain - pos, (int)seconds, 2, '0');
        if (local->show_ms)
        {
            pos += egui_sprintf_char(&local->time_buffer[pos], remain - pos, '.');
            pos += egui_sprintf_int_pad(&local->time_buffer[pos], remain - pos, (int)centisec, 2, '0');
        }
    }
    else
    {
        pos += egui_sprintf_int_pad(&local->time_buffer[pos], remain - pos, (int)minutes, 2, '0');
        pos += egui_sprintf_char(&local->time_buffer[pos], remain - pos, ':');
        pos += egui_sprintf_int_pad(&local->time_buffer[pos], remain - pos, (int)seconds, 2, '0');
        if (local->show_ms)
        {
            pos += egui_sprintf_char(&local->time_buffer[pos], remain - pos, '.');
            pos += egui_sprintf_int_pad(&local->time_buffer[pos], remain - pos, (int)centisec, 2, '0');
        }
    }

    if (pos >= remain)
    {
        local->time_buffer[remain - 1] = '\0';
    }
    else
    {
        local->time_buffer[pos] = '\0';
    }

    stopwatch_update_text(self, local, &old_text_region, has_old_text_region);
}

void egui_view_stopwatch_set_elapsed(egui_view_t *self, uint32_t elapsed_ms)
{
    EGUI_LOCAL_INIT(egui_view_stopwatch_t);
    if (local->elapsed_ms == elapsed_ms)
    {
        return;
    }
    local->elapsed_ms = elapsed_ms;
    format_elapsed(self);
}

uint32_t egui_view_stopwatch_get_elapsed(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_stopwatch_t);
    return local->elapsed_ms;
}

void egui_view_stopwatch_set_state(egui_view_t *self, uint8_t state)
{
    EGUI_LOCAL_INIT(egui_view_stopwatch_t);
    local->state = state;
}

uint8_t egui_view_stopwatch_get_state(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_stopwatch_t);
    return local->state;
}

void egui_view_stopwatch_set_show_ms(egui_view_t *self, uint8_t show)
{
    EGUI_LOCAL_INIT(egui_view_stopwatch_t);
    if (local->show_ms == show)
    {
        return;
    }
    local->show_ms = show;
    format_elapsed(self);
}

void egui_view_stopwatch_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_stopwatch_t);
    // call super init.
    egui_view_label_init(self);
    local->base.font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    // init local data.
    local->elapsed_ms = 0;
    local->state = EGUI_VIEW_STOPWATCH_STATE_STOPPED;
    local->show_ms = 1;
    memset(local->time_buffer, 0, sizeof(local->time_buffer));

    format_elapsed(self);

    egui_view_set_view_name(self, "egui_view_stopwatch");
}

void egui_view_stopwatch_init_with_params(egui_view_t *self, const egui_view_label_params_t *params)
{
    egui_view_stopwatch_init(self);
    egui_view_label_apply_params(self, params);
}
