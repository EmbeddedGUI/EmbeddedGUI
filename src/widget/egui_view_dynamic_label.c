#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_dynamic_label.h"
#include "font/egui_font.h"

void egui_view_dynamic_label_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_dynamic_label_t);
    int len = strlen(text);
    // int is_same = 1;
    if (len > EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE)
    {
        len = EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE;
    }
    // EGUI_LOG_DBG("egui_view_dynamic_label_set_text new: %s, old: %s\n", text, local->text_buffer);
    for (int i = 0; i < EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE; i++)
    {
        if (text[i] != local->text_buffer[i])
        {
            break;
        }
        if (text[i] == '\0')
        {
            return;
        }
    }
    egui_api_memcpy(local->text_buffer, text, len + 1);
    // EGUI_LOG_DBG("egui_view_dynamic_label_set_text after new: %s, old: %s, index: %d\n", text, local->text_buffer, len);

    egui_view_invalidate(self);
}

void egui_view_dynamic_label_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_dynamic_label_t);
    // call super init.
    egui_view_label_init(self);
    egui_view_label_set_text(self, local->text_buffer);

    // init local data.
    egui_api_memset(local->text_buffer, 0, EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE);
    egui_view_set_view_name(self, "egui_view_dynamic_label");
}

void egui_view_dynamic_label_init_with_params(egui_view_t *self, const egui_view_label_params_t *params)
{
    egui_view_dynamic_label_init(self);
    egui_view_label_apply_params(self, params);
}
