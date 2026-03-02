#ifndef _EGUI_PFB_MANAGER_H_
#define _EGUI_PFB_MANAGER_H_

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maximum number of PFB buffers supported.
 * User configures actual count via EGUI_CONFIG_PFB_BUFFER_COUNT (1..4).
 */
#define EGUI_PFB_BUFFER_MAX 4

/**
 * Draw parameters stored per ring entry.
 * When CPU submits a rendered tile, these params are saved so DMA ISR
 * can auto-chain the next transfer without CPU intervention.
 */
typedef struct egui_pfb_flush_params
{
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} egui_pfb_flush_params_t;

/**
 * PFB ring buffer manager.
 *
 * Single buffer (count=1): synchronous draw, no overlap.
 * Multi-buffer (count>=2): ring queue enables CPU/DMA parallelism.
 *   - CPU renders into buffers[render_idx], then submits.
 *   - DMA sends buffers[flush_idx], advances via ISR callback.
 *   - CPU can run ahead up to (count-1) tiles.
 */
struct egui_pfb_manager
{
    egui_color_int_t *buffers[EGUI_PFB_BUFFER_MAX];
    egui_pfb_flush_params_t flush_params[EGUI_PFB_BUFFER_MAX];

    int16_t width;
    int16_t height;
    int buffer_size_bytes;

    uint8_t buffer_count;           // Total buffers configured (1..EGUI_PFB_BUFFER_MAX)
    uint8_t render_idx;             // Next buffer for CPU to render into
    uint8_t flush_idx;              // Next buffer for DMA to send
    volatile uint8_t pending_count; // Buffers rendered but not yet flushed by DMA
    volatile uint8_t dma_busy;      // 1 if DMA transfer is in progress
    volatile uint8_t bus_locked;    // 1 if SPI bus is locked (e.g., flash access)
};

/**
 * Initialize PFB manager with a single buffer.
 * Call egui_pfb_manager_add_buffer() to add more buffers for multi-buffering.
 */
void egui_pfb_manager_init(egui_pfb_manager_t *mgr, egui_color_int_t *pfb, int16_t width, int16_t height, int color_bytes);

/**
 * Add an additional buffer to the ring.
 * Can be called multiple times up to EGUI_PFB_BUFFER_MAX total.
 * Must be called before rendering starts (i.e., before egui_screen_on).
 */
void egui_pfb_manager_add_buffer(egui_pfb_manager_t *mgr, egui_color_int_t *buf);

/**
 * Get the current render buffer for CPU to draw into.
 * If all buffers are occupied (ring full), blocks until DMA frees one.
 * Returns pointer to the buffer at render_idx.
 */
egui_color_int_t *egui_pfb_manager_get_render_buffer(egui_pfb_manager_t *mgr);

/**
 * Submit the current render buffer to the flush queue.
 * Stores draw parameters and advances render_idx.
 * If DMA is idle and display driver supports async, starts DMA immediately.
 * If buffer_count == 1, performs synchronous draw.
 */
void egui_pfb_manager_submit(egui_pfb_manager_t *mgr, int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data);

/**
 * Called from DMA completion ISR (or equivalent callback).
 * Advances flush_idx and auto-chains the next pending buffer if available.
 * Safe to call from interrupt context.
 */
void egui_pfb_manager_notify_flush_complete(egui_pfb_manager_t *mgr);

/**
 * Wait for all pending DMA transfers to complete.
 * Call this at end of frame to ensure all tiles are flushed before frame sync.
 */
void egui_pfb_manager_wait_all_complete(egui_pfb_manager_t *mgr);

/**
 * Check if multi-buffer async mode is active (buffer_count >= 2 and async draw available).
 */
int egui_pfb_manager_is_async(egui_pfb_manager_t *mgr);

/* ---- SPI bus sharing (flash + LCD on same bus) ---- */

/**
 * Acquire exclusive SPI bus access for non-display use (e.g., flash read).
 *
 * 1. Waits for all pending DMA transfers to complete.
 * 2. Sets bus_locked flag to prevent new DMA transfers from starting.
 *
 * While locked, submit() still queues rendered tiles in the ring buffer
 * (CPU rendering is not blocked), but no DMA transfer will be initiated.
 * This keeps the SPI bus free for flash or other peripherals.
 *
 * IMPORTANT: Do not call from within the rendering loop on the same thread,
 * as the ring buffer may fill up and cause a deadlock. Call from the main
 * loop or a separate task before/after egui_polling_work().
 */
void egui_pfb_manager_bus_acquire(egui_pfb_manager_t *mgr);

/**
 * Release SPI bus after non-display access.
 *
 * Clears bus_locked flag. If tiles were queued during the lock period,
 * immediately starts DMA to flush them.
 */
void egui_pfb_manager_bus_release(egui_pfb_manager_t *mgr);

/* ---- Legacy compatibility ---- */

/**
 * Legacy: enable double buffering with a backup buffer.
 * Equivalent to egui_pfb_manager_add_buffer(mgr, backup).
 */
void egui_pfb_manager_set_backup_buffer(egui_pfb_manager_t *mgr, egui_color_int_t *backup);

/**
 * Legacy: get active buffer pointer.
 * Returns buffers[render_idx].
 */
egui_color_int_t *egui_pfb_manager_get_active(egui_pfb_manager_t *mgr);

/**
 * Legacy: swap buffers (for backward compatibility with old double-buffer code).
 * In multi-buffer mode, this is a no-op — use submit/notify instead.
 */
void egui_pfb_manager_swap(egui_pfb_manager_t *mgr);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_PFB_MANAGER_H_ */
