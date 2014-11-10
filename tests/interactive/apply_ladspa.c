#include <stdlib.h>

#include "../../src/definitions.h"
#include "../../src/error.h"
#include "../../src/drivers/ringbuffer.h"
#include "../../src/import_export.h"
#include "../../src/graph.h"
#include "../../src/ladspa.h"
#include "../../src/buffer.h"
#include "../../src/dsp_obj.h"
#include "../../src/drivers/driver_utils.h"

#include "../lib/test_common.c"

#define DITHER 0

uint32_t htobe(uint32_t n)
{
    #if BMO_ENDIAN_LITTLE
    return (((((uint32_t)(n) & 0xFF)) << 24)|
        ((((uint32_t)(n) & 0xFF00)) << 8)   |
        ((((uint32_t)(n) & 0xFF0000)) >> 8) |
        ((((uint32_t)(n) & 0xFF000000)) >> 24));
    #else
    return n;
    #endif
}

void usage(const char * progname)
{
    fprintf(stderr, "%s /path/to/input/file /path/to/output/file /path/to/ladspa/plugin.so\n", progname);
    exit(EXIT_FAILURE);
}

int fp32_au_header(uint32_t channels, uint32_t rate, FILE * file)
{
    uint32_t header[7] = {
        0x2e736e64,
        0x0000001c,
        0xffffffff,
        0x00000007,
        htobe(rate),
        htobe(channels),
        htobe(0)
    };
    size_t n = fwrite(header, sizeof(uint32_t), 7, file);
    assert(n == 28);
    return n == 28;
}

int main(int argc, char ** argv)
{
    bmo_test_setup();
    if(argc < 4){
        usage(argv[0]);
    }
    const char * inpath = argv[1];
    const char * outpath = argv[2];
    const char * pluginpath = argv[3];

    BMO_state_t * state = bmo_new_state();

    BMO_dsp_obj_t * in = bmo_dsp_bo_new_fopen(inpath, 0, FRAMES);
    BMO_dsp_obj_t * plugin = bmo_dsp_ladspa_new(
        pluginpath,
        BMO_DSP_TYPE_OUTPUT,
        in->channels,
        FRAMES,
        in->rate
    );
    if(!plugin){
        bmo_err("couldn't load '%s'\n", pluginpath);
        exit(EXIT_FAILURE);
    }
    for(size_t ch = 0; ch < plugin->channels; ch++){
        for(size_t f = 0; f < FRAMES; f++)
            plugin->ctl_buffers[ch][f] = 1.;
    }
    FILE * out = fopen(outpath, "wb");
    fp32_au_header(plugin->channels, plugin->rate, out);

    bmo_dsp_connect(in, plugin, 0);

    //Actually apply the plugin
    do{
        bmo_update_dsp_tree(plugin, state->n_ticks, 0);
        bmo_fwrite_mb(
            out,
            plugin->out_buffers,
            plugin->channels,
            BMO_FMT_FLOAT_32_BE,
            FRAMES,
            DITHER
        );
    }while(state->n_ticks++ < 1000);

    fflush(out);
    fclose(out);
    return 0;
}
