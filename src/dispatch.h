#ifdef BMO_ENABLE_DISPATCH
#ifndef BMO_DISPATCH_H
#define BMO_DISPATCH_H
int BMO_pushMsg(int dspObj, BMO_message_t message);

int BMO_triggerPlayback(BMO_multichannel_buff *buf);

int bmo_dsp_connect(int dspObjFrom, int buffer,

typedef BMO_DSP_callback_t int (*)(

int setDSPCallback(int dspObj, BMO_DSP_callback_t * fn, void *data, void *arg2_varname);
/**<generic callback interface that calls a callback once per tick
// @data is with data as the arg.
This happens once before dspmain() is called


int setDSPVar(int dspObj, void * var, uint32_t datatype, const char * var_name);
//sets the value of the named variable to the given value
//This happens once at the next tick

int setDSPWatchVar(int dspObj, void * var, uint32_t datatype, size_t len, const char * var_name);
//    datatypes:
//    int
//    float
//    double
//    float_array
//    bytestring
//    BMO_multichannel_buff
//    bool

type struct watchvar_t {
    void * data;
    const char * name;
    size_t len;
    uint32_t type;
    int (*update_fn)(struct watchvar_t * var);

} watchvar_t;
void BMO_update_graph
#endif
#endif
