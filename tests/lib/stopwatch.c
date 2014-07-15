#include <sys/time.h>
#include <stdio.h>

struct timeval begin;
struct timeval end;
double secsa;
double secsb;

void stopwatch_start(void)
{
	gettimeofday(&begin, NULL);
}

double stopwatch_stop(void)
{
	gettimeofday(&end, NULL);
	secsa = (double)begin.tv_sec + ((double)begin.tv_usec / 1000000.0f);
	secsb = (double)end.tv_sec + ((double)end.tv_usec / 1000000.0f);
	return (secsb - secsa);
}
