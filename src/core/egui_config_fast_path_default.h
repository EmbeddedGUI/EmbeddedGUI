#ifndef _EGUI_CONFIG_FAST_PATH_DEFAULT_H_
#define _EGUI_CONFIG_FAST_PATH_DEFAULT_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* ---- Image codec / cache policy ---- */

/* Maximum bytes per pixel for image decode buffers. */
#ifndef EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE
#define EGUI_CONFIG_IMAGE_DECODE_MAX_PIXEL_SIZE 2
#endif

/* Enable row-band decode cache for compressed image codecs. */
#ifndef EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_ROW_CACHE_ENABLE 0
#endif

/* Keep only the later-tile tail columns in codec row cache. */
#ifndef EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE
#define EGUI_CONFIG_IMAGE_CODEC_TAIL_ROW_CACHE_ENABLE 0
#endif

/* Optional full-image persistent cache budget for compressed images. */
#ifndef EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES
#define EGUI_CONFIG_IMAGE_CODEC_PERSISTENT_CACHE_MAX_BYTES 0
#endif

/* Optional full-image persistent cache budget for external standard images. */
#ifndef EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES
#define EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES 0
#endif

/* RLE external-resource I/O window size. */
#ifndef EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE
#define EGUI_CONFIG_IMAGE_RLE_EXTERNAL_CACHE_WINDOW_SIZE 1024
#endif

/* Remaining fast-path toggles now stay next to the implementation units that
 * consume them, with optional app-side override bridges. This shared default
 * header keeps only common codec/cache policy knobs.
 */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_FAST_PATH_DEFAULT_H_ */
