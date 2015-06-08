Writing an audio Driver
=======================

There are a couple of steps to get a working audio driver in libbeemo,
all of them fairly simple once you are familiar with your driver's API.
There are two models here: Adding to the main library, for everyone's benefit, or adding a new driver at runtime:


Modifying the public API
^^^^^^^^^^^^^^^^^^^^^^^^
First, in ``src/drivers/``, create your ``.[ch]`` files, exposing
``bmo_yourdriver_start()`` as a public function.

For the purposes of this article, your driver is assumed to be
callback-based (most are).

#. You initialise your audio driver with a callback that will
   consume/deliver audio samples from/to the hardware. The audio driver
   backend must provide your callback with a userdatum (usually as a
   ``void *``) that contains (among other things) a pointer to your
   ``BMO_state_t``.

   -  Your callback **must** be declared ``static`` (if you are not altering the libbeemo library itself, but registering you driver at runtime you can declare it however you like as you simply pass a pointer to an init function that should initialise your driver.
   -  Your callback **must not** allocate memory, make system calls, or other unbounded operations. i.e. it must have a completely predictable time complexity. Most system calls will result in an involuntary context switch and have potentially unbounded runtime causing dropouts. Nothing kills the mood quicker than unintentional glitching.
   -  In the same vein as above: Your callback must not wait on any locks or call functions that wait on a low-priority thread that may starve the audio buffer (priority inversion).

#. In your callback you read the required number of frames from
   state->ringbuffer and copy to the output. Audio date will be the machine float float, noninterleaved coming from the ringbuffer. If your driver produces/consumes another encoding/multiplex, use one of the converters from ``src/multiplexers.c``

#. Call ``bmo_driver_callback_done(state, status);`` before
   your callback returns, where ``status`` is one of ``BMO_DSP_RUNNING`, `BMO_DSP_STOPPED``.

#. Ensure that your callback handles the case where the library is in state `BMO_DSP_STOPPED`, but the audio card is still expecting data at regular intervals. If this is the case, the callback should write silence into the output and discard any input.

   -  To query the current status, check ``bmo_status(state)``. If this returns ``BMO_DSP_STOPPED`` you should output silence for the buffer and then top the audio stream.
   -  If you want to terminate the audio stream, call ``bmo_driver_callback_done(state, BMO_DSP_STOPPED);``


All done, you must expose a public ``bmo_yourdriver_start()`` function
as mentioned above. ``bmo_yourdriver_start()`` must set
state->driver\_id to a unique symbolic identifier for your driver. see
``src/definitions.h`` for the current list.

See ``src/drivers/pa.c`` and ``src/definitions.h`` for the Portaudio driver implementation.


Registering a Driver at Runtime
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. warning::
    This is a **TODO**. Ignore this section while this warning is here.

If you're using an audio driver that's so hip that nobody is likely to benefit from it (or your bogonic employer is afraid of open-source), you can build the vanilla library as usual, and then register your driver at runtime.

.. code-block:: C

    typedef struct BMO_driver_t {
        int (*start)(BMO_state_t * state, uint32_t channels, uint32_t rate, uint32_t buf_len, uint32_t flags, void *userdata);
        int (*stop)(BMO_state_t *state, void *userdata);
        uint32_t driver_id;
        const char * name;
        void * userdata;
    }

    bmo_register_driver(BMO_state_t *state, BMO_driver_t *driver);
