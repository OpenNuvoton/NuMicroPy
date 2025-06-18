from pyb import ADC, Pin, ADCALL

adc0 = ADC(Pin.board.A4)
val = adc0.read()
print(val)

#0x6000000: enable channal 25 (core tempture) and channel 26 (battery voltage)
adc_all = ADCALL(12, 0x6000000)
adc_all.read_core_temp()
adc_all.read_core_vbat()
