dsp = dsp or {}
do
local osc = {}
function osc.new(...)
--[[
This function is a closure factory that creates an oscillator closure.
It returns a function that can be run with a buffer as its first argument.
The oscillator will write into the buffer the correct number of samples with
the expected phase-alignment for the current tick.
osc.new() > out
]]
    --set defaults where not given
    local args = ... or {}
    local t = {
        shape='sine',
        phase=0,
        freq=440,
        amplitude=0.5,
        tick=1,
        osc=nil
    }
    for key, _ in pairs(t) do
        for __, val in pairs(args) do
            if __ == key then
                t[key] = val
            end
        end
    end
    t.osc = assert((
        {
            sine=dsp.osc_sine,
            saw=dsp.osc_saw,
            square=dsp.osc_square
        })[t.shape],
        "oscillator not selected"
    )

    --define the closure function
    return function(buf)
        t.phase = t.osc(
            buf.buffer,
            buf.channels,
            buf.frames,
            t.freq,
            t.phase,
            t.amplitude,
            buf.rate
        )
        t.tick = t.tick + 1
    end
end

function osc.chirp(...)
    local params = ...
    params.curve = params.curve or 'log'

    local function chirp(...)
        local t = {tick = 1,
                phase = 0,
                rate = 44100,
                factor = 0.001,
                totframes = 0,
                amplitude = 0.5,
                secs = 1.0,
                low = 1,
                high = 20000,
                channels = {}
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

        local function linfn(buf)
            for i=1, buf.frames do
                if t.totframes >= t.stopframe then return end
                t.totframes = t.totframes + 1
                t.phase = t.phase + (t.totframes / t.rate * t.factor) % 360
                local val = math.sin(t.phase) * t.amplitude(t.totframes)
                --write the samples into the masked channels list
                for _, ch in ipairs(t.channels) do
                    buf[ch][i] = val
                end
            end
        end

        local function logfn(buf)
            for i=1, buf.frames do
                t.totframes = t.totframes + 1
                t.phase=t.phase + math.log((t.totframes / t.rate * t.freq) % 360, 20)
                buf[1][i] = math.sin(phase)
            end
        end

        if params.curve == 'log' then return logfn end
        if params.curve == 'lin' then return linfn end
        return error("unknown curve type for chirp", 3)
    end

    return chirp(...)
end

function osc.sine(...)
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

setmetatable(osc, {
    __call = function(self, ...)
        return self.new(...)
    end
})
dsp.osc = osc
end
