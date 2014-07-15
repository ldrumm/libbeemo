--[[
We expect @dsp to be registered in the global environment, as well as @this_dsp_pointer
dsp prvides getters and setters to the BMO_dsp_obj_t to which this environment belongs.
We then define 3 global tables @input, @output, and @control which can both consume lists of buffers when __call()ed, or be passed to functions / closures that operate on buffer objects
]]
--[[
function dsptick()
function dsptick_timespan()
function dspid()
function dsprate()
function dspisrealtime()
function
]]
--assert(dsp)
--assert(this_dsp_pointer)
----assert(new_dsp_buffer)
--print('@@@@@@@')
--print('creating outputbuffer')
--print('@@@@@@@')
--debug.debug()
--output = new_dsp_buffer("out")
--input = new_dsp_buffer("in")
--control = new_dsp_buffer("ctl")




