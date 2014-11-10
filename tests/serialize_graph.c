#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "../src/definitions.h"
#include "../src/error.h"
#include "../src/graph.h"
#include "../src/dsp_obj.h"
#include "../src/lua/lua.h"
#include "../src/memory/map.h"

#include "lib/test_common.c"

#define FREQ 200
#define N_DSPS 100
#define BUF_LEN 1024

#include "lib/stopwatch.c"

static size_t
_max_consecutive_x(const char * s, char x)
{
    /** find the maximum number of repetitions of the given char x in string*/
    #ifndef MAX
    #define MAX(x, y) (((x) > (y))?(x): (y))
    #endif
    char accept[] = {x, '\0'};
    size_t max_subs = 0;
    size_t subs;
    const char * cp = s;
    while((cp = strchr(cp, x))){
       subs = strspn(cp, accept);
       max_subs = MAX(subs, max_subs);
       cp += MAX(subs, 1);
    }
    return max_subs;
}

static char *
strchrsubs(char *s, char old, char new)
{
    while(*s){
        if(*s == old)
            *s = new;
        s++;
    }
    return s;
}

static char *
serialize_lua_object(BMO_dsp_obj_t *node)
{
    size_t n_equals_signs = _max_consecutive_x(((BMO_lua_context_t *)node->handle)->user_script, '=') + 1;
    char * open_brackets = calloc((n_equals_signs) + 3, sizeof(char));
    char * close_brackets = calloc((n_equals_signs) + 3, sizeof(char));

    memset(open_brackets + 1, '=', (n_equals_signs));
    open_brackets[0] = '[';
    open_brackets[n_equals_signs + 1] = '[';
    strncpy(close_brackets, open_brackets, strlen(open_brackets));
    strchrsubs(close_brackets, '[', ']');

    size_t serialized_len = (n_equals_signs * 2) + 4 + ((BMO_lua_context_t *)node->handle)->user_script_len + 1;
    char * serialized = calloc(serialized_len, sizeof(char));
    snprintf(
        serialized,
        serialized_len,
        "%s%s%s",
        open_brackets,
        ((BMO_lua_context_t *)node->handle)->user_script,
        close_brackets
    );
    free(open_brackets);
    free(close_brackets);

    return serialized;
}

static char *
serialize_ladspa_object(BMO_dsp_obj_t *node)
{
    BMO_NOT_IMPLEMENTED;
    (void) node;
    return NULL;
}

static char *
serialize_bo_object(BMO_dsp_obj_t *node)
{
    BMO_NOT_IMPLEMENTED;
    (void)node;
    return NULL;
}

static char *
serialize_rb_object(BMO_dsp_obj_t *node)
{
    BMO_NOT_IMPLEMENTED;
    (void)node;
    return NULL;
}

static char *
serialize_deps(BMO_dsp_obj_t * node){
    //first, we count how many characters we're going to need in the malloced buffer.
    //each dependency takes 20 + 2 bytes in the worst case itoa(2^64-1), but is
    //likely to be 2-4 bytes in most cases in graphs with < 100 nodes.
    #ifdef _WIN32
    char * fmt_string = "%I64u, ";
    #else
    char * fmt_string = "%llu, ";
    #endif
    assert(node);
    assert(sizeof(node->id) <= 8);
    char buf[22] = {0}; //(2^64-1) as a char string is 20 bytes
    size_t n_chars = 0;
    BMO_ll_t * dep = node->in_ports;
    while(dep){
        snprintf(buf, sizeof(buf)-1, fmt_string,  (unsigned long long)((BMO_dsp_obj_t *)dep->data)->id);
        n_chars += strlen(buf);
        dep = dep->next;
    }
    char * retbuf = calloc(n_chars + 1, sizeof(char));
    if(!retbuf){
        return retbuf;
    }
    // Do the actual filling of the buffer now we know how long the result string will be
    dep = node->in_ports;
    while(dep){
        snprintf(buf, sizeof(buf)-1, fmt_string, (unsigned long long)((BMO_dsp_obj_t *)dep->data)->id);
        strncat(retbuf, buf, strlen(buf));
        dep = dep->next;
    }
    return retbuf;
}

struct bmo_dsp_serializer_callback_reg_t {
    char * (*serialize)(BMO_dsp_obj_t *);
    BMO_dsp_obj_t * (*unserialize)(void); //decide on params
    uint32_t type;
    char * name;
};

struct bmo_dsp_serializer_t {
    struct bmo_dsp_serializer_callback_reg_t *serializers;
    FILE * file;
};

static struct bmo_dsp_serializer_callback_reg_t *
get_dsp_serializer(const struct bmo_dsp_serializer_t * _serializers, BMO_dsp_obj_t * node)
{
    struct bmo_dsp_serializer_callback_reg_t * serializers = _serializers->serializers;
    while(serializers->serialize){
        if(node->type == serializers->type){
            return serializers;
        }
        serializers++;
    }
    return NULL;
}

int
serialize_callback(BMO_dsp_obj_t * node, void * userdata)
{
    //print a dsp object suitable for deserialising with lua.
    struct bmo_dsp_serializer_t * serializers = userdata;
/*    BMO_ll_t * dependency = node->in_ports;*/

    char * deps = serialize_deps(node);
    struct bmo_dsp_serializer_callback_reg_t * serializer = get_dsp_serializer(serializers, node);

    if(!serializer){
        bmo_err("serializer not found for type'%u'", node->type);
        return -1;
    }
    char * data = serializer->serialize(node);
    fprintf(
        serializers->file,
        "dsp{id=%llu, type=\"%s\", data=%s, inputs={%s}, channels=%d, rate=%d}\n",
         (unsigned long long)node->id,
        serializer->name,
        data,
        deps,
        node->channels,
        node->rate
    );
    fflush(serializers->file);
    free(data);
    free(deps);
    return 0;
}

static struct bmo_dsp_serializer_callback_reg_t reg[] = {
    {serialize_lua_object, NULL, BMO_DSP_OBJ_LUA, "lua"},
    {serialize_bo_object, NULL, BMO_DSP_OBJ_BO, "buffer"},
    {serialize_ladspa_object, NULL, BMO_DSP_OBJ_LADSPA, "ladspa"},
    {serialize_rb_object, NULL, BMO_DSP_OBJ_RB, "ringbuffer"},
    {NULL, NULL, 0, NULL}
};

int serialize(
    BMO_dsp_obj_t * node,
    const char * path,
    struct bmo_dsp_serializer_callback_reg_t * serializers
){
    #include <stdio.h>
    FILE * file = fopen(path, "w");
    if(!file){
        return -1;
    }

    struct bmo_dsp_serializer_t userdata;
    userdata.file = file;
    userdata.serializers = serializers;
    traverse_graph(node, serialize_callback, &userdata);
    fflush(file);
    return fclose(file);
    return 0;
}

int main(void)
{
    /**
        We build a simple graph of dependent objects, with a single cycle and
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
    int ret = 0;
    char output_file[BUF_LEN];

    strcpy(output_file, "tmp-serialize-graph-XXXXXX");
    if(!mkstemp(output_file)){
        bmo_err("couldn't created temp file:'%s'", strerror(errno));
        exit(EXIT_FAILURE);
    }
    BMO_dsp_obj_t * dsps[N_DSPS];
    for(size_t i = 0; i < N_DSPS; i++){
         dsps[i] = bmo_dsp_lua_new(0, CHANNELS, FRAMES, RATE, "a = [=[My name is ====]]]=];function dspmain() os.time() end", 0);
         dsps[i]->_init(dsps[i], 0);
         if(i > 0)
            bmo_dsp_connect(dsps[i -1], dsps[i], 0);
    }

    bmo_update_dsp_tree(dsps[N_DSPS-1], 1, 0);
    serialize(dsps[N_DSPS - 1], output_file, &reg[0]);
    for(size_t i = 0; i < N_DSPS; i++){
         dsps[i]->_close(dsps[i], 0);
         free(dsps[i]);
    }

    fflush(NULL);

    char luac_command[BUF_LEN] = "luac -p ";
    snprintf(
        luac_command + strlen(luac_command),
        BUF_LEN - strlen(luac_command) - 1,
        "\"%s\"",
        output_file
    );
    printf("%s\n", luac_command);
    //right now we only check whether the serialized tree is parsable lua.
    assert(bmo_fsize(output_file, &ret)!= 0);
    assert(!ret);
    ret = system(luac_command);
    if(ret != 0){
        bmo_err("failed to parse generated Lua script!\n");
    }
    remove(output_file);
    exit((ret == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
