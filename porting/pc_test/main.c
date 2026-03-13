#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "test/egui_test.h"

static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_BUFFER_COUNT][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

int main(int argc, const char *argv[])
{
    printf("EmbeddedGUI Test Runner\n");

    egui_test_init();

    extern void egui_port_init(void);
    egui_port_init();

    egui_init(egui_pfb);

    // uicode_create_ui() registers and runs all test suites
    uicode_create_ui();
    egui_screen_on();

    egui_test_print_summary();
    return egui_test_get_result();
}
