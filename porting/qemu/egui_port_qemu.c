#include <string.h>
#include <stdarg.h>
#include <stdint.h>

#include "egui.h"

#ifndef EGUI_CONFIG_QEMU_PLATFORM_MALLOC_ENABLE
#define EGUI_CONFIG_QEMU_PLATFORM_MALLOC_ENABLE 1
#endif

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

static int semihosting_call(uint32_t operation, void *arguments)
{
    register uint32_t r0 __asm__("r0") = operation;
    register void *r1 __asm__("r1") = arguments;

    __asm__ volatile("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
    return (int)r0;
}

static void semihosting_write0(const char *str)
{
    register uint32_t r0 __asm__("r0") = 0x04U; /* SYS_WRITE0 */
    register const char *r1 __asm__("r1") = str;

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
 * Display driver
 * ============================================================================ */

static void port_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(w);
    EGUI_UNUSED(h);
    EGUI_UNUSED(data);

#if QEMU_SPI_SPEED_MHZ > 0
    spi_sim_start_async((uint32_t)w * h * sizeof(egui_color_int_t));
#endif
}

#if QEMU_SPI_SPEED_MHZ > 0
static void port_display_wait_draw_complete(void)
{
    while (spi_sim_busy)
        ;
}
#endif

static const egui_display_driver_ops_t port_display_ops = {
        .draw_area = port_display_draw_area,
#if QEMU_SPI_SPEED_MHZ > 0
        .wait_draw_complete = port_display_wait_draw_complete,
#endif
};

static egui_display_driver_t port_display_driver = {
        .ops = &port_display_ops,
        .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_0,
        .brightness = 255,
        .power_on = 1,
};

/* ============================================================================
 * Platform driver
 * ============================================================================ */

static void qemu_append_char(char **cursor, int *remaining, char c)
{
    if (*remaining > 1)
    {
        **cursor = c;
        (*cursor)++;
        (*remaining)--;
    }
}

static void qemu_append_string(char **cursor, int *remaining, const char *str)
{
    const char *text = str == NULL ? "(null)" : str;

    while (*text != '\0')
    {
        qemu_append_char(cursor, remaining, *text++);
    }
}

static void qemu_append_uint(char **cursor, int *remaining, uint64_t value, uint32_t base, int min_width, char pad_char, bool uppercase)
{
    char temp[32];
    int digit_count = 0;
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    if (base < 2U || base > 16U)
    {
        return;
    }

    do
    {
        temp[digit_count++] = digits[value % base];
        value /= base;
    } while (value > 0U && digit_count < (int)sizeof(temp));

    while (digit_count < min_width)
    {
        qemu_append_char(cursor, remaining, pad_char);
        min_width--;
    }

    while (digit_count > 0)
    {
        qemu_append_char(cursor, remaining, temp[--digit_count]);
    }
}

static void qemu_append_int(char **cursor, int *remaining, int64_t value, int min_width, char pad_char)
{
    uint64_t magnitude;

    if (value < 0)
    {
        qemu_append_char(cursor, remaining, '-');
        magnitude = (uint64_t)(-(value + 1)) + 1U;
    }
    else
    {
        magnitude = (uint64_t)value;
    }

    qemu_append_uint(cursor, remaining, magnitude, 10U, min_width, pad_char, false);
}

static void qemu_format_to_buffer(char *buffer, int buffer_size, const char *format, va_list args)
{
    char *cursor = buffer;
    int remaining = buffer_size;

    if (buffer_size <= 0)
    {
        return;
    }

    while (*format != '\0')
    {
        int min_width = 0;
        char pad_char = ' ';
        int long_count = 0;
        char specifier;

        if (*format != '%')
        {
            qemu_append_char(&cursor, &remaining, *format++);
            continue;
        }

        format++;
        if (*format == '%')
        {
            qemu_append_char(&cursor, &remaining, *format++);
            continue;
        }

        if (*format == '0')
        {
            pad_char = '0';
            format++;
        }

        while (*format >= '0' && *format <= '9')
        {
            min_width = min_width * 10 + (*format - '0');
            format++;
        }

        while (*format == 'l')
        {
            long_count++;
            format++;
        }

        specifier = *format;
        if (specifier == '\0')
        {
            break;
        }
        format++;

        switch (specifier)
        {
        case 'c':
            qemu_append_char(&cursor, &remaining, (char)va_arg(args, int));
            break;

        case 's':
            qemu_append_string(&cursor, &remaining, va_arg(args, const char *));
            break;

        case 'd':
        case 'i':
            if (long_count >= 2)
            {
                qemu_append_int(&cursor, &remaining, va_arg(args, long long), min_width, pad_char);
            }
            else if (long_count == 1)
            {
                qemu_append_int(&cursor, &remaining, va_arg(args, long), min_width, pad_char);
            }
            else
            {
                qemu_append_int(&cursor, &remaining, va_arg(args, int), min_width, pad_char);
            }
            break;

        case 'u':
            if (long_count >= 2)
            {
                qemu_append_uint(&cursor, &remaining, va_arg(args, unsigned long long), 10U, min_width, pad_char, false);
            }
            else if (long_count == 1)
            {
                qemu_append_uint(&cursor, &remaining, va_arg(args, unsigned long), 10U, min_width, pad_char, false);
            }
            else
            {
                qemu_append_uint(&cursor, &remaining, va_arg(args, unsigned int), 10U, min_width, pad_char, false);
            }
            break;

        case 'x':
        case 'X':
            if (long_count >= 2)
            {
                qemu_append_uint(&cursor, &remaining, va_arg(args, unsigned long long), 16U, min_width, pad_char, specifier == 'X');
            }
            else if (long_count == 1)
            {
                qemu_append_uint(&cursor, &remaining, va_arg(args, unsigned long), 16U, min_width, pad_char, specifier == 'X');
            }
            else
            {
                qemu_append_uint(&cursor, &remaining, va_arg(args, unsigned int), 16U, min_width, pad_char, specifier == 'X');
            }
            break;

        case 'p':
            qemu_append_string(&cursor, &remaining, "0x");
            qemu_append_uint(&cursor, &remaining, (uintptr_t)va_arg(args, void *), 16U, min_width > 0 ? min_width : 1, '0', false);
            break;

        default:
            qemu_append_char(&cursor, &remaining, '%');
            qemu_append_char(&cursor, &remaining, specifier);
            break;
        }
    }

    *cursor = '\0';
}

void qemu_log_write(const char *str)
{
    if (str != NULL)
    {
        semihosting_write0(str);
    }
}

void qemu_log_printf(const char *format, ...)
{
    char buffer[384];
    va_list args;

    va_start(args, format);
    qemu_format_to_buffer(buffer, (int)sizeof(buffer), format, args);
    va_end(args);
    qemu_log_write(buffer);
}

/* ============================================================================
 * External resource loading via semihosting file I/O
 * ============================================================================ */

#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER

enum
{
    QEMU_SEMIHOSTING_SYS_OPEN = 0x01U,
    QEMU_SEMIHOSTING_SYS_CLOSE = 0x02U,
    QEMU_SEMIHOSTING_SYS_READ = 0x06U,
    QEMU_SEMIHOSTING_SYS_SEEK = 0x0AU,
    QEMU_SEMIHOSTING_MODE_READ_BINARY = 0x01U,
};

typedef struct qemu_semihosting_open_args
{
    const char *path;
    uint32_t mode;
    uint32_t path_length;
} qemu_semihosting_open_args_t;

typedef struct qemu_semihosting_read_args
{
    int handle;
    void *buffer;
    uint32_t size;
} qemu_semihosting_read_args_t;

typedef struct qemu_semihosting_seek_args
{
    int handle;
    uint32_t offset;
} qemu_semihosting_seek_args_t;

static int s_qemu_resource_handle = -1;
static uint32_t s_qemu_resource_file_offset = 0;
static uint8_t s_qemu_resource_file_offset_valid = 0;

static int qemu_semihosting_open(const char *path, uint32_t mode)
{
    qemu_semihosting_open_args_t args = {
            .path = path,
            .mode = mode,
            .path_length = (uint32_t)strlen(path),
    };

    return semihosting_call(QEMU_SEMIHOSTING_SYS_OPEN, &args);
}

static int qemu_semihosting_close(int handle)
{
    return semihosting_call(QEMU_SEMIHOSTING_SYS_CLOSE, &handle);
}

static int qemu_semihosting_read(int handle, void *buffer, uint32_t size)
{
    qemu_semihosting_read_args_t args = {
            .handle = handle,
            .buffer = buffer,
            .size = size,
    };

    return semihosting_call(QEMU_SEMIHOSTING_SYS_READ, &args);
}

static int qemu_semihosting_seek(int handle, uint32_t offset)
{
    qemu_semihosting_seek_args_t args = {
            .handle = handle,
            .offset = offset,
    };

    return semihosting_call(QEMU_SEMIHOSTING_SYS_SEEK, &args);
}

static void qemu_close_external_resource_file(void)
{
    if (s_qemu_resource_handle >= 0)
    {
        qemu_semihosting_close(s_qemu_resource_handle);
        s_qemu_resource_handle = -1;
    }
    s_qemu_resource_file_offset = 0;
    s_qemu_resource_file_offset_valid = 0;
}

static int qemu_get_external_resource_handle(void)
{
    if (s_qemu_resource_handle >= 0)
    {
        return s_qemu_resource_handle;
    }

    /* Try CWD first, then output/ subdirectory */
    s_qemu_resource_handle = qemu_semihosting_open("app_egui_resource_merge.bin", QEMU_SEMIHOSTING_MODE_READ_BINARY);
    if (s_qemu_resource_handle < 0)
    {
        s_qemu_resource_handle = qemu_semihosting_open("output/app_egui_resource_merge.bin", QEMU_SEMIHOSTING_MODE_READ_BINARY);
    }
    if (s_qemu_resource_handle < 0)
    {
        qemu_log_write("QEMU: Error opening app_egui_resource_merge.bin\n");
        return -1;
    }

    return s_qemu_resource_handle;
}

static void qemu_load_external_resource(void *dest, uint32_t res_id, uint32_t start_offset, uint32_t size)
{
    extern const uint32_t egui_ext_res_id_map[];
    uint32_t res_offset = egui_ext_res_id_map[res_id];
    uint32_t res_real_offset = res_offset + start_offset;
    int handle = qemu_get_external_resource_handle();
    if (handle < 0)
    {
        return;
    }

    if (!s_qemu_resource_file_offset_valid || s_qemu_resource_file_offset != res_real_offset)
    {
        if (qemu_semihosting_seek(handle, res_real_offset) != 0)
        {
            qemu_log_write("QEMU: Error seeking in resource file\n");
            qemu_close_external_resource_file();
            return;
        }
    }

    if (qemu_semihosting_read(handle, dest, size) != 0)
    {
        qemu_log_printf("QEMU: Error reading resource, size: %d\n", (int)size);
        qemu_close_external_resource_file();
        return;
    }

    s_qemu_resource_file_offset = res_real_offset + size;
    s_qemu_resource_file_offset_valid = 1;
}

#endif /* EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER */

static void qemu_vlog(const char *format, va_list args)
{
    char buffer[384];

    qemu_format_to_buffer(buffer, (int)sizeof(buffer), format, args);
    qemu_log_write(buffer);
}

static void qemu_assert_handler(const char *file, int line)
{
    qemu_log_printf("ASSERT: %s:%d\n", file, line);
    qemu_exit(1);
}

static void qemu_vsprintf(char *str, const char *format, va_list args)
{
    qemu_format_to_buffer(str, 0x7fffffff, format, args);
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

#ifndef QEMU_HEAP_MEASURE
#define QEMU_HEAP_MEASURE 0
#endif

#if QEMU_HEAP_MEASURE
static uint32_t s_qemu_heap_current_bytes = 0U;
static uint32_t s_qemu_heap_peak_bytes = 0U;
static uint32_t s_qemu_heap_alloc_count = 0U;
static uint32_t s_qemu_heap_free_count = 0U;
static uint32_t s_qemu_heap_measure_base_current = 0U;
static uint32_t s_qemu_heap_measure_peak_bytes = 0U;
static uint32_t s_qemu_heap_measure_base_alloc_count = 0U;
static uint32_t s_qemu_heap_measure_base_free_count = 0U;

static void qemu_heap_refresh_measure_peak(void)
{
    uint32_t delta = 0U;

    if (s_qemu_heap_current_bytes > s_qemu_heap_peak_bytes)
    {
        s_qemu_heap_peak_bytes = s_qemu_heap_current_bytes;
    }

    if (s_qemu_heap_current_bytes > s_qemu_heap_measure_base_current)
    {
        delta = s_qemu_heap_current_bytes - s_qemu_heap_measure_base_current;
    }

    if (delta > s_qemu_heap_measure_peak_bytes)
    {
        s_qemu_heap_measure_peak_bytes = delta;
    }
}

#define QEMU_HEAP_TRACK_ALLOC(_size)                                                                                                                        \
    do                                                                                                                                                      \
    {                                                                                                                                                       \
        uint32_t _payload_size = (_size);                                                                                                                   \
        s_qemu_heap_current_bytes += _payload_size;                                                                                                         \
        s_qemu_heap_alloc_count++;                                                                                                                          \
        qemu_heap_refresh_measure_peak();                                                                                                                   \
    } while (0)

#define QEMU_HEAP_TRACK_FREE(_size)                                                                                                                         \
    do                                                                                                                                                      \
    {                                                                                                                                                       \
        uint32_t _payload_size = (_size);                                                                                                                   \
        if (s_qemu_heap_current_bytes >= _payload_size)                                                                                                     \
        {                                                                                                                                                   \
            s_qemu_heap_current_bytes -= _payload_size;                                                                                                     \
        }                                                                                                                                                   \
        else                                                                                                                                                \
        {                                                                                                                                                   \
            s_qemu_heap_current_bytes = 0U;                                                                                                                 \
        }                                                                                                                                                   \
        s_qemu_heap_free_count++;                                                                                                                           \
        qemu_heap_refresh_measure_peak();                                                                                                                   \
    } while (0)
#else
#define QEMU_HEAP_TRACK_ALLOC(_size) EGUI_UNUSED(_size)
#define QEMU_HEAP_TRACK_FREE(_size)  EGUI_UNUSED(_size)
#endif

#if EGUI_CONFIG_QEMU_PLATFORM_MALLOC_ENABLE
extern uint8_t _ebss[];
extern uint8_t _estack[];
extern uint32_t _Min_Stack_Size;

#define QEMU_HEAP_ALIGN_BYTES 8U

typedef struct qemu_heap_alloc_header
{
    uint32_t block_size;
    uint32_t payload_size;
} qemu_heap_alloc_header_t;

typedef struct qemu_heap_free_block
{
    uint32_t block_size;
    struct qemu_heap_free_block *next;
} qemu_heap_free_block_t;

static qemu_heap_free_block_t *s_qemu_heap_free_list = NULL;
static uint8_t s_qemu_heap_initialized = 0U;

static uint32_t qemu_heap_align_up(uint32_t size)
{
    return (size + (QEMU_HEAP_ALIGN_BYTES - 1U)) & ~(QEMU_HEAP_ALIGN_BYTES - 1U);
}

static void qemu_heap_init(void)
{
    uintptr_t heap_start;
    uintptr_t heap_end;
    uint32_t heap_bytes;

    if (s_qemu_heap_initialized)
    {
        return;
    }

    s_qemu_heap_initialized = 1U;
    heap_start = qemu_heap_align_up((uint32_t)(uintptr_t)_ebss);
    heap_end = ((uintptr_t)_estack - (uintptr_t)&_Min_Stack_Size) & ~(uintptr_t)(QEMU_HEAP_ALIGN_BYTES - 1U);
    if (heap_end <= heap_start)
    {
        return;
    }

    heap_bytes = (uint32_t)(heap_end - heap_start);
    if (heap_bytes < sizeof(qemu_heap_free_block_t))
    {
        return;
    }

    s_qemu_heap_free_list = (qemu_heap_free_block_t *)heap_start;
    s_qemu_heap_free_list->block_size = heap_bytes;
    s_qemu_heap_free_list->next = NULL;
}

static void qemu_heap_insert_free_block(qemu_heap_free_block_t *block)
{
    qemu_heap_free_block_t *prev = NULL;
    qemu_heap_free_block_t *current = s_qemu_heap_free_list;

    while (current != NULL && current < block)
    {
        prev = current;
        current = current->next;
    }

    block->next = current;
    if (prev != NULL)
    {
        prev->next = block;
    }
    else
    {
        s_qemu_heap_free_list = block;
    }

    if (current != NULL && ((uintptr_t)block + block->block_size) == (uintptr_t)current)
    {
        block->block_size += current->block_size;
        block->next = current->next;
    }

    if (prev != NULL && ((uintptr_t)prev + prev->block_size) == (uintptr_t)block)
    {
        prev->block_size += block->block_size;
        prev->next = block->next;
    }
}
static void *qemu_malloc(int size)
{
    qemu_heap_free_block_t *prev = NULL;
    qemu_heap_free_block_t *block;
    qemu_heap_alloc_header_t *header;
    uint32_t aligned_payload;
    uint32_t needed_total;
    uint32_t remaining;

    if (size <= 0)
    {
        return NULL;
    }

    qemu_heap_init();
    aligned_payload = qemu_heap_align_up((uint32_t)size);
    needed_total = aligned_payload + (uint32_t)sizeof(qemu_heap_alloc_header_t);
    if (needed_total < aligned_payload)
    {
        return NULL;
    }

    block = s_qemu_heap_free_list;
    while (block != NULL)
    {
        if (block->block_size >= needed_total)
        {
            break;
        }
        prev = block;
        block = block->next;
    }

    if (block == NULL)
    {
        return NULL;
    }

    remaining = block->block_size - needed_total;
    if (remaining >= sizeof(qemu_heap_free_block_t))
    {
        qemu_heap_free_block_t *next_block = (qemu_heap_free_block_t *)((uint8_t *)block + needed_total);
        next_block->block_size = remaining;
        next_block->next = block->next;
        if (prev != NULL)
        {
            prev->next = next_block;
        }
        else
        {
            s_qemu_heap_free_list = next_block;
        }
        header = (qemu_heap_alloc_header_t *)block;
        header->block_size = needed_total;
    }
    else
    {
        if (prev != NULL)
        {
            prev->next = block->next;
        }
        else
        {
            s_qemu_heap_free_list = block->next;
        }
        header = (qemu_heap_alloc_header_t *)block;
        header->block_size = block->block_size;
    }

    header->payload_size = (uint32_t)size;
    QEMU_HEAP_TRACK_ALLOC(header->payload_size);

    return (void *)(header + 1);
}

static void qemu_free(void *ptr)
{
    qemu_heap_alloc_header_t *header;
    qemu_heap_free_block_t *block;

    if (ptr == NULL)
    {
        return;
    }

    header = ((qemu_heap_alloc_header_t *)ptr) - 1;
    QEMU_HEAP_TRACK_FREE(header->payload_size);

    block = (qemu_heap_free_block_t *)header;
    block->block_size = header->block_size;
    qemu_heap_insert_free_block(block);
}
#endif

#if QEMU_HEAP_MEASURE
void qemu_heap_reset_stats(void)
{
    s_qemu_heap_measure_base_current = s_qemu_heap_current_bytes;
    s_qemu_heap_measure_peak_bytes = 0U;
    s_qemu_heap_measure_base_alloc_count = s_qemu_heap_alloc_count;
    s_qemu_heap_measure_base_free_count = s_qemu_heap_free_count;
}

uint32_t qemu_heap_get_current_bytes(void)
{
    if (s_qemu_heap_current_bytes > s_qemu_heap_measure_base_current)
    {
        return s_qemu_heap_current_bytes - s_qemu_heap_measure_base_current;
    }
    return 0U;
}

uint32_t qemu_heap_get_peak_bytes(void)
{
    return s_qemu_heap_measure_peak_bytes;
}

uint32_t qemu_heap_get_alloc_count(void)
{
    return s_qemu_heap_alloc_count - s_qemu_heap_measure_base_alloc_count;
}

uint32_t qemu_heap_get_free_count(void)
{
    return s_qemu_heap_free_count - s_qemu_heap_measure_base_free_count;
}
#else
void qemu_heap_reset_stats(void)
{
}

uint32_t qemu_heap_get_current_bytes(void)
{
    return 0U;
}

uint32_t qemu_heap_get_peak_bytes(void)
{
    return 0U;
}

uint32_t qemu_heap_get_alloc_count(void)
{
    return 0U;
}

uint32_t qemu_heap_get_free_count(void)
{
    return 0U;
}
#endif

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
#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
#if EGUI_CONFIG_QEMU_PLATFORM_MALLOC_ENABLE
        .malloc = qemu_malloc,
        .free = qemu_free,
#else
        .malloc = NULL,
        .free = NULL,
#endif
#endif
#if EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF
        .vlog = qemu_vlog,
        .vsprintf = qemu_vsprintf,
#endif
        .assert_handler = qemu_assert_handler,
        .delay = qemu_delay,
        .get_tick_ms = qemu_get_tick_ms,
        .interrupt_disable = qemu_interrupt_disable,
        .interrupt_enable = qemu_interrupt_enable,
#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
        .load_external_resource = qemu_load_external_resource,
#else
        .load_external_resource = NULL,
#endif
        .mutex_create = NULL,
        .mutex_lock = NULL,
        .mutex_unlock = NULL,
        .mutex_destroy = NULL,
        .timer_start = NULL,
        .timer_stop = NULL,
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
    qemu_log_printf("TIMING_CAL: loop=%luus delay10ms=%luus\n", (unsigned long)(cal_mid - cal_start), (unsigned long)(cal_end - cal_mid));
#endif
    egui_display_driver_register(&port_display_driver);
    egui_platform_register(&qemu_platform);
}
