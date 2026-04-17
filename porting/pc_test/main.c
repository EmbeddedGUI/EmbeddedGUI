#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "test/egui_test.h"

EGUI_CONFIG_PFB_BUFFER_DECLARE(egui_pfb);

static const char *resolve_suite_filter(int argc, const char *argv[])
{
    const char *suite_filter = NULL;
    int i;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--suite") == 0 && i + 1 < argc)
        {
            suite_filter = argv[++i];
            continue;
        }
        if (strncmp(argv[i], "--suite=", 8) == 0)
        {
            suite_filter = argv[i] + 8;
        }
    }

    if ((suite_filter == NULL || suite_filter[0] == '\0'))
    {
        suite_filter = getenv("EGUI_TEST_SUITE_FILTER");
    }

    if (suite_filter != NULL && suite_filter[0] == '\0')
    {
        suite_filter = NULL;
    }

    return suite_filter;
}

int main(int argc, const char *argv[])
{
    const char *suite_filter = resolve_suite_filter(argc, argv);

    printf("EmbeddedGUI Test Runner\n");

    egui_test_init();
    if (suite_filter != NULL)
    {
        printf("Suite filter: %s\n", suite_filter);
        egui_test_set_suite_filter(suite_filter);
    }

    extern void egui_port_init(void);
    egui_port_init();

    egui_init(egui_pfb);

    // uicode_create_ui() registers and runs all test suites
    uicode_create_ui();
    egui_screen_on();

    egui_test_print_summary();
    return egui_test_get_result();
}
