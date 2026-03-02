#include <string.h>

#include "egui_view_stopwatch.h"
#include "utils/egui_sprintf.h"

static void format_elapsed(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_stopwatch_t);

    uint32_t total_sec = local->elapsed_ms / 1000;
    uint32_t hours = total_sec / 3600;
    uint32_t minutes = (total_sec % 3600) / 60;
    uint32_t seconds = total_sec % 60;
    uint32_t centisec = (local->elapsed_ms % 1000) / 10;

    int pos = 0;
    int remain = (int)sizeof(local->time_buffer);
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

    egui_view_label_set_text(self, local->time_buffer);
}

void egui_view_stopwatch_set_elapsed(egui_view_t *self, uint32_t elapsed_ms)
{
    EGUI_LOCAL_INIT(egui_view_stopwatch_t);
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
    local->show_ms = show;
    format_elapsed(self);
}

void egui_view_stopwatch_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_stopwatch_t);
    // call super init.
    egui_view_label_init(self);

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
