#ifndef _EGUI_OOP_H_
#define _EGUI_OOP_H_

/**
 * @file egui_oop.h
 * @brief C OOP helper macros for reducing explicit type casts
 *
 * This file provides macros to simplify C OOP patterns used in EmbeddedGUI.
 * All macros are compile-time expanded with zero runtime overhead.
 * Compatible with C99 standard.
 */

#include "egui_config.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Part 1: Self pointer cast macros
 * Used at function entry to cast base class pointer to derived class pointer
 * ============================================================================*/

/**
 * @brief Declare and initialize derived class local pointer
 * @param _type Derived class type name (e.g., egui_view_label_t)
 *
 * Usage:
 *   void egui_view_label_set_text(egui_view_t *self, const char *text) {
 *       EGUI_LOCAL_INIT(egui_view_label_t);
 *       local->text = text;
 *   }
 */
#define EGUI_LOCAL_INIT(_type)                                                                                                                                 \
    _type *local = (_type *)self;                                                                                                                              \
    (void)(local)

/**
 * @brief Self cast with custom variable name (no type checking)
 * @param _type Derived class type name
 * @param _var  Custom variable name
 */
#define EGUI_LOCAL_INIT_VAR(_type, _var)                                                                                                                       \
    _type *_var = (_type *)self;                                                                                                                               \
    (void)(_var)

/**
 * @brief Self cast for init functions
 * @param _type Derived class type name
 *
 * Usage:
 *   void egui_view_label_init(egui_view_t *self) {
 *       EGUI_INIT_LOCAL(egui_view_label_t);
 *       egui_view_init(self);
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
 * @brief Get base class pointer from embedded member (upcast)
 * @param _ptr Derived class struct pointer or base member pointer
 *
 * Usage:
 *   egui_view_group_add_child(EGUI_VIEW_OF(&local->container), child);
 */
#define EGUI_VIEW_OF(_ptr)   ((egui_view_t *)(_ptr))
#define EGUI_ANIM_OF(_ptr)   ((egui_animation_t *)(_ptr))
#define EGUI_BG_OF(_ptr)     ((egui_background_t *)(_ptr))
#define EGUI_INTERP_OF(_ptr) ((egui_interpolator_t *)(_ptr))
#define EGUI_MASK_OF(_ptr)   ((egui_mask_t *)(_ptr))
#define EGUI_FONT_OF(_ptr)   ((egui_font_t *)(_ptr))

/**
 * @brief Get base pointer via derived class's base member
 * @param _derived_ptr Derived class pointer
 *
 * Assumes derived class has 'base' as first member
 */
#define EGUI_BASE_OF(_derived_ptr) ((void *)&((_derived_ptr)->base))

/* ============================================================================
 * Part 3: Downcast macros (base -> derived)
 * Use only when actual type is known, no runtime check provided
 * ============================================================================*/

/**
 * @brief Explicit downcast (intentional unsafe operation)
 * @param _type Target derived class type
 * @param _ptr  Base class pointer
 *
 * Usage:
 *   egui_view_label_t *label = EGUI_CAST_TO(egui_view_label_t, base_ptr);
 */
#define EGUI_CAST_TO(_type, _ptr) ((_type *)(_ptr))

/* ============================================================================
 * Part 4: Parent pointer access macros
 * Simplify type casts in parent-child traversal
 * ============================================================================*/

/**
 * @brief Get view's parent pointer (typed as egui_view_t*)
 * @param _view egui_view_t pointer or derived class pointer
 */
#define EGUI_VIEW_PARENT(_view) ((egui_view_t *)(((egui_view_t *)(_view))->parent))

/**
 * @brief Check if view has parent
 */
#define EGUI_VIEW_HAS_PARENT(_view) (((egui_view_t *)(_view))->parent != NULL)

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_OOP_H_ */
