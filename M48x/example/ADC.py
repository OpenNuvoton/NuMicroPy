from pyb import ADC, Pin, ADCALL

adc0 = ADC(Pin.board.A4)
val = adc0.read()

adc_all = ADCALL(12, 0x60000) #0x60000: enable channal 17 (core tempture) and channel 18 (battery voltage)
adc_all.read_core_temp()
adc_all.read_core_vbat()
