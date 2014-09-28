#define _POSIX_C_SOURCE 2 //popen(3)
#include "../../src/definitions.h"
#include "../../src/error.h"
#include "../../src/graph.h"
#include "../../src/dsp_obj.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define CHANNELS 1
#define FRAMES 2048
#define RATE 48000
#define FREQ 200
#define N_DSPS 100

#include "../lib/stopwatch.c"

int graphviz_callback(BMO_dsp_obj_t * node, void * userdata){
    FILE * file = userdata;
    BMO_ll_t * dependency = node->in_ports;
    while(dependency){
        fprintf(file,"%lu -> %lu;\n", ((BMO_dsp_obj_t *)dependency->data)->id, node->id);
        dependency = dependency->next;
    }
    return 0;
}
int visualize(BMO_dsp_obj_t * graph)
{
    #include <stdio.h>
    FILE * file = popen("xdot", "w");
    if(!file){
        return -1;
    }
    fprintf(file, "digraph G {\n");
    traverse_graph(graph, graphviz_callback, file);
    fprintf(file, "\n}\n");
    fflush(file);
    return pclose(file);
}


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
        dsp_bottom <-after updating the graph's dependencies, we should see the
        mixed values from dsp_top_a and dsp_top_b

    */
    bmo_verbosity(BMO_MESSAGE_DEBUG);

    BMO_dsp_obj_t * dsps[N_DSPS];
    for(size_t i = 0; i < N_DSPS; i++){
         dsps[i] = bmo_dsp_new(0, CHANNELS, FRAMES, RATE);
         if(i > 0){
            bmo_dsp_connect(dsps[i -1], dsps[i], 0);
        }
    }
    stopwatch_start();
    bmo_update_dsp_tree(dsps[N_DSPS - 1], 1, 0);
    printf("took %fs to update graph of %d objects", stopwatch_stop(), N_DSPS);
    visualize(dsps[N_DSPS - 1]);

    return 0;
}
