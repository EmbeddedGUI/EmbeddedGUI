#include <assert.h>
#include <string.h>

#include "egui_view_digital_clock.h"
#include "core/egui_api.h"
#include "utils/egui_sprintf.h"
#include "font/egui_font.h"
#include "resource/egui_resource.h"

/**
 * @file egui_view_digital_clock.c
 * @brief Digital clock implemented as a formatted label wrapper.
 *
 * The widget stores raw hour/minute/second fields, converts them into one
 * short ASCII buffer, and reuses the normal label draw path for rendering.
 */

/**
 * @brief Rebuild the visible time string from the stored clock fields.
 *
 * 24-hour mode optionally renders seconds. 12-hour mode uses `HH:MM AM/PM`.
 * Colon blinking is implemented by replacing ':' with spaces in the buffer.
 */
static void format_time(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);

    int pos = 0;
    int remain = (int)sizeof(local->time_buffer);
    if (local->format_24h)
    {
        pos += egui_sprintf_int_pad(&local->time_buffer[pos], remain - pos, local->hour, 2, '0');
        pos += egui_sprintf_char(&local->time_buffer[pos], remain - pos, ':');
        pos += egui_sprintf_int_pad(&local->time_buffer[pos], remain - pos, local->minute, 2, '0');
        if (local->show_second)
        {
            pos += egui_sprintf_char(&local->time_buffer[pos], remain - pos, ':');
            pos += egui_sprintf_int_pad(&local->time_buffer[pos], remain - pos, local->second, 2, '0');
        }
    }
    else
    {
        uint8_t display_h = local->hour % 12;
        if (display_h == 0)
        {
            display_h = 12;
        }
        const char *suffix = local->hour >= 12 ? "PM" : "AM";
        pos += egui_sprintf_int_pad(&local->time_buffer[pos], remain - pos, display_h, 2, ' ');
        pos += egui_sprintf_char(&local->time_buffer[pos], remain - pos, ':');
        pos += egui_sprintf_int_pad(&local->time_buffer[pos], remain - pos, local->minute, 2, '0');
        pos += egui_sprintf_char(&local->time_buffer[pos], remain - pos, ' ');
        pos += egui_sprintf_str(&local->time_buffer[pos], remain - pos, suffix);
    }

    // Colon blink: replace ':' with ' '
    if (local->colon_blink && !local->colon_visible)
    {
        for (int i = 0; local->time_buffer[i]; i++)
        {
            if (local->time_buffer[i] == ':')
            {
                local->time_buffer[i] = ' ';
            }
        }
    }

    egui_view_label_set_text(self, local->time_buffer);
}

/**
 * @brief Store raw time fields and immediately refresh the visible label text.
 */
void egui_view_digital_clock_set_time(egui_view_t *self, uint8_t hour, uint8_t minute, uint8_t second)
{
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    local->hour = hour;
    local->minute = minute;
    local->second = second;
    format_time(self);
}

uint8_t egui_view_digital_clock_get_hour(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    return local->hour;
}

uint8_t egui_view_digital_clock_get_minute(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    return local->minute;
}

uint8_t egui_view_digital_clock_get_second(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    return local->second;
}

const char *egui_view_digital_clock_get_time_text(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    return local->time_buffer;
}

/**
 * @brief Switch between 24-hour and 12-hour text formatting modes.
 */
void egui_view_digital_clock_set_format(egui_view_t *self, uint8_t format_24h)
{
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    local->format_24h = format_24h;
    format_time(self);
}

uint8_t egui_view_digital_clock_get_format_24h(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    return local->format_24h;
}

/**
 * @brief Enable or disable the colon-replacement blink behavior.
 */
void egui_view_digital_clock_set_colon_blink(egui_view_t *self, uint8_t enable)
{
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    local->colon_blink = enable;
    format_time(self);
}

uint8_t egui_view_digital_clock_get_colon_blink(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    return local->colon_blink;
}

/**
 * @brief Control whether blink-enabled colons currently appear visible.
 */
void egui_view_digital_clock_set_colon_visible(egui_view_t *self, uint8_t visible)
{
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    local->colon_visible = visible;
    format_time(self);
}

uint8_t egui_view_digital_clock_get_colon_visible(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    return local->colon_visible;
}

/**
 * @brief Show or hide the seconds field when 24-hour formatting is active.
 */
void egui_view_digital_clock_set_show_second(egui_view_t *self, uint8_t show)
{
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    local->show_second = show;
    format_time(self);
}

uint8_t egui_view_digital_clock_get_show_second(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_digital_clock_t);
    return local->show_second;
}

/**
 * @brief Initialize the digital clock as a label with local time-format state.
 */
void egui_view_digital_clock_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_digital_clock_t);
    // call super init.
    egui_view_label_init(self, core);
    local->base.font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_label_set_text(self, local->time_buffer);

    // init local data.
    local->hour = 0;
    local->minute = 0;
    local->second = 0;
    local->format_24h = 1;
    local->show_second = 1;
    local->colon_blink = 0;
    local->colon_visible = 1;
    egui_api_memset(local->time_buffer, 0, sizeof(local->time_buffer));
    format_time(self);
    egui_view_set_view_name(self, "egui_view_digital_clock");
}

/**
 * @brief Convenience initializer that chains clock init and label params.
 */
void egui_view_digital_clock_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_label_params_t *params)
{
    egui_view_digital_clock_init(self, core);
    egui_view_label_apply_params(self, params);
}
