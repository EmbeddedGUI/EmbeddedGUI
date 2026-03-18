#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include "egui.h"
#include "egui_lcd.h"

/**
 * QEMU ARM port: display with optional SPI timing simulation + semihosting I/O + SysTick timing.
 *
 * Timing uses a simple software counter incremented by SysTick_Handler.
 * With QEMU -icount shift=0, each instruction takes 1 virtual ns,
 * giving deterministic, reproducible timing results.
 *
 * SPI simulation uses tick-based deadlines for async transfers.
 * The QEMU LCD HAL driver is registered through the common bridge layer.
 * When QEMU_SPI_SPEED_MHZ > 0, SysTick polls the transfer deadline and the ISR
 * path completes PFB flushes via egui_pfb_notify_flush_complete().
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
#if QEMU_SPI_SPEED_MHZ > 0
static uint32_t spi_calc_ticks(uint32_t bytes)
{
    uint32_t us = bytes * 8 / QEMU_SPI_SPEED_MHZ;
    uint32_t ticks = (us + SYSTICK_PERIOD_US - 1) / SYSTICK_PERIOD_US;
    return ticks > 0 ? ticks : 1;
}

/**
 * Start a simulated async SPI transfer.
 * Sets a tick deadline; SysTick_Handler polls for completion.
 */
static void spi_sim_start_async(uint32_t bytes)
{
    uint32_t ticks = spi_calc_ticks(bytes);
    spi_sim_end_tick = g_raw_tick_count + ticks;
    spi_sim_busy = 1;
}
#endif

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
 * Display HAL driver
 * ============================================================================ */

static egui_hal_lcd_driver_t s_qemu_lcd_driver;

static egui_display_driver_ops_t port_display_ops = {0};

static egui_display_driver_t port_display_driver = {
        .ops = &port_display_ops,
        .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_0,
        .brightness = 255,
        .power_on = 1,
};

static int qemu_lcd_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    memcpy(&self->config, config, sizeof(*config));
    return 0;
}

static int qemu_lcd_reset(egui_hal_lcd_driver_t *self)
{
    (void)self;
    return 0; /* No hardware to reset in QEMU */
}

static void qemu_lcd_del(egui_hal_lcd_driver_t *self)
{
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

static void qemu_lcd_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h, const void *data, uint32_t len)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(w);
    EGUI_UNUSED(h);
    EGUI_UNUSED(data);

#if QEMU_SPI_SPEED_MHZ > 0
    spi_sim_start_async(len);
#else
    EGUI_UNUSED(len);
#endif
}

#if QEMU_SPI_SPEED_MHZ > 0
static void qemu_lcd_wait_dma_complete(void)
{
    while (spi_sim_busy)
        ;
}

#endif
static void qemu_lcd_setup(egui_hal_lcd_driver_t *storage)
{
    memset(storage, 0, sizeof(*storage));
    storage->name = "QEMU_LCD";
    storage->reset = qemu_lcd_reset;
    storage->init = qemu_lcd_init;
    storage->del = qemu_lcd_del;
    storage->draw_area = qemu_lcd_draw_area;
    storage->mirror = NULL;
    storage->swap_xy = NULL;
    storage->set_power = NULL;
    storage->set_invert = NULL;
    storage->io = NULL;
    storage->set_rst = NULL;
}

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

#if QEMU_SPI_SPEED_MHZ > 0
static void port_display_wait_draw_complete(void)
{
    qemu_lcd_wait_dma_complete();
}
#endif

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
    egui_hal_lcd_config_t lcd_config = {
            .width = EGUI_CONFIG_SCEEN_WIDTH,
            .height = EGUI_CONFIG_SCEEN_HEIGHT,
            .color_depth = EGUI_CONFIG_COLOR_DEPTH,
            .color_swap = 0,
            .x_offset = 0,
            .y_offset = 0,
            .invert_color = 0,
            .mirror_x = 0,
            .mirror_y = 0,
            .custom_init = NULL,
    };

    qemu_lcd_setup(&s_qemu_lcd_driver);
    egui_hal_lcd_register(&port_display_driver, &s_qemu_lcd_driver, &lcd_config);

#if QEMU_SPI_SPEED_MHZ > 0
    /* Patch in SPI simulation wait callback */
    port_display_ops.wait_draw_complete = port_display_wait_draw_complete;
#endif

    egui_platform_register(&qemu_platform);
}
