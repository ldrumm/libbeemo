#ifndef BMO_SNDFILE_H
#define BMO_SNDFILE_H
#ifdef BMO_HAVE_SNDFILE
BMO_buffer_obj_t * _bmo_fopen_sndfile(const char * path, uint32_t flags);
#endif
#endif
