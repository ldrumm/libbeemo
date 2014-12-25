#include <stdint.h>
#include <stdarg.h>

#ifndef BMO_ERROR_H
#define BMO_ERROR_H

#define BMO_ERR_LEN 512

#define BMO_MESSAGE_DEBUG	3	//all messages
#define BMO_MESSAGE_INFO 	2	//a bit less info
#define BMO_MESSAGE_CRIT	1 	//only critical messages
#define BMO_MESSAGE_NONE 	0	//no message

#ifdef NDEBUG
#define bmo_debug(msg, ...) ;
#define bmo_err(msg, ...) (_bmo_message((BMO_MESSAGE_CRIT), "" ,(msg), ##__VA_ARGS__))
#define bmo_info(msg, ...) 

#define ANSI_COLOR_RED     " "
#define ANSI_COLOR_GREEN   " "
#define ANSI_COLOR_YELLOW  " "
#define ANSI_COLOR_RESET   " "

#else
//these colours from http://stackoverflow.com/questions/3219393
#ifndef _WIN32
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#else //win32
#include <windows.h>
#define ANSI_COLOR_RED     win_setcolor(FOREGROUND_RED)
#define ANSI_COLOR_GREEN   win_setcolor(FOREGROUND_GREEN)
#define ANSI_COLOR_YELLOW  win_setcolor(FOREGROUND_GREEN|FOREGROUND_RED)
#define ANSI_COLOR_RESET   ""
#endif

#define bmo_debug(msg, ...) (_bmo_message((BMO_MESSAGE_DEBUG),  __func__ ,(msg), ##__VA_ARGS__))	//These variadic macros require GCC extensions (clang works)
#define bmo_err(msg, ...) (_bmo_message((BMO_MESSAGE_CRIT), __func__ ,(msg), ##__VA_ARGS__))
#define bmo_info(msg, ...) (_bmo_message((BMO_MESSAGE_INFO), __func__ , (msg), ##__VA_ARGS__)) 

#endif

void _bmo_message(uint32_t level, const char * fn, const char * message, ...);
const char * bmo_strerror(void);
void bmo_verbosity(uint32_t level);
#endif
