/*aud_map.h*/
#ifndef BMO_MAP
#define BMO_MAP
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sys/stat.h>
#endif
#endif

#ifdef __linux__
#ifndef HAVE_MMAP
#define HAVE_MMAP
#endif
#ifndef HAVE_DLOPEN
#define HAVE_DLOPEN
#endif
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <fcntl.h>
#endif

#ifndef MAPPING_FLAGS
#define MAPPING_FLAGS
#define BMO_MAP_READONLY 1
#define BMO_MAP_WRITEONLY 2
#define BMO_MAP_READWRITE 4
#define BMO_MAP_EXECUTABLE 8
#define BMO_MAP
#endif
size_t bmo_fsize(const char * path, int * err);
void * bmo_map(const char * path, uint32_t flags, size_t offset);
void bmo_unmap(void * mapping, size_t length);
void * bmo_loadlib(const char * path, char * func_name);
#endif
