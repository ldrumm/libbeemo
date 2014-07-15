function dsp.new_osc(...)
--[[
This function is a closure factory that creates an oscillator closure. It returns a function that can be run with a buffer as its first argument.
The oscillator will write into the buffer the correct number of samples with the expected phase-alignment for the current tick
]]
    --set defaults where not given
    local t =  {shape='sine', 
                phase=0, 
                freq=440, 
                amplitude=0.5, 
                tick=1, 
                osc=nil
            }
    for key, _ in pairs(t) do
        for __, val in pairs(...) do
            if __ == key then
                t[key] = val
            end
        end
    end
--    for key, val in pairs(t) do 
--        print(key, val)
--    end
    --set the oscillator function
    if t.shape == 'sine' then
        t.osc = dsp.osc_sine
    elseif t.shape == 'saw' then
        t.osc = dsp.osc_saw
    elseif t.shape == 'square' then
        t.osc = dsp.osc_square
    else
        error("oscillator not selected", 2)
    end

    --define the closure function
    local osc_func = function(buf)
        t.phase = t.phase + ((buf.frames / buf.rate / t.freq) * 360) % 360
        t.osc(buf.buffer, buf.channels, buf.frames, t.freq, t.phase, t.amplitude, buf.rate)
        t.tick = t.tick + 1
    end
    return osc_func
end

function dsp.chirp_lin(...)
   local t = {tick =1,
        phase = 0,
        rate = 44100,
        factor = 0.001,
        totframes = 0,
        amplitude = 0.5,
        secs = 1.0,
        low = 1,
        high = 20000
    }
    for key, _ in pairs(t) do
        for __, val in pairs(...) do
            if __ == key then
                t[key] = val
            end
        end
    end
    t.stopframe = math.floor(t.secs * t.rate)
    if type(t.amplitude) ~= 'function' then 
            local ampfactor = t.amplitude
            t.amplitude = function(f) return ampfactor end
        end
    fn = function(buf)
        for i=1, buf.frames do
            if t.totframes >= t.stopframe then return end
            t.totframes = t.totframes + 1
            t.phase = t.phase + (t.totframes / t.rate * t.factor) % 360
            buf[1][i] = math.sin(t.phase) * t.amplitude(t.totframes)
        end
    end
    return fn
end

function dsp.chirp_log(...)
    local tick = 1
    local phase = 0
    local rate = 44100
    local freq = 100
    local totframes = 0
    local fn = function(buf)
        for i=1, buf.frames do
            totframes = totframes + 1
            phase=phase + math.log((totframes / rate * freq) %360, 20)
            buf[1][i] = math.sin(phase)
        end
    end
    return fn 
end

function dsp.sineosc(...)
   local t = {tick = 1,
        phase = 0,
        rate = 44100,
        freq = 440,
        totframes = 0,
        amplitude = 0.5,
        secs = 1.0,
    }
    for key, _ in pairs(t) do
        for __, val in pairs(...) do
            if __ == key then
                t[key] = val
            end
        end
    end
    t.phase = t.phase % 360
    t.stopframe = math.floor(t.secs * t.rate)
    if type(t.amplitude) ~= 'function' then 
            local ampfactor = t.amplitude
            t.amplitude = function(f) return ampfactor end
        end
    fn = function(buf)
        local rads = math.pi / 180
        local angle = 0
        local _ = 0
        for i=1, buf.frames do
            if t.totframes >= t.stopframe then return end
            t.totframes = t.totframes + 1
            angle = t.phase + ((360 / (t.rate / t.freq)) * t.totframes)
            _ = buf[1][i]
            buf[1][i] = _ + math.sin(angle * rads) * t.amplitude(t.totframes)
        end
    end
    return fn
end

