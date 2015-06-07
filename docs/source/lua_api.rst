| N.B This document is mostly just a scratching post, and doesn't
  actually reflect the current state of the lua API.
| It *will*, but these are mostly just sketches/ideas.

Lua API
-------

Buffer
------

-  A buffer is both a source and a sink.
-  Numerical indexing of the buffer should be natural, and efficient,
   returning the numeric representation of the audio at the given
   sample.
-  Writing a set of samples into an output buffer at an offset should be
   trivially easy. This is a pre-requisite for .. Sample-Accurate
   Scheduling.
-  System buffers should be predefined and global; these are analogous
   to stdin, stdout, stderr pointers in C.


Scheduler
---------

| The primary aim for the scheduler should be extensibility.
| Simplicity is nice, but the complex nature of a musical scheduler
  requires much more power. Therefore, a number of different paradigms
  should be supported.
|  1. Generators.
|  - The programmer should be able to pass in a generator function that
  when called, provides the next scheduled time for action.
|  - When a generator is exhausted it should return ``nil`` to align
  with the general style of generator/iterator functions in Lua.
|  - Raising an error should allow remaining events to be completed, but
  the generator to be removed from the list.
|  2. A fixed table of events.
|  - The programmer passes a table (and optionally an iterator function
  to traverse the table).
|  - When the table is has been traversed, it is removed from the list
  of scheduled events.
|  - numeric values correspond to a timestamp.
|  - table values correspond to another scheduled event (recursive
  tables should be possible). In such cases we need to define the
  semantics of timestamps going backwards in time.
|  - function values are expected to behave as generators.

::

        ```Lua
        Event{
            action(timestamp)               -- function of three arguments to be called at time of action.
            schedule -- (required) (function|table|coroutine)` -- table or generator of timestamps for action to be called. timestamps are expected to be a monotonically-increasing with each call. Scheduling behaviour with out-of-order timestamps is undefined.
            iterator -- (optional) (function)





        }
        ```

Sample-Accurate Scheduling
--------------------------

| A key requirement for musical scheduling is the ability for a sound
  event to be aligned to individual audio samples.
| There may be pure-math situations in which sub-sample scheduling is
  necessary (perhaps certain filters), so the api should allow for this
  in future.

Files
-----

| Opening an audio file should return a .. Buffer.
| Interpretation of raw string data as a buffer should have well-defined
  semantics (access to underlying format conversion api perhaps?)

Gain Control
------------

-  simple gain control of a buffer with a wrapper:
   ``gain(-6, my_buf) : Buffer``
-  gain control as a time-series. Generator API returning
   ``index, gain`` pairs for each generator run.

Global DSP object
-----------------

| dsp {
|  buf = function(channels, frames) -- get a temporary buffer.
  ``channels`` and ``frames`` default to the current output buffer
|  generator
|  osc = function(
|  open() -- open an audio file as a buffer
|  str\_to\_buf = function(str, format=BMO\_FORMAT\_PCM16\_LE,
  channels=CHANNELS, frames=FRAMES) --given a string, create a buffer
  using the given format
|  envelope = function(interpolator, data, ),
|  at = function({func, args}, schedule, absolute\_time=true) --schedule
  the given function to be called at the given time

::

    dsp.at(timestamp, callback[, schedule]) --[[
        ``schedule`` may be a generator function or a table with the following key-value pairs:

| ]]
| }
