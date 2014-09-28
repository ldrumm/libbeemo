function dsp.new_buffer(template)
    local buffer = template or error("template required to instantiate dsp buffer", 2)
    local raw_dsp_obj = template.raw_dsp_obj
    local dsp_prototype = {
        rate = buffer.rate or 44100, --FIXME
        buffer = buffer.rawbuf or dsp.getbufferpointer(
            raw_dsp_obj, 
            buffer.name
        ) or error('', 2),
        channels = buffer.channels or dsp.getdspchannels(raw_dsp_obj) or error('', 2),
        frames = buffer.frames or dsp.getdspframes(raw_dsp_obj) or error('', 2),
        shallow_copy = function(self) 
            local t = {}
            for k, v in pairs(self) do
                t[k] = v
            end
            return t
        end
        ,
        seekable = false,
        seekable_len = 0,
        read = function(self, frames) --[[TODO--]] end,
        seek = function(self, offset, whence)--[[TODO--]]end,
        tell = function(self)--[[TODO--]]end,
        frameget = function(self, ch, idx)     
            if ch > self.channels then error("out of bounds access", 2) end 
            if idx > self.frames then error("out of bounds access", 2) end
            
            return dsp.get(self.buffer, ch, idx) 
        end,
        frameset = function(self, ch, idx, val)     
            if ch > self.channels then error("out of bounds access", 2) end 
            if idx > self.frames then error("out of bounds access", 2) end

            return dsp.set(self.buffer, ch, idx, val) 
        end,
        zero = function(self) 
            return dsp.zero_buffer(self.buffer, self.channels, self.frames) 
        end,
        mix = function(self, buf) 
            return dsp.mix_buffers(self.buffer, 
                self.buffer, 
                buf.buffer, 
                self.channels, 
                self.frames) 
        end,
        bufferset = function(self, buf) 
            error('not implemented')
            return dsp.set_buffers(self.buffer, 
                buf.buffer, 
                self.channels, 
                self.frames, 
                buf.channels, 
                buf.frames) 
        end
        ,
        channel_idx = 0,
    --[[because we need to use dual indexes (buffer[channel][frame]), 
        the table index metamethod does not work without some trickery. 
        The first dimensional index (channel) needs to return an object whose 
        __index method already has the channel defined. 
        Then we can return / set the frame needed in a more natural manner
        We make some assumptions on the use of this table:
        -all numeric indexes into the buffer are assumed to be /channel/sample indexes
        -all other indexes (hash table lookups) should behave as usual
        
        channel_idx is used as a flag for multidimensional indexing.  
            On first index i.e. 
               val = t[2] 
            channel_idx is set to 2 and t is returned.
            on second index i.e. 
              val = t[2][50]
              The expected sample is returned
              TODO
              it is yet to be decided whether a one dimensional lookup for a 
              mono buffer should skip the middle man and return the frame value
            --[[TODO
        An interesting behaviour would be to overload an infix operator such as 
        greater than(>), concat(..) or pow(^) to allow redirection from one dsp object to another.
        This would emulate one of the nice features of chuck and avoid lots of nested function calls.
--]]
    --]]
    }
--for this tables metamethods
    local mt = {
        __index = function(t, k)
            if type(k) ~= "number" then
                return rawget(t, k)
            end
            if t.channel_idx == 0 then
                t.channel_idx = k
                return t
            end
            local ch = t.channel_idx
            t.channel_idx = 0
            return t:frameget(ch, k)  
        end
    ,
        __newindex = function(t, k, v)
            if type(k) ~= "number" then
                return rawset(t, k, v)
            end
            if t.channel_idx == 0 then
                error("cant set channel index")
            else
                local ch = t.channel_idx
                t.channel_idx = 0
                return t:frameset(ch, k, v)
            end
        end
    ,
        __lt = function(t, cmp)
            local _ = type(t)
            if  _ == 'table' then
                t:mix(cmp)
            elseif _ == 'function' then
                t(cmp)
            end
        end
    ,
        __call = function(t, ...)
        --mix all the given arguments buffers together
            for k, v in pairs(...) do
                if type(v) == 'table' then
                    t:mix(v.buffer)
                elseif type(v) == 'function'  then
                    v(t)
                else
                    error("buffer or function object expected, received:" .. type(v), 2)
                end
            end
        end
    ,
        __tostring = function(self)
            return "DSP object:[" .. tostring(self.channels) .. "][" .. tostring(self.frames) .. "]"
        end
    }
    return setmetatable(dsp_prototype, mt)
end

function dsp.temp_buf(channels, frames)
    frames = frames or error("frames parameter expected", 2)
    channels = channels or error("channels parameter expected", 2)
    local buf = dsp.new_buffer{
        frames=frames,  
        channels=channels, 
        rawbuf=dsp.gettempbuf(channels, frames)
    }
    local mt = getmetatable(buf)
    mt.__gc = function(self) dsp.gctempbuf(self.buffer, self.channels) end
    return setmetatable(buf, mt)
end

function dsp.sys_buf(name)
    name = name or "out"
    local _ = {output="out", input="in", control="ctl"}
    name = _[name] or name
    return dsp.new_buffer{raw_dsp_obj=_dsp, name=name}
end

function dsp.buf_from(t, ch, frames)
    --TODO
end

