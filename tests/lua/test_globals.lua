global_functions = TestingUnit{
    fixtures = {
        test_global_exists={
            {_dsp},
            {dsp},
        },
        test_obj_params = {
            {'getdspchannels', 'BMO_CHANNELS'},
            {'getdsprate', 'BMO_RATE'},
            {'getdspframes', 'BMO_FRAMES'}
        },
    },
    test_global_exists = function(self, arg)
        self:assert_true(arg ~= nil)
    end,

    test_obj_params = function(self, func_name, env_var)
        --[[
            The DSP object's frames, channels, rate should obviously match the C
            environment.  This tests that assertion.
        ]]
        local expected_val = tonumber(os.getenv(env_var))
        if expected_val == nil then
            return self:fail("The test runner did not detect an expected environment variable")
        end
        self:assert_equal(dsp[func_name](_dsp), expected_val)
    end,
}
