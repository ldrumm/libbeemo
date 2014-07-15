
mix_operator_tests = TestingUnit{

    test_mix_to_sys_buffer_operator = function(self)
        local out = dsp.sys_buf("out")
        local tmp = dsp.temp_buf(out.channels, out.frames)
        --set everything to zero
        dsp.zero(out)
        dsp.zero(tmp)
        local vals = {}
        for i=1, out.frames do
            vals[i] = math.random()
        end
        for ch = 1, out.channels do
            for f = 1, out.frames do
                out[ch][f] = vals[f]
                tmp[ch][f]= vals[f]
            end
        end
        
        --mix tmp into out using the `>` operator
        _ = tmp > out
        for ch = 1, out.channels do
            for f = 1, out.frames do
                self:assert_almost_equal(out[ch][f], vals[f] * 2)
            end
        end
        
    end,
}
