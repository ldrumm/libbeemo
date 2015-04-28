#include <math.h>
#include <assert.h>

#include "../src/multiplexers.c"
#include "../src/dsp/simple.c"
#include "../src/error.h"
#include "../src/drivers/ringbuffer.h"

#define LEN 65536
#define FRAMES 1024
#define CHANNELS 8


int main(void)
{
	float **input = NULL;
	float **output = NULL;
	BMO_ringbuffer_t *rb = NULL;

	/*allocators*/
	rb = bmo_init_rb(FRAMES, CHANNELS);
	input = malloc(CHANNELS * sizeof(float *));
	output = malloc((CHANNELS) * sizeof(float *));

	if(!input || !output || !rb)
		exit(EXIT_FAILURE);

	for(uint32_t ch = 0; ch < CHANNELS; ch++) {
		input[ch] = malloc(LEN * sizeof(float));
		output[ch] = malloc((LEN + FRAMES) * sizeof(float));
	}

	/*init with sine wave*/
	for(uint32_t ch = 0; ch < CHANNELS;ch++)
		for(uint32_t i = 0; i < LEN; i++)	//init array
			input[ch][i] = sinf(i * 0.001);

	/*run data through the rb and write the output to file */
	for(uint32_t j = 0; j < (LEN - FRAMES); j += FRAMES) {
		uint32_t written = bmo_write_rb(rb, input, FRAMES);
		bmo_debug("wrote %d\n", written);
		uint32_t read = bmo_read_rb(rb, output, FRAMES);
		for (uint32_t ch = 0; ch < CHANNELS; ch++) {
			input[ch] += read;
			output[ch] += read;
		}
	}
	for(uint32_t j = 0; j < (LEN - FRAMES); j += FRAMES) {
		for (uint32_t ch = 0; ch < CHANNELS; ch++) {
			input[ch] -= FRAMES;
			output[ch] -= FRAMES;
		}
	}
	for(uint32_t ch = 0; ch < CHANNELS; ch++){
		for(uint32_t i = 0; i < LEN - FRAMES; i++){
			assert(output[ch][i] == input[ch][i]);
		}
	}
	bmo_info("input and processed buffers are identical\n");
	for(uint32_t ch = 0; ch < CHANNELS; ch++) {
		free(input[ch]);
		free(output[ch]);
	}
    free(input);
    free(output);
    bmo_rb_free(rb);
	exit(EXIT_SUCCESS);
}
