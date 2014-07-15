#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/error.h"
#define MAX_CHAR 126
#define MIN_CHAR 32


int main(int argc, char ** argv)
{
    char * long_message = ""
"This message is long.This message is long.This message is"
"long.This message is long.This message is long.This message is"
"long.This message is long.This message is long.This message is"
"long.This message is long.This message is long.This message is"
"long.This message is long.This message is long.This message is"
"long.This message is long.This message is long.This message is"
"long.This message is long.This message is long.This message is"
"long.This message is long.This message is long.This message is";

	if(argc < 2){
		bmo_verbosity(BMO_MESSAGE_DEBUG);
	}
	else{
		bmo_verbosity((uint32_t)atoi(argv[1]));	
	}
	
	bmo_debug("string:%s\n","stringhere");
	assert(strcmp(strchr(bmo_strerror(), ':')+1, "string:stringhere\n") == 0);
	
	bmo_info("integer:%d\n",3);
	assert(strcmp(strchr(bmo_strerror(), ':')+1, "integer:3\n") == 0);
	bmo_err("float:%1.1f\n", 1.0);
	assert(strcmp(strchr(bmo_strerror(), ':')+1, "float:1.0\n") == 0);
	bmo_err("pointer:%p\n", 0x5);
	assert(strcmp(strchr(bmo_strerror(), ':')+1, "pointer:0x5\n") == 0);
	bmo_err("unsigned:%u\n", 9u);
	assert(strcmp(strchr(bmo_strerror(), ':')+1, "unsigned:9\n") == 0);
	bmo_verbosity(BMO_MESSAGE_CRIT);
	bmo_debug("This should not appear!\n");
	assert(strcmp(strchr(bmo_strerror(), ':')+1, "unsigned:9\n") == 0);
	bmo_info("nor should this\n");
	assert(strcmp(strchr(bmo_strerror(), ':')+1, "unsigned:9\n") == 0);
	bmo_err("This message should be truncated and valgrind should be happy:\n%s\n", long_message);
	return 0;
}
