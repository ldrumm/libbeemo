#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

struct timespec start;
struct timespec stop;

int pipefd[2];


static int res(void){
    /** Get the realtime clock resolution in nanosecs */
    /*this is not thread-safe, but in practice shouldn't matter because 
    the clock's resolution should not change per process [citation needed].
    */
    static struct timespec _res = {.tv_nsec = 0, .tv_sec = 0};
    if(!_res.tv_nsec)
        clock_getres(CLOCK_MONOTONIC, &_res);
    return _res.tv_nsec;
}

static void print_time(void)
{
    double _start;
    double _stop;
    _start = (double)start.tv_sec + start.tv_nsec / (1000000000./res());
     _stop = (double)stop.tv_sec + stop.tv_nsec / (1000000000./res());
    printf("%fs\n", _stop - _start);
}

void * write_thread(void * arg)
{
    int n = 1;
    //write a single int into the pipe and start the timer
    sleep(1);
    clock_gettime(CLOCK_MONOTONIC, &start);
    write(pipefd[1], &n, sizeof(n));
    return NULL;
}

int main(void)
{
    int read_res;
    pthread_t writer;
    pipe(pipefd);
    fcntl(pipefd[1], F_SETFD, O_ASYNC);
    pthread_create(&writer, NULL, write_thread, NULL);
    
    read(pipefd[0], &read_res, sizeof(read_res));
    clock_gettime(CLOCK_MONOTONIC, &stop);
    print_time();
    return 0;
}
