dsp.schedule = {
    events = {},
    at = function(self, fn, starts, stops, repeats, period )
    --This module implements scheduling
        --TODO
        local starts = tonumber(starts) or error("must supply a valid timestamp", 2)
        local stops = tonumber(stops) or math.huge
        local repeats = repeats or 0
        local period = period and period > 0 or false
--        local fn = type(fn) == 'function'  or getmetatable(fn).__call and fn or error("fn must be callable", 2)
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
    end
,  
    registercallbackattick = function(self, callback, tick)

    end
,
    is_current = function(self, ev)
        
    
    end
,
    current_events = function(self)
--        print("CURRENT EVENTS")
        local start, fin = dspticktimewindow()
--        print("dsptickdone")
        local t = {}
        for k, v in ipairs(self.events) do
--            print("loop")
            if time.starts < start and time.ends < fin then
                t[#t + 1] = v
            elseif v.time.starts >= start and v.time.starts < fin then
                print("adding event")
                t[#t + 1] = v
            end
--            print("CURRENT EVENTS return ")
        end
--        print("CURRENT EVENTS return ")
        return t 
    end
,
    clearexpired = function(self)
        local start, fin = dspticktimewindow()
        for k, v in ipairs(self.events) do
            if v.time.starts < start then
                if v.period > 0 then
                
                end
            end
        end 
    end
,
    update = function(self)
        local events = self:current_events()
        for k, v in pairs(events) do
                print("schedule.update" .. tostring(#events))
                for _k, _v in pairs(v) do print(_k, _v) end
            v.fn()
        end
        
    end

}

function dsptick()
    return dsp.getdsptick(this_dsp_pointer)
end

function dspframes()
    return dsp.getdspframes(this_dsp_pointer)
end

function dsprate()
    return dsp.getdsprate(this_dsp_pointer)
end

function dspticktimewindow()
    local tick = dsptick()
    local start_time = tick * dspframes() / dsprate()
    local end_time = (tick + 1) * dspframes() / dsprate()
    return start_time, end_time
end
--event = {
--    start=0,
--    stop=0,
--    period=0
--    payload=

--}

