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
