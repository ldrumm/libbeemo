dsp.debug = {
    pprint = function (self, t, indent)
        indent = indent or 0
        if indent == 4 then return end
--        print('indent:' .. tostring(indent))
        local indent_string = string.rep('\t', indent)
        
        for k, v in pairs(t) do
            if type(v) == 'table' then
                if k ~= v then
                    print(indent_string .. tostring(k).. ':' ..tostring(v))
                    self:pprint(v, indent+1)
                end
            else 
                print(indent_string .. tostring(k) .. ':' .. tostring(v))
            end
        end
    end
    ,
    bufprint = function(dsp, verbosity)
        print(dsp)
        local verbosity = verbosity or 1

        for k, v in pairs(dsp) do
            print(tostring(k) .. ':' .. tostring(v))
        end
        if verbosity > 1 then
            for i=1, dsp.frames do
                io.write('\n')
                for ch=1, dsp.channels do
                    io.write(tostring(dsp[ch][i]) .. ',\t')
                end
            end
            io.write('\n')
            io.flush()
        end
    end
}

