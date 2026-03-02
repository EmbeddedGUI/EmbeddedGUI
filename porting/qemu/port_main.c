#include <stdio.h>
#include "egui.h"
#include "uicode.h"

extern void qemu_systick_init(void);
extern void qemu_exit(int code);
extern void initialise_monitor_handles(void);

static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 2
static egui_color_int_t egui_pfb2[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];
#endif
#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 3
static egui_color_int_t egui_pfb3[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];
#endif
#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 4
static egui_color_int_t egui_pfb4[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];
#endif

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

    egui_init_config_t init_config = {
            .pfb = egui_pfb,
            .pfb_backup = NULL,
    };
    egui_init(&init_config);

    /* Add extra PFB buffers for multi-buffering */
#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 2
    egui_pfb_add_buffer(egui_pfb2);
#endif
#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 3
    egui_pfb_add_buffer(egui_pfb3);
#endif
#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 4
    egui_pfb_add_buffer(egui_pfb4);
#endif

    printf("QEMU EGUI Performance Benchmark\n");

    /* Create UI and run benchmark */
    uicode_create_ui();

    /* Turn on screen - this resumes the core and enables rendering */
    egui_screen_on();

    /* Run polling loop until benchmark completes.
     * HelloPerformace uses a timer callback that runs
     * all test modes then sets g_qemu_perf_complete. */
    while (!g_qemu_perf_complete)
    {
        egui_polling_work();
    }

    printf("PERF_EXIT\n");
    qemu_exit(0);

    return 0;
}
