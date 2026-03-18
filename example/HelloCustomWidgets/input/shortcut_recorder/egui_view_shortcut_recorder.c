#include <stdio.h>
#include <string.h>

#include "egui_view_shortcut_recorder.h"

#define SR_STD_RADIUS    11
#define SR_STD_PAD_X     10
#define SR_STD_PAD_Y     8
#define SR_STD_TITLE_H   10
#define SR_STD_HELPER_H  10
#define SR_STD_FIELD_H   30
#define SR_STD_ROW_H     18
#define SR_STD_CLEAR_H   18
#define SR_STD_GAP       5
#define SR_STD_STATUS_W  46
#define SR_STD_CLEAR_W   52
#define SR_STD_TOKEN_H   16
#define SR_STD_TOKEN_GAP 4
#define SR_STD_PLUS_W    8

#define SR_COMPACT_RADIUS   8
#define SR_COMPACT_PAD_X    7
#define SR_COMPACT_PAD_Y    6
#define SR_COMPACT_FIELD_H  24
#define SR_COMPACT_ROW_H    14
#define SR_COMPACT_STATUS_W 34

typedef struct egui_view_shortcut_recorder_metrics egui_view_shortcut_recorder_metrics_t;
struct egui_view_shortcut_recorder_metrics
{
    egui_region_t title_region;
    egui_region_t helper_region;
    egui_region_t field_region;
    egui_region_t footer_region;
    egui_region_t preset_regions[EGUI_VIEW_SHORTCUT_RECORDER_MAX_PRESETS];
    egui_region_t clear_region;
    uint8_t show_title;
    uint8_t show_helper;
    uint8_t show_footer;
    uint8_t show_clear;
    uint8_t visible_presets;
};

static uint8_t shortcut_recorder_has_text(const char *text)
{
    return (text != NULL && text[0] != '\0') ? 1 : 0;
}

static uint8_t shortcut_recorder_region_contains_point(const egui_region_t *region, egui_dim_t x, egui_dim_t y)
{
    if (region == NULL)
    {
        return 0;
    }
    if (region->size.width <= 0 || region->size.height <= 0)
    {
        return 0;
    }
    return (x >= region->location.x && x < region->location.x + region->size.width && y >= region->location.y && y < region->location.y + region->size.height)
                   ? 1
                   : 0;
}

static const char *shortcut_recorder_key_code_text(uint8_t key_code, char *buffer)
{
    if (buffer == NULL)
    {
        return "";
    }

    if (key_code >= EGUI_KEY_CODE_A && key_code <= EGUI_KEY_CODE_Z)
    {
        buffer[0] = (char)('A' + (key_code - EGUI_KEY_CODE_A));
        buffer[1] = '\0';
        return buffer;
    }
    if (key_code >= EGUI_KEY_CODE_0 && key_code <= EGUI_KEY_CODE_9)
    {
        buffer[0] = (char)('0' + (key_code - EGUI_KEY_CODE_0));
        buffer[1] = '\0';
        return buffer;
    }

    switch (key_code)
    {
    case EGUI_KEY_CODE_ENTER:
        return "Enter";
    case EGUI_KEY_CODE_SPACE:
        return "Space";
    case EGUI_KEY_CODE_TAB:
        return "Tab";
    case EGUI_KEY_CODE_HOME:
        return "Home";
    case EGUI_KEY_CODE_END:
        return "End";
    case EGUI_KEY_CODE_PLUS:
        return "+";
    case EGUI_KEY_CODE_MINUS:
        return "-";
    case EGUI_KEY_CODE_PERIOD:
        return ".";
    case EGUI_KEY_CODE_COMMA:
        return ",";
    case EGUI_KEY_CODE_SLASH:
        return "/";
    default:
        return "?";
    }
}

static void shortcut_recorder_format_binding_parts(uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl, uint8_t has_binding, char *buffer, int size)
{
    char key_buffer[8];
    int pos = 0;

    if (buffer == NULL || size <= 0)
    {
        return;
    }

    buffer[0] = '\0';
    if (!has_binding)
    {
        snprintf(buffer, (size_t)size, "Unassigned");
        return;
    }

    if (is_ctrl)
    {
        pos += snprintf(buffer + pos, (size_t)(size - pos), "Ctrl");
    }
    if (is_shift && pos < size - 1)
    {
        pos += snprintf(buffer + pos, (size_t)(size - pos), "%sShift", pos > 0 ? " + " : "");
    }
    if (pos < size - 1)
    {
        snprintf(buffer + pos, (size_t)(size - pos), "%s%s", pos > 0 ? " + " : "", shortcut_recorder_key_code_text(key_code, key_buffer));
    }
}

static void shortcut_recorder_format_binding_compact_parts(uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl, uint8_t has_binding, char *buffer, int size)
{
    char key_buffer[8];
    int pos = 0;

    if (buffer == NULL || size <= 0)
    {
        return;
    }

    buffer[0] = '\0';
    if (!has_binding)
    {
        snprintf(buffer, (size_t)size, "None");
        return;
    }

    if (is_ctrl)
    {
        pos += snprintf(buffer + pos, (size_t)(size - pos), "C");
    }
    if (is_shift && pos < size - 1)
    {
        pos += snprintf(buffer + pos, (size_t)(size - pos), "%sS", pos > 0 ? "+" : "");
    }
    if (pos < size - 1)
    {
        snprintf(buffer + pos, (size_t)(size - pos), "%s%s", pos > 0 ? "+" : "", shortcut_recorder_key_code_text(key_code, key_buffer));
    }
}

static uint8_t shortcut_recorder_is_capture_key(uint8_t key_code)
{
    if ((key_code >= EGUI_KEY_CODE_A && key_code <= EGUI_KEY_CODE_Z) || (key_code >= EGUI_KEY_CODE_0 && key_code <= EGUI_KEY_CODE_9))
    {
        return 1;
    }

    switch (key_code)
    {
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
    case EGUI_KEY_CODE_TAB:
    case EGUI_KEY_CODE_HOME:
    case EGUI_KEY_CODE_END:
    case EGUI_KEY_CODE_PLUS:
    case EGUI_KEY_CODE_MINUS:
    case EGUI_KEY_CODE_PERIOD:
    case EGUI_KEY_CODE_COMMA:
    case EGUI_KEY_CODE_SLASH:
        return 1;
    default:
        return 0;
    }
}

static uint8_t shortcut_recorder_valid_part(const egui_view_shortcut_recorder_t *local, uint8_t part)
{
    if (part == EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD)
    {
        return 1;
    }
    if (part == EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET)
    {
        return local->preset_count > 0 ? 1 : 0;
    }
    if (part == EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR)
    {
        return local->compact_mode ? 0 : (local->has_binding ? 1 : 0);
    }
    return 0;
}

static void shortcut_recorder_normalize_state(egui_view_shortcut_recorder_t *local)
{
    if (local->preset_count > EGUI_VIEW_SHORTCUT_RECORDER_MAX_PRESETS)
    {
        local->preset_count = EGUI_VIEW_SHORTCUT_RECORDER_MAX_PRESETS;
    }
    if (local->preset_count == 0)
    {
        local->current_preset = 0;
        local->pressed_preset = 0;
    }
    else
    {
        if (local->current_preset >= local->preset_count)
        {
            local->current_preset = (uint8_t)(local->preset_count - 1);
        }
        if (local->pressed_preset >= local->preset_count)
        {
            local->pressed_preset = 0;
        }
    }
    if (!shortcut_recorder_valid_part(local, local->current_part))
    {
        local->current_part = shortcut_recorder_valid_part(local, EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET) ? EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET
                                                                                                           : EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD;
    }
    if (!shortcut_recorder_valid_part(local, local->pressed_part))
    {
        local->pressed_part = EGUI_VIEW_SHORTCUT_RECORDER_PART_NONE;
        local->pressed_preset = 0;
    }
    if (local->read_only_mode || local->compact_mode)
    {
        local->listening = 0;
        local->pressed_part = EGUI_VIEW_SHORTCUT_RECORDER_PART_NONE;
        local->pressed_preset = 0;
    }
}

static void shortcut_recorder_notify(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    if (local->on_changed != NULL)
    {
        local->on_changed(self, local->current_part, local->current_preset);
    }
}

static void shortcut_recorder_set_current_part_inner(egui_view_t *self, uint8_t part, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    shortcut_recorder_normalize_state(local);
    if (!shortcut_recorder_valid_part(local, part))
    {
        part = shortcut_recorder_valid_part(local, EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET) ? EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET
                                                                                            : EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD;
    }
    if (local->current_part != part)
    {
        local->current_part = part;
        egui_view_invalidate(self);
    }
    if (notify)
    {
        shortcut_recorder_notify(self);
    }
}

static void shortcut_recorder_set_current_preset_inner(egui_view_t *self, uint8_t preset_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    if (local->preset_count == 0)
    {
        return;
    }
    if (preset_index >= local->preset_count)
    {
        preset_index = (uint8_t)(local->preset_count - 1);
    }
    if (local->current_preset != preset_index)
    {
        local->current_preset = preset_index;
        egui_view_invalidate(self);
    }
    if (notify)
    {
        shortcut_recorder_notify(self);
    }
}

static void shortcut_recorder_set_binding_inner(egui_view_t *self, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    local->has_binding = 1;
    local->binding_key_code = key_code;
    local->binding_is_shift = is_shift ? 1 : 0;
    local->binding_is_ctrl = is_ctrl ? 1 : 0;
    local->listening = 0;
    shortcut_recorder_set_current_part_inner(self, EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD, 0);
    egui_view_invalidate(self);
    if (notify)
    {
        shortcut_recorder_notify(self);
    }
}

static void shortcut_recorder_clear_binding_inner(egui_view_t *self, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    local->has_binding = 0;
    local->binding_key_code = EGUI_KEY_CODE_NONE;
    local->binding_is_shift = 0;
    local->binding_is_ctrl = 0;
    local->listening = 0;
    shortcut_recorder_set_current_part_inner(self,
                                             shortcut_recorder_valid_part(local, EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET)
                                                     ? EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET
                                                     : EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD,
                                             0);
    egui_view_invalidate(self);
    if (notify)
    {
        shortcut_recorder_notify(self);
    }
}

static void shortcut_recorder_set_listening_inner(egui_view_t *self, uint8_t listening, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);
    uint8_t value = listening ? 1 : 0;

    if (local->compact_mode || local->read_only_mode)
    {
        value = 0;
    }

    if (local->listening != value)
    {
        local->listening = value;
        if (value)
        {
            shortcut_recorder_set_current_part_inner(self, EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD, 0);
        }
        egui_view_invalidate(self);
    }
    if (notify)
    {
        shortcut_recorder_notify(self);
    }
}

static void shortcut_recorder_apply_preset_inner(egui_view_t *self, uint8_t preset_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);
    const egui_view_shortcut_recorder_preset_t *preset;

    if (local->preset_count == 0)
    {
        return;
    }
    if (preset_index >= local->preset_count)
    {
        preset_index = (uint8_t)(local->preset_count - 1);
    }

    preset = &local->presets[preset_index];
    local->current_preset = preset_index;
    shortcut_recorder_set_current_part_inner(self, EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET, 0);
    shortcut_recorder_set_binding_inner(self, preset->key_code, preset->is_shift, preset->is_ctrl, 0);
    local->current_part = EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET;
    egui_view_invalidate(self);
    if (notify)
    {
        shortcut_recorder_notify(self);
    }
}

static void shortcut_recorder_cycle_part(egui_view_t *self, int delta)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);
    uint8_t parts[3];
    int count = 0;
    int i;
    int current = 0;

    shortcut_recorder_normalize_state(local);
    parts[count++] = EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD;
    if (shortcut_recorder_valid_part(local, EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET))
    {
        parts[count++] = EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET;
    }
    if (shortcut_recorder_valid_part(local, EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR))
    {
        parts[count++] = EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR;
    }
    if (count <= 1)
    {
        shortcut_recorder_set_current_part_inner(self, EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD, 1);
        return;
    }

    for (i = 0; i < count; i++)
    {
        if (parts[i] == local->current_part)
        {
            current = i;
            break;
        }
    }
    current += delta;
    if (current < 0)
    {
        current = count - 1;
    }
    else if (current >= count)
    {
        current = 0;
    }
    shortcut_recorder_set_current_part_inner(self, parts[current], 1);
}

static void shortcut_recorder_get_metrics(egui_view_shortcut_recorder_t *local, egui_view_t *self, egui_view_shortcut_recorder_metrics_t *metrics)
{
    egui_region_t work_region;
    egui_dim_t pad_x = local->compact_mode ? SR_COMPACT_PAD_X : SR_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? SR_COMPACT_PAD_Y : SR_STD_PAD_Y;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t y;
    egui_dim_t row_h;
    uint8_t i;

    memset(metrics, 0, sizeof(*metrics));
    shortcut_recorder_normalize_state(local);
    egui_view_get_work_region(self, &work_region);

    content_x = work_region.location.x + pad_x;
    content_y = work_region.location.y + pad_y;
    content_w = work_region.size.width - pad_x * 2;
    row_h = local->compact_mode ? SR_COMPACT_ROW_H : SR_STD_ROW_H;
    y = content_y;

    metrics->show_title = (!local->compact_mode && shortcut_recorder_has_text(local->title)) ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && shortcut_recorder_has_text(local->helper)) ? 1 : 0;
    metrics->visible_presets = local->preset_count;
    if (metrics->visible_presets > EGUI_VIEW_SHORTCUT_RECORDER_MAX_PRESETS)
    {
        metrics->visible_presets = EGUI_VIEW_SHORTCUT_RECORDER_MAX_PRESETS;
    }
    if (local->compact_mode && metrics->visible_presets > 1)
    {
        metrics->visible_presets = 1;
    }

    if (metrics->show_title)
    {
        metrics->title_region.location.x = content_x;
        metrics->title_region.location.y = y;
        metrics->title_region.size.width = content_w;
        metrics->title_region.size.height = SR_STD_TITLE_H;
        y += SR_STD_TITLE_H + 3;
    }

    if (metrics->show_helper)
    {
        metrics->helper_region.location.x = content_x;
        metrics->helper_region.location.y = y;
        metrics->helper_region.size.width = content_w;
        metrics->helper_region.size.height = SR_STD_HELPER_H;
        y += SR_STD_HELPER_H + SR_STD_GAP;
    }

    metrics->field_region.location.x = content_x;
    metrics->field_region.location.y = y;
    metrics->field_region.size.width = content_w;
    metrics->field_region.size.height = local->compact_mode ? SR_COMPACT_FIELD_H : SR_STD_FIELD_H;
    y += metrics->field_region.size.height + SR_STD_GAP;

    for (i = 0; i < metrics->visible_presets; i++)
    {
        metrics->preset_regions[i].location.x = content_x;
        metrics->preset_regions[i].location.y = y;
        metrics->preset_regions[i].size.width = content_w;
        metrics->preset_regions[i].size.height = row_h;
        y += row_h + (local->compact_mode ? 3 : 4);
    }

    metrics->show_clear = local->compact_mode ? 0 : (local->has_binding ? 1 : 0);
    metrics->show_footer = local->compact_mode ? 0 : shortcut_recorder_has_text(local->footer_prefix);
    if (metrics->show_footer || metrics->show_clear)
    {
        egui_dim_t footer_y = work_region.location.y + work_region.size.height - pad_y - SR_STD_CLEAR_H;

        if (footer_y < y)
        {
            footer_y = y;
        }
        if (metrics->show_footer)
        {
            metrics->footer_region.location.x = content_x;
            metrics->footer_region.location.y = footer_y;
            metrics->footer_region.size.width = content_w - (metrics->show_clear ? (SR_STD_CLEAR_W + 6) : 0);
            metrics->footer_region.size.height = SR_STD_CLEAR_H;
        }
        if (metrics->show_clear)
        {
            metrics->clear_region.location.x = content_x + content_w - SR_STD_CLEAR_W;
            metrics->clear_region.location.y = footer_y;
            metrics->clear_region.size.width = SR_STD_CLEAR_W;
            metrics->clear_region.size.height = SR_STD_CLEAR_H;
        }
    }
}

static void shortcut_recorder_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint16_t align,
                                        egui_color_t color)
{
    egui_region_t draw_region;

    if (font == NULL || !shortcut_recorder_has_text(text) || region == NULL)
    {
        return;
    }

    draw_region = *region;
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void shortcut_recorder_draw_focus(egui_view_t *self, const egui_region_t *region, egui_dim_t radius, egui_color_t color)
{
    if (region == NULL)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(region->location.x - 1, region->location.y - 1, region->size.width + 2, region->size.height + 2, radius, 1, color,
                                     egui_color_alpha_mix(self->alpha, 72));
}

static egui_dim_t shortcut_recorder_token_width(const char *text, uint8_t compact_mode)
{
    egui_dim_t base = compact_mode ? 8 : 10;
    egui_dim_t char_width = compact_mode ? 4 : 5;

    if (!shortcut_recorder_has_text(text))
    {
        return compact_mode ? 28 : 34;
    }
    return (egui_dim_t)(base + (egui_dim_t)strlen(text) * char_width);
}

static void shortcut_recorder_draw_binding_tokens(egui_view_shortcut_recorder_t *local, egui_view_t *self, const egui_region_t *region,
                                                  egui_color_t field_color, egui_color_t text_color, egui_color_t muted_text_color)
{
    static const char *ctrl_text = "CTRL";
    static const char *shift_text = "SHIFT";
    char key_text[8];
    const char *labels[3];
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t token_h = SR_STD_TOKEN_H;
    egui_dim_t plus_w = SR_STD_PLUS_W;
    egui_dim_t i;
    uint8_t label_count = 0;

    if (region == NULL)
    {
        return;
    }

    if (!local->has_binding)
    {
        shortcut_recorder_draw_text(local->font, self, "Unassigned", region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
        return;
    }

    if (local->binding_is_ctrl)
    {
        labels[label_count++] = ctrl_text;
    }
    if (local->binding_is_shift)
    {
        labels[label_count++] = shift_text;
    }
    labels[label_count++] = shortcut_recorder_key_code_text(local->binding_key_code, key_text);

    x = region->location.x;
    y = region->location.y + (region->size.height - token_h) / 2;
    for (i = 0; i < label_count; i++)
    {
        egui_dim_t token_w = shortcut_recorder_token_width(labels[i], 0);
        egui_region_t token_region;

        if (x + token_w > region->location.x + region->size.width)
        {
            break;
        }

        token_region.location.x = x;
        token_region.location.y = y;
        token_region.size.width = token_w;
        token_region.size.height = token_h;
        egui_canvas_draw_round_rectangle_fill(token_region.location.x, token_region.location.y, token_region.size.width, token_region.size.height, token_h / 2,
                                              egui_rgb_mix(field_color, local->accent_color, i + 1 == label_count ? 34 : 12),
                                              egui_color_alpha_mix(self->alpha, 100));
        egui_canvas_draw_round_rectangle(token_region.location.x, token_region.location.y, token_region.size.width, token_region.size.height, token_h / 2, 1,
                                         egui_rgb_mix(local->border_color, local->accent_color, i + 1 == label_count ? 28 : 12),
                                         egui_color_alpha_mix(self->alpha, 56));
        shortcut_recorder_draw_text(local->meta_font, self, labels[i], &token_region, EGUI_ALIGN_CENTER, text_color);

        x += token_w;
        if (i + 1 < label_count)
        {
            egui_region_t plus_region;

            plus_region.location.x = x;
            plus_region.location.y = y;
            plus_region.size.width = plus_w;
            plus_region.size.height = token_h;
            shortcut_recorder_draw_text(local->meta_font, self, "+", &plus_region, EGUI_ALIGN_CENTER, muted_text_color);
            x += plus_w + SR_STD_TOKEN_GAP;
        }
    }
}

static void shortcut_recorder_draw_field(egui_view_shortcut_recorder_t *local, egui_view_t *self, const egui_view_shortcut_recorder_metrics_t *metrics,
                                         egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color, egui_color_t muted_text_color)
{
    egui_region_t token_region;
    egui_region_t status_region;
    egui_color_t field_fill;
    egui_color_t focus_color = local->listening ? local->listening_color : local->accent_color;
    const char *status_text = local->read_only_mode ? "LOCK" : (local->listening ? "LISTEN" : "READY");
    egui_color_t status_fill = local->read_only_mode ? egui_rgb_mix(surface_color, border_color, 12)
                                                     : (local->listening ? egui_rgb_mix(surface_color, local->listening_color, 28)
                                                                         : egui_rgb_mix(surface_color, local->accent_color, 12));

    field_fill = egui_rgb_mix(surface_color, local->listening ? local->listening_color : local->accent_color, local->listening ? 10 : 4);
    egui_canvas_draw_round_rectangle_fill(metrics->field_region.location.x, metrics->field_region.location.y, metrics->field_region.size.width,
                                          metrics->field_region.size.height, local->compact_mode ? SR_COMPACT_RADIUS : SR_STD_RADIUS, field_fill,
                                          egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_round_rectangle(metrics->field_region.location.x, metrics->field_region.location.y, metrics->field_region.size.width,
                                     metrics->field_region.size.height, local->compact_mode ? SR_COMPACT_RADIUS : SR_STD_RADIUS, 1,
                                     egui_rgb_mix(border_color, focus_color, local->listening ? 28 : 10), egui_color_alpha_mix(self->alpha, 62));

    status_region.location.x =
            metrics->field_region.location.x + metrics->field_region.size.width - (local->compact_mode ? SR_COMPACT_STATUS_W : SR_STD_STATUS_W) - 6;
    status_region.location.y = metrics->field_region.location.y + 5;
    status_region.size.width = local->compact_mode ? SR_COMPACT_STATUS_W : SR_STD_STATUS_W;
    status_region.size.height = metrics->field_region.size.height - 10;

    egui_canvas_draw_round_rectangle_fill(status_region.location.x, status_region.location.y, status_region.size.width, status_region.size.height,
                                          status_region.size.height / 2, status_fill, egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_round_rectangle(status_region.location.x, status_region.location.y, status_region.size.width, status_region.size.height,
                                     status_region.size.height / 2, 1, egui_rgb_mix(border_color, focus_color, local->listening ? 30 : 12),
                                     egui_color_alpha_mix(self->alpha, 54));
    shortcut_recorder_draw_text(local->meta_font, self, status_text, &status_region, EGUI_ALIGN_CENTER,
                                local->listening ? local->listening_color : (local->read_only_mode ? muted_text_color : text_color));

    token_region.location.x = metrics->field_region.location.x + 8;
    token_region.location.y = metrics->field_region.location.y + 3;
    token_region.size.width = status_region.location.x - token_region.location.x - 6;
    token_region.size.height = metrics->field_region.size.height - 6;

    if (local->compact_mode || local->read_only_mode)
    {
        char binding_text[32];

        shortcut_recorder_format_binding_compact_parts(local->binding_key_code, local->binding_is_shift, local->binding_is_ctrl, local->has_binding,
                                                       binding_text, sizeof(binding_text));
        shortcut_recorder_draw_text(local->font, self, binding_text, &token_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                    local->has_binding ? text_color : muted_text_color);
    }
    else
    {
        shortcut_recorder_draw_binding_tokens(local, self, &token_region, field_fill, text_color, muted_text_color);
    }

    if (local->current_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD && !local->read_only_mode)
    {
        shortcut_recorder_draw_focus(self, &metrics->field_region, local->compact_mode ? SR_COMPACT_RADIUS : SR_STD_RADIUS, focus_color);
    }
}

static void shortcut_recorder_draw_preset_row(egui_view_shortcut_recorder_t *local, egui_view_t *self, const egui_region_t *region, uint8_t preset_index,
                                              egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color, egui_color_t muted_text_color)
{
    const egui_view_shortcut_recorder_preset_t *preset = &local->presets[preset_index];
    char binding_text[32];
    egui_region_t label_region = *region;
    egui_region_t binding_region = *region;
    egui_color_t row_fill;
    egui_color_t row_border;

    if (local->compact_mode)
    {
        shortcut_recorder_format_binding_compact_parts(preset->key_code, preset->is_shift, preset->is_ctrl, 1, binding_text, sizeof(binding_text));
    }
    else
    {
        shortcut_recorder_format_binding_parts(preset->key_code, preset->is_shift, preset->is_ctrl, 1, binding_text, sizeof(binding_text));
    }
    row_fill = egui_rgb_mix(surface_color, local->accent_color,
                            preset_index == local->current_preset
                                    ? (local->pressed_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET && preset_index == local->pressed_preset ? 18 : 10)
                                    : 4);
    row_border = egui_rgb_mix(border_color, local->accent_color, preset_index == local->current_preset ? 18 : 6);

    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, region->size.height / 2, row_fill,
                                          egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, region->size.height / 2, 1, row_border,
                                     egui_color_alpha_mix(self->alpha, 46));

    label_region.location.x += 8;
    label_region.size.width = region->size.width / 2;
    binding_region.location.x = region->location.x + region->size.width / 2;
    binding_region.size.width = region->size.width / 2 - 8;

    shortcut_recorder_draw_text(local->meta_font, self, preset->label, &label_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color);
    shortcut_recorder_draw_text(local->meta_font, self, binding_text, &binding_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER,
                                preset_index == local->current_preset ? text_color : muted_text_color);

    if (local->current_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET && preset_index == local->current_preset && !local->read_only_mode)
    {
        shortcut_recorder_draw_focus(self, region, region->size.height / 2, local->accent_color);
    }
}

static void shortcut_recorder_draw_footer(egui_view_shortcut_recorder_t *local, egui_view_t *self, const egui_view_shortcut_recorder_metrics_t *metrics,
                                          egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color, egui_color_t muted_text_color)
{
    if (metrics->show_footer)
    {
        shortcut_recorder_draw_text(local->meta_font, self, local->footer_prefix, &metrics->footer_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                    muted_text_color);
    }
    if (metrics->show_clear)
    {
        egui_color_t fill = egui_rgb_mix(surface_color, local->danger_color, local->pressed_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR ? 20 : 8);

        egui_canvas_draw_round_rectangle_fill(metrics->clear_region.location.x, metrics->clear_region.location.y, metrics->clear_region.size.width,
                                              metrics->clear_region.size.height, metrics->clear_region.size.height / 2, fill,
                                              egui_color_alpha_mix(self->alpha, 100));
        egui_canvas_draw_round_rectangle(metrics->clear_region.location.x, metrics->clear_region.location.y, metrics->clear_region.size.width,
                                         metrics->clear_region.size.height, metrics->clear_region.size.height / 2, 1,
                                         egui_rgb_mix(border_color, local->danger_color, 22), egui_color_alpha_mix(self->alpha, 52));
        shortcut_recorder_draw_text(local->meta_font, self, "Clear", &metrics->clear_region, EGUI_ALIGN_CENTER, text_color);

        if (local->current_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR && !local->read_only_mode)
        {
            shortcut_recorder_draw_focus(self, &metrics->clear_region, metrics->clear_region.size.height / 2, local->danger_color);
        }
    }
}

static void egui_view_shortcut_recorder_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);
    egui_view_shortcut_recorder_metrics_t metrics;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_text_color = local->muted_text_color;
    uint8_t i;

    if (local->read_only_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xEEF2F6), 42);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0xA6B2BF), 34);
        text_color = egui_rgb_mix(text_color, EGUI_COLOR_HEX(0x7A8794), 28);
        muted_text_color = egui_rgb_mix(muted_text_color, EGUI_COLOR_HEX(0x8E99A5), 30);
    }

    shortcut_recorder_get_metrics(local, self, &metrics);
    egui_canvas_draw_round_rectangle_fill(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                          self->region_screen.size.height, local->compact_mode ? SR_COMPACT_RADIUS : SR_STD_RADIUS, surface_color,
                                          egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                     self->region_screen.size.height, local->compact_mode ? SR_COMPACT_RADIUS : SR_STD_RADIUS, 1, border_color,
                                     egui_color_alpha_mix(self->alpha, 58));

    if (metrics.show_title)
    {
        shortcut_recorder_draw_text(local->meta_font, self, local->title, &metrics.title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    }
    if (metrics.show_helper)
    {
        shortcut_recorder_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    }

    shortcut_recorder_draw_field(local, self, &metrics, surface_color, border_color, text_color, muted_text_color);
    for (i = 0; i < metrics.visible_presets; i++)
    {
        shortcut_recorder_draw_preset_row(local, self, &metrics.preset_regions[i], i, surface_color, border_color, text_color, muted_text_color);
    }
    shortcut_recorder_draw_footer(local, self, &metrics, surface_color, border_color, text_color, muted_text_color);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static uint8_t shortcut_recorder_hit_test(egui_view_t *self, egui_dim_t x, egui_dim_t y, uint8_t *part, uint8_t *preset_index)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);
    egui_view_shortcut_recorder_metrics_t metrics;
    uint8_t i;

    shortcut_recorder_get_metrics(local, self, &metrics);
    if (shortcut_recorder_region_contains_point(&metrics.field_region, x, y))
    {
        *part = EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD;
        *preset_index = 0;
        return 1;
    }
    for (i = 0; i < metrics.visible_presets; i++)
    {
        if (shortcut_recorder_region_contains_point(&metrics.preset_regions[i], x, y))
        {
            *part = EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET;
            *preset_index = i;
            return 1;
        }
    }
    if (metrics.show_clear && shortcut_recorder_region_contains_point(&metrics.clear_region, x, y))
    {
        *part = EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR;
        *preset_index = 0;
        return 1;
    }
    return 0;
}

static int egui_view_shortcut_recorder_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);
    uint8_t hit_part = EGUI_VIEW_SHORTCUT_RECORDER_PART_NONE;
    uint8_t hit_preset = 0;

    shortcut_recorder_normalize_state(local);
    if (!egui_view_get_enable(self) || local->compact_mode || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        if (!shortcut_recorder_hit_test(self, event->location.x, event->location.y, &hit_part, &hit_preset))
        {
            return 0;
        }
        local->pressed_part = hit_part;
        local->pressed_preset = hit_preset;
        if (hit_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET)
        {
            shortcut_recorder_set_current_preset_inner(self, hit_preset, 0);
        }
        shortcut_recorder_set_current_part_inner(self, hit_part, 0);
        egui_view_set_pressed(self, 1);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        egui_view_set_pressed(self, 0);
        if (local->pressed_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_NONE)
        {
            return 0;
        }
        if (shortcut_recorder_hit_test(self, event->location.x, event->location.y, &hit_part, &hit_preset) && hit_part == local->pressed_part &&
            (hit_part != EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET || hit_preset == local->pressed_preset))
        {
            if (hit_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD)
            {
                shortcut_recorder_set_listening_inner(self, !local->listening, 1);
            }
            else if (hit_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET)
            {
                shortcut_recorder_apply_preset_inner(self, hit_preset, 1);
            }
            else if (hit_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR)
            {
                shortcut_recorder_clear_binding_inner(self, 1);
            }
        }
        local->pressed_part = EGUI_VIEW_SHORTCUT_RECORDER_PART_NONE;
        local->pressed_preset = 0;
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        egui_view_set_pressed(self, 0);
        local->pressed_part = EGUI_VIEW_SHORTCUT_RECORDER_PART_NONE;
        local->pressed_preset = 0;
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_shortcut_recorder_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    shortcut_recorder_normalize_state(local);
    if (!egui_view_get_enable(self) || local->compact_mode || local->read_only_mode)
    {
        return 0;
    }

    if (event->type == EGUI_KEY_EVENT_ACTION_UP)
    {
        switch (event->key_code)
        {
        case EGUI_KEY_CODE_TAB:
        case EGUI_KEY_CODE_LEFT:
        case EGUI_KEY_CODE_RIGHT:
        case EGUI_KEY_CODE_UP:
        case EGUI_KEY_CODE_DOWN:
        case EGUI_KEY_CODE_HOME:
        case EGUI_KEY_CODE_END:
        case EGUI_KEY_CODE_ENTER:
        case EGUI_KEY_CODE_SPACE:
        case EGUI_KEY_CODE_PLUS:
        case EGUI_KEY_CODE_MINUS:
        case EGUI_KEY_CODE_ESCAPE:
            return 1;
        default:
            return local->listening ? 1 : egui_view_on_key_event(self, event);
        }
    }

    if (local->listening)
    {
        if (event->key_code == EGUI_KEY_CODE_ESCAPE)
        {
            shortcut_recorder_set_listening_inner(self, 0, 1);
            return 1;
        }
        if (shortcut_recorder_is_capture_key(event->key_code))
        {
            shortcut_recorder_set_binding_inner(self, event->key_code, event->is_shift, event->is_ctrl, 1);
            return 1;
        }
        return 1;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_TAB:
    case EGUI_KEY_CODE_RIGHT:
        shortcut_recorder_cycle_part(self, 1);
        return 1;
    case EGUI_KEY_CODE_LEFT:
        shortcut_recorder_cycle_part(self, -1);
        return 1;
    case EGUI_KEY_CODE_UP:
    case EGUI_KEY_CODE_MINUS:
        if (local->current_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET && local->preset_count > 0)
        {
            if (local->current_preset > 0)
            {
                shortcut_recorder_set_current_preset_inner(self, (uint8_t)(local->current_preset - 1), 1);
            }
            return 1;
        }
        return 0;
    case EGUI_KEY_CODE_DOWN:
    case EGUI_KEY_CODE_PLUS:
        if (local->current_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET && local->preset_count > 0)
        {
            if (local->current_preset + 1 < local->preset_count)
            {
                shortcut_recorder_set_current_preset_inner(self, (uint8_t)(local->current_preset + 1), 1);
            }
            return 1;
        }
        return 0;
    case EGUI_KEY_CODE_HOME:
        if (local->current_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET && local->preset_count > 0)
        {
            shortcut_recorder_set_current_preset_inner(self, 0, 1);
            return 1;
        }
        return 0;
    case EGUI_KEY_CODE_END:
        if (local->current_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET && local->preset_count > 0)
        {
            shortcut_recorder_set_current_preset_inner(self, (uint8_t)(local->preset_count - 1), 1);
            return 1;
        }
        return 0;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (local->current_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD)
        {
            shortcut_recorder_set_listening_inner(self, 1, 1);
            return 1;
        }
        if (local->current_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET)
        {
            shortcut_recorder_apply_preset_inner(self, local->current_preset, 1);
            return 1;
        }
        if (local->current_part == EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR)
        {
            shortcut_recorder_clear_binding_inner(self, 1);
            return 1;
        }
        return 0;
    case EGUI_KEY_CODE_ESCAPE:
        if (local->current_part != EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD)
        {
            shortcut_recorder_set_current_part_inner(self, EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD, 1);
            return 1;
        }
        return 0;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_shortcut_recorder_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_shortcut_recorder_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_shortcut_recorder_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_shortcut_recorder_on_key_event,
#endif
};

void egui_view_shortcut_recorder_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_shortcut_recorder_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_shortcut_recorder_t);
    egui_view_set_padding_all(self, 2);

    local->on_changed = NULL;
    local->presets = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->title = NULL;
    local->helper = NULL;
    local->footer_prefix = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD9E0E7);
    local->text_color = EGUI_COLOR_HEX(0x1F2933);
    local->muted_text_color = EGUI_COLOR_HEX(0x708090);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->listening_color = EGUI_COLOR_HEX(0xD97706);
    local->preview_color = EGUI_COLOR_HEX(0x0F766E);
    local->danger_color = EGUI_COLOR_HEX(0xBE5168);
    local->preset_count = 0;
    local->current_part = EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD;
    local->current_preset = 0;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->listening = 0;
    local->has_binding = 0;
    local->binding_key_code = EGUI_KEY_CODE_NONE;
    local->binding_is_shift = 0;
    local->binding_is_ctrl = 0;
    local->pressed_part = EGUI_VIEW_SHORTCUT_RECORDER_PART_NONE;
    local->pressed_preset = 0;

    egui_view_set_view_name(self, "egui_view_shortcut_recorder");
}

void egui_view_shortcut_recorder_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    local->font = font == NULL ? (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT : font;
    egui_view_invalidate(self);
}

void egui_view_shortcut_recorder_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    local->meta_font = font == NULL ? (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT : font;
    egui_view_invalidate(self);
}

void egui_view_shortcut_recorder_set_header(egui_view_t *self, const char *title, const char *helper, const char *footer_prefix)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    local->title = title;
    local->helper = helper;
    local->footer_prefix = footer_prefix;
    egui_view_invalidate(self);
}

void egui_view_shortcut_recorder_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                             egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t listening_color, egui_color_t preview_color,
                                             egui_color_t danger_color)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->listening_color = listening_color;
    local->preview_color = preview_color;
    local->danger_color = danger_color;
    egui_view_invalidate(self);
}

void egui_view_shortcut_recorder_set_presets(egui_view_t *self, const egui_view_shortcut_recorder_preset_t *presets, uint8_t preset_count)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    local->presets = presets;
    local->preset_count = presets == NULL ? 0 : preset_count;
    shortcut_recorder_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_shortcut_recorder_set_binding(egui_view_t *self, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl)
{
    shortcut_recorder_set_binding_inner(self, key_code, is_shift, is_ctrl, 0);
}

void egui_view_shortcut_recorder_commit_binding(egui_view_t *self, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl)
{
    shortcut_recorder_set_binding_inner(self, key_code, is_shift, is_ctrl, 1);
}

void egui_view_shortcut_recorder_clear_binding(egui_view_t *self)
{
    shortcut_recorder_clear_binding_inner(self, 0);
}

uint8_t egui_view_shortcut_recorder_has_binding(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    return local->has_binding;
}

uint8_t egui_view_shortcut_recorder_get_key_code(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    return local->binding_key_code;
}

uint8_t egui_view_shortcut_recorder_get_is_shift(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    return local->binding_is_shift;
}

uint8_t egui_view_shortcut_recorder_get_is_ctrl(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    return local->binding_is_ctrl;
}

void egui_view_shortcut_recorder_get_binding_text(egui_view_t *self, char *buffer, int size)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    shortcut_recorder_format_binding_parts(local->binding_key_code, local->binding_is_shift, local->binding_is_ctrl, local->has_binding, buffer, size);
}

void egui_view_shortcut_recorder_set_listening(egui_view_t *self, uint8_t listening)
{
    shortcut_recorder_set_listening_inner(self, listening, 0);
}

uint8_t egui_view_shortcut_recorder_is_listening(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    return local->listening;
}

void egui_view_shortcut_recorder_apply_preset(egui_view_t *self, uint8_t preset_index)
{
    shortcut_recorder_apply_preset_inner(self, preset_index, 0);
}

void egui_view_shortcut_recorder_set_current_part(egui_view_t *self, uint8_t part)
{
    shortcut_recorder_set_current_part_inner(self, part, 0);
}

uint8_t egui_view_shortcut_recorder_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    return local->current_part;
}

void egui_view_shortcut_recorder_set_current_preset(egui_view_t *self, uint8_t preset_index)
{
    shortcut_recorder_set_current_preset_inner(self, preset_index, 0);
}

uint8_t egui_view_shortcut_recorder_get_current_preset(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    return local->current_preset;
}

void egui_view_shortcut_recorder_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    local->compact_mode = compact_mode ? 1 : 0;
    shortcut_recorder_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_shortcut_recorder_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    local->read_only_mode = read_only_mode ? 1 : 0;
    shortcut_recorder_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_shortcut_recorder_set_on_changed_listener(egui_view_t *self, egui_view_on_shortcut_recorder_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);

    local->on_changed = listener;
}

uint8_t egui_view_shortcut_recorder_get_part_region(egui_view_t *self, uint8_t part, uint8_t preset_index, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_shortcut_recorder_t);
    egui_view_shortcut_recorder_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }

    shortcut_recorder_get_metrics(local, self, &metrics);
    switch (part)
    {
    case EGUI_VIEW_SHORTCUT_RECORDER_PART_FIELD:
        *region = metrics.field_region;
        return 1;
    case EGUI_VIEW_SHORTCUT_RECORDER_PART_PRESET:
        if (preset_index >= metrics.visible_presets)
        {
            return 0;
        }
        *region = metrics.preset_regions[preset_index];
        return 1;
    case EGUI_VIEW_SHORTCUT_RECORDER_PART_CLEAR:
        if (!metrics.show_clear)
        {
            return 0;
        }
        *region = metrics.clear_region;
        return 1;
    default:
        return 0;
    }
}
