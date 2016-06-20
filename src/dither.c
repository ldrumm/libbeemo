#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dither.h"
#include "definitions.h"
#include "error.h"
#include "multiplexers.h"
#define BMO_rand32 rand // TODO fast portable LCG


int8_t bmo_rand_tpdf8(void)
{
    /**
    Triangular Probability Density is the same as the roll of two dice.
    the static variables allow one random call per invocation halving rand()
    calls
    */
    static int8_t state;
    static int8_t prev_state;
    prev_state = state;
    state = (BMO_rand32() % 2) - 1;

    return state - prev_state;
}

void bmo_dither_tpdf(void *stream, uint32_t fmt, size_t samples)
{
    bmo_debug("additive dither\n");
    /**
    dithers a PCM stream of arbitrary resolution by setting the least
    significant bit with the output
    of a TPDF random number generator.

    Here the focus is on speed.  Some Dither functions ADD the output of a urand
    function but involves the possibility of integer overflow
    which must be caught and reversed with a 'test-and-branch' in the inner
    loop.
    The drawback with the method used here is an average DC offset of 0.5LSbit.
    */
    int16_t *in16 = (int16_t *)stream;
    int8_t *in8 = (int8_t *)stream;
    uint32_t stride = bmo_fmt_stride(fmt);
    switch (stride) {
        case 1: {
            for (uint32_t i = 0; i < samples; i++) {
                *in8 += bmo_rand_tpdf8();
                in8++;
            }
            break;
        }
        case 2: {
            for (uint32_t i = 0; i < samples; i++) {
                *in16 += bmo_rand_tpdf8();
                in16++;
            }
            break;
        }
        default: bmo_err("cannot dither %dbit streams\n", stride * 8);
    }
}

