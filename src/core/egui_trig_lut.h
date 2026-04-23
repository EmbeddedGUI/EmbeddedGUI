#ifndef _EGUI_TRIG_LUT_H_
#define _EGUI_TRIG_LUT_H_

/**
 * @file egui_trig_lut.h
 * @brief Small trigonometry helpers used by fixed-point drawing code.
 */

#include <stdint.h>

#include "utils/egui_fixmath.h"

/** Precomputed sine values for 0..90 degrees, used to mirror results into the other quadrants. */
extern const egui_float_t egui_trig_sin_lut[91];

/** Round one `egui_float_t` value to the nearest signed 32-bit integer. */
static inline int32_t egui_trig_round_to_int32(egui_float_t value)
{
#if !EGUI_CONFIG_PERFORMANCE_USE_FLOAT
    if (value >= 0)
    {
        return (int32_t)((value + EGUI_FLOAT_VALUE(0.5f)) >> EGUI_FLOAT_FRAC);
    }

    return -(int32_t)(((-value) + EGUI_FLOAT_VALUE(0.5f)) >> EGUI_FLOAT_FRAC);
#else
    if (value >= 0.0f)
    {
        return (int32_t)(value + 0.5f);
    }

    return -(int32_t)((-value) + 0.5f);
#endif
}

/** Convert one normalized `egui_float_t` value in approximately [-1, 1] to signed Q15 format. */
static inline int32_t egui_trig_float_to_q15(egui_float_t value)
{
    return egui_trig_round_to_int32(EGUI_FLOAT_MULT(value, EGUI_FLOAT_VALUE(32767.0f)));
}

#endif /* _EGUI_TRIG_LUT_H_ */
