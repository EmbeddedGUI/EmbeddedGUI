#include "egui_view_password_box.h"

#include <string.h>

#include "resource/egui_icon_material_symbols.h"

#define PASSWORD_BOX_STD_RADIUS           10
#define PASSWORD_BOX_STD_PAD_X            10
#define PASSWORD_BOX_STD_PAD_Y            8
#define PASSWORD_BOX_STD_LABEL_H          10
#define PASSWORD_BOX_STD_LABEL_GAP        5
#define PASSWORD_BOX_STD_ROW_H            26
#define PASSWORD_BOX_STD_HELPER_H         10
#define PASSWORD_BOX_STD_HELPER_GAP       5
#define PASSWORD_BOX_STD_ROW_RADIUS       7
#define PASSWORD_BOX_STD_REVEAL_W         24
#define PASSWORD_BOX_STD_REVEAL_GAP       6
#define PASSWORD_BOX_STD_FIELD_PAD_X      9
#define PASSWORD_BOX_STD_FIELD_PAD_Y      4
#define PASSWORD_BOX_STD_FILL_ALPHA       94
#define PASSWORD_BOX_STD_BORDER_ALPHA     66
#define PASSWORD_BOX_STD_ROW_FILL_ALPHA   44
#define PASSWORD_BOX_STD_ROW_BORDER_ALPHA 58

#define PASSWORD_BOX_COMPACT_RADIUS           8
#define PASSWORD_BOX_COMPACT_PAD_X            7
#define PASSWORD_BOX_COMPACT_PAD_Y            6
#define PASSWORD_BOX_COMPACT_ROW_H            20
#define PASSWORD_BOX_COMPACT_ROW_RADIUS       6
#define PASSWORD_BOX_COMPACT_REVEAL_W         20
#define PASSWORD_BOX_COMPACT_REVEAL_GAP       4
#define PASSWORD_BOX_COMPACT_FIELD_PAD_X      7
#define PASSWORD_BOX_COMPACT_FIELD_PAD_Y      3
#define PASSWORD_BOX_COMPACT_FILL_ALPHA       90
#define PASSWORD_BOX_COMPACT_BORDER_ALPHA     60
#define PASSWORD_BOX_COMPACT_ROW_FILL_ALPHA   38
#define PASSWORD_BOX_COMPACT_ROW_BORDER_ALPHA 52

#define PASSWORD_BOX_CURSOR_W 1

typedef struct egui_view_password_box_metrics egui_view_password_box_metrics_t;
struct egui_view_password_box_metrics
{
    egui_region_t content_region;
    egui_region_t label_region;
    egui_region_t helper_region;
    egui_region_t row_region;
    egui_region_t field_region;
    egui_region_t reveal_region;
    uint8_t show_meta;
    uint8_t show_reveal;
};

static void password_box_update_mask(egui_view_password_box_t *local)
{
    uint8_t index;

    for (index = 0; index < local->text_len && index < EGUI_VIEW_PASSWORD_BOX_MAX_TEXT_LEN; index++)
    {
        local->masked_text[index] = '*';
    }
    local->masked_text[index] = '\0';
}

static const char *password_box_get_display_text(egui_view_password_box_t *local)
{
    return local->revealed ? local->text : local->masked_text;
}

static uint8_t password_box_has_text(const char *text)
{
    return (text != NULL && text[0] != '\0') ? 1 : 0;
}

static uint8_t password_box_is_reveal_visible(egui_view_password_box_t *local)
{
    if (local->read_only_mode)
    {
        return 0;
    }
    return local->text_len > 0 ? 1 : 0;
}

static void password_box_notify_changed(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);

    if (local->on_changed)
    {
        local->on_changed(self, local->text, local->revealed ? 1 : 0, part);
    }
}

static egui_dim_t password_box_get_text_width_to_pos(egui_view_password_box_t *local, uint8_t pos)
{
    egui_dim_t width = 0;
    egui_dim_t height = 0;
    char tmp[EGUI_VIEW_PASSWORD_BOX_MAX_TEXT_LEN + 1];

    if (local->font == NULL || pos == 0)
    {
        return 0;
    }

    if (pos > local->text_len)
    {
        pos = local->text_len;
    }

    memcpy(tmp, password_box_get_display_text(local), pos);
    tmp[pos] = '\0';
    local->font->api->get_str_size(local->font, tmp, 0, 0, &width, &height);
    return width;
}

static void password_box_get_metrics(egui_view_password_box_t *local, egui_view_t *self, egui_view_password_box_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? PASSWORD_BOX_COMPACT_PAD_X : PASSWORD_BOX_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? PASSWORD_BOX_COMPACT_PAD_Y : PASSWORD_BOX_STD_PAD_Y;
    egui_dim_t row_h = local->compact_mode ? PASSWORD_BOX_COMPACT_ROW_H : PASSWORD_BOX_STD_ROW_H;
    egui_dim_t reveal_w = local->compact_mode ? PASSWORD_BOX_COMPACT_REVEAL_W : PASSWORD_BOX_STD_REVEAL_W;
    egui_dim_t reveal_gap = local->compact_mode ? PASSWORD_BOX_COMPACT_REVEAL_GAP : PASSWORD_BOX_STD_REVEAL_GAP;
    egui_dim_t row_y;
    egui_dim_t row_w;

    memset(metrics, 0, sizeof(*metrics));
    egui_view_get_work_region(self, &region);
    metrics->show_meta = local->compact_mode ? 0 : 1;
    metrics->show_reveal = password_box_is_reveal_visible(local);

    metrics->content_region.location.x = region.location.x + pad_x;
    metrics->content_region.location.y = region.location.y + pad_y;
    metrics->content_region.size.width = region.size.width - pad_x * 2;
    metrics->content_region.size.height = region.size.height - pad_y * 2;

    metrics->label_region.location.x = metrics->content_region.location.x;
    metrics->label_region.location.y = metrics->content_region.location.y;
    metrics->label_region.size.width = metrics->content_region.size.width;
    metrics->label_region.size.height = metrics->show_meta ? PASSWORD_BOX_STD_LABEL_H : 0;

    metrics->helper_region.location.x = metrics->content_region.location.x;
    metrics->helper_region.location.y =
            metrics->content_region.location.y + PASSWORD_BOX_STD_LABEL_H + PASSWORD_BOX_STD_LABEL_GAP + PASSWORD_BOX_STD_ROW_H + PASSWORD_BOX_STD_HELPER_GAP;
    metrics->helper_region.size.width = metrics->content_region.size.width;
    metrics->helper_region.size.height = metrics->show_meta ? PASSWORD_BOX_STD_HELPER_H : 0;

    row_y = metrics->show_meta ? (metrics->content_region.location.y + PASSWORD_BOX_STD_LABEL_H + PASSWORD_BOX_STD_LABEL_GAP)
                               : (metrics->content_region.location.y + (metrics->content_region.size.height - row_h) / 2);
    row_w = metrics->content_region.size.width;

    metrics->row_region.location.x = metrics->content_region.location.x;
    metrics->row_region.location.y = row_y;
    metrics->row_region.size.width = row_w;
    metrics->row_region.size.height = row_h;

    metrics->field_region = metrics->row_region;
    if (metrics->show_reveal)
    {
        metrics->reveal_region.location.x = metrics->row_region.location.x + metrics->row_region.size.width - reveal_w;
        metrics->reveal_region.location.y = metrics->row_region.location.y;
        metrics->reveal_region.size.width = reveal_w;
        metrics->reveal_region.size.height = row_h;
        metrics->field_region.size.width -= reveal_w + reveal_gap;
    }
}

static void password_box_update_scroll(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    egui_view_password_box_metrics_t metrics;
    egui_dim_t cursor_x;
    egui_dim_t work_width;
    egui_dim_t pad_x = local->compact_mode ? PASSWORD_BOX_COMPACT_FIELD_PAD_X : PASSWORD_BOX_STD_FIELD_PAD_X;

    if (self->region.size.width <= 0 || self->region.size.height <= 0)
    {
        return;
    }

    password_box_get_metrics(local, self, &metrics);
    work_width = metrics.field_region.size.width - pad_x * 2;
    if (work_width <= 0)
    {
        local->scroll_offset_x = 0;
        return;
    }

    cursor_x = password_box_get_text_width_to_pos(local, local->cursor_pos);
    if (cursor_x - local->scroll_offset_x > work_width - PASSWORD_BOX_CURSOR_W)
    {
        local->scroll_offset_x = cursor_x - work_width + PASSWORD_BOX_CURSOR_W;
    }
    if (cursor_x < local->scroll_offset_x)
    {
        local->scroll_offset_x = cursor_x;
    }
    if (local->scroll_offset_x < 0)
    {
        local->scroll_offset_x = 0;
    }
}

static void password_box_cursor_timer_callback(egui_timer_t *timer)
{
    egui_view_password_box_t *local = (egui_view_password_box_t *)timer->user_data;
    egui_view_t *self = (egui_view_t *)local;

    local->cursor_visible = !local->cursor_visible;
    egui_view_invalidate(self);
    egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
static void password_box_on_focus_change(egui_view_t *self, int is_focused)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);

    if (is_focused && !local->read_only_mode)
    {
        local->cursor_visible = 1;
        egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);
    }
    else
    {
        local->cursor_visible = 0;
        egui_timer_stop_timer(&local->cursor_timer);
    }
    egui_view_invalidate(self);
}
#endif

static void password_box_reset_cursor_blink(egui_view_password_box_t *local)
{
    local->cursor_visible = 1;
    egui_timer_stop_timer(&local->cursor_timer);
    egui_timer_start_timer(&local->cursor_timer, EGUI_CONFIG_TEXTINPUT_CURSOR_BLINK_MS, 0);
}

static void password_box_normalize_state(egui_view_password_box_t *local)
{
    if (local->text_len > EGUI_VIEW_PASSWORD_BOX_MAX_TEXT_LEN)
    {
        local->text_len = EGUI_VIEW_PASSWORD_BOX_MAX_TEXT_LEN;
    }
    if (local->cursor_pos > local->text_len)
    {
        local->cursor_pos = local->text_len;
    }
    if (local->read_only_mode)
    {
        local->revealed = 0;
    }
    if (!password_box_is_reveal_visible(local))
    {
        if (local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_REVEAL)
        {
            local->current_part = EGUI_VIEW_PASSWORD_BOX_PART_FIELD;
        }
        if (local->pressed_part == EGUI_VIEW_PASSWORD_BOX_PART_REVEAL)
        {
            local->pressed_part = EGUI_VIEW_PASSWORD_BOX_PART_NONE;
        }
        if (local->text_len == 0)
        {
            local->revealed = 0;
        }
    }
    if (local->current_part != EGUI_VIEW_PASSWORD_BOX_PART_FIELD && local->current_part != EGUI_VIEW_PASSWORD_BOX_PART_REVEAL)
    {
        local->current_part = EGUI_VIEW_PASSWORD_BOX_PART_FIELD;
    }
}

static void password_box_commit_text_change(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);

    password_box_update_mask(local);
    password_box_normalize_state(local);
    password_box_update_scroll(self);
    egui_view_invalidate(self);
    password_box_notify_changed(self, part);
}
void egui_view_password_box_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    size_t len = 0;

    if (text != NULL)
    {
        len = strlen(text);
        if (len > EGUI_VIEW_PASSWORD_BOX_MAX_TEXT_LEN)
        {
            len = EGUI_VIEW_PASSWORD_BOX_MAX_TEXT_LEN;
        }
        if (len > 0)
        {
            memcpy(local->text, text, len);
        }
    }
    local->text[len] = '\0';
    local->text_len = (uint8_t)len;
    local->cursor_pos = (uint8_t)len;
    local->scroll_offset_x = 0;
    password_box_commit_text_change(self, EGUI_VIEW_PASSWORD_BOX_PART_FIELD);
}

const char *egui_view_password_box_get_text(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    return local->text;
}

void egui_view_password_box_clear(egui_view_t *self)
{
    egui_view_password_box_set_text(self, NULL);
}

void egui_view_password_box_insert_char(egui_view_t *self, char ch)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    int index;

    if (local->read_only_mode || ch == 0 || local->text_len >= EGUI_VIEW_PASSWORD_BOX_MAX_TEXT_LEN)
    {
        return;
    }

    for (index = local->text_len; index > local->cursor_pos; index--)
    {
        local->text[index] = local->text[index - 1];
    }
    local->text[local->cursor_pos] = ch;
    local->text_len++;
    local->cursor_pos++;
    local->text[local->text_len] = '\0';
    password_box_reset_cursor_blink(local);
    password_box_commit_text_change(self, EGUI_VIEW_PASSWORD_BOX_PART_FIELD);
}

void egui_view_password_box_delete_char(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    int index;

    if (local->read_only_mode || local->cursor_pos == 0)
    {
        return;
    }

    for (index = local->cursor_pos - 1; index < local->text_len - 1; index++)
    {
        local->text[index] = local->text[index + 1];
    }
    local->cursor_pos--;
    local->text_len--;
    local->text[local->text_len] = '\0';
    password_box_reset_cursor_blink(local);
    password_box_commit_text_change(self, EGUI_VIEW_PASSWORD_BOX_PART_FIELD);
}

void egui_view_password_box_delete_forward(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    int index;

    if (local->read_only_mode || local->cursor_pos >= local->text_len)
    {
        return;
    }

    for (index = local->cursor_pos; index < local->text_len - 1; index++)
    {
        local->text[index] = local->text[index + 1];
    }
    local->text_len--;
    local->text[local->text_len] = '\0';
    password_box_reset_cursor_blink(local);
    password_box_commit_text_change(self, EGUI_VIEW_PASSWORD_BOX_PART_FIELD);
}

void egui_view_password_box_set_cursor_pos(egui_view_t *self, uint8_t cursor_pos)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);

    if (cursor_pos > local->text_len)
    {
        cursor_pos = local->text_len;
    }
    if (cursor_pos == local->cursor_pos)
    {
        return;
    }

    local->cursor_pos = cursor_pos;
    password_box_reset_cursor_blink(local);
    password_box_update_scroll(self);
    egui_view_invalidate(self);
}

void egui_view_password_box_move_cursor_left(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);

    if (local->cursor_pos > 0)
    {
        egui_view_password_box_set_cursor_pos(self, (uint8_t)(local->cursor_pos - 1));
    }
}

void egui_view_password_box_move_cursor_right(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);

    if (local->cursor_pos < local->text_len)
    {
        egui_view_password_box_set_cursor_pos(self, (uint8_t)(local->cursor_pos + 1));
    }
}

void egui_view_password_box_move_cursor_home(egui_view_t *self)
{
    egui_view_password_box_set_cursor_pos(self, 0);
}

void egui_view_password_box_move_cursor_end(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    egui_view_password_box_set_cursor_pos(self, local->text_len);
}

static void password_box_set_current_part_inner(egui_view_t *self, uint8_t part, uint8_t invalidate)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);

    if (part == EGUI_VIEW_PASSWORD_BOX_PART_REVEAL && !password_box_is_reveal_visible(local))
    {
        part = EGUI_VIEW_PASSWORD_BOX_PART_FIELD;
    }
    if (part != EGUI_VIEW_PASSWORD_BOX_PART_FIELD && part != EGUI_VIEW_PASSWORD_BOX_PART_REVEAL)
    {
        part = EGUI_VIEW_PASSWORD_BOX_PART_FIELD;
    }
    if (local->current_part == part)
    {
        return;
    }
    local->current_part = part;
    if (part == EGUI_VIEW_PASSWORD_BOX_PART_FIELD && !local->read_only_mode)
    {
        password_box_reset_cursor_blink(local);
    }
    if (invalidate)
    {
        egui_view_invalidate(self);
    }
}

void egui_view_password_box_set_current_part(egui_view_t *self, uint8_t part)
{
    password_box_set_current_part_inner(self, part, 1);
}

uint8_t egui_view_password_box_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    password_box_normalize_state(local);
    return local->current_part;
}

void egui_view_password_box_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    local->compact_mode = compact_mode ? 1 : 0;
    password_box_update_scroll(self);
    egui_view_invalidate(self);
}

void egui_view_password_box_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    local->pressed_part = EGUI_VIEW_PASSWORD_BOX_PART_NONE;
    if (local->read_only_mode)
    {
        local->revealed = 0;
        local->current_part = EGUI_VIEW_PASSWORD_BOX_PART_FIELD;
    }
    password_box_update_scroll(self);
    egui_view_invalidate(self);
}

void egui_view_password_box_set_revealed(egui_view_t *self, uint8_t revealed)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    uint8_t next = (revealed && password_box_is_reveal_visible(local)) ? 1 : 0;

    if (local->revealed == next)
    {
        return;
    }

    local->revealed = next;
    password_box_update_scroll(self);
    egui_view_invalidate(self);
    password_box_notify_changed(self, EGUI_VIEW_PASSWORD_BOX_PART_REVEAL);
}

uint8_t egui_view_password_box_get_revealed(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    return local->revealed ? 1 : 0;
}

void egui_view_password_box_set_on_changed_listener(egui_view_t *self, egui_view_on_password_box_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    local->on_changed = listener;
}

static void password_box_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align, egui_color_t color,
                                   egui_alpha_t alpha)
{
    egui_region_t draw_region = *region;

    if (font == NULL || !password_box_has_text(text) || egui_region_is_empty(&draw_region))
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, egui_color_alpha_mix(self->alpha, alpha));
}

void egui_view_password_box_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    local->font = font == NULL ? (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT : font;
    password_box_update_scroll(self);
    egui_view_invalidate(self);
}

void egui_view_password_box_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    local->meta_font = font == NULL ? (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT : font;
    egui_view_invalidate(self);
}

void egui_view_password_box_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    local->icon_font = font == NULL ? EGUI_FONT_ICON_MS_16 : font;
    egui_view_invalidate(self);
}

void egui_view_password_box_set_label(egui_view_t *self, const char *label)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    local->label = label;
    egui_view_invalidate(self);
}

void egui_view_password_box_set_helper(egui_view_t *self, const char *helper)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    local->helper = helper;
    egui_view_invalidate(self);
}

void egui_view_password_box_set_placeholder(egui_view_t *self, const char *placeholder)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    local->placeholder = placeholder;
    egui_view_invalidate(self);
}

void egui_view_password_box_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                        egui_color_t muted_text_color, egui_color_t accent_color)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    egui_view_invalidate(self);
}

static uint8_t password_box_hit_test(egui_view_password_box_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_password_box_metrics_t metrics;

    password_box_get_metrics(local, self, &metrics);
    if (metrics.show_reveal && egui_region_pt_in_rect(&metrics.reveal_region, x, y))
    {
        return EGUI_VIEW_PASSWORD_BOX_PART_REVEAL;
    }
    if (egui_region_pt_in_rect(&metrics.row_region, x, y) || egui_region_pt_in_rect(&self->region_screen, x, y))
    {
        return EGUI_VIEW_PASSWORD_BOX_PART_FIELD;
    }
    return EGUI_VIEW_PASSWORD_BOX_PART_NONE;
}

uint8_t egui_view_password_box_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    egui_view_password_box_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }

    password_box_get_metrics(local, self, &metrics);
    if (part == EGUI_VIEW_PASSWORD_BOX_PART_FIELD)
    {
        egui_region_copy(region, &metrics.field_region);
        return 1;
    }
    if (part == EGUI_VIEW_PASSWORD_BOX_PART_REVEAL && metrics.show_reveal)
    {
        egui_region_copy(region, &metrics.reveal_region);
        return 1;
    }
    egui_region_init_empty(region);
    return 0;
}
static void password_box_draw_row(egui_view_password_box_t *local, egui_view_t *self, const egui_view_password_box_metrics_t *metrics)
{
    egui_color_t row_border = local->border_color;
    egui_color_t row_fill = egui_rgb_mix(local->surface_color, EGUI_COLOR_WHITE, local->read_only_mode ? 10 : 16);
    egui_color_t text_color = local->text_color;
    egui_color_t helper_color = local->muted_text_color;
    egui_region_t text_region = metrics->field_region;
    const char *display_text = password_box_get_display_text(local);
    egui_dim_t text_x;
    egui_dim_t text_y;
    egui_dim_t text_h = 0;
    egui_dim_t text_w = 0;
    egui_dim_t cursor_x;
    egui_dim_t cursor_y;
    egui_dim_t cursor_h = 0;
    egui_dim_t pad_x = local->compact_mode ? PASSWORD_BOX_COMPACT_FIELD_PAD_X : PASSWORD_BOX_STD_FIELD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? PASSWORD_BOX_COMPACT_FIELD_PAD_Y : PASSWORD_BOX_STD_FIELD_PAD_Y;
    egui_dim_t row_radius = local->compact_mode ? PASSWORD_BOX_COMPACT_ROW_RADIUS : PASSWORD_BOX_STD_ROW_RADIUS;

    if (self->is_focused && local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_FIELD)
    {
        row_border = egui_rgb_mix(local->accent_color, local->border_color, 8);
    }
    if (local->pressed_part == EGUI_VIEW_PASSWORD_BOX_PART_FIELD)
    {
        row_fill = egui_rgb_mix(row_fill, local->accent_color, 8);
    }
    if (local->read_only_mode)
    {
        row_border = egui_rgb_mix(row_border, helper_color, 14);
        row_fill = egui_rgb_mix(row_fill, EGUI_COLOR_HEX(0xF7F9FB), 12);
        text_color = egui_rgb_mix(text_color, helper_color, 24);
    }

    egui_canvas_draw_round_rectangle_fill(
            metrics->row_region.location.x, metrics->row_region.location.y, metrics->row_region.size.width, metrics->row_region.size.height, row_radius,
            row_fill, egui_color_alpha_mix(self->alpha, local->compact_mode ? PASSWORD_BOX_COMPACT_ROW_FILL_ALPHA : PASSWORD_BOX_STD_ROW_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(
            metrics->row_region.location.x, metrics->row_region.location.y, metrics->row_region.size.width, metrics->row_region.size.height, row_radius, 1,
            row_border, egui_color_alpha_mix(self->alpha, local->compact_mode ? PASSWORD_BOX_COMPACT_ROW_BORDER_ALPHA : PASSWORD_BOX_STD_ROW_BORDER_ALPHA));

    text_region.location.x += pad_x;
    text_region.location.y += pad_y;
    text_region.size.width -= pad_x * 2;
    text_region.size.height -= pad_y * 2;

    if (local->font != NULL)
    {
        local->font->api->get_str_size(local->font, password_box_has_text(display_text) ? display_text : "A", 0, 0, &text_w, &text_h);
    }
    text_x = text_region.location.x - local->scroll_offset_x;
    text_y = text_region.location.y + (text_region.size.height - text_h) / 2;

    if (local->text_len == 0)
    {
        password_box_draw_text(local->font, self, local->placeholder, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, helper_color,
                               local->read_only_mode ? 88 : 100);
    }
    else
    {
        egui_canvas_draw_text(local->font, display_text, text_x, text_y, text_color, self->alpha);
    }

    if (self->is_focused && !local->read_only_mode && local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_FIELD && local->cursor_visible)
    {
        cursor_x = text_region.location.x + password_box_get_text_width_to_pos(local, local->cursor_pos) - local->scroll_offset_x;
        if (local->font != NULL)
        {
            local->font->api->get_str_size(local->font, "A", 0, 0, &text_w, &cursor_h);
        }
        cursor_y = text_region.location.y + (text_region.size.height - cursor_h) / 2;
        egui_canvas_draw_rectangle_fill(cursor_x, cursor_y, PASSWORD_BOX_CURSOR_W, cursor_h > 0 ? cursor_h : (text_region.size.height - 2), local->accent_color,
                                        egui_color_alpha_mix(self->alpha, 100));
    }

    if (metrics->show_reveal)
    {
        egui_color_t reveal_fill = egui_rgb_mix(row_fill, local->surface_color, 14);
        egui_color_t reveal_border = egui_rgb_mix(row_border, local->border_color, 12);
        egui_color_t reveal_text = egui_rgb_mix(local->text_color, local->accent_color, local->revealed ? 18 : 10);
        const char *glyph = local->revealed ? EGUI_ICON_MS_VISIBILITY_OFF : EGUI_ICON_MS_VISIBILITY;

        if (self->is_focused && local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_REVEAL)
        {
            reveal_fill = egui_rgb_mix(reveal_fill, local->accent_color, 10);
            reveal_border = egui_rgb_mix(reveal_border, local->accent_color, 10);
        }
        if (local->pressed_part == EGUI_VIEW_PASSWORD_BOX_PART_REVEAL)
        {
            reveal_fill = egui_rgb_mix(reveal_fill, local->accent_color, 16);
        }

        egui_canvas_draw_round_rectangle_fill(metrics->reveal_region.location.x, metrics->reveal_region.location.y, metrics->reveal_region.size.width,
                                              metrics->reveal_region.size.height, row_radius, reveal_fill,
                                              egui_color_alpha_mix(self->alpha, local->compact_mode ? 32 : 38));
        egui_canvas_draw_round_rectangle(metrics->reveal_region.location.x, metrics->reveal_region.location.y, metrics->reveal_region.size.width,
                                         metrics->reveal_region.size.height, row_radius, 1, reveal_border,
                                         egui_color_alpha_mix(self->alpha, local->compact_mode ? 48 : 54));
        password_box_draw_text(local->icon_font, self, glyph, &metrics->reveal_region, EGUI_ALIGN_CENTER, reveal_text, 100);
    }
}

void egui_view_password_box_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    egui_view_password_box_metrics_t metrics;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_color = local->muted_text_color;
    egui_dim_t radius = local->compact_mode ? PASSWORD_BOX_COMPACT_RADIUS : PASSWORD_BOX_STD_RADIUS;

    password_box_normalize_state(local);
    password_box_get_metrics(local, self, &metrics);
    password_box_update_scroll(self);

    if (local->read_only_mode)
    {
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFBFCFD), 14);
        border_color = egui_rgb_mix(border_color, muted_color, 18);
        text_color = egui_rgb_mix(text_color, muted_color, 24);
    }

    egui_canvas_draw_round_rectangle_fill(
            self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width, self->region_screen.size.height, radius,
            surface_color, egui_color_alpha_mix(self->alpha, local->compact_mode ? PASSWORD_BOX_COMPACT_FILL_ALPHA : PASSWORD_BOX_STD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(
            self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width, self->region_screen.size.height, radius, 1,
            border_color, egui_color_alpha_mix(self->alpha, local->compact_mode ? PASSWORD_BOX_COMPACT_BORDER_ALPHA : PASSWORD_BOX_STD_BORDER_ALPHA));

    if (metrics.show_meta)
    {
        password_box_draw_text(local->meta_font, self, local->label, &metrics.label_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                               egui_rgb_mix(text_color, local->accent_color, 8), 100);
        password_box_draw_text(local->meta_font, self, local->helper, &metrics.helper_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_color,
                               local->read_only_mode ? 84 : 100);
    }

    password_box_draw_row(local, self, &metrics);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_password_box_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    uint8_t hit_part;

    if (!egui_view_get_enable(self))
    {
        if (event->type == EGUI_MOTION_EVENT_ACTION_UP || event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
        {
            local->pressed_part = EGUI_VIEW_PASSWORD_BOX_PART_NONE;
            egui_view_set_pressed(self, false);
        }
        return self->is_clickable ? 1 : 0;
    }

    hit_part = password_box_hit_test(local, self, event->location.x, event->location.y);

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        if (hit_part == EGUI_VIEW_PASSWORD_BOX_PART_NONE)
        {
            return 0;
        }
        local->pressed_part = hit_part;
        password_box_set_current_part_inner(self, hit_part, 0);
        egui_view_set_pressed(self, true);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (self->is_focusable)
        {
            egui_view_request_focus(self);
        }
#endif
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->pressed_part != EGUI_VIEW_PASSWORD_BOX_PART_NONE)
        {
            return 1;
        }
        return 0;
    case EGUI_MOTION_EVENT_ACTION_UP:
        if (local->pressed_part == EGUI_VIEW_PASSWORD_BOX_PART_NONE)
        {
            return hit_part != EGUI_VIEW_PASSWORD_BOX_PART_NONE ? 1 : 0;
        }
        if (local->pressed_part == hit_part)
        {
            if (hit_part == EGUI_VIEW_PASSWORD_BOX_PART_REVEAL && !local->read_only_mode)
            {
                egui_view_password_box_set_revealed(self, local->revealed ? 0 : 1);
            }
            else
            {
                password_box_set_current_part_inner(self, EGUI_VIEW_PASSWORD_BOX_PART_FIELD, 0);
            }
        }
        local->pressed_part = EGUI_VIEW_PASSWORD_BOX_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_PASSWORD_BOX_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif
static uint8_t password_box_handle_navigation_key_inner(egui_view_t *self, uint8_t key_code)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    uint8_t reveal_visible;

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    password_box_normalize_state(local);
    reveal_visible = password_box_is_reveal_visible(local);

    switch (key_code)
    {
    case EGUI_KEY_CODE_TAB:
        if (reveal_visible)
        {
            password_box_set_current_part_inner(
                    self, local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_FIELD ? EGUI_VIEW_PASSWORD_BOX_PART_REVEAL : EGUI_VIEW_PASSWORD_BOX_PART_FIELD, 1);
        }
        else
        {
            password_box_set_current_part_inner(self, EGUI_VIEW_PASSWORD_BOX_PART_FIELD, 1);
        }
        return 1;
    case EGUI_KEY_CODE_LEFT:
        if (local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_REVEAL)
        {
            password_box_set_current_part_inner(self, EGUI_VIEW_PASSWORD_BOX_PART_FIELD, 1);
            return 1;
        }
        egui_view_password_box_move_cursor_left(self);
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        if (local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_REVEAL)
        {
            return 1;
        }
        if (local->cursor_pos < local->text_len)
        {
            egui_view_password_box_move_cursor_right(self);
        }
        else if (reveal_visible)
        {
            password_box_set_current_part_inner(self, EGUI_VIEW_PASSWORD_BOX_PART_REVEAL, 1);
        }
        return 1;
    case EGUI_KEY_CODE_HOME:
        password_box_set_current_part_inner(self, EGUI_VIEW_PASSWORD_BOX_PART_FIELD, 0);
        egui_view_password_box_move_cursor_home(self);
        return 1;
    case EGUI_KEY_CODE_END:
        password_box_set_current_part_inner(self, EGUI_VIEW_PASSWORD_BOX_PART_FIELD, 0);
        egui_view_password_box_move_cursor_end(self);
        return 1;
    case EGUI_KEY_CODE_BACKSPACE:
        if (local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_FIELD)
        {
            egui_view_password_box_delete_char(self);
        }
        return 1;
    case EGUI_KEY_CODE_DELETE:
        if (local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_FIELD)
        {
            egui_view_password_box_delete_forward(self);
        }
        return 1;
    case EGUI_KEY_CODE_SPACE:
        if (local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_REVEAL)
        {
            egui_view_password_box_set_revealed(self, local->revealed ? 0 : 1);
            return 1;
        }
        return 0;
    case EGUI_KEY_CODE_ENTER:
        if (local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_REVEAL)
        {
            egui_view_password_box_set_revealed(self, local->revealed ? 0 : 1);
            return 1;
        }
        return 0;
    case EGUI_KEY_CODE_ESCAPE:
        if (local->revealed)
        {
            egui_view_password_box_set_revealed(self, 0);
        }
        password_box_set_current_part_inner(self, EGUI_VIEW_PASSWORD_BOX_PART_FIELD, 1);
        return 1;
    default:
        return 0;
    }
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(key_code);
    return 0;
#endif
}

uint8_t egui_view_password_box_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    return password_box_handle_navigation_key_inner(self, key_code);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_password_box_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    char ch;

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    if (event->type == EGUI_KEY_EVENT_ACTION_UP)
    {
        if (password_box_handle_navigation_key_inner(self, event->key_code))
        {
            return 1;
        }

        if (local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_FIELD)
        {
            ch = egui_key_event_to_char(event);
            if (ch != 0)
            {
                egui_view_password_box_insert_char(self, ch);
                return 1;
            }
        }
        return egui_view_on_key_event(self, event);
    }

    if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
    {
        if (event->key_code == EGUI_KEY_CODE_TAB || event->key_code == EGUI_KEY_CODE_LEFT || event->key_code == EGUI_KEY_CODE_RIGHT ||
            event->key_code == EGUI_KEY_CODE_HOME || event->key_code == EGUI_KEY_CODE_END || event->key_code == EGUI_KEY_CODE_BACKSPACE ||
            event->key_code == EGUI_KEY_CODE_DELETE || event->key_code == EGUI_KEY_CODE_ENTER || event->key_code == EGUI_KEY_CODE_ESCAPE)
        {
            return 1;
        }
        if (local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_REVEAL && event->key_code == EGUI_KEY_CODE_SPACE)
        {
            return 1;
        }
        if (local->current_part == EGUI_VIEW_PASSWORD_BOX_PART_FIELD)
        {
            ch = egui_key_event_to_char(event);
            if (ch != 0)
            {
                return 1;
            }
        }
    }
    return 0;
}
#endif

void egui_view_password_box_init(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_password_box_t);
    static const egui_view_api_t api = {
            .dispatch_touch_event = egui_view_dispatch_touch_event,
            .on_touch_event = egui_view_password_box_on_touch_event,
            .on_intercept_touch_event = egui_view_on_intercept_touch_event,
            .compute_scroll = egui_view_compute_scroll,
            .calculate_layout = egui_view_calculate_layout,
            .request_layout = egui_view_request_layout,
            .draw = egui_view_draw,
            .on_attach_to_window = egui_view_on_attach_to_window,
            .on_draw = egui_view_password_box_on_draw,
            .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
            .dispatch_key_event = egui_view_dispatch_key_event,
            .on_key_event = egui_view_password_box_on_key_event,
#endif
    };

    egui_view_init(self);
    memset((uint8_t *)local + sizeof(egui_view_t), 0, sizeof(*local) - sizeof(egui_view_t));
    self->api = &api;
    self->is_clickable = 1;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    self->is_focusable = 1;
    self->on_focus_change_listener = password_box_on_focus_change;
#endif
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->icon_font = EGUI_FONT_ICON_MS_16;
    local->label = "Password";
    local->helper = "Use at least 8 chars";
    local->placeholder = "Enter password";
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD8E0E7);
    local->text_color = EGUI_COLOR_HEX(0x1F2A35);
    local->muted_text_color = EGUI_COLOR_HEX(0x7A8794);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->current_part = EGUI_VIEW_PASSWORD_BOX_PART_FIELD;
    local->pressed_part = EGUI_VIEW_PASSWORD_BOX_PART_NONE;
    local->cursor_visible = 0;
    local->scroll_offset_x = 0;
    local->text[0] = '\0';
    local->masked_text[0] = '\0';
    egui_timer_init_timer(&local->cursor_timer, local, password_box_cursor_timer_callback);
}
