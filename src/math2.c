/* MATH2 - Elementary Math Functions */
/* Based on MATH2.ASM */

#include "gwbasic.h"
#include <math.h>
#include <stdlib.h>

/* Module initialization */
void math2_init(void) {
    /* Nothing special needed */
}

/* SQR(x) - Square root */
float math2_sqr(float x) {
    if (x < 0.0f) {
        /* GW-BASIC returns 0 for negative square root */
        return 0.0f;
    }
    return sqrtf(x);
}

/* ABS(x) - Absolute value */
float math2_abs(float x) {
    return fabsf(x);
}

/* SGN(x) - Sign function: -1, 0, or 1 */
float math2_sgn(float x) {
    if (x > 0.0f) return 1.0f;
    if (x < 0.0f) return -1.0f;
    return 0.0f;
}

/* RND(x) - Random number generator
 * GW-BASIC behavior:
 * - RND with positive x: returns next random number in sequence
 * - RND with negative x: reseeds generator with x, returns first number
 * - RND with zero: returns last random number generated
 * - RND without argument: same as positive x
 */
static float last_rnd = 0.0f;

float math2_rnd(float x) {
    if (x < 0.0f) {
        /* Reseed with absolute value of x */
        srand((unsigned int)fabsf(x));
    } else if (x == 0.0f) {
        /* Return last generated value */
        return last_rnd;
    }
    
    /* Generate random float in range [0, 1) */
    float r = (float)rand() / (float)RAND_MAX;
    last_rnd = r;
    return r;
}

/* INT(x) - Floor function (largest integer <= x) */
float math2_int(float x) {
    return floorf(x);
}

/* FIX(x) - Truncate towards zero */
float math2_fix(float x) {
    if (x >= 0.0f) {
        return floorf(x);
    } else {
        return ceilf(x);
    }
}