#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <string.h>

#include "../src/definitions.h"
#include "../src/multiplexers.h"

#include "lib/test_common.c"
extern float _bmo_fmt_pcm_range(uint32_t);

void *range(size_t *count, size_t *bytes, uint32_t fmt)
{
    /** Like a bastardised version of Python's `range()` function,
    generate an array containing every value in the given format dynamic range.
    For simplicity we treat the array as unsigned and let twos-complement
    do its thing...
    */
    *count = (size_t)_bmo_fmt_pcm_range(fmt);
    *bytes = bmo_fmt_stride(fmt) * (*count);
    void *samples = calloc(*bytes + 1, 1);  //Add an extra byte to avoid illegal access with 24bit.
    assert(samples);
    size_t i = *count;
    #define LOOP() while(i--){array[i] = i;}
    switch(bmo_fmt_stride(fmt)){
        case 1:{
            ;
            uint8_t *array = samples;
            LOOP();
            break;
        }
        case 2:{
            ;
            uint16_t *array = samples;
            LOOP();
            break;
        }
        case 3:{
            ;
            //packed 24bit is always a pain...
            uint8_t *array = samples;
            for(size_t j = 0; j < (i * 3); j += 3){
                uint32_t *p = (uint32_t*) & array[j];
                #if BMO_ENDIAN_LITTLE
                *p = (0x00ffffff & i) | (*p & 0xff000000);
                #else
                *p = (0xffffff00 & i) | (*p & 0x000000ff);
                #endif
                i--;
            }
            break;
        }
        case 4:{
            ;
            uint32_t *array = samples;
            LOOP();
            break;
        }
        default:assert(0);
    }

    return samples;
}

struct formats {
    uint32_t fmt;
    int depth;
};

int main(void)
{
    /** Converting from PCM formats to float can be done in many ways, but not
    all methods guarantee that the conversion is transparent when reversed.
    This tries to test that expectation for PCM formats <= 24 bits.
    */

    bmo_test_setup();
    struct formats binfmts[] = {
        {BMO_FMT_PCM_8, 8},
        {BMO_FMT_PCM_U8, 8},
        {BMO_FMT_PCM_16_LE, 16},
        {BMO_FMT_PCM_16_BE, 16},
        {BMO_FMT_PCM_24_LE, 24},
        {BMO_FMT_PCM_24_BE, 24},
        {0, 0}
    };

    size_t count, bytes;

    void *src, *dest;
    float *intermediate;

    for(size_t i = 0; binfmts[i].depth; i++){
        src = range(&count, &bytes, binfmts[i].fmt);
        dest = malloc(bytes);
        assert(dest);
        intermediate = malloc(sizeof(float) * count);
        assert(intermediate);
        bmo_conv_ipcmtoif(intermediate, src, BMO_FMT_NATIVE_FLOAT, binfmts[i].fmt, count);
        bmo_conv_iftoipcm(dest, intermediate, binfmts[i].fmt, BMO_FMT_NATIVE_FLOAT, count);
        assert(memcmp(dest, src, bytes) == 0);
        free(dest);
        free(src);
        free(intermediate);
    }
    return 0;
}
