#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ladspa.h>
#include <stdio.h>

#define CONTROL 0
#define INPUT  1
#define OUTPUT 2

typedef struct {
    LADSPA_Data * invert_toggle;
    LADSPA_Data * input;
    LADSPA_Data * output;
} Inverter;

LADSPA_Handle instantiate(const LADSPA_Descriptor * ld, unsigned long rate)
{
    (void)ld;
    (void)rate;
    return calloc(1, sizeof(Inverter));
}

void connect_port(LADSPA_Handle lh, unsigned long port, LADSPA_Data * data)
{
    Inverter * inverter = (Inverter *)lh;
    switch (port) {
        case CONTROL:
            inverter->invert_toggle = data;
            break;
        case INPUT:
            inverter->input = data;
            break;
        case OUTPUT:
            inverter->output = data;
            break;
        default: assert(0);
    }
}

void run(LADSPA_Handle lh, unsigned long frames)
{
    Inverter * inverter = (Inverter *)lh;
    unsigned long i;
    if ((*inverter->invert_toggle) > 0.) {
        for (i = 0; i < frames; i++) {
            inverter->output[i] = -inverter->input[i];
        }
    }
    return;
}

static const char * portnames[] = {"Invert", "Audio In", "Audio Out"};
static const LADSPA_PortDescriptor pd[3] = {
    LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
    LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

static const LADSPA_PortRangeHint prh[3] = {
    {(LADSPA_HINT_TOGGLED | LADSPA_HINT_DEFAULT_1), 0, 1},
    {0, 0, 0},
    {0, 0, 0}
};

static LADSPA_Descriptor ld = {
    .UniqueID = 257, // IDs < 1000 are for dev. only
    .Label = "bmo_test_inverter",
    .Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE,
    .Name = "Beemo Test Suite LADSPA plugin",
    .Maker = "(libbeemo test suite)",
    .Copyright = "LGPL",
    .PortCount = 3,
    .PortDescriptors = pd,
    .PortNames = (const char **)portnames,
    .PortRangeHints = prh,
    .instantiate = instantiate,
    .connect_port = connect_port,
    .activate = NULL,
    .run = run,
    .run_adding = NULL,
    .set_run_adding_gain = NULL,
    .deactivate = NULL,
    .cleanup = free,
};

const LADSPA_Descriptor * ladspa_descriptor(unsigned long index)
{
    if (index)
        return NULL;
    return &ld;
}
