#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"
#include "uicode_viewpage.h"
#include "uicode_scroll.h"

void uicode_init_ui(void)
{
    // Add To Root
    egui_core_add_user_root_view(uicode_init_ui_viewpage());
}

static egui_timer_t ui_timer;
void egui_view_test_timer_callback(egui_timer_t *timer)
{
}

void uicode_create_ui(void)
{
    uicode_init_ui();

    ui_timer.callback = egui_view_test_timer_callback;
    egui_timer_start_timer(&ui_timer, 1000, 1000);
}
