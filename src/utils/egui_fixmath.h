#ifndef _EGUI_FIXMATH_H_
#define _EGUI_FIXMATH_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "config/egui_config.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  The fixed-point data type.
 */
typedef int32_t egui_fixed_t;

/**
 *  Count the number of bits set in a 32-bit word.
 */
static inline uint32_t egui_fx_bitcount_u32(uint32_t word)
{
    word = word - ((word >> 1) & UINT32_C(0x55555555));
    word = (word & UINT32_C(0x33333333)) + ((word >> 2) & UINT32_C(0x33333333));
    return (((word + (word >> 4)) & UINT32_C(0x0f0f0f0f)) * UINT32_C(0x01010101)) >> 24;
}

/**
 *  Count the number of leading zeros in a 32-bit word.
 *  Note that fx_clz(0) is undefined.
 */
static inline uint32_t egui_fx_clz_u32(uint32_t word)
{
    word |= (word >> 1);
    word |= (word >> 2);
    word |= (word >> 4);
    word |= (word >> 8);
    word |= (word >> 16);
    return egui_fx_bitcount_u32(~word);
}

/**
 *  Count the number of trailing zeros in a 32-bit word.
 *  Note that fx_ctz(0) is undefined.
 */
static inline uint32_t egui_fx_ctz_u32(uint32_t word)
{
    word |= (word << 1);
    word |= (word << 2);
    word |= (word << 4);
    word |= (word << 8);
    word |= (word << 16);
    return egui_fx_bitcount_u32(~word);
}

#define EGUI_FX_BITCOUNT(word) egui_fx_bitcount_u32((uint32_t)(word))
#define EGUI_FX_CLZ(word)      egui_fx_clz_u32((uint32_t)(word))
#define EGUI_FX_CTZ(word)      egui_fx_ctz_u32((uint32_t)(word))

/**
 *  Signed fixed-point multiply, i.e. s32 x s32 -> s64.
 *  avoid use 64bit multiply.
 */
#define EGUI_FX_SMUL_LIMIT(x1, x2, frac) ((int32_t)(x1) * (int32_t)(x2) >> (frac))

/**
 *  Unsigned fixed-point multiply, i.e. u32 x u32 -> u64.
 *  avoid use 64bit multiply.
 */
#define EGUI_FX_UMUL_LIMIT(x1, x2, frac) ((uint32_t)(x1) * (uint32_t)(x2) >> (frac))

/**
 *  Signed fixed-point multiply, i.e. s32 x s32 -> s64.
 */
#define EGUI_FX_SMUL(x1, x2, frac) ((int64_t)(x1) * (int64_t)(x2) >> (frac))

/**
 *  Unsigned fixed-point multiply, i.e. u32 x u32 -> u64.
 */
#define EGUI_FX_UMUL(x1, x2, frac) ((uint64_t)(x1) * (uint64_t)(x2) >> (frac))

/**
 *  Convert a fixed-point number to a normalized
 *  floating-point number with 32-bit mantissa.
 */
#define EGUI_FX_NORMALIZE(mant, expn, xval, frac)                                                                                                              \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        int nz__ = EGUI_FX_CLZ(xval);                                                                                                                          \
        (mant) = (uint32_t)(xval) << nz__;                                                                                                                     \
        (expn) = 31 - nz__ - (frac);                                                                                                                           \
    } while (0)

/**
 *  Compute value * 2**shift.
 */
#define EGUI_FX_SHIFT(value, shift) (uint32_t)((shift) > 0 ? (value) << (shift) : (shift) > -31 ? (value) >> -(shift) : 0)

/**
 *  Fixed-point divide. The error is within 1/2 ulp.
 *  The number of fraction bits in the result is @e f1 - @e f2 + @e frac,
 *  where @e f1 and @e f2 are the number of fraction bits of @e x1 and
 *  @e x2, respectively. Thus, if both @e f1 and @e f2 are equal to @e frac,
 *  the number of fraction bits in the result will also be @e frac.
 *
 *  @param x1    Fixed-point numerator.
 *  @param x2    Fixed-point denominator.
 *  @param frac  Number of fraction bits, see explaination above.
 *  @return      The quotient result.
 */
#define EGUI_FX_DIVX(x1, x2, frac) (int32_t)((((int64_t)(x1) << (frac)) + ((int32_t)(x1) < 0 ? -labs((int32_t)(x2)) : labs((int32_t)(x2))) / 2) / (x2))

#define EGUI_FX_DIVX_LIMIT(x1, x2, frac) (int32_t)((((int32_t)(x1) << (frac)) + ((int32_t)(x1) < 0 ? -labs((int32_t)(x2)) : labs((int32_t)(x2))) / 2) / (x2))

/**
 *  Integer to fixed-point conversion.
 */
#define EGUI_FX_ITOX(ival, frac) ((int32_t)((uint32_t)(int32_t)(ival) << (frac)))

/**
 *  Single precision float to fixed-point conversion.
 */
#define EGUI_FX_FTOX(fval, frac) (int32_t)((float)(fval) * (float)(1UL << (frac)) + ((float)(fval) > 0 ? 0.5f : -0.5f))

#if !EGUI_CONFIG_PERFORMANCE_USE_FLOAT
#define EGUI_FLOAT_FRAC 16

#define EGUI_FLOAT_VALUE_INT(_val) ((int32_t)(_val) << EGUI_FLOAT_FRAC)
#define EGUI_FLOAT_VALUE(_val)     EGUI_FX_FTOX(_val, EGUI_FLOAT_FRAC)
#define egui_float_t               int32_t // Q16.16 fixed-point type

// In performance-critical code, use the following macros to avoid
// but need make sure that the result is int32_t.
// Second argument must be a value with EGUI_FLOAT_FRAC.
#define EGUI_FLOAT_MULT_LIMIT(x1, x2) EGUI_FX_SMUL_LIMIT((x1), (x2) >> 8, (EGUI_FLOAT_FRAC - 8))
#define EGUI_FLOAT_DIV_LIMIT(x1, x2)  EGUI_FX_DIVX_LIMIT(x1, x2, EGUI_FLOAT_FRAC)

#define EGUI_FLOAT_MULT(x1, x2) EGUI_FX_SMUL(x1, x2, EGUI_FLOAT_FRAC)
#define EGUI_FLOAT_DIV(x1, x2)  EGUI_FX_DIVX(x1, x2, EGUI_FLOAT_FRAC)

#define EGUI_FLOAT_POWER(x1, x2) egui_fx_powx(x1, EGUI_FLOAT_FRAC, x2, EGUI_FLOAT_FRAC)
#define EGUI_FLOAT_SIN(x1)       egui_fx_sinx(x1, EGUI_FLOAT_FRAC)
#define EGUI_FLOAT_COS(x1)       egui_fx_cosx(x1, EGUI_FLOAT_FRAC)

#define EGUI_FLOAT_PI EGUI_FLOAT_VALUE(3.14159265358979f)

#define EGUI_FLOAT_INT_PART(_x)  ((_x >> EGUI_FLOAT_FRAC))
#define EGUI_FLOAT_FRAC_PART(_x) ((_x & ((1 << EGUI_FLOAT_FRAC) - 1)))

#else
#define EGUI_FLOAT_FRAC 0

#define EGUI_FLOAT_VALUE_INT(_val) _val
#define EGUI_FLOAT_VALUE(_val)     _val
#define egui_float_t               float // Q16.16 fixed-point type

#define EGUI_FLOAT_MULT_LIMIT(x1, x2) (x1) * (x2)
#define EGUI_FLOAT_DIV_LIMIT(x1, x2)  (float)(x1) / (float)(x2)

#define EGUI_FLOAT_MULT(x1, x2) (x1) * (x2)
#define EGUI_FLOAT_DIV(x1, x2)  (float)(x1) / (float)(x2)

#define EGUI_FLOAT_POWER(x1, x2) powf((float)(x1), (float)(x2))
#define EGUI_FLOAT_SIN(x1)       sinf((float)(x1))
#define EGUI_FLOAT_COS(x1)       cosf((float)(x1))

#define EGUI_FLOAT_PI 3.14159265358979f

#define EGUI_FLOAT_INT_PART(_x)  ((int32_t)_x)
#define EGUI_FLOAT_FRAC_PART(_x) (_x - EGUI_FLOAT_INT_PART(_x))
#endif

egui_fixed_t egui_fx_sinx(egui_fixed_t xval, unsigned frac);
egui_fixed_t egui_fx_cosx(egui_fixed_t xval, unsigned frac);

egui_fixed_t egui_fx_powx(egui_fixed_t xval, unsigned xfrac, egui_fixed_t yval, unsigned yfrac);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_FIXMATH_H_ */
