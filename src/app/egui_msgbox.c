#include <stddef.h>

#include "egui_msgbox.h"

#if EGUI_CONFIG_FUNCTION_MSGBOX

#include "background/egui_background_color.h"
#include "core/egui_core.h"

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(s_msgbox_panel_bg_param, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 6);
EGUI_BACKGROUND_PARAM_INIT(s_msgbox_panel_bg_params, &s_msgbox_panel_bg_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_msgbox_panel_bg, &s_msgbox_panel_bg_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(s_msgbox_button_bg_normal_param, EGUI_THEME_PRIMARY, EGUI_ALPHA_100, 4);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(s_msgbox_button_bg_pressed_param, EGUI_THEME_PRIMARY_DARK, EGUI_ALPHA_100, 4);
EGUI_BACKGROUND_PARAM_INIT(s_msgbox_button_bg_params, &s_msgbox_button_bg_normal_param, &s_msgbox_button_bg_pressed_param, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(s_msgbox_button_bg, &s_msgbox_button_bg_params);

static egui_msgbox_t *egui_msgbox_from_button(egui_view_t *button, uint8_t *index)
{
    uint8_t i;
    egui_view_t *panel_view;
    egui_msgbox_t *self;

    if (button == NULL || button->parent == NULL)
    {
        return NULL;
    }

    panel_view = EGUI_VIEW_OF(button->parent);
    self = (egui_msgbox_t *)((uint8_t *)panel_view - offsetof(egui_msgbox_t, panel));

    for (i = 0; i < self->button_count; i++)
    {
        if (EGUI_VIEW_OF(&self->buttons[i]) == button)
        {
            if (index != NULL)
            {
                *index = i;
            }
            return self;
        }
    }

    return NULL;
}

static void egui_msgbox_button_click_cb(egui_view_t *button)
{
    uint8_t index = 0;
    egui_msgbox_t *self = egui_msgbox_from_button(button, &index);

    if (self == NULL)
    {
        return;
    }

    if (self->button_cb != NULL)
    {
        self->button_cb(self, index, self->button_texts[index], self->user_data);
    }

    egui_msgbox_close(self);
}

static void egui_msgbox_apply_layout(egui_msgbox_t *self)
{
    egui_core_t *core;
    egui_dim_t panel_w;
    egui_dim_t panel_h;
    egui_dim_t panel_x;
    egui_dim_t panel_y;
    egui_dim_t margin;
    egui_dim_t gap;
    egui_dim_t button_h;
    egui_dim_t button_w;
    egui_region_t region;
    uint8_t i;

    if (self == NULL)
    {
        return;
    }

    core = egui_dialog_get_core(&self->dialog);
    if (core == NULL)
    {
        return;
    }

    panel_w = (egui_dim_t)((core->screen_width * 4) / 5);
    if (panel_w > 220)
    {
        panel_w = 220;
    }
    if (panel_w < 120)
    {
        panel_w = (egui_dim_t)(core->screen_width - 20);
    }

    panel_h = 136;
    if (panel_h > core->screen_height - 20)
    {
        panel_h = (egui_dim_t)(core->screen_height - 20);
    }

    panel_x = (egui_dim_t)((core->screen_width - panel_w) / 2);
    panel_y = (egui_dim_t)((core->screen_height - panel_h) / 2);
    margin = 10;
    gap = 6;
    button_h = 28;

    egui_region_init(&region, panel_x, panel_y, panel_w, panel_h);
    egui_view_layout(EGUI_VIEW_OF(&self->panel), &region);

    egui_region_init(&region, margin, 8, (egui_dim_t)(panel_w - 2 * margin), 24);
    egui_view_layout(EGUI_VIEW_OF(&self->title), &region);

    egui_region_init(&region, margin, 34, (egui_dim_t)(panel_w - 2 * margin), (egui_dim_t)(panel_h - button_h - 48));
    egui_view_layout(EGUI_VIEW_OF(&self->message), &region);

    if (self->button_count == 0)
    {
        return;
    }

    button_w = (egui_dim_t)((panel_w - 2 * margin - (self->button_count - 1) * gap) / self->button_count);
    if (button_w < 1)
    {
        button_w = 1;
    }

    for (i = 0; i < self->button_count; i++)
    {
        egui_region_init(&region, (egui_dim_t)(margin + i * (button_w + gap)), (egui_dim_t)(panel_h - button_h - margin), button_w, button_h);
        egui_view_layout(EGUI_VIEW_OF(&self->buttons[i]), &region);
    }
}

void egui_msgbox_init(egui_msgbox_t *self, egui_core_t *core)
{
    uint8_t i;

    if (self == NULL || core == NULL)
    {
        return;
    }

    egui_dialog_init(&self->dialog, core);
    self->dialog.is_cancel_on_touch_outside = 0;

    egui_view_group_init(EGUI_VIEW_OF(&self->panel), core);
    egui_view_set_background(EGUI_VIEW_OF(&self->panel), (egui_background_t *)&s_msgbox_panel_bg);
    egui_view_set_view_name(EGUI_VIEW_OF(&self->panel), "egui_msgbox_panel");

    egui_view_label_init(EGUI_VIEW_OF(&self->title), core);
    egui_view_label_set_text(EGUI_VIEW_OF(&self->title), NULL);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&self->title), EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&self->title), EGUI_ALIGN_CENTER);

    egui_view_label_init(EGUI_VIEW_OF(&self->message), core);
    egui_view_label_set_text(EGUI_VIEW_OF(&self->message), NULL);
    egui_view_label_set_font_color(EGUI_VIEW_OF(&self->message), EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&self->message), EGUI_ALIGN_CENTER);
#if EGUI_CONFIG_FUNCTION_LABEL_LONG_MODE && EGUI_CONFIG_FUNCTION_LABEL_WORD_WRAP
    egui_view_label_set_long_mode(EGUI_VIEW_OF(&self->message), EGUI_LABEL_LONG_WRAP);
#endif

    egui_view_group_add_child(EGUI_VIEW_OF(&self->panel), EGUI_VIEW_OF(&self->title));
    egui_view_group_add_child(EGUI_VIEW_OF(&self->panel), EGUI_VIEW_OF(&self->message));

    for (i = 0; i < EGUI_CONFIG_MSGBOX_MAX_BUTTONS; i++)
    {
        egui_view_button_init(EGUI_VIEW_OF(&self->buttons[i]), core);
        egui_view_set_background(EGUI_VIEW_OF(&self->buttons[i]), (egui_background_t *)&s_msgbox_button_bg);
        egui_view_label_set_font_color(EGUI_VIEW_OF(&self->buttons[i]), EGUI_COLOR_WHITE, EGUI_ALPHA_100);
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&self->buttons[i]), egui_msgbox_button_click_cb);
        egui_view_set_visible(EGUI_VIEW_OF(&self->buttons[i]), 0);
        self->button_texts[i] = NULL;
        egui_view_group_add_child(EGUI_VIEW_OF(&self->panel), EGUI_VIEW_OF(&self->buttons[i]));
    }

    self->button_cb = NULL;
    self->user_data = NULL;
    self->button_count = 0;

    egui_dialog_add_view(&self->dialog, EGUI_VIEW_OF(&self->panel));
    egui_msgbox_apply_layout(self);
}

void egui_msgbox_set_text(egui_msgbox_t *self, const char *title, const char *message)
{
    if (self == NULL)
    {
        return;
    }

    egui_view_label_set_text(EGUI_VIEW_OF(&self->title), title);
    egui_view_label_set_text(EGUI_VIEW_OF(&self->message), message);
}

int egui_msgbox_set_buttons(egui_msgbox_t *self, const char *const *texts, uint8_t count, egui_msgbox_button_cb_t cb, void *user_data)
{
    uint8_t i;

    if (self == NULL || (count > 0 && texts == NULL) || count > EGUI_CONFIG_MSGBOX_MAX_BUTTONS)
    {
        return -1;
    }

    self->button_count = count;
    self->button_cb = cb;
    self->user_data = user_data;

    for (i = 0; i < EGUI_CONFIG_MSGBOX_MAX_BUTTONS; i++)
    {
        if (i < count)
        {
            self->button_texts[i] = texts[i];
            egui_view_label_set_text(EGUI_VIEW_OF(&self->buttons[i]), texts[i]);
            egui_view_set_visible(EGUI_VIEW_OF(&self->buttons[i]), 1);
        }
        else
        {
            self->button_texts[i] = NULL;
            egui_view_label_set_text(EGUI_VIEW_OF(&self->buttons[i]), NULL);
            egui_view_set_visible(EGUI_VIEW_OF(&self->buttons[i]), 0);
        }
    }

    egui_msgbox_apply_layout(self);
    return 0;
}

void egui_msgbox_show(egui_msgbox_t *self, egui_activity_t *activity)
{
    if (self == NULL)
    {
        return;
    }

    egui_msgbox_apply_layout(self);
    egui_dialog_start(&self->dialog, activity);
}

void egui_msgbox_close(egui_msgbox_t *self)
{
    if (self == NULL)
    {
        return;
    }

    egui_dialog_finish(&self->dialog);
}

egui_dialog_t *egui_msgbox_get_dialog(egui_msgbox_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }

    return &self->dialog;
}

egui_view_t *egui_msgbox_get_button(egui_msgbox_t *self, uint8_t index)
{
    if (self == NULL || index >= self->button_count)
    {
        return NULL;
    }

    return EGUI_VIEW_OF(&self->buttons[index]);
}

#endif /* EGUI_CONFIG_FUNCTION_MSGBOX */
