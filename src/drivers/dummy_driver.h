#include "../definitions.h"
#ifndef BMO_DUMMY_DRIVER_H
#define BMO_DUMMY_DRIVER_H

#ifdef __linux__
#include <pthread.h>
#include <sys/epoll.h>

struct BMO_dummy_ipc_t_ {
    int epfd;
    int fd;
    struct epoll_event event;
};

typedef struct {
    FILE *file;
    const char *file_path;
    uint64_t nano_secs_per_buffer;
    uint64_t nano_secs_last_callback;
    uint64_t nano_secs_sleep_callback;
    uint32_t error_num;
    uint32_t dither_type;
    uint32_t stream_format;
    float **out_buffers;
    int (*process_callback)(
        void *arg, uint32_t frames); // the dummy process callback address.
    int (*xrun_callback)(void *arg, double secs_late);
    int (*error_callback)(void *arg, uint32_t err);
    int (*finished_callback)(void *arg, uint32_t n);
    int is_realtime;
    int is_running;
    pthread_t disk_io_thread;
    pthread_t supervisor_thread;
    struct BMO_dummy_ipc_t_ ipc;
} BMO_dummy_state_t;
#endif

BMO_state_t *bmo_dummy_start(BMO_state_t *params, uint32_t channels,
                             uint32_t rate, uint32_t buf_len, uint32_t flags,
                             const char *path);
#endif
