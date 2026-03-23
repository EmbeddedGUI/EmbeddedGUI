#include "egui.h"
#include <string.h>
#include "uicode.h"
#include "demo_virtual_stage_internal.h"

static egui_view_t *demo_adapter_create_view(void *adapter_context, uint16_t view_type)
{
    EGUI_UNUSED(adapter_context);

    switch (view_type)
    {
    case DEMO_VIEW_TYPE_BUTTON:
    {
        demo_virtual_button_view_t *button_view = (demo_virtual_button_view_t *)egui_malloc(sizeof(demo_virtual_button_view_t));
        if (button_view == NULL)
        {
            return NULL;
        }

        memset(button_view, 0, sizeof(*button_view));
        button_view->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
        egui_view_button_init(EGUI_VIEW_OF(&button_view->button));
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_view->button), demo_button_click_cb);
        egui_view_label_set_align_type(EGUI_VIEW_OF(&button_view->button), EGUI_ALIGN_CENTER);
        egui_view_set_padding(EGUI_VIEW_OF(&button_view->button), 4, 4, 4, 4);
        egui_view_set_background(EGUI_VIEW_OF(&button_view->button), demo_get_button_background(DEMO_BUTTON_STYLE_NEUTRAL));
        return EGUI_VIEW_OF(&button_view->button);
    }
    case DEMO_VIEW_TYPE_TEXTINPUT:
    {
        demo_virtual_textinput_view_t *textinput_view = (demo_virtual_textinput_view_t *)egui_malloc(sizeof(demo_virtual_textinput_view_t));
        if (textinput_view == NULL)
        {
            return NULL;
        }

        memset(textinput_view, 0, sizeof(*textinput_view));
        textinput_view->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
        egui_view_textinput_init(EGUI_VIEW_OF(&textinput_view->textinput));
        egui_view_textinput_set_font(EGUI_VIEW_OF(&textinput_view->textinput), DEMO_FONT_BODY);
        egui_view_textinput_set_text_color(EGUI_VIEW_OF(&textinput_view->textinput), DEMO_COLOR_TEXT_PRIMARY, EGUI_ALPHA_100);
        egui_view_textinput_set_placeholder_color(EGUI_VIEW_OF(&textinput_view->textinput), DEMO_COLOR_TEXT_MUTED, EGUI_ALPHA_100);
        egui_view_textinput_set_cursor_color(EGUI_VIEW_OF(&textinput_view->textinput), EGUI_COLOR_HEX(0x3A6EA5));
        egui_view_textinput_set_on_text_changed(EGUI_VIEW_OF(&textinput_view->textinput), demo_textinput_changed);
        egui_view_override_api_on_focus_changed(EGUI_VIEW_OF(&textinput_view->textinput), &textinput_view->focus_api, demo_textinput_focus_changed);
        egui_view_set_padding(EGUI_VIEW_OF(&textinput_view->textinput), 10, 10, 7, 7);
        egui_view_set_background(EGUI_VIEW_OF(&textinput_view->textinput), demo_get_textinput_background(0U));
        return EGUI_VIEW_OF(&textinput_view->textinput);
    }
    case DEMO_VIEW_TYPE_SWITCH:
    {
        demo_virtual_switch_view_t *switch_view = (demo_virtual_switch_view_t *)egui_malloc(sizeof(demo_virtual_switch_view_t));
        if (switch_view == NULL)
        {
            return NULL;
        }

        memset(switch_view, 0, sizeof(*switch_view));
        switch_view->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
        egui_view_switch_init(EGUI_VIEW_OF(&switch_view->switch_view));
        egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&switch_view->switch_view), demo_switch_changed);
        egui_view_switch_set_state_icons(EGUI_VIEW_OF(&switch_view->switch_view), EGUI_ICON_MS_DONE, EGUI_ICON_MS_CROSS);
        egui_view_switch_set_icon_font(EGUI_VIEW_OF(&switch_view->switch_view), EGUI_FONT_ICON_MS_16);
        return EGUI_VIEW_OF(&switch_view->switch_view);
    }
    case DEMO_VIEW_TYPE_CHECKBOX:
    {
        demo_virtual_checkbox_view_t *checkbox_view = (demo_virtual_checkbox_view_t *)egui_malloc(sizeof(demo_virtual_checkbox_view_t));
        if (checkbox_view == NULL)
        {
            return NULL;
        }

        memset(checkbox_view, 0, sizeof(*checkbox_view));
        checkbox_view->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
        egui_view_checkbox_init(EGUI_VIEW_OF(&checkbox_view->checkbox));
        egui_view_checkbox_set_on_checked_listener(EGUI_VIEW_OF(&checkbox_view->checkbox), demo_checkbox_changed);
        egui_view_checkbox_set_font(EGUI_VIEW_OF(&checkbox_view->checkbox), DEMO_FONT_CAP);
        egui_view_checkbox_set_text_color(EGUI_VIEW_OF(&checkbox_view->checkbox), DEMO_COLOR_TEXT_PRIMARY);
        egui_view_checkbox_set_icon_text_gap(EGUI_VIEW_OF(&checkbox_view->checkbox), 4);
        return EGUI_VIEW_OF(&checkbox_view->checkbox);
    }
    case DEMO_VIEW_TYPE_RADIO:
    {
        demo_virtual_radio_view_t *radio_view = (demo_virtual_radio_view_t *)egui_malloc(sizeof(demo_virtual_radio_view_t));
        if (radio_view == NULL)
        {
            return NULL;
        }

        memset(radio_view, 0, sizeof(*radio_view));
        radio_view->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
        egui_view_radio_button_init(EGUI_VIEW_OF(&radio_view->radio));
        egui_view_radio_button_set_font(EGUI_VIEW_OF(&radio_view->radio), DEMO_FONT_CAP);
        egui_view_radio_button_set_text_color(EGUI_VIEW_OF(&radio_view->radio), DEMO_COLOR_TEXT_PRIMARY);
        egui_view_radio_button_set_icon_text_gap(EGUI_VIEW_OF(&radio_view->radio), 4);
        egui_view_set_on_click_listener(EGUI_VIEW_OF(&radio_view->radio), demo_radio_click_cb);
        return EGUI_VIEW_OF(&radio_view->radio);
    }
    case DEMO_VIEW_TYPE_SLIDER:
    {
        demo_virtual_slider_view_t *slider_view = (demo_virtual_slider_view_t *)egui_malloc(sizeof(demo_virtual_slider_view_t));
        if (slider_view == NULL)
        {
            return NULL;
        }

        memset(slider_view, 0, sizeof(*slider_view));
        slider_view->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
        egui_view_slider_init(EGUI_VIEW_OF(&slider_view->slider));
        egui_view_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&slider_view->slider), demo_slider_changed);
        return EGUI_VIEW_OF(&slider_view->slider);
    }
    case DEMO_VIEW_TYPE_TOGGLE:
    {
        demo_virtual_toggle_view_t *toggle_view = (demo_virtual_toggle_view_t *)egui_malloc(sizeof(demo_virtual_toggle_view_t));
        if (toggle_view == NULL)
        {
            return NULL;
        }

        memset(toggle_view, 0, sizeof(*toggle_view));
        toggle_view->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
        egui_view_toggle_button_init(EGUI_VIEW_OF(&toggle_view->toggle));
        egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&toggle_view->toggle), demo_toggle_changed);
        egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&toggle_view->toggle), EGUI_FONT_ICON_MS_16);
        egui_view_toggle_button_set_icon_text_gap(EGUI_VIEW_OF(&toggle_view->toggle), 4);
        egui_view_toggle_button_set_text_color(EGUI_VIEW_OF(&toggle_view->toggle), EGUI_COLOR_WHITE);
        return EGUI_VIEW_OF(&toggle_view->toggle);
    }
    case DEMO_VIEW_TYPE_NUMBER_PICKER:
    {
        demo_virtual_picker_view_t *picker_view = (demo_virtual_picker_view_t *)egui_malloc(sizeof(demo_virtual_picker_view_t));
        if (picker_view == NULL)
        {
            return NULL;
        }

        memset(picker_view, 0, sizeof(*picker_view));
        picker_view->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
        egui_view_number_picker_init(EGUI_VIEW_OF(&picker_view->picker));
        egui_view_number_picker_set_on_value_changed_listener(EGUI_VIEW_OF(&picker_view->picker), demo_picker_changed);
        egui_view_number_picker_set_button_icons(EGUI_VIEW_OF(&picker_view->picker), EGUI_ICON_MS_ADD, EGUI_ICON_MS_REMOVE);
        egui_view_number_picker_set_icon_font(EGUI_VIEW_OF(&picker_view->picker), EGUI_FONT_ICON_MS_16);
        picker_view->picker.font = DEMO_FONT_BODY;
        picker_view->picker.text_color = DEMO_COLOR_TEXT_PRIMARY;
        picker_view->picker.button_color = DEMO_COLOR_TEXT_SECONDARY;
        return EGUI_VIEW_OF(&picker_view->picker);
    }
    case DEMO_VIEW_TYPE_COMBOBOX:
    {
        demo_virtual_combobox_view_t *combobox_view = (demo_virtual_combobox_view_t *)egui_malloc(sizeof(demo_virtual_combobox_view_t));
        if (combobox_view == NULL)
        {
            return NULL;
        }

        memset(combobox_view, 0, sizeof(*combobox_view));
        combobox_view->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
        egui_view_combobox_init(EGUI_VIEW_OF(&combobox_view->combobox));
        egui_view_combobox_set_on_selected_listener(EGUI_VIEW_OF(&combobox_view->combobox), demo_combobox_selected);
        egui_view_combobox_set_font(EGUI_VIEW_OF(&combobox_view->combobox), DEMO_FONT_CAP);
        egui_view_combobox_set_icon_font(EGUI_VIEW_OF(&combobox_view->combobox), EGUI_FONT_ICON_MS_16);
        egui_view_combobox_set_icon_text_gap(EGUI_VIEW_OF(&combobox_view->combobox), 3);
        egui_view_combobox_set_arrow_icons(EGUI_VIEW_OF(&combobox_view->combobox), EGUI_ICON_MS_EXPAND_MORE, EGUI_ICON_MS_EXPAND_LESS);
        egui_view_combobox_set_max_visible_items(EGUI_VIEW_OF(&combobox_view->combobox), 2);
        return EGUI_VIEW_OF(&combobox_view->combobox);
    }
    case DEMO_VIEW_TYPE_ROLLER:
    {
        demo_virtual_roller_view_t *roller_view = (demo_virtual_roller_view_t *)egui_malloc(sizeof(demo_virtual_roller_view_t));
        if (roller_view == NULL)
        {
            return NULL;
        }

        memset(roller_view, 0, sizeof(*roller_view));
        roller_view->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
        egui_view_roller_init(EGUI_VIEW_OF(&roller_view->roller));
        egui_view_roller_set_on_selected_listener(EGUI_VIEW_OF(&roller_view->roller), demo_roller_selected);
        roller_view->roller.font = DEMO_FONT_CAP;
        roller_view->roller.text_color = DEMO_COLOR_TEXT_SECONDARY;
        roller_view->roller.selected_text_color = DEMO_COLOR_TEXT_PRIMARY;
        roller_view->roller.highlight_color = EGUI_COLOR_HEX(0xDCEAFE);
        return EGUI_VIEW_OF(&roller_view->roller);
    }
    case DEMO_VIEW_TYPE_SEGMENTED:
    {
        demo_virtual_segmented_view_t *segmented_view = (demo_virtual_segmented_view_t *)egui_malloc(sizeof(demo_virtual_segmented_view_t));
        if (segmented_view == NULL)
        {
            return NULL;
        }

        memset(segmented_view, 0, sizeof(*segmented_view));
        segmented_view->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
        egui_view_segmented_control_init(EGUI_VIEW_OF(&segmented_view->segmented));
        egui_view_segmented_control_set_on_segment_changed_listener(EGUI_VIEW_OF(&segmented_view->segmented), demo_segment_changed);
        egui_view_segmented_control_set_font(EGUI_VIEW_OF(&segmented_view->segmented), DEMO_FONT_CAP);
        egui_view_segmented_control_set_corner_radius(EGUI_VIEW_OF(&segmented_view->segmented), 8);
        egui_view_segmented_control_set_segment_gap(EGUI_VIEW_OF(&segmented_view->segmented), 2);
        egui_view_segmented_control_set_horizontal_padding(EGUI_VIEW_OF(&segmented_view->segmented), 1);
        return EGUI_VIEW_OF(&segmented_view->segmented);
    }
    case DEMO_VIEW_TYPE_BUTTON_MATRIX:
    {
        demo_virtual_button_matrix_view_t *matrix_view = (demo_virtual_button_matrix_view_t *)egui_malloc(sizeof(demo_virtual_button_matrix_view_t));
        if (matrix_view == NULL)
        {
            return NULL;
        }

        memset(matrix_view, 0, sizeof(*matrix_view));
        matrix_view->stable_id = EGUI_VIEW_VIRTUAL_STAGE_INVALID_ID;
        egui_view_button_matrix_init(EGUI_VIEW_OF(&matrix_view->matrix));
        egui_view_button_matrix_set_on_click(EGUI_VIEW_OF(&matrix_view->matrix), demo_button_matrix_clicked);
        egui_view_button_matrix_set_selection_enabled(EGUI_VIEW_OF(&matrix_view->matrix), 1);
        egui_view_button_matrix_set_font(EGUI_VIEW_OF(&matrix_view->matrix), DEMO_FONT_CAP);
        egui_view_button_matrix_set_gap(EGUI_VIEW_OF(&matrix_view->matrix), 2);
        egui_view_button_matrix_set_corner_radius(EGUI_VIEW_OF(&matrix_view->matrix), 7);
        return EGUI_VIEW_OF(&matrix_view->matrix);
    }
    default:
        return NULL;
    }
}

static void demo_adapter_destroy_view(void *adapter_context, egui_view_t *view, uint16_t view_type)
{
    demo_virtual_stage_context_t *ctx = (demo_virtual_stage_context_t *)adapter_context;

    if (view_type == DEMO_VIEW_TYPE_TEXTINPUT)
    {
        demo_virtual_textinput_view_t *textinput_view = (demo_virtual_textinput_view_t *)view;
        egui_timer_stop_timer(&textinput_view->textinput.cursor_timer);
    }

    ctx->destroy_count++;
    egui_free(view);
}

static void demo_adapter_bind_view(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    demo_virtual_stage_context_t *ctx = (demo_virtual_stage_context_t *)adapter_context;
    demo_virtual_node_t *node = &ctx->nodes[index];

    ctx->bind_count++;

    if (node->desc.view_type == DEMO_VIEW_TYPE_BUTTON)
    {
        demo_virtual_button_view_t *button_view = (demo_virtual_button_view_t *)view;
        uint8_t style = demo_get_button_style(stable_id);
        egui_color_t fill;
        egui_color_t border;
        egui_color_t text_color;

        button_view->stable_id = stable_id;
        demo_get_button_label(stable_id, button_view->label, sizeof(button_view->label));
        demo_get_button_style_colors(style, &fill, &border, &text_color);

        EGUI_UNUSED(fill);
        EGUI_UNUSED(border);

        egui_view_label_set_text(view, button_view->label);
        egui_view_label_set_font(view, node->kind == DEMO_NODE_KIND_MACHINE ? DEMO_FONT_BUTTON : DEMO_FONT_CAP);
        egui_view_label_set_font_color(view, text_color, EGUI_ALPHA_100);
        egui_view_set_background(view, demo_get_button_background(style));
        return;
    }

    if (node->desc.view_type == DEMO_VIEW_TYPE_TEXTINPUT)
    {
        demo_virtual_textinput_view_t *textinput_view = (demo_virtual_textinput_view_t *)view;

        textinput_view->stable_id = stable_id;
        egui_view_textinput_set_placeholder(view, demo_get_input_placeholder(stable_id));
        egui_view_textinput_set_max_length(view, stable_id == DEMO_SEARCH_INPUT_ID ? 18U : 24U);
        egui_view_set_background(view, demo_get_textinput_background(view->is_focused ? 1U : 0U));
    }

    if (node->desc.view_type == DEMO_VIEW_TYPE_SWITCH)
    {
        demo_virtual_switch_view_t *switch_view = (demo_virtual_switch_view_t *)view;

        switch_view->stable_id = stable_id;
        egui_view_switch_set_state_icons(view, EGUI_ICON_MS_DONE, EGUI_ICON_MS_CROSS);
        egui_view_switch_set_icon_font(view, EGUI_FONT_ICON_MS_16);
        return;
    }

    if (node->desc.view_type == DEMO_VIEW_TYPE_CHECKBOX)
    {
        demo_virtual_checkbox_view_t *checkbox_view = (demo_virtual_checkbox_view_t *)view;

        checkbox_view->stable_id = stable_id;
        egui_view_checkbox_set_text(view, node->title);
        egui_view_checkbox_set_font(view, DEMO_FONT_CAP);
        egui_view_checkbox_set_text_color(view, DEMO_COLOR_TEXT_PRIMARY);
        egui_view_checkbox_set_icon_text_gap(view, 4);
        return;
    }

    if (node->desc.view_type == DEMO_VIEW_TYPE_RADIO)
    {
        demo_virtual_radio_view_t *radio_view = (demo_virtual_radio_view_t *)view;

        radio_view->stable_id = stable_id;
        egui_view_radio_button_set_text(view, node->title);
        egui_view_radio_button_set_font(view, DEMO_FONT_CAP);
        egui_view_radio_button_set_text_color(view, DEMO_COLOR_TEXT_PRIMARY);
        egui_view_radio_button_set_icon_text_gap(view, 4);
        return;
    }

    if (node->desc.view_type == DEMO_VIEW_TYPE_SLIDER)
    {
        demo_virtual_slider_view_t *slider_view = (demo_virtual_slider_view_t *)view;
        egui_view_slider_t *slider = (egui_view_slider_t *)view;

        slider_view->stable_id = stable_id;
        slider->track_color = EGUI_COLOR_HEX(0xD6E0E8);
        slider->active_color = node->data_index == 0U ? EGUI_COLOR_HEX(0x3A6EA5) : EGUI_COLOR_HEX(0x167C88);
        slider->thumb_color = EGUI_COLOR_WHITE;
        return;
    }

    if (node->desc.view_type == DEMO_VIEW_TYPE_TOGGLE)
    {
        demo_virtual_toggle_view_t *toggle_view = (demo_virtual_toggle_view_t *)view;
        egui_view_toggle_button_t *toggle = (egui_view_toggle_button_t *)view;

        toggle_view->stable_id = stable_id;
        egui_view_toggle_button_set_text(view, node->title);
        egui_view_toggle_button_set_icon(view, demo_get_toggle_icon(stable_id));
        egui_view_toggle_button_set_text_color(view, EGUI_COLOR_WHITE);
        toggle->on_color = node->data_index == 0U ? EGUI_COLOR_HEX(0x2E8E58) : EGUI_COLOR_HEX(0x3A6EA5);
        toggle->off_color = EGUI_COLOR_HEX(0xA9B9C6);
        return;
    }

    if (node->desc.view_type == DEMO_VIEW_TYPE_NUMBER_PICKER)
    {
        demo_virtual_picker_view_t *picker_view = (demo_virtual_picker_view_t *)view;

        picker_view->stable_id = stable_id;
        if (stable_id == DEMO_PICKER_BATCH_ID)
        {
            egui_view_number_picker_set_range(view, 10, 40);
            egui_view_number_picker_set_step(view, 2);
        }
        else
        {
            egui_view_number_picker_set_range(view, 1, 8);
            egui_view_number_picker_set_step(view, 1);
        }
        return;
    }

    if (node->desc.view_type == DEMO_VIEW_TYPE_COMBOBOX)
    {
        demo_virtual_combobox_view_t *combobox_view = (demo_virtual_combobox_view_t *)view;
        uint8_t item_count = 0;
        const char **items = demo_get_combobox_items(stable_id, &item_count);

        combobox_view->stable_id = stable_id;
        egui_view_combobox_set_items(view, items, item_count);
        egui_view_combobox_set_font(view, DEMO_FONT_CAP);
        egui_view_combobox_set_icon_font(view, EGUI_FONT_ICON_MS_16);
        egui_view_combobox_set_max_visible_items(view, 2);
        return;
    }

    if (node->desc.view_type == DEMO_VIEW_TYPE_ROLLER)
    {
        demo_virtual_roller_view_t *roller_view = (demo_virtual_roller_view_t *)view;
        uint8_t item_count = 0;
        const char **items = demo_get_roller_items(stable_id, &item_count);

        roller_view->stable_id = stable_id;
        egui_view_roller_set_items(view, items, item_count);
        ((egui_view_roller_t *)view)->font = DEMO_FONT_CAP;
        ((egui_view_roller_t *)view)->text_color = DEMO_COLOR_TEXT_SECONDARY;
        ((egui_view_roller_t *)view)->selected_text_color = DEMO_COLOR_TEXT_PRIMARY;
        ((egui_view_roller_t *)view)->highlight_color = EGUI_COLOR_HEX(0xDCEAFE);
        return;
    }

    if (node->desc.view_type == DEMO_VIEW_TYPE_SEGMENTED)
    {
        demo_virtual_segmented_view_t *segmented_view = (demo_virtual_segmented_view_t *)view;
        uint8_t item_count = 0;
        const char **items = demo_get_segment_items(stable_id, &item_count);
        egui_color_t active = stable_id == DEMO_SEGMENT_ROUTE_ID ? EGUI_COLOR_HEX(0x3A6EA5) : EGUI_COLOR_HEX(0x167C88);

        segmented_view->stable_id = stable_id;
        egui_view_segmented_control_set_segments(view, items, item_count);
        egui_view_segmented_control_set_font(view, DEMO_FONT_CAP);
        egui_view_segmented_control_set_bg_color(view, EGUI_COLOR_HEX(0xE8EEF4));
        egui_view_segmented_control_set_selected_bg_color(view, active);
        egui_view_segmented_control_set_text_color(view, DEMO_COLOR_TEXT_SECONDARY);
        egui_view_segmented_control_set_selected_text_color(view, EGUI_COLOR_WHITE);
        egui_view_segmented_control_set_border_color(view, EGUI_COLOR_HEX(0xC8D4DE));
        return;
    }

    if (node->desc.view_type == DEMO_VIEW_TYPE_BUTTON_MATRIX)
    {
        demo_virtual_button_matrix_view_t *matrix_view = (demo_virtual_button_matrix_view_t *)view;

        matrix_view->stable_id = stable_id;
        egui_view_button_matrix_set_labels(view, demo_quick_matrix_titles, (uint8_t)EGUI_ARRAY_SIZE(demo_quick_matrix_titles), 4);
        egui_view_button_matrix_set_font(view, DEMO_FONT_CAP);
        egui_view_button_matrix_set_gap(view, 2);
        egui_view_button_matrix_set_corner_radius(view, 7);
        egui_view_button_matrix_set_btn_color(view, EGUI_COLOR_HEX(0xE8EEF4));
        egui_view_button_matrix_set_btn_pressed_color(view, EGUI_COLOR_HEX(0xD6E5F5));
        egui_view_button_matrix_set_border_color(view, EGUI_COLOR_HEX(0xC8D4DE));
        egui_view_button_matrix_set_text_color(view, DEMO_COLOR_TEXT_PRIMARY);
        return;
    }

    EGUI_UNUSED(desc);
}

static void demo_adapter_save_state(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    demo_virtual_stage_context_t *ctx = (demo_virtual_stage_context_t *)adapter_context;
    int widget_index;

    EGUI_UNUSED(index);

    if (desc == NULL)
    {
        return;
    }

    switch (desc->view_type)
    {
    case DEMO_VIEW_TYPE_TEXTINPUT:
        demo_set_input_text(stable_id, egui_view_textinput_get_text(view));
        break;
    case DEMO_VIEW_TYPE_SWITCH:
        widget_index = demo_get_switch_index(stable_id);
        if (widget_index >= 0)
        {
            ctx->switch_enabled[widget_index] = ((egui_view_switch_t *)view)->is_checked;
        }
        break;
    case DEMO_VIEW_TYPE_CHECKBOX:
        widget_index = demo_get_checkbox_index(stable_id);
        if (widget_index >= 0)
        {
            ctx->checkbox_enabled[widget_index] = ((egui_view_checkbox_t *)view)->is_checked;
        }
        break;
    case DEMO_VIEW_TYPE_RADIO:
        widget_index = demo_get_radio_index(stable_id);
        if (widget_index >= 0 && ((egui_view_radio_button_t *)view)->is_checked)
        {
            ctx->radio_selected_index = (uint8_t)widget_index;
        }
        break;
    case DEMO_VIEW_TYPE_SLIDER:
        widget_index = demo_get_slider_index(stable_id);
        if (widget_index >= 0)
        {
            ctx->slider_value[widget_index] = egui_view_slider_get_value(view);
        }
        break;
    case DEMO_VIEW_TYPE_TOGGLE:
        widget_index = demo_get_toggle_index(stable_id);
        if (widget_index >= 0)
        {
            ctx->toggle_enabled[widget_index] = egui_view_toggle_button_is_toggled(view);
        }
        break;
    case DEMO_VIEW_TYPE_NUMBER_PICKER:
        widget_index = demo_get_picker_index(stable_id);
        if (widget_index >= 0)
        {
            ctx->picker_value[widget_index] = egui_view_number_picker_get_value(view);
        }
        break;
    case DEMO_VIEW_TYPE_COMBOBOX:
        widget_index = demo_get_combobox_index(stable_id);
        if (widget_index >= 0)
        {
            ctx->combobox_index[widget_index] = egui_view_combobox_get_current_index(view);
        }
        break;
    case DEMO_VIEW_TYPE_ROLLER:
        widget_index = demo_get_roller_index(stable_id);
        if (widget_index >= 0)
        {
            ctx->roller_index[widget_index] = egui_view_roller_get_current_index(view);
        }
        break;
    case DEMO_VIEW_TYPE_SEGMENTED:
        widget_index = demo_get_segment_index(stable_id);
        if (widget_index >= 0)
        {
            ctx->segment_index[widget_index] = egui_view_segmented_control_get_current_index(view);
        }
        break;
    case DEMO_VIEW_TYPE_BUTTON_MATRIX:
    {
        uint8_t selected_index = egui_view_button_matrix_get_selected_index(view);

        if (selected_index < EGUI_ARRAY_SIZE(demo_quick_matrix_titles))
        {
            ctx->quick_matrix_index = selected_index;
        }
        break;
    }
    default:
        break;
    }
}

static void demo_adapter_restore_state(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id, const egui_virtual_stage_node_desc_t *desc)
{
    demo_virtual_stage_context_t *ctx = (demo_virtual_stage_context_t *)adapter_context;
    int widget_index;

    EGUI_UNUSED(index);

    if (desc == NULL)
    {
        return;
    }

    switch (desc->view_type)
    {
    case DEMO_VIEW_TYPE_TEXTINPUT:
        egui_view_textinput_set_text(view, demo_get_input_text(stable_id));
        break;
    case DEMO_VIEW_TYPE_SWITCH:
        widget_index = demo_get_switch_index(stable_id);
        if (widget_index >= 0)
        {
            ((egui_view_switch_t *)view)->is_checked = ctx->switch_enabled[widget_index];
            egui_view_invalidate(view);
        }
        break;
    case DEMO_VIEW_TYPE_CHECKBOX:
        widget_index = demo_get_checkbox_index(stable_id);
        if (widget_index >= 0)
        {
            ((egui_view_checkbox_t *)view)->is_checked = ctx->checkbox_enabled[widget_index];
            egui_view_invalidate(view);
        }
        break;
    case DEMO_VIEW_TYPE_RADIO:
        widget_index = demo_get_radio_index(stable_id);
        if (widget_index >= 0)
        {
            ((egui_view_radio_button_t *)view)->is_checked = ctx->radio_selected_index == (uint8_t)widget_index ? 1U : 0U;
            egui_view_invalidate(view);
        }
        break;
    case DEMO_VIEW_TYPE_SLIDER:
        widget_index = demo_get_slider_index(stable_id);
        if (widget_index >= 0)
        {
            ((egui_view_slider_t *)view)->value = ctx->slider_value[widget_index];
            ((egui_view_slider_t *)view)->is_dragging = 0;
            egui_view_invalidate(view);
        }
        break;
    case DEMO_VIEW_TYPE_TOGGLE:
        widget_index = demo_get_toggle_index(stable_id);
        if (widget_index >= 0)
        {
            ((egui_view_toggle_button_t *)view)->is_toggled = ctx->toggle_enabled[widget_index];
            egui_view_toggle_button_set_icon(view, demo_get_toggle_icon(stable_id));
            egui_view_invalidate(view);
        }
        break;
    case DEMO_VIEW_TYPE_NUMBER_PICKER:
        widget_index = demo_get_picker_index(stable_id);
        if (widget_index >= 0)
        {
            ((egui_view_number_picker_t *)view)->value = ctx->picker_value[widget_index];
            egui_view_invalidate(view);
        }
        break;
    case DEMO_VIEW_TYPE_COMBOBOX:
        widget_index = demo_get_combobox_index(stable_id);
        if (widget_index >= 0)
        {
            egui_view_combobox_set_current_index(view, ctx->combobox_index[widget_index]);
            egui_view_combobox_collapse(view);
        }
        break;
    case DEMO_VIEW_TYPE_ROLLER:
        widget_index = demo_get_roller_index(stable_id);
        if (widget_index >= 0)
        {
            egui_view_roller_set_current_index(view, ctx->roller_index[widget_index]);
            egui_view_invalidate(view);
        }
        break;
    case DEMO_VIEW_TYPE_SEGMENTED:
        widget_index = demo_get_segment_index(stable_id);
        if (widget_index >= 0)
        {
            egui_view_segmented_control_set_current_index(view, ctx->segment_index[widget_index]);
        }
        break;
    case DEMO_VIEW_TYPE_BUTTON_MATRIX:
        egui_view_button_matrix_set_selected_index(view, ctx->quick_matrix_index);
        break;
    default:
        break;
    }
}

static uint8_t demo_adapter_hit_test(void *adapter_context, uint32_t index, const egui_virtual_stage_node_desc_t *desc, const egui_region_t *screen_region,
                                     egui_dim_t screen_x, egui_dim_t screen_y)
{
    egui_view_t *live_view;

    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(index);

    if ((desc->flags & EGUI_VIRTUAL_STAGE_NODE_FLAG_INTERACTIVE) == 0)
    {
        return 0;
    }

    if (desc->view_type == DEMO_VIEW_TYPE_COMBOBOX)
    {
        live_view = demo_find_live_view(desc->stable_id);
        if (live_view != NULL && egui_view_combobox_is_expanded(live_view))
        {
            return egui_region_pt_in_rect(&live_view->region_screen, screen_x, screen_y) ? 1U : 0U;
        }
    }

    return egui_region_pt_in_rect(screen_region, screen_x, screen_y) ? 1U : 0U;
}

static uint8_t demo_adapter_should_keep_alive(void *adapter_context, egui_view_t *view, uint32_t index, uint32_t stable_id,
                                              const egui_virtual_stage_node_desc_t *desc)
{
    EGUI_UNUSED(adapter_context);
    EGUI_UNUSED(index);
    EGUI_UNUSED(stable_id);

    if (desc != NULL && desc->view_type == DEMO_VIEW_TYPE_COMBOBOX)
    {
        return egui_view_combobox_is_expanded(view);
    }

    return 0;
}

EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_STATEFUL_CONST_INIT(demo_stage_ops, demo_adapter_create_view, demo_adapter_destroy_view, demo_adapter_bind_view,
                                                      demo_adapter_save_state, demo_adapter_restore_state, demo_adapter_draw_node, demo_adapter_hit_test,
                                                      demo_adapter_should_keep_alive);

uint16_t demo_get_view_instance_size(uint16_t view_type)
{
    switch (view_type)
    {
    case DEMO_VIEW_TYPE_BUTTON:
        return (uint16_t)sizeof(demo_virtual_button_view_t);
    case DEMO_VIEW_TYPE_TEXTINPUT:
        return (uint16_t)sizeof(demo_virtual_textinput_view_t);
    case DEMO_VIEW_TYPE_SWITCH:
        return (uint16_t)sizeof(demo_virtual_switch_view_t);
    case DEMO_VIEW_TYPE_CHECKBOX:
        return (uint16_t)sizeof(demo_virtual_checkbox_view_t);
    case DEMO_VIEW_TYPE_RADIO:
        return (uint16_t)sizeof(demo_virtual_radio_view_t);
    case DEMO_VIEW_TYPE_SLIDER:
        return (uint16_t)sizeof(demo_virtual_slider_view_t);
    case DEMO_VIEW_TYPE_TOGGLE:
        return (uint16_t)sizeof(demo_virtual_toggle_view_t);
    case DEMO_VIEW_TYPE_NUMBER_PICKER:
        return (uint16_t)sizeof(demo_virtual_picker_view_t);
    case DEMO_VIEW_TYPE_COMBOBOX:
        return (uint16_t)sizeof(demo_virtual_combobox_view_t);
    case DEMO_VIEW_TYPE_ROLLER:
        return (uint16_t)sizeof(demo_virtual_roller_view_t);
    case DEMO_VIEW_TYPE_SEGMENTED:
        return (uint16_t)sizeof(demo_virtual_segmented_view_t);
    case DEMO_VIEW_TYPE_BUTTON_MATRIX:
        return (uint16_t)sizeof(demo_virtual_button_matrix_view_t);
    default:
        return 0;
    }
}
