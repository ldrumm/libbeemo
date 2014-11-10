#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "../src/definitions.h"
#include "../src/error.h"
#include "../src/graph.h"
#include "../src/dsp_obj.h"

#include "lib/test_common.c"

#define FREQ 200

int main(void)
{
    /**
        We build a simple graph of dependent objects, with a simple cycle and
        populate the head's input buffers with data.
        After calling bmo_update_dsp_tree() on the bottom of the graph, we expect to
        see that the data mixed into the bottom dsp object's output buffers is
        == dsp_top_a as all cycles should return silence.


dsp_top_a       dsp_top_b
    |               |   |
    |______   ______|   | <---Cycle
           |  |         |
        dsp_middle<-----
            |
        dsp_bottom <-after updating the graph's dependencies, we should see the mixed values from dsp_top_a and dsp_top_b

    */
    bmo_verbosity(BMO_MESSAGE_DEBUG);
    float * test_buf_a = malloc(FRAMES * sizeof(float));
    assert(test_buf_a);
    float * test_buf_b = malloc(FRAMES * sizeof(float));
    assert(test_buf_b);

    for(size_t i=0; i < FRAMES;i++){
        test_buf_a[i] = (float)(rand() % RAND_MAX) / (float)RAND_MAX;
        test_buf_b[i] = (float)(rand() % RAND_MAX) / (float)RAND_MAX;
    }

    BMO_dsp_obj_t * dsp_top_a, * dsp_top_b,*dsp_middle, *dsp_bottom;

    dsp_top_a = bmo_dsp_new(0, CHANNELS, FRAMES, RATE);
    dsp_top_b = bmo_dsp_new(0, CHANNELS, FRAMES, RATE);
    dsp_middle = bmo_dsp_new(0, CHANNELS, FRAMES, RATE);
    dsp_bottom = bmo_dsp_new(0, CHANNELS, FRAMES, RATE);

    bmo_dsp_connect(dsp_top_a, dsp_middle, 0);
    bmo_dsp_connect(dsp_top_b, dsp_middle, 0);
    bmo_dsp_connect(dsp_middle, dsp_bottom, 0);
    //Here is the cycle:
    bmo_dsp_connect(dsp_middle, dsp_top_b, 0);

    memcpy(dsp_top_a->in_buffers[0], test_buf_a, FRAMES * sizeof(float));
    memcpy(dsp_top_b->in_buffers[0], test_buf_b, FRAMES * sizeof(float));

    bmo_update_dsp_tree(dsp_bottom, 1, 0);

    for(size_t i=0; i < FRAMES;i++)
        assert(dsp_bottom->out_buffers[0][i] == test_buf_a[i]);

    free(test_buf_a);
    free(test_buf_b);
    bmo_dsp_close(dsp_top_a);
    free(dsp_top_a);
    bmo_dsp_close(dsp_top_b);
    free(dsp_top_b);
    bmo_dsp_close(dsp_middle);
    free(dsp_middle);
    bmo_dsp_close(dsp_bottom);
    free(dsp_bottom);
    return 0;
}
