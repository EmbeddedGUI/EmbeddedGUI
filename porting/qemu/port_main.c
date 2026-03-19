#include <stdio.h>
#include "egui.h"
#include "uicode.h"

extern void qemu_systick_init(void);
extern void qemu_exit(int code);
extern void initialise_monitor_handles(void);

static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_BUFFER_COUNT][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

/* Flag set by benchmark code when all tests are done */
volatile int g_qemu_perf_complete = 0;

void qemu_notify_perf_complete(void)
{
    g_qemu_perf_complete = 1;
}

int main(void)
{
    /* Initialize semihosting for printf output */
    initialise_monitor_handles();

    /* Initialize SysTick for millisecond timing */
    qemu_systick_init();

    /* Initialize EGUI */
    extern void egui_port_init(void);
    egui_port_init();

    egui_init(egui_pfb);

    printf("QEMU EGUI Performance Benchmark\n");

    /* Create UI and run benchmark */
    uicode_create_ui();

    /* Turn on screen - this resumes the core and enables rendering */
    egui_screen_on();

    /* Run polling loop until benchmark completes.
     * HelloPerformance uses a timer callback that runs
     * all test modes then sets g_qemu_perf_complete. */
    while (!g_qemu_perf_complete)
    {
        egui_polling_work();
    }

    printf("PERF_EXIT\n");
    qemu_exit(0);

    return 0;
}
