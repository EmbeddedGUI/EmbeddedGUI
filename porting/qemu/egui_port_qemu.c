#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include "egui.h"

/**
 * QEMU ARM port: display with optional SPI timing simulation + semihosting I/O + SysTick timing.
 *
 * Timing uses a simple software counter incremented by SysTick_Handler.
 * With QEMU -icount shift=0, each instruction takes 1 virtual ns,
 * giving deterministic, reproducible timing results.
 *
 * SPI simulation uses tick-based deadline for async transfers.
 * When QEMU_SPI_SPEED_MHZ > 0, draw_area() busy-waits for the transfer time,
 * and draw_area() sets a tick deadline polled by SysTick_Handler.
 */

/* ============================================================================
 * SysTick timing
 * ============================================================================ */

/* SysTick registers (Cortex-M standard, memory-mapped) */
#define SYST_CSR (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR (*(volatile uint32_t *)0xE000E018)

#define SYST_CSR_ENABLE    (1 << 0)
#define SYST_CSR_TICKINT   (1 << 1)
#define SYST_CSR_CLKSOURCE (1 << 2)

/* Assume QEMU default CPU clock ~25MHz for mps2-an385 */
#define SYSTICK_FREQ_HZ 25000000

/*
 * SysTick period: 1ms normally, 100us when SPI simulation is active
 * (need finer granularity to poll SPI completion for multi-buffer overlap).
 */
#if QEMU_SPI_SPEED_MHZ > 0
#define SYSTICK_PERIOD_US 100
#else
#define SYSTICK_PERIOD_US 1000
#endif
#define SYSTICK_RELOAD_VAL (SYSTICK_FREQ_HZ / (1000000 / SYSTICK_PERIOD_US) - 1)

static volatile uint32_t g_tick_ms = 0;
volatile uint32_t g_raw_tick_count = 0;

#if SYSTICK_PERIOD_US < 1000
#define SYSTICK_TICKS_PER_MS (1000 / SYSTICK_PERIOD_US)
static volatile uint32_t g_tick_sub = 0;
#endif

/* Forward declaration for SPI simulation polling */
static void spi_sim_poll(void);

void SysTick_Handler(void)
{
    g_raw_tick_count++;
#if SYSTICK_PERIOD_US < 1000
    if (++g_tick_sub >= SYSTICK_TICKS_PER_MS)
    {
        g_tick_sub = 0;
        g_tick_ms++;
    }
#else
    g_tick_ms++;
#endif
    spi_sim_poll();
}

void qemu_systick_init(void)
{
    SYST_RVR = SYSTICK_RELOAD_VAL;
    SYST_CVR = 0;
    SYST_CSR = SYST_CSR_ENABLE | SYST_CSR_TICKINT | SYST_CSR_CLKSOURCE;
}

/**
 * High-resolution microsecond timer using SysTick CVR.
 * Combines the ms counter with the current SysTick countdown value
 * to achieve ~1us resolution (at 25MHz, 1 tick = 40ns).
 */
uint32_t qemu_get_tick_us(void)
{
    uint32_t ms1, ms2, cvr;
#if SYSTICK_PERIOD_US < 1000
    uint32_t sub1, sub2;
    do
    {
        ms1 = g_tick_ms;
        sub1 = g_tick_sub;
        cvr = SYST_CVR;
        ms2 = g_tick_ms;
        sub2 = g_tick_sub;
    } while (ms1 != ms2 || sub1 != sub2);
    uint32_t elapsed_ticks = (SYSTICK_RELOAD_VAL + 1) - cvr;
    uint32_t us_fraction = elapsed_ticks / (SYSTICK_FREQ_HZ / 1000000);
    return ms1 * 1000 + sub1 * SYSTICK_PERIOD_US + us_fraction;
#else
    do
    {
        ms1 = g_tick_ms;
        cvr = SYST_CVR;
        ms2 = g_tick_ms;
    } while (ms1 != ms2);
    uint32_t elapsed_ticks = (SYSTICK_RELOAD_VAL + 1) - cvr;
    uint32_t us_fraction = elapsed_ticks / (SYSTICK_FREQ_HZ / 1000000);
    return ms1 * 1000 + us_fraction;
#endif
}

/* ============================================================================
 * ARM semihosting
 * ============================================================================ */

static inline void semihosting_exit(int code)
{
    (void)code;
    register uint32_t r0 __asm__("r0") = 0x18;    /* SYS_EXIT */
    register uint32_t r1 __asm__("r1") = 0x20026; /* ADP_Stopped_ApplicationExit */
    __asm__ volatile("bkpt 0xAB" : : "r"(r0), "r"(r1) : "memory");
}

void qemu_exit(int code)
{
    semihosting_exit(code);
    while (1)
        ;
}

/* ============================================================================
 * SPI transfer simulation
 *
 * Uses raw SysTick tick counting for deadlines. Each tick = SYSTICK_PERIOD_US.
 * For async mode, SysTick_Handler polls and calls egui_pfb_notify_flush_complete().
 * ============================================================================ */

#ifndef QEMU_SPI_SPEED_MHZ
#define QEMU_SPI_SPEED_MHZ 0
#endif

static volatile int spi_sim_busy = 0;
static volatile uint32_t spi_sim_end_tick = 0;

/**
 * Compute number of SysTick ticks needed for SPI transfer of 'bytes'.
 * ticks = ceil(transfer_us / SYSTICK_PERIOD_US)
 * transfer_us = bytes * 8 / SPI_MHz
 */
static uint32_t spi_calc_ticks(uint32_t bytes)
{
#if QEMU_SPI_SPEED_MHZ > 0
    uint32_t us = bytes * 8 / QEMU_SPI_SPEED_MHZ;
    uint32_t ticks = (us + SYSTICK_PERIOD_US - 1) / SYSTICK_PERIOD_US;
    return ticks > 0 ? ticks : 1;
#else
    EGUI_UNUSED(bytes);
    return 0;
#endif
}

/**
 * Start a simulated async SPI transfer.
 * Sets a tick deadline; SysTick_Handler polls for completion.
 */
static void spi_sim_start_async(uint32_t bytes)
{
#if QEMU_SPI_SPEED_MHZ > 0
    uint32_t ticks = spi_calc_ticks(bytes);
    spi_sim_end_tick = g_raw_tick_count + ticks;
    spi_sim_busy = 1;
#else
    EGUI_UNUSED(bytes);
#endif
}

/**
 * Poll for SPI completion — called from SysTick_Handler.
 */
static void spi_sim_poll(void)
{
#if QEMU_SPI_SPEED_MHZ > 0
    if (spi_sim_busy)
    {
        if ((int32_t)(g_raw_tick_count - spi_sim_end_tick) >= 0)
        {
            spi_sim_busy = 0;
            egui_pfb_notify_flush_complete();
        }
    }
#endif
}

/* ============================================================================
 * Display driver
 * ============================================================================ */

static void qemu_display_init(void)
{
}

static void qemu_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(data);

#if QEMU_SPI_SPEED_MHZ > 0
    /* Synchronous SPI simulation: busy-wait for transfer time */
    uint32_t bytes = (uint32_t)w * h * sizeof(egui_color_int_t);
    uint32_t ticks = spi_calc_ticks(bytes);
    uint32_t end_tick = g_raw_tick_count + ticks;
    while ((int32_t)(g_raw_tick_count - end_tick) < 0)
        ;
#else
    EGUI_UNUSED(w);
    EGUI_UNUSED(h);
#endif
}

static void qemu_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(data);

#if QEMU_SPI_SPEED_MHZ > 0
    uint32_t bytes = (uint32_t)w * h * sizeof(egui_color_int_t);
    spi_sim_start_async(bytes);
#else
    EGUI_UNUSED(w);
    EGUI_UNUSED(h);
#endif
}

static void qemu_display_wait_draw_complete(void)
{
#if QEMU_SPI_SPEED_MHZ > 0
    while (spi_sim_busy)
        ;
#endif
}

static void qemu_display_flush(void)
{
}

static const egui_display_driver_ops_t qemu_display_ops = {
        .init = qemu_display_init,
        .draw_area = qemu_display_draw_area,
        .wait_draw_complete = qemu_display_wait_draw_complete,
        .flush = qemu_display_flush,
        .set_brightness = NULL,
        .set_power = NULL,
        .set_rotation = NULL,
        .fill_rect = NULL,
        .blit = NULL,
        .blend = NULL,
        .wait_vsync = NULL,
};

static egui_display_driver_t qemu_display_driver = {
        .ops = &qemu_display_ops,
        .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_0,
        .brightness = 255,
        .power_on = 1,
};

/* ============================================================================
 * Platform driver
 * ============================================================================ */

static void qemu_vlog(const char *format, va_list args)
{
    vprintf(format, args);
}

static void qemu_assert_handler(const char *file, int line)
{
    printf("ASSERT: %s:%d\n", file, line);
    qemu_exit(1);
}

static void qemu_vsprintf(char *str, const char *format, va_list args)
{
    vsprintf(str, format, args);
}

static uint32_t qemu_get_tick_ms(void)
{
    return g_tick_ms;
}

static void qemu_delay(uint32_t ms)
{
    uint32_t target = g_tick_ms + ms;
    while (g_tick_ms < target)
        ;
}

static void qemu_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}

static egui_base_t qemu_interrupt_disable(void)
{
    uint32_t primask;
    __asm__ volatile("mrs %0, primask" : "=r"(primask));
    __asm__ volatile("cpsid i" ::: "memory");
    return primask;
}

static void qemu_interrupt_enable(egui_base_t level)
{
    __asm__ volatile("msr primask, %0" ::"r"(level) : "memory");
}

static const egui_platform_ops_t qemu_platform_ops = {
        .malloc = NULL,
        .free = NULL,
        .vlog = qemu_vlog,
        .assert_handler = qemu_assert_handler,
        .vsprintf = qemu_vsprintf,
        .delay = qemu_delay,
        .get_tick_ms = qemu_get_tick_ms,
        .pfb_clear = qemu_pfb_clear,
        .interrupt_disable = qemu_interrupt_disable,
        .interrupt_enable = qemu_interrupt_enable,
        .load_external_resource = NULL,
        .mutex_create = NULL,
        .mutex_lock = NULL,
        .mutex_unlock = NULL,
        .mutex_destroy = NULL,
        .timer_start = NULL,
        .timer_stop = NULL,
        .memcpy_fast = NULL,
        .watchdog_feed = NULL,
};

static egui_platform_t qemu_platform = {
        .ops = &qemu_platform_ops,
};

/* ============================================================================
 * Port initialization
 * ============================================================================ */

void egui_port_init(void)
{
#if QEMU_SPI_SPEED_MHZ > 0
    /* Timing calibration: verify qemu_get_tick_us() accuracy */
    uint32_t cal_start = qemu_get_tick_us();
    volatile uint32_t dummy = 0;
    for (volatile int i = 0; i < 10000; i++)
    {
        dummy += i;
    }
    uint32_t cal_mid = qemu_get_tick_us();
    qemu_delay(10);
    uint32_t cal_end = qemu_get_tick_us();
    printf("TIMING_CAL: loop=%luus delay10ms=%luus\n", (unsigned long)(cal_mid - cal_start), (unsigned long)(cal_end - cal_mid));
#endif
    egui_display_driver_register(&qemu_display_driver);
    egui_platform_register(&qemu_platform);
}
