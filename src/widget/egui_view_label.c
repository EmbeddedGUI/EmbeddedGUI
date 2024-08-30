#include <stdio.h>
#include <assert.h>

#include "egui_view_label.h"
#include "font/egui_font.h"

void egui_view_label_on_draw(egui_view_t *self)
{
    egui_view_label_t *local = (egui_view_label_t *)self;
    if(local->font == NULL || local->text == NULL)
    {
        return;
    }
    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_canvas_draw_text_in_rect(local->font, local->text, &region, local->align_type, local->color, local->alpha);
}

void egui_view_label_set_font(egui_view_t *self, egui_font_t *font)
{
    egui_view_label_t *local = (egui_view_label_t *)self;
    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_label_set_font_color(egui_view_t *self, egui_color_t color, egui_alpha_t alpha)
{
    egui_view_label_t *local = (egui_view_label_t *)self;
    local->color = color;
    local->alpha = alpha;
    egui_view_invalidate(self);
}

void egui_view_label_set_align_type(egui_view_t *self, uint8_t align_type)
{
    egui_view_label_t *local = (egui_view_label_t *)self;
    local->align_type = align_type;
    egui_view_invalidate(self);
}

void egui_view_label_set_text(egui_view_t *self, const char *text)
{
    egui_view_label_t *local = (egui_view_label_t *)self;
    local->text = text;
    egui_view_invalidate(self);
}

int egui_view_label_get_str_size(egui_view_t *self, const void *string, egui_dim_t *width, egui_dim_t *height)
{
    egui_view_label_t *local = (egui_view_label_t *)self;

    local->font->api->get_str_size(local->font, string, width, height);

    return 0;
}

int egui_view_label_get_str_size_with_padding(egui_view_t *self, const void *string, egui_dim_t *width, egui_dim_t *height)
{
    if (!egui_view_label_get_str_size(self, string, width, height))
    {
        *width += self->padding.left + self->padding.right;
        *height += self->padding.top + self->padding.bottom;
    }

    return 0;
}

EGUI_VIEW_API_DEFINE(egui_view_label_t, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, egui_view_label_on_draw, NULL);

void egui_view_label_init(egui_view_t *self)
{
    egui_view_label_t *local = (egui_view_label_t *)self;
    EGUI_UNUSED(local);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_label_t);

    // init local data.

    egui_view_set_view_name(self, "egui_view_label");
}
