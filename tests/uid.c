#ifdef __linux__
#define  _POSIX_C_SOURCE 2  //for popen(3)
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "../src/atomics.h"
#include "../src/util.h"
#include "../src/error.h"
#include "lib/stopwatch.c"
#include <pthread.h>
#ifdef _WIN32
#include <windows.h>
#define WIN32_LEAN_AND_MEAN
#endif
#define MAX_THREADS 1024


#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

int nproc(void)
{

    #ifdef __linux__
    char buf[1024];
    ssize_t nread;
    FILE * fd = popen("nproc", "r");

    nread = read(fileno(fd), buf, 1024);
    pclose(fd);
    if( nread < 1){
        return nread;
    }
    return atoi(buf);
    #elif _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (int)info.dwNumberOfProcessors;
    #else
    #error
    #endif
}

void * bmo_uid_ntimes(void * arg)
{
    int64_t iter = *(int64_t *)arg;
    for(int64_t i = 0; i < iter; i++){
        bmo_uid();
    }
    return NULL;
}

void spawn_test_threads(int count, void * arg)
{
    pthread_t workers[MAX_THREADS];
    for(int t = 0; t < count; t++)
        pthread_create(&workers[t], NULL, bmo_uid_ntimes, arg);
    for(int t = 0; t < count; t++)
        pthread_join(workers[t], NULL);
}

int main(void)
{
    /*This tests that bmo_uid properly provides unique values across multiple
    threads.
    We can't force a race condition but we can try to run as many hardware
    threads as available over billions of calls which would _likely_ fail
    if bmo_uid() was racy.
    If only one hardware thread is available, just warn, as a single hardware
    thread will never cause a race.
    */
    int nthreads = nproc();
    if(getenv("TRAVIS") || getenv("JENKINS_HOME")){
        nthreads = MIN(4, nthreads); //CI builds time out because there are more physical processor in the host than the guest
    }
    uint64_t iter = 1000000;
    int i = 0;
    if(nthreads == 1){
        fprintf(stderr, "only one hardware thread available. Bugs will be masked\n");
    }
    assert(nthreads < MAX_THREADS);
    printf("running bmo_uid() over %d threads\n", nthreads);
    spawn_test_threads(nthreads, &iter);

    assert(bmo_uid() == (iter * nthreads));
    assert(BMO_ATOM_INC(&i) - BMO_ATOM_INC(&i) == -1);

    return 0;
}
