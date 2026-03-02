// main_page.c — User implementation for main_page
// This file is YOUR code. The designer will NEVER overwrite it.
// Layout/widget init is in main_page_layout.c (auto-generated).

#include "egui.h"
#include <stdlib.h>

#include "uicode.h"
#include "main_page.h"

// ── Your includes ──
#include "my_sensor_lib.h"

// ── Your static variables ──
static int s_update_count = 0;

// ── Your callback functions ──
static void on_btn_click(egui_view_t *self)
{
    s_update_count++;
}

static void egui_main_page_on_open(egui_page_base_t *self)
{
    egui_main_page_t *local = (egui_main_page_t *)self;
    EGUI_UNUSED(local);
    // Call super on_open
    egui_page_base_on_open(self);

    // Auto-generated layout initialization
    egui_main_page_layout_init(self);

    // TODO: Add your post-init logic here
    // e.g. register click listeners, start timers, set dynamic text
}

static void egui_main_page_on_close(egui_page_base_t *self)
{
    egui_main_page_t *local = (egui_main_page_t *)self;
    EGUI_UNUSED(local);
    // Call super on_close
    egui_page_base_on_close(self);

    // TODO: Add your cleanup logic here
}

static void egui_main_page_on_key_pressed(egui_page_base_t *self, uint16_t keycode)
{
    egui_main_page_t *local = (egui_main_page_t *)self;
    EGUI_UNUSED(local);

    // TODO: Handle key events here
}

static const egui_page_base_api_t EGUI_VIEW_API_TABLE_NAME(egui_main_page_t) = {
        .on_open = egui_main_page_on_open,
        .on_close = egui_main_page_on_close,
        .on_key_pressed = egui_main_page_on_key_pressed,
};

void egui_main_page_init(egui_page_base_t *self)
{
    egui_main_page_t *local = (egui_main_page_t *)self;
    EGUI_UNUSED(local);
    // Call super init
    egui_page_base_init(self);
    // Set vtable
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_main_page_t);
    egui_page_base_set_name(self, "main_page");

    // TODO: Add your custom init logic here
}
