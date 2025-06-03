#include <string.h>

#include "egui_utils.h"


void egui_utils_view_image_std_normal_init(egui_view_t *self, egui_image_t *image, egui_dim_t x, egui_dim_t y)
{
    egui_dim_t width, height;

    egui_image_std_get_width_height(image, &width, &height);
    egui_view_image_init(self);
    egui_view_set_position(self, x, y);
    egui_view_set_size(self, width, height);
    egui_view_image_set_image(self, image);
}


void egui_utils_view_label_std_normal_init(egui_view_t *self, const egui_font_t *font, egui_color_t color, const char *str, egui_dim_t x, egui_dim_t y)
{
    egui_dim_t width, height;

    egui_font_std_get_str_size(font, str, &width, &height);

    egui_view_label_init(self);
    egui_view_set_position(self, x, y);
    egui_view_set_size(self, width, height);
    egui_view_label_set_text(self, str);
    egui_view_label_set_align_type(self, EGUI_ALIGN_CENTER);
    egui_view_label_set_font_with_std_height(self, font);
    egui_view_label_set_font_color(self, color, EGUI_ALPHA_100);
}

void egui_utils_view_dynamic_label_std_normal_init(egui_view_t *self, const egui_font_t *font, egui_color_t color, const char *str, egui_dim_t x, egui_dim_t y)
{
    egui_dim_t width, height;

    egui_font_std_get_str_size(font, str, &width, &height);

    egui_view_dynamic_label_init(self);
    egui_view_set_position(self, x, y);
    egui_view_set_size(self, width, height);
    egui_view_dynamic_label_set_text(self, str);
    egui_view_label_set_align_type(self, EGUI_ALIGN_CENTER);
    egui_view_label_set_font_with_std_height(self, font);
    egui_view_label_set_font_color(self, color, EGUI_ALPHA_100);
}

