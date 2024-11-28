#include <string.h>

#include "egui_fixmath.h"
#include "core/egui_api.h"

#if !EGUI_CONFIG_PERFORMANCE_USE_FLOAT
/**
 *  Base-2 fractional logarithm.
 *  Computes log2(1 + x), where x is an unsigned Q.32 fractional number
 *  in the range [0,1). This yields a Q1.31 result in the [0,1] range.
 *
 *  The logarithm is approximated by a per-segment polynomial of degree
 *  four, computed by the Remez' function approximation algorithm. There
 *  are 16 segments, requiring a lookup table of 320 bytes for the
 *  coefficients. The result is an unsigned Q1.31 fixed-point number.
 */
static uint32_t egui_fx_core_log2(uint32_t fpart32)
{
    /* The 320 bytes large lookup table of polynomial coefficients */
    static const uint32_t table[16][5] = {
            {0000000002UL, 0xb8aa3809UL, 0xb8a71111UL, 0xf5181ba1UL, 0xa3741666UL}, {0x1663f6fcUL, 0xadcd6287UL, 0xa391c7e9UL, 0xcc703403UL, 0x8128cedeUL},
            {0x2b803475UL, 0xa4258829UL, 0x91e6b29dUL, 0xac4bac7eUL, 0x67680fa0UL}, {0x3f782d73UL, 0x9b81dfa0UL, 0x82f2bdb3UL, 0x928ce148UL, 0x53c3aca5UL},
            {0x5269e130UL, 0x93bb617aUL, 0x762e71acUL, 0x7daff66fUL, 0x449294cbUL}, {0x646eea25UL, 0x8cb27563UL, 0x6b31ccc5UL, 0x6c9a3c9dUL, 0x38ac9277UL},
            {0x759d4f81UL, 0x864d41a3UL, 0x61abe29bUL, 0x5e7a6031UL, 0x2f3f8599UL}, {0x86082807UL, 0x80766b68UL, 0x595cf97fUL, 0x52b32231UL, 0x27b3f257UL},
            {0x95c01a3aUL, 0x7b1c2702UL, 0x5212559bUL, 0x48ccd634UL, 0x219b062aUL}, {0xa4d3c25fUL, 0x762f81acUL, 0x4ba32b4eUL, 0x406b50e8UL, 0x1ca29548UL},
            {0xb3500472UL, 0x71a3d559UL, 0x45ee5fbaUL, 0x3946ce96UL, 0x188cf14aUL}, {0xc1404eaeUL, 0x6d6e5bb5UL, 0x40d8dba4UL, 0x3326de10UL, 0x152b42bbUL},
            {0xceaecfebUL, 0x6985d876UL, 0x3c4c47a5UL, 0x2ddeb388UL, 0x1259a653UL}, {0xdba4a47bUL, 0x65e25570UL, 0x38361502UL, 0x294a6f77UL, 0x0ffc3eb3UL},
            {0xe829fb69UL, 0x627cec35UL, 0x3486bf54UL, 0x254d2025UL, 0x0dfd4de4UL}, {0xf446359bUL, 0x5f4f9a49UL, 0x3131385eUL, 0x21cf3d01UL, 0x0c4bacbdUL}};

    uint32_t c0, c1, c2, c3, c4; /* Polynomial coefficients   Q.32     */
    uint32_t x1, x2, x3, x4;     /* Fractional powers         Q.32     */
    int64_t acc;                 /* Result accumulation word  Q31.32   */
    int ix;                      /* Lookup table index        integral */

    /* Compute the segment index from the four most significant bits */
    ix = fpart32 >> 28;

    /* Fetch the five polynomial coefficients for this segment */
    c0 = table[ix][0];
    c1 = table[ix][1];
    c2 = table[ix][2];
    c3 = table[ix][3];
    c4 = table[ix][4];

    /* Initialize segment fractional x1 and accumulator acc */
    x1 = fpart32 << 4;
    acc = 0;

    /* Two independent multiplies */
    x2 = (uint32_t)EGUI_FX_UMUL(x1, x1, 32);
    acc += EGUI_FX_UMUL(c1, x1, 35);

    /* Three independent multiplies */
    x3 = (uint32_t)EGUI_FX_UMUL(x2, x1, 32);
    x4 = (uint32_t)EGUI_FX_UMUL(x2, x2, 32);
    acc -= EGUI_FX_UMUL(c2, x2, 40);

    /* Two independent multiplies */
    acc += EGUI_FX_UMUL(c3, x3, 45);
    acc -= EGUI_FX_UMUL(c4, x4, 49);

    /* Add constant term */
    acc += c0;

    return (uint32_t)((acc + 1) >> 1);
}

/**
 *  Base-2 fractional exponential.
 *  Computes 2**x, where x is an unsigned Q.32 fractional number in
 *  the range [0,1). This yields a Q1.31 result in the [1,2) range.
 *
 *  The exponential is approximated by a per-segment polynomial of degree
 *  three, computed by the Remez' function approximation algorithm. There
 *  are 32 segments, requiring a lookup table of 512 bytes for the
 *  coefficients. The result is an unsigned Q1.31 fixed-point number
 *  with an implicit exponent of one.
 */
static uint32_t egui_fx_core_exp2(uint32_t fpart32)
{
    /* The 512 bytes large lookup table of polynomial coefficients */
    static const uint32_t table[32][4] = {{0x80000000UL, 0x58b90c9bUL, 0x7afd6ab0UL, 0x72e946ceUL}, {0x82cd8699UL, 0x5aaa6677UL, 0x7daedb8cUL, 0x756d6e58UL},
                                          {0x85aac368UL, 0x5ca6a450UL, 0x806f652fUL, 0x77ffb0d3UL}, {0x88980e80UL, 0x5eae0331UL, 0x833f5c39UL, 0x7aa05d42UL},
                                          {0x8b95c1e4UL, 0x60c0c17eUL, 0x861f1727UL, 0x7d4fc480UL}, {0x8ea4398bUL, 0x62df1ef7UL, 0x890eee57UL, 0x800e3913UL},
                                          {0x91c3d373UL, 0x65095cc2UL, 0x8c0f3c19UL, 0x82dc0f68UL}, {0x94f4efa9UL, 0x673fbd72UL, 0x8f205cb7UL, 0x85b99db0UL},
                                          {0x9837f051UL, 0x6982850fUL, 0x9242ae7fUL, 0x88a73c0fUL}, {0x9b8d39baUL, 0x6bd1f91fUL, 0x957691d0UL, 0x8ba54488UL},
                                          {0x9ef53260UL, 0x6e2e60adUL, 0x98bc6928UL, 0x8eb4131fUL}, {0xa2704303UL, 0x70980452UL, 0x9c149928UL, 0x91d405e1UL},
                                          {0xa5fed6a9UL, 0x730f2e40UL, 0x9f7f88aaUL, 0x95057ce1UL}, {0xa9a15ab5UL, 0x75942a46UL, 0xa2fda0c5UL, 0x9848da4fUL},
                                          {0xad583eeaUL, 0x782745deUL, 0xa68f4cdfUL, 0x9b9e828aUL}, {0xb123f582UL, 0x7ac8d034UL, 0xaa34fab9UL, 0x9f06dc16UL},
                                          {0xb504f334UL, 0x7d791a2fUL, 0xadef1a78UL, 0xa2824fbdUL}, {0xb8fbaf47UL, 0x8038767cUL, 0xb1be1eb8UL, 0xa6114890UL},
                                          {0xbd08a39fUL, 0x83073998UL, 0xb5a27c97UL, 0xa9b43406UL}, {0xc12c4ccaUL, 0x85e5b9d8UL, 0xb99cabc3UL, 0xad6b81deUL},
                                          {0xc5672a11UL, 0x88d44f77UL, 0xbdad268bUL, 0xb137a476UL}, {0xc9b9bd86UL, 0x8bd3549eUL, 0xc1d469e8UL, 0xb519107bUL},
                                          {0xce248c15UL, 0x8ee3256eUL, 0xc612f593UL, 0xb9103d52UL}, {0xd2a81d92UL, 0x9204200eUL, 0xca694c0fUL, 0xbd1da4daUL},
                                          {0xd744fccbUL, 0x9536a4b4UL, 0xced7f2bbUL, 0xc141c3d0UL}, {0xdbfbb798UL, 0x987b15b2UL, 0xd35f71e1UL, 0xc57d1964UL},
                                          {0xe0ccdeecUL, 0x9bd1d780UL, 0xd80054c9UL, 0xc9d027cdUL}, {0xe5b906e7UL, 0x9f3b50cbUL, 0xdcbb29c6UL, 0xce3b7407UL},
                                          {0xeac0c6e8UL, 0xa2b7ea7dUL, 0xe1908249UL, 0xd2bf85e4UL}, {0xefe4b99cUL, 0xa6480fcfUL, 0xe680f2f3UL, 0xd75ce85aUL},
                                          {0xf5257d15UL, 0xa9ec2e52UL, 0xeb8d13a6UL, 0xdc142939UL}, {0xfa83b2dbUL, 0xada4b5fbUL, 0xf0b57f97UL, 0xe0e5d996UL}};

    uint32_t c0, c1, c2, c3; /* Polynomial coefficients   Q.32     */
    uint32_t x1, x2, x3;     /* Fractional powers         Q.32     */
    int64_t acc;             /* Result accumulation word  Q31.32   */
    int ix;                  /* Lookup table index        integral */

    /* Compute the segment index from the five most significant bits */
    ix = fpart32 >> 27;

    /* Fetch the four polynomial coefficients for this segment */
    c0 = table[ix][0];
    c1 = table[ix][1];
    c2 = table[ix][2];
    c3 = table[ix][3];

    /* Initialize segment fractional x1 and accumulator acc */
    x1 = fpart32 << 5;
    acc = 0;

    /* Two independent multiplies */
    x2 = (uint32_t)EGUI_FX_UMUL(x1, x1, 32);
    acc += EGUI_FX_UMUL(c1, x1, 36);

    /* Two independent multiplies */
    x3 = (uint32_t)EGUI_FX_UMUL(x2, x1, 32);
    acc += EGUI_FX_UMUL(c2, x2, 43);

    /* Final multiply */
    acc += EGUI_FX_UMUL(c3, x3, 50);

    /* Add constant term */
    acc += (int64_t)c0 << 1;

    return (uint32_t)((acc + 1) >> 1);
}

/**
 *  Fractional sine function.
 *  Computes sin(pi*x/2), where x is an unsigned Q.32 fractional number.
 *  This yields a Q1.31 result in the range [0,1].
 *
 *  The function is approximated by a per-segment polynomial of degree
 *  four, computed by the Remez' function approximation algorithm. There
 *  are 16 segments, requiring a lookup table of 320 bytes for the
 *  coefficients. The result is an unsigned Q1.31 fixed-point number.
 */
static uint32_t egui_fx_core_sin(uint32_t fpart32)
{
    /* The 320 bytes large lookup table of polynomial coefficients */
    static const uint32_t table[16][5] =
            /*   c0 2**32      c1 2**35  (s)c2 2**38    -c3 2**44     c4 2**49 */
            {{0000000001UL, 0xc90fd9a3UL, 0x00003fb0L, 0xa58a7e76UL, 0x065f386bUL},  {0x1917a6bdUL, 0xc817ffc8UL, -0x07bcf80fL, 0xa4be44bcUL, 0x130df3a4UL},
             {0x31f17079UL, 0xc532d541UL, -0x0f671be3L, 0xa25be1b9UL, 0x1f8db4a9UL}, {0x4a5018bcUL, 0xc0677d57UL, -0x16eb464bL, 0x9e693649UL, 0x2bbfaac2UL},
             {0x61f78a9bUL, 0xb9c1c9f7UL, -0x1e36ef68L, 0x98effe28UL, 0x3785c4f8UL}, {0x78ad74e1UL, 0xb1521e87UL, -0x25381aa7L, 0x91fdb7ebUL, 0x42c2fc45UL},
             {0x8e39d9ceUL, 0xa72d4786UL, -0x2bdd8320L, 0x89a383c5UL, 0x4d5b9b12UL}, {0xa2679929UL, 0x9b6c4741UL, -0x3216c622L, 0x7ff5f946UL, 0x57358192UL},
             {0xb504f334UL, 0x8e2c182bUL, -0x37d48b9eL, 0x750cf49dUL, 0x60386657UL}, {0xc5e40359UL, 0x7f8d656dUL, -0x3d08abfaL, 0x69035bc2UL, 0x684e11ddUL},
             {0xd4db3149UL, 0x6fb43a5aUL, -0x41a652f5L, 0x5bf6dc22UL, 0x6f629596UL}, {0xe1c5978cUL, 0x5ec7a990UL, -0x45a21f4aL, 0x4e07a17aUL, 0x75647cecUL},
             {0xec835e7aUL, 0x4cf16ca9UL, -0x48f23ebfL, 0x3f580680UL, 0x7a44f841UL}, {0xf4fa0ab6UL, 0x3a5d7d55UL, -0x4b8e8659L, 0x300c403cUL, 0x7df801d6UL},
             {0xfb14be80UL, 0x2739a8f5UL, -0x4d708680L, 0x204a04aeUL, 0x80747a4cUL}, {0xfec46d1fUL, 0x13b51fadUL, -0x4e939ae0L, 0x10382e0bUL, 0x81b441c8UL}};

    uint32_t c0, c1, c2, c3, c4; /* Polynomial coefficients   Q.31     */
    uint32_t x1, x2, x3, x4;     /* Fractional powers         Q.32     */
    int64_t acc;                 /* Result accumulation word  Q32.31   */
    int ix;                      /* Lookup table index        integral */

    /* Compute the segment index from the four most significant bits */
    ix = fpart32 >> 28;

    /* Fetch the five polynomial coefficients for this segment */
    c0 = table[ix][0];
    c1 = table[ix][1];
    c2 = table[ix][2];
    c3 = table[ix][3];
    c4 = table[ix][4];

    /* Initialize segment fractional x1 and accumulator acc */
    x1 = fpart32 << 4;
    acc = 0;

    /* Two independent multiplies */
    x2 = (uint32_t)EGUI_FX_UMUL(x1, x1, 32);
    acc += EGUI_FX_UMUL(c1, x1, 35 - 2);

    /* Three independent multiplies */
    x3 = (uint32_t)EGUI_FX_UMUL(x2, x1, 32);
    x4 = (uint32_t)EGUI_FX_UMUL(x2, x2, 32);
    acc += EGUI_FX_UMUL((int32_t)c2, x2, 38 - 2);

    /* Two independent multiplies */
    acc -= EGUI_FX_UMUL(c3, x3, 44 - 2);
    acc += EGUI_FX_UMUL(c4, x4, 49 - 2);

    /* Add constant term */
    acc += (int64_t)c0 << 2;

    return (uint32_t)((acc + 4) >> 3);
}

/**
 *  Sine with phase shift.
 *  Computes sin(uval + phase*pi/2).
 */
static egui_fixed_t egui_fx_sin_phase(uint32_t uval, unsigned frac, int phase)
{
    uint32_t fpart;     /* uval / 2pi fractional       Q.32     */
    uint32_t value = 0; /* abs(sin(uval + phase*pi/2)) Q1.31    */
    int segm;           /* Segment number (0-3)        integral */

    /* Handle illegal values */
    if (frac > 31)
    {
        // errno = EDOM;
        return 0;
    }

    /* Multiply with 1 / 2pi constant to map the period [0, 2pi] to [0, 1] */
    fpart = (uint32_t)EGUI_FX_UMUL(uval, 0xa2f9836eUL, frac + 2);

    /* Compute the segment number */
    segm = ((fpart >> 30) + phase) & 3;

    /* Compute the absolute sin value */
    switch (segm)
    {
    case 0:
    case 2:
        /*  0 <= x <  pi/2 ==> sin(x) =  core(4 * x/2pi) or
         * pi <= x < 3pi/2 ==> sin(x) = -core(4 * x/2pi)
         */
        value = egui_fx_core_sin(fpart << 2);
        break;

    case 1:
    case 3:
        /*  pi/2 <= x <  pi ==> sin(x) =  core(1 - 4 * x/2pi) or
         * 3pi/2 <= x < 2pi ==> sin(x) = -core(1 - 4 * x/2pi)
         */
        value = egui_fx_core_sin(UINT32_MAX - (fpart << 2));
        break;

    default:
        EGUI_ASSERT(0);
    }

    /* Convert to Q.frac format */
    value >>= (31 - frac);

    /* Add sign */
    return segm > 1 ? -(egui_fixed_t)value : (egui_fixed_t)value;
}

/**
 *  Fixed-point sine.
 */
egui_fixed_t egui_fx_sinx(egui_fixed_t xval, unsigned frac)
{
    /* sin(x) = sign(x)*sin(abs(x)) */
    egui_fixed_t value = egui_fx_sin_phase(labs(xval), frac, 0);
    return xval < 0 ? -value : value;
}

/**
 *  Fixed-point cosine.
 */
egui_fixed_t egui_fx_cosx(egui_fixed_t xval, unsigned frac)
{
    /* cos(x) = sin(x + pi/2) */
    return egui_fx_sin_phase(labs(xval), frac, 1);
}

/**
 *  Fixed-point power function.
 */
egui_fixed_t egui_fx_powx(egui_fixed_t xval, unsigned xfrac, egui_fixed_t yval, unsigned yfrac)
{
    int32_t logx;   /* log2(xval)         Q1.30    */
    int64_t ylogx;  /* yval*log2(xval)    Q33.30   */
    int64_t ipart;  /* floor(ylogx)       integral */
    uint32_t fpart; /* frac(ylogx)        Q.32     */

    uint32_t mant;  /* xval mantissa      Q1.31    */
    int expn;       /* xval exponent      integral */
    uint32_t value; /* exp2(fpart)        Q1.31    */
    int64_t shift;  /* Conversion shift   integral */
    int sh;         /* Normalizing shift  integral */

    /* Handle illegal values */
    if (xfrac > 31 || yfrac > 31 || /* Out-of-range           */
        xval < 0 ||                 /* Negative base          */
        (xval == 0 && yval < 0))    /* Negative power of zero */
    {
        // errno = EDOM;
        return 0;
    }

    if (xval == 0)
    {
        return 0; /* FX_NORMALIZE is undefined for 0 (or rather, clz is) */
    }

    /* Convert fixed-point base and exponent to floating-point */
    EGUI_FX_NORMALIZE(mant, expn, xval, xfrac);

    /* Compute the logarithm of xm = 1.f */
    mant = egui_fx_core_log2(mant << 1);

    /* Compute the new normalizing shift */
    // sh = EGUI_FX_CLZ(labs(expn) | 1) - 1;
    sh = EGUI_FX_CLZ(labs(expn) | 1) - 1;

    /* Compute the logarithm of xval in Q1.30 format */
    logx = (expn << sh) + (((mant >> (30 - sh)) + 1) >> 1);

    /* Perform fixed-point multiply with the exponent yval */
    ylogx = EGUI_FX_SMUL(logx, yval, yfrac);

    /* Decompose the new exponent into integral and fractional parts */
    ipart = ylogx >> sh;
    fpart = (uint32_t)(ylogx << (32 - sh));

    /* Compute the base-2 exponential of the fractional part */
    value = egui_fx_core_exp2(fpart);

    /* Compute the fixed-point conversion shift */
    shift = ipart + xfrac - 31;

    /* Convert to output fixed-point format */
    return EGUI_FX_SHIFT(value, shift);
}
#endif /* EGUI_CONFIG_PERFORMANCE_USE_FLOAT */
