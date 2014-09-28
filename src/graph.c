#include <assert.h>
#include <stdint.h>
#include "definitions.h"
#include "atomics.h"
#include "error.h"
#include "dsp/simple.h"

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

int
bmo_dsp_connect(BMO_dsp_obj_t *source , BMO_dsp_obj_t * sink, uint32_t flags)
{
	assert(source);
	assert(sink);
	(void)flags;
	if(!source || !sink){
		bmo_err("could not connect DSP objects:%p -> %p\n", source, sink);
		return -1;
	}
	BMO_ll_t * head = sink->in_ports;
	BMO_ll_t * new = malloc(sizeof(BMO_ll_t));
	if(!new){
	    bmo_err("alloc failure\n");
	    return -1;
	}

	//push the new item to the front of the list.  Insertion is O(1)
	new->next = head;
	new->data = source;
    sink->in_ports = new;
	return 0;

}

void
bmo_dsp_detach(BMO_dsp_obj_t *a, BMO_dsp_obj_t *b)
{
	BMO_NOT_IMPLEMENTED;
	(void)a;
	(void)b;
}

int
bmo_update_dsp_tree(BMO_dsp_obj_t * node, uint64_t tick, uint32_t flags)
{
    /**Traverses a subtree of dsp objects, and updates their internal state,
    copying from out_buffers to in_buffers.
    If the given node has no dependencies it is updated immediately.  Otherwise,
    all dependencies of the node are updated recursively until all dependencies
    are satisfied.  This function does not attempt to detect cycles, as it assumes
    that the graph is acyclic.  However, as each dependency is atomically updated
    with the current tick value on function entry and this, acts as a conditional
    for the update we shouldn't see infinite loops, just silence returned
    from subgraphs that depend on cycles.

    */
    BMO_ll_t * dependency;
    if(!node){
        bmo_err("null node");
        return -1;
    }
    if(!BMO_ATOM_CAS_POINTER(&node->tick, tick-1, tick)){
        // if a node has been updated (possibly by another thread),
        // then we can assume its dependencies will be updated by
        // the other function too - so we can just return.
        assert(node->tick == tick);
        return 0;
    }
    uint32_t channels;

    dependency = node->in_ports;
    if(dependency){
        //If we will be mixing data into the current input buffers, we want them to
        // be zero beforehand.  This only occurs when this node is a dependant of other nodes.
        // This has the nice property of silencing cycles, if they occur.
        bmo_zero_mb(node->in_buffers, node->channels, node->frames);
    }
    while(dependency){
        bmo_update_dsp_tree(dependency->data, tick, flags);
        // If there is a channel mismatch between the two nodes, we don't
        // really have a method of deciding how the channels are mixed, so
        // for now, we just drop the upper channels. This is a TODO.
        channels = MIN(node->channels, ((BMO_dsp_obj_t *)dependency->data)->channels);
        bmo_mix_mb(
            node->in_buffers,
            node->in_buffers,
            ((BMO_dsp_obj_t *)dependency->data)->out_buffers,
            channels,
            node->frames
        );
        dependency = dependency->next;
    }
    dependency = node->ctl_ports;
    while(dependency){
        BMO_NOT_IMPLEMENTED; //FIXME control port directional semantics haven't been decided yet.
        bmo_update_dsp_tree(dependency->data, tick, flags);
        dependency = dependency->next;
    }
    return node->_update(node, flags);
}

int traverse_graph(
    BMO_dsp_obj_t * node,
    int (*callback)(BMO_dsp_obj_t * node, void * userdata),
    void * userdata
){
    /* Traverse a graph and apply the user's callback to each node. ``userdata``
    is given as the second argument to ``callback`` but is otherwise unused so
    can be NULL. If the callback returns nonzero this function will return.
    */
    int ret;
    BMO_ll_t * dependency;
    if(!node){
        return 0;
    }
    dependency=node->in_ports;
    ret = callback(node, userdata);
    if(ret){
        return ret;
    };
    while(dependency){
        ret = traverse_graph(dependency->data, callback, userdata);
        if(ret){
            return ret;
        }
        dependency = dependency->next;
    }
    return 0;
}
