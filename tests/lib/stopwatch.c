#include <stdio.h>
#include <sys/time.h>

static struct timeval begin;
static struct timeval end;

void stopwatch_start(void)
{
    gettimeofday(&begin, NULL);
}

double stopwatch_stop(void)
{
    gettimeofday(&end, NULL);
    double secs_a = (double)begin.tv_sec + ((double)begin.tv_usec / 1000000.);
    double secs_b = (double)end.tv_sec + ((double)end.tv_usec / 1000000.);
    return (secs_b - secs_a);
}
