#include <stdlib.h>

#include "../../src/definitions.h"
#include "../../src/sched.h"
#include "../../src/drivers/drivers.h"
#include "../../src/drivers/driver_utils.h"
#include "../../src/error.h"
#include "../../src/lua/getline_nowait.h"
#include "../../src/lua/lua.h"
#include "../../src/drivers/ringbuffer.h"
#include "../../src/graph.h"
#include "../../src/dsp_obj.h"
#include "../../src/import_export.h"

#include "../lib/test_common.c"


int main(int argc, char ** argv)
{
    bmo_test_setup();

    BMO_state_t * state = bmo_new_state();
    state->ringbuffer = bmo_init_rb(FRAMES, CHANNELS);
    BMO_dsp_obj_t * dsp = bmo_dsp_lua_new(
        0,
        CHANNELS,
        FRAMES,
        RATE,
        "io.write('[~]');"
        "io.flush();"
        "out = setmetatable({"
        "   pop=function(self, k)"
        "   table.remove(self, k)"
        "end"
        "}, {"
        "   __call=function(self, val)"
        "       self[#self + 1] = val"
        "   end"
        "});"
        "o = dsp.sys();"
        "i = dsp.sys('in');"
        "function dspmain()"
        "   o:zero()"
        "   async.repl()"
        "   for n in ipairs(out) do "
        "        o{out[n]};"
        "   end;"
        " end;\n",
        0
    );

    BMO_dsp_obj_t * fp = NULL;
    if (argc > 1)
        fp = bmo_dsp_bo_new_fopen(argv[1], 0, FRAMES);
    BMO_dsp_obj_t * rb = bmo_dsp_rb_new(
        state->ringbuffer,
        BMO_DSP_TYPE_OUTPUT,
        CHANNELS,
        FRAMES,
        RATE
    );

    if (fp)
        bmo_dsp_connect(fp, dsp, 0);

    bmo_dsp_connect(dsp, rb, 0);
    bmo_init_ipc(state);
    bmo_start(state);
    bmo_driver_init(state, CHANNELS, RATE, FRAMES, DRIVER, NULL);
    dsp->_init(dsp, 0);
    bmo_process_graph(state, rb, 0);
    bmo_driver_close(state);

    return 0;
}
