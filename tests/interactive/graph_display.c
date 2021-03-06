#define _POSIX_C_SOURCE 2 // popen(3)

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

#include "../../src/definitions.h"
#include "../../src/error.h"
#include "../../src/graph.h"
#include "../../src/dsp_obj.h"
#include "../../src/util.h"

#include "../lib/stopwatch.c"

#define CHANNELS 1
#define FRAMES 2048
#define RATE 48000
#define FREQ 200
#define N_DSPS 100u


int graphviz_callback(BMO_dsp_obj_t * node, void * userdata)
{
    FILE *file = userdata;
    BMO_ll_t *dependency = node->in_ports;

    while (dependency) {
        BMO_dsp_obj_t * dsp = dependency->data;
        fprintf(file, "%" PRIu64" -> %" PRIu64 ";\n", dsp->id, node->id);
        dependency = dependency->next;
    }
    return 0;
}

int visualize(BMO_dsp_obj_t * graph)
{
    FILE * file = popen("xdot -", "w");
    if (!file) {
        return -1;
    }
    fprintf(file, "digraph G {\n");
    bmo_traverse_dag(graph, graphviz_callback, file);
    fprintf(file, "\n}\n");
    fflush(file);
    return pclose(file);
}

int main(void)
{
    bmo_verbosity(BMO_MESSAGE_DEBUG);

    BMO_dsp_obj_t * dsps[N_DSPS];
    for (unsigned i = 0; i < N_DSPS; i++) {
         dsps[i] = bmo_dsp_new(0, CHANNELS, FRAMES, RATE);
         if (i > 0) {
            bmo_dsp_connect(dsps[i -1], dsps[i], 0);
        }
    }
    stopwatch_start();
    bmo_update_dsp_tree(dsps[N_DSPS - 1], 1, 0);
    printf("took %fs to update graph of %ud objects", stopwatch_stop(), N_DSPS);
    visualize(dsps[N_DSPS - 1]);

    return 0;
}
