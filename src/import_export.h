#include "definitions.h"
#ifndef BMO_IMPORT_EXPORT_H
#define BMO_IMPORT_EXPORT_H
BMO_buffer_obj_t * bmo_fopen(const char * path, uint32_t flags);
size_t bmo_fwrite_mb(FILE * file, float ** buf, uint32_t channels, uint32_t out_fmt, uint32_t frames, uint32_t dither);
size_t bmo_fwrite_ib(FILE * file, void * buf, uint32_t channels, uint32_t out_fmt, uint32_t in_fmt, uint32_t frames, uint32_t dither);
BMO_dsp_obj_t * bmo_dsp_bo_new_fopen(const char * path, uint32_t flags, uint32_t frames);
BMO_dsp_obj_t * bmo_dsp_bo_new_fwrite(const char *path, uint32_t flags, uint32_t channels, uint32_t frames, uint32_t rate);

#endif
