#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "egui.h"
#include "uicode_disp0.h"
#include "test/egui_test.h"

EGUI_CONFIG_PFB_BUFFER_DECLARE(egui_pfb);
static egui_core_t core;

int main(int argc, const char *argv[])
{
    const char *test_filter = NULL;

    if (argc > 1)
    {
        test_filter = argv[1];
    }
    if ((test_filter == NULL || test_filter[0] == '\0'))
    {
        test_filter = getenv("EGUI_TEST_FILTER");
    }

    printf("EmbeddedGUI Test Runner\n");

    egui_test_init();

    extern void egui_port_init(egui_core_t * core);
    extern egui_display_driver_t *egui_port_get_display_driver(void);

    egui_init(&core, egui_pfb);
    egui_port_init(&core);
    egui_display_driver_register(&core, egui_port_get_display_driver());
    uicode_set_test_filter(test_filter);

    uicode_disp0_init(&core);
    egui_screen_on(&core);

    egui_test_print_summary();
    return egui_test_get_result();
}
