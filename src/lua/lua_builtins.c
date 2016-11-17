#ifdef BMO_HAVE_LUA
/*builtin funcs passed to the lua interpreter as callbacks to C dsp routines*/
/*could be implemented in Lua, but they are already exposed in C and the C
 * versions are fast*/
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "../buffer.h"
#include "../definitions.h"
#include "../dsp/simple.h"
#include "../error.h"
#include "../import_export.h"
#include "../util.h"


// Higher-order wrapper for single-buffer functions in the dsp_basics library
static int
_bmo_lua_calldspfunc_sb(
    lua_State *L,
    void(*dspfunc)(float * buffer, uint32_t samples)
)
{
    uint32_t ch;
    uint32_t frames;
    float **buf = NULL;
    luaL_argcheck(
        L,
        lua_islightuserdata(L, 1),
        1,
        "expected lightuserdata for buffer"
    );
    buf = lua_touserdata(L, 1);
    if (!buf) {
        return luaL_error(L, "passed NULL dsp_pointer");
    }
    ch = luaL_checkinteger(L, 2);
    frames = luaL_checkinteger(L, 3);
    for (uint32_t i = 0; i < ch; i++) {
        dspfunc(buf[i], frames);
    }
    return 0;
}

// Higher-order wrapper for oscillator functions in the dsp_basics library
static int _bmo_lua_calldsposc(
    lua_State *L,
    double (*dspfunc)(
        float *dest, float freq, double phase, float amp, uint32_t rate,
        uint32_t frames)
)
{
    uint32_t ch;
    uint32_t frames;
    uint32_t rate;
    float amplitude;
    double phase;
    double ret = 0.; // phase after cycle
    float freq;
    float **buf = NULL;
    luaL_argcheck(
        L,
        lua_islightuserdata(L, 1),
        1,
        "expected lightuserdata for buffer"
    );
    buf = lua_touserdata(L, 1);
    if (!buf) {
        return luaL_error(L, "passed NULL dsp_pointer");
    }
    ch = luaL_checkinteger(L, 2);
    frames = luaL_checkinteger(L, 3);
    freq = (float)luaL_checknumber(L, 4);
    phase = (double)luaL_checknumber(L, 5);
    amplitude = (float)luaL_checknumber(L, 6);
    rate = (uint32_t)luaL_checkinteger(L, 7);
    for (uint32_t i = 0; i < ch; i++) {
        ret = dspfunc(buf[i], freq, phase, amplitude, rate, frames);
    }
    lua_pushnumber(L, ret);
    return 1;
}

static int _bmo_mix_sbLua(lua_State *L)
{
    uint32_t ch;
    uint32_t frames;
    float **out = NULL;
    float **inA = NULL;
    float **inB = NULL;
    luaL_argcheck(
        L,
        lua_islightuserdata(L, 1),
        1,
        "expected lightuserdata for buffer"
    );
    out = lua_touserdata(L, 1);
    inA = lua_touserdata(L, 2);
    inB = lua_touserdata(L, 3);
    if (!out || !inA || !inB) {
        return luaL_error(L, "received nil buffer");
    }
    ch = luaL_checkinteger(L, 4);
    frames = luaL_checkinteger(L, 5);
    for (uint32_t i = 0; i < ch; i++) {
        bmo_mix_sb(out[i], inA[i], inB[i], frames);
    }
    return 0;
}

static int _bmo_sbcpyLua(lua_State *L)
{
    uint32_t ch;
    uint32_t frames;
    float **out = NULL;
    float **in = NULL;
    luaL_argcheck(
        L,
        lua_islightuserdata(L, 1),
        1,
        "expected lightuserdata for buffer"
    );
    out = lua_touserdata(L, 1);
    in = lua_touserdata(L, 2);
    if (!out || !in) {
        return luaL_error(L, "received NULL buffer");
    }
    ch = luaL_checkinteger(L, 3);
    frames = luaL_checkinteger(L, 4);
    for (uint32_t i = 0; i < ch; i++) {
        bmo_sbcpy(out[i], in[i], frames);
    }
    return 0;
}

static int _bmo_osc_sine_mix_sbLua(lua_State *L)
// void bmo_osc_sine_mix_sb(float * buffer, float freq, float phase, float
// amplitude, uint32_t rate, uint32_t samples);
//--[[function osc_sine(float ** buffer, channels, frames, freq, phase,
//amplitude, rate)]]
{
    return _bmo_lua_calldsposc(L, bmo_osc_sine_mix_sb);
}

static int _bmo_osc_sq_mix_sbLua(lua_State *L)
{
    return _bmo_lua_calldsposc(L, bmo_osc_sq_mix_sb);
}

static int _bmo_osc_saw_sbLua(lua_State *L)
{
    return _bmo_lua_calldsposc(L, bmo_osc_saw_sb);
}

static int _bmo_gain_sbLua(lua_State *L)
{
    uint32_t ch;
    uint32_t frames;
    float gain;
    float **buf = NULL;
    luaL_argcheck(
        L,
        lua_islightuserdata(L, 1),
        1,
        "expected lightuserdata for buffer"
    );
    buf = lua_touserdata(L, 1);
    if (!buf) {
        return luaL_error(L, "received NULL buffer");
    }
    ch = luaL_checkinteger(L, 2);
    frames = luaL_checkinteger(L, 3);
    gain = (float)luaL_checknumber(L, 4);
    for (uint32_t i = 0; i < ch; i++) {
        bmo_gain_sb(buf[i], frames, gain);
    }
    return 0;
}

static int _bmo_inv_sbLua(lua_State *L)
{
    return _bmo_lua_calldspfunc_sb(L, bmo_inv_sb);
}

static int _bmo_zero_sbLua(lua_State *L)
{
    //--zero_buffer(buffer, ch, frames)
    return _bmo_lua_calldspfunc_sb(L, bmo_zero_sb);
}

static inline BMO_dsp_obj_t *_bmo_lua_getdsplightuserdata(lua_State *L, int stack_idx)
{
    BMO_dsp_obj_t *dsp = NULL;
    luaL_argcheck(
        L,
        lua_islightuserdata(L, stack_idx),
        stack_idx,
        "expected lightuserdata for dsp_pointer"
    );
    dsp = lua_touserdata(L, stack_idx);
    if (!dsp) {
        luaL_error(L, "passed NULL dsp_pointer");
        return NULL;
    }
    return dsp;
}

static int _bmo_lua_gettick(lua_State *L)
{
    BMO_dsp_obj_t *dsp = NULL;
    dsp = _bmo_lua_getdsplightuserdata(L, 1);
    lua_pushnumber(L, (lua_Number)dsp->tick);
    return 1;
}


static int _bmo_lua_getrate(lua_State *L)
{
    BMO_dsp_obj_t *dsp = NULL;
    dsp = _bmo_lua_getdsplightuserdata(L, 1);
    lua_pushnumber(L, (lua_Number)dsp->rate);
    return 1;
}

static int _bmo_lua_getchannels(lua_State *L)
{
    BMO_dsp_obj_t *dsp = NULL;
    dsp = _bmo_lua_getdsplightuserdata(L, 1);
    lua_pushnumber(L, (lua_Number)dsp->channels);
    return 1;
}

static int _bmo_lua_getframes(lua_State *L)
{ // get n frames
    //--getdspframes(dsp_pointer)
    BMO_dsp_obj_t *dsp = NULL;
    dsp = _bmo_lua_getdsplightuserdata(L, 1);
    lua_pushnumber(L, (lua_Number)dsp->frames);
    return 1;
}

// static int __bmo_lua_get_dsp_id(lua_State *L)
//{
//    //--get_dspbyid(dsp_identifier)
//    return luaL_error(L, "not implemented%s", __func__);
//}

static int _bmo_lua_getbufferlightuserdata(lua_State *L)
{ // get buffer pointer from BMO_dsp_obj_t by name
    //--getbufferpointer(dsp_pointer, buffername)
    // buffername is expected to be 'in', 'out', or 'ctl'
    BMO_dsp_obj_t *dsp = NULL;
    void *buf = NULL;
    const char *buffername = NULL;
    size_t arglen = 0;

    luaL_argcheck(
        L,
        lua_islightuserdata(L, 1),
        1,
        "expected lightuserdata for dsp_pointer"
    );
    dsp = lua_touserdata(L, 1);
    if (!dsp) {
        return luaL_error(L, "passed NULL dsp_pointer");
    }
    luaL_argcheck(L, lua_isstring(L, 2), 2, "expected string for buffername");
    buffername = luaL_checklstring(L, 2, &arglen);
    if (strncasecmp(buffername, "in", 2) == 0)
        buf = dsp->in_buffers;
    else if (strncasecmp(buffername, "out", 2) == 0)
        buf = dsp->out_buffers;
    else if (strncasecmp(buffername, "ctl", 2) == 0)
        buf = dsp->out_buffers;
    else
        return luaL_error(L, "unknown buffername:%s", buffername);

    lua_pushlightuserdata(L, buf);
    return 1;
}

static int _bmo_lua_tmpbuf(lua_State *L)
{
    uint32_t channels = (uint32_t)luaL_checkinteger(L, 1);
    uint32_t frames = (uint32_t)luaL_checkinteger(L, 2);
    float **buf = bmo_mb_new(channels, frames);
    if (!buf) {
        return luaL_error(L, "couldn't create buffer:%s", strerror(errno));
    }
    lua_pushlightuserdata(L, buf);
    return 1;
}

static int _bmo_lua_tmpbuffree(lua_State *L)
{
    uint32_t channels = (uint32_t)luaL_checkinteger(L, 2);
    if (!lua_islightuserdata(L, 1)) {
        bmo_err("programmer error in lua tempbuffer\n");
        luaL_argcheck(L, 0, 1, "expected lightuserdata for the buffer");
        return luaL_error(L, "wrong argument type for buffer");
    }

    void *buf = lua_touserdata(L, 1);
    bmo_mb_free(buf, channels);
    return 0;
}

static int _bmo_lua_setsample(lua_State *L)
{
    //--set(buffer_pointer, ch, idx, val)
    uint32_t idx;
    uint32_t ch;
    float **buf;
    lua_Number val;
    luaL_argcheck(
        L,
        lua_islightuserdata(L, 1),
        1,
        "expected lightuserdata for buffer_pointer"
    );
    buf = lua_touserdata(L, 1);
    ch = (uint32_t)luaL_checkinteger(L, 2);
    luaL_argcheck(L, ch >= 1, 2, "invalid channel, index");
    idx = (uint32_t)luaL_checkinteger(L, 3);
    luaL_argcheck(L, idx >= 1, 3, "invalid frame index");
    val = (float)luaL_checknumber(L, 4);
    (void)val;
    (void)buf;
    buf[ch - 1][idx - 1] = (float)val;

    return 0;
}

static int _bmo_lua_getsample(lua_State *L)
{
    //--set(buffer_pointer, ch, idx)
    uint32_t idx;
    uint32_t ch;
    float **buf;
    luaL_argcheck(
        L,
        lua_islightuserdata(L, 1),
        1,
        "expected lightuserdata for buffer_pointer"
    );
    buf = lua_touserdata(L, 1);
    ch = (uint32_t)luaL_checkinteger(L, 2);
    luaL_argcheck(L, ch >= 1, 2, "invalid channel, index");
    idx = (uint32_t)luaL_checkinteger(L, 3);
    luaL_argcheck(L, idx >= 1, 3, "invalid frame index");

    lua_pushnumber(L, (lua_Number)buf[ch - 1][idx - 1]);

    return 1;
}

static int _bmo_lua_uid(lua_State *L)
{
    lua_pushnumber(L, bmo_uid());
    return 1;
}

static int _bmo_lua_fopen(lua_State *L)
{
    luaL_argcheck(L, lua_isstring(L, 1), 1, "file path required");
    const char *path = lua_tostring(L, 1);

    // TODO flags
    /*    local buffer_obj_p, channels, rate = dsp._fopen(path, flags)*/
    BMO_buffer_obj_t *obj = bmo_fopen(path, BMO_BUFFERED_DATA);
    if (!obj)
        return luaL_error(L, "failed to open '%s'", path);

    lua_pushlightuserdata(L, obj);
    lua_pushnumber(L, (lua_Number)obj->channels);
    lua_pushnumber(L, (lua_Number)obj->rate);

    return 3;
}

static int _bmo_lua_fread(lua_State *L)
{
    //    read from(BMO_buffer_obj_t * (1)) into a multichannel buffer (2) of
    //    channels(3), at most frames(4)
    luaL_argcheck(
        L,
        lua_islightuserdata(L, 1),
        1,
        "BMO_buffer_obj_t * expected"
    );
    BMO_buffer_obj_t *from = lua_touserdata(L, 1);
    float **to = lua_touserdata(L, 2);
    int frames = luaL_checkinteger(L, 3);
    if (from->read(from, to, frames) < frames) {
        lua_pushboolean(L, 0);
        return 1;
    };

    return 0;
}
static const struct luaL_Reg dsp_builtins[] = {
    {"zero_buffer", _bmo_zero_sbLua},
    {"invert_buffer", _bmo_inv_sbLua},
    {"gain_buffer", _bmo_gain_sbLua},
    {"mix_buffers", _bmo_mix_sbLua},
    {"invert_buffer", _bmo_inv_sbLua},
    {"copy_buffer", _bmo_sbcpyLua},
    // {"copy_buffer_off", _bmo_sbcpyOffsetLua},
    {"osc_sine", _bmo_osc_sine_mix_sbLua},
    {"osc_square", _bmo_osc_sq_mix_sbLua},
    {"osc_saw", _bmo_osc_saw_sbLua},
    {"set", _bmo_lua_setsample},
    {"get", _bmo_lua_getsample},
    {"getbufferpointer", _bmo_lua_getbufferlightuserdata},
    {"gettempbuf", _bmo_lua_tmpbuf},
    {"gctempbuf", _bmo_lua_tmpbuffree},
    {"getdspchannels", _bmo_lua_getchannels},
    {"getdspframes", _bmo_lua_getframes},
    {"getdsptick", _bmo_lua_gettick},
    {"getdsprate", _bmo_lua_getrate},
    {"uid", _bmo_lua_uid},
    {"_fopen", _bmo_lua_fopen},
    {"_fread", _bmo_lua_fread},
    {NULL, NULL}
    // Add new builtin functions here
};

// extern int _bmo_lua_reg_new_buffer(lua_State *L); //FIXME NEW


void _bmo_lua_reg_builtins(
    lua_State *L,
    void *dsp,
    const char *tablename,
    const char *dsp_pointer_name
)
{
    luaL_newlib(L, dsp_builtins); // +1
    lua_pushvalue(L, -1);         // +1
    lua_setglobal(L, tablename);  // -1

    // _bmo_lua_reg_new_buffer(L);         // +2
    // lua_setfield(L, -3, "___buf_new");  // -1
    // lua_pop(L, lua_gettop(L));          // -1
    lua_pushlightuserdata(L, dsp);      // +1
    lua_setglobal(L, dsp_pointer_name); // -1
    lua_settop(L, 0);
}

#endif
