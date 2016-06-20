#ifdef BMO_HAVE_LADSPA
#include <assert.h>
#include <math.h>
#include <ladspa.h>

#include "dsp_obj.h"
#include "definitions.h"
#include "error.h"
#include "memory/map.h"
#include "ladspa.h"

#define CONCERT_A 440.f

#define LOW(lower, upper, islog) ((islog) ? (exp(log((lower)) * 0.75 + log((upper)) * 0.25)) : ((lower) * 0.75 + (upper) * 0.25))
#define MID(lower, upper, islog) ((islog) ? (exp(log((lower)) * 0.5 + log((upper)) * 0.5)) : ((lower) * 0.5 + (upper) * 0.5))
#define HIGH(lower, upper, islog) ((islog) ? (exp(log((lower)) * 0.25 + log((upper)) * 0.75)) : ((lower) * 0.25 + (upper) * 0.75))
#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif
#define n_audio_inputs(ld) n_port_count_type((ld), LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT)
#define n_audio_outputs(ld) n_port_count_type((ld), LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT)
#define n_control_inputs(ld) n_port_count_type((ld), LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT)
#define n_control_outputs(ld) n_port_count_type((ld), LADSPA_PORT_CONTROL | LADSPA_PORT_OUTPUT)


static unsigned long
n_port_count_type(const LADSPA_Descriptor * ld, const LADSPA_PortDescriptor type)
{
    unsigned long n = 0;
    for (unsigned long i = 0; i < ld->PortCount; i++)
    if ((ld->PortDescriptors[i] & type) == type)
        n++;
    return n;
}


static LADSPA_Data normalise_port_val(
    const LADSPA_Descriptor * ld, unsigned long i,
    uint32_t rate, LADSPA_Data val)
{
    const LADSPA_PortRangeHint *hint = ld->PortRangeHints + i;
    const LADSPA_PortRangeHintDescriptor hd = hint->HintDescriptor;
    val = LADSPA_IS_HINT_BOUNDED_BELOW(hd) ? MAX(hint->LowerBound, val) : val;
    val =
    LADSPA_IS_HINT_BOUNDED_ABOVE(hd) ? MIN(hint->UpperBound, val) : val;
    val = LADSPA_IS_HINT_TOGGLED(hd) ? (val <= 0. ? 0. : 1.) : val;
    val = LADSPA_IS_HINT_SAMPLE_RATE(hd) ? val * (LADSPA_Data)rate : val;
    val = LADSPA_IS_HINT_INTEGER(hd) ? floor(val) : val;
    return val;
}

static LADSPA_Data
default_port_val(
    const LADSPA_Descriptor * ld, unsigned long i,
    uint32_t rate, float A440)
{
    const LADSPA_PortRangeHint *hint = ld->PortRangeHints + i;
    const LADSPA_PortRangeHintDescriptor hd = hint->HintDescriptor;
    LADSPA_Data lower, upper, val = 0.;
    lower = hint->LowerBound;
    upper = hint->UpperBound;
    int islog = LADSPA_IS_HINT_LOGARITHMIC(hd);
    if (LADSPA_IS_HINT_HAS_DEFAULT(hint->HintDescriptor)) {
        val = LADSPA_IS_HINT_DEFAULT_MINIMUM(hd) ? lower : val;
        val = LADSPA_IS_HINT_DEFAULT_MAXIMUM(hd) ? hint->UpperBound : val;
        val = LADSPA_IS_HINT_DEFAULT_LOW(hd) ? LOW(lower, upper, islog) : val;
        val = LADSPA_IS_HINT_DEFAULT_MIDDLE(hd) ? MID(lower, upper, islog) : val;
        val = LADSPA_IS_HINT_DEFAULT_HIGH(hd) ? HIGH(lower, upper, islog) : val;
        val = LADSPA_IS_HINT_DEFAULT_0(hd) ? 0. : val;
        val = LADSPA_IS_HINT_DEFAULT_1(hd) ? 1. : val;
        val = LADSPA_IS_HINT_DEFAULT_100(hd) ? 100. : val;
        val = LADSPA_IS_HINT_DEFAULT_440(hd) ? A440 : val;
    }
    return normalise_port_val(ld, i, rate, val);
}

static void
print_port_default(
    const LADSPA_Descriptor * ld, unsigned long i,
    uint32_t rate, float A440)
{
    const LADSPA_PortRangeHint *hint = ld->PortRangeHints + i;
    const LADSPA_PortRangeHintDescriptor hd = hint->HintDescriptor;
    printf(
        "Port %lu:%s:%s%s%s%s::%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s>=%1.1f<=%1.1f-->%f\n",
         i + 1, ld->PortNames[i],
         LADSPA_IS_PORT_AUDIO(ld->PortDescriptors[i]) ? "(audio)" : "",
         LADSPA_IS_PORT_CONTROL(ld->PortDescriptors[i]) ? "(control)" : "",
         LADSPA_IS_PORT_INPUT(ld->PortDescriptors[i]) ? "(input)" : "",
         LADSPA_IS_PORT_OUTPUT(ld->PortDescriptors[i]) ? "(output)" : "",
         LADSPA_IS_HINT_BOUNDED_BELOW(hd) ? "(bounded below)" : "",
         LADSPA_IS_HINT_BOUNDED_ABOVE(hd) ? "(bounded above)" : "",
         LADSPA_IS_HINT_TOGGLED(hd) ? "(toggled)" : "",
         LADSPA_IS_HINT_SAMPLE_RATE(hd) ? "(samplerate)" : "",
         LADSPA_IS_HINT_LOGARITHMIC(hd) ? "(logarithmic)" : "",
         LADSPA_IS_HINT_INTEGER(hd) ? "(integer)" : "",
         LADSPA_IS_HINT_HAS_DEFAULT(hd) ? "(has default)" : "",
         LADSPA_IS_HINT_DEFAULT_MINIMUM(hd) ? "(has default min)" : "",
         LADSPA_IS_HINT_DEFAULT_LOW(hd) ? "(default low)" : "",
         LADSPA_IS_HINT_DEFAULT_MIDDLE(hd) ? "(default mid)" : "",
         LADSPA_IS_HINT_DEFAULT_HIGH(hd) ? "(default high)" : "",
         LADSPA_IS_HINT_DEFAULT_MAXIMUM(hd) ? "(default max)" : "",
         LADSPA_IS_HINT_DEFAULT_0(hd) ? "(default 0)" : "",
         LADSPA_IS_HINT_DEFAULT_1(hd) ? "(default 1)" : "",
         LADSPA_IS_HINT_DEFAULT_100(hd) ? "(default 100)" : "",
         LADSPA_IS_HINT_DEFAULT_440(hd) ? "(default 440)" : "",
         hint->LowerBound,
         hint->UpperBound,
         default_port_val(ld, i, rate, A440)
    );
}

void
bmo_ladspa_print_info(const LADSPA_Descriptor * ld, uint32_t rate, float A440)
{
    LADSPA_Properties properties = ld->Properties;
    printf(
        "'%s':\n"
       "\tLabel:%s\n"
       "\tUniqueID:%lu\n"
       "\tMaker:%s\n"
       "\tCopyright:%s\n",
       ld->Name,
       ld->Label,
       ld->UniqueID,
       ld->Maker,
       ld->Copyright
    );
    printf(
        "plugin has:\n"
       "\t%lu audio input(s),\n"
       "\t%lu audio output(s),\n"
       "\t%lu control input(s),\n"
       "\t%lu control output(s)\n",
       n_audio_inputs(ld),
       n_audio_outputs(ld),
       n_control_inputs(ld),
       n_control_outputs(ld)
    );

    if (!LADSPA_IS_REALTIME(properties)) {
        printf("'%s' can run offline.\n", ld->Name);
    }
    if (LADSPA_IS_INPLACE_BROKEN(properties)) {
        printf("'%s' does not support in-place buffer writes.\n", ld->Name);
    }
    if (LADSPA_IS_HARD_RT_CAPABLE(properties)) {
        printf("'%s' is Hard-Realtime capable.\n", ld->Name);
    }
    bmo_debug("'%s' has %lu ports.\n", ld->Name, ld->PortCount);
    for (unsigned long i = 0; i < ld->PortCount; i++) {
        print_port_default(ld, i, rate, A440);
    }
    return;
}


static const
LADSPA_Descriptor *bmo_load_ladspa(const char *path, unsigned long index)
{
    LADSPA_Descriptor_Function ld;
    ld = bmo_loadlib(path, "ladspa_descriptor");

    if (ld) {
        bmo_debug("loaded %s at %p\n", path, ld);
        return ld(index);
    } else {
        bmo_debug("invalid or corrupt ladspa plugin %s\n", path);
        return NULL;
    }
}


static int _bmo_update_dsp_ladspa(BMO_dsp_obj_t *dsp, uint32_t flags)
{
    assert(dsp);
    (void) flags;
    BMO_LADSPA_dsp_state *state = (BMO_LADSPA_dsp_state *) dsp->userdata;
    state->ld->run(state->lh, dsp->frames);
    bmo_debug("\n");
    return 0;
}


static int _bmo_init_dsp_ladspa(BMO_dsp_obj_t *dsp, uint32_t flags)
{
    (void) flags;
    BMO_LADSPA_dsp_state *state = (BMO_LADSPA_dsp_state *) dsp->userdata;
    const LADSPA_Descriptor *ld = state->ld;
    LADSPA_Handle *lh = state->lh;
    unsigned long inports, outports, ctlports;
    inports = outports = ctlports = 0;
    for (unsigned long i = 0; i < ld->PortCount; i++) {
        LADSPA_PortDescriptor port = ld->PortDescriptors[i];
        if (LADSPA_IS_PORT_AUDIO(port)) {
            if (LADSPA_IS_PORT_INPUT(port) && inports < dsp->channels) {
                ld->connect_port(lh, i, dsp->in_buffers[inports]);
                inports++;
            } else if (LADSPA_IS_PORT_OUTPUT(port) && outports < dsp->channels) {
                ld->connect_port(lh, i, dsp->out_buffers[outports]);
                outports++;
            } else if (LADSPA_IS_PORT_OUTPUT(port) && LADSPA_IS_PORT_INPUT(port)) {
                bmo_err("Don't know how to connect multipurpose plugin port\n");
                return -1;
            }
        }
    }
    for (unsigned long i = 0; i < ld->PortCount; i++) {
        if (LADSPA_IS_PORT_CONTROL(ld->PortDescriptors[i])) {
            ld->connect_port(lh, i, state->ctl_ports + ctlports);
            state->ctl_ports[ctlports] =
            default_port_val(ld, i, dsp->rate, CONCERT_A);
            ctlports++;
        }
    }

    bmo_debug("connected %d input ports\n", inports);
    bmo_debug("connected %d output ports\n", outports);
    bmo_debug("connected %d control ports\n", ctlports);

    assert(inports <= dsp->channels);
    assert(outports <= dsp->channels);
    if (ld->activate)
        ld->activate(lh);
    return -1;
}


static int _bmo_close_dsp_ladspa(BMO_dsp_obj_t *dsp, uint32_t flags)
{
    (void)flags;
    BMO_LADSPA_dsp_state *state = dsp->userdata;
    if (state->ld->deactivate) {
        state->ld->deactivate(state->lh);
    }
    if (state->ld->cleanup) {
        state->ld->cleanup(state->lh);
    }
    free(state);
    return 0;
}


BMO_dsp_obj_t *bmo_dsp_ladspa_new(
    const char *path, uint32_t flags, uint32_t channels,
    uint32_t frames, uint32_t rate)
{
    (void)flags;
    const LADSPA_Descriptor *ld = NULL;
    LADSPA_Handle *lh = NULL;
    BMO_dsp_obj_t *dsp = NULL;
    BMO_LADSPA_dsp_state *state = calloc(1, sizeof(BMO_LADSPA_dsp_state));
    if (!state)
        return NULL;

    if (!(ld = bmo_load_ladspa(path, 0))) {
        goto err;
    }
    if (!LADSPA_IS_HARD_RT_CAPABLE(ld->Properties)) {
        bmo_err("'%s' is not realtime capable.\n", ld->Name);
        goto err;
    }
    if (!(lh = ld->instantiate(ld, rate))) {
        bmo_err("plugin instantiation failed for '%s'\n", ld->Name);
        goto err;
    }
    dsp = bmo_dsp_new(flags, channels, frames, rate);
    if (!dsp)
        goto err;

    if (n_audio_inputs(ld) > dsp->channels ||
        n_audio_outputs(ld) > dsp->channels) {
        bmo_err(
            "'%s'requires %lu inputs and %lu outputs;\n"
            "have %u inputs and %u outputs.\n",
            ld->Name,
            n_audio_inputs(ld),
            n_audio_outputs(ld),
            dsp->channels,
            dsp->channels
        );
        goto err;
    }
    bmo_ladspa_print_info(ld, rate, CONCERT_A);

    dsp->type = BMO_DSP_OBJ_LADSPA;
    dsp->userdata = state;
    dsp->_init = _bmo_init_dsp_ladspa;
    dsp->_update = _bmo_update_dsp_ladspa;
    dsp->_close = _bmo_close_dsp_ladspa;

    state->lh = lh;
    state->ld = ld;
    bmo_debug("%s:%s-%s loaded\n", path, ld->Name, ld->Label);
    return dsp;
err:
    free(state);
    free(dsp);
    return NULL;
}
#endif
