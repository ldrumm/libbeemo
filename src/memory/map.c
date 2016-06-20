/* functions for mapping files into program address space */
#define _GNU_SOURCE
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "map.h"
#include "../definitions.h"
#include "../error.h"

size_t bmo_fsize(const char *path, int *err)
{
    if (!path) {
        *err = 1;
        return 0;
    }
    // #ifdef __linux__
    struct stat buf;
    if (stat(path, &buf) == -1) {
        *err = 1;
        return 0;
    }
    // #elif _WIN32
    // struct __stat64 buf;
    // if (_stat64(path, &buf) == -1){
    // *err = 1;
    // return 0;
    // }
    // #endif
    *err = 0;
    return buf.st_size;
}


void *bmo_map(const char *path, uint32_t flags, size_t offset)
{
#ifdef HAVE_MMAP
    size_t size;
    int err;
    int fd;
    void *map;
    if (!flags)
        flags = MAP_PRIVATE;
    int prot = PROT_READ;

    if (flags & BMO_MAP_READWRITE) {
        prot = PROT_READ | PROT_WRITE;
        flags = (MAP_SHARED);
    }
    size = bmo_fsize(path, &err);
    if (err) {
        bmo_err("could not determine size of %s\n", path);
        return NULL;
    }

    fd = open(path, O_RDWR);
    if (fd == -1) {
        bmo_err("%s\n", strerror(errno));
        return NULL;
    }

    map = mmap(NULL, size, prot, flags, fd, offset);
    if (map == MAP_FAILED) {
        bmo_err("could not map: '%s': %s", path, strerror(errno));
        return NULL;
    }
    return map;
#else
    size_t size;
    int err;
    HANDLE fd;
    HANDLE mmaph;
    void *data = NULL;
    int prot = PAGE_READONLY;

    if (flags & BMO_MAP_READWRITE) {
        prot = PAGE_READWRITE;
        flags = 0;
    }

    size = bmo_fsize(path, &err);
    if (err) {
        bmo_err("couldn't get filesize%d\n", size);
        return NULL;
    }

    fd = CreateFile(path, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, 0);
    bmo_err("last error was %s\n", strerror(GetLastError()));
    if (fd == INVALID_HANDLE_VALUE) {
        bmo_err("CreateFile() failed\n");
        return NULL;
    }

    bmo_info("size of %p is %zu\n", fd, size);
    mmaph =
        CreateFileMapping(fd, NULL, prot, offset, size - (size - offset), "");
    if (!mmaph || mmaph == INVALID_HANDLE_VALUE) {
        bmo_err("CreatefileMapping failed>%d\n", strerror(GetLastError()));
        return NULL;
    }
    CloseHandle(fd);
    data = MapViewOfFile(mmaph, FILE_MAP_READ, 0, offset, size);
    bmo_info("%s mapped at %p\n", path, data);
    return data;
#endif
}


void bmo_unmap(void *mapping, size_t length)
{
#ifdef __linux__
    munmap(mapping, length);
#elif _WIN32
    (void)length;
    UnmapViewOfFile(mapping);
#endif
}


void *bmo_loadlib(const char *path, char *sym_name)
{
#ifdef HAVE_DLOPEN
    void *obj_handle = NULL;
    void *func_handle = NULL;

    if (!path || !sym_name)
        return NULL;

    obj_handle = dlopen(path, RTLD_LAZY);
    if (obj_handle)
        func_handle = dlsym(obj_handle, sym_name);

    return func_handle;
#elif _WIN32
    HMODULE obj_handle = NULL;
    FARPROC func_handle = NULL;
    obj_handle = LoadLibrary(path);
    func_handle = GetProcAddress(obj_handle, sym_name);

    return func_handle;
#else
#error UNSUPPORTED PLATFORM
#endif
}
