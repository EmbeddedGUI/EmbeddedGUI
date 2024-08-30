#include <stdio.h>
#include <assert.h>

#include "egui_view_label.h"
#include "font/egui_font.h"

void egui_view_button_init(egui_view_t *self)
{
    egui_view_label_t *local = (egui_view_label_t *)self;
    EGUI_UNUSED(local);
    // call super init.
    egui_view_label_init(self);

    // init local data.

    egui_view_set_view_name(self, "egui_view_button");
}
