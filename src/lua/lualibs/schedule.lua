function dsp.time()
    --return the current time assumed by the dsp as tick start and tick end
    local tick = dsp.getdsptick(_dsp)
    local rate = dsp.getdsprate(_dsp)
    local frames = dsp.getdspframes(_dsp)
    local start_time = tick * frames / rate
    local end_time = (tick + 1) * frames / rate
    return start_time, end_time
end

do
    local schedule = {
        events = {},
        at = function(self, fn, starts, stops, repeats, period)
        --This module implements scheduling
            --TODO
            local starts = tonumber(starts) or error("must supply a valid timestamp", 2)
            local stops = tonumber(stops) or math.huge
            local repeats = repeats or 0
            local period = period and period > 0 or false
            self.events[#self.events + 1] = {
                time={
                    starts=starts,
                    stops=stops
                },
                repeats=repeats,
                period=period,
                fn=fn
            }
            --every event gets given an integer id
            return #self.events + 1
        end,

        registercallbackattick = function(self, callback, tick)
        end,

        is_current = function(self, ev)
        end,

        current_events = function(self)
            local start, fin = dsp.time()
            local t = {}
            for k, v in ipairs(self.events) do
                if time.starts < start and time.ends < fin then
                    t[#t + 1] = v
                elseif v.time.starts >= start and v.time.starts < fin then
                    print("adding event")
                    t[#t + 1] = v
                end
            end
            return t
        end,

        clearexpired = function(self)
            local start, fin = dsp.time()
            for k, v in ipairs(self.events) do
                if v.time.starts < start then
                    if v.period > 0 then

                    end
                end
            end
        end,

        update = function(self)
            local events = self:current_events()
            for k, v in pairs(events) do
                    print("schedule.update" .. tostring(#events))
                    for _k, _v in pairs(v) do print(_k, _v) end
                v.fn()
            end
        end
    }

    dsp.schedule = setmetatable(schedule, {
        __call = function(self, ...)
            return self:at(unpack(...))
        end
    })
end
