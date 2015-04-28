#ifdef BMO_HAVE_LADSPA
#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <math.h>

#include "../src/definitions.h"
#include "../src/error.h"
#include "../src/ladspa.h"
#include "../src/dsp_obj.h"
#include "../src/memory/map.h"

#include "lib/test_common.c"
#include "lib/compiler.c"

int main(void)
{
    bmo_test_setup();
    const char * const plugin_source = "./lib/invert_plugin.c";
    int err = 0;
    size_t len = bmo_fsize(plugin_source, &err);
    assert(!err);
    char * input = bmo_map(plugin_source, 0, 0);
    char * source = calloc(len + 1, sizeof(char));
    assert(source);
    memcpy(source, input, len);
    bmo_unmap(input, len);
    char * inpath = compile_string(source, "-fPIC -shared -g", "so");
    assert(inpath);

    BMO_dsp_obj_t * plugin = bmo_dsp_ladspa_new(
        inpath,
        0,
        CHANNELS,
        FRAMES,
        RATE
    );
    if(!plugin){
        bmo_err("couldn't load '%s'\n", inpath);
        exit(EXIT_FAILURE);
    }
    plugin->_init(plugin, 0);
    for(uint32_t ch = 0; ch < CHANNELS; ch++){
        for(size_t i = 0; i < FRAMES; i++){
            plugin->in_buffers[ch][i] = sinf((float)i);
        }
    }
    plugin->_update(plugin, FRAMES);
    //The plugin is supposed to invert, and not alter the output signal in any other way
    for(uint32_t ch = 0; ch < CHANNELS; ch++){
        for(size_t i = 0; i < FRAMES; i++){
            assert_fequal(plugin->out_buffers[ch][i], -sinf((float)i));
        }
    }
    plugin->_close(plugin, 0);
    bmo_dsp_close(plugin);

    remove(inpath);
    free(plugin);
    free(source);
    free(inpath);
    return 0;
}
#else
int main(void){}
#endif
