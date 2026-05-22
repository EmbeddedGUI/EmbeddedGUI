#ifndef _EGUI_WIDGET_STYLE_H_
#define _EGUI_WIDGET_STYLE_H_

#include "core/egui_common.h"
#include "background/egui_background.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Lightweight shared style descriptor.
 *
 * An egui_view_style_t is a small, ROM-storable struct that bundles a set of
 * optional appearance properties.  Multiple style objects can be stacked on
 * one view (see egui_view_add_style / egui_view_remove_style).  The view's
 * own inline properties always win; among styles the last-added (highest
 * index) wins.
 *
 * Current cascade properties
 *   - background : fallback background when the view has no inline background
 *   - alpha      : fallback alpha    when the view alpha was never explicitly set
 *
 * Style objects are meant to be declared const (in ROM) and shared across
 * many views.  Never copy or heap-allocate them.
 */
typedef struct egui_view_style egui_view_style_t;
struct egui_view_style
{
    const egui_background_t *background; /**< NULL = not provided by this style */
    egui_alpha_t alpha;                  /**< valid only when has_alpha != 0    */
    uint8_t has_alpha : 1;
#if EGUI_CONFIG_FUNCTION_VIEW_STATE_STYLES
    /**
     * State filter for this style entry.
     * 0 = always applied (DEFAULT).
     * Non-zero = only applied when ALL of these bits are present in the view's
     * current computed state (pressed / focused / disabled / checked).
     * Use EGUI_VIEW_STATE_* constants from egui_view.h.
     */
    uint8_t state_mask;
#endif
};

/* ----------------------------- Init macros ----------------------------- */

/** Style providing only a background (alpha unchanged). */
#define EGUI_STYLE_INIT_BACKGROUND(_bg) {.background = (_bg), .has_alpha = 0}

/** Style providing only an alpha value (background unchanged). */
#define EGUI_STYLE_INIT_ALPHA(_alpha) {.background = NULL, .alpha = (_alpha), .has_alpha = 1}

/** Style providing both background and alpha. */
#define EGUI_STYLE_INIT(_bg, _alpha) {.background = (_bg), .alpha = (_alpha), .has_alpha = 1}

/* ----------------------------- Declare macros -------------------------- */

/**
 * Declare a const egui_style_t with a background only (no alpha override).
 *   EGUI_STYLE_DECLARE_BG(my_style, &my_background);
 */
#define EGUI_STYLE_DECLARE_BG(_name, _bg) static const egui_view_style_t _name = EGUI_STYLE_INIT_BACKGROUND(_bg)

/**
 * Declare a const egui_style_t with both background and alpha.
 *   EGUI_STYLE_DECLARE(my_style, &my_background, EGUI_ALPHA_50);
 */
#define EGUI_STYLE_DECLARE(_name, _bg, _alpha) static const egui_view_style_t _name = EGUI_STYLE_INIT(_bg, _alpha)

/* ----------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_WIDGET_STYLE_H_ */
