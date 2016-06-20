dsp = dsp or {}
do

function dsp.closure(closure_t, fn)
    local t = closure_t or {}
    local func = function() fn(t) end
    return setmetatable(t, {__call = func})
end

end