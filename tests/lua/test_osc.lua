------print("doing user script")
------output.rate = 44100
----osc = new_osc{shape='sine', freq=220.5, amplitude=0.8, phase=0}
--------osc2 = new_osc{shape='square', freq=100.06640625, amplitude=.2}
--------osc3 = new_osc{shape='saw', freq=440.06640625, amplitude=.1}


--fundamental = 50
--series = {}
--partials = 1
--power = 0.8
--for i=1, partials do
--    series[i] = {
--            --add some random deviation from the perfect frequency for nice times
--            freq=(fundamental * i) + math.random() - 0.5,
--            --reduce power as we go up the harmonic series
--            amplitude = enveloper(100000, 0.005 * math.random() - 0.5),
--            phase = math.random() * 10 - 5
--        }
--end
--oscs = {}
--for i=1, partials do
--    oscs[i] = sineosc(series[i])
--end

----for i=1, 1024 do
----    oscs[i] = sineosc{freq=(math.random() * math.log(20000) *100), amplitude=math.random()}
----end

--output = sys_buf("out")
--output.name = 'output'
----mybuf = temp_buf(1, 1024)
----mybuf.name = 'mybuf'
----vol={}
----for i=1, 1000 do
----    vol[i] =  gain(3)
----end
----vol.a = mybuf
----vol.b = osc
----vol.z = zero
----for i=1, 1024 do
----    mybuf[1][i] = math.sin(i*0.1)
----end
----s = chirp_lin{}

----schedule:at(function()osc(output) end, 3.2)
--a = sineosc{phase=90, freq=10}
--function dspmain()
--    output:zero()
------    print(output)
------    print(mybuf)
------    debug.debug()
------    zero(mybuf)
----    s(output)
----    i =1
----    while i <= output.frames do
----        print(output[1][i])
----        i = i + 1
----    end
----    _ = mybuf > output
--    for i=1, # oscs do
--        oscs[i](output)
--    end
----  _ =   a (output)
----    output{oscs}
----    debug.debug()
----        output{a}
--end
----debug.debug()
--print("done user script")

a = TestingUnit{
    fixtures = {
        test_a = (function()
            local t = {}
            for i=1, 100000 do
                t[#t+1] = {math.random()}
            end
            return t
            end)(),
    },

    test_a = function(self, x)
        self:assert_truthy(2, x)
    end,
}
