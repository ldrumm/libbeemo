local function garbagebuf(channels, frames)
    local garbage = {channels=channels, frames=frames}
    for ch = 1, channels do
        garbage[ch] = {}
        for i = 1, frames do
            garbage[ch][i] = math.random()
        end
    end
    return garbage
end

buffer_tests = TestingUnit{
    assert_buf_data_equal = function(self, x, y)
        if x.channels ~= y.channels or x.frames ~= y.frames then
            return self:_assertion_failure{
                traceback=debug.traceback(2),
                err_str=[[channel or frame mismatch]],
                debug_info=debug.getinfo(2),
                args={x, y}
            }
        end
        for ch =1 , x.channels do
            for f = 1, x.frames do
                self:assert_almost_equal(x[ch][f], y[ch][f])
            end
        end
    end,

    fixtures = {
        test_new_buffer_len = (
            function()
                local ch ={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}
                local f = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096}
                local t = {}
                for _, channels in ipairs(ch) do
                    for _, frames in ipairs(f) do
                        t[#t+1] = {channels, frames}
                    end
                end
                return t
            end
        )(),
        test_new_buffer_zero = (
            function()
                local f={}
                for i = 1, 10 do
                    f[i] = {i}
                end
                return f
            end
        )(),
    },

    test_buffer_table_call = function(self)
    --[[
        Buffers can be be called with a function or table argument.
        The contents of a table argument should be copied into the callee's
        internal buffer.
        A function argument must be called with the buffer table as its only
        argument
    ]]
        local buf = dsp.sys()
        buf:zero()

        local garbage = garbagebuf(buf.channels, buf.frames)

        local function write_garbage(buf)
            for ch in ipairs(garbage) do
                for f = 1, buf.frames do
                    buf[ch][f] = garbage[ch][f]
                end
            end
        end
        --buf should now call this function and be filled with garbage
        buf{write_garbage}
        self:assert_buf_data_equal(buf, garbage)
        self:assert_calls(function() buf{write_garbage} end, write_garbage, 1)
        buf:zero()
        write_garbage(buf)
        self:assert_buf_data_equal(buf, garbage)
    end,

    test_new_buffer_zero = function(self)
        -- New buffers should be initialized to all samples zero
        local buf = dsp.temp_buf(1, 1024)
        for ch = 1, buf.channels do
            for f = 1, buf.frames do
                self:assert_equal(buf[ch][f], 0)
            end
        end
    end,

    test_new_buffer_len = function(self, channels, frames)
        local buf = dsp.temp_buf(channels, frames)
        self:assert_equal(buf.channels, channels)
        self:assert_equal(buf.frames, frames)
    end,

    test_sys_pointer_not_equal = function(self)
        local out = dsp.sys("out")
        local input  = dsp.sys("in")
        local ctl = dsp.sys("ctl")
        local t = {}
        t[ctl] = ctl
        t[input] = input
        t[out] = out
        local vals = 0
        for k, v in pairs(t) do vals = vals + 1 end
        self:assert_equal(vals, 3)
    end,
}
