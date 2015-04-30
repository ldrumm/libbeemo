Writing an audio Driver
================

There are a couple of steps to get a working audio driver in libbeemo, all of them fairly simple once you are familiar with your driver's API.

First, in `src/drivers/`, create your `.[ch]` files, exposing `bmo_yourdriver_start()` as a public function.

For the purposes of this article, your driver is assumed to be callback-based (most are).

1. You initialise your audio driver with a callback that will consume/deliver audio samples from/to the hardware. The audio driver backend must provide your callback with a userdatum (usually as a `void *`) that contains (among other things) a pointer to your `BMO_dsp_state_t`.
    - Your callback **must** be declared `static`.
    - Your callback **must not** allocate memory, make system calls, or other unbounded operations. i.e. it must have a completely predictable time complexity. Most system call will result in an involuntary context switch and have potentially unbounded runtime causing dropouts.
    - Your callback must not wait on any locks that will may starve a resource. Outcome is as above.
2. In your callback you read the required number of frames from state->ringbuffer and copy to the output. Audio date will be 32bit float, noninterleaved coming from the ringbuffer. If you need another format/multiplexed data, use one of the converters from `src/multiplexers.c`

3. Call `bmo_driver_callback_done(state, BMO_DSP_RUNNING);` before your callback returns.
    - If you want to terminate the audio stream, call `bmo_driver_callback_done(state, BMO_DSP_STOPPED);`
    - To query the current status, check `bmo_status(state)`. If this returns `BMO_DSP_STOPPED` you should output silence and then stop the audio stream.

All done, you must expose a public `bmo_yourdriver_start()` function as mentioned above. `bmo_yourdriver_start()` must set state->driver_id to a unique symbolic identifier for your driver. see `src/definitions.h` for the current list.

See` src/drivers/pa.c` for the Portaudio driver implementation.
