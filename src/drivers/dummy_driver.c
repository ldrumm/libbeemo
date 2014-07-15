/*BMO_dummy_driver.c*/
/** @module
*   This driver backend implements a dummy callback interface that allows a live performance stream to be written to disk.
*	requests to begin audio processsing using this dummy interface should not fail unless their is insufficient disk space available.
*/

#ifdef __linux__
#undef _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h> 
#include <pthread.h>

#include "../definitions.h"
#include "../multiplexers.h"
#include "../buffer.h"
#include "../memory/map.h"
#include "../import_export.h"
#include "../error.h"
#include "../atomics.h"
#include "driver_utils.h"
#include "ringbuffer.h"
#if 0
#define DUMMY_RT_SIG SIGRTMIN+1
#define DUMMY_DISK_BUFFERS 4

enum {
	BMO_DUMMY_RUN,
	BMO_DUMMY_FINISH,
	BMO_DUMMY_ERROR,
	BMO_DUMMY_PAUSE,
};

struct BMO_dummy_timer_t_
{
	struct sigaction action;
	struct sigevent event;
	struct itimerspec interval;
	timer_t timerid;
};

struct BMO_dummy_ipc_t_{
	int epfd;
	int fd;
	struct epoll_event* event;
	pthread_t * thread;
};

typedef struct BMO_dummy_buffer_t_ {
	char * buf;
	size_t len;
	int written;	///<to disk
	int tick;		///<the dsp tick this refers to
	struct BMO_dummy_buffer_t_ * next;
}BMO_dummy_buffer_t_;


static int n_timers_fired = 0;
static int pipefd[2] = {0};
static BMO_dummy_buffer_t_ disk_buffers[DUMMY_DISK_BUFFERS];

/**
Finds the next buffer that has been confirmed as written to disk.
*/
static BMO_dummy_buffer_t_ * 
_next_buf_to_fill(const BMO_dummy_buffer_t_ * head)
{
	bmo_debug("\n");
	assert(head);
	while(head->next){
		if(head->written)
			return (BMO_dummy_buffer_t_ *) head;
		head=head->next;
	}
	return NULL;
}

static BMO_dummy_buffer_t_ * 
_next_buf_to_disk(BMO_dummy_buffer_t_ * head)
{

//	BMO_dummy_buffer_t_ * head_top = head;
	BMO_dummy_buffer_t_ * ret = NULL;
	assert(head);
	int earliest_tick = head->tick;
	while(head){
		if(!head->written){
			if(head->tick < earliest_tick){
				earliest_tick = head->tick;
				ret = head;
			}
		}
		head = head->next;
	}
	return ret;
}


/**
entry point into the disk writer thread.  
This thread wait for available IO at the read-end of a pipe, and then checks for data it can write to disk.  
If a full buffer is available, it does a blocking write to disk.  
On delivery of the BMO_DUMMY_FINISH flag, 
*/

static void *
_bmo_dummy_fwrite(void * arg)
{
	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, DUMMY_RT_SIG);
	pthread_sigmask(SIG_BLOCK, &sigmask, NULL);
	bmo_debug("\n");
	BMO_state_t *state =(BMO_state_t *)arg;
	BMO_dummy_ipc_t_ * data = &state->driver.dummy.ipc;
	bmo_debug("data->epfd:%d\n", data->epfd);
//	int res;
	char errbuf[BMO_DEFAULT_STR_BUF] = {'\0'};
	int command;
	size_t written;
    size_t read_n;
	BMO_dummy_buffer_t_ * todisk;
	for(;;)
	{	
//		int last_tick = 0;
//		sleep(3);
//		bmo_debug("waiting for file %d with epollfd %d\n", data->fd, data->epfd);
		if(epoll_wait(data->epfd, &(data->event), 1, -1) < 1){
			(void)strerror_r(errno, errbuf, BMO_DEFAULT_STR_BUF);
			bmo_err("epoll wait returned error:%s\n", errbuf); 
		}
//		bmo_debug("wait finished\n");
		read_n = read(data->fd, &command, sizeof(command));
		if(read_n != sizeof(command)){
		    bmo_err("couldn't read from %d\n", data->fd);
		}
		if(command != BMO_DUMMY_RUN){
			bmo_debug("command is %d\n", command);
			break;
		}
		while((todisk = _next_buf_to_disk(disk_buffers))!=NULL){
//			if todisk->tick < last
			written = fwrite(todisk->buf,
				3,
//				bmo_fmt_stride(state->driver.dummy.stream_format),
				todisk->len,
				state->driver.dummy.file
			);
			if(written != todisk->len){
				if(feof(state->driver.dummy.file)){
					bmo_err("unexpected EOF for %s\n", state->driver.dummy.file_path);
					return NULL;
				}
				else if(ferror(state->driver.dummy.file)){
					(void) strerror_r(errno, errbuf, BMO_DEFAULT_STR_BUF);
					bmo_err("unexpected error for %s:\n%s\n", state->driver.dummy.file_path, errbuf);;
					return NULL;
				}
				todisk->written = 1;
			}
		}
	}
	if(command == BMO_DUMMY_FINISH){
		while((todisk = _next_buf_to_disk(disk_buffers))!=NULL){
			fwrite(todisk->buf,
				bmo_fmt_stride(state->driver.dummy.stream_format),
				todisk->len,
				state->driver.dummy.file);
		}
		fflush(state->driver.dummy.file);
	}
	else{
		bmo_err("unknown command received in disk IO thread with value %d\n", command);
	}
	return NULL;
}

/**
start the timer created with timer_factory()
*/
static int 
start_tt(struct BMO_dummy_timer_t_ * my_timer)
{
	bmo_debug("\n");    
    return timer_settime(my_timer->timerid, 0, &my_timer->interval, NULL);
}

/**
* disarm and delete a timer created with timer_factory()
*/
static int 
stop_tt(struct BMO_dummy_timer_t_ * my_timer)
{
	bmo_debug("\n");
    return timer_delete(my_timer->timerid);
}

/**
* create a new timer based on a monotonic clock, and emitting a realtime signal
*/
static struct BMO_dummy_timer_t_ * 
timer_factory(int signo, double delay_s, double every_s, void (*fn)(int, siginfo_t * info, void * data))
{
	bmo_debug("\n");

	struct BMO_dummy_timer_t_ * my_timer = malloc(sizeof(struct BMO_dummy_timer_t_));
	if(!my_timer)
		return NULL;
		
	//set the behaviour that our chosen signal will invoke note that sigaction can be used fo
	my_timer->action.sa_sigaction = fn;
	my_timer->action.sa_flags = SA_SIGINFO;
	sigemptyset(&my_timer->action.sa_mask);
	sigaddset(&my_timer->action.sa_mask, signo);
	sigaction(signo, &my_timer->action, NULL);
	
	//set event parameters
	my_timer->event.sigev_notify = SIGEV_SIGNAL;
	my_timer->event.sigev_signo = signo;
	my_timer->event.sigev_value.sival_ptr = &my_timer->timerid;		//this data is passed to our handler on the event firing
	
	//create the timer which will generate the signal given in event as per the type of the first argument
	if(timer_create(CLOCK_MONOTONIC, &my_timer->event, &my_timer->timerid) < 0){
		bmo_err("couldn't create timer\n");
		return NULL;
	}
	else
		bmo_debug("timer created at 0x%p\n", my_timer->timerid);
	
	//set the interval of the created timer
    my_timer->interval.it_interval.tv_sec = lrint(floor(every_s));
    my_timer->interval.it_interval.tv_nsec = lrint((every_s - floor(every_s)) * 1000000000);
    my_timer->interval.it_value.tv_sec = lrint(floor(delay_s));
    my_timer->interval.it_value.tv_nsec = lrint((delay_s - floor(delay_s)) * 1000000000);
	return my_timer;
}

static int 
BMO_dummyErrorCallback(void * data, uint32_t errNum)
{
	bmo_debug("\n");	
	(void)data;
	(void)errNum;
	bmo_err("an error occurred");
	return 0; //TODO
}

static int 
BMO_dummyFinishedCallback(void * data, uint32_t n)
{
	bmo_debug("\n");
	(void)data;
	(void)n;
	bmo_info("stream finished\n");
	return 0; //TODO
}

static int 
BMO_dummyXrunCallback(void * data, double secsLate)
{
	bmo_debug("\n");
	(void)data;
	(void)secsLate;
	bmo_err("a buffer overrun of %f seconds occurred\n", secsLate);
	return 0; //TODO
}

#ifdef DEF_BUT_NOT_USED
static int
BMO_streamFinishedCallback(void * data)
{
    (void)data;
	bmo_debug("\n");
    bmo_stop();
    bmo_info("Stream finished\n");
    //TODO write file header
    return 0;
}
#endif

static int 
BMO_dummyProcessCallback(void * data, uint32_t frames)
{
    bmo_debug("\n");
	assert(data && frames);
	BMO_state_t * state = (BMO_state_t *) data;
	BMO_dummy_buffer_t_ * outbuf = _next_buf_to_fill(disk_buffers);
	if(!outbuf){
		state->driver.dummy.xrun_callback(data, 0.0);
		return 0;
	}
	if(bmo_status() == BMO_DSP_STOPPED){
	    bmo_err("DSP is stopped\n");
		return 0;
	}
	else if (bmo_status() == BMO_DSP_RUNNING)
	{
		state->getter->get_samples(state->driver.dummy.out_buffers, state->getter, frames);
		bmo_conv_mftoix(outbuf,state->driver.dummy.out_buffers,
			state->n_playback_ch,
			state->driver.dummy.stream_format, 
			frames);
        bmo_debug("multiplexed\n");
        //Mark the disk buffers as ready to go on it's journey
        outbuf->written = 0;
        outbuf->tick = BMO_ATOM_INC(&state->n_ticks);
	}
	return frames;
}

static void 
_disk_io_sig_h(int sig, siginfo_t *info, void *data)
{
    (void)sig;
    (void)data;
    (void)info;
//	bmo_debug("\n");
//    write_into_pipe();
    int val = BMO_DUMMY_RUN;
//    int fd = 
//    bmo_debug("writing to fd:%d\n", epipefd[1]);
    //FIXME when to send BMO_DUMMY_FINISH / other
    if (write(pipefd[1], &val, sizeof(val)) <= 0){
    	bmo_err("can't do IPC to disk thread:\n%s\n");
    }
}

/**
 schedule the DSP callback that reads from the ringbuffer, and alert the diskbuffer thread that there is work to be done from the signal handler.
*/
static void 
BMO_dummySupervisor(int (*callback)(void *, uint32_t frames), BMO_state_t * data)
{
    (void) callback;
	bmo_debug("data == %p\n", data);
	double wait = 1.0;
	double secsPerBuffer = (1.0 / (double) data->driver_rate) * (double) data->buffer_size;
	bmo_debug("secsPerBuffer is %f\n", secsPerBuffer);
	struct BMO_dummy_timer_t_ * scheduler = timer_factory(DUMMY_RT_SIG, wait, secsPerBuffer, _disk_io_sig_h);
	bmo_debug("created timer with id %d\n", scheduler->timerid);

	int current_tick = 0;
//	int wait_return;
//	int signo;
	sigset_t set, empty, oldmask;
	sigemptyset(&set);
	sigemptyset(&empty);
	sigaddset(&set, DUMMY_RT_SIG);
	bmo_debug("starting DSP");
	bmo_start();
	start_tt(scheduler);
	while(1)
	{
//		pthread_sigmask(SIG_BLOCK, &empty, &oldmask);
        if(sigwaitinfo(&set, NULL)){
        	bmo_err("sigwait");
        }
       
        current_tick++;
        bmo_debug("current tick is %d\n", current_tick);
		if(!data->driver.dummy.is_running){
			data->driver.dummy.finished_callback(data, 0);
			break;
		}
//		if(!data->driver.dummy.is_realtime){
//			if(BMO_dummyProcessCallback(data, frames))
//				data->driver.dummy.error_callback(data, 1);
//		}
		if(!BMO_dummyProcessCallback(data, data->buffer_size)){
			data->driver.dummy.error_callback(data, 1);
			break;
		}
		if ((current_tick - n_timers_fired) < -2){
	        BMO_dummyXrunCallback(data, 0);
	        break;
        }
        pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
	}
	stop_tt(scheduler);
	//TODO join disk thread
	return;
}

void * _bmo_dummy_scheduler(void * arg)
{
	bmo_debug("\n");
    BMO_state_t * state = (BMO_state_t *)arg;
    BMO_dummySupervisor(BMO_dummyProcessCallback, state);
	return NULL;
}


BMO_state_t * 
bmo_dummy_start(const char * filePath, uint32_t channels, uint32_t rate, uint32_t bufferSize, uint32_t flags)
{
	
	/*set up the write cache*/
	uint32_t stream_format = bmo_fmt_enc(flags & ~BMO_DUMMY_DRIVER);
	bmo_debug("stream format is 0%x\n", stream_format );
	for(int i = 0; i < DUMMY_DISK_BUFFERS; i++){
		disk_buffers[i].len = bmo_fmt_stride(stream_format) * channels * bufferSize;
		disk_buffers[i].buf = malloc(disk_buffers[i].len);
		if(!disk_buffers[i].buf){
			bmo_err("malloc failed:%s\n", strerror(errno));
			return NULL;
		}
		disk_buffers[i].next = &disk_buffers[i+1];
		disk_buffers[i].written = 1;
		disk_buffers[i].tick = i;
	}
	disk_buffers[DUMMY_DISK_BUFFERS -1].next = NULL;

	assert(flags && channels && rate && bufferSize && filePath);
	int pthr_errnum;
		
	/*open file and write header*/
	//TODO write header
	BMO_state_t *state = BMO_getEnginestate();
	assert(state->driver_id == 0);         //We don't want to clobber existing drivers
	state->driver_id = BMO_DUMMY_DRIVER;         
	state->driver_rate = rate;
	state->n_playback_ch = channels;
	state->n_capture_ch = 0;
	state->n_ticks = 0;
	state->flags = (flags & (~(BMO_DSP_RUNNING|BMO_DSP_STOPPED)));
	state->buffer_size = bufferSize;
	state->driver.dummy.file = fopen(filePath, "w");
	if(!state->driver.dummy.file){
	    bmo_err("couldn't open %s:\n%s\n", filePath, strerror(errno));
		return NULL;
    }
	state->driver.dummy.file_path = filePath;
	state->ringbuffer = bmo_init_rb(state->buffer_size, state->n_playback_ch);
	if(!state->ringbuffer){
	    bmo_err("couldn't create ringbuffer:\n%s\n", strerror(errno));
		return NULL;
	}
//	state->driver.dummy.nano_secs_last_callback = 0;
//	state->driver.dummy.nano_secs_per_buffer = (1.0 / (double)rate) * (double) frames * 1000000000;
//	state->driver.dummy.nano_secs_sleep_callback = 0;
	
	state->driver.dummy.error_num = 0;
	state->driver.dummy.dither_type = bmo_fmt_dither(flags);
	state->driver.dummy.stream_format = bmo_fmt_enc(flags);
	state->driver.dummy.out_buffers = bmo_mb_new(channels, bufferSize);
	if(!state->driver.dummy.out_buffers){
	    bmo_err("driver does not have output buffers.");
	    return NULL;
	}
	state->driver.dummy.xrun_callback = BMO_dummyXrunCallback;
	state->driver.dummy.process_callback = BMO_dummyProcessCallback;
	state->driver.dummy.error_callback = BMO_dummyErrorCallback;
	state->driver.dummy.finished_callback = BMO_dummyFinishedCallback;
	state->driver.dummy.is_realtime = (flags & BMO_REALTIME) ? 1 : 0;
	
	/*set-up IPC*/
	//extern int pipefd[2];
//	int pipefd[2];
	
	//O_NONBLOCK is essential as we'll be writing to the pipe in a signal handler and we can't wait
	if(pipe2(pipefd, O_NONBLOCK) == -1){
		perror("pipe2");
		exit(EXIT_FAILURE);
	}
//	epipefd[0] = pipefd[0];
//	epipefd[1] = pipefd[1];
//	else
//	{
//		bmo_debug("fd[0]:%d fd[1]:%d\n",pipefd[0], pipefd[1]);
//	}
	state->driver.dummy.ipc.epfd = epoll_create(1);
	state->driver.dummy.ipc.fd = pipefd[0];
	state->driver.dummy.ipc.event.data.fd = pipefd[0];
	state->driver.dummy.ipc.event.events = EPOLLIN;
//	state->driver.dummy.ipc.event.data.fd = pipefd[1];
	if(epoll_ctl(state->driver.dummy.ipc.epfd, EPOLL_CTL_ADD, pipefd[0], &state->driver.dummy.ipc.event) != 0){
		perror("epoll_ctl");
	}
	
	/* create and detach new threads to call the callback and perform disk writes at appropriate times.*/
	bmo_debug("spawning disk_io_thread\n");
	pthr_errnum = pthread_create(&state->driver.dummy.disk_io_thread, NULL, _bmo_dummy_fwrite, state);
	if(pthr_errnum){
	    bmo_err("couldn't create thread:\n%s", strerror(pthr_errnum));
	    return NULL;
	}
	bmo_debug("spawning supervisor_thread\n");
	pthr_errnum = pthread_create(&state->driver.dummy.supervisor_thread, NULL, _bmo_dummy_scheduler, state);
	if(pthr_errnum){
	    bmo_err("couldn't create thread:\n%s", strerror(pthr_errnum));
	    return NULL;
	}
	return state;
}

void BMO_dummyStop(void * data)
{
	BMO_state_t * state = (BMO_state_t *)data;
	bmo_stop();
	state->driver.dummy.is_running = 0;
}


#else //if 0
#endif//__linux__
#endif
#include "../definitions.h"
BMO_state_t * 
bmo_dummy_start(BMO_state_t * state, uint32_t channels, uint32_t rate, uint32_t buf_len, uint32_t flags, const char * path)
{
    (void)path; (void)channels; (void)rate; (void)buf_len; (void)flags; (void)state;
    BMO_NOT_IMPLEMENTED;
    return NULL;
}



