#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_dynamic_label.h"
#include "core/egui_api.h"
#include "font/egui_font.h"

/**
 * @file egui_view_dynamic_label.c
 * @brief Label wrapper with fixed-size internal text storage.
 *
 * This widget is useful when caller text is short-lived or stack-backed. The
 * wrapper copies the new string into its own buffer before delegating drawing
 * to the normal label implementation.
 */

/**
 * @brief Copy caller text into the in-place buffer and redraw on real changes.
 */
void egui_view_dynamic_label_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_dynamic_label_t);
    int len = strlen(text);
    // Clamp to the internal buffer size so the embedded label always points to valid storage.
    if (len > EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE)
    {
        len = EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE;
    }
    // Skip invalidation when the copied text would be byte-for-byte identical.
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

    egui_view_invalidate(self);
}

/**
 * @brief Initialize the dynamic label and bind the label text pointer to its buffer.
 */
void egui_view_dynamic_label_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_dynamic_label_t);
    // Reuse the normal label implementation, then redirect its text pointer to the local buffer.
    egui_view_label_init(self, core);
    egui_view_label_set_text(self, local->text_buffer);

    // Start with an empty owned string so drawing is safe before callers set any text.
    egui_api_memset(local->text_buffer, 0, EGUI_VIEW_DYNAMIC_LABEL_MAX_SIZE);
    egui_view_set_view_name(self, "egui_view_dynamic_label");
}

/**
 * @brief Convenience initializer that chains dynamic-label init and label params.
 */
void egui_view_dynamic_label_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_label_params_t *params)
{
    egui_view_dynamic_label_init(self, core);
    egui_view_label_apply_params(self, params);
}
