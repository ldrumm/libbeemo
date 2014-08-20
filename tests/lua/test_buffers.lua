buffer_tests = TestingUnit{
    
    fixtures = {
        test_new_buffer_len = (function() 
            local ch ={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
            local f = {16,32,64,128,256,512,1024,2048,4096}
            local t = {}
            for _, channels in ipairs(ch) do
                for _, frames in ipairs(f) do
                    t[#t+1] = {channels, frames}
                end
            end
            return t
        end
        )(),
    },
    
    test_buffer_table_call = function(self)
        
    
    end,
    
    test_new_buffer_zero = function(self)
        local buf = dsp.temp_buf(1, 1024)
        for ch=1, buf.channels do
            for f=1, buf.frames do
                self:assert_equal(buf[ch][f], 0)
            end
        end
    end,
    
    test_new_buffer_len = function(self, channels, frames)
        local buf = dsp.temp_buf(channels, frames)
        self:assert_equal(buf.channels, channels)
        self:assert_equal(buf.frames, frames)
    end,
    
    test_sys_buf_pointer_not_equal = function(self)
        local out = dsp.sys_buf("out")
        local input  = dsp.sys_buf("in")
        local ctl = dsp.sys_buf("ctl")
        local t = {}
        t[ctl] = ctl
        t[input] = input
        t[out] = out
        local vals = 0
        for k, v in pairs(t) do vals = vals + 1 end
        self:assert_equal(vals, 3)
    end,
    
}
