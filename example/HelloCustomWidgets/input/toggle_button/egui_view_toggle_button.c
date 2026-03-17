#include "egui_view_toggle_button.h"

static egui_view_toggle_button_t *hcw_toggle_button_local(egui_view_t *self)
{
    return (egui_view_toggle_button_t *)self;
}

static void hcw_toggle_button_apply_style(egui_view_t *self, egui_dim_t radius, egui_color_t on_color, egui_color_t off_color, egui_color_t text_color,
                                          egui_dim_t gap)
{
    egui_view_toggle_button_t *local = hcw_toggle_button_local(self);

    local->corner_radius = radius;
    local->on_color = on_color;
    local->off_color = off_color;
    local->text_color = text_color;
    local->icon_text_gap = gap;
    egui_view_invalidate(self);
}

void hcw_toggle_button_apply_standard_style(egui_view_t *self)
{
    hcw_toggle_button_apply_style(self, 10, EGUI_COLOR_HEX(0x2563EB), EGUI_COLOR_HEX(0xEAF1FB), EGUI_COLOR_WHITE, 5);
}

void hcw_toggle_button_apply_compact_style(egui_view_t *self)
{
    hcw_toggle_button_apply_style(self, 7, EGUI_COLOR_HEX(0x0C7C73), EGUI_COLOR_HEX(0xDBEAE5), EGUI_COLOR_WHITE, 3);
}

void hcw_toggle_button_apply_read_only_style(egui_view_t *self)
{
    hcw_toggle_button_apply_style(self, 7, EGUI_COLOR_HEX(0xAFB8C3), EGUI_COLOR_HEX(0xF3F6F8), EGUI_COLOR_HEX(0xF7F9FB), 3);
}

int hcw_toggle_button_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (!egui_view_get_enable(self))
    {
        return 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
        {
            egui_view_set_pressed(self, 1);
            return 1;
        }
        if (event->type == EGUI_KEY_EVENT_ACTION_UP)
        {
            egui_view_set_pressed(self, 0);
            egui_view_toggle_button_set_toggled(self, !egui_view_toggle_button_is_toggled(self));
            return 1;
        }
        return 0;
    default:
        return egui_view_on_key_event(self, event);
    }
}
