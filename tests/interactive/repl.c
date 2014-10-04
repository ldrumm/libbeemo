#include <stdlib.h>

#include "../../src/definitions.h"
#include "../../src/deleteme_sched.h"
#include "../../src/drivers/drivers.h"
#include "../../src/drivers/driver_utils.h"
#include "../../src/error.h"
#include "../../src/lua/getline_nowait.h"
#include "../../src/lua/lua.h"
#include "../../src/drivers/ringbuffer.h"
#include "../../src/graph.h"
#include "../../src/dsp_obj.h"

#define CHANNELS 2
#define FRAMES 512
#define RATE 44100

static void init(BMO_dsp_obj_t * dsp)
{
    BMO_lua_context_t * lua_dsp = (BMO_lua_context_t *)dsp->handle;
    char script[] =
        "function debug.repl()\n"\
            "line = async.getline()\n"\
            "if #line > 1 then\n"\
            "   func = loadstring(line)\n"\
            "   if func then "\
            "        local ok, msg = pcall(func)\n"\
            "        if not ok then \n"\
            "          print(msg)\n"\
            "       end \n"\
            " io.write('[~]');io.flush()\n"\
            "   end\n"\
            "end\n"\
        "end\n";
   if(luaL_dostring(lua_dsp->L, script) != 0){
        bmo_err("failed to load script:[[%s]]\n", script);
        exit(EXIT_FAILURE);
   };
}

int main(int argc, char ** argv)
{
    bmo_verbosity(BMO_MESSAGE_INFO);


    if(argc < 2){
        bmo_err("missing required argument (path to audiofile\n");
        exit(EXIT_FAILURE);
    }
    const char * path = argv[1];
    uint32_t driver = getenv("JACK") ? BMO_JACK_DRIVER : 0;
    BMO_state_t * state = bmo_new_state();
    state->ringbuffer = bmo_init_rb(FRAMES, CHANNELS);
    BMO_dsp_obj_t * dsp = bmo_lua_new(
        0,
        CHANNELS,
        FRAMES,
        RATE,
        "io.write('[~]');io.flush()\n"
        "out = setmetatable({\n"
            "pop=function(self, k)\n"
                "table.remove(self, k)\n"
            "end\n"
        "}, {__call=function(self, val)\n"
            "self[#self+1] = val end\n"
        "})\n "
        "o = dsp.sys()\n"
        "i = dsp.sys('in')\n"git s
        "function dspmain()\n"
            "debug.repl(); dsp.sys():zero()\n"

            "for n in ipairs(out) do o{out[n]} end\n"
        " end"
    );
    dsp->_init(dsp, 0);
    init(dsp);
    BMO_dsp_obj_t * file = bmo_dsp_bo_new_fopen(path, 0, FRAMES);
    BMO_dsp_obj_t * rb = bmo_dsp_rb_new(
        state->ringbuffer,
        BMO_DSP_TYPE_OUTPUT,
        CHANNELS,
        FRAMES,
        RATE
    );
    bmo_dsp_connect(file, dsp, 0);
    bmo_dsp_connect(dsp, rb, 0);
    bmo_init_ipc(state);
    bmo_start(state);
    bmo_driver_init(state, CHANNELS, RATE, FRAMES, driver, NULL);
    bmo_process_graph(state, rb, 0);
    bmo_driver_close(state);

    return 0;
}
