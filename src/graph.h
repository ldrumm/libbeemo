#ifndef BMO_GRAPH_H
#define BMO_GRAPH_H
#include "definitions.h"
int bmo_dsp_connect(BMO_dsp_obj_t *source, BMO_dsp_obj_t *sink, uint32_t flags);
void bmo_dsp_detach(BMO_dsp_obj_t *a, BMO_dsp_obj_t *b);
int bmo_update_dsp_tree(BMO_dsp_obj_t *node, uint64_t tick, uint32_t flags);
void bmo_traverse_dag(BMO_dsp_obj_t *node,
                      int (*callback)(BMO_dsp_obj_t *node, void *userdata),
                      void *userdata);
#endif
