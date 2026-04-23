#ifndef _EGUI_OOP_H_
#define _EGUI_OOP_H_

/**
 * @file egui_oop.h
 * @brief C OOP helper macros used by EmbeddedGUI's struct-embedding pattern.
 *
 * EmbeddedGUI models inheritance by embedding a base struct as the first
 * member of a derived struct. These macros keep the resulting upcasts and
 * downcasts readable while still expanding to plain C casts with zero
 * runtime overhead.
 */

#include "config/egui_config.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Part 1: Self pointer cast macros
 * Used at function entry to cast base class pointer to derived class pointer
 * ============================================================================*/

/**
 * @brief Downcast the incoming `self` pointer and expose it as `local`.
 * @param _type Derived type name (for example `egui_view_label_t`)
 *
 * Use this at the top of instance-style functions whose first parameter is a base pointer named `self`.
 *
 * Example:
 *   void egui_view_label_set_text(egui_view_t *self, const char *text) {
 *       EGUI_LOCAL_INIT(egui_view_label_t);
 *       local->text = text;
 *   }
 */
#define EGUI_LOCAL_INIT(_type)                                                                                                                                 \
    _type *local = (_type *)self;                                                                                                                              \
    (void)(local)

/**
 * @brief Downcast the incoming `self` pointer with a custom local variable name.
 * @param _type Derived type name
 * @param _var  Custom variable name that will receive the cast result
 */
#define EGUI_LOCAL_INIT_VAR(_type, _var)                                                                                                                       \
    _type *_var = (_type *)self;                                                                                                                               \
    (void)(_var)

/**
 * @brief Same as `EGUI_LOCAL_INIT`, but named for constructor/init functions.
 * @param _type Derived type name
 *
 * The generated code is identical to `EGUI_LOCAL_INIT`; the separate name simply makes initialization code read more naturally.
 *
 * Example:
 *   void egui_view_label_init(egui_view_t *self, egui_core_t *core) {
 *       EGUI_INIT_LOCAL(egui_view_label_t);
 *       egui_view_init(self, core);
 *       self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_label_t);
 *       local->text = NULL;
 *   }
 */
#define EGUI_INIT_LOCAL(_type)                                                                                                                                 \
    _type *local = (_type *)self;                                                                                                                              \
    (void)(local)

/* ============================================================================
 * Part 2: Upcast macros (derived -> base)
 * Type-safe, used to cast derived class pointer to base class pointer
 * ============================================================================*/

/**
 * @brief Upcast a derived object or embedded base member to the base API type.
 * @param _ptr Pointer whose address is layout-compatible with the requested base type
 *
 * Example:
 *   egui_view_group_add_child(EGUI_VIEW_OF(&local->container), child);
 */
#define EGUI_VIEW_OF(_ptr)   ((egui_view_t *)(_ptr))
#define EGUI_ANIM_OF(_ptr)   ((egui_animation_t *)(_ptr))
#define EGUI_BG_OF(_ptr)     ((egui_background_t *)(_ptr))
#define EGUI_INTERP_OF(_ptr) ((egui_interpolator_t *)(_ptr))
#define EGUI_MASK_OF(_ptr)   ((egui_mask_t *)(_ptr))
#define EGUI_FONT_OF(_ptr)   ((egui_font_t *)(_ptr))

/**
 * @brief Get the address of a derived object's `base` member.
 * @param _derived_ptr Derived object pointer whose base field is named `base`
 *
 * This macro assumes the derived type embeds a member named `base`.
 */
#define EGUI_BASE_OF(_derived_ptr) ((void *)&((_derived_ptr)->base))

/* ============================================================================
 * Part 3: Downcast macros (base -> derived)
 * Use only when actual type is known, no runtime check provided
 * ============================================================================*/

/**
 * @brief Explicitly downcast a base pointer when the real dynamic type is known.
 * @param _type Target derived type
 * @param _ptr  Base pointer or compatible object pointer
 *
 * No runtime type checking is performed.
 *
 * Example:
 *   egui_view_label_t *label = EGUI_CAST_TO(egui_view_label_t, base_ptr);
 */
#define EGUI_CAST_TO(_type, _ptr) ((_type *)(_ptr))

/* ============================================================================
 * Part 4: Parent pointer access macros
 * Simplify type casts in parent-child traversal
 * ============================================================================*/

/**
 * @brief Fetch one view's parent pointer as `egui_view_t *`.
 * @param _view `egui_view_t *` or any derived view pointer
 */
#define EGUI_VIEW_PARENT(_view) ((egui_view_t *)(((egui_view_t *)(_view))->parent))

/**
 * @brief Check whether one view is already attached to a parent container.
 */
#define EGUI_VIEW_HAS_PARENT(_view) (((egui_view_t *)(_view))->parent != NULL)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_OOP_H_ */
