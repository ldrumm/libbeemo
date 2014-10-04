function dsp.gain(factor)
--[[
This function is a closure factory that creates an a constant gain closure. It returns a function that can be run with a buffer as its first argument.
]]
    --set defaults where not given
    local db_gain = factor or 0.0
    --define the closure function
    local gain_fn = function(buf)
        dsp.gain_buffer(buf.buffer, buf.channels, buf.frames, db_gain)
    end
    return gain_fn
end

function dsp.zero(t)
    if type(t) == 'table' then
        return dsp.zero_buffer(t.buffer, t.channels, t.frames)
    end
        return function(buf)
            return dsp.zero_buffer(buf.buffer, buf.channels, buf.frames)
        end
end

function dsp.envelope(points, interpfn)
--{{time, gain}, {time, gain}, curve_func=function()end}

end

function dsp.enveloper(samples, factor)
    return function(f) samples = samples -1 ; return 1/math.exp(1/(samples * factor)) end
end

