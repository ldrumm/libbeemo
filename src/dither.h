#ifndef BMO_DITHER_H
#define BMO_DITHER_H
int8_t bmo_rand_tpdf8(void);
void bmo_dither_tpdf(void * stream, uint32_t fmt, size_t samples);
#endif

