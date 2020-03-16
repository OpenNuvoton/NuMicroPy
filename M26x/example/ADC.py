from pyb import ADC, Pin

adc0 = ADC(Pin.board.A4)
val = adc0.read()

adc_all = ADCALL(12, 0x00003)
val = adc.read_channel(1)
val = adc.read_core_temp()
val = adc.read_core_vbat()
