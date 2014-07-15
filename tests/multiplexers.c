#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../src/error.h"
#include "../src/multiplexers.h"
#include "../src/definitions.h"

size_t arr_int8(int8_t ** arr){

    int i = INT8_MAX - INT8_MIN + 1;
    *arr = malloc(sizeof(int8_t) * i);
    while(i--){
        (*arr)[i] =  i - INT8_MIN;
    }
    return INT8_MAX - INT8_MIN + 1;
}

size_t arr_int16(int16_t ** arr){

    int i = INT16_MAX - INT16_MIN + 1;
    *arr = malloc(sizeof(int16_t) * i);
    while(i--){
        (*arr)[i] =  i - INT16_MIN;
    }
    return INT16_MAX - INT16_MIN + 1;
}


/*void float_arrays(size_t len, size_t n_arr, ...)*/
/*{*/
/*    va_list ap;*/
/*    float * f;*/
/*    va_start(ap, n_arr);*/
/*    for(size_t i = 0; i < n_arr; i++){*/
/*        f = *va_arg(ap, float **);*/
/*        f = malloc(sizeof(float) * len);*/
/*    }*/
/*    va_end(ap);*/
/*}*/

int test_swap()
{
    int failed = 0;
    uint16_t s16 = 0xaabb;
    uint32_t s32 = 0xaabbccdd;
    uint64_t s64 = 0xaabbccddeeff0011;
    
    _bmo_swap_16(&s16);
    _bmo_swap_32(&s32);
    _bmo_swap_64(&s64);
    bmo_debug("0xaabb swapped is 0x%x", s16);
    bmo_debug("0xaabbccdd swapped is 0x%x", s32);
    bmo_debug("0xaabbccddeeff0011 swapped is 0x%x", s64);
    if(s16 != 0xbbaa){
        bmo_err("0x%x != 0xbbaa\n", s16);
        failed++;
    }
    if(s32 != 0xddccbbaa){
        bmo_err("0x%x != 0xddccbbaa\n", s32);
        failed++;
    }
    if(s64 != 0x1100ffeeddccbbaa){
        bmo_err("0x%x != 0x1100ffeeddccbbaa\n", s64);
        failed++;
    }
    return failed;
}

int main(void)
{
    bmo_verbosity(BMO_MESSAGE_INFO);
    if(getenv("BMO_DEBUG")){
        bmo_verbosity(BMO_MESSAGE_DEBUG);
    }
    float to_f;
/*    float from_f;*/
    int failed = 0;
    size_t len;
    
    //8bit tests
    int8_t * arr_8; 
    int8_t res_8;
    len = arr_int8(&arr_8);
    
    for(size_t i = 0; i < len; i++){
        bmo_conv_ipcmtoif(&to_f, arr_8 + i, BMO_FMT_NATIVE_FLOAT, BMO_FMT_PCM_8, 1);
        bmo_conv_iftoipcm(&res_8, &to_f, BMO_FMT_PCM_8, BMO_FMT_NATIVE_FLOAT, 1);
        if(arr_8[i] != res_8){
            bmo_err("%d != %d != (%E * %f)\n", arr_8[i], res_8, to_f, (float)len);
            failed++;
        }else{
            bmo_debug("%d == %d == (%E * %f)\n", arr_8[i], res_8, to_f, (float)len);
        }
    }
    int16_t res_16;
    int16_t * arr_16;
    len = arr_int16(&arr_16);
    for(size_t i = 0; i < len; i++){
        bmo_conv_ipcmtoif(&to_f, arr_16 + i, BMO_FMT_NATIVE_FLOAT, BMO_FMT_PCM_16_LE, 1);
        bmo_conv_iftoipcm(&res_16, &to_f, BMO_FMT_PCM_16_LE, BMO_FMT_NATIVE_FLOAT, 1);
        if(arr_16[i] != res_16){
            bmo_err("%d != %d != (%E * %f)\n", arr_16[i], res_16, to_f, (float)len);
            failed++;
        }else{
            bmo_debug("%d == %d == (%E * %f)\n", arr_16[i], res_16, to_f, (float)len);
        }
     }   
    
    free(arr_8);
    free(arr_16);
    
    failed += test_swap();
    
    return failed;
}
