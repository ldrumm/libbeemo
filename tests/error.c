#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

#include "../src/error.h"

#include "lib/test_common.c"


int main(void)
{
	bmo_test_setup();
    char * long_message = ""
	"This message is long.This message is long.This message is"
    "long.This message is long.This message is long.This message is"
	"long.This message is long.This message is long.This message is"
	"long.This message is long.This message is long.This message is"
	"long.This message is long.This message is long.This message is"
	"long.This message is long.This message is long.This message is"
	"long.This message is long.This message is long.This message is"
	"long.This message is long.This message is long.This message is";

	bmo_debug("string:%s\n","stringhere");
	assert(strcmp(bmo_strerror(), "string:stringhere\n") == 0);

	bmo_info("integer:%d\n", -3);
	assert(strcmp(bmo_strerror(), "integer:-3\n") == 0);
	bmo_err("float:%1.1f\n", 1.0);
	assert(strcmp(bmo_strerror(), "float:1.0\n") == 0);
	bmo_err("unsigned:%u\n", 9u);
	assert(strcmp(bmo_strerror(), "unsigned:9\n") == 0);
	bmo_verbosity(BMO_MESSAGE_CRIT);
	bmo_debug("This should not appear!\n");
	assert(strcmp(bmo_strerror(), "unsigned:9\n") == 0);
	bmo_info("nor should this appear\n");
	assert(strcmp(bmo_strerror(), "unsigned:9\n") == 0);
	bmo_err("This message should be truncated and valgrind should be happy:\n%s\n", long_message);

	return 0;
}
