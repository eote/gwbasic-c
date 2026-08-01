/* MATH1 - Transcendental Math Functions */
/* Based on MATH1.ASM */

#include "gwbasic.h"
#include <math.h>
#include <stdlib.h>

/* Module initialization */
void math1_init(void) {
    /* Seed the random number generator */
    srand(12345);  /* Fixed seed for reproducibility, will be overridden by RND */
}

/* SIN(x) - Sine function */
float math1_sin(float x) {
    return sinf(x);
}

/* COS(x) - Cosine function */
float math1_cos(float x) {
    return cosf(x);
}

/* TAN(x) - Tangent function */
float math1_tan(float x) {
    return tanf(x);
}

/* ATN(x) - Arctangent function */
float math1_atn(float x) {
    return atanf(x);
}

/* EXP(x) - Exponential function e^x */
float math1_exp(float x) {
    return expf(x);
}

/* LOG(x) - Natural logarithm */
float math1_log(float x) {
    if (x <= 0.0f) {
        /* GW-BASIC returns 0 or error for LOG(0) or negative */
        return 0.0f;
    }
    return logf(x);
}