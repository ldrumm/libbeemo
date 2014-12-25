#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "error.h"

static uint32_t debug_level = BMO_MESSAGE_INFO;
static char msg[BMO_ERR_LEN] = {'\0'};
static FILE * out_stream = NULL;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincon.h>
static int console_wasinit;
static HANDLE console_handle;
//The following should be CONSOLE_SCREEN_BUFFER_INFOEX so we can reset the background
//easily, but the MinGW CRT doesn't have the function definition.
static CONSOLE_SCREEN_BUFFER_INFO console_info; 

static void color_setup(void)
{
	static int init;
	if(init){
		return;
	}
	console_wasinit = 1;
	console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(console_handle, &console_info);
}

static const char *win_setcolor(WORD color)
{
	color_setup();
	SetConsoleTextAttribute(console_handle, color);
	return "";
}

static void win_resetcolor(void)
{
	SetConsoleTextAttribute(console_handle, console_info.wAttributes);
}

#endif

void bmo_verbosity(uint32_t level)
{
	if (level > BMO_MESSAGE_DEBUG)
		level = BMO_MESSAGE_DEBUG;
	debug_level = level;
}

FILE * 
bmo_err_stream(FILE * file)
{
    if(!file) 
        return out_stream;
    out_stream = file;
    return out_stream;
}

void _bmo_message(uint32_t level, const char * fn, const char * message, ...)
{
	va_list argp;
	va_start(argp, message);
	
	if(!message)
		return;
	
	int doprint = 1;
	
	if(level <= debug_level)
	{
		const char * color;
		int len;
		switch(level)
		{
			case BMO_MESSAGE_CRIT: color = ANSI_COLOR_RED; break;
			case BMO_MESSAGE_INFO: color = ANSI_COLOR_YELLOW; break;
			case BMO_MESSAGE_DEBUG: color = ANSI_COLOR_GREEN; break;
			default:color = "";
		}
		
		len = snprintf(msg, BMO_ERR_LEN-1, "[ %s%s()%s ]:", color, fn, ANSI_COLOR_RESET);
		
		assert(BMO_ERR_LEN - 1 - len >=0);
		
		vsnprintf(msg + len, (size_t)(BMO_ERR_LEN - 1 - len), message, argp);
		fputs(msg, out_stream != NULL ? out_stream:stderr);
		win_resetcolor();
//		doprint = 0;
	}
	else if(level == BMO_MESSAGE_CRIT)
	{
		if(doprint) 
			vsnprintf(msg, BMO_ERR_LEN-1, message, argp);
	}
	
	va_end(argp);
}


const char * bmo_strerror(void)
{
	return msg;
}

