#include "egui_pfb_manager.h"
#include "egui_api.h"
#include "egui_core.h"
#include "egui_display_driver.h"
#include "egui_platform.h"

/**
 * @file egui_pfb_manager.c
 * @brief Ring-buffer manager that overlaps tile rendering with display flushes when possible.
 */

/** Initialize the manager with one mandatory render buffer and a fully idle queue state. */
void egui_pfb_manager_init(egui_pfb_manager_t *mgr, egui_color_int_t *pfb, int16_t width, int16_t height, int color_bytes)
{
    int i;
    for (i = 0; i < EGUI_PFB_BUFFER_MAX_COUNT; i++)
    {
        mgr->buffers[i] = NULL;
    }
    mgr->buffers[0] = pfb;
    mgr->buffer_count = 1;
    mgr->width = width;
    mgr->height = height;
    mgr->buffer_size_bytes = width * height * color_bytes;
    mgr->render_idx = 0;
    mgr->flush_idx = 0;
    mgr->pending_count = 0;
    mgr->dma_busy = 0;
    mgr->bus_locked = 0;
}

/** Append one extra PFB storage block to the render/flush ring before rendering starts. */
void egui_pfb_manager_add_buffer(egui_pfb_manager_t *mgr, egui_color_int_t *buf)
{
    if (mgr->buffer_count >= EGUI_PFB_BUFFER_MAX_COUNT || buf == NULL)
    {
        return;
    }
    mgr->buffers[mgr->buffer_count] = buf;
    mgr->buffer_count++;
}

/** Backward-compatible alias for enabling a second buffer. */
void egui_pfb_manager_set_backup_buffer(egui_pfb_manager_t *mgr, egui_color_int_t *backup)
{
    egui_pfb_manager_add_buffer(mgr, backup);
}

/**
 * Start DMA for the buffer at flush_idx.
 * Called from submit() or from notify_flush_complete() to chain next transfer.
 */
#if EGUI_CONFIG_COLOR_DEPTH == 16
#if EGUI_TARGET_TC32
#define EGUI_PFB_MANAGER_RGB565_PAIR_FAST_PATH 0
#elif defined(__GNUC__) || defined(__clang__)
typedef uint32_t egui_pfb_manager_rgb565_pair_t __EGUI_MAY_ALIAS_ATTR__;
#define EGUI_PFB_MANAGER_RGB565_PAIR_FAST_PATH 1
#else
#define EGUI_PFB_MANAGER_RGB565_PAIR_FAST_PATH 0
#endif

__EGUI_STATIC_INLINE__ uint16_t egui_pfb_manager_swap_rgb565_pixel(uint16_t v)
{
    return (uint16_t)((v >> 8) | (v << 8));
}

#if EGUI_PFB_MANAGER_RGB565_PAIR_FAST_PATH
__EGUI_STATIC_INLINE__ uint32_t egui_pfb_manager_swap_rgb565_pair(uint32_t v)
{
#if defined(__GNUC__) && (defined(__arm__) || defined(__thumb__))
    __asm__("rev16 %0, %1" : "=r"(v) : "r"(v));
    return v;
#else
    return ((v >> 8) & 0x00FF00FFu) | ((v << 8) & 0xFF00FF00u);
#endif
}
#endif

static void egui_pfb_manager_swap_rgb565_buffer(egui_color_int_t *buf, uint32_t pixel_count)
{
    uint16_t *buf16 = (uint16_t *)buf;

    if (pixel_count == 0)
    {
        return;
    }

#if EGUI_PFB_MANAGER_RGB565_PAIR_FAST_PATH
    // Peel one pixel when needed so the bulk path never performs an unaligned 32-bit access.
    if (((egui_uintptr_t)buf16 & 0x03U) != 0U)
    {
        *buf16 = egui_pfb_manager_swap_rgb565_pixel(*buf16);
        buf16++;
        pixel_count--;
    }

    if (pixel_count >= 2)
    {
        egui_pfb_manager_rgb565_pair_t *buf32 = (egui_pfb_manager_rgb565_pair_t *)buf16;
        uint32_t pair_count = pixel_count >> 1;

        for (uint32_t i = 0; i < pair_count; i++)
        {
            buf32[i] = egui_pfb_manager_swap_rgb565_pair(buf32[i]);
        }

        buf16 = (uint16_t *)&buf32[pair_count];
        pixel_count -= pair_count << 1;
    }
#endif

    while (pixel_count != 0)
    {
        *buf16 = egui_pfb_manager_swap_rgb565_pixel(*buf16);
        buf16++;
        pixel_count--;
    }
}
#endif

static void egui_pfb_manager_start_flush(egui_pfb_manager_t *mgr)
{
    egui_display_driver_t *drv = egui_display_driver_get(mgr->core);
    if (drv == NULL || drv->ops->draw_area == NULL)
    {
        return;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
    // Bulk byte-swap the PFB tile before sending to display.
    // Per-display runtime check replaces the old compile-time EGUI_CONFIG_COLOR_16_SWAP.
    if (mgr->core->render.color_16_swap)
    {
        // Swap the buffer in place because this ring slot is now owned by the flush side, not by the renderer.
        egui_pfb_flush_params_t *p = &mgr->flush_params[mgr->flush_idx];
        uint32_t pixel_count = (uint32_t)p->w * p->h;
        egui_pfb_manager_swap_rgb565_buffer(mgr->buffers[mgr->flush_idx], pixel_count);
    }
#endif

    egui_pfb_flush_params_t *p = &mgr->flush_params[mgr->flush_idx];
    mgr->dma_busy = 1;
    drv->ops->draw_area(mgr->core, p->x, p->y, p->w, p->h, mgr->buffers[mgr->flush_idx]);
}

/** Advance the flush queue after one tile transfer completes and auto-start the next queued tile when allowed. */
void egui_pfb_manager_notify_flush_complete(egui_pfb_manager_t *mgr)
{
    // Retire the tile that just finished transferring.
    mgr->flush_idx = (mgr->flush_idx + 1) % mgr->buffer_count;
    mgr->pending_count--;

    // Immediately chain the next queued tile unless another peripheral currently owns the display bus.
    if (mgr->pending_count > 0 && !mgr->bus_locked)
    {
        egui_pfb_manager_start_flush(mgr);
    }
    else
    {
        mgr->dma_busy = 0;
    }
}

/** Wait for the currently active transfer to finish, handling both ISR-driven and polling-only ports. */
static void egui_pfb_manager_wait_last_complete(egui_pfb_manager_t *mgr)
{
    if (mgr->dma_busy)
    {
        egui_display_driver_t *drv = egui_display_driver_get(mgr->core);
        if (drv != NULL && drv->ops->wait_draw_complete != NULL)
        {
            drv->ops->wait_draw_complete(mgr->core);
        }
        // Only notify if the ISR hasn't already done so during wait_draw_complete.
        // When an ISR calls notify_flush_complete while we busy-waited, dma_busy
        // is already cleared; calling notify again would underflow pending_count.
        if (mgr->dma_busy)
        {
            egui_pfb_manager_notify_flush_complete(mgr);
        }
    }
}

/** Return the buffer currently reserved for CPU rendering. */
egui_color_int_t *egui_pfb_manager_get_active(egui_pfb_manager_t *mgr)
{
    return mgr->buffers[mgr->render_idx];
}

/** Get a renderable buffer, blocking only when every ring slot is still owned by the flush side. */
egui_color_int_t *egui_pfb_manager_get_render_buffer(egui_pfb_manager_t *mgr)
{
    // Ring full: all buffers occupied by pending/sending data.
    // Wait for DMA to free one.
    while (mgr->pending_count >= mgr->buffer_count)
    {
        // Try polling: use wait_draw_complete + manual notify for non-interrupt mode
        egui_pfb_manager_wait_last_complete(mgr);
    }

    return mgr->buffers[mgr->render_idx];
}

/**
 * Submit the current tile for flushing.
 * The render index is advanced first so CPU rendering can continue in parallel as soon as another free slot becomes available.
 */
void egui_pfb_manager_submit(egui_pfb_manager_t *mgr, int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    EGUI_UNUSED(data);

    egui_display_driver_t *drv = egui_display_driver_get(mgr->core);
    if (drv == NULL || drv->ops->draw_area == NULL)
    {
        return;
    }

    // Store flush params and enqueue
    egui_pfb_flush_params_t *p = &mgr->flush_params[mgr->render_idx];
    p->x = x;
    p->y = y;
    p->w = w;
    p->h = h;

    // Hand ownership of the current slot to the flush side and move the renderer to the next slot.
    mgr->render_idx = (mgr->render_idx + 1) % mgr->buffer_count;

    // If the ring is full, wait until at least one previously submitted slot retires.
    while (mgr->pending_count >= mgr->buffer_count)
    {
        egui_pfb_manager_wait_last_complete(mgr);
    }

    // Update the shared queue state atomically because flush completion may happen in interrupt context.
    __egui_disable_isr();
    mgr->pending_count++;
    uint8_t was_idle = !mgr->dma_busy;
    uint8_t locked = mgr->bus_locked;
    __egui_enable_isr();

    // Kick the transfer engine only when no older tile is already in flight.
    if (was_idle && !locked)
    {
        egui_pfb_manager_start_flush(mgr);

        // wait_draw_complete == NULL means draw_area is synchronous (blocking),
        // mark flush as complete immediately since transfer already finished.
        if (drv->ops->wait_draw_complete == NULL)
        {
            egui_pfb_manager_notify_flush_complete(mgr);
        }
    }
}

/** Block until the flush queue becomes fully empty. */
void egui_pfb_manager_wait_all_complete(egui_pfb_manager_t *mgr)
{
    while (mgr->pending_count > 0 || mgr->dma_busy)
    {
        // Try polling: use wait_draw_complete + manual notify for non-interrupt mode
        egui_pfb_manager_wait_last_complete(mgr);
    }
}

/** Return non-zero when the display driver exposes an asynchronous flush completion hook. */
int egui_pfb_manager_is_async(egui_pfb_manager_t *mgr)
{
    egui_display_driver_t *drv = egui_display_driver_get(mgr->core);
    if (drv != NULL && drv->ops->wait_draw_complete != NULL)
    {
        return 1;
    }
    return 0;
}

/** Legacy no-op kept for old double-buffer call sites. */
void egui_pfb_manager_swap(egui_pfb_manager_t *mgr)
{
    EGUI_UNUSED(mgr);
    // Legacy no-op: multi-buffer mode uses submit/notify instead.
    // For single buffer, nothing to swap.
}

/** Prevent new transfers from starting and wait until the current display-bus user finishes. */
void egui_pfb_manager_bus_acquire(egui_pfb_manager_t *mgr)
{
    // Set locked flag first to prevent new DMA starts from ISR
    mgr->bus_locked = 1;

    // Wait for any in-progress DMA transfer to complete
    egui_pfb_manager_wait_last_complete(mgr);
}

/** Release a previously locked display bus and resume queued tile flushes if needed. */
void egui_pfb_manager_bus_release(egui_pfb_manager_t *mgr)
{
    mgr->bus_locked = 0;

    // If tiles were queued during the lock period, start flushing now
    if (mgr->pending_count > 0 && !mgr->dma_busy)
    {
        egui_pfb_manager_start_flush(mgr);
    }
}
